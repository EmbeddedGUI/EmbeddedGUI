#ifndef _EGUI_VIEW_COMPASS_H_
#define _EGUI_VIEW_COMPASS_H_

#include "egui_view.h"
#include "core/egui_theme.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_compass egui_view_compass_t;
struct egui_view_compass
{
    egui_view_t base;

    int16_t heading; // 0-359 degrees
    egui_dim_t stroke_width;
    egui_color_t dial_color;
    egui_color_t north_color;
    egui_color_t needle_color;
    egui_color_t text_color;
    uint8_t show_degree;
    const egui_font_t *font;
};

// ============== Compass Params ==============
typedef struct egui_view_compass_params egui_view_compass_params_t;
struct egui_view_compass_params
{
    egui_region_t region;
    int16_t heading;
};

#define EGUI_VIEW_COMPASS_PARAMS_INIT(_name, _x, _y, _w, _h, _heading)                                                                                         \
    static const egui_view_compass_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .heading = (_heading)}

void egui_view_compass_apply_params(egui_view_t *self, const egui_view_compass_params_t *params);
void egui_view_compass_init_with_params(egui_view_t *self, const egui_view_compass_params_t *params);

void egui_view_compass_set_heading(egui_view_t *self, int16_t heading);
void egui_view_compass_set_show_degree(egui_view_t *self, uint8_t show);
void egui_view_compass_on_draw(egui_view_t *self);
void egui_view_compass_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_COMPASS_H_ */
