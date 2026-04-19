#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_display_driver.h"
#include "egui_platform.h"

void egui_core_setup_display_start(egui_core_t *core, const egui_display_setup_t *setup)
{
    egui_platform_register(core, setup->platform);
    egui_display_driver_register(core, setup->display_driver);

    if (setup->touch_register != NULL)
    {
        setup->touch_register(core);
    }

    if (setup->uicode_init != NULL)
    {
        setup->uicode_init(core);
    }

    egui_screen_on(core);
}
