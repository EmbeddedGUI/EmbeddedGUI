#ifndef _EGUI_BACKGROUND_COLOR_H_
#define _EGUI_BACKGROUND_COLOR_H_

#include "egui_background.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_BACKGROUND_COLOR_TYPE_SOLID                   0
#define EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE         1
#define EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE_CORNERS 2
#define EGUI_BACKGROUND_COLOR_TYPE_CIRCLE                  3

#define EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(_name, _color, _alpha)                                                                                          \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_SOLID,                                                              \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = 0,                                                                                     \
                                                        .stroke_alpha = 0,                                                                                     \
                                                        .stroke_color = {0},                                                                                   \
                                                        .round_rectangle = {.radius = 0}}

#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(_name, _color, _alpha, _radius)                                                                       \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE,                                                    \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = 0,                                                                                     \
                                                        .stroke_alpha = 0,                                                                                     \
                                                        .stroke_color = {0},                                                                                   \
                                                        .round_rectangle = {.radius = _radius}}

#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS(_name, _color, _alpha, _radius_left_top, _radius_left_bottom, _radius_right_top,              \
                                                                 _radius_right_bottom)                                                                         \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE_CORNERS,                                            \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = 0,                                                                                     \
                                                        .stroke_alpha = 0,                                                                                     \
                                                        .stroke_color = {0},                                                                                   \
                                                        .round_rectangle_corners = {.radius_left_top = _radius_left_top,                                       \
                                                                                    .radius_left_bottom = _radius_left_bottom,                                 \
                                                                                    .radius_right_top = _radius_right_top,                                     \
                                                                                    .radius_right_bottom = _radius_right_bottom}}

#define EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(_name, _color, _alpha, _radius)                                                                                \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_CIRCLE,                                                             \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = 0,                                                                                     \
                                                        .stroke_alpha = 0,                                                                                     \
                                                        .stroke_color = {0},                                                                                   \
                                                        .circle = {.radius = _radius}}

#define EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE(_name, _color, _alpha, _stroke_width, _stroke_color, _stroke_alpha)                                      \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_SOLID,                                                              \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = _stroke_width,                                                                         \
                                                        .stroke_alpha = _stroke_alpha,                                                                         \
                                                        .stroke_color = _stroke_color,                                                                         \
                                                        .round_rectangle = {.radius = 0}}

#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(_name, _color, _alpha, _radius, _stroke_width, _stroke_color, _stroke_alpha)                   \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE,                                                    \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = _stroke_width,                                                                         \
                                                        .stroke_alpha = _stroke_alpha,                                                                         \
                                                        .stroke_color = _stroke_color,                                                                         \
                                                        .round_rectangle = {.radius = _radius}}

#define EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS_STROKE(_name, _color, _alpha, _radius_left_top, _radius_left_bottom, _radius_right_top,       \
                                                                        _radius_right_bottom, _stroke_width, _stroke_color, _stroke_alpha)                     \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE_CORNERS,                                            \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = _stroke_width,                                                                         \
                                                        .stroke_alpha = _stroke_alpha,                                                                         \
                                                        .stroke_color = _stroke_color,                                                                         \
                                                        .round_rectangle_corners = {.radius_left_top = _radius_left_top,                                       \
                                                                                    .radius_left_bottom = _radius_left_bottom,                                 \
                                                                                    .radius_right_top = _radius_right_top,                                     \
                                                                                    .radius_right_bottom = _radius_right_bottom}}

#define EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE_STROKE(_name, _color, _alpha, _radius, _stroke_width, _stroke_color, _stroke_alpha)                            \
    static const egui_background_color_param_t _name = {.type = EGUI_BACKGROUND_COLOR_TYPE_CIRCLE,                                                             \
                                                        .alpha = _alpha,                                                                                       \
                                                        .color = _color,                                                                                       \
                                                        .stroke_width = _stroke_width,                                                                         \
                                                        .stroke_alpha = _stroke_alpha,                                                                         \
                                                        .stroke_color = _stroke_color,                                                                         \
                                                        .circle = {.radius = _radius}}

extern const egui_background_api_t egui_background_color_t_api_table;

#define EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(_name, _params)                                                                                                \
    static const egui_background_color_t _name = {.base = {.api = &egui_background_color_t_api_table, .params = _params}}

typedef struct egui_background_color_param egui_background_color_param_t;
struct egui_background_color_param
{
    uint8_t type; // 0: solid color, 1: rounded rectangle, 2: circle
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
    };
};

typedef struct egui_background_color egui_background_color_t;
struct egui_background_color
{
    egui_background_t base;
};

void egui_background_color_on_draw(egui_background_t *self, egui_region_t *region, const void *param);
void egui_background_color_init(egui_background_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BACKGROUND_COLOR_H_ */
