/**
 * @file egui_touch_tt21100.c
 * @brief TT21100 capacitive touch driver implementation
 */

#include "egui_touch_tt21100.h"
#include <string.h>
#include "core/egui_api.h"

/* TT21100 I2C address (7-bit, shifted for HAL) */
#define TT21100_ADDR            0x48  /* 0x24 << 1 */

/* TT21100 Report IDs */
#define TT21100_REPORT_ID_TOUCH 0x01
#define TT21100_REPORT_ID_BTN   0x03

#define TT21100_MAX_POINTS      10
#define TT21100_HEADER_SIZE     7
#define TT21100_POINT_SIZE      10
#define TT21100_BUTTON_REPORT_SIZE 14
#define TT21100_MAX_REPORT_SIZE (TT21100_HEADER_SIZE + TT21100_MAX_POINTS * TT21100_POINT_SIZE)

#define TT21100_RECORD_NUM_MASK 0x1F
#define TT21100_EVENT_MASK      0x06
#define TT21100_EVENT_SHIFT     1
#define TT21100_ID_SHIFT        3

static void tt21100_hw_reset(egui_hal_touch_driver_t *self)
{
    if (self->gpio && self->gpio->set_rst) {
        self->gpio->set_rst(0);
        egui_api_delay(10);
        self->gpio->set_rst(1);
        egui_api_delay(10);
    }
}

static int tt21100_read_data(egui_hal_touch_driver_t *self, uint8_t *data, uint16_t len)
{
    if (!self->bus.i2c->read_raw) {
        return -1;
    }
    return self->bus.i2c->read_raw(TT21100_ADDR, data, len);
}

static int tt21100_write_reg(egui_hal_touch_driver_t *self, uint16_t reg, const uint8_t *data, uint16_t len)
{
    if (!self->bus.i2c->write_reg) {
        return -1;
    }
    return self->bus.i2c->write_reg(TT21100_ADDR, reg, data, len);
}

static void tt21100_transform_point(egui_hal_touch_driver_t *self, int16_t *x, int16_t *y)
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

static int tt21100_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t buf[2];

    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    if (self->bus.i2c->init) {
        self->bus.i2c->init();
    }
    if (self->gpio && self->gpio->init) {
        self->gpio->init();
    }

    tt21100_hw_reset(self);

    if (tt21100_read_data(self, buf, sizeof(buf)) != 0) {
        return -1;
    }

    return 0;
}

static void tt21100_deinit(egui_hal_touch_driver_t *self)
{
    if (self->bus.i2c->deinit) {
        self->bus.i2c->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

static int tt21100_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t buf[TT21100_MAX_REPORT_SIZE];
    uint16_t data_len;
    uint8_t report_id;
    uint8_t num_points;
    uint8_t max_records;

    memset(data, 0, sizeof(egui_hal_touch_data_t));

    if (tt21100_read_data(self, buf, 2) != 0) {
        return -1;
    }

    data_len = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    if (data_len == 0 || data_len == 0xFFFF) {
        return 0;
    }
    if (data_len > TT21100_MAX_REPORT_SIZE) {
        data_len = TT21100_MAX_REPORT_SIZE;
    }
    if (data_len < 3) {
        return 0;
    }

    if (tt21100_read_data(self, buf, data_len) != 0) {
        return -1;
    }

    report_id = buf[2];
    if (report_id == TT21100_REPORT_ID_BTN && data_len == TT21100_BUTTON_REPORT_SIZE) {
        return 0;
    }
    if (report_id != TT21100_REPORT_ID_TOUCH || data_len < TT21100_HEADER_SIZE) {
        return 0;
    }

    num_points = buf[5] & TT21100_RECORD_NUM_MASK;
    max_records = (uint8_t)((data_len - TT21100_HEADER_SIZE) / TT21100_POINT_SIZE);
    if (num_points > max_records) {
        num_points = max_records;
    }
    if (num_points > TT21100_MAX_POINTS) {
        num_points = TT21100_MAX_POINTS;
    }
    if (num_points > EGUI_HAL_TOUCH_MAX_POINTS) {
        num_points = EGUI_HAL_TOUCH_MAX_POINTS;
    }

    for (uint8_t i = 0; i < num_points; i++) {
        uint8_t *p = &buf[TT21100_HEADER_SIZE + i * TT21100_POINT_SIZE];
        uint8_t touch_meta = p[1];
        uint8_t touch_id = touch_meta >> TT21100_ID_SHIFT;
        uint8_t event_id = (touch_meta & TT21100_EVENT_MASK) >> TT21100_EVENT_SHIFT;
        int16_t x = (int16_t)((uint16_t)p[2] | ((uint16_t)p[3] << 8));
        int16_t y = (int16_t)((uint16_t)p[4] | ((uint16_t)p[5] << 8));
        uint8_t pressure = p[6];

        EGUI_UNUSED(event_id);

        tt21100_transform_point(self, &x, &y);

        data->points[data->point_count].x = x;
        data->points[data->point_count].y = y;
        data->points[data->point_count].id = touch_id;
        data->points[data->point_count].pressure = pressure;
        data->point_count++;
    }

    return 0;
}

static void tt21100_enter_sleep(egui_hal_touch_driver_t *self)
{
    static const uint8_t power_save_cmd[2] = {0x01, 0x08};
    (void)tt21100_write_reg(self, 0x0500, power_save_cmd, sizeof(power_save_cmd));
}

static void tt21100_exit_sleep(egui_hal_touch_driver_t *self)
{
    static const uint8_t power_save_cmd[2] = {0x00, 0x08};

    if (tt21100_write_reg(self, 0x0500, power_save_cmd, sizeof(power_save_cmd)) != 0) {
        tt21100_hw_reset(self);
    }
}

static void tt21100_setup_driver(egui_hal_touch_driver_t *driver,
                                  const egui_bus_i2c_ops_t *i2c,
                                  const egui_touch_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "TT21100";
    driver->bus_type = EGUI_BUS_TYPE_I2C;
    driver->max_points = TT21100_MAX_POINTS;

    driver->init = tt21100_init;
    driver->deinit = tt21100_deinit;
    driver->read = tt21100_read;
    driver->set_rotation = NULL;
    driver->enter_sleep = tt21100_enter_sleep;
    driver->exit_sleep = tt21100_exit_sleep;

    driver->bus.i2c = i2c;
    driver->gpio = gpio;
}

void egui_touch_tt21100_init(egui_hal_touch_driver_t *storage,
                              const egui_bus_i2c_ops_t *i2c,
                              const egui_touch_gpio_ops_t *gpio)
{
    if (!storage || !i2c || !i2c->read_raw) {
        return;
    }

    tt21100_setup_driver(storage, i2c, gpio);
}
