#if EGUI_DRIVER_TOUCH_AXS15231B_ENABLE

/**
 * @file egui_touch_axs15231b.c
 * @brief AXS15231B capacitive touch driver implementation
 *
 * Uses unified Panel IO for I2C communication.
 * For raw I2C transfers, downcasts to egui_panel_io_i2c_t.
 */

#include "egui_touch_axs15231b.h"
#include "egui_panel_io_i2c.h"
#include <string.h>
#include "core/egui_api.h"

#define AXS15231B_ADDR         0x76 /* 0x3B << 1 */
#define AXS15231B_MAX_POINTS   5
#define AXS15231B_READ_HDR_LEN 2
#define AXS15231B_POINT_STRIDE 6
#define AXS15231B_READ_LEN     (AXS15231B_READ_HDR_LEN + AXS15231B_MAX_POINTS * AXS15231B_POINT_STRIDE)

#define AXS15231B_EVENT_MASK     0xC0
#define AXS15231B_EVENT_SHIFT    6
#define AXS15231B_EVENT_LIFT_UP  0x01
#define AXS15231B_EVENT_NO_EVENT 0x03

static const uint8_t AXS15231B_READ_CMD[11] = {0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00, (uint8_t)(AXS15231B_READ_LEN >> 8), (uint8_t)(AXS15231B_READ_LEN & 0xFF),
                                               0x00, 0x00, 0x00};

static int axs15231b_reset(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
        egui_api_delay(200);
        self->set_rst(1);
        egui_api_delay(200);
    }
    return 0;
}

static int axs15231b_write_raw(egui_hal_touch_driver_t *self, const uint8_t *data, uint16_t len)
{
    egui_panel_io_i2c_t *io_i2c = (egui_panel_io_i2c_t *)self->io;
    if (!io_i2c->i2c->write_raw)
    {
        return -1;
    }
    return io_i2c->i2c->write_raw(io_i2c->dev_addr, data, len);
}

static int axs15231b_read_raw(egui_hal_touch_driver_t *self, uint8_t *data, uint16_t len)
{
    egui_panel_io_i2c_t *io_i2c = (egui_panel_io_i2c_t *)self->io;
    if (!io_i2c->i2c->read_raw)
    {
        return -1;
    }
    return io_i2c->i2c->read_raw(io_i2c->dev_addr, data, len);
}

static int axs15231b_query(egui_hal_touch_driver_t *self, uint8_t *data, uint16_t len)
{
    if (axs15231b_write_raw(self, AXS15231B_READ_CMD, sizeof(AXS15231B_READ_CMD)) != 0)
    {
        return -1;
    }
    if (axs15231b_read_raw(self, data, len) != 0)
    {
        return -1;
    }
    return 0;
}

static int axs15231b_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t buf[AXS15231B_READ_LEN];

    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    if (axs15231b_query(self, buf, sizeof(buf)) != 0)
    {
        return -1;
    }

    return 0;
}

static void axs15231b_del(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
}

static int axs15231b_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t buf[AXS15231B_READ_LEN];
    uint8_t num_points;

    memset(data, 0, sizeof(egui_hal_touch_data_t));

    if (axs15231b_query(self, buf, sizeof(buf)) != 0)
    {
        return -1;
    }

    data->gesture = buf[0];
    num_points = buf[1];
    if (num_points > AXS15231B_MAX_POINTS)
    {
        num_points = AXS15231B_MAX_POINTS;
    }
    if (num_points > EGUI_HAL_TOUCH_MAX_POINTS)
    {
        num_points = EGUI_HAL_TOUCH_MAX_POINTS;
    }

    for (uint8_t i = 0; i < num_points; i++)
    {
        uint8_t base = (uint8_t)(AXS15231B_READ_HDR_LEN + i * AXS15231B_POINT_STRIDE);
        uint8_t event = (buf[base] & AXS15231B_EVENT_MASK) >> AXS15231B_EVENT_SHIFT;
        int16_t x;
        int16_t y;

        if (event == AXS15231B_EVENT_NO_EVENT || event == AXS15231B_EVENT_LIFT_UP)
        {
            continue;
        }

        x = (int16_t)((((uint16_t)buf[base] & 0x0F) << 8) | buf[base + 1]);
        y = (int16_t)((((uint16_t)buf[base + 2] & 0x0F) << 8) | buf[base + 3]);

        data->points[data->point_count].x = x;
        data->points[data->point_count].y = y;
        data->points[data->point_count].id = i;
        data->points[data->point_count].pressure = 0;
        data->point_count++;
    }

    return 0;
}

void egui_touch_axs15231b_init(egui_hal_touch_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                               uint8_t (*get_int)(void))
{
    if (!storage || !io)
    {
        return;
    }

    memset(storage, 0, sizeof(egui_hal_touch_driver_t));

    storage->name = "AXS15231B";
    storage->max_points = AXS15231B_MAX_POINTS;

    storage->reset = axs15231b_reset;
    storage->init = axs15231b_init;
    storage->del = axs15231b_del;
    storage->read = axs15231b_read;

    storage->io = io;
    storage->set_rst = set_rst;
    storage->set_int = set_int;
    storage->get_int = get_int;
}

#endif /* EGUI_DRIVER_TOUCH_AXS15231B_ENABLE */
