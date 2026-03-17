#include "egui_view_auto_suggest_box.h"

static void hcw_auto_suggest_box_apply_palette(egui_view_t *self, egui_dim_t collapsed_height, egui_dim_t item_height, uint8_t max_visible_items,
                                               egui_color_t bg_color, egui_color_t border_color, egui_color_t text_color, egui_color_t arrow_color,
                                               egui_color_t highlight_color)
{
    egui_view_combobox_t *local = (egui_view_combobox_t *)self;

    local->collapsed_height = collapsed_height;
    local->item_height = item_height;
    local->bg_color = bg_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->arrow_color = arrow_color;
    local->highlight_color = highlight_color;
    egui_view_autocomplete_set_max_visible_items(self, max_visible_items);
    egui_view_invalidate(self);
}

void hcw_auto_suggest_box_apply_standard_style(egui_view_t *self)
{
    hcw_auto_suggest_box_apply_palette(self, 34, 24, 4, EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD7DEE7), EGUI_COLOR_HEX(0x1E2933), EGUI_COLOR_HEX(0x617384),
                                       EGUI_COLOR_HEX(0xDBEAFE));
}

void hcw_auto_suggest_box_apply_compact_style(egui_view_t *self)
{
    hcw_auto_suggest_box_apply_palette(self, 28, 21, 3, EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD3DEDA), EGUI_COLOR_HEX(0x203039), EGUI_COLOR_HEX(0x4E6B67),
                                       EGUI_COLOR_HEX(0xCCFBF1));
}

void hcw_auto_suggest_box_apply_read_only_style(egui_view_t *self)
{
    hcw_auto_suggest_box_apply_palette(self, 28, 21, 3, EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xDBE2E8), EGUI_COLOR_HEX(0x5B6976), EGUI_COLOR_HEX(0x8996A4),
                                       EGUI_COLOR_HEX(0xE5EAF0));
}
