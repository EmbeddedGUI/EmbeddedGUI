/**
 * @file egui_touch_cst820.c
 * @brief CST820 capacitive touch driver implementation
 *
 * CST820 is an upgraded version of CST816S with improved performance.
 */

#include "egui_touch_cst820.h"
#include <string.h>
#include "core/egui_api.h"

/* CST820 I2C address (7-bit, shifted for HAL) */
#define CST820_ADDR             0x2A  /* 0x15 << 1 */

/* CST820 Registers */
#define CST820_REG_GEST_ID      0x00  /* Gesture ID */
#define CST820_REG_FINGER_NUM   0x01  /* Number of touch points */
#define CST820_REG_XH           0x02  /* X position high + event flag */
#define CST820_REG_XL           0x03  /* X position low */
#define CST820_REG_YH           0x04  /* Y position high + touch ID */
#define CST820_REG_YL           0x05  /* Y position low */
#define CST820_REG_CHIP_ID      0xA7  /* Chip ID register */
#define CST820_REG_SLEEP        0xE5  /* Sleep mode register */

/* Expected chip ID */
#define CST820_CHIP_ID          0xB7

/* Touch event flags */
#define CST820_EVENT_PRESS_DOWN 0x00
#define CST820_EVENT_LIFT_UP    0x01
#define CST820_EVENT_CONTACT    0x02
#define CST820_EVENT_NO_EVENT   0x03

/* Gesture IDs */
#define CST820_GEST_NONE        0x00
#define CST820_GEST_SLIDE_UP    0x01
#define CST820_GEST_SLIDE_DOWN  0x02
#define CST820_GEST_SLIDE_LEFT  0x03
#define CST820_GEST_SLIDE_RIGHT 0x04
#define CST820_GEST_SINGLE_CLICK 0x05
#define CST820_GEST_DOUBLE_CLICK 0x0B
#define CST820_GEST_LONG_PRESS  0x0C

/* Helper: reset */
static int cst820_reset(egui_hal_touch_driver_t *self)
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
static int cst820_read_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t *data, uint16_t len)
{
    return self->io->rx_param(self->io, (int)reg, data, (size_t)len);
}

/* Driver: init */
static int cst820_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t chip_id;

    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    /* Read and verify chip ID */
    if (cst820_read_reg(self, CST820_REG_CHIP_ID, &chip_id, 1) != 0) {
        return -1;  /* I2C read failed */
    }

    if (chip_id != CST820_CHIP_ID) {
        /* Accept anyway - some variants have different IDs */
    }

    return 0;
}

/* Driver: del */
static void cst820_del(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
}

/* Driver: read */
static int cst820_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t buf[6];  /* Read touch data: gest_id + finger_num + XH + XL + YH + YL */
    uint8_t num_points;
    uint8_t gesture;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));

    /* Read gesture and touch data starting from register 0x00 */
    if (cst820_read_reg(self, CST820_REG_GEST_ID, buf, 6) != 0) {
        return -1;
    }

    gesture = buf[0];
    num_points = buf[1] & 0x0F;  /* Lower 4 bits = number of points */

    /* Store gesture ID */
    data->gesture = gesture;

    if (num_points > 1) {
        num_points = 1;  /* CST820 supports max 1 point */
    }

    if (num_points == 0) {
        return 0;  /* No touch */
    }

    int16_t x = ((buf[2] & 0x0F) << 8) | buf[3];
    int16_t y = ((buf[4] & 0x0F) << 8) | buf[5];

    /* Store point */
    data->points[0].x = x;
    data->points[0].y = y;
    data->points[0].id = 0;
    data->points[0].pressure = 0;  /* Not supported */
    data->point_count = 1;

    return 0;
}

/* Internal: setup driver function pointers */
static void cst820_setup_driver(egui_hal_touch_driver_t *driver,
                                 egui_panel_io_handle_t io,
                                 void (*set_rst)(uint8_t level),
                                 void (*set_int)(uint8_t level),
                                 uint8_t (*get_int)(void))
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "CST820";
    driver->max_points = 1;

    driver->reset = cst820_reset;
    driver->init = cst820_init;
    driver->del = cst820_del;
    driver->read = cst820_read;

    driver->io = io;
    driver->set_rst = set_rst;
    driver->set_int = set_int;
    driver->get_int = get_int;
}

/* Public: init (static allocation) */
void egui_touch_cst820_init(egui_hal_touch_driver_t *storage,
                             egui_panel_io_handle_t io,
                             void (*set_rst)(uint8_t level),
                             void (*set_int)(uint8_t level),
                             uint8_t (*get_int)(void))
{
    if (!storage || !io || !io->rx_param || !io->tx_param) {
        return;
    }

    cst820_setup_driver(storage, io, set_rst, set_int, get_int);
}
