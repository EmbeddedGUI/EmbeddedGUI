#ifndef _EGUI_VIEW_ROLLER_H_
#define _EGUI_VIEW_ROLLER_H_

#include "egui_view.h"
#include "core/egui_theme.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_roller_selected_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_roller egui_view_roller_t;
struct egui_view_roller
{
    egui_view_t base;

    egui_view_on_roller_selected_listener_t on_selected;
    const char **items;
    uint8_t item_count;
    uint8_t current_index;
    uint8_t visible_count;
    egui_dim_t scroll_offset;
    egui_dim_t last_touch_y;
    uint8_t is_dragging;
    egui_color_t text_color;
    egui_color_t selected_text_color;
    egui_color_t highlight_color;
    const egui_font_t *font;
};

// ============== Roller Params ==============
typedef struct egui_view_roller_params egui_view_roller_params_t;
struct egui_view_roller_params
{
    egui_region_t region;
    const char **items;
    uint8_t item_count;
    uint8_t current_index;
};

#define EGUI_VIEW_ROLLER_PARAMS_INIT(_name, _x, _y, _w, _h, _items, _count, _index)                                                                            \
    static const egui_view_roller_params_t _name = {                                                                                                           \
            .region = {{(_x), (_y)}, {(_w), (_h)}}, .items = (_items), .item_count = (_count), .current_index = (_index)}

void egui_view_roller_apply_params(egui_view_t *self, const egui_view_roller_params_t *params);
void egui_view_roller_init_with_params(egui_view_t *self, const egui_view_roller_params_t *params);

void egui_view_roller_set_on_selected_listener(egui_view_t *self, egui_view_on_roller_selected_listener_t listener);
void egui_view_roller_set_items(egui_view_t *self, const char **items, uint8_t count);
void egui_view_roller_set_current_index(egui_view_t *self, uint8_t index);
uint8_t egui_view_roller_get_current_index(egui_view_t *self);
void egui_view_roller_on_draw(egui_view_t *self);
void egui_view_roller_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ROLLER_H_ */
