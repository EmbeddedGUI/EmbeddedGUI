#ifndef _EGUI_VIEW_LINE_H_
#define _EGUI_VIEW_LINE_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief One point in the line widget's local coordinate space.
 */
typedef struct egui_view_line_point
{
    egui_dim_t x;
    egui_dim_t y;
} egui_view_line_point_t;

typedef struct egui_view_line egui_view_line_t;
/**
 * @brief Simple polyline widget backed by a borrowed point array.
 */
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
/**
 * @brief Construction-time parameter block for one line widget.
 */
struct egui_view_line_params
{
    egui_region_t region;
    uint8_t line_width;
    egui_color_t line_color;
};

/** Build a line parameter block with region, stroke width, and stroke color. */
#define EGUI_VIEW_LINE_PARAMS_INIT(_name, _x, _y, _w, _h, _line_width, _line_color)                                                                            \
    static const egui_view_line_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .line_width = (_line_width), .line_color = _line_color}

/** Apply a line-view parameter block after initialization. */
void egui_view_line_apply_params(egui_view_t *self, const egui_view_line_params_t *params);
/** Initialize a line view and immediately apply its parameter block. */
void egui_view_line_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_line_params_t *params);

/** Set the polyline point array. The view keeps the pointer, so the caller must keep the data alive. */
void egui_view_line_set_points(egui_view_t *self, const egui_view_line_point_t *points, uint8_t count);
/** Return the borrowed polyline point array. */
const egui_view_line_point_t *egui_view_line_get_points(egui_view_t *self);
/** Return the current number of points in the borrowed polyline array. */
uint8_t egui_view_line_get_point_count(egui_view_t *self);
/** Set the stroke width used for every segment. */
void egui_view_line_set_line_width(egui_view_t *self, uint8_t width);
/** Return the stroke width used for every segment. */
uint8_t egui_view_line_get_line_width(egui_view_t *self);
/** Set the stroke color used for every segment. */
void egui_view_line_set_line_color(egui_view_t *self, egui_color_t color);
/** Return the stroke color used for every segment. */
egui_color_t egui_view_line_get_line_color(egui_view_t *self);
/** Toggle rounded end caps for each line segment. */
void egui_view_line_set_use_round_cap(egui_view_t *self, uint8_t enable);
/** Return whether rounded end caps are enabled for line segments. */
uint8_t egui_view_line_get_use_round_cap(egui_view_t *self);
/** Default draw hook used by the line API table. */
void egui_view_line_on_draw(egui_view_t *self);
/** Initialize an empty polyline view. Nothing is drawn until at least two points are provided. */
void egui_view_line_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LINE_H_ */
