#include "egui_touch_driver.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

static egui_touch_driver_t *s_touch_driver = NULL;

void egui_touch_driver_register(egui_touch_driver_t *driver)
{
    s_touch_driver = driver;
}

egui_touch_driver_t *egui_touch_driver_get(void)
{
    return s_touch_driver;
}

#endif /* EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH */
