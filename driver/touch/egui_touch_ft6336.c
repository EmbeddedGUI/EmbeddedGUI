/**
 * @file egui_touch_ft6336.c
 * @brief FT6336 capacitive touch driver implementation
 *
 * Uses unified Panel IO for I2C communication.
 */

#include "egui_touch_ft6336.h"
#include <string.h>
#include "core/egui_api.h"

/* FT6336 Registers */
#define FT6336_REG_DEV_MODE   0x00
#define FT6336_REG_GEST_ID    0x01
#define FT6336_REG_TD_STATUS  0x02 /* Number of touch points */
#define FT6336_REG_P1_XH      0x03 /* Point 1 X high + event flag */
#define FT6336_REG_P1_XL      0x04 /* Point 1 X low */
#define FT6336_REG_P1_YH      0x05 /* Point 1 Y high + touch ID */
#define FT6336_REG_P1_YL      0x06 /* Point 1 Y low */
#define FT6336_REG_P2_XH      0x09 /* Point 2 X high + event flag */
#define FT6336_REG_P2_XL      0x0A /* Point 2 X low */
#define FT6336_REG_P2_YH      0x0B /* Point 2 Y high + touch ID */
#define FT6336_REG_P2_YL      0x0C /* Point 2 Y low */
#define FT6336_REG_CHIP_ID    0xA3 /* Chip ID register */
#define FT6336_REG_POWER_MODE 0xA5

/* Expected chip ID */
#define FT6336_CHIP_ID 0x64

/* Touch event flags */
#define FT6336_EVENT_PRESS_DOWN 0x00
#define FT6336_EVENT_LIFT_UP    0x01
#define FT6336_EVENT_CONTACT    0x02
#define FT6336_EVENT_NO_EVENT   0x03

/* Helper: reset */
static int ft6336_reset(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
        egui_api_delay(5);
        self->set_rst(1);
        egui_api_delay(50);
    }
    return 0;
}

/* Helper: read register via Panel IO */
static int ft6336_read_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (!self->io || !self->io->rx_param)
    {
        return -1;
    }
    return self->io->rx_param(self->io, reg, data, len);
}

/* Driver: init */
static int ft6336_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t chip_id;

    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));
    /* Read and verify chip ID */
    if (ft6336_read_reg(self, FT6336_REG_CHIP_ID, &chip_id, 1) != 0)
    {
        return -1; /* I2C read failed */
    }

    if (chip_id != FT6336_CHIP_ID)
    {
        /* Accept anyway - some variants have different IDs */
    }

    return 0;
}

/* Driver: del */
static void ft6336_del(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
}

/* Driver: read */
static int ft6336_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t buf[14]; /* Read gesture + status + touch data at once */
    uint8_t num_points;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));

    /* Read gesture ID and touch status together */
    if (ft6336_read_reg(self, FT6336_REG_GEST_ID, buf, 2) != 0)
    {
        return -1;
    }

    data->gesture = buf[0];     /* Gesture ID */
    num_points = buf[1] & 0x0F; /* Lower 4 bits = number of points */

    if (num_points > 2)
    {
        num_points = 2; /* FT6336 supports max 2 points */
    }

    if (num_points == 0)
    {
        return 0; /* No touch */
    }

    /* Read touch point data */
    if (ft6336_read_reg(self, FT6336_REG_P1_XH, buf, num_points * 6) != 0)
    {
        return -1;
    }

    /* Parse touch points */
    for (uint8_t i = 0; i < num_points; i++)
    {
        uint8_t *p = &buf[i * 6];
        uint8_t event = (p[0] >> 6) & 0x03;

        /* Skip if no event or lift up */
        if (event == FT6336_EVENT_NO_EVENT || event == FT6336_EVENT_LIFT_UP)
        {
            continue;
        }

        int16_t x = ((p[0] & 0x0F) << 8) | p[1];
        int16_t y = ((p[2] & 0x0F) << 8) | p[3];
        uint8_t id = (p[2] >> 4) & 0x0F;

        /* Store point */
        data->points[data->point_count].x = x;
        data->points[data->point_count].y = y;
        data->points[data->point_count].id = id;
        data->points[data->point_count].pressure = 0; /* Not supported */
        data->point_count++;
    }

    return 0;
}

/* Public: init (static allocation) */
void egui_touch_ft6336_init(egui_hal_touch_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                            uint8_t (*get_int)(void))
{
    if (!storage || !io)
    {
        return;
    }

    memset(storage, 0, sizeof(egui_hal_touch_driver_t));

    storage->name = "FT6336";
    storage->max_points = 2;

    storage->reset = ft6336_reset;
    storage->init = ft6336_init;
    storage->del = ft6336_del;
    storage->read = ft6336_read;

    storage->io = io;
    storage->set_rst = set_rst;
    storage->set_int = set_int;
    storage->get_int = get_int;
}
