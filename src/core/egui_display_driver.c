#include "egui_display_driver.h"
#include "egui_core.h"

static egui_display_driver_t *g_display_driver = NULL;

void egui_display_driver_register(egui_display_driver_t *driver)
{
    g_display_driver = driver;
}

egui_display_driver_t *egui_display_driver_get(void)
{
    return g_display_driver;
}

void egui_display_set_brightness(uint8_t level)
{
    if (g_display_driver == NULL)
    {
        return;
    }
    g_display_driver->brightness = level;
    if (g_display_driver->ops->set_brightness != NULL)
    {
        g_display_driver->ops->set_brightness(level);
    }
}

void egui_display_set_power(uint8_t on)
{
    if (g_display_driver == NULL)
    {
        return;
    }
    g_display_driver->power_on = on;
    if (g_display_driver->ops->set_power != NULL)
    {
        g_display_driver->ops->set_power(on);
    }
}

void egui_display_set_rotation(egui_display_rotation_t rotation)
{
    if (g_display_driver == NULL)
    {
        return;
    }

    egui_display_rotation_t old_rotation = g_display_driver->rotation;
    g_display_driver->rotation = rotation;

    // If hardware supports rotation, delegate to it
    if (g_display_driver->ops->set_rotation != NULL)
    {
        g_display_driver->ops->set_rotation(rotation);
    }

    // Update core screen dimensions if rotation changed between portrait/landscape
    if (old_rotation != rotation)
    {
        int16_t new_w = egui_display_get_width();
        int16_t new_h = egui_display_get_height();

        egui_core_set_screen_size(new_w, new_h);
    }
}

egui_display_rotation_t egui_display_get_rotation(void)
{
    if (g_display_driver == NULL)
    {
        return EGUI_DISPLAY_ROTATION_0;
    }
    return g_display_driver->rotation;
}

int16_t egui_display_get_width(void)
{
    if (g_display_driver == NULL)
    {
        return EGUI_CONFIG_SCEEN_WIDTH;
    }
#if EGUI_CONFIG_SOFTWARE_ROTATION
    if (g_display_driver->rotation == EGUI_DISPLAY_ROTATION_90 || g_display_driver->rotation == EGUI_DISPLAY_ROTATION_270)
    {
        return g_display_driver->physical_height;
    }
#endif
    return g_display_driver->physical_width;
}

int16_t egui_display_get_height(void)
{
    if (g_display_driver == NULL)
    {
        return EGUI_CONFIG_SCEEN_HEIGHT;
    }
#if EGUI_CONFIG_SOFTWARE_ROTATION
    if (g_display_driver->rotation == EGUI_DISPLAY_ROTATION_90 || g_display_driver->rotation == EGUI_DISPLAY_ROTATION_270)
    {
        return g_display_driver->physical_width;
    }
#endif
    return g_display_driver->physical_height;
}

void egui_display_notify_vsync(void)
{
    if (g_display_driver != NULL)
    {
        g_display_driver->frame_sync_ready = 1;
    }
}
