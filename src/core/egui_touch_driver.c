#include "egui_touch_driver.h"
#include "egui_core.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

void egui_touch_driver_register(egui_core_t *core, egui_touch_driver_t *driver)
{
    core->touch.driver = driver;

    if (driver != NULL && driver->ops != NULL && driver->ops->init != NULL)
    {
        driver->ops->init(core);
    }
}

egui_touch_driver_t *egui_touch_driver_get(egui_core_t *core)
{
    return core->touch.driver;
}

#endif /* EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH */
