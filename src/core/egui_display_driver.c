#include "egui_display_driver.h"
#include "egui_core.h"

void egui_display_driver_register(egui_core_t *core, egui_display_driver_t *driver)
{
    core->render.driver = driver;

    if (driver == NULL)
    {
        return;
    }

    // Initialize display hardware
    if (driver->ops->init != NULL)
    {
        driver->ops->init(core);
    }

    // Apply initial display configuration from driver struct
    if (driver->ops->set_rotation != NULL)
    {
        driver->ops->set_rotation(core, driver->rotation);
    }
    if (driver->ops->set_brightness != NULL)
    {
        driver->ops->set_brightness(core, driver->brightness);
    }
}

egui_display_driver_t *egui_display_driver_get(egui_core_t *core)
{
    return core->render.driver;
}

void egui_display_set_brightness(egui_core_t *core, uint8_t level)
{
    if (core->render.driver == NULL)
    {
        return;
    }
    core->render.driver->brightness = level;
    if (core->render.driver->ops->set_brightness != NULL)
    {
        core->render.driver->ops->set_brightness(core, level);
    }
}

void egui_display_set_power(egui_core_t *core, uint8_t on)
{
    if (core->render.driver == NULL)
    {
        return;
    }
    core->render.driver->power_on = on;
    if (core->render.driver->ops->set_power != NULL)
    {
        core->render.driver->ops->set_power(core, on);
    }
}

void egui_display_set_rotation(egui_core_t *core, egui_display_rotation_t rotation)
{
    if (core->render.driver == NULL)
    {
        return;
    }

    egui_display_rotation_t old_rotation = core->render.driver->rotation;
    core->render.driver->rotation = rotation;

    // If hardware supports rotation, delegate to it
    if (core->render.driver->ops->set_rotation != NULL)
    {
        core->render.driver->ops->set_rotation(core, rotation);
    }

    // Update core screen dimensions if rotation changed between portrait/landscape
    if (old_rotation != rotation)
    {
        int16_t new_w = egui_display_get_width(core);
        int16_t new_h = egui_display_get_height(core);

        egui_core_set_screen_size(core, new_w, new_h);
    }
}

egui_display_rotation_t egui_display_get_rotation(egui_core_t *core)
{
    if (core->render.driver == NULL)
    {
        return EGUI_DISPLAY_ROTATION_0;
    }
    return core->render.driver->rotation;
}

int16_t egui_display_get_width(egui_core_t *core)
{
    if (core->render.driver == NULL)
    {
        return EGUI_CONFIG_SCEEN_WIDTH;
    }
    // Runtime software rotation check (replaces compile-time EGUI_CONFIG_SOFTWARE_ROTATION)
    if (core->render.software_rotation)
    {
        if (core->render.driver->rotation == EGUI_DISPLAY_ROTATION_90 || core->render.driver->rotation == EGUI_DISPLAY_ROTATION_270)
        {
            return core->render.driver->physical_height;
        }
    }
    return core->render.driver->physical_width;
}

int16_t egui_display_get_height(egui_core_t *core)
{
    if (core->render.driver == NULL)
    {
        return EGUI_CONFIG_SCEEN_HEIGHT;
    }
    // Runtime software rotation check (replaces compile-time EGUI_CONFIG_SOFTWARE_ROTATION)
    if (core->render.software_rotation)
    {
        if (core->render.driver->rotation == EGUI_DISPLAY_ROTATION_90 || core->render.driver->rotation == EGUI_DISPLAY_ROTATION_270)
        {
            return core->render.driver->physical_width;
        }
    }
    return core->render.driver->physical_height;
}

void egui_display_notify_vsync(egui_core_t *core)
{
    if (core->render.driver != NULL)
    {
        core->render.driver->frame_sync_ready = 1;
    }
}
