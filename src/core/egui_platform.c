#include "egui_platform.h"
#include "egui_api.h"
#include "egui_core.h"

/**
 * @file egui_platform.c
 * @brief Registration helpers for the per-core platform service table.
 */

static egui_platform_t *s_default_platform = NULL;

/**
 * Attach the single platform service table used by the current process.
 * Re-registering the same instance is accepted so repeated init paths remain
 * idempotent.
 */
void egui_platform_register(egui_platform_t *platform)
{
    EGUI_ASSERT(platform != NULL);

    if (s_default_platform != NULL)
    {
        EGUI_ASSERT(s_default_platform == platform);
    }
    else
    {
        s_default_platform = platform;
    }
}

/** Return the process-global platform service table. */
egui_platform_t *egui_platform_get(void)
{
    return s_default_platform;
}
