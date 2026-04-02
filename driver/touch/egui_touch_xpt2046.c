#if EGUI_DRIVER_TOUCH_XPT2046_ENABLE

/**
 * @file egui_touch_xpt2046.c
 * @brief XPT2046 resistive touch driver implementation
 *
 * Uses Panel IO for SPI communication.
 * XPT2046 protocol: send 1-byte command, read 2-byte ADC result.
 *   rx_param(io, cmd, buf, 2) -> sends cmd byte, reads 2 bytes response
 *   tx_param(io, cmd, NULL, 0) -> sends cmd byte only (for power down)
 * The Panel IO SPI implementation handles CS automatically.
 * DC pin should be NULL in the Panel IO config for XPT2046.
 */

#include "egui_touch_xpt2046.h"
#include <string.h>
#include "core/egui_api.h"

/* XPT2046 Control byte commands (8-bit) */
#define XPT2046_CMD_X_POS      0xD0 /* Read X position (12-bit, differential) */
#define XPT2046_CMD_Y_POS      0x90 /* Read Y position (12-bit, differential) */
#define XPT2046_CMD_Z1         0xB0 /* Read Z1 (pressure) */
#define XPT2046_CMD_Z2         0xC0 /* Read Z2 (pressure) */
#define XPT2046_CMD_POWER_DOWN 0x00 /* Power down between conversions */

/* ADC resolution */
#define XPT2046_ADC_MAX 4095

/* Default pressure threshold */
#define XPT2046_DEFAULT_PRESSURE_THRESHOLD 100

/* Helper: reset */
static int xpt2046_reset(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
        egui_api_delay(10);
        self->set_rst(1);
        egui_api_delay(50);
    }
    return 0;
}

/* Helper: SPI transfer (send command, receive 12-bit result) via Panel IO */
static uint16_t xpt2046_read_adc(egui_hal_touch_driver_t *self, uint8_t cmd)
{
    uint8_t rx_buf[2] = {0, 0};

    /* rx_param: send cmd byte, read 2 bytes response */
    self->io->rx_param(self->io, (int)cmd, rx_buf, 2);

    /* Extract 12-bit ADC value (bits 11:0 from 16-bit response) */
    uint16_t value = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
    value >>= 3; /* Shift to get 12-bit value */
    value &= 0x0FFF;

    return value;
}

/* Helper: read with averaging for noise reduction */
static uint16_t xpt2046_read_adc_avg(egui_hal_touch_driver_t *self, uint8_t cmd, uint8_t samples)
{
    uint32_t sum = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        sum += xpt2046_read_adc(self, cmd);
    }

    return (uint16_t)(sum / samples);
}

/* Helper: check if touch is pressed using pressure measurement */
static int xpt2046_is_pressed(egui_hal_touch_driver_t *self)
{
    egui_touch_xpt2046_priv_t *priv = (egui_touch_xpt2046_priv_t *)self->priv;
    uint16_t z1, z2;

    (void)(z2); /* In case pressure threshold is not used */

    z1 = xpt2046_read_adc(self, XPT2046_CMD_Z1);
    z2 = xpt2046_read_adc(self, XPT2046_CMD_Z2);

    /* Calculate pressure: lower z1 and higher z2 means more pressure */
    /* Simple threshold check on z1 */
    if (z1 > priv->pressure_threshold && z1 < (XPT2046_ADC_MAX - priv->pressure_threshold))
    {
        return 1;
    }

    return 0;
}

/* Helper: map raw ADC value to screen coordinate */
static int16_t xpt2046_map_coordinate(int16_t raw, int16_t raw_min, int16_t raw_max, int16_t screen_size)
{
    int32_t range = raw_max - raw_min;
    if (range == 0)
    {
        return 0;
    }

    int32_t coord = ((int32_t)(raw - raw_min) * screen_size) / range;

    /* Clamp to valid range */
    if (coord < 0)
    {
        coord = 0;
    }
    if (coord >= screen_size)
    {
        coord = screen_size - 1;
    }

    return (int16_t)coord;
}

/* Driver: init */
static int xpt2046_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    egui_touch_xpt2046_priv_t *priv = (egui_touch_xpt2046_priv_t *)self->priv;

    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

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

/* Driver: del */
static void xpt2046_del(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
}

/* Driver: read */
static int xpt2046_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    egui_touch_xpt2046_priv_t *priv = (egui_touch_xpt2046_priv_t *)self->priv;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));

    /* Check if touch is pressed */
    if (!xpt2046_is_pressed(self))
    {
        return 0; /* No touch */
    }

    /* Read raw coordinates with averaging */
    uint16_t raw_x = xpt2046_read_adc_avg(self, XPT2046_CMD_X_POS, 4);
    uint16_t raw_y = xpt2046_read_adc_avg(self, XPT2046_CMD_Y_POS, 4);

    /* Map to screen coordinates */
    int16_t x = xpt2046_map_coordinate(raw_x, priv->cal.x_min, priv->cal.x_max, self->config.width);
    int16_t y = xpt2046_map_coordinate(raw_y, priv->cal.y_min, priv->cal.y_max, self->config.height);

    /* Store point */
    data->points[0].x = x;
    data->points[0].y = y;
    data->points[0].id = 0;
    data->points[0].pressure = 0; /* Could calculate from Z1/Z2 if needed */
    data->point_count = 1;

    return 0;
}

/* Internal: setup driver function pointers */
static void xpt2046_setup_driver(egui_hal_touch_driver_t *driver, egui_touch_xpt2046_priv_t *priv, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level),
                                 void (*set_int)(uint8_t level), uint8_t (*get_int)(void))
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));
    memset(priv, 0, sizeof(egui_touch_xpt2046_priv_t));

    driver->name = "XPT2046";
    driver->max_points = 1;

    driver->reset = xpt2046_reset;
    driver->init = xpt2046_init;
    driver->del = xpt2046_del;
    driver->read = xpt2046_read;

    driver->io = io;
    driver->set_rst = set_rst;
    driver->set_int = set_int;
    driver->get_int = get_int;
    driver->priv = priv;
}

/* Public: init (static allocation) */
void egui_touch_xpt2046_init(egui_hal_touch_driver_t *storage, egui_touch_xpt2046_priv_t *priv_storage, egui_panel_io_handle_t io,
                             void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level), uint8_t (*get_int)(void))
{
    if (!storage || !priv_storage || !io || !io->rx_param || !io->tx_param)
    {
        return;
    }

    xpt2046_setup_driver(storage, priv_storage, io, set_rst, set_int, get_int);
}

/* Public: set calibration */
void egui_touch_xpt2046_set_calibration(egui_hal_touch_driver_t *driver, const egui_touch_xpt2046_calibration_t *cal)
{
    if (!driver || !driver->priv || !cal)
    {
        return;
    }

    egui_touch_xpt2046_priv_t *priv = (egui_touch_xpt2046_priv_t *)driver->priv;
    memcpy(&priv->cal, cal, sizeof(egui_touch_xpt2046_calibration_t));
}

/* Public: set pressure threshold */
void egui_touch_xpt2046_set_pressure_threshold(egui_hal_touch_driver_t *driver, uint16_t threshold)
{
    if (!driver || !driver->priv)
    {
        return;
    }

    egui_touch_xpt2046_priv_t *priv = (egui_touch_xpt2046_priv_t *)driver->priv;
    priv->pressure_threshold = threshold;
}

#endif /* EGUI_DRIVER_TOUCH_XPT2046_ENABLE */
