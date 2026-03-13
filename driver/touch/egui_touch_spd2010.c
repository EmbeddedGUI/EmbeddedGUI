/**
 * @file egui_touch_spd2010.c
 * @brief SPD2010 capacitive touch driver implementation
 */

#include "egui_touch_spd2010.h"
#include <string.h>
#include "core/egui_api.h"

#define SPD2010_ADDR            0x5C  /* 0x2E << 1 */
#define SPD2010_MAX_POINTS      5
#define SPD2010_STATUS_LEN      4
#define SPD2010_HDP_STATUS_LEN  8
#define SPD2010_FW_VER_LEN      18
#define SPD2010_POINT_STRIDE    6
#define SPD2010_HDP_HEADER_LEN  4
#define SPD2010_MAX_HDP_LEN     (SPD2010_HDP_HEADER_LEN + SPD2010_MAX_POINTS * SPD2010_POINT_STRIDE)

#define SPD2010_STATUS_PT_EXIST   0x01
#define SPD2010_STATUS_GESTURE    0x02
#define SPD2010_STATUS_AUX        0x08
#define SPD2010_CPU_RUN           0x08
#define SPD2010_TIC_IN_CPU        0x20
#define SPD2010_TIC_IN_BIOS       0x40

static const uint8_t SPD2010_CMD_POINT_MODE[4] = {0x50, 0x00, 0x00, 0x00};
static const uint8_t SPD2010_CMD_START[4]      = {0x46, 0x00, 0x00, 0x00};
static const uint8_t SPD2010_CMD_CPU_START[4]  = {0x04, 0x00, 0x01, 0x00};
static const uint8_t SPD2010_CMD_CLEAR_INT[4]  = {0x02, 0x00, 0x01, 0x00};
static const uint8_t SPD2010_CMD_STATUS[2]     = {0x20, 0x00};
static const uint8_t SPD2010_CMD_HDP[2]        = {0x00, 0x03};
static const uint8_t SPD2010_CMD_HDP_STATUS[2] = {0xFC, 0x02};
static const uint8_t SPD2010_CMD_FW_VER[2]     = {0x26, 0x00};

typedef struct spd2010_status {
    uint8_t status_low;
    uint8_t status_high;
    uint16_t read_len;
} spd2010_status_t;

static void spd2010_hw_reset(egui_hal_touch_driver_t *self)
{
    if (self->gpio && self->gpio->set_rst) {
        self->gpio->set_rst(0);
        egui_api_delay(2);
        self->gpio->set_rst(1);
        egui_api_delay(22);
    }
}

static int spd2010_write_raw(egui_hal_touch_driver_t *self, const uint8_t *data, uint16_t len)
{
    if (!self->bus.i2c->write_raw) {
        return -1;
    }
    return self->bus.i2c->write_raw(SPD2010_ADDR, data, len);
}

static int spd2010_read_raw(egui_hal_touch_driver_t *self, uint8_t *data, uint16_t len)
{
    if (!self->bus.i2c->read_raw) {
        return -1;
    }
    return self->bus.i2c->read_raw(SPD2010_ADDR, data, len);
}

static int spd2010_query(egui_hal_touch_driver_t *self, const uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len)
{
    if (spd2010_write_raw(self, tx, tx_len) != 0) {
        return -1;
    }
    egui_api_delay(1);
    if (spd2010_read_raw(self, rx, rx_len) != 0) {
        return -1;
    }
    egui_api_delay(1);
    return 0;
}

static void spd2010_transform_point(egui_hal_touch_driver_t *self, int16_t *x, int16_t *y)
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

static int spd2010_read_status(egui_hal_touch_driver_t *self, spd2010_status_t *status)
{
    uint8_t sample[SPD2010_STATUS_LEN];

    if (spd2010_query(self, SPD2010_CMD_STATUS, sizeof(SPD2010_CMD_STATUS), sample, sizeof(sample)) != 0) {
        return -1;
    }

    status->status_low = sample[0];
    status->status_high = sample[1];
    status->read_len = (uint16_t)sample[2] | ((uint16_t)sample[3] << 8);
    return 0;
}

static int spd2010_clear_int(egui_hal_touch_driver_t *self)
{
    if (spd2010_write_raw(self, SPD2010_CMD_CLEAR_INT, sizeof(SPD2010_CMD_CLEAR_INT)) != 0) {
        return -1;
    }
    egui_api_delay(1);
    return 0;
}

static int spd2010_read_fw_version(egui_hal_touch_driver_t *self)
{
    uint8_t sample[SPD2010_FW_VER_LEN];
    return spd2010_query(self, SPD2010_CMD_FW_VER, sizeof(SPD2010_CMD_FW_VER), sample, sizeof(sample));
}

static int spd2010_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    if (self->bus.i2c->init) {
        self->bus.i2c->init();
    }
    if (self->gpio && self->gpio->init) {
        self->gpio->init();
    }

    spd2010_hw_reset(self);

    if (spd2010_read_fw_version(self) != 0) {
        return -1;
    }
    return 0;
}

static void spd2010_deinit(egui_hal_touch_driver_t *self)
{
    if (self->bus.i2c->deinit) {
        self->bus.i2c->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

static int spd2010_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    spd2010_status_t status;
    uint8_t sample[SPD2010_MAX_HDP_LEN];
    uint8_t hdp_status[SPD2010_HDP_STATUS_LEN];
    uint8_t touch_num = 0;

    memset(data, 0, sizeof(egui_hal_touch_data_t));

    if (spd2010_read_status(self, &status) != 0) {
        return -1;
    }

    if (status.status_high & SPD2010_TIC_IN_BIOS) {
        if (spd2010_clear_int(self) != 0) {
            return -1;
        }
        if (spd2010_write_raw(self, SPD2010_CMD_CPU_START, sizeof(SPD2010_CMD_CPU_START)) != 0) {
            return -1;
        }
        egui_api_delay(1);
        return 0;
    }

    if (status.status_high & SPD2010_TIC_IN_CPU) {
        if (spd2010_write_raw(self, SPD2010_CMD_POINT_MODE, sizeof(SPD2010_CMD_POINT_MODE)) != 0) {
            return -1;
        }
        egui_api_delay(1);
        if (spd2010_write_raw(self, SPD2010_CMD_START, sizeof(SPD2010_CMD_START)) != 0) {
            return -1;
        }
        egui_api_delay(1);
        if (spd2010_clear_int(self) != 0) {
            return -1;
        }
        return 0;
    }

    if ((status.status_high & SPD2010_CPU_RUN) && status.read_len == 0) {
        return spd2010_clear_int(self);
    }

    if ((status.status_low & (SPD2010_STATUS_PT_EXIST | SPD2010_STATUS_GESTURE)) == 0) {
        if ((status.status_high & SPD2010_CPU_RUN) && (status.status_low & SPD2010_STATUS_AUX)) {
            return spd2010_clear_int(self);
        }
        return 0;
    }

    if (status.read_len > sizeof(sample) || status.read_len < SPD2010_HDP_HEADER_LEN) {
        return 0;
    }

    if (spd2010_query(self, SPD2010_CMD_HDP, sizeof(SPD2010_CMD_HDP), sample, status.read_len) != 0) {
        return -1;
    }

    if ((sample[4] <= 0x0A) && (status.status_low & SPD2010_STATUS_PT_EXIST)) {
        touch_num = (uint8_t)((status.read_len - SPD2010_HDP_HEADER_LEN) / SPD2010_POINT_STRIDE);
        if (touch_num > SPD2010_MAX_POINTS) {
            touch_num = SPD2010_MAX_POINTS;
        }
        if (touch_num > EGUI_HAL_TOUCH_MAX_POINTS) {
            touch_num = EGUI_HAL_TOUCH_MAX_POINTS;
        }

        for (uint8_t i = 0; i < touch_num; i++) {
            uint8_t offset = (uint8_t)(i * SPD2010_POINT_STRIDE);
            int16_t x = (int16_t)((((uint16_t)sample[7 + offset] & 0xF0) << 4) | sample[5 + offset]);
            int16_t y = (int16_t)((((uint16_t)sample[7 + offset] & 0x0F) << 8) | sample[6 + offset]);

            spd2010_transform_point(self, &x, &y);

            data->points[data->point_count].id = sample[4 + offset];
            data->points[data->point_count].x = x;
            data->points[data->point_count].y = y;
            data->points[data->point_count].pressure = sample[8 + offset];
            data->point_count++;
        }
    } else if ((sample[4] == 0xF6) && (status.status_low & SPD2010_STATUS_GESTURE)) {
        data->gesture = sample[6] & 0x07;
    }

    while (1) {
        if (spd2010_query(self, SPD2010_CMD_HDP_STATUS, sizeof(SPD2010_CMD_HDP_STATUS), hdp_status, sizeof(hdp_status)) != 0) {
            return -1;
        }

        if (hdp_status[5] == 0x82) {
            return spd2010_clear_int(self);
        }
        if (hdp_status[5] != 0x00) {
            break;
        }

        if (hdp_status[2] == 0 && hdp_status[3] == 0) {
            break;
        }

        {
            uint16_t remain_len = (uint16_t)hdp_status[2] | ((uint16_t)hdp_status[3] << 8);
            if (remain_len == 0 || remain_len > sizeof(sample)) {
                break;
            }
            if (spd2010_query(self, SPD2010_CMD_HDP, sizeof(SPD2010_CMD_HDP), sample, remain_len) != 0) {
                return -1;
            }
        }
    }

    return 0;
}

static void spd2010_enter_sleep(egui_hal_touch_driver_t *self)
{
    static const uint8_t sleep_cmd[4] = {0x02, 0x00, 0x01, 0x00};
    (void)spd2010_write_raw(self, sleep_cmd, sizeof(sleep_cmd));
}

static void spd2010_exit_sleep(egui_hal_touch_driver_t *self)
{
    spd2010_hw_reset(self);
}

static void spd2010_setup_driver(egui_hal_touch_driver_t *driver,
                                  const egui_bus_i2c_ops_t *i2c,
                                  const egui_touch_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "SPD2010";
    driver->bus_type = EGUI_BUS_TYPE_I2C;
    driver->max_points = SPD2010_MAX_POINTS;

    driver->init = spd2010_init;
    driver->deinit = spd2010_deinit;
    driver->read = spd2010_read;
    driver->set_rotation = NULL;
    driver->enter_sleep = spd2010_enter_sleep;
    driver->exit_sleep = spd2010_exit_sleep;

    driver->bus.i2c = i2c;
    driver->gpio = gpio;
}

void egui_touch_spd2010_init(egui_hal_touch_driver_t *storage,
                             const egui_bus_i2c_ops_t *i2c,
                             const egui_touch_gpio_ops_t *gpio)
{
    if (!storage || !i2c || !i2c->write_raw || !i2c->read_raw) {
        return;
    }

    spd2010_setup_driver(storage, i2c, gpio);
}
