/**
 * @file egui_touch.c
 * @brief Touch driver common helper functions
 */

#include <string.h>
#include "egui_touch.h"
#include "core/egui_core.h"
#include "core/egui_touch_driver.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

static void hal_touch_transform_point(const egui_hal_touch_config_t *cfg, egui_hal_touch_point_t *point)
{
    int16_t tx = point->x;
    int16_t ty = point->y;

    if (cfg->swap_xy)
    {
        int16_t tmp = tx;
        tx = ty;
        ty = tmp;
    }
    if (cfg->mirror_x)
    {
        tx = cfg->width - 1 - tx;
    }
    if (cfg->mirror_y)
    {
        ty = cfg->height - 1 - ty;
    }

    point->x = tx;
    point->y = ty;
}

static void hal_touch_read_ex(egui_core_t *core, uint8_t *pressed, int16_t *x, int16_t *y, uint8_t *has_position)
{
    egui_hal_touch_driver_t *hal_touch = (egui_hal_touch_driver_t *)core->touch.hal_bridge_hal_driver;

    *pressed = core->touch.hal_touch_last_pressed;
    *x = core->touch.hal_touch_last_x;
    *y = core->touch.hal_touch_last_y;
    *has_position = core->touch.hal_touch_last_position_valid;

    if (!hal_touch || !hal_touch->read)
    {
        return;
    }

    /*
     * Some touch controllers expose INT as an event pulse rather than a
     * level that stays asserted for the whole press. Only skip the read when
     * there is no active touch state; otherwise keep reading until the
     * controller explicitly reports release.
     */
    if (hal_touch->get_int)
    {
        if (!hal_touch->get_int() && !core->touch.hal_touch_last_pressed)
        {
            *pressed = 0;
            return;
        }
    }

    egui_hal_touch_data_t data;
    if (hal_touch->read(hal_touch, core, &data) == 0)
    {
        const egui_hal_touch_config_t *cfg = &hal_touch->config;

        if (data.has_release_point)
        {
            hal_touch_transform_point(cfg, &data.release_point);
            *x = data.release_point.x;
            *y = data.release_point.y;
            *has_position = 1;
            core->touch.hal_touch_last_x = *x;
            core->touch.hal_touch_last_y = *y;
            core->touch.hal_touch_last_position_valid = 1;
        }

        if (data.point_count > 0)
        {
            for (uint8_t i = 0; i < data.point_count; i++)
            {
                hal_touch_transform_point(cfg, &data.points[i]);
            }

            *pressed = 1;
            *x = data.points[0].x;
            *y = data.points[0].y;
            *has_position = 1;
            core->touch.hal_touch_last_x = *x;
            core->touch.hal_touch_last_y = *y;
            core->touch.hal_touch_last_position_valid = 1;
        }

        if (data.point_count == 0)
        {
            *pressed = 0;
        }
    }

    core->touch.hal_touch_last_pressed = *pressed;
}

static void hal_touch_read(egui_core_t *core, uint8_t *pressed, int16_t *x, int16_t *y)
{
    uint8_t has_position = 0;

    hal_touch_read_ex(core, pressed, x, y, &has_position);
    if (!*pressed)
    {
        *x = 0;
        *y = 0;
    }
}

void egui_hal_touch_register(egui_core_t *core, egui_hal_touch_driver_t *touch, const egui_hal_touch_config_t *config)
{
    /* Reset and initialize the HAL touch driver */
    if (touch->reset)
    {
        touch->reset(touch);
    }
    if (touch->init)
    {
        touch->init(touch, config);
    }

    /* Register with Core */
    core->touch.hal_bridge_hal_driver = touch;
    core->touch.hal_bridge_ops.init = NULL;
    core->touch.hal_bridge_ops.read = hal_touch_read;
    core->touch.hal_bridge_ops.read_ex = hal_touch_read_ex;
    core->touch.hal_bridge_driver.ops = &core->touch.hal_bridge_ops;
    core->touch.hal_touch_last_position_valid = 0;
    core->touch.hal_touch_last_pressed = 0;
    core->touch.hal_touch_last_x = 0;
    core->touch.hal_touch_last_y = 0;

    egui_touch_driver_register(core, &core->touch.hal_bridge_driver);
}

#else

void egui_hal_touch_register(egui_core_t *core, egui_hal_touch_driver_t *touch, const egui_hal_touch_config_t *config)
{
    (void)core;
    (void)touch;
    (void)config;
}

#endif
