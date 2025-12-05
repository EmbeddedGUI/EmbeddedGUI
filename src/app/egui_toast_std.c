#include "egui.h"
#include <stdlib.h>
#include <math.h>

#include "egui_toast_std.h"

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_toast_param_normal, EGUI_COLOR_BLACK, EGUI_ALPHA_50, 30);
EGUI_BACKGROUND_PARAM_INIT(bg_toast_params, &bg_toast_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_toast, &bg_toast_params);

void egui_toast_std_on_show(egui_toast_t *self, const char *text)
{
    egui_toast_std_t *local = (egui_toast_std_t *)self;
    // Call super on_show
    egui_toast_on_show(self, text);

    egui_dim_t width = 0;
    egui_dim_t height = 0;
    egui_view_label_get_str_size_with_padding((egui_view_t *)&local->label, text, &width, &height);

    egui_view_label_set_text((egui_view_t *)&local->label, self->info);
    egui_view_set_position((egui_view_t *)&local->label, (EGUI_CONFIG_SCEEN_WIDTH - width) / 2, (EGUI_CONFIG_SCEEN_HEIGHT - height - 20));
    egui_view_set_size((egui_view_t *)&local->label, width, height);
    egui_view_set_visible((egui_view_t *)&local->label, 1);
}

void egui_toast_std_on_hide(egui_toast_t *self)
{
    egui_toast_std_t *local = (egui_toast_std_t *)self;
    // Call super on_hide
    egui_toast_on_hide(self);

    // Hide all views
    egui_view_set_visible((egui_view_t *)&local->label, 0);
}

char *egui_toast_std_get_str_buf(egui_toast_t *self)
{
    egui_toast_std_t *local = (egui_toast_std_t *)self;

    return local->toast_str;
}

static const egui_toast_api_t EGUI_TOAST_API_TABLE_NAME(egui_toast_std_t) = {
        .on_show = egui_toast_std_on_show, // changed
        .on_hide = egui_toast_std_on_hide, // changed
        .get_str_buf = egui_toast_std_get_str_buf, // changed
};

void egui_toast_std_init(egui_toast_t *self)
{
    egui_toast_std_t *local = (egui_toast_std_t *)self;
    // call super init.
    egui_toast_init(self);
    // update api.
    self->api = &EGUI_DIALOG_API_TABLE_NAME(egui_toast_std_t);

    // init local data.
    egui_toast_set_name(self, "egui_toast_std");

    // Init all views
    // label
    egui_view_label_init((egui_view_t *)&local->label);
    egui_view_label_set_align_type((egui_view_t *)&local->label, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->label, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->label, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_set_padding((egui_view_t *)&local->label, 10, 10, 5, 5);

    egui_view_set_visible((egui_view_t *)&local->label, 0);

    // Add To Root
    egui_core_add_root_view((egui_view_t *)&local->label);

    // Set Background
    egui_view_set_background((egui_view_t *)&local->label, (egui_background_t *)&bg_toast);
}
