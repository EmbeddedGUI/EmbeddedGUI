#include "egui_platform.h"
#include "egui_api.h"
#include "egui_core.h"

/**
 * @file egui_platform.c
 * @brief Registration helpers for the per-core platform service table.
 */

/**
 * Attach one platform service table to the core.
 * The binding is write-once during normal startup, but re-registering the
 * same instance is accepted so repeated init paths remain idempotent.
 */
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

/** Return the platform services currently attached to the given core. */
egui_platform_t *egui_platform_get(egui_core_t *core)
{
    EGUI_ASSERT(core != NULL);
    return core->render.platform;
}
