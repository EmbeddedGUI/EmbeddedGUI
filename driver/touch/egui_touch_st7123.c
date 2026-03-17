/**
 * @file egui_touch_st7123.c
 * @brief ST7123 capacitive touch driver implementation
 */

#include "egui_touch_st7123.h"
#include <string.h>
#include "core/egui_api.h"

/* ST7123 I2C address (7-bit, shifted for HAL) */
#define ST7123_ADDR 0xAA /* 0x55 << 1 */

/* ST7123 Registers */
#define ST7123_REG_FW_VERSION     0x0000
#define ST7123_REG_MAX_TOUCHES    0x0009
#define ST7123_REG_ADV_INFO       0x0010
#define ST7123_REG_REPORT_COORD_0 0x0014

/* Maximum touch points */
#define ST7123_MAX_POINTS    5
#define ST7123_HW_MAX_POINTS 10

/* Helper: hardware reset */
static void st7123_hw_reset(egui_hal_touch_driver_t *self)
{
    if (self->gpio && self->gpio->set_rst)
    {
        egui_api_delay(10);
        self->gpio->set_rst(0);
        egui_api_delay(10);
        self->gpio->set_rst(1);
        egui_api_delay(10);
    }
}

/* Helper: read register */
static int st7123_read_reg(egui_hal_touch_driver_t *self, uint16_t reg, uint8_t *data, uint16_t len)
{
    return self->bus.i2c->read_reg(ST7123_ADDR, reg, data, len);
}

/* Helper: transform coordinates based on config */
static void st7123_transform_point(egui_hal_touch_driver_t *self, int16_t *x, int16_t *y)
{
    int16_t tx = *x;
    int16_t ty = *y;

    if (self->config.swap_xy)
    {
        int16_t tmp = tx;
        tx = ty;
        ty = tmp;
    }

    if (self->config.mirror_x)
    {
        tx = self->config.width - 1 - tx;
    }

    if (self->config.mirror_y)
    {
        ty = self->config.height - 1 - ty;
    }

    *x = tx;
    *y = ty;
}

/* Driver: init */
static int st7123_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t fw_version;

    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    if (self->bus.i2c->init)
    {
        self->bus.i2c->init();
    }
    if (self->gpio && self->gpio->init)
    {
        self->gpio->init();
    }

    st7123_hw_reset(self);

    if (st7123_read_reg(self, ST7123_REG_FW_VERSION, &fw_version, 1) != 0)
    {
        return -1;
    }

    return 0;
}

/* Driver: deinit */
static void st7123_deinit(egui_hal_touch_driver_t *self)
{
    if (self->bus.i2c->deinit)
    {
        self->bus.i2c->deinit();
    }
    if (self->gpio && self->gpio->deinit)
    {
        self->gpio->deinit();
    }
}

/* Driver: read */
static int st7123_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    typedef struct __attribute__((packed)) st7123_touch_report
    {
        uint8_t x_h : 6;
        uint8_t reserved_6 : 1;
        uint8_t valid : 1;
        uint8_t x_l;
        uint8_t y_h;
        uint8_t y_l;
        uint8_t area;
        uint8_t intensity;
        uint8_t reserved_49_55;
    } st7123_touch_report_t;
    typedef struct __attribute__((packed)) st7123_adv_info
    {
        uint8_t reserved_0_1 : 2;
        uint8_t with_prox : 1;
        uint8_t with_coord : 1;
        uint8_t prox_status : 3;
        uint8_t rst_chip : 1;
    } st7123_adv_info_t;

    st7123_touch_report_t report[ST7123_HW_MAX_POINTS];
    st7123_adv_info_t adv_info;
    uint8_t max_touches = 0;
    uint8_t point_count = 0;

    memset(data, 0, sizeof(egui_hal_touch_data_t));
    memset(report, 0, sizeof(report));
    memset(&adv_info, 0, sizeof(adv_info));

    if (st7123_read_reg(self, ST7123_REG_ADV_INFO, (uint8_t *)&adv_info, 1) != 0)
    {
        return -1;
    }

    if (!adv_info.with_coord)
    {
        return 0;
    }

    if (st7123_read_reg(self, ST7123_REG_MAX_TOUCHES, &max_touches, 1) != 0)
    {
        return -1;
    }
    if (max_touches > ST7123_HW_MAX_POINTS)
    {
        max_touches = ST7123_HW_MAX_POINTS;
    }
    if (max_touches == 0)
    {
        return 0;
    }

    if (st7123_read_reg(self, ST7123_REG_REPORT_COORD_0, (uint8_t *)&report[0], (uint16_t)(sizeof(st7123_touch_report_t) * max_touches)) != 0)
    {
        return -1;
    }

    for (uint8_t i = 0; (i < max_touches) && (point_count < EGUI_HAL_TOUCH_MAX_POINTS); i++)
    {
        int16_t x = ((int16_t)report[i].x_h << 8) | report[i].x_l;
        int16_t y = ((int16_t)report[i].y_h << 8) | report[i].y_l;

        if (!report[i].valid)
        {
            continue;
        }

        if (((x == 0) && (y == 0)) || (x >= self->config.width) || (y >= self->config.height))
        {
            continue;
        }

        st7123_transform_point(self, &x, &y);

        data->points[point_count].x = x;
        data->points[point_count].y = y;
        data->points[point_count].id = point_count;
        data->points[point_count].pressure = report[i].area;
        point_count++;
    }

    data->point_count = point_count;

    return 0;
}

/* Internal: setup driver function pointers */
static void st7123_setup_driver(egui_hal_touch_driver_t *driver, const egui_bus_i2c_ops_t *i2c, const egui_touch_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "ST7123";
    driver->bus_type = EGUI_BUS_TYPE_I2C;
    driver->max_points = ST7123_MAX_POINTS;

    driver->init = st7123_init;
    driver->deinit = st7123_deinit;
    driver->read = st7123_read;
    driver->set_rotation = NULL;
    driver->enter_sleep = NULL;
    driver->exit_sleep = NULL;

    driver->bus.i2c = i2c;
    driver->gpio = gpio;
}

/* Public: init (static allocation) */
void egui_touch_st7123_init(egui_hal_touch_driver_t *storage, const egui_bus_i2c_ops_t *i2c, const egui_touch_gpio_ops_t *gpio)
{
    if (!storage || !i2c || !i2c->read_reg || !i2c->write_reg)
    {
        return;
    }

    st7123_setup_driver(storage, i2c, gpio);
}
