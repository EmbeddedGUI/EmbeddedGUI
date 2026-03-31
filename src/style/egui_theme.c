#include "egui_theme.h"
#include "core/egui_core.h"

const egui_theme_t *egui_current_theme = EGUI_CONFIG_THEME_DEFAULT;

void egui_theme_set(const egui_theme_t *theme)
{
    if (theme == egui_current_theme)
    {
        return;
    }
    egui_current_theme = theme;
    egui_core_force_refresh();
}

const egui_theme_t *egui_theme_get(void)
{
    return egui_current_theme;
}
