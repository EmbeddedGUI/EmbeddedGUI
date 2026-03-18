/**
 * @file egui_touch_tt21100.c
 * @brief TT21100 capacitive touch driver implementation
 *
 * Uses Panel IO for I2C communication.
 * Raw reads (no register phase) use rx_param with cmd=-1.
 * Register writes use tx_param with cmd=register address.
 */

#include "egui_touch_tt21100.h"
#include <string.h>
#include "core/egui_api.h"

/* TT21100 I2C address (7-bit, shifted for HAL) */
#define TT21100_ADDR 0x48 /* 0x24 << 1 */

/* TT21100 Report IDs */
#define TT21100_REPORT_ID_TOUCH 0x01
#define TT21100_REPORT_ID_BTN   0x03

#define TT21100_MAX_POINTS         10
#define TT21100_HEADER_SIZE        7
#define TT21100_POINT_SIZE         10
#define TT21100_BUTTON_REPORT_SIZE 14
#define TT21100_MAX_REPORT_SIZE    (TT21100_HEADER_SIZE + TT21100_MAX_POINTS * TT21100_POINT_SIZE)

#define TT21100_RECORD_NUM_MASK 0x1F
#define TT21100_EVENT_MASK      0x06
#define TT21100_EVENT_SHIFT     1
#define TT21100_ID_SHIFT        3

static int tt21100_reset(egui_hal_touch_driver_t *self)
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

/* Raw read: use rx_param with cmd=-1 for no-register-phase I2C read */
static int tt21100_read_data(egui_hal_touch_driver_t *self, uint8_t *data, uint16_t len)
{
    return self->io->rx_param(self->io, -1, data, (size_t)len);
}

static int tt21100_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t buf[2];

    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    if (tt21100_read_data(self, buf, sizeof(buf)) != 0)
    {
        return -1;
    }

    return 0;
}

static void tt21100_del(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
}

static int tt21100_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t buf[TT21100_MAX_REPORT_SIZE];
    uint16_t data_len;
    uint8_t report_id;
    uint8_t num_points;
    uint8_t max_records;

    memset(data, 0, sizeof(egui_hal_touch_data_t));

    if (tt21100_read_data(self, buf, 2) != 0)
    {
        return -1;
    }

    data_len = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    if (data_len == 0 || data_len == 0xFFFF)
    {
        return 0;
    }
    if (data_len > TT21100_MAX_REPORT_SIZE)
    {
        data_len = TT21100_MAX_REPORT_SIZE;
    }
    if (data_len < 3)
    {
        return 0;
    }

    if (tt21100_read_data(self, buf, data_len) != 0)
    {
        return -1;
    }

    report_id = buf[2];
    if (report_id == TT21100_REPORT_ID_BTN && data_len == TT21100_BUTTON_REPORT_SIZE)
    {
        return 0;
    }
    if (report_id != TT21100_REPORT_ID_TOUCH || data_len < TT21100_HEADER_SIZE)
    {
        return 0;
    }

    num_points = buf[5] & TT21100_RECORD_NUM_MASK;
    max_records = (uint8_t)((data_len - TT21100_HEADER_SIZE) / TT21100_POINT_SIZE);
    if (num_points > max_records)
    {
        num_points = max_records;
    }
    if (num_points > TT21100_MAX_POINTS)
    {
        num_points = TT21100_MAX_POINTS;
    }
    if (num_points > EGUI_HAL_TOUCH_MAX_POINTS)
    {
        num_points = EGUI_HAL_TOUCH_MAX_POINTS;
    }

    for (uint8_t i = 0; i < num_points; i++)
    {
        uint8_t *p = &buf[TT21100_HEADER_SIZE + i * TT21100_POINT_SIZE];
        uint8_t touch_meta = p[1];
        uint8_t touch_id = touch_meta >> TT21100_ID_SHIFT;
        uint8_t event_id = (touch_meta & TT21100_EVENT_MASK) >> TT21100_EVENT_SHIFT;
        int16_t x = (int16_t)((uint16_t)p[2] | ((uint16_t)p[3] << 8));
        int16_t y = (int16_t)((uint16_t)p[4] | ((uint16_t)p[5] << 8));
        uint8_t pressure = p[6];

        EGUI_UNUSED(event_id);

        data->points[data->point_count].x = x;
        data->points[data->point_count].y = y;
        data->points[data->point_count].id = touch_id;
        data->points[data->point_count].pressure = pressure;
        data->point_count++;
    }

    return 0;
}

static void tt21100_setup_driver(egui_hal_touch_driver_t *driver, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                                 uint8_t (*get_int)(void))
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "TT21100";
    driver->max_points = TT21100_MAX_POINTS;

    driver->reset = tt21100_reset;
    driver->init = tt21100_init;
    driver->del = tt21100_del;
    driver->read = tt21100_read;

    driver->io = io;
    driver->set_rst = set_rst;
    driver->set_int = set_int;
    driver->get_int = get_int;
}

void egui_touch_tt21100_init(egui_hal_touch_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                             uint8_t (*get_int)(void))
{
    if (!storage || !io || !io->rx_param)
    {
        return;
    }

    tt21100_setup_driver(storage, io, set_rst, set_int, get_int);
}
