#include <stdlib.h>
#include "egui_view_sample_input.h"

void egui_view_sample_input_init(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    EGUI_LOCAL_INIT(egui_view_sample_input_t);

    // Init base linearlayout
    egui_view_linearlayout_init(self);
    egui_view_set_position(self, x, y);
    egui_view_set_size(self, w, h);
    egui_view_linearlayout_set_orientation(self, 0); // vertical
    egui_view_linearlayout_set_align_type(self, EGUI_ALIGN_HCENTER);

    // Init label child
    egui_view_label_init(EGUI_VIEW_OF(&local->label));
    egui_view_set_size(EGUI_VIEW_OF(&local->label), w, 20);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&local->label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&local->label), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&local->label), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->label));

    // Init slider child
    egui_view_slider_init(EGUI_VIEW_OF(&local->slider));
    egui_view_set_size(EGUI_VIEW_OF(&local->slider), w - 20, 20);
    egui_view_slider_set_value(EGUI_VIEW_OF(&local->slider), 50);
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->slider));

    // Layout children
    egui_view_linearlayout_layout_childs(self);
}
