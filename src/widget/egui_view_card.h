#ifndef _EGUI_VIEW_CARD_H_
#define _EGUI_VIEW_CARD_H_

#include "egui_view_group.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_card egui_view_card_t;
struct egui_view_card
{
    egui_view_group_t base;

    egui_dim_t corner_radius;
    egui_dim_t border_width;
    egui_color_t border_color;
    egui_color_t bg_color;
    egui_alpha_t bg_alpha;
    uint8_t bg_color_custom; // 1 if bg_color was explicitly set by user
};

// ============== Card Params ==============
typedef struct egui_view_card_params egui_view_card_params_t;
struct egui_view_card_params
{
    egui_region_t region;
    egui_dim_t corner_radius;
};

#define EGUI_VIEW_CARD_PARAMS_INIT(_name, _x, _y, _w, _h, _radius)                                                                                             \
    static const egui_view_card_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .corner_radius = (_radius)}

void egui_view_card_apply_params(egui_view_t *self, const egui_view_card_params_t *params);
void egui_view_card_init_with_params(egui_view_t *self, const egui_view_card_params_t *params);

void egui_view_card_set_corner_radius(egui_view_t *self, egui_dim_t radius);
void egui_view_card_set_border(egui_view_t *self, egui_dim_t width, egui_color_t color);
void egui_view_card_set_bg_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
void egui_view_card_add_child(egui_view_t *self, egui_view_t *child);
void egui_view_card_layout_childs(egui_view_t *self, uint8_t is_horizontal, uint8_t align_type);
void egui_view_card_on_draw(egui_view_t *self);
void egui_view_card_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CARD_H_ */
