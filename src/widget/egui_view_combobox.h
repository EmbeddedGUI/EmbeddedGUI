#ifndef _EGUI_VIEW_COMBOBOX_H_
#define _EGUI_VIEW_COMBOBOX_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_COMBOBOX_PRESSED_NONE 0xFF

/** Listener fired when the combobox commits a user-visible selection. */
typedef void (*egui_view_on_combobox_selected_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_combobox egui_view_combobox_t;
/**
 * @brief Combobox widget state stored directly after the base view.
 *
 * The widget keeps its item arrays by reference, so callers normally provide
 * static string tables. When expanded, the same view grows vertically instead
 * of spawning a popup object, which keeps the runtime model small and easy to
 * reason about on embedded targets.
 */
struct egui_view_combobox
{
    egui_view_t base;

    egui_view_on_combobox_selected_listener_t on_selected;
    const char **items;
    const char **item_icons;
    uint8_t item_count;
    uint8_t current_index;
    uint8_t is_expanded;
    uint8_t max_visible_items;
    uint8_t pressed_index;
    uint8_t pressed_is_header;

    egui_alpha_t alpha;
    egui_color_t text_color;
    egui_color_t bg_color;
    egui_color_t border_color;
    egui_color_t highlight_color;
    egui_color_t arrow_color;
    const egui_font_t *font;
    const egui_font_t *icon_font;
    const char *expand_icon;
    const char *collapse_icon;

    egui_dim_t collapsed_height;
    egui_dim_t item_height;
    egui_dim_t icon_text_gap;
};

// ============== ComboBox Params ==============
typedef struct egui_view_combobox_params egui_view_combobox_params_t;
/**
 * @brief Construction-time parameter block for one combobox.
 */
struct egui_view_combobox_params
{
    egui_region_t region;
    const char **items;
    const char **item_icons;
    uint8_t item_count;
    uint8_t current_index;
};

/** Build a parameter block with text items and no icon list. */
#define EGUI_VIEW_COMBOBOX_PARAMS_INIT(_name, _x, _y, _w, _h, _items, _count, _index)                                                                          \
    static const egui_view_combobox_params_t _name = {                                                                                                         \
            .region = {{(_x), (_y)}, {(_w), (_h)}}, .items = (_items), .item_icons = NULL, .item_count = (_count), .current_index = (_index)}

/** Apply a combobox parameter block after initialization. */
void egui_view_combobox_apply_params(egui_view_t *self, const egui_view_combobox_params_t *params);
/** Initialize a combobox and immediately apply its parameter block. */
void egui_view_combobox_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_combobox_params_t *params);

/** Register the callback fired when the user commits a selection. */
void egui_view_combobox_set_on_selected_listener(egui_view_t *self, egui_view_on_combobox_selected_listener_t listener);
/** Return the registered selection callback. */
egui_view_on_combobox_selected_listener_t egui_view_combobox_get_on_selected_listener(egui_view_t *self);
/** Replace the item array and visible item count. The current index is clamped if needed. */
void egui_view_combobox_set_items(egui_view_t *self, const char **items, uint8_t count);
/** Return the borrowed item array. */
const char **egui_view_combobox_get_items(egui_view_t *self);
/** Return the number of items currently in the list. Returns 0 when self is NULL. */
uint8_t egui_view_combobox_get_item_count(egui_view_t *self);
/** Set the optional icon array that parallels the item text array. */
void egui_view_combobox_set_item_icons(egui_view_t *self, const char **item_icons);
/** Return the optional icon array that parallels the item text array. */
const char **egui_view_combobox_get_item_icons(egui_view_t *self);
/** Change the current selection programmatically without firing the selection listener. */
void egui_view_combobox_set_current_index(egui_view_t *self, uint8_t index);
/** Return the currently selected item index. */
uint8_t egui_view_combobox_get_current_index(egui_view_t *self);
/** Return the text of the currently selected item, or NULL when there are no items. */
const char *egui_view_combobox_get_current_text(egui_view_t *self);
/** Limit how many items may be shown when the dropdown expands. */
void egui_view_combobox_set_max_visible_items(egui_view_t *self, uint8_t max_items);
/** Return the configured maximum number of visible dropdown items. */
uint8_t egui_view_combobox_get_max_visible_items(egui_view_t *self);
/** Override the font used for item text and header text. */
void egui_view_combobox_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the configured text font. */
const egui_font_t *egui_view_combobox_get_font(egui_view_t *self);
/** Override the icon font used for item icons and expand/collapse arrows. */
void egui_view_combobox_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the configured icon font override, or NULL when auto font selection is used. */
const egui_font_t *egui_view_combobox_get_icon_font(egui_view_t *self);
/** Override the icons used for collapsed and expanded arrow states. Passing NULL restores defaults. */
void egui_view_combobox_set_arrow_icons(egui_view_t *self, const char *expand_icon, const char *collapse_icon);
/** Return the icon used when the combobox is collapsed. */
const char *egui_view_combobox_get_expand_icon(egui_view_t *self);
/** Return the icon used when the combobox is expanded. */
const char *egui_view_combobox_get_collapse_icon(egui_view_t *self);
/** Set the horizontal gap between an item icon and its text. */
void egui_view_combobox_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
/** Return the horizontal gap between an item icon and its text. */
egui_dim_t egui_view_combobox_get_icon_text_gap(egui_view_t *self);
/** Expand the dropdown if there is enough space to show at least one item. */
void egui_view_combobox_expand(egui_view_t *self);
/** Collapse the dropdown back to its header height. */
void egui_view_combobox_collapse(egui_view_t *self);
/** Return whether the combobox is currently expanded. */
uint8_t egui_view_combobox_is_expanded(egui_view_t *self);
/** Default draw hook used by the combobox API table. */
void egui_view_combobox_on_draw(egui_view_t *self);
/** Initialize the focusable combobox widget. */
void egui_view_combobox_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_COMBOBOX_H_ */
