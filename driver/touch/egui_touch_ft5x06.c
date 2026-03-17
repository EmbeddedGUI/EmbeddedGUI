/**
 * @file egui_touch_ft5x06.c
 * @brief FT5x06 capacitive touch driver implementation
 */

#include "egui_touch_ft5x06.h"
#include <string.h>
#include "core/egui_api.h"

/* FT5x06 I2C address (7-bit, shifted for HAL) */
#define FT5X06_ADDR             0x70  /* 0x38 << 1 */

/* FT5x06 Registers */
#define FT5X06_REG_DEV_MODE     0x00
#define FT5X06_REG_GEST_ID      0x01
#define FT5X06_REG_TD_STATUS    0x02  /* Number of touch points */
#define FT5X06_REG_P1_XH        0x03  /* Point 1 X high + event flag */
#define FT5X06_REG_P1_XL        0x04  /* Point 1 X low */
#define FT5X06_REG_P1_YH        0x05  /* Point 1 Y high + touch ID */
#define FT5X06_REG_P1_YL        0x06  /* Point 1 Y low */

/* Touch point data offset: 6 bytes per point */
#define FT5X06_POINT_SIZE       6

/* Configuration registers */
#define FT5X06_REG_THGROUP      0x80  /* Valid touching detect threshold */
#define FT5X06_REG_THPEAK       0x81  /* Valid touching peak detect threshold */
#define FT5X06_REG_THCAL        0x82  /* Touch focus threshold */
#define FT5X06_REG_THWATER      0x83  /* Threshold when there is surface water */
#define FT5X06_REG_THTEMP       0x84  /* Threshold of temperature compensation */
#define FT5X06_REG_THDIFF       0x85  /* Touch difference threshold */
#define FT5X06_REG_CTRL         0x86  /* Power control mode */
#define FT5X06_REG_TIMEENTER    0x87  /* Delay to enter monitor status */
#define FT5X06_REG_PERIODACTIVE 0x88  /* Period of active status */
#define FT5X06_REG_PERIODMON    0x89  /* Timer to enter idle when in monitor */
#define FT5X06_REG_CIPHER       0xA3  /* Chip ID register */
#define FT5X06_REG_G_MODE       0xA4  /* Interrupt mode */
#define FT5X06_REG_PMODE        0xA5  /* Power mode */

/* Touch event flags */
#define FT5X06_EVENT_PRESS_DOWN 0x00
#define FT5X06_EVENT_LIFT_UP    0x01
#define FT5X06_EVENT_CONTACT    0x02
#define FT5X06_EVENT_NO_EVENT   0x03

/* Maximum touch points */
#define FT5X06_MAX_POINTS       5

/* Helper: reset */
static int ft5x06_reset(egui_hal_touch_driver_t *self)
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
static int ft5x06_read_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t *data, uint16_t len)
{
    return self->io->rx_param(self->io, (int)reg, data, (size_t)len);
}

/* Helper: write register */
static int ft5x06_write_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t data)
{
    return self->io->tx_param(self->io, (int)reg, &data, 1);
}

/* Driver: init */
static int ft5x06_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    /* Configure touch controller */
    ft5x06_write_reg(self, FT5X06_REG_THGROUP, 70);      /* Valid touching detect threshold */
    ft5x06_write_reg(self, FT5X06_REG_THPEAK, 60);       /* Valid touching peak detect threshold */
    ft5x06_write_reg(self, FT5X06_REG_THCAL, 16);        /* Touch focus threshold */
    ft5x06_write_reg(self, FT5X06_REG_THWATER, 60);      /* Threshold when there is surface water */
    ft5x06_write_reg(self, FT5X06_REG_THTEMP, 10);       /* Threshold of temperature compensation */
    ft5x06_write_reg(self, FT5X06_REG_THDIFF, 20);       /* Touch difference threshold */
    ft5x06_write_reg(self, FT5X06_REG_TIMEENTER, 2);     /* Delay to enter monitor status (s) */
    ft5x06_write_reg(self, FT5X06_REG_PERIODACTIVE, 12); /* Period of active status (ms) */
    ft5x06_write_reg(self, FT5X06_REG_PERIODMON, 40);    /* Timer to enter idle when in monitor (ms) */

    return 0;
}

/* Driver: del */
static void ft5x06_del(egui_hal_touch_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
}

/* Driver: read */
static int ft5x06_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t buf[FT5X06_MAX_POINTS * FT5X06_POINT_SIZE];
    uint8_t num_points;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));

    /* Read touch status */
    if (ft5x06_read_reg(self, FT5X06_REG_TD_STATUS, &num_points, 1) != 0) {
        return -1;
    }

    num_points &= 0x0F;  /* Lower 4 bits = number of points */
    if (num_points > FT5X06_MAX_POINTS) {
        num_points = FT5X06_MAX_POINTS;
    }

    if (num_points == 0) {
        return 0;  /* No touch */
    }

    /* Read touch point data */
    if (ft5x06_read_reg(self, FT5X06_REG_P1_XH, buf, num_points * FT5X06_POINT_SIZE) != 0) {
        return -1;
    }

    /* Parse touch points */
    for (uint8_t i = 0; i < num_points; i++) {
        uint8_t *p = &buf[i * FT5X06_POINT_SIZE];
        uint8_t event = (p[0] >> 6) & 0x03;

        /* Skip if no event or lift up */
        if (event == FT5X06_EVENT_NO_EVENT || event == FT5X06_EVENT_LIFT_UP) {
            continue;
        }

        int16_t x = ((p[0] & 0x0F) << 8) | p[1];
        int16_t y = ((p[2] & 0x0F) << 8) | p[3];
        uint8_t id = (p[2] >> 4) & 0x0F;

        /* Store point */
        data->points[data->point_count].x = x;
        data->points[data->point_count].y = y;
        data->points[data->point_count].id = id;
        data->points[data->point_count].pressure = 0;  /* Not supported */
        data->point_count++;
    }

    return 0;
}

/* Internal: setup driver function pointers */
static void ft5x06_setup_driver(egui_hal_touch_driver_t *driver,
                                 egui_panel_io_handle_t io,
                                 void (*set_rst)(uint8_t level),
                                 void (*set_int)(uint8_t level),
                                 uint8_t (*get_int)(void))
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "FT5x06";
    driver->max_points = FT5X06_MAX_POINTS;

    driver->reset = ft5x06_reset;
    driver->init = ft5x06_init;
    driver->del = ft5x06_del;
    driver->read = ft5x06_read;

    driver->io = io;
    driver->set_rst = set_rst;
    driver->set_int = set_int;
    driver->get_int = get_int;
}

/* Public: init (static allocation) */
void egui_touch_ft5x06_init(egui_hal_touch_driver_t *storage,
                            egui_panel_io_handle_t io,
                            void (*set_rst)(uint8_t level),
                            void (*set_int)(uint8_t level),
                            uint8_t (*get_int)(void))
{
    if (!storage || !io || !io->rx_param || !io->tx_param) {
        return;
    }

    ft5x06_setup_driver(storage, io, set_rst, set_int, get_int);
}
