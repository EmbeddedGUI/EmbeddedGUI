/**
 * @file egui_touch_gt1151.c
 * @brief GT1151 capacitive touch driver implementation
 */

#include "egui_touch_gt1151.h"
#include <string.h>
#include "core/egui_api.h"

/* GT1151 I2C address (7-bit, shifted for HAL) */
#define GT1151_ADDR_DEFAULT 0x28 /* 0x14 << 1 */
#define GT1151_ADDR_BACKUP  0xBA /* 0x5D << 1 */

/* GT1151 Registers (16-bit addresses, same as GT911) */
#define GT1151_REG_PRODUCT_ID  0x8140 /* Product ID (4 bytes: "1151") */
#define GT1151_REG_CONFIG      0x8047 /* Configuration register */
#define GT1151_REG_READ_XY     0x814E /* Touch status and data */
#define GT1151_REG_ENTER_SLEEP 0x8040 /* Sleep control register */

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
#define GT1151_MAX_POINTS 5

/* Current I2C address (can be changed based on INT pin state) */
static uint8_t gt1151_i2c_addr = GT1151_ADDR_DEFAULT;

/* Helper: hardware reset */
static void gt1151_hw_reset(egui_hal_touch_driver_t *self)
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

/* Helper: read register (16-bit address) */
static int gt1151_read_reg(egui_hal_touch_driver_t *self, uint16_t reg, uint8_t *data, uint16_t len)
{
    return self->bus.i2c->read_reg(gt1151_i2c_addr, reg, data, len);
}

/* Helper: write register (16-bit address) */
static int gt1151_write_reg(egui_hal_touch_driver_t *self, uint16_t reg, uint8_t *data, uint16_t len)
{
    return self->bus.i2c->write_reg(gt1151_i2c_addr, reg, data, len);
}

/* Helper: transform coordinates based on config */
static void gt1151_transform_point(egui_hal_touch_driver_t *self, int16_t *x, int16_t *y)
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
static int gt1151_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t product_id[4];

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
    gt1151_hw_reset(self);

    /* Try default address first */
    gt1151_i2c_addr = GT1151_ADDR_DEFAULT;
    if (gt1151_read_reg(self, GT1151_REG_PRODUCT_ID, product_id, 4) != 0)
    {
        /* Try backup address */
        gt1151_i2c_addr = GT1151_ADDR_BACKUP;
        if (gt1151_read_reg(self, GT1151_REG_PRODUCT_ID, product_id, 4) != 0)
        {
            return -1; /* I2C read failed */
        }
    }

    /* Verify product ID (should be "1151" or similar) */
    /* Accept anyway - some variants have different IDs */

    return 0;
}

/* Driver: deinit */
static void gt1151_deinit(egui_hal_touch_driver_t *self)
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
static int gt1151_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t status;
    uint8_t buf[GT1151_MAX_POINTS * 8]; /* 8 bytes per point */
    uint8_t num_points;
    uint8_t clear = 0;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));

    /* Read touch status */
    if (gt1151_read_reg(self, GT1151_REG_READ_XY, &status, 1) != 0)
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
    if (num_points > GT1151_MAX_POINTS)
    {
        num_points = GT1151_MAX_POINTS;
    }

    if (num_points > 0)
    {
        /* Read touch point data */
        if (gt1151_read_reg(self, GT1151_REG_READ_XY + 1, buf, num_points * 8) != 0)
        {
            /* Clear status anyway */
            gt1151_write_reg(self, GT1151_REG_READ_XY, &clear, 1);
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

            /* Transform coordinates */
            gt1151_transform_point(self, &x, &y);

            /* Store point */
            data->points[data->point_count].x = x;
            data->points[data->point_count].y = y;
            data->points[data->point_count].id = id;
            data->points[data->point_count].pressure = size;
            data->point_count++;
        }
    }

    /* Clear status register (write 0 to 0x814E) */
    gt1151_write_reg(self, GT1151_REG_READ_XY, &clear, 1);

    return 0;
}

/* Driver: enter_sleep */
static void gt1151_enter_sleep(egui_hal_touch_driver_t *self)
{
    uint8_t cmd = 0x05; /* Sleep command */
    gt1151_write_reg(self, GT1151_REG_ENTER_SLEEP, &cmd, 1);
}

/* Driver: exit_sleep */
static void gt1151_exit_sleep(egui_hal_touch_driver_t *self)
{
    /* Hardware reset to wake up */
    gt1151_hw_reset(self);
}

/* Internal: setup driver function pointers */
static void gt1151_setup_driver(egui_hal_touch_driver_t *driver, const egui_bus_i2c_ops_t *i2c, const egui_touch_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "GT1151";
    driver->bus_type = EGUI_BUS_TYPE_I2C;
    driver->max_points = GT1151_MAX_POINTS;

    driver->init = gt1151_init;
    driver->deinit = gt1151_deinit;
    driver->read = gt1151_read;
    driver->set_rotation = NULL; /* Use config swap/mirror instead */
    driver->enter_sleep = gt1151_enter_sleep;
    driver->exit_sleep = gt1151_exit_sleep;

    driver->bus.i2c = i2c;
    driver->gpio = gpio;
}

/* Public: init (static allocation) */
void egui_touch_gt1151_init(egui_hal_touch_driver_t *storage, const egui_bus_i2c_ops_t *i2c, const egui_touch_gpio_ops_t *gpio)
{
    if (!storage || !i2c || !i2c->read_reg || !i2c->write_reg)
    {
        return;
    }

    gt1151_setup_driver(storage, i2c, gpio);
}
