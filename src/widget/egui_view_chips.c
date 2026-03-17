#include "egui_view_chips.h"

static uint8_t egui_view_chips_resolve_cols(uint8_t count, uint8_t cols)
{
    if (count == 0)
    {
        return 1;
    }
    if (cols == 0 || cols > count)
    {
        return count;
    }
    return cols;
}

void egui_view_chips_set_chips(egui_view_t *self, const char **labels, uint8_t count, uint8_t cols)
{
    egui_view_button_matrix_set_labels(self, labels, count, egui_view_chips_resolve_cols(count, cols));
}

void egui_view_chips_set_chip_icons(egui_view_t *self, const char **icons)
{
    egui_view_button_matrix_set_icons(self, icons);
}

void egui_view_chips_set_on_selected_listener(egui_view_t *self, egui_view_on_chip_selected_listener_t listener)
{
    egui_view_button_matrix_set_on_click(self, listener);
}

void egui_view_chips_set_gap(egui_view_t *self, uint8_t gap)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->gap == gap)
    {
        return;
    }
    egui_view_button_matrix_set_gap(self, gap);
}

void egui_view_chips_set_selected_index(egui_view_t *self, uint8_t index)
{
    egui_view_button_matrix_set_selected_index(self, index);
}

void egui_view_chips_clear_selection(egui_view_t *self)
{
    egui_view_button_matrix_set_selected_index(self, EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE);
}

uint8_t egui_view_chips_get_selected_index(egui_view_t *self)
{
    return egui_view_button_matrix_get_selected_index(self);
}

void egui_view_chips_set_corner_radius(egui_view_t *self, uint8_t radius)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->corner_radius == radius)
    {
        return;
    }
    egui_view_button_matrix_set_corner_radius(self, radius);
}

void egui_view_chips_set_bg_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->btn_color.full == color.full)
    {
        return;
    }
    egui_view_button_matrix_set_btn_color(self, color);
}

void egui_view_chips_set_selected_bg_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->btn_pressed_color.full == color.full)
    {
        return;
    }
    egui_view_button_matrix_set_btn_pressed_color(self, color);
}

void egui_view_chips_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->text_color.full == color.full)
    {
        return;
    }
    egui_view_button_matrix_set_text_color(self, color);
}

void egui_view_chips_set_border_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->border_color.full == color.full)
    {
        return;
    }
    egui_view_button_matrix_set_border_color(self, color);
}

void egui_view_chips_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (font == NULL || local->font == font)
    {
        return;
    }
    egui_view_button_matrix_set_font(self, font);
}

void egui_view_chips_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->icon_font == font)
    {
        return;
    }
    egui_view_button_matrix_set_icon_font(self, font);
}

void egui_view_chips_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->icon_text_gap == gap)
    {
        return;
    }
    egui_view_button_matrix_set_icon_text_gap(self, gap);
}

void egui_view_chips_init(egui_view_t *self)
{
    egui_view_button_matrix_init(self);
    egui_view_button_matrix_set_selection_enabled(self, 1);
    egui_view_set_view_name(self, "egui_view_chips");
}

void egui_view_chips_apply_params(egui_view_t *self, const egui_view_chips_params_t *params)
{
    egui_view_button_matrix_params_t matrix_params = {
            .region = params->region,
            .cols = egui_view_chips_resolve_cols(params->chip_count, params->cols),
            .gap = params->gap,
    };
    egui_view_button_matrix_apply_params(self, &matrix_params);
    egui_view_chips_set_chips(self, params->labels, params->chip_count, matrix_params.cols);
    egui_view_chips_set_chip_icons(self, params->icons);
}

void egui_view_chips_init_with_params(egui_view_t *self, const egui_view_chips_params_t *params)
{
    egui_view_chips_init(self);
    egui_view_chips_apply_params(self, params);
}
