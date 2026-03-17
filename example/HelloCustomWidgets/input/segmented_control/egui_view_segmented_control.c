#include "egui_view_segmented_control.h"

void hcw_segmented_control_apply_standard_style(egui_view_t *self)
{
    egui_view_segmented_control_set_corner_radius(self, 10);
    egui_view_segmented_control_set_segment_gap(self, 2);
    egui_view_segmented_control_set_horizontal_padding(self, 2);
    egui_view_segmented_control_set_bg_color(self, EGUI_COLOR_HEX(0xFFFFFF));
    egui_view_segmented_control_set_selected_bg_color(self, EGUI_COLOR_HEX(0x2563EB));
    egui_view_segmented_control_set_text_color(self, EGUI_COLOR_HEX(0x1E2933));
    egui_view_segmented_control_set_selected_text_color(self, EGUI_COLOR_HEX(0xFFFFFF));
    egui_view_segmented_control_set_border_color(self, EGUI_COLOR_HEX(0xD7DEE7));
}

void hcw_segmented_control_apply_compact_style(egui_view_t *self)
{
    egui_view_segmented_control_set_corner_radius(self, 8);
    egui_view_segmented_control_set_segment_gap(self, 1);
    egui_view_segmented_control_set_horizontal_padding(self, 1);
    egui_view_segmented_control_set_bg_color(self, EGUI_COLOR_HEX(0xFFFFFF));
    egui_view_segmented_control_set_selected_bg_color(self, EGUI_COLOR_HEX(0x0C7C73));
    egui_view_segmented_control_set_text_color(self, EGUI_COLOR_HEX(0x203039));
    egui_view_segmented_control_set_selected_text_color(self, EGUI_COLOR_HEX(0xFFFFFF));
    egui_view_segmented_control_set_border_color(self, EGUI_COLOR_HEX(0xD3DEDA));
}

void hcw_segmented_control_apply_read_only_style(egui_view_t *self)
{
    egui_view_segmented_control_set_corner_radius(self, 8);
    egui_view_segmented_control_set_segment_gap(self, 1);
    egui_view_segmented_control_set_horizontal_padding(self, 1);
    egui_view_segmented_control_set_bg_color(self, EGUI_COLOR_HEX(0xFBFCFD));
    egui_view_segmented_control_set_selected_bg_color(self, EGUI_COLOR_HEX(0x97A4B4));
    egui_view_segmented_control_set_text_color(self, EGUI_COLOR_HEX(0x5B6976));
    egui_view_segmented_control_set_selected_text_color(self, EGUI_COLOR_HEX(0xFFFFFF));
    egui_view_segmented_control_set_border_color(self, EGUI_COLOR_HEX(0xDBE2E8));
}
