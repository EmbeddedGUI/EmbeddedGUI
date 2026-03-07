#ifndef _EGUI_VIEW_LINE_H_
#define _EGUI_VIEW_LINE_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_line_point
{
    egui_dim_t x;
    egui_dim_t y;
} egui_view_line_point_t;

typedef struct egui_view_line egui_view_line_t;
struct egui_view_line
{
    egui_view_t base;

    const egui_view_line_point_t *points;
    uint8_t point_count;
    uint8_t line_width;
    uint8_t use_round_cap;
    egui_color_t line_color;
};

// ============== Line Params ==============
typedef struct egui_view_line_params egui_view_line_params_t;
struct egui_view_line_params
{
    egui_region_t region;
    uint8_t line_width;
    egui_color_t line_color;
};

#define EGUI_VIEW_LINE_PARAMS_INIT(_name, _x, _y, _w, _h, _line_width, _line_color)                                                                            \
    static const egui_view_line_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .line_width = (_line_width), .line_color = _line_color}

void egui_view_line_apply_params(egui_view_t *self, const egui_view_line_params_t *params);
void egui_view_line_init_with_params(egui_view_t *self, const egui_view_line_params_t *params);

void egui_view_line_set_points(egui_view_t *self, const egui_view_line_point_t *points, uint8_t count);
void egui_view_line_set_line_width(egui_view_t *self, uint8_t width);
void egui_view_line_set_line_color(egui_view_t *self, egui_color_t color);
void egui_view_line_set_use_round_cap(egui_view_t *self, uint8_t enable);
void egui_view_line_on_draw(egui_view_t *self);
void egui_view_line_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LINE_H_ */
