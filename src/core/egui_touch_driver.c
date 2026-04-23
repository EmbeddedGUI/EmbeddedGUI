#include "egui_touch_driver.h"
#include "egui_core.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

/**
 * @file egui_touch_driver.c
 * @brief Touch-driver registration helpers for the per-core input pipeline.
 */

/** Bind one touch driver to the core and eagerly initialize the port when possible. */
void egui_touch_driver_register(egui_core_t *core, egui_touch_driver_t *driver)
{
    core->touch.driver = driver;

    if (driver != NULL && driver->ops != NULL && driver->ops->init != NULL)
    {
        driver->ops->init(core);
    }
}

/** Return the touch driver currently used by this core's polling input path. */
egui_touch_driver_t *egui_touch_driver_get(egui_core_t *core)
{
    return core->touch.driver;
}

#endif /* EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH */
