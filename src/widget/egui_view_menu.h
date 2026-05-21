#ifndef _EGUI_VIEW_MENU_H_
#define _EGUI_VIEW_MENU_H_

#include "egui_view.h"
#include "font/egui_font.h"

/**
 * @brief Stack-based menu widget that renders one page at a time.
 *
 * Callers provide a borrowed tree of static page descriptors. The widget keeps
 * only navigation state, row sizing, and paint configuration for the current
 * page plus a short back stack for submenu traversal.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Fixed capacities used by the menu's borrowed descriptor tables and back stack. */
#define EGUI_VIEW_MENU_MAX_PAGES 8
#define EGUI_VIEW_MENU_MAX_ITEMS 8
#define EGUI_VIEW_MENU_MAX_STACK 4

/* Marker stored in `sub_page_index` for leaf rows without a submenu target. */
#define EGUI_VIEW_MENU_ITEM_LEAF     0xFF
/* Sentinel used when no row is currently selected by keyboard navigation. */
#define EGUI_VIEW_MENU_SELECTED_NONE 0xFF

typedef struct egui_view_menu_item
{
    /* Primary label shown for one menu row. */
    const char *text;
    /* Destination page index, or `EGUI_VIEW_MENU_ITEM_LEAF` for a leaf action. */
    uint8_t sub_page_index; // 0xFF = leaf item (no sub-page)
    /* Optional leading icon glyph string rendered with the icon font. */
    const char *icon;
} egui_view_menu_item_t;

typedef struct egui_view_menu_page
{
    /* Title rendered in the header while this page is active. */
    const char *title;
    /* Borrowed row descriptors for this page. */
    const egui_view_menu_item_t *items;
    /* Number of valid items in `items`. */
    uint8_t item_count;
} egui_view_menu_page_t;

/** Listener fired when a leaf item is activated. */
typedef void (*egui_view_menu_item_click_cb_t)(egui_view_t *self, uint8_t page_index, uint8_t item_index);

typedef struct egui_view_menu egui_view_menu_t;
struct egui_view_menu
{
    egui_view_t base;

    /* Borrowed page table for the entire menu tree. */
    const egui_view_menu_page_t *pages;
    /* Number of valid pages in `pages`. */
    uint8_t page_count;
    /* Currently visible page index. */
    uint8_t current_page;
    /* Back-stack of previously visited page indices. */
    uint8_t page_stack[EGUI_VIEW_MENU_MAX_STACK];
    /* Number of entries currently stored in `page_stack`. */
    uint8_t stack_depth;

    /* Header row height. */
    egui_dim_t header_height;
    /* Height of each menu item row. */
    egui_dim_t item_height;
    /* Background color of the header row. */
    egui_color_t header_color;
    /* Background color of normal item rows. */
    egui_color_t item_color;
    /* Text and icon color for item rows. */
    egui_color_t text_color;
    /* Text and icon color for the header row. */
    egui_color_t header_text_color;
    /* Highlight color used for pressed rows and separators. */
    egui_color_t highlight_color;
    /* Horizontal space between an item icon and its label. */
    egui_dim_t icon_text_gap;
    /* Font used for titles and item labels. */
    const egui_font_t *font;
    /* Optional icon font override for item and navigation glyphs. */
    const egui_font_t *icon_font;
    /* Optional glyph for the header back button. */
    const char *back_icon;
    /* Optional glyph for rows that open submenus. */
    const char *submenu_icon;

    /* Press tracking: `-1` none, `-2` back button, otherwise item index. */
    int8_t pressed_index; // -1 = none, -2 = back button
    /* Keyboard-selected row index on the current page, or `EGUI_VIEW_MENU_SELECTED_NONE`. */
    uint8_t selected_index;
    /* Leaf-item activation callback. */
    egui_view_menu_item_click_cb_t on_item_click;
};

// ============== Menu Params ==============
typedef struct egui_view_menu_params egui_view_menu_params_t;
struct egui_view_menu_params
{
    egui_region_t region;
    egui_dim_t header_height;
    egui_dim_t item_height;
};

/** Build a menu parameter block with region, header height, and item height. */
#define EGUI_VIEW_MENU_PARAMS_INIT(_name, _x, _y, _w, _h, _hdr_h, _item_h)                                                                                     \
    static const egui_view_menu_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .header_height = (_hdr_h), .item_height = (_item_h)}

/** Borrow the page array, reset navigation to page 0, and clear the back stack. */
void egui_view_menu_set_pages(egui_view_t *self, const egui_view_menu_page_t *pages, uint8_t page_count);
/** Return the borrowed page array currently attached to the menu. */
const egui_view_menu_page_t *egui_view_menu_get_pages(egui_view_t *self);
/** Return the number of pages currently attached to the menu. */
uint8_t egui_view_menu_get_page_count(egui_view_t *self);
/** Navigate to a page programmatically. The previous page is pushed to the back stack while capacity remains. */
void egui_view_menu_navigate_to(egui_view_t *self, uint8_t page_index);
/** Return the currently visible page index. */
uint8_t egui_view_menu_get_current_page(egui_view_t *self);
/** Return the current back-stack depth. */
uint8_t egui_view_menu_get_stack_depth(egui_view_t *self);
/** Return the currently selected row index, or `EGUI_VIEW_MENU_SELECTED_NONE`. */
uint8_t egui_view_menu_get_selected_index(egui_view_t *self);
/** Pop one page from the back stack. This is a no-op when there is no history. */
void egui_view_menu_go_back(egui_view_t *self);
/** Register the callback fired when a leaf item is released successfully. Submenu items navigate instead of calling this. */
void egui_view_menu_set_on_item_click(egui_view_t *self, egui_view_menu_item_click_cb_t callback);
/** Return the callback fired when a leaf item is released successfully. */
egui_view_menu_item_click_cb_t egui_view_menu_get_on_item_click(egui_view_t *self);
/** Set the header row height in pixels. */
void egui_view_menu_set_header_height(egui_view_t *self, egui_dim_t height);
/** Return the header row height in pixels. */
egui_dim_t egui_view_menu_get_header_height(egui_view_t *self);
/** Set the height of each menu item row in pixels. */
void egui_view_menu_set_item_height(egui_view_t *self, egui_dim_t height);
/** Return the item row height in pixels. */
egui_dim_t egui_view_menu_get_item_height(egui_view_t *self);
/** Return the text and icon color used by item rows. */
egui_color_t egui_view_menu_get_text_color(egui_view_t *self);
/** Set the text color used by the page title row. */
void egui_view_menu_set_header_text_color(egui_view_t *self, egui_color_t color);
/** Return the text color used by the page title row. */
egui_color_t egui_view_menu_get_header_text_color(egui_view_t *self);
/** Override the icon font used for optional item icons and navigation glyphs. */
void egui_view_menu_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the icon font override, or NULL when automatic icon-font resolution is used. */
const egui_font_t *egui_view_menu_get_icon_font(egui_view_t *self);
/** Override the Back and submenu glyph strings. Passing NULL resets each one to the default icon. */
void egui_view_menu_set_navigation_icons(egui_view_t *self, const char *back_icon, const char *submenu_icon);
/** Return the effective Back glyph string. */
const char *egui_view_menu_get_back_icon(egui_view_t *self);
/** Return the effective submenu glyph string. */
const char *egui_view_menu_get_submenu_icon(egui_view_t *self);
/** Set the horizontal gap between an item icon and its label text. Negative values clamp to 0. */
void egui_view_menu_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
/** Return the horizontal gap between an item icon and its label text. */
egui_dim_t egui_view_menu_get_icon_text_gap(egui_view_t *self);
/** Default draw hook used by the menu API table. */
void egui_view_menu_on_draw(egui_view_t *self);
/** Initialize the stacked menu widget with default colors, icons, and a bounded back stack. */
void egui_view_menu_init(egui_view_t *self, egui_core_t *core);
/** Apply the region, header height, and item height from one parameter block. */
void egui_view_menu_apply_params(egui_view_t *self, const egui_view_menu_params_t *params);
/** Initialize a menu and immediately apply its parameter block. */
void egui_view_menu_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_menu_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_MENU_H_ */
