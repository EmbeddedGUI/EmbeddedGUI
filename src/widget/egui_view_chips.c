#include "egui_view_chips.h"

/**
 * @file egui_view_chips.c
 * @brief Chip selector implemented as a button-matrix compatibility wrapper.
 *
 * The wrapper keeps the button-matrix rendering and hit-testing behavior, but
 * exposes chip-oriented naming and turns on persistent selection mode by
 * default.
 */

/**
 * @brief Normalize the requested column count for the current chip count.
 *
 * Passing `0` or an oversized value means "use one row", so the helper falls
 * back to the chip count itself.
 */
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

/**
 * @brief Replace chip labels and resolve how many columns the matrix should use.
 */
void egui_view_chips_set_chips(egui_view_t *self, const char **labels, uint8_t count, uint8_t cols)
{
    egui_view_button_matrix_set_labels(self, labels, count, egui_view_chips_resolve_cols(count, cols));
}

const char **egui_view_chips_get_chips(egui_view_t *self)
{
    return egui_view_button_matrix_get_labels(self);
}

uint8_t egui_view_chips_get_chip_count(egui_view_t *self)
{
    return egui_view_button_matrix_get_button_count(self);
}

uint8_t egui_view_chips_get_cols(egui_view_t *self)
{
    return egui_view_button_matrix_get_cols(self);
}

/**
 * @brief Forward the optional chip icon array to the button-matrix backend.
 */
void egui_view_chips_set_chip_icons(egui_view_t *self, const char **icons)
{
    egui_view_button_matrix_set_icons(self, icons);
}

const char **egui_view_chips_get_chip_icons(egui_view_t *self)
{
    return egui_view_button_matrix_get_icons(self);
}

/**
 * @brief Register the callback fired when a chip is chosen by the user.
 */
void egui_view_chips_set_on_selected_listener(egui_view_t *self, egui_view_on_chip_selected_listener_t listener)
{
    egui_view_button_matrix_set_on_click(self, listener);
}

egui_view_on_chip_selected_listener_t egui_view_chips_get_on_selected_listener(egui_view_t *self)
{
    return egui_view_button_matrix_get_on_click(self);
}

/**
 * @brief Change the gap between neighboring chip cells.
 */
void egui_view_chips_set_gap(egui_view_t *self, uint8_t gap)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->gap == gap)
    {
        return;
    }
    egui_view_button_matrix_set_gap(self, gap);
}

uint8_t egui_view_chips_get_gap(egui_view_t *self)
{
    return egui_view_button_matrix_get_gap(self);
}

/**
 * @brief Programmatically select one chip without invoking the click callback.
 */
void egui_view_chips_set_selected_index(egui_view_t *self, uint8_t index)
{
    egui_view_button_matrix_set_selected_index(self, index);
}

/**
 * @brief Clear any stored persistent chip selection.
 */
void egui_view_chips_clear_selection(egui_view_t *self)
{
    egui_view_button_matrix_set_selected_index(self, EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE);
}

/**
 * @brief Return the index of the selected chip, if any.
 */
uint8_t egui_view_chips_get_selected_index(egui_view_t *self)
{
    return egui_view_button_matrix_get_selected_index(self);
}

/**
 * @brief Override the rounded corner radius used by each chip cell.
 */
void egui_view_chips_set_corner_radius(egui_view_t *self, uint8_t radius)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->corner_radius == radius)
    {
        return;
    }
    egui_view_button_matrix_set_corner_radius(self, radius);
}

uint8_t egui_view_chips_get_corner_radius(egui_view_t *self)
{
    return egui_view_button_matrix_get_corner_radius(self);
}

/**
 * @brief Override the normal background color for unselected chips.
 */
void egui_view_chips_set_bg_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->btn_color.full == color.full)
    {
        return;
    }
    egui_view_button_matrix_set_btn_color(self, color);
}

egui_color_t egui_view_chips_get_bg_color(egui_view_t *self)
{
    return egui_view_button_matrix_get_btn_color(self);
}

/**
 * @brief Override the highlighted background color for the selected chip.
 */
void egui_view_chips_set_selected_bg_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->btn_pressed_color.full == color.full)
    {
        return;
    }
    egui_view_button_matrix_set_btn_pressed_color(self, color);
}

egui_color_t egui_view_chips_get_selected_bg_color(egui_view_t *self)
{
    return egui_view_button_matrix_get_btn_pressed_color(self);
}

/**
 * @brief Override the shared label color used by all chip cells.
 */
void egui_view_chips_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->text_color.full == color.full)
    {
        return;
    }
    egui_view_button_matrix_set_text_color(self, color);
}

egui_color_t egui_view_chips_get_text_color(egui_view_t *self)
{
    return egui_view_button_matrix_get_text_color(self);
}

/**
 * @brief Override the border color drawn around each chip cell.
 */
void egui_view_chips_set_border_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->border_color.full == color.full)
    {
        return;
    }
    egui_view_button_matrix_set_border_color(self, color);
}

egui_color_t egui_view_chips_get_border_color(egui_view_t *self)
{
    return egui_view_button_matrix_get_border_color(self);
}

/**
 * @brief Override the font used for chip labels.
 */
void egui_view_chips_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (font == NULL || local->font == font)
    {
        return;
    }
    egui_view_button_matrix_set_font(self, font);
}

const egui_font_t *egui_view_chips_get_font(egui_view_t *self)
{
    return egui_view_button_matrix_get_font(self);
}

/**
 * @brief Override the icon font used when chip icons are configured.
 */
void egui_view_chips_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->icon_font == font)
    {
        return;
    }
    egui_view_button_matrix_set_icon_font(self, font);
}

const egui_font_t *egui_view_chips_get_icon_font(egui_view_t *self)
{
    return egui_view_button_matrix_get_icon_font(self);
}

/**
 * @brief Override the spacing between one chip icon and its text.
 */
void egui_view_chips_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->icon_text_gap == gap)
    {
        return;
    }
    egui_view_button_matrix_set_icon_text_gap(self, gap);
}

egui_dim_t egui_view_chips_get_icon_text_gap(egui_view_t *self)
{
    return egui_view_button_matrix_get_icon_text_gap(self);
}

/**
 * @brief Initialize the chips wrapper and enable persistent single selection.
 */
void egui_view_chips_init(egui_view_t *self, egui_core_t *core)
{
    egui_view_button_matrix_init(self, core);
    egui_view_button_matrix_set_selection_enabled(self, 1);
    egui_view_set_view_name(self, "egui_view_chips");
}

/**
 * @brief Translate chips-style params into the button-matrix parameter block.
 */
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

/**
 * @brief Convenience initializer that chains chips init and params.
 */
void egui_view_chips_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_chips_params_t *params)
{
    egui_view_chips_init(self, core);
    egui_view_chips_apply_params(self, params);
}
