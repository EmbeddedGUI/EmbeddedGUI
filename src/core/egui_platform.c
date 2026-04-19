#include "egui_platform.h"
#include "egui_api.h"
#include "egui_core.h"

void egui_platform_register(egui_core_t *core, egui_platform_t *platform)
{
    EGUI_ASSERT(core != NULL);
    EGUI_ASSERT(platform != NULL);

    if (core->render.platform != NULL)
    {
        EGUI_ASSERT(core->render.platform == platform);
    }
    else
    {
        core->render.platform = platform;
    }
}

egui_platform_t *egui_platform_get(egui_core_t *core)
{
    EGUI_ASSERT(core != NULL);
    return core->render.platform;
}
