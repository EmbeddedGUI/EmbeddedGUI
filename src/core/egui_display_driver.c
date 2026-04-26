#include "egui_display_driver.h"
#include "egui_core.h"

/**
 * @file egui_display_driver.c
 * @brief Registration and state helpers for the per-core display driver instance.
 */

/**
 * Bind one display driver to the core and immediately apply the defaults already stored in the driver instance, such as rotation and brightness.
 */
void egui_display_driver_register(egui_core_t *core, egui_display_driver_t *driver)
{
    core->render.driver = driver;

    if (driver == NULL)
    {
        return;
    }

    // Initialize the port-side panel driver before any runtime state is applied.
    if (driver->ops->init != NULL)
    {
        driver->ops->init(core);
    }

    // Replay the driver's cached defaults so the core and the panel start in sync.
    if (driver->ops->set_rotation != NULL)
    {
        driver->ops->set_rotation(core, driver->rotation);
    }
    if (driver->ops->set_brightness != NULL)
    {
        driver->ops->set_brightness(core, driver->brightness);
    }
}

/** Return the display driver currently registered on this core, if any. */
egui_display_driver_t *egui_display_driver_get(egui_core_t *core)
{
    return core->render.driver;
}

/** Update the cached brightness value and forward it to the hardware when supported. */
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

/** Update the cached power state and forward it to the hardware when supported. */
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

/**
 * Update the runtime display rotation.
 * The driver is notified first, then the core's logical screen size is refreshed when the rotation changes the effective orientation.
 */
void egui_display_set_rotation(egui_core_t *core, egui_display_rotation_t rotation)
{
    if (core->render.driver == NULL)
    {
        return;
    }

    egui_display_rotation_t old_rotation = core->render.driver->rotation;
    core->render.driver->rotation = rotation;

    // Let the hardware rotate first when the controller supports it.
    if (core->render.driver->ops->set_rotation != NULL)
    {
        core->render.driver->ops->set_rotation(core, rotation);
    }

    // Refresh the logical screen size so layout/input code sees the new orientation.
    if (old_rotation != rotation)
    {
        int16_t new_w = egui_display_get_width(core);
        int16_t new_h = egui_display_get_height(core);

        egui_core_set_screen_size(core, new_w, new_h);
    }
}

/** Return the current runtime display rotation, or the default orientation when no driver is present. */
egui_display_rotation_t egui_display_get_rotation(egui_core_t *core)
{
    if (core->render.driver == NULL)
    {
        return EGUI_DISPLAY_ROTATION_0;
    }
    return core->render.driver->rotation;
}

/** Return the logical display width currently exposed to the rest of the core. */
int16_t egui_display_get_width(egui_core_t *core)
{
    if (core->render.driver == NULL)
    {
        if (core != NULL && core->screen_width > 0)
        {
            return (int16_t)core->screen_width;
        }
        return EGUI_CONFIG_SCREEN_WIDTH;
    }
#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
    // Software rotation swaps the logical width/height for quarter turns.
    if (core->render.software_rotation)
    {
        if (core->render.driver->rotation == EGUI_DISPLAY_ROTATION_90 || core->render.driver->rotation == EGUI_DISPLAY_ROTATION_270)
        {
            return core->render.driver->physical_height;
        }
    }
#endif
    return core->render.driver->physical_width;
}

/** Return the logical display height currently exposed to the rest of the core. */
int16_t egui_display_get_height(egui_core_t *core)
{
    if (core->render.driver == NULL)
    {
        if (core != NULL && core->screen_height > 0)
        {
            return (int16_t)core->screen_height;
        }
        return EGUI_CONFIG_SCREEN_HEIGHT;
    }
#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
    // Software rotation swaps the logical width/height for quarter turns.
    if (core->render.software_rotation)
    {
        if (core->render.driver->rotation == EGUI_DISPLAY_ROTATION_90 || core->render.driver->rotation == EGUI_DISPLAY_ROTATION_270)
        {
            return core->render.driver->physical_width;
        }
    }
#endif
    return core->render.driver->physical_height;
}

/** Mark one non-blocking frame-sync slot as ready after a VSync/TE edge. */
void egui_display_notify_vsync(egui_core_t *core)
{
    if (core->render.driver != NULL)
    {
        core->render.driver->frame_sync_ready = 1;
    }
}
