#ifndef _EGUI_VIEW_BUTTON_MATRIX_H_
#define _EGUI_VIEW_BUTTON_MATRIX_H_

#include "egui_view.h"
#include "font/egui_font.h"

/**
 * @brief Grid of equal-sized buttons that can show text, icons, or both.
 *
 * The widget borrows label and icon arrays from the caller and splits its work
 * region into a fixed-capacity matrix. It can behave like a plain click pad or
 * keep one cell highlighted persistently for chip-like single selection.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Fixed storage capacity for borrowed button labels and icons. */
#define EGUI_VIEW_BUTTON_MATRIX_MAX_BUTTONS   16
/* Sentinel stored in `pressed_index` when no pointer currently owns a cell. */
#define EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE  0xFF
/* Sentinel stored in `selected_index` when persistent selection is cleared. */
#define EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE 0xFF

/** Listener fired when one button cell is clicked. */
typedef void (*egui_view_button_matrix_click_cb_t)(egui_view_t *self, uint8_t btn_index);

typedef struct egui_view_button_matrix egui_view_button_matrix_t;
struct egui_view_button_matrix
{
    egui_view_t base;

    /* Borrowed label array for each visible cell. */
    const char **labels;
    /* Optional borrowed icon array parallel to `labels`. */
    const char **icons;
    /* Number of active cells currently borrowed from the arrays. */
    uint8_t btn_count;
    /* Number of columns used to split the matrix region. */
    uint8_t cols;
    /* Pixel gap inserted between neighboring cells. */
    uint8_t gap;
    /* Cell index captured by the current pointer gesture. */
    uint8_t pressed_index;
    /* Persistently highlighted cell index, or `SELECTED_NONE`. */
    uint8_t selected_index;
    /* Whether `selected_index` affects drawing and touch-up behavior. */
    uint8_t selection_enabled;
    /* Normal background color for idle cells. */
    egui_color_t btn_color;
    /* Background color for pressed cells and selected cells. */
    egui_color_t btn_pressed_color;
    /* Shared tint color used for labels and icons. */
    egui_color_t text_color;
    /* Outline color drawn around each cell. */
    egui_color_t border_color;
    /* Rounded corner radius shared by all cells. */
    uint8_t corner_radius;
    /* Font used for label text. */
    const egui_font_t *font;
    /* Optional icon font override; auto-selected when NULL. */
    const egui_font_t *icon_font;
    /* Vertical spacing between a cell icon and its label. */
    egui_dim_t icon_text_gap;
    /* Callback fired after a successful touch-up inside one cell. */
    egui_view_button_matrix_click_cb_t on_click;
};

// ============== ButtonMatrix Params ==============
typedef struct egui_view_button_matrix_params egui_view_button_matrix_params_t;
/**
 * @brief Construction-time geometry block for one button matrix.
 */
struct egui_view_button_matrix_params
{
    /* Outer region occupied by the matrix widget. */
    egui_region_t region;
    /* Number of columns used to derive equal-width cells. */
    uint8_t cols;
    /* Pixel gap inserted between neighboring cells. */
    uint8_t gap;
};

/** Build a button-matrix parameter block with region, columns, and gap. */
#define EGUI_VIEW_BUTTON_MATRIX_PARAMS_INIT(_name, _x, _y, _w, _h, _cols, _gap)                                                                                \
    static const egui_view_button_matrix_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .cols = (_cols), .gap = (_gap)}

/** Apply a button-matrix parameter block after initialization. */
void egui_view_button_matrix_apply_params(egui_view_t *self, const egui_view_button_matrix_params_t *params);
/** Initialize a button matrix and immediately apply its parameter block. */
void egui_view_button_matrix_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_button_matrix_params_t *params);

/** Set the label array, button count, and column count. The count is clamped to the fixed maximum. */
void egui_view_button_matrix_set_labels(egui_view_t *self, const char **labels, uint8_t count, uint8_t cols);
/** Return the borrowed label array currently attached to the matrix. */
const char **egui_view_button_matrix_get_labels(egui_view_t *self);
/** Return the number of active button cells. */
uint8_t egui_view_button_matrix_get_button_count(egui_view_t *self);
/** Return the number of configured columns. */
uint8_t egui_view_button_matrix_get_cols(egui_view_t *self);
/** Register the callback fired when a cell is clicked. */
void egui_view_button_matrix_set_on_click(egui_view_t *self, egui_view_button_matrix_click_cb_t callback);
/** Return the callback fired when a cell is clicked. */
egui_view_button_matrix_click_cb_t egui_view_button_matrix_get_on_click(egui_view_t *self);
/** Enable or disable persistent single-selection highlighting. */
void egui_view_button_matrix_set_selection_enabled(egui_view_t *self, uint8_t enabled);
/** Return whether persistent single-selection highlighting is enabled. */
uint8_t egui_view_button_matrix_get_selection_enabled(egui_view_t *self);
/** Set the selected cell index. This only changes visible highlight when selection mode is enabled. */
void egui_view_button_matrix_set_selected_index(egui_view_t *self, uint8_t index);
/** Return the stored selected cell index, or `SELECTED_NONE`. */
uint8_t egui_view_button_matrix_get_selected_index(egui_view_t *self);
/** Set the normal background color used by each button cell. */
void egui_view_button_matrix_set_btn_color(egui_view_t *self, egui_color_t color);
/** Return the normal background color used by each button cell. */
egui_color_t egui_view_button_matrix_get_btn_color(egui_view_t *self);
/** Set the pressed or selected background color used by active cells. */
void egui_view_button_matrix_set_btn_pressed_color(egui_view_t *self, egui_color_t color);
/** Return the pressed or selected background color used by active cells. */
egui_color_t egui_view_button_matrix_get_btn_pressed_color(egui_view_t *self);
/** Set the text and icon tint color used inside each button cell. */
void egui_view_button_matrix_set_text_color(egui_view_t *self, egui_color_t color);
/** Return the text and icon tint color used inside each button cell. */
egui_color_t egui_view_button_matrix_get_text_color(egui_view_t *self);
/** Set the gap between button cells. */
void egui_view_button_matrix_set_gap(egui_view_t *self, uint8_t gap);
/** Return the gap between button cells. */
uint8_t egui_view_button_matrix_get_gap(egui_view_t *self);
/** Set the corner radius used for each button cell. */
void egui_view_button_matrix_set_corner_radius(egui_view_t *self, uint8_t radius);
/** Return the corner radius used for each button cell. */
uint8_t egui_view_button_matrix_get_corner_radius(egui_view_t *self);
/** Set the border color drawn around each button cell. */
void egui_view_button_matrix_set_border_color(egui_view_t *self, egui_color_t color);
/** Return the border color drawn around each button cell. */
egui_color_t egui_view_button_matrix_get_border_color(egui_view_t *self);
/** Override the font used for button labels. */
void egui_view_button_matrix_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the font used for button labels. */
const egui_font_t *egui_view_button_matrix_get_font(egui_view_t *self);
/** Set the optional icon array that parallels the label array. */
void egui_view_button_matrix_set_icons(egui_view_t *self, const char **icons);
/** Return the optional icon array that parallels the label array. */
const char **egui_view_button_matrix_get_icons(egui_view_t *self);
/** Override the icon font used for per-cell icons. */
void egui_view_button_matrix_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the icon font override, or NULL when automatic icon-font resolution is used. */
const egui_font_t *egui_view_button_matrix_get_icon_font(egui_view_t *self);
/** Set the vertical gap between icon and text when one cell shows both. */
void egui_view_button_matrix_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
/** Return the vertical gap between icon and text when one cell shows both. */
egui_dim_t egui_view_button_matrix_get_icon_text_gap(egui_view_t *self);
/** Default draw hook used by the button-matrix API table. */
void egui_view_button_matrix_on_draw(egui_view_t *self);
/** Initialize the grid-style button matrix widget. */
void egui_view_button_matrix_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_BUTTON_MATRIX_H_ */
