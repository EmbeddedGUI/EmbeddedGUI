/**
 * @file egui_touch_stmpe610.c
 * @brief STMPE610 resistive touch driver implementation
 *
 * Uses Panel IO for SPI communication.
 * The STMPE610 SPI protocol encodes R/W in the address byte:
 *   Write: [0x00|addr, value]  -> tx_param(io, WRITE|addr, &value, 1)
 *   Read:  [0x80|addr] -> read -> rx_param(io, READ|addr, &value, 1)
 * The Panel IO SPI implementation handles CS automatically.
 * DC pin should be NULL in the Panel IO config for STMPE610.
 */

#include "egui_touch_stmpe610.h"
#include <string.h>
#include "core/egui_api.h"

/* STMPE610 Register addresses */
#define STMPE610_REG_CHIP_ID       0x00 /* Chip ID (should read 0x0811) */
#define STMPE610_REG_ID_VER        0x03 /* ID version */
#define STMPE610_REG_SYS_CTRL1     0x04 /* System control 1 */
#define STMPE610_REG_SYS_CTRL2     0x05 /* System control 2 */
#define STMPE610_REG_SPI_CFG       0x40 /* SPI configuration */
#define STMPE610_REG_INT_CTRL      0x41 /* Interrupt control */
#define STMPE610_REG_INT_EN        0x42 /* Interrupt enable */
#define STMPE610_REG_INT_STA       0x43 /* Interrupt status */
#define STMPE610_REG_ADC_CTRL1     0x4A /* ADC control 1 */
#define STMPE610_REG_ADC_CTRL2     0x4B /* ADC control 2 */
#define STMPE610_REG_TSC_DATA_X    0x4D /* Touch data X (16-bit) */
#define STMPE610_REG_TSC_DATA_Y    0x4F /* Touch data Y (16-bit) */
#define STMPE610_REG_TSC_DATA_Z    0x51 /* Touch data Z (8-bit) */
#define STMPE610_REG_TSC_DATA_XYZ  0x52 /* Touch data XYZ (combined) */
#define STMPE610_REG_TSC_FRACT_XYZ 0x56 /* Fractional XYZ */
#define STMPE610_REG_TSC_CTRL      0x57 /* Touch screen control */
#define STMPE610_REG_TSC_CFG       0x58 /* Touch screen configuration */
#define STMPE610_REG_FIFO_TH       0x59 /* FIFO threshold */
#define STMPE610_REG_FIFO_STA      0x5B /* FIFO status */
#define STMPE610_REG_FIFO_SIZE     0x5C /* FIFO size */
#define STMPE610_REG_TSC_I_DRIVE   0x5D /* Touch screen I drive */

/* STMPE610 SPI protocol */
#define STMPE610_SPI_READ      0x80 /* Read bit (set for read) */
#define STMPE610_SPI_WRITE     0x00 /* Write bit (clear for write) */
#define STMPE610_SPI_ADDR_MASK 0x7F /* Address mask */

/* STMPE610 Chip ID */
#define STMPE610_CHIP_ID 0x0811

/* System control 1 bits */
#define STMPE610_SYS_CTRL1_RESET     0x02 /* Soft reset */
#define STMPE610_SYS_CTRL1_HIBERNATE 0x0C /* Hibernate mode */

/* TSC control bits */
#define STMPE610_TSC_CTRL_EN  0x01 /* Enable TSC */
#define STMPE610_TSC_CTRL_XYZ 0x00 /* XYZ acquisition mode */
#define STMPE610_TSC_CTRL_STA 0x80 /* Touch status bit */

/* ADC/TSC configuration bits (aligned with ESP32 reference driver) */
#define STMPE610_ADC_CTRL1_10BIT    0x00
#define STMPE610_ADC_CTRL1_12BIT    0x08
#define STMPE610_ADC_CTRL1_96CLK    (0x06 << 4)
#define STMPE610_ADC_CTRL2_3_25MHZ  0x01
#define STMPE610_ADC_CTRL2_6_5MHZ   0x02
#define STMPE610_TSC_CFG_4SAMPLE    0x80
#define STMPE610_TSC_CFG_DELAY_1MS  0x20
#define STMPE610_TSC_CFG_SETTLE_5MS 0x04
#define STMPE610_TSC_I_DRIVE_50MA   0x01

/* Interrupt control bits */
#define STMPE610_INT_CTRL_POL_LOW 0x00
#define STMPE610_INT_CTRL_EDGE    0x02
#define STMPE610_INT_CTRL_ENABLE  0x01

/* FIFO status bits */
#define STMPE610_FIFO_STA_RESET 0x01 /* Reset FIFO */
#define STMPE610_FIFO_STA_EMPTY 0x20 /* FIFO empty */

/* ADC resolution */
#define STMPE610_ADC_MAX 4095

/* Default pressure threshold */
#define STMPE610_DEFAULT_PRESSURE_THRESHOLD 30

/* Helper: SPI write register via Panel IO */
static void stmpe610_write_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t value)
{
    /* STMPE610 write: cmd = WRITE|addr, param = value byte */
    int cmd = (int)(STMPE610_SPI_WRITE | (reg & STMPE610_SPI_ADDR_MASK));
    self->io->tx_param(self->io, cmd, &value, 1);
}

/* Helper: SPI read register (8-bit) via Panel IO */
static uint8_t stmpe610_read_reg8(egui_hal_touch_driver_t *self, uint8_t reg)
{
    uint8_t value = 0;
    /* STMPE610 read: cmd = READ|addr, read 1 byte */
    int cmd = (int)(STMPE610_SPI_READ | (reg & STMPE610_SPI_ADDR_MASK));
    self->io->rx_param(self->io, cmd, &value, 1);
    return value;
}

/* Helper: SPI read register (16-bit) via Panel IO */
static uint16_t stmpe610_read_reg16(egui_hal_touch_driver_t *self, uint8_t reg)
{
    uint8_t buf[2] = {0, 0};
    int cmd = (int)(STMPE610_SPI_READ | (reg & STMPE610_SPI_ADDR_MASK));
    self->io->rx_param(self->io, cmd, buf, 2);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

/* Helper: read one FIFO sample (XYZ data register is consumed on each access) */
static int stmpe610_read_fifo_sample(egui_hal_touch_driver_t *self, uint16_t *raw_x, uint16_t *raw_y, uint8_t *raw_z)
{
    uint8_t buf[4];

    for (uint8_t i = 0; i < 4; i++)
    {
        buf[i] = stmpe610_read_reg8(self, STMPE610_REG_TSC_DATA_XYZ);
    }

    *raw_x = (uint16_t)(((uint16_t)buf[0] << 4) | ((buf[1] >> 4) & 0x0F));
    *raw_y = (uint16_t)((((uint16_t)buf[1] & 0x0F) << 8) | buf[2]);
    *raw_z = buf[3];

    return 0;
}

/* Helper: check if touch is pressed */
static int stmpe610_is_pressed(egui_hal_touch_driver_t *self)
{
    uint8_t tsc_ctrl = stmpe610_read_reg8(self, STMPE610_REG_TSC_CTRL);
    return (tsc_ctrl & STMPE610_TSC_CTRL_STA) ? 1 : 0;
}

/* Helper: check if FIFO has data */
static int stmpe610_fifo_empty(egui_hal_touch_driver_t *self)
{
    uint8_t fifo_sta = stmpe610_read_reg8(self, STMPE610_REG_FIFO_STA);
    return (fifo_sta & STMPE610_FIFO_STA_EMPTY) ? 1 : 0;
}

/* Helper: reset FIFO */
static void stmpe610_fifo_reset(egui_hal_touch_driver_t *self)
{
    stmpe610_write_reg(self, STMPE610_REG_FIFO_STA, STMPE610_FIFO_STA_RESET);
    stmpe610_write_reg(self, STMPE610_REG_FIFO_STA, 0x00);
}

/* Helper: map raw ADC value to screen coordinate */
static int16_t stmpe610_map_coordinate(int16_t raw, int16_t raw_min, int16_t raw_max, int16_t screen_size)
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

/* Helper: reset */
static int stmpe610_reset(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
        egui_api_delay(10);
        self->set_rst(1);
        egui_api_delay(10);
    }
    return 0;
}

/* Helper: initialize STMPE610 hardware registers */
static void stmpe610_hw_init(egui_hal_touch_driver_t *self)
{
    stmpe610_write_reg(self, STMPE610_REG_SYS_CTRL1, STMPE610_SYS_CTRL1_RESET);

    /* Small delay for reset */
    egui_api_delay(5);

    /* Enable TSC and ADC clocks */
    stmpe610_write_reg(self, STMPE610_REG_SYS_CTRL2, 0x00);

    /* Configure ADC and touchscreen timings using the proven ESP32 reference values */
    stmpe610_write_reg(self, STMPE610_REG_TSC_CTRL, STMPE610_TSC_CTRL_EN | STMPE610_TSC_CTRL_XYZ);
    stmpe610_write_reg(self, STMPE610_REG_INT_EN, 0x01);
    stmpe610_write_reg(self, STMPE610_REG_ADC_CTRL1, STMPE610_ADC_CTRL1_10BIT | STMPE610_ADC_CTRL1_96CLK);
    stmpe610_write_reg(self, STMPE610_REG_ADC_CTRL2, STMPE610_ADC_CTRL2_6_5MHZ);

    /* Configure touch screen */
    stmpe610_write_reg(self, STMPE610_REG_TSC_CFG, STMPE610_TSC_CFG_4SAMPLE | STMPE610_TSC_CFG_DELAY_1MS | STMPE610_TSC_CFG_SETTLE_5MS);
    stmpe610_write_reg(self, STMPE610_REG_TSC_FRACT_XYZ, 0x06);

    /* Set FIFO threshold */
    stmpe610_write_reg(self, STMPE610_REG_FIFO_TH, 0x01);

    /* Reset FIFO */
    stmpe610_fifo_reset(self);

    /* Touch screen drive current */
    stmpe610_write_reg(self, STMPE610_REG_TSC_I_DRIVE, STMPE610_TSC_I_DRIVE_50MA);

    /* Clear any pending interrupts and keep controller state aligned with reference init */
    stmpe610_write_reg(self, STMPE610_REG_INT_STA, 0xFF);
    stmpe610_write_reg(self, STMPE610_REG_INT_CTRL, STMPE610_INT_CTRL_POL_LOW | STMPE610_INT_CTRL_EDGE | STMPE610_INT_CTRL_ENABLE);
}

/* Driver: init */
static int stmpe610_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    egui_touch_stmpe610_priv_t *priv = (egui_touch_stmpe610_priv_t *)self->priv;
    uint16_t chip_id;

    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    /* Set default calibration (full ADC range) */
    priv->cal.x_min = 0;
    priv->cal.x_max = STMPE610_ADC_MAX;
    priv->cal.y_min = 0;
    priv->cal.y_max = STMPE610_ADC_MAX;
    priv->pressure_threshold = STMPE610_DEFAULT_PRESSURE_THRESHOLD;

    /* Initialize hardware */
    stmpe610_hw_init(self);

    chip_id = stmpe610_read_reg16(self, STMPE610_REG_CHIP_ID);
    if (chip_id != STMPE610_CHIP_ID)
    {
        return -1;
    }

    return 0;
}

/* Driver: del */
static void stmpe610_del(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
}

/* Driver: read */
static int stmpe610_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    egui_touch_stmpe610_priv_t *priv = (egui_touch_stmpe610_priv_t *)self->priv;
    uint8_t fifo_count;
    uint32_t sum_x = 0;
    uint32_t sum_y = 0;
    uint32_t sum_z = 0;
    uint8_t valid_samples = 0;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));

    /* Check if touch is pressed */
    if (!stmpe610_is_pressed(self))
    {
        return 0; /* No touch */
    }

    /* Check if FIFO has data */
    if (stmpe610_fifo_empty(self))
    {
        return 0; /* No data */
    }

    fifo_count = stmpe610_read_reg8(self, STMPE610_REG_FIFO_SIZE);
    if (fifo_count == 0)
    {
        return 0;
    }

    for (uint8_t i = 0; i < fifo_count; i++)
    {
        uint16_t raw_x;
        uint16_t raw_y;
        uint8_t raw_z;

        if (stmpe610_read_fifo_sample(self, &raw_x, &raw_y, &raw_z) != 0)
        {
            stmpe610_fifo_reset(self);
            return -1;
        }

        sum_x += raw_x;
        sum_y += raw_y;
        sum_z += raw_z;
        valid_samples++;
    }

    stmpe610_fifo_reset(self);
    stmpe610_write_reg(self, STMPE610_REG_INT_STA, 0xFF);

    if (valid_samples == 0)
    {
        return 0;
    }

    uint16_t raw_x = (uint16_t)(sum_x / valid_samples);
    uint16_t raw_y = (uint16_t)(sum_y / valid_samples);
    uint8_t raw_z = (uint8_t)(sum_z / valid_samples);

    /* Check pressure threshold */
    if (raw_z < priv->pressure_threshold)
    {
        return 0; /* Pressure too low */
    }

    /* Map to screen coordinates */
    int16_t x = stmpe610_map_coordinate(raw_x, priv->cal.x_min, priv->cal.x_max, self->config.width);
    int16_t y = stmpe610_map_coordinate(raw_y, priv->cal.y_min, priv->cal.y_max, self->config.height);

    /* Store point */
    data->points[0].x = x;
    data->points[0].y = y;
    data->points[0].id = 0;
    data->points[0].pressure = raw_z;
    data->point_count = 1;

    return 0;
}

/* Internal: setup driver function pointers */
static void stmpe610_setup_driver(egui_hal_touch_driver_t *driver, egui_touch_stmpe610_priv_t *priv, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level),
                                  void (*set_int)(uint8_t level), uint8_t (*get_int)(void))
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));
    memset(priv, 0, sizeof(egui_touch_stmpe610_priv_t));

    driver->name = "STMPE610";
    driver->max_points = 1;

    driver->reset = stmpe610_reset;
    driver->init = stmpe610_init;
    driver->del = stmpe610_del;
    driver->read = stmpe610_read;

    driver->io = io;
    driver->set_rst = set_rst;
    driver->set_int = set_int;
    driver->get_int = get_int;
    driver->priv = priv;
}

/* Public: init (static allocation) */
void egui_touch_stmpe610_init(egui_hal_touch_driver_t *storage, egui_touch_stmpe610_priv_t *priv_storage, egui_panel_io_handle_t io,
                              void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level), uint8_t (*get_int)(void))
{
    if (!storage || !priv_storage || !io || !io->tx_param || !io->rx_param)
    {
        return;
    }

    stmpe610_setup_driver(storage, priv_storage, io, set_rst, set_int, get_int);
}

/* Public: set calibration */
void egui_touch_stmpe610_set_calibration(egui_hal_touch_driver_t *driver, const egui_touch_stmpe610_calibration_t *cal)
{
    if (!driver || !driver->priv || !cal)
    {
        return;
    }

    egui_touch_stmpe610_priv_t *priv = (egui_touch_stmpe610_priv_t *)driver->priv;
    memcpy(&priv->cal, cal, sizeof(egui_touch_stmpe610_calibration_t));
}

/* Public: set pressure threshold */
void egui_touch_stmpe610_set_pressure_threshold(egui_hal_touch_driver_t *driver, uint8_t threshold)
{
    if (!driver || !driver->priv)
    {
        return;
    }

    egui_touch_stmpe610_priv_t *priv = (egui_touch_stmpe610_priv_t *)driver->priv;
    priv->pressure_threshold = threshold;
}
