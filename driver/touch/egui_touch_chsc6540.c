/**
 * @file egui_touch_chsc6540.c
 * @brief CHSC6540 capacitive touch driver implementation
 */

#include "egui_touch_chsc6540.h"
#include <string.h>
#include "core/egui_api.h"

/* CHSC6540 I2C address (7-bit, shifted for HAL) */
#define CHSC6540_ADDR             0x5C  /* 0x2E << 1 */

/* CHSC6540 Registers */
#define CHSC6540_REG_DATA_START   0x00  /* Touch report start register */
#define CHSC6540_REG_POWER_MODE   0xA5  /* Power mode register */

/* Maximum touch points */
#define CHSC6540_MAX_POINTS       1
#define CHSC6540_REPORT_SIZE      15

/* Helper: hardware reset */
static void chsc6540_hw_reset(egui_hal_touch_driver_t *self)
{
    if (self->gpio && self->gpio->set_rst) {
        self->gpio->set_rst(0);
        egui_api_delay(200);
        self->gpio->set_rst(1);
        egui_api_delay(200);
    }
}

/* Helper: read register */
static int chsc6540_read_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t *data, uint16_t len)
{
    return self->bus.i2c->read_reg(CHSC6540_ADDR, reg, data, len);
}

/* Helper: transform coordinates based on config */
static void chsc6540_transform_point(egui_hal_touch_driver_t *self, int16_t *x, int16_t *y)
{
    int16_t tx = *x;
    int16_t ty = *y;

    if (self->config.swap_xy) {
        int16_t tmp = tx;
        tx = ty;
        ty = tmp;
    }

    if (self->config.mirror_x) {
        tx = self->config.width - 1 - tx;
    }

    if (self->config.mirror_y) {
        ty = self->config.height - 1 - ty;
    }

    *x = tx;
    *y = ty;
}

/* Driver: init */
static int chsc6540_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    if (self->bus.i2c->init) {
        self->bus.i2c->init();
    }
    if (self->gpio && self->gpio->init) {
        self->gpio->init();
    }

    chsc6540_hw_reset(self);

    return 0;
}

/* Driver: deinit */
static void chsc6540_deinit(egui_hal_touch_driver_t *self)
{
    if (self->bus.i2c->deinit) {
        self->bus.i2c->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

/* Driver: read */
static int chsc6540_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t buf[CHSC6540_REPORT_SIZE];
    uint8_t point_count;
    int16_t x;
    int16_t y;

    memset(data, 0, sizeof(egui_hal_touch_data_t));
    memset(buf, 0, sizeof(buf));

    if (chsc6540_read_reg(self, CHSC6540_REG_DATA_START, buf, sizeof(buf)) != 0) {
        return -1;
    }

    point_count = buf[2];
    if (point_count == 0) {
        return 0;
    }
    if (point_count > CHSC6540_MAX_POINTS) {
        point_count = CHSC6540_MAX_POINTS;
    }

    x = (int16_t)(((buf[3] & 0x0F) << 8) | buf[4]);
    y = (int16_t)(((buf[5] & 0x0F) << 8) | buf[6]);

    if ((x >= self->config.width) || (y >= self->config.height)) {
        return 0;
    }

    chsc6540_transform_point(self, &x, &y);

    data->points[0].x = x;
    data->points[0].y = y;
    data->points[0].id = 0;
    data->points[0].pressure = 0;
    data->point_count = point_count;

    return 0;
}

/* Driver: enter_sleep */
static void chsc6540_enter_sleep(egui_hal_touch_driver_t *self)
{
    uint8_t mode = 0x03;  /* Sleep mode */
    self->bus.i2c->write_reg(CHSC6540_ADDR, CHSC6540_REG_POWER_MODE, &mode, 1);
}

/* Driver: exit_sleep */
static void chsc6540_exit_sleep(egui_hal_touch_driver_t *self)
{
    chsc6540_hw_reset(self);
}

/* Internal: setup driver function pointers */
static void chsc6540_setup_driver(egui_hal_touch_driver_t *driver,
                                  const egui_bus_i2c_ops_t *i2c,
                                  const egui_touch_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "CHSC6540";
    driver->bus_type = EGUI_BUS_TYPE_I2C;
    driver->max_points = CHSC6540_MAX_POINTS;

    driver->init = chsc6540_init;
    driver->deinit = chsc6540_deinit;
    driver->read = chsc6540_read;
    driver->set_rotation = NULL;
    driver->enter_sleep = chsc6540_enter_sleep;
    driver->exit_sleep = chsc6540_exit_sleep;

    driver->bus.i2c = i2c;
    driver->gpio = gpio;
}

/* Public: init (static allocation) */
void egui_touch_chsc6540_init(egui_hal_touch_driver_t *storage,
                              const egui_bus_i2c_ops_t *i2c,
                              const egui_touch_gpio_ops_t *gpio)
{
    if (!storage || !i2c || !i2c->read_reg || !i2c->write_reg) {
        return;
    }

    chsc6540_setup_driver(storage, i2c, gpio);
}
