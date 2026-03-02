#ifndef _EGUI_VIEW_LIST_H_
#define _EGUI_VIEW_LIST_H_

#include "egui_view_scroll.h"
#include "egui_view_button.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_LIST_MAX_ITEMS 16

typedef void (*egui_view_list_item_click_cb_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_list egui_view_list_t;
struct egui_view_list
{
    egui_view_scroll_t base;

    egui_view_button_t items[EGUI_VIEW_LIST_MAX_ITEMS];
    uint8_t item_count;
    egui_dim_t item_height;
    egui_view_list_item_click_cb_t on_item_click;
};

// ============== List Params ==============
typedef struct egui_view_list_params egui_view_list_params_t;
struct egui_view_list_params
{
    egui_region_t region;
    egui_dim_t item_height;
};

#define EGUI_VIEW_LIST_PARAMS_INIT(_name, _x, _y, _w, _h, _item_h)                                                                                             \
    static const egui_view_list_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .item_height = (_item_h)}

void egui_view_list_apply_params(egui_view_t *self, const egui_view_list_params_t *params);
void egui_view_list_init_with_params(egui_view_t *self, const egui_view_list_params_t *params);

int8_t egui_view_list_add_item(egui_view_t *self, const char *text);
void egui_view_list_clear(egui_view_t *self);
void egui_view_list_set_item_height(egui_view_t *self, egui_dim_t height);
void egui_view_list_set_on_item_click(egui_view_t *self, egui_view_list_item_click_cb_t callback);
void egui_view_list_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LIST_H_ */
