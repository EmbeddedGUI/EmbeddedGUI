#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_toast.h"
#include "egui_core_internal.h"
#include "app/egui_toast.h"

/**
 * @file egui_core_toast.c
 * @brief Tiny core-side bridge that stores the toast currently owned by one GUI core.
 */

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOAST

/** Return the toast currently registered on this core, or NULL when no toast is active. */
egui_toast_t *egui_core_toast_get(egui_core_t *core)
{
    return core->scene.toast;
}

/** Replace the current core-owned toast, rejecting toast objects that belong to another core. */
void egui_core_toast_set(egui_core_t *core, egui_toast_t *toast)
{
    if (toast != NULL && egui_toast_get_core(toast) != core)
    {
        return;
    }

    core->scene.toast = toast;
}

#else

egui_toast_t *egui_core_toast_get(egui_core_t *core)
{
    EGUI_UNUSED(core);
    return NULL;
}

void egui_core_toast_set(egui_core_t *core, egui_toast_t *toast)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(toast);
}

#endif
