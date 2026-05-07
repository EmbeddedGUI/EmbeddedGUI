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

static int hal_touch_read(egui_core_t *core, egui_touch_driver_data_t *out_data)
{
    egui_hal_touch_driver_t *hal_touch;

    if (out_data == NULL)
    {
        return -1;
    }

    memset(out_data, 0, sizeof(*out_data));
    if (core == NULL)
    {
        return -1;
    }

    hal_touch = (egui_hal_touch_driver_t *)core->touch.hal_bridge_hal_driver;
    if (!hal_touch || !hal_touch->read)
    {
        return -1;
    }

    if (hal_touch->get_int)
    {
        if (!hal_touch->get_int() && !core->touch.hal_touch_last_pressed)
        {
            return 0;
        }
    }

    egui_hal_touch_data_t data;
    memset(&data, 0, sizeof(data));
    if (hal_touch->read(hal_touch, core, &data) != 0)
    {
        return -1;
    }

    const egui_hal_touch_config_t *cfg = &hal_touch->config;
    if (data.point_count > EGUI_TOUCH_DRIVER_MAX_POINTS)
    {
        data.point_count = EGUI_TOUCH_DRIVER_MAX_POINTS;
    }
    if (data.point_count > EGUI_HAL_TOUCH_MAX_POINTS)
    {
        data.point_count = EGUI_HAL_TOUCH_MAX_POINTS;
    }

    for (uint8_t i = 0; i < data.point_count; i++)
    {
        hal_touch_transform_point(cfg, &data.points[i]);
        out_data->points[i].x = data.points[i].x;
        out_data->points[i].y = data.points[i].y;
        out_data->points[i].id = data.points[i].id;
        out_data->points[i].pressure = data.points[i].pressure;
    }

    out_data->point_count = data.point_count;
    core->touch.hal_touch_last_pressed = out_data->point_count > 0;
    return 0;
}

void egui_hal_touch_register(egui_core_t *core, egui_hal_touch_driver_t *touch, const egui_hal_touch_config_t *config)
{
    if (core == NULL || touch == NULL || config == NULL)
    {
        return;
    }

    /* Reset and initialize the HAL touch driver */
    if (touch->reset)
    {
        if (touch->reset(touch) != 0)
        {
            return;
        }
    }
    if (touch->init)
    {
        if (touch->init(touch, config) != 0)
        {
            return;
        }
    }

    /* Register with Core */
    core->touch.hal_bridge_hal_driver = touch;
    core->touch.hal_bridge_ops.init = NULL;
    core->touch.hal_bridge_ops.read = hal_touch_read;
    core->touch.hal_bridge_driver.ops = &core->touch.hal_bridge_ops;
    core->touch.hal_touch_last_pressed = 0;

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
