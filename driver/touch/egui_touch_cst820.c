/**
 * @file egui_touch_cst820.c
 * @brief CST820 capacitive touch driver implementation
 *
 * CST820 is an upgraded version of CST816S with improved performance.
 */

#include "egui_touch_cst820.h"
#include <string.h>
#include "core/egui_api.h"

/* CST820 I2C address (7-bit, shifted for HAL) */
#define CST820_ADDR             0x2A  /* 0x15 << 1 */

/* CST820 Registers */
#define CST820_REG_GEST_ID      0x00  /* Gesture ID */
#define CST820_REG_FINGER_NUM   0x01  /* Number of touch points */
#define CST820_REG_XH           0x02  /* X position high + event flag */
#define CST820_REG_XL           0x03  /* X position low */
#define CST820_REG_YH           0x04  /* Y position high + touch ID */
#define CST820_REG_YL           0x05  /* Y position low */
#define CST820_REG_CHIP_ID      0xA7  /* Chip ID register */
#define CST820_REG_SLEEP        0xE5  /* Sleep mode register */

/* Expected chip ID */
#define CST820_CHIP_ID          0xB7

/* Touch event flags */
#define CST820_EVENT_PRESS_DOWN 0x00
#define CST820_EVENT_LIFT_UP    0x01
#define CST820_EVENT_CONTACT    0x02
#define CST820_EVENT_NO_EVENT   0x03

/* Gesture IDs */
#define CST820_GEST_NONE        0x00
#define CST820_GEST_SLIDE_UP    0x01
#define CST820_GEST_SLIDE_DOWN  0x02
#define CST820_GEST_SLIDE_LEFT  0x03
#define CST820_GEST_SLIDE_RIGHT 0x04
#define CST820_GEST_SINGLE_CLICK 0x05
#define CST820_GEST_DOUBLE_CLICK 0x0B
#define CST820_GEST_LONG_PRESS  0x0C

/* Helper: hardware reset */
static void cst820_hw_reset(egui_hal_touch_driver_t *self)
{
    if (self->gpio && self->gpio->set_rst) {
        self->gpio->set_rst(0);
        /* Simple delay */
        egui_api_delay(5);
        self->gpio->set_rst(1);
        egui_api_delay(50);
    }
}

/* Helper: read register */
static int cst820_read_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t *data, uint16_t len)
{
    return self->bus.i2c->read_reg(CST820_ADDR, reg, data, len);
}

/* Helper: write register */
static int cst820_write_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t *data, uint16_t len)
{
    return self->bus.i2c->write_reg(CST820_ADDR, reg, data, len);
}

/* Helper: transform coordinates based on config */
static void cst820_transform_point(egui_hal_touch_driver_t *self, int16_t *x, int16_t *y)
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
static int cst820_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t chip_id;

    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    /* Initialize bus and GPIO */
    if (self->bus.i2c->init) {
        self->bus.i2c->init();
    }
    if (self->gpio && self->gpio->init) {
        self->gpio->init();
    }

    /* Hardware reset */
    cst820_hw_reset(self);

    /* Read and verify chip ID */
    if (cst820_read_reg(self, CST820_REG_CHIP_ID, &chip_id, 1) != 0) {
        return -1;  /* I2C read failed */
    }

    if (chip_id != CST820_CHIP_ID) {
        /* Accept anyway - some variants have different IDs */
    }

    return 0;
}

/* Driver: deinit */
static void cst820_deinit(egui_hal_touch_driver_t *self)
{
    if (self->bus.i2c->deinit) {
        self->bus.i2c->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

/* Driver: read */
static int cst820_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t buf[6];  /* Read touch data: gest_id + finger_num + XH + XL + YH + YL */
    uint8_t num_points;
    uint8_t gesture;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));

    /* Read gesture and touch data starting from register 0x00 */
    if (cst820_read_reg(self, CST820_REG_GEST_ID, buf, 6) != 0) {
        return -1;
    }

    gesture = buf[0];
    num_points = buf[1] & 0x0F;  /* Lower 4 bits = number of points */

    /* Store gesture ID */
    data->gesture = gesture;

    if (num_points > 1) {
        num_points = 1;  /* CST820 supports max 1 point */
    }

    if (num_points == 0) {
        return 0;  /* No touch */
    }

    int16_t x = ((buf[2] & 0x0F) << 8) | buf[3];
    int16_t y = ((buf[4] & 0x0F) << 8) | buf[5];

    /* Transform coordinates */
    cst820_transform_point(self, &x, &y);

    /* Store point */
    data->points[0].x = x;
    data->points[0].y = y;
    data->points[0].id = 0;
    data->points[0].pressure = 0;  /* Not supported */
    data->point_count = 1;

    return 0;
}

/* Driver: enter_sleep */
static void cst820_enter_sleep(egui_hal_touch_driver_t *self)
{
    uint8_t mode = 0x03;  /* Deep sleep mode */
    cst820_write_reg(self, CST820_REG_SLEEP, &mode, 1);
}

/* Driver: exit_sleep */
static void cst820_exit_sleep(egui_hal_touch_driver_t *self)
{
    /* Hardware reset to wake up */
    cst820_hw_reset(self);
}

/* Internal: setup driver function pointers */
static void cst820_setup_driver(egui_hal_touch_driver_t *driver,
                                 const egui_bus_i2c_ops_t *i2c,
                                 const egui_touch_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "CST820";
    driver->bus_type = EGUI_BUS_TYPE_I2C;
    driver->max_points = 1;

    driver->init = cst820_init;
    driver->deinit = cst820_deinit;
    driver->read = cst820_read;
    driver->set_rotation = NULL;  /* Use config swap/mirror instead */
    driver->enter_sleep = cst820_enter_sleep;
    driver->exit_sleep = cst820_exit_sleep;

    driver->bus.i2c = i2c;
    driver->gpio = gpio;
}

/* Public: init (static allocation) */
void egui_touch_cst820_init(egui_hal_touch_driver_t *storage,
                             const egui_bus_i2c_ops_t *i2c,
                             const egui_touch_gpio_ops_t *gpio)
{
    if (!storage || !i2c || !i2c->read_reg || !i2c->write_reg) {
        return;
    }

    cst820_setup_driver(storage, i2c, gpio);
}
