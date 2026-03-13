/**
 * @file egui_touch_xpt2046.c
 * @brief XPT2046 resistive touch driver implementation
 */

#include "egui_touch_xpt2046.h"
#include <string.h>

/* XPT2046 Control byte commands (8-bit) */
#define XPT2046_CMD_X_POS       0xD0  /* Read X position (12-bit, differential) */
#define XPT2046_CMD_Y_POS       0x90  /* Read Y position (12-bit, differential) */
#define XPT2046_CMD_Z1          0xB0  /* Read Z1 (pressure) */
#define XPT2046_CMD_Z2          0xC0  /* Read Z2 (pressure) */
#define XPT2046_CMD_POWER_DOWN  0x00  /* Power down between conversions */

/* ADC resolution */
#define XPT2046_ADC_MAX         4095

/* Default pressure threshold */
#define XPT2046_DEFAULT_PRESSURE_THRESHOLD  100

static void xpt2046_spi_wait_complete(egui_hal_touch_driver_t *self)
{
    if (self->bus.spi->wait_complete) {
        self->bus.spi->wait_complete();
    }
}

/* Helper: SPI transfer (send command, receive 12-bit result) */
static uint16_t xpt2046_read_adc(egui_hal_touch_driver_t *self, uint8_t cmd)
{
    uint8_t tx_buf[3];
    uint8_t rx_buf[3];

    tx_buf[0] = cmd;
    tx_buf[1] = 0x00;
    tx_buf[2] = 0x00;

    /* Assert CS if available */
    if (self->gpio && self->gpio->set_cs) {
        self->gpio->set_cs(0);
    }

    /* Send command and read response */
    self->bus.spi->write(tx_buf, 1);
    xpt2046_spi_wait_complete(self);
    if (self->bus.spi->read) {
        self->bus.spi->read(rx_buf, 2);
    } else {
        rx_buf[0] = 0;
        rx_buf[1] = 0;
    }

    /* Deassert CS */
    if (self->gpio && self->gpio->set_cs) {
        self->gpio->set_cs(1);
    }

    /* Extract 12-bit ADC value (bits 11:0 from 16-bit response) */
    uint16_t value = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
    value >>= 3;  /* Shift to get 12-bit value */
    value &= 0x0FFF;

    return value;
}

/* Helper: read with averaging for noise reduction */
static uint16_t xpt2046_read_adc_avg(egui_hal_touch_driver_t *self, uint8_t cmd, uint8_t samples)
{
    uint32_t sum = 0;

    for (uint8_t i = 0; i < samples; i++) {
        sum += xpt2046_read_adc(self, cmd);
    }

    return (uint16_t)(sum / samples);
}

/* Helper: check if touch is pressed using pressure measurement */
static int xpt2046_is_pressed(egui_hal_touch_driver_t *self)
{
    egui_touch_xpt2046_priv_t *priv = (egui_touch_xpt2046_priv_t *)self->priv;
    uint16_t z1, z2;

    (void)(z2);  /* In case pressure threshold is not used */

    z1 = xpt2046_read_adc(self, XPT2046_CMD_Z1);
    z2 = xpt2046_read_adc(self, XPT2046_CMD_Z2);

    /* Calculate pressure: lower z1 and higher z2 means more pressure */
    /* Simple threshold check on z1 */
    if (z1 > priv->pressure_threshold && z1 < (XPT2046_ADC_MAX - priv->pressure_threshold)) {
        return 1;
    }

    return 0;
}

/* Helper: map raw ADC value to screen coordinate */
static int16_t xpt2046_map_coordinate(int16_t raw, int16_t raw_min, int16_t raw_max, int16_t screen_size)
{
    int32_t range = raw_max - raw_min;
    if (range == 0) {
        return 0;
    }

    int32_t coord = ((int32_t)(raw - raw_min) * screen_size) / range;

    /* Clamp to valid range */
    if (coord < 0) {
        coord = 0;
    }
    if (coord >= screen_size) {
        coord = screen_size - 1;
    }

    return (int16_t)coord;
}

/* Helper: transform coordinates based on config */
static void xpt2046_transform_point(egui_hal_touch_driver_t *self, int16_t *x, int16_t *y)
{
    int16_t tx = *x;
    int16_t ty = *y;

    /* Swap X/Y */
    if (self->config.swap_xy) {
        int16_t tmp = tx;
        tx = ty;
        ty = tmp;
    }

    /* Mirror X */
    if (self->config.mirror_x) {
        tx = self->config.width - 1 - tx;
    }

    /* Mirror Y */
    if (self->config.mirror_y) {
        ty = self->config.height - 1 - ty;
    }

    *x = tx;
    *y = ty;
}

/* Driver: init */
static int xpt2046_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    egui_touch_xpt2046_priv_t *priv = (egui_touch_xpt2046_priv_t *)self->priv;

    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    /* Initialize bus and GPIO */
    if (self->bus.spi->init) {
        self->bus.spi->init();
    }
    if (self->gpio && self->gpio->init) {
        self->gpio->init();
    }

    /* Set default calibration (full ADC range) */
    priv->cal.x_min = 0;
    priv->cal.x_max = XPT2046_ADC_MAX;
    priv->cal.y_min = 0;
    priv->cal.y_max = XPT2046_ADC_MAX;
    priv->pressure_threshold = XPT2046_DEFAULT_PRESSURE_THRESHOLD;

    /* Perform a dummy read to initialize the controller */
    xpt2046_read_adc(self, XPT2046_CMD_X_POS);

    return 0;
}

/* Driver: deinit */
static void xpt2046_deinit(egui_hal_touch_driver_t *self)
{
    if (self->bus.spi->deinit) {
        self->bus.spi->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

/* Driver: read */
static int xpt2046_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    egui_touch_xpt2046_priv_t *priv = (egui_touch_xpt2046_priv_t *)self->priv;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));

    /* Check if touch is pressed */
    if (!xpt2046_is_pressed(self)) {
        return 0;  /* No touch */
    }

    /* Read raw coordinates with averaging */
    uint16_t raw_x = xpt2046_read_adc_avg(self, XPT2046_CMD_X_POS, 4);
    uint16_t raw_y = xpt2046_read_adc_avg(self, XPT2046_CMD_Y_POS, 4);

    /* Map to screen coordinates */
    int16_t x = xpt2046_map_coordinate(raw_x, priv->cal.x_min, priv->cal.x_max, self->config.width);
    int16_t y = xpt2046_map_coordinate(raw_y, priv->cal.y_min, priv->cal.y_max, self->config.height);

    /* Transform coordinates */
    xpt2046_transform_point(self, &x, &y);

    /* Store point */
    data->points[0].x = x;
    data->points[0].y = y;
    data->points[0].id = 0;
    data->points[0].pressure = 0;  /* Could calculate from Z1/Z2 if needed */
    data->point_count = 1;

    return 0;
}

/* Driver: enter_sleep */
static void xpt2046_enter_sleep(egui_hal_touch_driver_t *self)
{
    /* Send power down command */
    uint8_t cmd = XPT2046_CMD_POWER_DOWN;

    if (self->gpio && self->gpio->set_cs) {
        self->gpio->set_cs(0);
    }

    self->bus.spi->write(&cmd, 1);
    xpt2046_spi_wait_complete(self);

    if (self->gpio && self->gpio->set_cs) {
        self->gpio->set_cs(1);
    }
}

/* Driver: exit_sleep */
static void xpt2046_exit_sleep(egui_hal_touch_driver_t *self)
{
    /* Perform a dummy read to wake up */
    xpt2046_read_adc(self, XPT2046_CMD_X_POS);
}

/* Internal: setup driver function pointers */
static void xpt2046_setup_driver(egui_hal_touch_driver_t *driver,
                                  egui_touch_xpt2046_priv_t *priv,
                                  const egui_bus_spi_ops_t *spi,
                                  const egui_touch_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));
    memset(priv, 0, sizeof(egui_touch_xpt2046_priv_t));

    driver->name = "XPT2046";
    driver->bus_type = EGUI_BUS_TYPE_SPI;
    driver->max_points = 1;

    driver->init = xpt2046_init;
    driver->deinit = xpt2046_deinit;
    driver->read = xpt2046_read;
    driver->set_rotation = NULL;  /* Use config swap/mirror instead */
    driver->enter_sleep = xpt2046_enter_sleep;
    driver->exit_sleep = xpt2046_exit_sleep;

    driver->bus.spi = spi;
    driver->gpio = gpio;
    driver->priv = priv;
}

/* Public: init (static allocation) */
void egui_touch_xpt2046_init(egui_hal_touch_driver_t *storage,
                              egui_touch_xpt2046_priv_t *priv_storage,
                              const egui_bus_spi_ops_t *spi,
                              const egui_touch_gpio_ops_t *gpio)
{
    if (!storage || !priv_storage || !spi || !spi->write || !spi->read) {
        return;
    }

    xpt2046_setup_driver(storage, priv_storage, spi, gpio);
}

/* Public: set calibration */
void egui_touch_xpt2046_set_calibration(egui_hal_touch_driver_t *driver,
                                         const egui_touch_xpt2046_calibration_t *cal)
{
    if (!driver || !driver->priv || !cal) {
        return;
    }

    egui_touch_xpt2046_priv_t *priv = (egui_touch_xpt2046_priv_t *)driver->priv;
    memcpy(&priv->cal, cal, sizeof(egui_touch_xpt2046_calibration_t));
}

/* Public: set pressure threshold */
void egui_touch_xpt2046_set_pressure_threshold(egui_hal_touch_driver_t *driver,
                                                uint16_t threshold)
{
    if (!driver || !driver->priv) {
        return;
    }

    egui_touch_xpt2046_priv_t *priv = (egui_touch_xpt2046_priv_t *)driver->priv;
    priv->pressure_threshold = threshold;
}
