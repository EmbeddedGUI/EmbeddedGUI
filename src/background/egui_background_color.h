#ifndef _EGUI_BACKGROUND_COLOR_H_
#define _EGUI_BACKGROUND_COLOR_H_

#include "egui_background.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Fill the whole view area with one solid rectangle. */
#define EGUI_BACKGROUND_COLOR_TYPE_SOLID                   0
/** Fill the view area with one rounded rectangle that shares a single radius. */
#define EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE         1
/** Fill the view area with one rounded rectangle that uses per-corner radii. */
#define EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE_CORNERS 2
/** Fill the view area with one circle centered inside the region. */
#define EGUI_BACKGROUND_COLOR_TYPE_CIRCLE                  3

/** Build one solid-color parameter block without a stroke. */
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_COLOR(_name, _color, _alpha)                                                                                    \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_SOLID,                                                              \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = 0,                                                                                     \
                                                        .stroke_alpha = 0,                                                                                     \
                                                        .stroke_color = {0},                                                                                   \
                                                        .shape.round_rectangle = {.radius = 0}}

#if defined(_MSC_VER)
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(_name, _color, _alpha) EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_COLOR(_name, _color##_INIT, _alpha)
#else
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(_name, _color, _alpha) EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_COLOR(_name, (_color), _alpha)
#endif

/** Build one rounded-rectangle parameter block without a stroke. */
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_COLOR(_name, _color, _alpha, _radius)                                                                 \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE,                                                    \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = 0,                                                                                     \
                                                        .stroke_alpha = 0,                                                                                     \
                                                        .stroke_color = {0},                                                                                   \
                                                        .shape.round_rectangle = {.radius = _radius}}

#if defined(_MSC_VER)
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(_name, _color, _alpha, _radius)                                                                       \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_COLOR(_name, _color##_INIT, _alpha, _radius)
#else
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(_name, _color, _alpha, _radius)                                                                       \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_COLOR(_name, (_color), _alpha, _radius)
#endif

/** Build one per-corner rounded-rectangle parameter block without a stroke. */
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS_COLOR(_name, _color, _alpha, _radius_left_top, _radius_left_bottom, _radius_right_top,        \
                                                                       _radius_right_bottom)                                                                   \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE_CORNERS,                                            \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = 0,                                                                                     \
                                                        .stroke_alpha = 0,                                                                                     \
                                                        .stroke_color = {0},                                                                                   \
                                                        .shape.round_rectangle_corners = {.radius_left_top = _radius_left_top,                                 \
                                                                                          .radius_left_bottom = _radius_left_bottom,                           \
                                                                                          .radius_right_top = _radius_right_top,                               \
                                                                                          .radius_right_bottom = _radius_right_bottom}}

#if defined(_MSC_VER)
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS(_name, _color, _alpha, _radius_left_top, _radius_left_bottom, _radius_right_top,              \
                                                                 _radius_right_bottom)                                                                         \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS_COLOR(_name, _color##_INIT, _alpha, _radius_left_top, _radius_left_bottom, _radius_right_top,     \
                                                                   _radius_right_bottom)
#else
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS(_name, _color, _alpha, _radius_left_top, _radius_left_bottom, _radius_right_top,              \
                                                                 _radius_right_bottom)                                                                         \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS_COLOR(_name, (_color), _alpha, _radius_left_top, _radius_left_bottom, _radius_right_top,          \
                                                                   _radius_right_bottom)
#endif

/** Build one circular parameter block without a stroke. */
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE_COLOR(_name, _color, _alpha, _radius)                                                                          \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_CIRCLE,                                                             \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = 0,                                                                                     \
                                                        .stroke_alpha = 0,                                                                                     \
                                                        .stroke_color = {0},                                                                                   \
                                                        .shape.circle = {.radius = _radius}}

#if defined(_MSC_VER)
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(_name, _color, _alpha, _radius)                                                                                \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE_COLOR(_name, _color##_INIT, _alpha, _radius)
#else
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(_name, _color, _alpha, _radius) EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE_COLOR(_name, (_color), _alpha, _radius)
#endif

/** Build one solid-color parameter block with an optional stroke. */
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE_COLOR(_name, _color, _alpha, _stroke_width, _stroke_color, _stroke_alpha)                                \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_SOLID,                                                              \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = _stroke_width,                                                                         \
                                                        .stroke_alpha = _stroke_alpha,                                                                         \
                                                        .stroke_color = _stroke_color,                                                                         \
                                                        .shape.round_rectangle = {.radius = 0}}

#if defined(_MSC_VER)
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE(_name, _color, _alpha, _stroke_width, _stroke_color, _stroke_alpha)                                      \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE_COLOR(_name, _color##_INIT, _alpha, _stroke_width, _stroke_color##_INIT, _stroke_alpha)
#else
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE(_name, _color, _alpha, _stroke_width, _stroke_color, _stroke_alpha)                                      \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE_COLOR(_name, (_color), _alpha, _stroke_width, (_stroke_color), _stroke_alpha)
#endif

/** Build one rounded-rectangle parameter block with an optional stroke. */
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE_COLOR(_name, _color, _alpha, _radius, _stroke_width, _stroke_color, _stroke_alpha)             \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE,                                                    \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = _stroke_width,                                                                         \
                                                        .stroke_alpha = _stroke_alpha,                                                                         \
                                                        .stroke_color = _stroke_color,                                                                         \
                                                        .shape.round_rectangle = {.radius = _radius}}

#if defined(_MSC_VER)
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(_name, _color, _alpha, _radius, _stroke_width, _stroke_color, _stroke_alpha)                   \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE_COLOR(_name, _color##_INIT, _alpha, _radius, _stroke_width, _stroke_color##_INIT, _stroke_alpha)
#else
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(_name, _color, _alpha, _radius, _stroke_width, _stroke_color, _stroke_alpha)                   \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE_COLOR(_name, (_color), _alpha, _radius, _stroke_width, (_stroke_color), _stroke_alpha)
#endif

/** Build one per-corner rounded-rectangle parameter block with an optional stroke. */
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS_STROKE_COLOR(_name, _color, _alpha, _radius_left_top, _radius_left_bottom, _radius_right_top, \
                                                                              _radius_right_bottom, _stroke_width, _stroke_color, _stroke_alpha)               \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE_CORNERS,                                            \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = _stroke_width,                                                                         \
                                                        .stroke_alpha = _stroke_alpha,                                                                         \
                                                        .stroke_color = _stroke_color,                                                                         \
                                                        .shape.round_rectangle_corners = {.radius_left_top = _radius_left_top,                                 \
                                                                                          .radius_left_bottom = _radius_left_bottom,                           \
                                                                                          .radius_right_top = _radius_right_top,                               \
                                                                                          .radius_right_bottom = _radius_right_bottom}}

#if defined(_MSC_VER)
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS_STROKE(_name, _color, _alpha, _radius_left_top, _radius_left_bottom, _radius_right_top,       \
                                                                        _radius_right_bottom, _stroke_width, _stroke_color, _stroke_alpha)                     \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS_STROKE_COLOR(_name, _color##_INIT, _alpha, _radius_left_top, _radius_left_bottom,                 \
                                                                          _radius_right_top, _radius_right_bottom, _stroke_width, _stroke_color##_INIT,        \
                                                                          _stroke_alpha)
#else
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS_STROKE(_name, _color, _alpha, _radius_left_top, _radius_left_bottom, _radius_right_top,       \
                                                                        _radius_right_bottom, _stroke_width, _stroke_color, _stroke_alpha)                     \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS_STROKE_COLOR(_name, (_color), _alpha, _radius_left_top, _radius_left_bottom, _radius_right_top,   \
                                                                          _radius_right_bottom, _stroke_width, (_stroke_color), _stroke_alpha)
#endif

/** Build one circular parameter block with an optional stroke. */
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE_STROKE_COLOR(_name, _color, _alpha, _radius, _stroke_width, _stroke_color, _stroke_alpha)                      \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_CIRCLE,                                                             \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = _stroke_width,                                                                         \
                                                        .stroke_alpha = _stroke_alpha,                                                                         \
                                                        .stroke_color = _stroke_color,                                                                         \
                                                        .shape.circle = {.radius = _radius}}

#if defined(_MSC_VER)
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE_STROKE(_name, _color, _alpha, _radius, _stroke_width, _stroke_color, _stroke_alpha)                            \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE_STROKE_COLOR(_name, _color##_INIT, _alpha, _radius, _stroke_width, _stroke_color##_INIT, _stroke_alpha)
#else
#define EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE_STROKE(_name, _color, _alpha, _radius, _stroke_width, _stroke_color, _stroke_alpha)                            \
    EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE_STROKE_COLOR(_name, (_color), _alpha, _radius, _stroke_width, (_stroke_color), _stroke_alpha)
#endif

extern const egui_background_api_t egui_background_color_t_api_table;

/** Build one static color-background object that reuses a shared parameter table. */
#define EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(_name, _params)                                                                                                \
    static const egui_background_color_t _name = {.base = {.api = &egui_background_color_t_api_table, .params = _params}}

typedef struct egui_background_color_param egui_background_color_param_t;

/**
 * @brief Parameter block describing one color background state.
 *
 * The same struct covers several shape families. `type` selects which branch
 * of the
 * `shape` union is meaningful.
 */
struct egui_background_color_param
{
    uint8_t type; // one of EGUI_BACKGROUND_COLOR_TYPE_*
    egui_alpha_t alpha;
    egui_color_t color;
    egui_dim_t stroke_width;
    egui_alpha_t stroke_alpha;
    egui_color_t stroke_color;
    union
    {
        struct
        {
            egui_dim_t radius_left_top;
            egui_dim_t radius_left_bottom;
            egui_dim_t radius_right_top;
            egui_dim_t radius_right_bottom;
        } round_rectangle_corners;
        struct
        {
            egui_dim_t radius;
        } round_rectangle;
        struct
        {
            egui_dim_t radius;
        } circle;
    } shape;
};

typedef struct egui_background_color egui_background_color_t;

/**
 * @brief Concrete background subtype that renders color-based shapes.
 */
struct egui_background_color
{
    egui_background_t base;
};

/**
 * @brief Draw the selected color background inside the supplied region.
 */
void egui_background_color_on_draw(egui_background_t *self, egui_canvas_t *canvas, egui_region_t *region, const void *param);

/**
 * @brief Initialize a color-backed background implementation.
 */
void egui_background_color_init(egui_background_t *self);

/**
 * @brief Initialize a color-backed background and immediately bind its state table.
 */
void egui_background_color_init_with_params(egui_background_t *self, const egui_background_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BACKGROUND_COLOR_H_ */
