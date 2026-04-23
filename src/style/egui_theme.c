#include "egui_theme.h"
#include "core/egui_core.h"

/**
 * @file egui_theme.c
 * @brief Theme selection helpers stored on the core object.
 */

/**
 * @brief Install a new theme on the target core and trigger a full refresh.
 *
 * Passing `NULL` falls back to the configured default theme.
 */
void egui_theme_set(egui_core_t *core, const egui_theme_t *theme)
{
    if (core == NULL)
    {
        return;
    }

    if (theme == NULL)
    {
        theme = EGUI_CONFIG_THEME_DEFAULT;
    }
    if (theme == core->asset.theme_current)
    {
        return;
    }

    core->asset.theme_current = theme;
    egui_core_force_refresh(core);
}

/**
 * @brief Return the active theme, or the configured default when unset.
 */
const egui_theme_t *egui_theme_get(egui_core_t *core)
{
    if (core == NULL || core->asset.theme_current == NULL)
    {
        return EGUI_CONFIG_THEME_DEFAULT;
    }

    return core->asset.theme_current;
}
