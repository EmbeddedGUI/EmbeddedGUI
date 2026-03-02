#ifndef _EGUI_VIEW_MENU_H_
#define _EGUI_VIEW_MENU_H_

#include "egui_view.h"
#include "core/egui_theme.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_MENU_MAX_PAGES 8
#define EGUI_VIEW_MENU_MAX_ITEMS 8
#define EGUI_VIEW_MENU_MAX_STACK 4

#define EGUI_VIEW_MENU_ITEM_LEAF 0xFF

typedef struct egui_view_menu_item
{
    const char *text;
    uint8_t sub_page_index; // 0xFF = leaf item (no sub-page)
} egui_view_menu_item_t;

typedef struct egui_view_menu_page
{
    const char *title;
    const egui_view_menu_item_t *items;
    uint8_t item_count;
} egui_view_menu_page_t;

typedef void (*egui_view_menu_item_click_cb_t)(egui_view_t *self, uint8_t page_index, uint8_t item_index);

typedef struct egui_view_menu egui_view_menu_t;
struct egui_view_menu
{
    egui_view_t base;

    const egui_view_menu_page_t *pages;
    uint8_t page_count;
    uint8_t current_page;
    uint8_t page_stack[EGUI_VIEW_MENU_MAX_STACK];
    uint8_t stack_depth;

    egui_dim_t header_height;
    egui_dim_t item_height;
    egui_color_t header_color;
    egui_color_t item_color;
    egui_color_t text_color;
    egui_color_t highlight_color;
    const egui_font_t *font;

    int8_t pressed_index; // -1 = none, -2 = back button
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

#define EGUI_VIEW_MENU_PARAMS_INIT(_name, _x, _y, _w, _h, _hdr_h, _item_h)                                                                                     \
    static const egui_view_menu_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .header_height = (_hdr_h), .item_height = (_item_h)}

void egui_view_menu_set_pages(egui_view_t *self, const egui_view_menu_page_t *pages, uint8_t page_count);
void egui_view_menu_navigate_to(egui_view_t *self, uint8_t page_index);
void egui_view_menu_go_back(egui_view_t *self);
void egui_view_menu_set_on_item_click(egui_view_t *self, egui_view_menu_item_click_cb_t callback);
void egui_view_menu_set_header_height(egui_view_t *self, egui_dim_t height);
void egui_view_menu_set_item_height(egui_view_t *self, egui_dim_t height);
void egui_view_menu_on_draw(egui_view_t *self);
void egui_view_menu_init(egui_view_t *self);
void egui_view_menu_apply_params(egui_view_t *self, const egui_view_menu_params_t *params);
void egui_view_menu_init_with_params(egui_view_t *self, const egui_view_menu_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_MENU_H_ */
