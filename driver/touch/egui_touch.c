/**
 * @file egui_touch.c
 * @brief Touch driver common helper functions
 */

#include <string.h>
#include "egui_touch.h"
#include "core/egui_touch_driver.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

static egui_hal_touch_driver_t *s_hal_touch = NULL;
static egui_touch_driver_t s_core_touch;
static egui_touch_driver_ops_t s_core_touch_ops;
static uint8_t s_hal_touch_last_position_valid = 0;
static int16_t s_hal_touch_last_x = 0;
static int16_t s_hal_touch_last_y = 0;

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

static void hal_touch_read_ex(uint8_t *pressed, int16_t *x, int16_t *y, uint8_t *has_position)
{
    *pressed = 0;
    *x = s_hal_touch_last_x;
    *y = s_hal_touch_last_y;
    *has_position = s_hal_touch_last_position_valid;

    if (!s_hal_touch || !s_hal_touch->read)
    {
        return;
    }

    /* Skip read if INT pin available and no interrupt pending */
    if (s_hal_touch->get_int)
    {
        if (!s_hal_touch->get_int())
        {
            return;
        }
    }

    egui_hal_touch_data_t data;
    if (s_hal_touch->read(s_hal_touch, &data) == 0)
    {
        const egui_hal_touch_config_t *cfg = &s_hal_touch->config;

        if (data.has_release_point)
        {
            hal_touch_transform_point(cfg, &data.release_point);
            *x = data.release_point.x;
            *y = data.release_point.y;
            *has_position = 1;
            s_hal_touch_last_x = *x;
            s_hal_touch_last_y = *y;
            s_hal_touch_last_position_valid = 1;
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
            s_hal_touch_last_x = *x;
            s_hal_touch_last_y = *y;
            s_hal_touch_last_position_valid = 1;
        }
    }
}

static void hal_touch_read(uint8_t *pressed, int16_t *x, int16_t *y)
{
    uint8_t has_position = 0;

    hal_touch_read_ex(pressed, x, y, &has_position);
    if (!*pressed)
    {
        *x = 0;
        *y = 0;
    }
}

void egui_hal_touch_register(egui_hal_touch_driver_t *touch, const egui_hal_touch_config_t *config)
{
    s_hal_touch = touch;

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
    s_core_touch_ops.init = NULL;
    s_core_touch_ops.read = hal_touch_read;
    s_core_touch_ops.read_ex = hal_touch_read_ex;

    s_core_touch.ops = &s_core_touch_ops;
    s_hal_touch_last_position_valid = 0;
    s_hal_touch_last_x = 0;
    s_hal_touch_last_y = 0;

    egui_touch_driver_register(&s_core_touch);
}

#else

void egui_hal_touch_register(egui_hal_touch_driver_t *touch, const egui_hal_touch_config_t *config)
{
    (void)touch;
    (void)config;
}

#endif
