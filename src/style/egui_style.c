#include "egui_style.h"
#include "widget/egui_view.h"

egui_state_t egui_style_get_view_state(const void *view)
{
    const egui_view_t *v = (const egui_view_t *)view;

    if (!egui_view_get_enable((egui_view_t *)v))
    {
        return EGUI_STATE_DISABLED;
    }
    if (v->is_pressed)
    {
        return EGUI_STATE_PRESSED;
    }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (v->is_focused)
    {
        return EGUI_STATE_FOCUSED;
    }
#endif
    return EGUI_STATE_NORMAL;
}
