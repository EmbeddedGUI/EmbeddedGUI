/**
 * @file egui_touch_chsc6540.c
 * @brief CHSC6540 capacitive touch driver implementation
 */

#include "egui_touch_chsc6540.h"
#include <string.h>
#include "core/egui_api.h"

/* CHSC6540 I2C address (7-bit, shifted for HAL) */
#define CHSC6540_ADDR 0x5C /* 0x2E << 1 */

/* CHSC6540 Registers */
#define CHSC6540_REG_DATA_START 0x00 /* Touch report start register */
#define CHSC6540_REG_POWER_MODE 0xA5 /* Power mode register */

/* Maximum touch points */
#define CHSC6540_MAX_POINTS  1
#define CHSC6540_REPORT_SIZE 15

/* Helper: reset */
static int chsc6540_reset(egui_hal_touch_driver_t *self)
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

/* Helper: read register */
static int chsc6540_read_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t *data, uint16_t len)
{
    return self->io->rx_param(self->io, (int)reg, data, (size_t)len);
}

/* Driver: init */
static int chsc6540_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    return 0;
}

/* Driver: del */
static void chsc6540_del(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
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

    if (chsc6540_read_reg(self, CHSC6540_REG_DATA_START, buf, sizeof(buf)) != 0)
    {
        return -1;
    }

    point_count = buf[2];
    if (point_count == 0)
    {
        return 0;
    }
    if (point_count > CHSC6540_MAX_POINTS)
    {
        point_count = CHSC6540_MAX_POINTS;
    }

    x = (int16_t)(((buf[3] & 0x0F) << 8) | buf[4]);
    y = (int16_t)(((buf[5] & 0x0F) << 8) | buf[6]);

    if ((x >= self->config.width) || (y >= self->config.height))
    {
        return 0;
    }

    data->points[0].x = x;
    data->points[0].y = y;
    data->points[0].id = 0;
    data->points[0].pressure = 0;
    data->point_count = point_count;

    return 0;
}

/* Internal: setup driver function pointers */
static void chsc6540_setup_driver(egui_hal_touch_driver_t *driver, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                                  uint8_t (*get_int)(void))
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "CHSC6540";
    driver->max_points = CHSC6540_MAX_POINTS;

    driver->reset = chsc6540_reset;
    driver->init = chsc6540_init;
    driver->del = chsc6540_del;
    driver->read = chsc6540_read;

    driver->io = io;
    driver->set_rst = set_rst;
    driver->set_int = set_int;
    driver->get_int = get_int;
}

/* Public: init (static allocation) */
void egui_touch_chsc6540_init(egui_hal_touch_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                              uint8_t (*get_int)(void))
{
    if (!storage || !io || !io->rx_param || !io->tx_param)
    {
        return;
    }

    chsc6540_setup_driver(storage, io, set_rst, set_int, get_int);
}
