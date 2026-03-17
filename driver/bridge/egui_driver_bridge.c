/**
 * @file egui_driver_bridge.c
 * @brief Bridge layer implementation
 */

#include "egui_driver_bridge.h"
#include "egui.h"
#include "core/egui_touch_driver.h"
#include <string.h>

/* ============================================================
 * Display Bridge
 * ============================================================ */

static egui_hal_lcd_driver_t *s_lcd_driver = NULL;
static egui_display_driver_t s_display_driver;
static egui_display_driver_ops_t s_display_ops;

static void bridge_display_init(void)
{
    /* LCD init is called separately via lcd->init() */
}

static void bridge_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    if (s_lcd_driver)
    {
        s_lcd_driver->set_window(s_lcd_driver, x, y, w, h);
        s_lcd_driver->write_pixels(s_lcd_driver, data, w * h * sizeof(egui_color_int_t));
    }
}

static void bridge_display_wait_draw_complete(void)
{
    if (s_lcd_driver && s_lcd_driver->wait_dma_complete)
    {
        s_lcd_driver->wait_dma_complete(s_lcd_driver);
    }
}

static void bridge_display_flush(void)
{
    /* No-op for most LCD drivers */
}

static void bridge_display_set_brightness(uint8_t level)
{
    if (s_lcd_driver && s_lcd_driver->set_brightness)
    {
        s_lcd_driver->set_brightness(s_lcd_driver, level);
    }
}

static void bridge_display_set_power(uint8_t on)
{
    if (s_lcd_driver && s_lcd_driver->set_power)
    {
        s_lcd_driver->set_power(s_lcd_driver, on);
    }
}

static void bridge_display_set_rotation(egui_display_rotation_t rotation)
{
    if (s_lcd_driver && s_lcd_driver->set_rotation)
    {
        s_lcd_driver->set_rotation(s_lcd_driver, (uint8_t)rotation);
    }
}

egui_display_driver_t *egui_display_driver_from_lcd(egui_hal_lcd_driver_t *lcd)
{
    if (!lcd)
    {
        return NULL;
    }

    s_lcd_driver = lcd;

    /* Setup ops */
    memset(&s_display_ops, 0, sizeof(s_display_ops));
    s_display_ops.init = bridge_display_init;
    s_display_ops.draw_area = bridge_display_draw_area;
    s_display_ops.wait_draw_complete = lcd->wait_dma_complete ? bridge_display_wait_draw_complete : NULL;
    s_display_ops.flush = bridge_display_flush;
    s_display_ops.set_brightness = lcd->set_brightness ? bridge_display_set_brightness : NULL;
    s_display_ops.set_power = lcd->set_power ? bridge_display_set_power : NULL;
    s_display_ops.set_rotation = lcd->set_rotation ? bridge_display_set_rotation : NULL;

    /* Setup driver */
    memset(&s_display_driver, 0, sizeof(s_display_driver));
    s_display_driver.ops = &s_display_ops;
    s_display_driver.physical_width = lcd->config.width;
    s_display_driver.physical_height = lcd->config.height;
    s_display_driver.rotation = EGUI_DISPLAY_ROTATION_0;
    s_display_driver.brightness = 255;
    s_display_driver.power_on = 1;

    return &s_display_driver;
}

/* ============================================================
 * Touch Bridge
 * ============================================================ */

static egui_hal_touch_driver_t *s_hal_touch_driver = NULL;
static egui_touch_driver_t s_core_touch_driver;
static egui_touch_driver_ops_t s_core_touch_ops;
static int16_t s_last_touch_x = 0;
static int16_t s_last_touch_y = 0;

/**
 * Bridge touch read function.
 * Called by Core's egui_input_polling_work() via egui_touch_driver_t interface.
 */
static void bridge_touch_read(uint8_t *pressed, int16_t *x, int16_t *y)
{
    *pressed = 0;
    *x = s_last_touch_x;
    *y = s_last_touch_y;

    if (!s_hal_touch_driver || !s_hal_touch_driver->read)
    {
        return;
    }

    /* Check INT pin if available - skip read if no interrupt pending */
    if (s_hal_touch_driver->gpio && s_hal_touch_driver->gpio->get_int)
    {
        if (!s_hal_touch_driver->gpio->get_int())
        {
            return; /* No interrupt pending */
        }
    }

    egui_hal_touch_data_t data;
    if (s_hal_touch_driver->read(s_hal_touch_driver, &data) != 0)
    {
        return; /* Read failed */
    }

    if (data.point_count > 0)
    {
        *pressed = 1;
        *x = data.points[0].x;
        *y = data.points[0].y;
        s_last_touch_x = *x;
        s_last_touch_y = *y;
    }
}

void egui_touch_driver_bridge_register(egui_hal_touch_driver_t *touch)
{
    s_hal_touch_driver = touch;
    s_last_touch_x = 0;
    s_last_touch_y = 0;

    /* Setup Core touch driver ops */
    s_core_touch_ops.init = NULL; /* HAL driver init is called separately */
    s_core_touch_ops.read = bridge_touch_read;

    /* Setup Core touch driver */
    s_core_touch_driver.ops = &s_core_touch_ops;

    /* Register with Core - egui_input_polling_work() will call our read function */
    egui_touch_driver_register(&s_core_touch_driver);
}
