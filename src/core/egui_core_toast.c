#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "app/egui_toast.h"

egui_toast_t *egui_core_toast_get(egui_core_t *core)
{
    return core->scene.toast;
}

void egui_core_toast_set(egui_core_t *core, egui_toast_t *toast)
{
    if (toast != NULL && egui_toast_get_core(toast) != core)
    {
        return;
    }

    core->scene.toast = toast;
}
