#include "egui.h"
#include <stdlib.h>
#include <math.h>

#include "egui_toast_std.h"
#include "core/egui_core_internal.h"

/**
 * @file egui_toast_std.c
 * @brief Default toast implementation that renders one rounded label overlay near the bottom of the screen.
 */

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_toast_param_normal, EGUI_COLOR_BLACK, EGUI_ALPHA_50, 30);
EGUI_BACKGROUND_PARAM_INIT(bg_toast_params, &bg_toast_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_toast, &bg_toast_params);

static void egui_toast_std_on_set_default(egui_toast_t *self);

/** Show the standard toast by sizing and placing its shared label overlay around the text. */
void egui_toast_std_on_show(egui_toast_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_toast_std_t);
    egui_core_t *core = egui_toast_get_core(self);
    egui_toast_on_show(self, text);

    if (core == NULL)
    {
        return;
    }

    egui_dim_t width = 0;
    egui_dim_t height = 0;
    egui_view_label_get_str_size_with_padding((egui_view_t *)&local->label, text, &width, &height);

    egui_view_label_set_text((egui_view_t *)&local->label, self->info);
    egui_view_set_position((egui_view_t *)&local->label, (core->screen_width - width) / 2, (core->screen_height - height - 20));
    egui_view_set_size((egui_view_t *)&local->label, width, height);
    egui_view_set_visible((egui_view_t *)&local->label, 1);
}

/** Hide the standard toast by hiding its shared label overlay. */
void egui_toast_std_on_hide(egui_toast_t *self)
{
    EGUI_LOCAL_INIT(egui_toast_std_t);
    egui_toast_on_hide(self);

    egui_view_set_visible((egui_view_t *)&local->label, 0);
}

/** Return the persistent text buffer stored inside the standard-toast instance. */
char *egui_toast_std_get_str_buf(egui_toast_t *self)
{
    EGUI_LOCAL_INIT(egui_toast_std_t);

    return local->toast_str;
}

static const egui_toast_api_t EGUI_TOAST_API_TABLE_NAME(egui_toast_std_t) = {
        .on_set_default = egui_toast_std_on_set_default,
        .on_show = egui_toast_std_on_show,
        .on_hide = egui_toast_std_on_hide,
        .get_str_buf = egui_toast_std_get_str_buf,
};

/** Ensure the shared label overlay is attached to the owning core root when this toast becomes the default toast. */
static void egui_toast_std_on_set_default(egui_toast_t *self)
{
    EGUI_LOCAL_INIT(egui_toast_std_t);
    egui_view_t *label = EGUI_VIEW_OF(&local->label);
    egui_core_t *core = egui_toast_get_core(self);
    egui_view_t *root_view;

    if (core == NULL)
    {
        return;
    }

    root_view = EGUI_VIEW_OF(egui_core_get_root_view(core));

    if (egui_view_get_core(label) == core && EGUI_VIEW_PARENT(label) == root_view)
    {
        return;
    }

    if (egui_view_get_core(label) != core)
    {
        return;
    }

    if (EGUI_VIEW_PARENT(label) != root_view)
    {
        egui_core_add_root_view(core, label);
    }
}

/** Initialize the stock toast implementation and create its hidden shared label overlay. */
void egui_toast_std_init(egui_toast_t *self, egui_core_t *core)
{
    EGUI_LOCAL_INIT(egui_toast_std_t);

    egui_toast_init(self, core);
    self->api = &EGUI_TOAST_API_TABLE_NAME(egui_toast_std_t);

    egui_toast_set_name(self, "egui_toast_std");

    // Shared label view that is positioned and shown on demand for each toast message.
    egui_view_label_init((egui_view_t *)&local->label, core);
    egui_view_label_set_align_type((egui_view_t *)&local->label, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->label, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->label, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_set_padding((egui_view_t *)&local->label, 10, 10, 5, 5);

    egui_view_set_visible((egui_view_t *)&local->label, 0);

    egui_view_set_background((egui_view_t *)&local->label, (egui_background_t *)&bg_toast);

    egui_core_add_root_view(core, EGUI_VIEW_OF(&local->label));
}
