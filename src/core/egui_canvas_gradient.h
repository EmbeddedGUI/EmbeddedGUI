#ifndef _EGUI_CANVAS_GRADIENT_H_
#define _EGUI_CANVAS_GRADIENT_H_

#include "egui_common.h"
#include "image/egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Gradient type definitions */
#define EGUI_GRADIENT_TYPE_LINEAR_VERTICAL   0
#define EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL 1
#define EGUI_GRADIENT_TYPE_RADIAL            2
#define EGUI_GRADIENT_TYPE_ANGULAR           3

/* Maximum color stops (compile-time limit to bound stack usage) */
#ifndef EGUI_GRADIENT_MAX_STOPS
#define EGUI_GRADIENT_MAX_STOPS 8
#endif

/* A single color stop in the gradient */
typedef struct egui_gradient_stop egui_gradient_stop_t;
struct egui_gradient_stop
{
    uint8_t position; /* 0-255, maps to 0.0-1.0 along gradient axis */
    egui_color_t color;
};

/* Unified gradient descriptor */
#ifndef EGUI_GRADIENT_T_DEFINED
#define EGUI_GRADIENT_T_DEFINED
typedef struct egui_gradient egui_gradient_t;
#endif
struct egui_gradient
{
    uint8_t type;                      /* EGUI_GRADIENT_TYPE_* */
    uint8_t stop_count;                /* Number of stops (2..EGUI_GRADIENT_MAX_STOPS) */
    egui_alpha_t alpha;                /* Global alpha for the gradient */
    const egui_gradient_stop_t *stops; /* Pointer to const stop array (ROM-friendly) */
    /* Radial-specific fields (ignored for linear) */
    egui_dim_t center_x; /* Center X relative to shape origin */
    egui_dim_t center_y; /* Center Y relative to shape origin */
    egui_dim_t radius;   /* Gradient radius in pixels */
};

/* ---- Convenience macros for static initialization ---- */

/* Define a 2-stop linear gradient (backward compatible with old background_gradient) */
#define EGUI_GRADIENT_LINEAR_2COLOR(_name, _dir, _c0, _c1, _alpha)                                                                                             \
    static const egui_gradient_stop_t _name##_stops[] = {                                                                                                      \
            {.position = 0, .color = _c0},                                                                                                                     \
            {.position = 255, .color = _c1},                                                                                                                   \
    };                                                                                                                                                         \
    static const egui_gradient_t _name = {                                                                                                                     \
            .type = (_dir),                                                                                                                                    \
            .stop_count = 2,                                                                                                                                   \
            .alpha = (_alpha),                                                                                                                                 \
            .stops = _name##_stops,                                                                                                                            \
    }

/* Define a multi-stop linear gradient */
#define EGUI_GRADIENT_LINEAR(_name, _dir, _alpha, _stops_arr, _count)                                                                                          \
    static const egui_gradient_t _name = {                                                                                                                     \
            .type = (_dir),                                                                                                                                    \
            .stop_count = (_count),                                                                                                                            \
            .alpha = (_alpha),                                                                                                                                 \
            .stops = (_stops_arr),                                                                                                                             \
    }

/* Define a radial gradient */
#define EGUI_GRADIENT_RADIAL(_name, _alpha, _cx, _cy, _r, _stops_arr, _count)                                                                                  \
    static const egui_gradient_t _name = {                                                                                                                     \
            .type = EGUI_GRADIENT_TYPE_RADIAL,                                                                                                                 \
            .stop_count = (_count),                                                                                                                            \
            .alpha = (_alpha),                                                                                                                                 \
            .stops = (_stops_arr),                                                                                                                             \
            .center_x = (_cx),                                                                                                                                 \
            .center_y = (_cy),                                                                                                                                 \
            .radius = (_r),                                                                                                                                    \
    }

/* Define an angular (conic) gradient that sweeps 360 degrees around a center point.
 * Color maps from 0 degrees (right / 3 o'clock) clockwise through 360 degrees. */
#define EGUI_GRADIENT_ANGULAR(_name, _alpha, _cx, _cy, _stops_arr, _count)                                                                                     \
    static const egui_gradient_t _name = {                                                                                                                     \
            .type = EGUI_GRADIENT_TYPE_ANGULAR,                                                                                                                \
            .stop_count = (_count),                                                                                                                            \
            .alpha = (_alpha),                                                                                                                                 \
            .stops = (_stops_arr),                                                                                                                             \
            .center_x = (_cx),                                                                                                                                 \
            .center_y = (_cy),                                                                                                                                 \
    }

/* ---- Core utility ---- */

/* Compute gradient color at position t (0-255) given a stop array */
egui_color_t egui_gradient_get_color(const egui_gradient_stop_t *stops, uint8_t stop_count, uint8_t t);

#if EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
/* Compute gradient color with ordered dithering applied.
 * px, py are absolute screen coordinates used for Bayer matrix lookup. */
egui_color_t egui_gradient_get_color_dithered(const egui_gradient_stop_t *stops, uint8_t stop_count, uint8_t t, egui_dim_t px, egui_dim_t py);
#endif

/* ---- Gradient fill functions ---- */

/* Rectangle gradient fill */
void egui_canvas_draw_rectangle_fill_gradient(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const egui_gradient_t *gradient);

/* Round rectangle gradient fill (uniform radius) */
void egui_canvas_draw_round_rectangle_fill_gradient(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius,
                                                    const egui_gradient_t *gradient);

/* Round rectangle gradient fill (per-corner radii) */
void egui_canvas_draw_round_rectangle_corners_fill_gradient(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius_left_top,
                                                            egui_dim_t radius_left_bottom, egui_dim_t radius_right_top, egui_dim_t radius_right_bottom,
                                                            const egui_gradient_t *gradient);

/* Circle gradient fill */
void egui_canvas_draw_circle_fill_gradient(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, const egui_gradient_t *gradient);

/* Triangle gradient fill */
void egui_canvas_draw_triangle_fill_gradient(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t x3, egui_dim_t y3,
                                             const egui_gradient_t *gradient);

/* Ellipse gradient fill */
void egui_canvas_draw_ellipse_fill_gradient(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius_x, egui_dim_t radius_y,
                                            const egui_gradient_t *gradient);

/* Polygon gradient fill */
void egui_canvas_draw_polygon_fill_gradient(const egui_dim_t *points, uint8_t count, const egui_gradient_t *gradient);

/* ---- Public gradient color utility ---- */

/* Get gradient color for a pixel at (px, py) relative to shape origin with shape size (w, h).
 * Exposed for use by mask and other modules. */
egui_color_t egui_gradient_color_at_pos(const egui_gradient_t *gradient, egui_dim_t px, egui_dim_t py, egui_dim_t w, egui_dim_t h);

/* ---- Ring / stroke shape gradient fill ---- */

/* Circle ring (annulus) gradient fill: outer_r and inner_r define the ring width.
 * Both inner and outer edges are anti-aliased. */
void egui_canvas_draw_circle_ring_fill_gradient(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t outer_r, egui_dim_t inner_r,
                                                const egui_gradient_t *gradient);

/* Rectangle ring (stroke) gradient fill: stroke_w pixels wide on all four sides. */
void egui_canvas_draw_rectangle_ring_fill_gradient(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t stroke_w,
                                                   const egui_gradient_t *gradient);

/* Round rectangle ring (stroke) gradient fill: stroke_w pixels wide with rounded corners.
 * The outer corner radius is given; inner radius is max(0, radius - stroke_w). */
void egui_canvas_draw_round_rectangle_ring_fill_gradient(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t stroke_w,
                                                         egui_dim_t radius, const egui_gradient_t *gradient);

/* Capsule line (round-capped stroke) gradient fill.
 * Draws a stroke of width stroke_w from (x1,y1) to (x2,y2) with circular caps at each end. */
void egui_canvas_draw_line_capsule_fill_gradient(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_w,
                                                 const egui_gradient_t *gradient);

/* Arc ring gradient fill: a sector of an annulus defined by outer_r, inner_r and angle range.
 * Angles in degrees: 0 = right (3 o'clock), increasing clockwise (screen convention).
 * Both inner and outer circle edges and the two radial cut edges are anti-aliased. */
void egui_canvas_draw_arc_ring_fill_gradient(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t outer_r, egui_dim_t inner_r, int32_t start_angle_deg,
                                             int32_t end_angle_deg, const egui_gradient_t *gradient);

/* Arc ring gradient fill with round caps.
 * cap_mode: 0=no caps, 1=end cap only, 2=start cap only, 3=both caps.
 * Caps are drawn as filled circles at the midpoint of the ring stroke. */
#define EGUI_ARC_CAP_NONE  0
#define EGUI_ARC_CAP_END   1
#define EGUI_ARC_CAP_START 2
#define EGUI_ARC_CAP_BOTH  3
void egui_canvas_draw_arc_ring_fill_gradient_round_cap(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t outer_r, egui_dim_t inner_r,
                                                       int32_t start_angle_deg, int32_t end_angle_deg, const egui_gradient_t *gradient, uint8_t cap_mode);

/* ---- Image + gradient overlay ---- */

/* Draw an image and overlay a gradient on top with the given overlay_alpha blend strength.
 * w, h specify the display size (matching egui_canvas_draw_image_resize).
 * overlay_alpha=0 means full original image color; overlay_alpha=255 means full gradient color. */
void egui_canvas_draw_image_gradient_overlay(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, const egui_gradient_t *gradient,
                                             egui_alpha_t overlay_alpha);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CANVAS_GRADIENT_H_ */
