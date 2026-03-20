#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_mask_image.h"
#include "image/egui_image_std.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

static void egui_mask_image_update_cache(egui_mask_image_t *local)
{
    egui_mask_t *self = &local->base;
    if (local->img == NULL)
    {
        local->width_radio = 0;
        local->height_radio = 0;
        return;
    }
    egui_image_std_info_t *image = (egui_image_std_info_t *)local->img->res;
    egui_dim_t w = self->region.size.width;
    egui_dim_t h = self->region.size.height;
    if (w > 0 && h > 0)
    {
        local->width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(w));
        local->height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(h));
    }
    else
    {
        local->width_radio = 0;
        local->height_radio = 0;
    }
}

void egui_mask_image_set_image(egui_mask_t *self, egui_image_t *img)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    local->img = img;
    egui_mask_image_update_cache(local);
}

void egui_mask_image_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    if (local->img == NULL)
    {
        return;
    }
    // Use cached ratios to avoid per-pixel EGUI_FLOAT_DIV
    egui_dim_t src_x = (egui_dim_t)EGUI_FLOAT_MULT(x, local->width_radio);
    egui_dim_t src_y = (egui_dim_t)EGUI_FLOAT_MULT(y, local->height_radio);

    egui_color_t orign_color;
    egui_alpha_t orign_alpha;
    if (local->img->api->get_point(local->img, src_x, src_y, &orign_color, &orign_alpha))
    {
        *alpha = orign_alpha;
    }
    else
    {
        *alpha = 0;
    }
}

static int egui_mask_image_get_row_range(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start, egui_dim_t *x_end)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    if (local->img == NULL)
    {
        return EGUI_MASK_ROW_OUTSIDE;
    }

    egui_image_std_info_t *image = (egui_image_std_info_t *)local->img->res;
    egui_dim_t mask_w = self->region.size.width;
    egui_dim_t mask_h = self->region.size.height;

    if (y < 0 || y >= mask_h)
    {
        return EGUI_MASK_ROW_OUTSIDE;
    }

    egui_dim_t src_y = (egui_dim_t)EGUI_FLOAT_MULT(y, local->height_radio);
    if (src_y >= image->height)
    {
        return EGUI_MASK_ROW_OUTSIDE;
    }

    // No alpha channel → entirely opaque image
    if (image->alpha_buf == NULL)
    {
        *x_start = EGUI_MAX(x_min, (egui_dim_t)0);
        *x_end = EGUI_MIN(x_max, mask_w);
        if (*x_start >= *x_end)
        {
            return EGUI_MASK_ROW_OUTSIDE;
        }
        return (*x_start <= x_min && *x_end >= x_max) ? EGUI_MASK_ROW_INSIDE : EGUI_MASK_ROW_PARTIAL;
    }

    // Fast path for ALPHA_TYPE_8: direct alpha array scan on source row
    if (image->data_type == EGUI_IMAGE_DATA_TYPE_RGB565 && image->alpha_type == EGUI_IMAGE_ALPHA_TYPE_8)
    {
        const uint8_t *alpha_row = (const uint8_t *)image->alpha_buf + (uint32_t)src_y * image->width;
        egui_dim_t src_w = image->width;

        // Single pass: find longest contiguous opaque run and check for any visible pixel
        egui_dim_t run_start = -1;
        egui_dim_t best_start = -1;
        egui_dim_t best_len = 0;
        int any_visible = 0;

        for (egui_dim_t i = 0; i < src_w; i++)
        {
            uint8_t a = alpha_row[i];
            if (a > 0)
            {
                any_visible = 1;
            }

            if (a == EGUI_ALPHA_100)
            {
                if (run_start < 0)
                {
                    run_start = i;
                }
            }
            else
            {
                if (run_start >= 0)
                {
                    egui_dim_t len = i - run_start;
                    if (len > best_len)
                    {
                        best_start = run_start;
                        best_len = len;
                    }
                    run_start = -1;
                }
            }
        }
        // Handle final run
        if (run_start >= 0)
        {
            egui_dim_t len = src_w - run_start;
            if (len > best_len)
            {
                best_start = run_start;
                best_len = len;
            }
        }

        if (!any_visible)
        {
            return EGUI_MASK_ROW_OUTSIDE;
        }

        if (best_len == 0)
        {
            // Visible but no fully opaque pixels
            *x_start = x_min;
            *x_end = x_min;
            return EGUI_MASK_ROW_PARTIAL;
        }

        // Map source opaque bounds to display coordinates (conservative)
        egui_dim_t best_end_src = best_start + best_len; // exclusive in source
        egui_dim_t disp_start = (egui_dim_t)(((int32_t)best_start * mask_w + src_w - 1) / src_w);   // ceil
        egui_dim_t disp_end = (egui_dim_t)(((int32_t)(best_end_src - 1) * mask_w) / src_w) + 1;     // floor + 1

        *x_start = EGUI_MAX(disp_start, x_min);
        *x_end = EGUI_MIN(disp_end, x_max);

        if (*x_start >= *x_end)
        {
            *x_start = x_min;
            *x_end = x_min;
            return EGUI_MASK_ROW_PARTIAL;
        }

        if (*x_start <= x_min && *x_end >= x_max)
        {
            return EGUI_MASK_ROW_INSIDE;
        }

        return EGUI_MASK_ROW_PARTIAL;
    }

    // Other formats: conservative fallback
    *x_start = x_min;
    *x_end = x_min;
    return EGUI_MASK_ROW_PARTIAL;
}

static int egui_mask_image_get_row_visible_range(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start, egui_dim_t *x_end)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    if (local->img == NULL)
    {
        return 0;
    }

    egui_image_std_info_t *image = (egui_image_std_info_t *)local->img->res;
    egui_dim_t mask_w = self->region.size.width;
    egui_dim_t mask_h = self->region.size.height;

    if (y < 0 || y >= mask_h)
    {
        return 0;
    }

    egui_dim_t src_y = (egui_dim_t)EGUI_FLOAT_MULT(y, local->height_radio);
    if (src_y >= image->height)
    {
        return 0;
    }

    // No alpha channel → full row visible
    if (image->alpha_buf == NULL)
    {
        *x_start = EGUI_MAX(x_min, (egui_dim_t)0);
        *x_end = EGUI_MIN(x_max, mask_w);
        return (*x_start < *x_end);
    }

    // Fast path for ALPHA_TYPE_8
    if (image->data_type == EGUI_IMAGE_DATA_TYPE_RGB565 && image->alpha_type == EGUI_IMAGE_ALPHA_TYPE_8)
    {
        const uint8_t *alpha_row = (const uint8_t *)image->alpha_buf + (uint32_t)src_y * image->width;
        egui_dim_t src_w = image->width;

        // Scan from left for first visible
        egui_dim_t first_vis_src = -1;
        for (egui_dim_t i = 0; i < src_w; i++)
        {
            if (alpha_row[i] > 0)
            {
                first_vis_src = i;
                break;
            }
        }
        if (first_vis_src < 0)
        {
            return 0;
        }

        // Scan from right for last visible
        egui_dim_t last_vis_src = first_vis_src;
        for (egui_dim_t i = src_w - 1; i > first_vis_src; i--)
        {
            if (alpha_row[i] > 0)
            {
                last_vis_src = i;
                break;
            }
        }

        // Map to display coordinates (conservative: expand to include all visible)
        egui_dim_t disp_start = (egui_dim_t)(((int32_t)first_vis_src * mask_w) / src_w);             // floor
        egui_dim_t disp_end = (egui_dim_t)(((int32_t)last_vis_src * mask_w + src_w - 1) / src_w) + 1; // ceil + 1

        *x_start = EGUI_MAX(disp_start, x_min);
        *x_end = EGUI_MIN(disp_end, x_max);
        return (*x_start < *x_end);
    }

    // Fallback: assume full row visible
    *x_start = EGUI_MAX(x_min, (egui_dim_t)0);
    *x_end = EGUI_MIN(x_max, mask_w);
    return (*x_start < *x_end);
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_mask_api_t egui_mask_image_t_api_table = {
        .mask_point = egui_mask_image_mask_point,
        .mask_get_row_range = egui_mask_image_get_row_range,
        .mask_get_row_visible_range = egui_mask_image_get_row_visible_range,
        .mask_blend_row_color = NULL,
};

void egui_mask_image_init(egui_mask_t *self)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    // call super init.
    egui_mask_init(self);
    // update api.
    self->api = &egui_mask_image_t_api_table;

    // init local data.
    local->img = NULL;
    local->width_radio = 0;
    local->height_radio = 0;
}
