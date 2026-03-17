/**
 * @file egui_touch_ft5x06.c
 * @brief FT5x06 capacitive touch driver implementation
 */

#include "egui_touch_ft5x06.h"
#include <string.h>
#include "core/egui_api.h"

/* FT5x06 I2C address (7-bit, shifted for HAL) */
#define FT5X06_ADDR 0x70 /* 0x38 << 1 */

/* FT5x06 Registers */
#define FT5X06_REG_DEV_MODE  0x00
#define FT5X06_REG_GEST_ID   0x01
#define FT5X06_REG_TD_STATUS 0x02 /* Number of touch points */
#define FT5X06_REG_P1_XH     0x03 /* Point 1 X high + event flag */
#define FT5X06_REG_P1_XL     0x04 /* Point 1 X low */
#define FT5X06_REG_P1_YH     0x05 /* Point 1 Y high + touch ID */
#define FT5X06_REG_P1_YL     0x06 /* Point 1 Y low */

/* Touch point data offset: 6 bytes per point */
#define FT5X06_POINT_SIZE 6

/* Configuration registers */
#define FT5X06_REG_THGROUP      0x80 /* Valid touching detect threshold */
#define FT5X06_REG_THPEAK       0x81 /* Valid touching peak detect threshold */
#define FT5X06_REG_THCAL        0x82 /* Touch focus threshold */
#define FT5X06_REG_THWATER      0x83 /* Threshold when there is surface water */
#define FT5X06_REG_THTEMP       0x84 /* Threshold of temperature compensation */
#define FT5X06_REG_THDIFF       0x85 /* Touch difference threshold */
#define FT5X06_REG_CTRL         0x86 /* Power control mode */
#define FT5X06_REG_TIMEENTER    0x87 /* Delay to enter monitor status */
#define FT5X06_REG_PERIODACTIVE 0x88 /* Period of active status */
#define FT5X06_REG_PERIODMON    0x89 /* Timer to enter idle when in monitor */
#define FT5X06_REG_CIPHER       0xA3 /* Chip ID register */
#define FT5X06_REG_G_MODE       0xA4 /* Interrupt mode */
#define FT5X06_REG_PMODE        0xA5 /* Power mode */

/* Touch event flags */
#define FT5X06_EVENT_PRESS_DOWN 0x00
#define FT5X06_EVENT_LIFT_UP    0x01
#define FT5X06_EVENT_CONTACT    0x02
#define FT5X06_EVENT_NO_EVENT   0x03

/* Maximum touch points */
#define FT5X06_MAX_POINTS 5

/* Helper: hardware reset */
static void ft5x06_hw_reset(egui_hal_touch_driver_t *self)
{
    if (self->gpio && self->gpio->set_rst)
    {
        self->gpio->set_rst(0);
        /* Simple delay */
        egui_api_delay(5);
        self->gpio->set_rst(1);
        egui_api_delay(50);
    }
}

/* Helper: read register */
static int ft5x06_read_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t *data, uint16_t len)
{
    return self->bus.i2c->read_reg(FT5X06_ADDR, reg, data, len);
}

/* Helper: write register */
static int ft5x06_write_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t data)
{
    return self->bus.i2c->write_reg(FT5X06_ADDR, reg, &data, 1);
}

/* Helper: transform coordinates based on config */
static void ft5x06_transform_point(egui_hal_touch_driver_t *self, int16_t *x, int16_t *y)
{
    int16_t tx = *x;
    int16_t ty = *y;

    /* Swap X/Y */
    if (self->config.swap_xy)
    {
        int16_t tmp = tx;
        tx = ty;
        ty = tmp;
    }

    /* Mirror X */
    if (self->config.mirror_x)
    {
        tx = self->config.width - 1 - tx;
    }

    /* Mirror Y */
    if (self->config.mirror_y)
    {
        ty = self->config.height - 1 - ty;
    }

    *x = tx;
    *y = ty;
}

/* Driver: init */
static int ft5x06_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    /* Initialize bus and GPIO */
    if (self->bus.i2c->init)
    {
        self->bus.i2c->init();
    }
    if (self->gpio && self->gpio->init)
    {
        self->gpio->init();
    }

    /* Hardware reset */
    ft5x06_hw_reset(self);

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

/* Driver: deinit */
static void ft5x06_deinit(egui_hal_touch_driver_t *self)
{
    if (self->bus.i2c->deinit)
    {
        self->bus.i2c->deinit();
    }
    if (self->gpio && self->gpio->deinit)
    {
        self->gpio->deinit();
    }
}

/* Driver: read */
static int ft5x06_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t buf[FT5X06_MAX_POINTS * FT5X06_POINT_SIZE];
    uint8_t num_points;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));

    /* Read touch status */
    if (ft5x06_read_reg(self, FT5X06_REG_TD_STATUS, &num_points, 1) != 0)
    {
        return -1;
    }

    num_points &= 0x0F; /* Lower 4 bits = number of points */
    if (num_points > FT5X06_MAX_POINTS)
    {
        num_points = FT5X06_MAX_POINTS;
    }

    if (num_points == 0)
    {
        return 0; /* No touch */
    }

    /* Read touch point data */
    if (ft5x06_read_reg(self, FT5X06_REG_P1_XH, buf, num_points * FT5X06_POINT_SIZE) != 0)
    {
        return -1;
    }

    /* Parse touch points */
    for (uint8_t i = 0; i < num_points; i++)
    {
        uint8_t *p = &buf[i * FT5X06_POINT_SIZE];
        uint8_t event = (p[0] >> 6) & 0x03;

        /* Skip if no event or lift up */
        if (event == FT5X06_EVENT_NO_EVENT || event == FT5X06_EVENT_LIFT_UP)
        {
            continue;
        }

        int16_t x = ((p[0] & 0x0F) << 8) | p[1];
        int16_t y = ((p[2] & 0x0F) << 8) | p[3];
        uint8_t id = (p[2] >> 4) & 0x0F;

        /* Transform coordinates */
        ft5x06_transform_point(self, &x, &y);

        /* Store point */
        data->points[data->point_count].x = x;
        data->points[data->point_count].y = y;
        data->points[data->point_count].id = id;
        data->points[data->point_count].pressure = 0; /* Not supported */
        data->point_count++;
    }

    return 0;
}

/* Driver: enter_sleep */
static void ft5x06_enter_sleep(egui_hal_touch_driver_t *self)
{
    ft5x06_write_reg(self, FT5X06_REG_PMODE, 0x03); /* Hibernate mode */
}

/* Driver: exit_sleep */
static void ft5x06_exit_sleep(egui_hal_touch_driver_t *self)
{
    /* Hardware reset to wake up */
    ft5x06_hw_reset(self);
}

/* Internal: setup driver function pointers */
static void ft5x06_setup_driver(egui_hal_touch_driver_t *driver, const egui_bus_i2c_ops_t *i2c, const egui_touch_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "FT5x06";
    driver->bus_type = EGUI_BUS_TYPE_I2C;
    driver->max_points = FT5X06_MAX_POINTS;

    driver->init = ft5x06_init;
    driver->deinit = ft5x06_deinit;
    driver->read = ft5x06_read;
    driver->set_rotation = NULL; /* Use config swap/mirror instead */
    driver->enter_sleep = ft5x06_enter_sleep;
    driver->exit_sleep = ft5x06_exit_sleep;

    driver->bus.i2c = i2c;
    driver->gpio = gpio;
}

/* Public: init (static allocation) */
void egui_touch_ft5x06_init(egui_hal_touch_driver_t *storage, const egui_bus_i2c_ops_t *i2c, const egui_touch_gpio_ops_t *gpio)
{
    if (!storage || !i2c || !i2c->read_reg || !i2c->write_reg)
    {
        return;
    }

    ft5x06_setup_driver(storage, i2c, gpio);
}
