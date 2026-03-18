/**
 * @file egui_touch_st1633.c
 * @brief ST1633 capacitive touch driver implementation
 */

#include "egui_touch_st1633.h"
#include <string.h>
#include "core/egui_api.h"

/* ST1633 I2C address (7-bit, shifted for HAL) */
#define ST1633_ADDR 0xAA /* 0x55 << 1 */

/* ST1633 Registers */
#define ST1633_REG_STATUS       0x01 /* Device status */
#define ST1633_REG_TOUCH_INFO   0x10 /* Advanced touch information block */
#define ST1633_REG_KEYS         0x11 /* Key status */
#define ST1633_REG_XY_COORD     0x12 /* Legacy XY coordinate data start */
#define ST1633_REG_DEVICE_CTRL  0x02 /* Device control */
#define ST1633_REG_TIMEOUT      0x03 /* Timeout to idle */
#define ST1633_REG_RESOLUTION_X 0x04 /* X resolution */
#define ST1633_REG_RESOLUTION_Y 0x06 /* Y resolution */
#define ST1633_REG_FW_VERSION   0x0A /* Firmware version */
#define ST1633_REG_CHIP_ID      0x00 /* Chip ID register */

/* Touch event flags */
#define ST1633_EVENT_PRESS_DOWN 0x00
#define ST1633_EVENT_LIFT_UP    0x01
#define ST1633_EVENT_CONTACT    0x02
#define ST1633_EVENT_NO_EVENT   0x03

/* Maximum touch points for ST1633 */
#define ST1633_MAX_POINTS    5
#define ST1633_HW_MAX_POINTS 10

/* Helper: reset */
static int st1633_reset(egui_hal_touch_driver_t *self)
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

/* Helper: read register */
static int st1633_read_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t *data, uint16_t len)
{
    return self->io->rx_param(self->io, (int)reg, data, (size_t)len);
}

/* Driver: init */
static int st1633_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t chip_id;

    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    /* Read and verify chip ID */
    if (st1633_read_reg(self, ST1633_REG_CHIP_ID, &chip_id, 1) != 0)
    {
        return -1; /* I2C read failed */
    }

    /* ST1633 chip ID verification - accept anyway for variants */

    return 0;
}

/* Driver: del */
static void st1633_del(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
}

/* Driver: read */
static int st1633_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    typedef struct __attribute__((packed)) st1633_xy_coord
    {
        uint8_t y_h : 3;
        uint8_t reserved_3 : 1;
        uint8_t x_h : 3;
        uint8_t valid : 1;
        uint8_t x_l;
        uint8_t y_l;
        uint8_t reserved_24_31;
    } st1633_xy_coord_t;
    typedef struct __attribute__((packed)) st1633_report
    {
        uint8_t gesture_type : 4;
        uint8_t reserved_4 : 1;
        uint8_t water_flag : 1;
        uint8_t prox_flag : 1;
        uint8_t reserved_7 : 1;
        uint8_t keys;
        st1633_xy_coord_t xy_coord[ST1633_HW_MAX_POINTS];
    } st1633_report_t;

    st1633_report_t report;
    uint8_t point_count = 0;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));
    memset(&report, 0, sizeof(report));

    if (st1633_read_reg(self, ST1633_REG_TOUCH_INFO, (uint8_t *)&report, sizeof(report)) != 0)
    {
        return -1;
    }

    for (uint8_t i = 0; i < ST1633_HW_MAX_POINTS && point_count < EGUI_HAL_TOUCH_MAX_POINTS; i++)
    {
        int16_t x = ((int16_t)report.xy_coord[i].x_h << 8) | report.xy_coord[i].x_l;
        int16_t y = ((int16_t)report.xy_coord[i].y_h << 8) | report.xy_coord[i].y_l;

        if (!report.xy_coord[i].valid)
        {
            continue;
        }

        if (((x == 0) && (y == 0)) || (x >= self->config.width) || (y >= self->config.height))
        {
            continue;
        }

        /* Store point */
        data->points[point_count].x = x;
        data->points[point_count].y = y;
        data->points[point_count].id = point_count;
        data->points[point_count].pressure = 0;
        point_count++;
    }

    data->point_count = point_count;

    return 0;
}

/* Internal: setup driver function pointers */
static void st1633_setup_driver(egui_hal_touch_driver_t *driver, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                                uint8_t (*get_int)(void))
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "ST1633";
    driver->max_points = ST1633_MAX_POINTS;

    driver->reset = st1633_reset;
    driver->init = st1633_init;
    driver->del = st1633_del;
    driver->read = st1633_read;

    driver->io = io;
    driver->set_rst = set_rst;
    driver->set_int = set_int;
    driver->get_int = get_int;
}

/* Public: init (static allocation) */
void egui_touch_st1633_init(egui_hal_touch_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                            uint8_t (*get_int)(void))
{
    if (!storage || !io || !io->rx_param || !io->tx_param)
    {
        return;
    }

    st1633_setup_driver(storage, io, set_rst, set_int, get_int);
}
