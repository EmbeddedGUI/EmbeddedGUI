/**
 * @file egui_touch.c
 * @brief Touch driver common helper functions
 */

#include <string.h>
#include "egui_touch.h"
#include "core/egui_touch_driver.h"

static egui_hal_touch_driver_t *s_hal_touch = NULL;
static egui_touch_driver_t s_core_touch;
static egui_touch_driver_ops_t s_core_touch_ops;

static void hal_touch_read(uint8_t *pressed, int16_t *x, int16_t *y)
{
    *pressed = 0;

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
    if (s_hal_touch->read(s_hal_touch, &data) == 0 && data.point_count > 0)
    {
        /* Apply coordinate transform for all points */
        const egui_hal_touch_config_t *cfg = &s_hal_touch->config;
        for (uint8_t i = 0; i < data.point_count; i++)
        {
            int16_t tx = data.points[i].x;
            int16_t ty = data.points[i].y;

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

            data.points[i].x = tx;
            data.points[i].y = ty;
        }

        *pressed = 1;
        *x = data.points[0].x;
        *y = data.points[0].y;
    }
}

void egui_hal_touch_register(egui_hal_touch_driver_t *touch,
                              const egui_hal_touch_config_t *config)
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

    s_core_touch.ops = &s_core_touch_ops;

    egui_touch_driver_register(&s_core_touch);
}
