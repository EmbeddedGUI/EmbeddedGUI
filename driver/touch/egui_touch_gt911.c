#if EGUI_DRIVER_TOUCH_GT911_ENABLE

/**
 * @file egui_touch_gt911.c
 * @brief GT911 capacitive touch driver implementation
 */

#include "egui_touch_gt911.h"
#include "egui_panel_io_i2c.h"
#include <string.h>
#include "core/egui_api.h"

/* GT911 I2C address (7-bit, shifted for HAL) */
#define GT911_ADDR_DEFAULT 0xBA /* 0x5D << 1 */
#define GT911_ADDR_BACKUP  0x28 /* 0x14 << 1 */

/* GT911 Registers (16-bit addresses) */
#define GT911_REG_PRODUCT_ID  0x8140 /* Product ID (4 bytes: "911\0") */
#define GT911_REG_CONFIG      0x8047 /* Configuration register */
#define GT911_REG_READ_XY     0x814E /* Touch status and data */
#define GT911_REG_ENTER_SLEEP 0x8040 /* Sleep control register */

/* Touch data structure:
 * 0x814E: Status (bit7=ready, bit3:0=touch count)
 * 0x814F+: Point data (8 bytes per point)
 *   - Byte 0: Track ID
 *   - Byte 1-2: X coordinate (low, high)
 *   - Byte 3-4: Y coordinate (low, high)
 *   - Byte 5-6: Size (low, high)
 *   - Byte 7: Reserved
 */

/* Maximum touch points */
#define GT911_MAX_POINTS 5

/* Helper: reset */
static int gt911_reset(egui_hal_touch_driver_t *self)
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

/* Helper: read register (16-bit address) */
static int gt911_read_reg(egui_hal_touch_driver_t *self, uint16_t reg, uint8_t *data, uint16_t len)
{
    return self->io->rx_param(self->io, (int)reg, data, (size_t)len);
}

/* Helper: write register (16-bit address) */
static int gt911_write_reg(egui_hal_touch_driver_t *self, uint16_t reg, uint8_t *data, uint16_t len)
{
    return self->io->tx_param(self->io, (int)reg, data, (size_t)len);
}

/* Driver: init */
static int gt911_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t product_id[4];

    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    /* Try default address first */
    egui_panel_io_i2c_set_addr((egui_panel_io_i2c_t *)self->io, GT911_ADDR_DEFAULT);
    if (gt911_read_reg(self, GT911_REG_PRODUCT_ID, product_id, 4) != 0)
    {
        /* Try backup address */
        egui_panel_io_i2c_set_addr((egui_panel_io_i2c_t *)self->io, GT911_ADDR_BACKUP);
        if (gt911_read_reg(self, GT911_REG_PRODUCT_ID, product_id, 4) != 0)
        {
            return -1; /* I2C read failed */
        }
    }

    /* Verify product ID (should be "911\0" or similar) */
    /* Accept anyway - some variants have different IDs */

    return 0;
}

/* Driver: del */
static void gt911_del(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
}

/* Driver: read */
static int gt911_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t status;
    uint8_t buf[GT911_MAX_POINTS * 8]; /* 8 bytes per point */
    uint8_t num_points;
    uint8_t clear = 0;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));

    /* Read touch status */
    if (gt911_read_reg(self, GT911_REG_READ_XY, &status, 1) != 0)
    {
        return -1;
    }

    /* Check if data is ready (bit 7) */
    if ((status & 0x80) == 0)
    {
        return 0; /* No new data */
    }

    /* Get number of touch points (bits 3:0) */
    num_points = status & 0x0F;
    if (num_points > GT911_MAX_POINTS)
    {
        num_points = GT911_MAX_POINTS;
    }

    if (num_points > 0)
    {
        /* Read touch point data */
        if (gt911_read_reg(self, GT911_REG_READ_XY + 1, buf, num_points * 8) != 0)
        {
            /* Clear status anyway */
            gt911_write_reg(self, GT911_REG_READ_XY, &clear, 1);
            return -1;
        }

        /* Parse touch points */
        for (uint8_t i = 0; i < num_points; i++)
        {
            uint8_t *p = &buf[i * 8];
            uint8_t id = p[0];
            int16_t x = (p[2] << 8) | p[1];
            int16_t y = (p[4] << 8) | p[3];
            uint16_t size = (p[6] << 8) | p[5];

            /* Store point */
            data->points[data->point_count].x = x;
            data->points[data->point_count].y = y;
            data->points[data->point_count].id = id;
            data->points[data->point_count].pressure = size;
            data->point_count++;
        }
    }

    /* Clear status register (write 0 to 0x814E) */
    gt911_write_reg(self, GT911_REG_READ_XY, &clear, 1);

    return 0;
}

/* Internal: setup driver function pointers */
static void gt911_setup_driver(egui_hal_touch_driver_t *driver, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                               uint8_t (*get_int)(void))
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "GT911";
    driver->max_points = GT911_MAX_POINTS;

    driver->reset = gt911_reset;
    driver->init = gt911_init;
    driver->del = gt911_del;
    driver->read = gt911_read;

    driver->io = io;
    driver->set_rst = set_rst;
    driver->set_int = set_int;
    driver->get_int = get_int;
}

/* Public: init (static allocation) */
void egui_touch_gt911_init(egui_hal_touch_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                           uint8_t (*get_int)(void))
{
    if (!storage || !io || !io->rx_param || !io->tx_param)
    {
        return;
    }

    gt911_setup_driver(storage, io, set_rst, set_int, get_int);
}

#endif /* EGUI_DRIVER_TOUCH_GT911_ENABLE */
