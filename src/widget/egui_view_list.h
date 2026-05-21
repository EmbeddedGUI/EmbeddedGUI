#ifndef _EGUI_VIEW_LIST_H_
#define _EGUI_VIEW_LIST_H_

#include "egui_view_scroll.h"
#include "egui_view_button.h"

/**
 * @brief Fixed-capacity scrollable list built from internal button rows.
 *
 * This widget targets small menus and settings lists where a lightweight,
 * non-virtualized implementation is enough. Each row is backed by an internal
 * button so touch feedback and click delivery reuse the existing button logic,
 * while icons and text are drawn by the list itself for consistent alignment.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of rows stored directly inside one list instance. */
#define EGUI_VIEW_LIST_MAX_ITEMS     16
/* Sentinel used when no row is currently selected by keyboard navigation. */
#define EGUI_VIEW_LIST_SELECTED_NONE 0xFF

/** Listener fired when a list row is clicked. */
typedef void (*egui_view_list_item_click_cb_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_list egui_view_list_t;
struct egui_view_list
{
    egui_view_scroll_t base;

    /* Internal button rows used for touch state and click dispatch. */
    egui_view_button_t items[EGUI_VIEW_LIST_MAX_ITEMS];
    /* Optional icon glyph string for each row. */
    const char *item_icons[EGUI_VIEW_LIST_MAX_ITEMS];
    /* Optional text label for each row. */
    const char *item_texts[EGUI_VIEW_LIST_MAX_ITEMS];
    /* Number of active rows in the fixed-capacity arrays. */
    uint8_t item_count;
    /* Shared row height. */
    egui_dim_t item_height;
    /* Horizontal gap between icon and text when a row shows both. */
    egui_dim_t icon_gap;
    /* Tint color used for icons. */
    egui_color_t icon_color;
    /* Optional icon font override. */
    const egui_font_t *icon_font;
    /* Row-click callback. */
    egui_view_list_item_click_cb_t on_item_click;
    /* Keyboard-selected row index, or `EGUI_VIEW_LIST_SELECTED_NONE`. */
    uint8_t selected_index;
};

// ============== List Params ==============
typedef struct egui_view_list_params egui_view_list_params_t;
struct egui_view_list_params
{
    egui_region_t region;
    egui_dim_t item_height;
};

/** Build a list parameter block with region and shared row height. */
#define EGUI_VIEW_LIST_PARAMS_INIT(_name, _x, _y, _w, _h, _item_h)                                                                                             \
    static const egui_view_list_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .item_height = (_item_h)}

/** Apply a list parameter block after initialization. */
void egui_view_list_apply_params(egui_view_t *self, const egui_view_list_params_t *params);
/** Initialize a list and immediately apply its parameter block. */
void egui_view_list_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_list_params_t *params);

/** Append a text-only row. Returns the new index, or `-1` when the fixed-capacity list is full. */
int8_t egui_view_list_add_item(egui_view_t *self, const char *text);
/** Append a row with both icon and text. Returns the new index, or `-1` when full. */
int8_t egui_view_list_add_item_with_icon(egui_view_t *self, const char *icon, const char *text);
/** Remove all rows and reset the list content. */
void egui_view_list_clear(egui_view_t *self);
/** Return the number of active rows. */
uint8_t egui_view_list_get_item_count(egui_view_t *self);
/** Set the height used for every row and relayout the list. */
void egui_view_list_set_item_height(egui_view_t *self, egui_dim_t height);
/** Return the height used for every row. */
egui_dim_t egui_view_list_get_item_height(egui_view_t *self);
/** Replace the icon of one existing row. */
void egui_view_list_set_item_icon(egui_view_t *self, uint8_t index, const char *icon);
/** Return the borrowed icon string for one row. */
const char *egui_view_list_get_item_icon(egui_view_t *self, uint8_t index);
/** Return the borrowed text string for one row. */
const char *egui_view_list_get_item_text(egui_view_t *self, uint8_t index);
/** Override the icon font used by rows that have icons. */
void egui_view_list_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the explicit icon font override, or NULL when auto-selection is used. */
const egui_font_t *egui_view_list_get_icon_font(egui_view_t *self);
/** Set the horizontal gap between a row icon and its text. */
void egui_view_list_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
/** Return the horizontal gap between a row icon and its text. */
egui_dim_t egui_view_list_get_icon_text_gap(egui_view_t *self);
/** Set the tint color used for all row icons. */
void egui_view_list_set_icon_color(egui_view_t *self, egui_color_t color);
/** Return the tint color used for all row icons. */
egui_color_t egui_view_list_get_icon_color(egui_view_t *self);
/** Register the callback fired when a row button is clicked. */
void egui_view_list_set_on_item_click(egui_view_t *self, egui_view_list_item_click_cb_t callback);
/** Return the callback fired when a row button is clicked. */
egui_view_list_item_click_cb_t egui_view_list_get_on_item_click(egui_view_t *self);
/** Select one row for keyboard navigation. Out-of-range indices are ignored. */
void egui_view_list_set_selected_index(egui_view_t *self, uint8_t index);
/** Return the keyboard-selected row index, or `EGUI_VIEW_LIST_SELECTED_NONE`. */
uint8_t egui_view_list_get_selected_index(egui_view_t *self);
/** Initialize the fixed-capacity scrollable list widget. */
void egui_view_list_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LIST_H_ */
