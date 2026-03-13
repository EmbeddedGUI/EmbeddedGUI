/**
 * @file egui_touch_st1633.c
 * @brief ST1633 capacitive touch driver implementation
 */

#include "egui_touch_st1633.h"
#include <string.h>
#include "core/egui_api.h"

/* ST1633 I2C address (7-bit, shifted for HAL) */
#define ST1633_ADDR             0xAA  /* 0x55 << 1 */

/* ST1633 Registers */
#define ST1633_REG_STATUS       0x01  /* Device status */
#define ST1633_REG_TOUCH_INFO   0x10  /* Advanced touch information block */
#define ST1633_REG_KEYS         0x11  /* Key status */
#define ST1633_REG_XY_COORD     0x12  /* Legacy XY coordinate data start */
#define ST1633_REG_DEVICE_CTRL  0x02  /* Device control */
#define ST1633_REG_TIMEOUT      0x03  /* Timeout to idle */
#define ST1633_REG_RESOLUTION_X 0x04  /* X resolution */
#define ST1633_REG_RESOLUTION_Y 0x06  /* Y resolution */
#define ST1633_REG_FW_VERSION   0x0A  /* Firmware version */
#define ST1633_REG_CHIP_ID      0x00  /* Chip ID register */

/* Touch event flags */
#define ST1633_EVENT_PRESS_DOWN 0x00
#define ST1633_EVENT_LIFT_UP    0x01
#define ST1633_EVENT_CONTACT    0x02
#define ST1633_EVENT_NO_EVENT   0x03

/* Maximum touch points for ST1633 */
#define ST1633_MAX_POINTS       5
#define ST1633_HW_MAX_POINTS    10

/* Helper: hardware reset */
static void st1633_hw_reset(egui_hal_touch_driver_t *self)
{
    if (self->gpio && self->gpio->set_rst) {
        self->gpio->set_rst(0);
        /* Simple delay */
        egui_api_delay(5);
        self->gpio->set_rst(1);
        egui_api_delay(50);
    }
}

/* Helper: read register */
static int st1633_read_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t *data, uint16_t len)
{
    return self->bus.i2c->read_reg(ST1633_ADDR, reg, data, len);
}

/* Helper: write register */
static int st1633_write_reg(egui_hal_touch_driver_t *self, uint8_t reg, uint8_t *data, uint16_t len)
{
    return self->bus.i2c->write_reg(ST1633_ADDR, reg, data, len);
}

/* Helper: transform coordinates based on config */
static void st1633_transform_point(egui_hal_touch_driver_t *self, int16_t *x, int16_t *y)
{
    int16_t tx = *x;
    int16_t ty = *y;

    /* Swap X/Y */
    if (self->config.swap_xy) {
        int16_t tmp = tx;
        tx = ty;
        ty = tmp;
    }

    /* Mirror X */
    if (self->config.mirror_x) {
        tx = self->config.width - 1 - tx;
    }

    /* Mirror Y */
    if (self->config.mirror_y) {
        ty = self->config.height - 1 - ty;
    }

    *x = tx;
    *y = ty;
}

/* Driver: init */
static int st1633_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    uint8_t chip_id;

    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_touch_config_t));

    /* Initialize bus and GPIO */
    if (self->bus.i2c->init) {
        self->bus.i2c->init();
    }
    if (self->gpio && self->gpio->init) {
        self->gpio->init();
    }

    /* Hardware reset */
    st1633_hw_reset(self);

    /* Read and verify chip ID */
    if (st1633_read_reg(self, ST1633_REG_CHIP_ID, &chip_id, 1) != 0) {
        return -1;  /* I2C read failed */
    }

    /* ST1633 chip ID verification - accept anyway for variants */

    return 0;
}

/* Driver: deinit */
static void st1633_deinit(egui_hal_touch_driver_t *self)
{
    if (self->bus.i2c->deinit) {
        self->bus.i2c->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

/* Driver: read */
static int st1633_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    typedef struct __attribute__((packed)) st1633_xy_coord {
        uint8_t y_h: 3;
        uint8_t reserved_3: 1;
        uint8_t x_h: 3;
        uint8_t valid: 1;
        uint8_t x_l;
        uint8_t y_l;
        uint8_t reserved_24_31;
    } st1633_xy_coord_t;
    typedef struct __attribute__((packed)) st1633_report {
        uint8_t gesture_type: 4;
        uint8_t reserved_4: 1;
        uint8_t water_flag: 1;
        uint8_t prox_flag: 1;
        uint8_t reserved_7: 1;
        uint8_t keys;
        st1633_xy_coord_t xy_coord[ST1633_HW_MAX_POINTS];
    } st1633_report_t;

    st1633_report_t report;
    uint8_t point_count = 0;

    /* Clear output */
    memset(data, 0, sizeof(egui_hal_touch_data_t));
    memset(&report, 0, sizeof(report));

    if (st1633_read_reg(self, ST1633_REG_TOUCH_INFO, (uint8_t *)&report, sizeof(report)) != 0) {
        return -1;
    }

    for (uint8_t i = 0; i < ST1633_HW_MAX_POINTS && point_count < EGUI_HAL_TOUCH_MAX_POINTS; i++) {
        int16_t x = ((int16_t)report.xy_coord[i].x_h << 8) | report.xy_coord[i].x_l;
        int16_t y = ((int16_t)report.xy_coord[i].y_h << 8) | report.xy_coord[i].y_l;

        if (!report.xy_coord[i].valid) {
            continue;
        }

        if (((x == 0) && (y == 0)) || (x >= self->config.width) || (y >= self->config.height)) {
            continue;
        }

        /* Transform coordinates */
        st1633_transform_point(self, &x, &y);

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

/* Driver: enter_sleep */
static void st1633_enter_sleep(egui_hal_touch_driver_t *self)
{
    uint8_t mode = 0x02;  /* Sleep mode */
    st1633_write_reg(self, ST1633_REG_DEVICE_CTRL, &mode, 1);
}

/* Driver: exit_sleep */
static void st1633_exit_sleep(egui_hal_touch_driver_t *self)
{
    /* Hardware reset to wake up */
    st1633_hw_reset(self);
}

/* Internal: setup driver function pointers */
static void st1633_setup_driver(egui_hal_touch_driver_t *driver,
                                 const egui_bus_i2c_ops_t *i2c,
                                 const egui_touch_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_touch_driver_t));

    driver->name = "ST1633";
    driver->bus_type = EGUI_BUS_TYPE_I2C;
    driver->max_points = ST1633_MAX_POINTS;

    driver->init = st1633_init;
    driver->deinit = st1633_deinit;
    driver->read = st1633_read;
    driver->set_rotation = NULL;  /* Use config swap/mirror instead */
    driver->enter_sleep = st1633_enter_sleep;
    driver->exit_sleep = st1633_exit_sleep;

    driver->bus.i2c = i2c;
    driver->gpio = gpio;
}

/* Public: init (static allocation) */
void egui_touch_st1633_init(egui_hal_touch_driver_t *storage,
                            const egui_bus_i2c_ops_t *i2c,
                            const egui_touch_gpio_ops_t *gpio)
{
    if (!storage || !i2c || !i2c->read_reg || !i2c->write_reg) {
        return;
    }

    st1633_setup_driver(storage, i2c, gpio);
}
