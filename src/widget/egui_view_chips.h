#ifndef _EGUI_VIEW_CHIPS_H_
#define _EGUI_VIEW_CHIPS_H_

#include "egui_view_button_matrix.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired when one chip cell is clicked or selected. */
typedef egui_view_button_matrix_click_cb_t egui_view_on_chip_selected_listener_t;

typedef struct egui_view_chips egui_view_chips_t;
/**
 * @brief Thin chips wrapper built on top of the button-matrix widget.
 *
 * The wrapper enables persistent single-selection by default and renames the
 * underlying button-matrix API to match chip-style UI terminology.
 */
struct egui_view_chips
{
    egui_view_button_matrix_t matrix;
};

typedef struct egui_view_chips_params egui_view_chips_params_t;
/**
 * @brief Construction-time parameter block for one chips group.
 */
struct egui_view_chips_params
{
    egui_region_t region;
    const char **labels;
    const char **icons;
    uint8_t chip_count;
    uint8_t cols;
    uint8_t gap;
};

/** Build a chips parameter block with labels, optional wrapping, and gap. */
#define EGUI_VIEW_CHIPS_PARAMS_INIT(_name, _x, _y, _w, _h, _labels, _count, _cols, _gap)                                                                       \
    static const egui_view_chips_params_t _name = {                                                                                                            \
            .region = {{(_x), (_y)}, {(_w), (_h)}}, .labels = (_labels), .icons = NULL, .chip_count = (_count), .cols = (_cols), .gap = (_gap)}

/** Apply a chips parameter block after initialization. */
void egui_view_chips_apply_params(egui_view_t *self, const egui_view_chips_params_t *params);
/** Initialize a chips view and immediately apply its parameter block. */
void egui_view_chips_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_chips_params_t *params);

/** Set chip labels and column count. A zero or oversized column count expands to one row. */
void egui_view_chips_set_chips(egui_view_t *self, const char **labels, uint8_t count, uint8_t cols);
/** Set the optional icon array that parallels the label array. */
void egui_view_chips_set_chip_icons(egui_view_t *self, const char **icons);
/** Register the callback fired when a chip is clicked by the user. */
void egui_view_chips_set_on_selected_listener(egui_view_t *self, egui_view_on_chip_selected_listener_t listener);
/** Set the gap between chip cells. */
void egui_view_chips_set_gap(egui_view_t *self, uint8_t gap);
/** Set the selected chip programmatically. This updates highlight state but does not fire the click listener. */
void egui_view_chips_set_selected_index(egui_view_t *self, uint8_t index);
/** Clear the stored selection. */
void egui_view_chips_clear_selection(egui_view_t *self);
/** Return the selected chip index, or `SELECTED_NONE` when nothing is active. */
uint8_t egui_view_chips_get_selected_index(egui_view_t *self);
/** Set the corner radius used for each chip. */
void egui_view_chips_set_corner_radius(egui_view_t *self, uint8_t radius);
/** Set the normal background color for unselected chips. */
void egui_view_chips_set_bg_color(egui_view_t *self, egui_color_t color);
/** Set the background color used for the selected chip. */
void egui_view_chips_set_selected_bg_color(egui_view_t *self, egui_color_t color);
/** Set the text tint color used by every chip label. */
void egui_view_chips_set_text_color(egui_view_t *self, egui_color_t color);
/** Set the border color drawn around every chip. */
void egui_view_chips_set_border_color(egui_view_t *self, egui_color_t color);
/** Override the font used for chip labels. */
void egui_view_chips_set_font(egui_view_t *self, const egui_font_t *font);
/** Override the icon font used when chip icons are configured. */
void egui_view_chips_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Set the vertical gap between one chip icon and its text. */
void egui_view_chips_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
/** Initialize chips as a button matrix with persistent single-selection enabled. */
void egui_view_chips_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHIPS_H_ */
