#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_background_color.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

/**
 * @file egui_background_color.c
 * @brief Color-background implementation built on top of canvas primitives.
 *
 * This module translates one compact parameter struct into the matching canvas
 * draw calls. It intentionally delegates all rasterization details such as
 * anti-aliasing, stroke blending, and clipping to the canvas layer.
 */

/**
 * @brief Draw one color background using the configured shape and stroke setup.
 *
 * The canvas has already been prepared in view-local coordinates, so every
 * shape starts at `(0, 0)` and only uses `region->size` for the target bounds.
 *
 * Reading tip:
 * - Fill-only cases go straight to the canvas fill helper.
 * - Stroke cases optionally draw an inset fill first, then the outer stroke.
 * - Circle backgrounds use the region center as their origin, while rectangle
 *   variants use the full local width and height of the view.
 */
void egui_background_color_on_draw(egui_background_t *self, egui_canvas_t *canvas, egui_region_t *region, const void *param)
{
    EGUI_LOCAL_INIT(egui_background_color_t);
    const egui_background_color_param_t *color_param = param;
    EGUI_UNUSED(local);

    switch (color_param->type)
    {
    case EGUI_BACKGROUND_COLOR_TYPE_SOLID:
        // Solid rectangle backgrounds can either be pure fills or filled rectangles with an outline.
        if (color_param->stroke_width == 0)
        {
            egui_canvas_draw_rectangle_fill(canvas, 0, 0, region->size.width, region->size.height, color_param->color, color_param->alpha);
        }
        else
        {
            if (color_param->alpha != EGUI_ALPHA_0)
            {
                egui_canvas_draw_rectangle_fill(canvas, color_param->stroke_width / 2, color_param->stroke_width / 2,
                                                region->size.width - color_param->stroke_width, region->size.height - color_param->stroke_width,
                                                color_param->color, color_param->alpha);
            }
            egui_canvas_draw_rectangle(canvas, 0, 0, region->size.width, region->size.height, color_param->stroke_width, color_param->stroke_color,
                                       color_param->stroke_alpha);
        }
        break;
    case EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE:
        // Round-rectangle backgrounds reuse the same fill/stroke split while preserving one shared radius.
        if (color_param->stroke_width == 0)
        {
            egui_canvas_draw_round_rectangle_fill(canvas, 0, 0, region->size.width, region->size.height, color_param->shape.round_rectangle.radius,
                                                  color_param->color, color_param->alpha);
        }
        else
        {
            if (color_param->alpha != EGUI_ALPHA_0)
            {
                egui_canvas_draw_round_rectangle_fill(canvas, color_param->stroke_width / 2, color_param->stroke_width / 2,
                                                      region->size.width - color_param->stroke_width, region->size.height - color_param->stroke_width,
                                                      color_param->shape.round_rectangle.radius, color_param->color, color_param->alpha);
            }
            egui_canvas_draw_round_rectangle(canvas, 0, 0, region->size.width, region->size.height, color_param->shape.round_rectangle.radius,
                                             color_param->stroke_width, color_param->stroke_color, color_param->stroke_alpha);
        }
        break;
    case EGUI_BACKGROUND_COLOR_TYPE_ROUND_RECTANGLE_CORNERS:
        // Corner-rounded backgrounds allow each corner to have its own radius, so the inset fill adjusts every corner independently.
        if (color_param->stroke_width == 0)
        {
            egui_canvas_draw_round_rectangle_corners_fill(
                    canvas, 0, 0, region->size.width, region->size.height, color_param->shape.round_rectangle_corners.radius_left_top,
                    color_param->shape.round_rectangle_corners.radius_left_bottom, color_param->shape.round_rectangle_corners.radius_right_top,
                    color_param->shape.round_rectangle_corners.radius_right_bottom, color_param->color, color_param->alpha);
        }
        else
        {
            if (color_param->alpha != EGUI_ALPHA_0)
            {
                egui_canvas_draw_round_rectangle_corners_fill(canvas, color_param->stroke_width / 2, color_param->stroke_width / 2,
                                                              region->size.width - color_param->stroke_width, region->size.height - color_param->stroke_width,
                                                              color_param->shape.round_rectangle_corners.radius_left_top - color_param->stroke_width / 2,
                                                              color_param->shape.round_rectangle_corners.radius_left_bottom - color_param->stroke_width / 2,
                                                              color_param->shape.round_rectangle_corners.radius_right_top - color_param->stroke_width / 2,
                                                              color_param->shape.round_rectangle_corners.radius_right_bottom - color_param->stroke_width / 2,
                                                              color_param->color, color_param->alpha);
            }
            egui_canvas_draw_round_rectangle_corners(
                    canvas, 0, 0, region->size.width, region->size.height, color_param->shape.round_rectangle_corners.radius_left_top,
                    color_param->shape.round_rectangle_corners.radius_left_bottom, color_param->shape.round_rectangle_corners.radius_right_top,
                    color_param->shape.round_rectangle_corners.radius_right_bottom, color_param->stroke_width, color_param->stroke_color,
                    color_param->stroke_alpha);
        }
        break;
    case EGUI_BACKGROUND_COLOR_TYPE_CIRCLE:
        // Circle backgrounds are centered in the view region and can optionally reserve space for a stroke ring.
        if (color_param->stroke_width == 0)
        {
            egui_canvas_draw_circle_fill(canvas, (region->size.width >> 1), (region->size.height >> 1), color_param->shape.circle.radius, color_param->color,
                                         color_param->alpha);
        }
        else
        {
            if (color_param->alpha != EGUI_ALPHA_0)
            {
                egui_canvas_draw_circle_fill(canvas, (region->size.width >> 1), (region->size.height >> 1),
                                             color_param->shape.circle.radius - color_param->stroke_width / 2, color_param->color, color_param->alpha);
            }
            egui_canvas_draw_circle(canvas, (region->size.width >> 1), (region->size.height >> 1), color_param->shape.circle.radius, color_param->stroke_width,
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

/**
 * @brief Initialize a color background and install the color-specific API table.
 */
void egui_background_color_init(egui_background_t *self)
{
    egui_background_init(self);
    self->api = &egui_background_color_t_api_table;
}

/**
 * @brief Initialize a color background and immediately bind one parameter table.
 */
void egui_background_color_init_with_params(egui_background_t *self, const egui_background_params_t *params)
{
    egui_background_color_init(self);
    egui_background_set_params(self, params);
}
