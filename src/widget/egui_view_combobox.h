#ifndef _EGUI_VIEW_COMBOBOX_H_
#define _EGUI_VIEW_COMBOBOX_H_

#include "egui_view.h"
#include "core/egui_theme.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_combobox_selected_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_combobox egui_view_combobox_t;
struct egui_view_combobox
{
    egui_view_t base;

    egui_view_on_combobox_selected_listener_t on_selected;
    const char **items;
    uint8_t item_count;
    uint8_t current_index;
    uint8_t is_expanded;
    uint8_t max_visible_items;

    egui_alpha_t alpha;
    egui_color_t text_color;
    egui_color_t bg_color;
    egui_color_t border_color;
    egui_color_t highlight_color;
    egui_color_t arrow_color;
    const egui_font_t *font;

    egui_dim_t collapsed_height;
    egui_dim_t item_height;
};

// ============== ComboBox Params ==============
typedef struct egui_view_combobox_params egui_view_combobox_params_t;
struct egui_view_combobox_params
{
    egui_region_t region;
    const char **items;
    uint8_t item_count;
    uint8_t current_index;
};

#define EGUI_VIEW_COMBOBOX_PARAMS_INIT(_name, _x, _y, _w, _h, _items, _count, _index)                                                                          \
    static const egui_view_combobox_params_t _name = {                                                                                                         \
            .region = {{(_x), (_y)}, {(_w), (_h)}}, .items = (_items), .item_count = (_count), .current_index = (_index)}

void egui_view_combobox_apply_params(egui_view_t *self, const egui_view_combobox_params_t *params);
void egui_view_combobox_init_with_params(egui_view_t *self, const egui_view_combobox_params_t *params);

void egui_view_combobox_set_on_selected_listener(egui_view_t *self, egui_view_on_combobox_selected_listener_t listener);
void egui_view_combobox_set_items(egui_view_t *self, const char **items, uint8_t count);
void egui_view_combobox_set_current_index(egui_view_t *self, uint8_t index);
uint8_t egui_view_combobox_get_current_index(egui_view_t *self);
const char *egui_view_combobox_get_current_text(egui_view_t *self);
void egui_view_combobox_set_max_visible_items(egui_view_t *self, uint8_t max_items);
void egui_view_combobox_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_combobox_expand(egui_view_t *self);
void egui_view_combobox_collapse(egui_view_t *self);
uint8_t egui_view_combobox_is_expanded(egui_view_t *self);
void egui_view_combobox_on_draw(egui_view_t *self);
void egui_view_combobox_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_COMBOBOX_H_ */
