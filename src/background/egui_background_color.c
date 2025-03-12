#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_background_color.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

void egui_background_color_on_draw(egui_background_t *self, egui_region_t *region, const void *param)
{
    egui_background_color_t *local = (egui_background_color_t *)self;
    const egui_background_color_param_t *color_param = param;
    EGUI_UNUSED(local);

    switch (color_param->type)
    {
    case EGUI_BACKGROUND_COLOR_TYPE_SOLID:
        if (color_param->stroke_width == 0)
        {
            egui_canvas_draw_rectangle_fill(0, 0, region->size.width, region->size.height, color_param->color, color_param->alpha);
        }
        else
        {
            if (color_param->alpha != EGUI_ALPHA_0)
            {
                egui_canvas_draw_rectangle_fill(color_param->stroke_width / 2, color_param->stroke_width / 2,
                                                region->size.width - color_param->stroke_width, region->size.height - color_param->stroke_width,
                                                color_param->color, color_param->alpha);
            }
            egui_canvas_draw_rectangle(0, 0, region->size.width, region->size.height, color_param->stroke_width, color_param->stroke_color,
                                       color_param->stroke_alpha);
        }
        break;
    case EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE:
        if (color_param->stroke_width == 0)
        {
            egui_canvas_draw_round_rectangle_fill(0, 0, region->size.width, region->size.height, color_param->shape.round_rectangle.radius,
                                                  color_param->color, color_param->alpha);
        }
        else
        {
            if (color_param->alpha != EGUI_ALPHA_0)
            {
                egui_canvas_draw_round_rectangle_fill(color_param->stroke_width / 2, color_param->stroke_width / 2,
                                                      region->size.width - color_param->stroke_width, region->size.height - color_param->stroke_width,
                                                      color_param->shape.round_rectangle.radius, color_param->color, color_param->alpha);
            }
            egui_canvas_draw_round_rectangle(0, 0, region->size.width, region->size.height, color_param->shape.round_rectangle.radius,
                                             color_param->stroke_width, color_param->stroke_color, color_param->stroke_alpha);
        }
        break;
    case EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE_CORNERS:
        if (color_param->stroke_width == 0)
        {
            egui_canvas_draw_round_rectangle_corners_fill(
                    0, 0, region->size.width, region->size.height, color_param->shape.round_rectangle_corners.radius_left_top,
                    color_param->shape.round_rectangle_corners.radius_left_bottom, color_param->shape.round_rectangle_corners.radius_right_top,
                    color_param->shape.round_rectangle_corners.radius_right_bottom, color_param->color, color_param->alpha);
        }
        else
        {
            if (color_param->alpha != EGUI_ALPHA_0)
            {
                egui_canvas_draw_round_rectangle_corners_fill(
                        color_param->stroke_width / 2, color_param->stroke_width / 2, region->size.width - color_param->stroke_width,
                        region->size.height - color_param->stroke_width, color_param->shape.round_rectangle_corners.radius_left_top - color_param->stroke_width / 2,
                        color_param->shape.round_rectangle_corners.radius_left_bottom - color_param->stroke_width / 2,
                        color_param->shape.round_rectangle_corners.radius_right_top - color_param->stroke_width / 2,
                        color_param->shape.round_rectangle_corners.radius_right_bottom - color_param->stroke_width / 2, color_param->color, color_param->alpha);
            }
            egui_canvas_draw_round_rectangle_corners(
                    0, 0, region->size.width, region->size.height, color_param->shape.round_rectangle_corners.radius_left_top,
                    color_param->shape.round_rectangle_corners.radius_left_bottom, color_param->shape.round_rectangle_corners.radius_right_top,
                    color_param->shape.round_rectangle_corners.radius_right_bottom, color_param->stroke_width, color_param->stroke_color, color_param->stroke_alpha);
        }
        break;
    case EGUI_BACKGROUND_COLOR_TYPE_CIRCLE:
        if (color_param->stroke_width == 0)
        {
            egui_canvas_draw_circle_fill((region->size.width >> 1), (region->size.height >> 1), color_param->shape.circle.radius, color_param->color,
                                         color_param->alpha);
        }
        else
        {
            if (color_param->alpha != EGUI_ALPHA_0)
            {
                egui_canvas_draw_circle_fill((region->size.width >> 1), (region->size.height >> 1),
                                             color_param->shape.circle.radius - color_param->stroke_width / 2, color_param->color, color_param->alpha);
            }
            egui_canvas_draw_circle((region->size.width >> 1), (region->size.height >> 1), color_param->shape.circle.radius, color_param->stroke_width,
                                    color_param->stroke_color, color_param->stroke_alpha);
        }
        break;
    default:
        break;
    }
}

const egui_background_api_t egui_background_color_t_api_table = {
        .draw = egui_background_draw,
        .on_draw = egui_background_color_on_draw,
};

void egui_background_color_init(egui_background_t *self)
{
    egui_background_color_t *local = (egui_background_color_t *)self;
    EGUI_UNUSED(local);
    // call super init.
    egui_background_init(self);
    // update api.
    self->api = &egui_background_color_t_api_table;
}
