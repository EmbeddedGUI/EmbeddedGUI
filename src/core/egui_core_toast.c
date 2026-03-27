#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "app/egui_toast.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOAST

egui_toast_t *egui_core_toast_get(void)
{
    return egui_core.toast;
}

void egui_core_toast_set(egui_toast_t *toast)
{
    egui_core.toast = toast;
}

void egui_core_toast_show_info_with_duration(const char *text, uint16_t duration)
{
    if (!egui_core.toast)
    {
        return;
    }
    egui_toast_set_duration(egui_core.toast, duration);
    egui_toast_show(egui_core.toast, text);
}

void egui_core_toast_show_info(const char *text)
{
    egui_core_toast_show_info_with_duration(text, EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME);
}

#endif
