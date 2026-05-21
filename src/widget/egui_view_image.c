#include <assert.h>

#include "egui_view_image.h"
#include "core/egui_core.h"
#include "image/egui_image.h"

/**
 * @file egui_view_image.c
 * @brief Small image view that forwards drawing to the canvas helpers.
 *
 * Learning path:
 * - the widget stores only draw
 * mode, resource pointer, and optional tint,
 * - draw decides between original-size and stretched rendering,
 * - tinting is enabled only when
 * `image_color_alpha` is non-zero.
 */

/**
 * @brief Draw the configured image resource inside the current work region.
 */
#if EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
static egui_dim_t egui_view_image_scale_dimension(egui_dim_t value, int16_t scale_q8)
{
    int32_t scaled;

    if (value <= 0 || scale_q8 <= 0)
    {
        return 0;
    }

    scaled = ((int32_t)value * scale_q8 + 128) >> 8;
    if (scaled <= 0)
    {
        return 1;
    }
    if (scaled > 32767)
    {
        return 32767;
    }
    return (egui_dim_t)scaled;
}
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
static void egui_view_image_draw_transform(egui_view_image_t *local, egui_canvas_t *canvas, const egui_region_t *region)
{
    egui_dim_t source_w;
    egui_dim_t source_h;
    egui_dim_t draw_w;
    egui_dim_t draw_h;
    egui_dim_t center_x;
    egui_dim_t center_y;

    if (!egui_image_get_size(local->image, &source_w, &source_h))
    {
        return;
    }

    draw_w = egui_view_image_scale_dimension(source_w, local->scale_q8);
    draw_h = egui_view_image_scale_dimension(source_h, local->scale_q8);
    if (draw_w <= 0 || draw_h <= 0)
    {
        return;
    }

    center_x = region->location.x + (draw_w >> 1);
    center_y = region->location.y + (draw_h >> 1);
    egui_canvas_draw_image_transform(canvas, local->image, center_x, center_y, local->angle_deg, local->scale_q8);
}
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

#if EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE && !EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
static void egui_view_image_draw_scale_lite(egui_view_image_t *local, egui_canvas_t *canvas, const egui_region_t *region)
{
    egui_dim_t base_w = region->size.width;
    egui_dim_t base_h = region->size.height;
    egui_dim_t draw_w;
    egui_dim_t draw_h;

    if (local->image_type == EGUI_VIEW_IMAGE_TYPE_NORMAL)
    {
        if (!egui_image_get_size(local->image, &base_w, &base_h))
        {
            return;
        }
    }

    draw_w = egui_view_image_scale_dimension(base_w, local->scale_q8);
    draw_h = egui_view_image_scale_dimension(base_h, local->scale_q8);
    if (draw_w <= 0 || draw_h <= 0)
    {
        return;
    }

    if (local->image_color_alpha != 0)
    {
        egui_canvas_draw_image_resize_color(canvas, local->image, region->location.x, region->location.y, draw_w, draw_h, local->image_color,
                                            local->image_color_alpha);
    }
    else
    {
        egui_canvas_draw_image_resize(canvas, local->image, region->location.x, region->location.y, draw_w, draw_h);
    }
}
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE && !EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

void egui_view_image_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_image_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    if (local->image == NULL)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
    if (local->angle_deg != 0 || local->scale_q8 != 256)
    {
        egui_view_image_draw_transform(local, canvas, &region);
        return;
    }
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

#if EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE && !EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
    if (local->scale_q8 != 256)
    {
        egui_view_image_draw_scale_lite(local, canvas, &region);
        return;
    }
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE && !EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

    if (local->image_color_alpha != 0)
    {
        // alpha-only color rendering path
        if (local->image_type == EGUI_VIEW_IMAGE_TYPE_NORMAL)
        {
            egui_canvas_draw_image_color(canvas, local->image, region.location.x, region.location.y, local->image_color, local->image_color_alpha);
        }
        else
        {
            egui_canvas_draw_image_resize_color(canvas, local->image, region.location.x, region.location.y, region.size.width, region.size.height,
                                                local->image_color, local->image_color_alpha);
        }
    }
    else if (local->image_type == EGUI_VIEW_IMAGE_TYPE_NORMAL)
    {
        egui_canvas_draw_image(canvas, local->image, region.location.x, region.location.y);
    }
    else
    {
        egui_canvas_draw_image_resize(canvas, local->image, region.location.x, region.location.y, region.size.width, region.size.height);
    }
}

/**
 * @brief Switch between source-size drawing and stretched drawing.
 */
void egui_view_image_set_image_type(egui_view_t *self, int image_type)
{
    EGUI_LOCAL_INIT(egui_view_image_t);
    if (local->image_type == image_type)
    {
        return;
    }
    local->image_type = image_type;
    egui_view_invalidate(self);
}

int egui_view_image_get_image_type(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_image_t);
    return local->image_type;
}

/**
 * @brief Replace the borrowed image resource pointer.
 */
void egui_view_image_set_image(egui_view_t *self, egui_image_t *image)
{
    EGUI_LOCAL_INIT(egui_view_image_t);
    if (local->image == image)
    {
        return;
    }
    local->image = image;
    egui_view_invalidate(self);
}

const egui_image_t *egui_view_image_get_image(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_image_t);
    return local->image;
}

/**
 * @brief Configure the optional image tint path.
 *
 * Passing alpha `0` disables tinting and falls back to the normal draw helper.
 */
void egui_view_image_set_image_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_view_image_t);
    if (local->image_color.full == color.full && local->image_color_alpha == alpha)
    {
        return;
    }
    local->image_color = color;
    local->image_color_alpha = alpha;
    egui_view_invalidate(self);
}

egui_color_t egui_view_image_get_image_color(egui_view_t *self)
{
    egui_color_t zero;

    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_image_t);
    return local->image_color;
}

egui_alpha_t egui_view_image_get_image_alpha(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_image_t);
    return local->image_color_alpha;
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_image_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_image_on_draw, // changed
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

/**
 * @brief Initialize the image widget with no resource and normal draw mode.
 */
void egui_view_image_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_image_t);

    // call super init.
    egui_view_init(self, core);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_image_t);

    // init local data.
    egui_view_set_view_name(self, "egui_view_image");

    local->image_type = EGUI_VIEW_IMAGE_TYPE_NORMAL;
    local->image = NULL;
    local->image_color.full = 0;
    local->image_color_alpha = 0;
#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
    local->angle_deg = 0;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
    local->scale_q8 = 256;
#endif
}

/**
 * @brief Apply region and initial image resource from one parameter block.
 */
void egui_view_image_apply_params(egui_view_t *self, const egui_view_image_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_image_t);

    self->region = params->region;

    local->image = params->image;

    egui_view_invalidate(self);
}

/**
 * @brief Convenience initializer that chains image init and params.
 */
void egui_view_image_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_image_params_t *params)
{
    egui_view_image_init(self, core);
    egui_view_image_apply_params(self, params);
}

#if EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
/**
 * @brief Set the scale factor in Q8 format (256 = 1x). Triggers a full redraw.
 */
void egui_view_image_set_scale(egui_view_t *self, int16_t scale_q8)
{
    EGUI_LOCAL_INIT(egui_view_image_t);
    if (local->scale_q8 == scale_q8)
    {
        return;
    }
    local->scale_q8 = scale_q8;
    egui_view_invalidate(self);
}

/**
 * @brief Return the current scale factor.
 */
int16_t egui_view_image_get_scale(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_image_t);
    return local->scale_q8;
}
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
/**
 * @brief Set the rotation angle in degrees. Triggers a full redraw.
 */
void egui_view_image_set_angle(egui_view_t *self, int16_t angle_deg)
{
    EGUI_LOCAL_INIT(egui_view_image_t);
    if (local->angle_deg == angle_deg)
    {
        return;
    }
    local->angle_deg = angle_deg;
    egui_view_invalidate(self);
}

/**
 * @brief Return the current rotation angle.
 */
int16_t egui_view_image_get_angle(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_image_t);
    return local->angle_deg;
}

/**
 * @brief Convenience setter that updates both angle and scale in one redraw.
 */
void egui_view_image_set_transform(egui_view_t *self, int16_t angle_deg, int16_t scale_q8)
{
    EGUI_LOCAL_INIT(egui_view_image_t);
    if (local->angle_deg == angle_deg && local->scale_q8 == scale_q8)
    {
        return;
    }
    local->angle_deg = angle_deg;
    local->scale_q8 = scale_q8;
    egui_view_invalidate(self);
}
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */
