#ifndef _EGUI_VIEW_DIVIDER_H_
#define _EGUI_VIEW_DIVIDER_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_divider egui_view_divider_t;
struct egui_view_divider
{
    egui_view_t base;

    egui_alpha_t alpha;
    egui_color_t color;
};

// ============== Divider Params ==============
typedef struct egui_view_divider_params egui_view_divider_params_t;
struct egui_view_divider_params
{
    egui_region_t region;
    egui_color_t color;
};

#define EGUI_VIEW_DIVIDER_PARAMS_INIT(_name, _x, _y, _w, _h, _color)                                                                                           \
    static const egui_view_divider_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .color = _color}

void egui_view_divider_apply_params(egui_view_t *self, const egui_view_divider_params_t *params);
void egui_view_divider_init_with_params(egui_view_t *self, const egui_view_divider_params_t *params);

void egui_view_divider_set_color(egui_view_t *self, egui_color_t color);
void egui_view_divider_on_draw(egui_view_t *self);
void egui_view_divider_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_DIVIDER_H_ */
