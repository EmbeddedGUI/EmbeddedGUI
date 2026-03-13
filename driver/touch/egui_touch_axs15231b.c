/**
 * @file egui_touch_axs15231b.c
 * @brief AXS15231B capacitive touch driver implementation
 */

#include "egui_touch_axs15231b.h"
#include <string.h>
#include "core/egui_api.h"

#define AXS15231B_ADDR          0x76  /* 0x3B << 1 */
#define AXS15231B_MAX_POINTS    5
#define AXS15231B_READ_HDR_LEN  2
#define AXS15231B_POINT_STRIDE  6
#define AXS15231B_READ_LEN      (AXS15231B_READ_HDR_LEN + AXS15231B_MAX_POINTS * AXS15231B_POINT_STRIDE)

#define AXS15231B_EVENT_MASK      0xC0
#define AXS15231B_EVENT_SHIFT     6
#define AXS15231B_EVENT_LIFT_UP   0x01
#define AXS15231B_EVENT_NO_EVENT  0x03

static const uint8_t AXS15231B_READ_CMD[11] = {
    0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00,
    (uint8_t)(AXS15231B_READ_LEN >> 8), (uint8_t)(AXS15231B_READ_LEN & 0xFF),
    0x00, 0x00, 0x00
};

static void axs15231b_hw_reset(egui_hal_touch_driver_t *self)
{
    if (self->gpio && self->gpio->set_rst) {
        self->gpio->set_rst(0);
        egui_api_delay(200);
        self->gpio->set_rst(1);
        egui_api_delay(200);
    }
}

static int axs15231b_write_raw(egui_hal_touch_driver_t *self, const uint8_t *data, uint16_t len)
{
    if (!self->bus.i2c->write_raw) {
        return -1;
    }
    return self->bus.i2c->write_raw(AXS15231B_ADDR, data, len);
}

static int axs15231b_read_raw(egui_hal_touch_driver_t *self, uint8_t *data, uint16_t len)
{
    if (!self->bus.i2c->read_raw) {
        return -1;
    }
    return self->bus.i2c->read_raw(AXS15231B_ADDR, data, len);
}

static int axs15231b_query(egui_hal_touch_driver_t *self, uint8_t *data, uint16_t len)
{
    if (axs15231b_write_raw(self, AXS15231B_READ_CMD, sizeof(AXS15231B_READ_CMD)) != 0) {
        return -1;
    }
    if (axs15231b_read_raw(self, data, len) != 0) {
        return -1;
    }
    return 0;
}

static void axs15231b_transform_point(egui_hal_touch_driver_t *self, int16_t *x, int16_t *y)
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

static int axs15231b_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t buf[AXS15231B_READ_LEN];

    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    if (self->bus.i2c->init) {
        self->bus.i2c->init();
    }
    if (self->gpio && self->gpio->init) {
        self->gpio->init();
    }

    axs15231b_hw_reset(self);

    if (axs15231b_query(self, buf, sizeof(buf)) != 0) {
        return -1;
    }

    return 0;
}

static void axs15231b_deinit(egui_hal_touch_driver_t *self)
{
    if (self->bus.i2c->deinit) {
        self->bus.i2c->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

static int axs15231b_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t buf[AXS15231B_READ_LEN];
    uint8_t num_points;

    memset(data, 0, sizeof(egui_hal_touch_data_t));

    if (axs15231b_query(self, buf, sizeof(buf)) != 0) {
        return -1;
    }

    data->gesture = buf[0];
    num_points = buf[1];
    if (num_points > AXS15231B_MAX_POINTS) {
        num_points = AXS15231B_MAX_POINTS;
    }
    if (num_points > EGUI_HAL_TOUCH_MAX_POINTS) {
        num_points = EGUI_HAL_TOUCH_MAX_POINTS;
    }

    for (uint8_t i = 0; i < num_points; i++) {
        uint8_t base = (uint8_t)(AXS15231B_READ_HDR_LEN + i * AXS15231B_POINT_STRIDE);
        uint8_t event = (buf[base] & AXS15231B_EVENT_MASK) >> AXS15231B_EVENT_SHIFT;
        int16_t x;
        int16_t y;

        if (event == AXS15231B_EVENT_NO_EVENT || event == AXS15231B_EVENT_LIFT_UP) {
            continue;
        }

        x = (int16_t)((((uint16_t)buf[base] & 0x0F) << 8) | buf[base + 1]);
        y = (int16_t)((((uint16_t)buf[base + 2] & 0x0F) << 8) | buf[base + 3]);

        axs15231b_transform_point(self, &x, &y);

        data->points[data->point_count].x = x;
        data->points[data->point_count].y = y;
        data->points[data->point_count].id = i;
        data->points[data->point_count].pressure = 0;
        data->point_count++;
    }

    return 0;
}

static void axs15231b_enter_sleep(egui_hal_touch_driver_t *self)
{
    EGUI_UNUSED(self);
}

static void axs15231b_exit_sleep(egui_hal_touch_driver_t *self)
{
    axs15231b_hw_reset(self);
}

static void axs15231b_setup_driver(egui_hal_touch_driver_t *driver,
                                    const egui_bus_i2c_ops_t *i2c,
                                    const egui_touch_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "AXS15231B";
    driver->bus_type = EGUI_BUS_TYPE_I2C;
    driver->max_points = AXS15231B_MAX_POINTS;

    driver->init = axs15231b_init;
    driver->deinit = axs15231b_deinit;
    driver->read = axs15231b_read;
    driver->set_rotation = NULL;
    driver->enter_sleep = axs15231b_enter_sleep;
    driver->exit_sleep = axs15231b_exit_sleep;

    driver->bus.i2c = i2c;
    driver->gpio = gpio;
}

void egui_touch_axs15231b_init(egui_hal_touch_driver_t *storage,
                               const egui_bus_i2c_ops_t *i2c,
                               const egui_touch_gpio_ops_t *gpio)
{
    if (!storage || !i2c || !i2c->write_raw || !i2c->read_raw) {
        return;
    }

    axs15231b_setup_driver(storage, i2c, gpio);
}
