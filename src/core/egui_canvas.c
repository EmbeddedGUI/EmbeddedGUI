#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_canvas.h"
#include "font/egui_font.h"
#include "image/egui_image.h"
#include "image/egui_image_std.h"
#include "egui_api.h"

#define TEST_CANVAS_TEST_WOKR 0

egui_canvas_t canvas_data;

enum
{
    EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP,
    EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM,
    EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP,
    EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM,
};

extern const egui_circle_info_t egui_res_circle_info_arr[];

static const egui_float_t tan_val_list[91] = {
        EGUI_FLOAT_VALUE(0.000000f),  EGUI_FLOAT_VALUE(0.017455f),  EGUI_FLOAT_VALUE(0.034921f),  EGUI_FLOAT_VALUE(0.052408f),  EGUI_FLOAT_VALUE(0.069927f),
        EGUI_FLOAT_VALUE(0.087489f),  EGUI_FLOAT_VALUE(0.105104f),  EGUI_FLOAT_VALUE(0.122785f),  EGUI_FLOAT_VALUE(0.140541f),  EGUI_FLOAT_VALUE(0.158384f),
        EGUI_FLOAT_VALUE(0.176327f),  EGUI_FLOAT_VALUE(0.194380f),  EGUI_FLOAT_VALUE(0.212557f),  EGUI_FLOAT_VALUE(0.230868f),  EGUI_FLOAT_VALUE(0.249328f),
        EGUI_FLOAT_VALUE(0.267949f),  EGUI_FLOAT_VALUE(0.286745f),  EGUI_FLOAT_VALUE(0.305731f),  EGUI_FLOAT_VALUE(0.324920f),  EGUI_FLOAT_VALUE(0.344328f),
        EGUI_FLOAT_VALUE(0.363970f),  EGUI_FLOAT_VALUE(0.383864f),  EGUI_FLOAT_VALUE(0.404026f),  EGUI_FLOAT_VALUE(0.424475f),  EGUI_FLOAT_VALUE(0.445229f),
        EGUI_FLOAT_VALUE(0.466308f),  EGUI_FLOAT_VALUE(0.487733f),  EGUI_FLOAT_VALUE(0.509525f),  EGUI_FLOAT_VALUE(0.531709f),  EGUI_FLOAT_VALUE(0.554309f),
        EGUI_FLOAT_VALUE(0.577350f),  EGUI_FLOAT_VALUE(0.600861f),  EGUI_FLOAT_VALUE(0.624869f),  EGUI_FLOAT_VALUE(0.649408f),  EGUI_FLOAT_VALUE(0.674509f),
        EGUI_FLOAT_VALUE(0.700208f),  EGUI_FLOAT_VALUE(0.726543f),  EGUI_FLOAT_VALUE(0.753554f),  EGUI_FLOAT_VALUE(0.781286f),  EGUI_FLOAT_VALUE(0.809784f),
        EGUI_FLOAT_VALUE(0.839100f),  EGUI_FLOAT_VALUE(0.869287f),  EGUI_FLOAT_VALUE(0.900404f),  EGUI_FLOAT_VALUE(0.932515f),  EGUI_FLOAT_VALUE(0.965689f),
        EGUI_FLOAT_VALUE(1.000000f),  EGUI_FLOAT_VALUE(1.035530f),  EGUI_FLOAT_VALUE(1.072369f),  EGUI_FLOAT_VALUE(1.110613f),  EGUI_FLOAT_VALUE(1.150368f),
        EGUI_FLOAT_VALUE(1.191754f),  EGUI_FLOAT_VALUE(1.234897f),  EGUI_FLOAT_VALUE(1.279942f),  EGUI_FLOAT_VALUE(1.327045f),  EGUI_FLOAT_VALUE(1.376382f),
        EGUI_FLOAT_VALUE(1.428148f),  EGUI_FLOAT_VALUE(1.482561f),  EGUI_FLOAT_VALUE(1.539865f),  EGUI_FLOAT_VALUE(1.600335f),  EGUI_FLOAT_VALUE(1.664279f),
        EGUI_FLOAT_VALUE(1.732051f),  EGUI_FLOAT_VALUE(1.804048f),  EGUI_FLOAT_VALUE(1.880726f),  EGUI_FLOAT_VALUE(1.962611f),  EGUI_FLOAT_VALUE(2.050304f),
        EGUI_FLOAT_VALUE(2.144507f),  EGUI_FLOAT_VALUE(2.246037f),  EGUI_FLOAT_VALUE(2.355852f),  EGUI_FLOAT_VALUE(2.475087f),  EGUI_FLOAT_VALUE(2.605089f),
        EGUI_FLOAT_VALUE(2.747477f),  EGUI_FLOAT_VALUE(2.904211f),  EGUI_FLOAT_VALUE(3.077684f),  EGUI_FLOAT_VALUE(3.270853f),  EGUI_FLOAT_VALUE(3.487414f),
        EGUI_FLOAT_VALUE(3.732051f),  EGUI_FLOAT_VALUE(4.010781f),  EGUI_FLOAT_VALUE(4.331476f),  EGUI_FLOAT_VALUE(4.704630f),  EGUI_FLOAT_VALUE(5.144554f),
        EGUI_FLOAT_VALUE(5.671282f),  EGUI_FLOAT_VALUE(6.313752f),  EGUI_FLOAT_VALUE(7.115370f),  EGUI_FLOAT_VALUE(8.144346f),  EGUI_FLOAT_VALUE(9.514364f),
        EGUI_FLOAT_VALUE(11.430052f), EGUI_FLOAT_VALUE(14.300666f), EGUI_FLOAT_VALUE(19.081137f), EGUI_FLOAT_VALUE(28.636253f), EGUI_FLOAT_VALUE(57.289962f),
        EGUI_FLOAT_VALUE(9999.0f),
};

static const egui_float_t ctan_val_list[91] = {
        EGUI_FLOAT_VALUE(9999.0f),    EGUI_FLOAT_VALUE(57.289962f), EGUI_FLOAT_VALUE(28.636253f), EGUI_FLOAT_VALUE(19.081137f), EGUI_FLOAT_VALUE(14.300666f),
        EGUI_FLOAT_VALUE(11.430052f), EGUI_FLOAT_VALUE(9.514364f),  EGUI_FLOAT_VALUE(8.144346f),  EGUI_FLOAT_VALUE(7.115370f),  EGUI_FLOAT_VALUE(6.313752f),
        EGUI_FLOAT_VALUE(5.671282f),  EGUI_FLOAT_VALUE(5.144554f),  EGUI_FLOAT_VALUE(4.704630f),  EGUI_FLOAT_VALUE(4.331476f),  EGUI_FLOAT_VALUE(4.010781f),
        EGUI_FLOAT_VALUE(3.732051f),  EGUI_FLOAT_VALUE(3.487414f),  EGUI_FLOAT_VALUE(3.270853f),  EGUI_FLOAT_VALUE(3.077684f),  EGUI_FLOAT_VALUE(2.904211f),
        EGUI_FLOAT_VALUE(2.747477f),  EGUI_FLOAT_VALUE(2.605089f),  EGUI_FLOAT_VALUE(2.475087f),  EGUI_FLOAT_VALUE(2.355852f),  EGUI_FLOAT_VALUE(2.246037f),
        EGUI_FLOAT_VALUE(2.144507f),  EGUI_FLOAT_VALUE(2.050304f),  EGUI_FLOAT_VALUE(1.962611f),  EGUI_FLOAT_VALUE(1.880726f),  EGUI_FLOAT_VALUE(1.804048f),
        EGUI_FLOAT_VALUE(1.732051f),  EGUI_FLOAT_VALUE(1.664279f),  EGUI_FLOAT_VALUE(1.600335f),  EGUI_FLOAT_VALUE(1.539865f),  EGUI_FLOAT_VALUE(1.482561f),
        EGUI_FLOAT_VALUE(1.428148f),  EGUI_FLOAT_VALUE(1.376382f),  EGUI_FLOAT_VALUE(1.327045f),  EGUI_FLOAT_VALUE(1.279942f),  EGUI_FLOAT_VALUE(1.234897f),
        EGUI_FLOAT_VALUE(1.191754f),  EGUI_FLOAT_VALUE(1.150368f),  EGUI_FLOAT_VALUE(1.110613f),  EGUI_FLOAT_VALUE(1.072369f),  EGUI_FLOAT_VALUE(1.035530f),
        EGUI_FLOAT_VALUE(1.000000f),  EGUI_FLOAT_VALUE(0.965689f),  EGUI_FLOAT_VALUE(0.932515f),  EGUI_FLOAT_VALUE(0.900404f),  EGUI_FLOAT_VALUE(0.869287f),
        EGUI_FLOAT_VALUE(0.839100f),  EGUI_FLOAT_VALUE(0.809784f),  EGUI_FLOAT_VALUE(0.781286f),  EGUI_FLOAT_VALUE(0.753554f),  EGUI_FLOAT_VALUE(0.726543f),
        EGUI_FLOAT_VALUE(0.700208f),  EGUI_FLOAT_VALUE(0.674509f),  EGUI_FLOAT_VALUE(0.649408f),  EGUI_FLOAT_VALUE(0.624869f),  EGUI_FLOAT_VALUE(0.600861f),
        EGUI_FLOAT_VALUE(0.577350f),  EGUI_FLOAT_VALUE(0.554309f),  EGUI_FLOAT_VALUE(0.531709f),  EGUI_FLOAT_VALUE(0.509525f),  EGUI_FLOAT_VALUE(0.487733f),
        EGUI_FLOAT_VALUE(0.466308f),  EGUI_FLOAT_VALUE(0.445229f),  EGUI_FLOAT_VALUE(0.424475f),  EGUI_FLOAT_VALUE(0.404026f),  EGUI_FLOAT_VALUE(0.383864f),
        EGUI_FLOAT_VALUE(0.363970f),  EGUI_FLOAT_VALUE(0.344328f),  EGUI_FLOAT_VALUE(0.324920f),  EGUI_FLOAT_VALUE(0.305731f),  EGUI_FLOAT_VALUE(0.286745f),
        EGUI_FLOAT_VALUE(0.267949f),  EGUI_FLOAT_VALUE(0.249328f),  EGUI_FLOAT_VALUE(0.230868f),  EGUI_FLOAT_VALUE(0.212557f),  EGUI_FLOAT_VALUE(0.194380f),
        EGUI_FLOAT_VALUE(0.176327f),  EGUI_FLOAT_VALUE(0.158384f),  EGUI_FLOAT_VALUE(0.140541f),  EGUI_FLOAT_VALUE(0.122785f),  EGUI_FLOAT_VALUE(0.105104f),
        EGUI_FLOAT_VALUE(0.087489f),  EGUI_FLOAT_VALUE(0.069927f),  EGUI_FLOAT_VALUE(0.052408f),  EGUI_FLOAT_VALUE(0.034921f),  EGUI_FLOAT_VALUE(0.017455f),
        EGUI_FLOAT_VALUE(0.0f),
};

static const egui_float_t cos_val_list[91] = {
        EGUI_FLOAT_VALUE(1.000000f), EGUI_FLOAT_VALUE(0.999848f), EGUI_FLOAT_VALUE(0.999391f), EGUI_FLOAT_VALUE(0.998630f), EGUI_FLOAT_VALUE(0.997564f),
        EGUI_FLOAT_VALUE(0.996195f), EGUI_FLOAT_VALUE(0.994522f), EGUI_FLOAT_VALUE(0.992546f), EGUI_FLOAT_VALUE(0.990268f), EGUI_FLOAT_VALUE(0.987688f),
        EGUI_FLOAT_VALUE(0.984808f), EGUI_FLOAT_VALUE(0.981627f), EGUI_FLOAT_VALUE(0.978148f), EGUI_FLOAT_VALUE(0.974370f), EGUI_FLOAT_VALUE(0.970296f),
        EGUI_FLOAT_VALUE(0.965926f), EGUI_FLOAT_VALUE(0.961262f), EGUI_FLOAT_VALUE(0.956305f), EGUI_FLOAT_VALUE(0.951057f), EGUI_FLOAT_VALUE(0.945519f),
        EGUI_FLOAT_VALUE(0.939693f), EGUI_FLOAT_VALUE(0.933580f), EGUI_FLOAT_VALUE(0.927184f), EGUI_FLOAT_VALUE(0.920505f), EGUI_FLOAT_VALUE(0.913545f),
        EGUI_FLOAT_VALUE(0.906308f), EGUI_FLOAT_VALUE(0.898794f), EGUI_FLOAT_VALUE(0.891007f), EGUI_FLOAT_VALUE(0.882948f), EGUI_FLOAT_VALUE(0.874620f),
        EGUI_FLOAT_VALUE(0.866025f), EGUI_FLOAT_VALUE(0.857167f), EGUI_FLOAT_VALUE(0.848048f), EGUI_FLOAT_VALUE(0.838671f), EGUI_FLOAT_VALUE(0.829038f),
        EGUI_FLOAT_VALUE(0.819152f), EGUI_FLOAT_VALUE(0.809017f), EGUI_FLOAT_VALUE(0.798636f), EGUI_FLOAT_VALUE(0.788011f), EGUI_FLOAT_VALUE(0.777146f),
        EGUI_FLOAT_VALUE(0.766044f), EGUI_FLOAT_VALUE(0.754710f), EGUI_FLOAT_VALUE(0.743145f), EGUI_FLOAT_VALUE(0.731354f), EGUI_FLOAT_VALUE(0.719340f),
        EGUI_FLOAT_VALUE(0.707107f), EGUI_FLOAT_VALUE(0.694658f), EGUI_FLOAT_VALUE(0.681998f), EGUI_FLOAT_VALUE(0.669131f), EGUI_FLOAT_VALUE(0.656059f),
        EGUI_FLOAT_VALUE(0.642788f), EGUI_FLOAT_VALUE(0.629320f), EGUI_FLOAT_VALUE(0.615661f), EGUI_FLOAT_VALUE(0.601815f), EGUI_FLOAT_VALUE(0.587785f),
        EGUI_FLOAT_VALUE(0.573576f), EGUI_FLOAT_VALUE(0.559193f), EGUI_FLOAT_VALUE(0.544639f), EGUI_FLOAT_VALUE(0.529919f), EGUI_FLOAT_VALUE(0.515038f),
        EGUI_FLOAT_VALUE(0.500000f), EGUI_FLOAT_VALUE(0.484810f), EGUI_FLOAT_VALUE(0.469472f), EGUI_FLOAT_VALUE(0.453990f), EGUI_FLOAT_VALUE(0.438371f),
        EGUI_FLOAT_VALUE(0.422618f), EGUI_FLOAT_VALUE(0.406737f), EGUI_FLOAT_VALUE(0.390731f), EGUI_FLOAT_VALUE(0.374607f), EGUI_FLOAT_VALUE(0.358368f),
        EGUI_FLOAT_VALUE(0.342020f), EGUI_FLOAT_VALUE(0.325568f), EGUI_FLOAT_VALUE(0.309017f), EGUI_FLOAT_VALUE(0.292372f), EGUI_FLOAT_VALUE(0.275637f),
        EGUI_FLOAT_VALUE(0.258819f), EGUI_FLOAT_VALUE(0.241922f), EGUI_FLOAT_VALUE(0.224951f), EGUI_FLOAT_VALUE(0.207912f), EGUI_FLOAT_VALUE(0.190809f),
        EGUI_FLOAT_VALUE(0.173648f), EGUI_FLOAT_VALUE(0.156434f), EGUI_FLOAT_VALUE(0.139173f), EGUI_FLOAT_VALUE(0.121869f), EGUI_FLOAT_VALUE(0.104528f),
        EGUI_FLOAT_VALUE(0.087156f), EGUI_FLOAT_VALUE(0.069756f), EGUI_FLOAT_VALUE(0.052336f), EGUI_FLOAT_VALUE(0.034899f), EGUI_FLOAT_VALUE(0.017452f),
        EGUI_FLOAT_VALUE(0.000000f),
};

void egui_canvas_draw_point(egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    if (self->mask != NULL)
    {
        self->mask->api->mask_point(self->mask, x, y, &color, &alpha);
    }

    // mix alpha
    alpha = egui_color_alpha_mix(self->alpha, alpha);
    // if color is fully transparent, do not draw anything.
    if (alpha == 0)
    {
        return;
    }

    if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
        egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;

        egui_color_t *back_color = (egui_color_t *)&self->pfb[pos_y * self->pfb_region.size.width + pos_x];
#if TEST_CANVAS_TEST_WOKR
        back_color->full = alpha; // For test
#else
        if (alpha == EGUI_ALPHA_100)
        {
            *back_color = color;
        }
        else
        {
            egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
        }
#endif
    }
}

const egui_circle_info_t *egui_canvas_get_circle_item(egui_dim_t r)
{
    egui_canvas_t *self = &canvas_data;

    if (r < EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE)
    {
        return &egui_res_circle_info_arr[r];
    }

    for (int i = 0; i < self->res_circle_info_count_spec; i++)
    {
        if (self->res_circle_info_spec_arr[i].radius == r)
        {
            return &self->res_circle_info_spec_arr[i];
        }
    }

    return NULL;
}

void egui_canvas_draw_circle_corner_fill(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int type, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    egui_dim_t len;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    egui_region_t region;
    egui_region_t region_intersect;
    switch (type)
    {
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
        egui_region_init(&region, center_x - radius, center_y - radius, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
        egui_region_init(&region, center_x - radius, center_y + 1, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
        egui_region_init(&region, center_x + 1, center_y - radius, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
        egui_region_init(&region, center_x + 1, center_y + 1, radius, radius);
        break;
    }
    egui_region_intersect(&region, &self->base_view_work_region, &region_intersect);
    if (egui_region_is_empty(&region_intersect))
    {
        return;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return;
    }

    // Compute visible row/col ranges from intersection (same as circle_corner outline)
    egui_dim_t diff_x = region_intersect.location.x - region.location.x;
    egui_dim_t diff_y = region_intersect.location.y - region.location.y;
    egui_dim_t row_start = 0;
    egui_dim_t row_end = 0;
    egui_dim_t col_start = 0;
    egui_dim_t col_end = 0;

    switch (type)
    {
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
        row_start = diff_y;
        row_end = row_start + region_intersect.size.height;
        col_start = diff_x;
        col_end = col_start + region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
        row_end = radius - diff_y;
        row_start = row_end - region_intersect.size.height;
        col_start = diff_x;
        col_end = col_start + region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
        row_start = diff_y;
        row_end = row_start + region_intersect.size.height;
        col_end = radius - diff_x;
        col_start = col_end - region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
        row_end = radius - diff_y;
        row_start = row_end - region_intersect.size.height;
        col_end = radius - diff_x;
        col_start = col_end - region_intersect.size.width;
        break;
    }

    // Effective iteration range: union of rows producing visible primary [row_start, row_end)
    // and rows producing visible mirror output [col_start, col_end)
    egui_dim_t iter_start = EGUI_MAX(EGUI_MIN(row_start, col_start), 0);
    egui_dim_t iter_end = EGUI_MIN(EGUI_MAX(row_end, col_end), (egui_dim_t)info->item_count);

    for (row_index = iter_start; row_index < iter_end; row_index++)
    {
        const egui_circle_item_t *ptr = &((const egui_circle_item_t *)info->items)[row_index];
        uint16_t start_offset = ptr->start_offset;
        uint16_t valid_count = ptr->valid_count;
        uint16_t data_value_offset = ptr->data_offset;

        sel_y = radius - row_index;

        // Primary row visible: row_index acts as y-offset in corner coords
        int primary_visible = (row_index >= row_start && row_index < row_end);
        // Mirror column visible: row_index acts as x-offset in corner coords (mirrored)
        int mirror_visible = (row_index >= col_start && row_index < col_end);

        // Edge pixels
        for (int i = 0; i < valid_count; i++)
        {
            col_index = start_offset + i;
            sel_x = radius - col_index;
            mix_alpha = info->data[data_value_offset + i];

            mix_alpha = egui_color_alpha_mix(alpha, mix_alpha);

            // Primary pixel: visible when row_index in Y-range AND col_index in X-range
            if (primary_visible && col_index >= col_start && col_index < col_end)
            {
                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (-sel_y), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (sel_y), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (-sel_y), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                    egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (sel_y), color, mix_alpha);
                    break;
                }
            }
            // skip the diagonal write twice
            if (sel_x == sel_y)
            {
                continue;
            }
            // Mirror pixel: visible when row_index in X-range AND col_index in Y-range
            if (mirror_visible && col_index >= row_start && col_index < row_end)
            {
                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    egui_canvas_draw_point_limit(center_x + (-sel_y), center_y + (-sel_x), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    egui_canvas_draw_point_limit(center_x + (-sel_y), center_y + (sel_x), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    egui_canvas_draw_point_limit(center_x + (sel_y), center_y + (-sel_x), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                    egui_canvas_draw_point_limit(center_x + (sel_y), center_y + (sel_x), color, mix_alpha);
                    break;
                }
            }
        }

        // write the reserve value to reserve the space, here can use write line or write column
        egui_dim_t offset = start_offset + valid_count;
        len = radius - offset;
        if (len > 0)
        {
            // hline: at primary y position, visible when row_index in Y-range
            if (primary_visible)
            {
                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    egui_canvas_draw_hline(center_x + (-(radius - offset)), center_y + (-sel_y), len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    egui_canvas_draw_hline(center_x + (-(radius - offset)), center_y + (sel_y), len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    egui_canvas_draw_hline(center_x + 1, center_y + (-sel_y), len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                    egui_canvas_draw_hline(center_x + 1, center_y + (sel_y), len, color, alpha);
                    break;
                }
            }
            // vline: at mirror x position, visible when row_index in X-range
            if (mirror_visible)
            {
                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    egui_canvas_draw_vline(center_x + (-sel_y), center_y + (-(radius - offset)), len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    egui_canvas_draw_vline(center_x + (-sel_y), center_y + 1, len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    egui_canvas_draw_vline(center_x + (sel_y), center_y + (-(radius - offset)), len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                    egui_canvas_draw_vline(center_x + (sel_y), center_y + 1, len, color, alpha);
                    break;
                }
            }
        }
    }

    // write reserve value with rect
    len = radius - info->item_count;
    if (len > 0)
    {
        switch (type)
        {
        case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
            egui_canvas_draw_fillrect(center_x + (-len), center_y + (-len), len, len, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
            egui_canvas_draw_fillrect(center_x + (-len), center_y + 1, len, len, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
            egui_canvas_draw_fillrect(center_x + 1, center_y + (-len), len, len, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
            egui_canvas_draw_fillrect(center_x + 1, center_y + 1, len, len, color, alpha);
            break;
        }
    }
}

/* egui_canvas_get_circle_corner_value moved to egui_canvas.h for reuse by gradient code */

int egui_canvas_get_circle_left_top(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    // int info_type;
    // int info_type_inner;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    // egui_dim_t len;
    // uint16_t start_offset;
    // uint16_t valid_count;
    // uint16_t data_value_offset;
    EGUI_UNUSED(self);

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, center_x - radius, center_y - radius, radius, radius);
    if (!egui_region_pt_in_rect(&region, x, y))
    {
        return 0;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return 0;
    }

    sel_x = (-x) + center_x;
    sel_y = (-y) + center_y;

    col_index = radius - sel_x;
    row_index = radius - sel_y;

    // get the mix alpha value
    mix_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
    if (mix_alpha == 0)
    {
        *alpha = 0;
        return 1;
    }

    *alpha = egui_color_alpha_mix(mix_alpha, *alpha);

    return 1;
}

int egui_canvas_get_circle_left_bottom(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha)
{
    // egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    // int info_type_inner;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    // egui_dim_t len;
    // uint16_t start_offset;
    // uint16_t valid_count;
    // uint16_t data_value_offset;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, center_x - radius, center_y + 1, radius, radius);
    if (!egui_region_pt_in_rect(&region, x, y))
    {
        return 0;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return 0;
    }

    sel_x = (-x) + center_x;
    sel_y = y - center_y;

    col_index = radius - sel_x;
    row_index = radius - sel_y;

    // get the mix alpha value
    mix_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
    if (mix_alpha == 0)
    {
        *alpha = 0;
        return 1;
    }

    *alpha = egui_color_alpha_mix(mix_alpha, *alpha);

    return 1;
}

int egui_canvas_get_circle_right_top(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha)
{
    // egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    // int info_type;
    //  int info_type_inner;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    // egui_dim_t len;
    // uint16_t start_offset;
    // uint16_t valid_count;
    // uint16_t data_value_offset;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, center_x + 1, center_y - radius, radius, radius);
    if (!egui_region_pt_in_rect(&region, x, y))
    {
        return 0;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return 0;
    }

    sel_x = x - center_x;
    sel_y = (-y) + center_y;

    col_index = radius - sel_x;
    row_index = radius - sel_y;

    // get the mix alpha value
    mix_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
    if (mix_alpha == 0)
    {
        *alpha = 0;
        return 1;
    }

    *alpha = egui_color_alpha_mix(mix_alpha, *alpha);

    return 1;
}

int egui_canvas_get_circle_right_bottom(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha)
{
    // egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    // int info_type;
    // int info_type_inner;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    // egui_dim_t len;
    // uint16_t start_offset;
    // uint16_t valid_count;
    // uint16_t data_value_offset;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, center_x + 1, center_y + 1, radius, radius);
    if (!egui_region_pt_in_rect(&region, x, y))
    {
        return 0;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return 0;
    }

    sel_x = x - center_x;
    sel_y = y - center_y;

    col_index = radius - sel_x;
    row_index = radius - sel_y;

    // get the mix alpha value
    mix_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
    if (mix_alpha == 0)
    {
        *alpha = 0;
        return 1;
    }

    *alpha = egui_color_alpha_mix(mix_alpha, *alpha);

    return 1;
}

void egui_canvas_draw_circle_corner(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t stroke_width, int type, egui_color_t color,
                                    egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    // egui_dim_t len;
    // uint16_t start_offset;
    // uint16_t valid_count;
    // uint16_t data_value_offset;
    egui_alpha_t circle_alpha;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    egui_region_t region;
    egui_region_t region_intersect;
    switch (type)
    {
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
        egui_region_init(&region, center_x - radius, center_y - radius, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
        egui_region_init(&region, center_x - radius, center_y + 1, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
        egui_region_init(&region, center_x + 1, center_y - radius, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
        egui_region_init(&region, center_x + 1, center_y + 1, radius, radius);
        break;
    }
    egui_region_intersect(&region, &self->base_view_work_region, &region_intersect);
    if (egui_region_is_empty(&region_intersect))
    {
        return;
    }

    // if radius <= stroke_width, draw a filled circle
    if (radius <= stroke_width)
    {
        egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, type, color, alpha);
        return;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return;
    }

    egui_dim_t radius_inner = radius - stroke_width;
    const egui_circle_info_t *info_inner = egui_canvas_get_circle_item(radius_inner);
    if (info_inner == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius_inner);
        return;
    }

    // Get the start and end row/col index of the arc
    egui_dim_t row_index_start, row_index_end;
    egui_dim_t col_index_start, col_index_end;

    row_index_start = 0;
    row_index_end = radius;

    col_index_start = 0;
    col_index_end = radius;

    egui_dim_t diff_x = region_intersect.location.x - region.location.x;
    egui_dim_t diff_y = region_intersect.location.y - region.location.y;

    switch (type)
    {
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
        row_index_start = diff_y;
        row_index_end = row_index_start + region_intersect.size.height;

        col_index_start = diff_x;
        col_index_end = col_index_start + region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
        row_index_end = radius - diff_y;
        row_index_start = row_index_end - region_intersect.size.height;

        col_index_start = diff_x;
        col_index_end = col_index_start + region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
        row_index_start = diff_y;
        row_index_end = row_index_start + region_intersect.size.height;

        col_index_end = radius - diff_x;
        col_index_start = col_index_end - region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
        row_index_end = radius - diff_y;
        row_index_start = row_index_end - region_intersect.size.height;

        col_index_end = radius - diff_x;
        col_index_start = col_index_end - region_intersect.size.width;
        break;
    }

    for (row_index = row_index_start; row_index < row_index_end; row_index++)
    {
        sel_y = radius - row_index;

        circle_alpha = EGUI_ALPHA_0;
        col_index = 0;

        // get the start index of the valid data
        if (row_index < info->item_count)
        {
            col_index = ((const egui_circle_item_t *)info->items)[row_index].start_offset;
        }

        for (col_index = EGUI_MAX(col_index, col_index_start); col_index < col_index_end; col_index++)
        {
            sel_x = radius - col_index;

            // get the alpha value
            if (circle_alpha != EGUI_ALPHA_100)
            {
                circle_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
                // check if the point is inside the arc
                if (circle_alpha == 0)
                {
                    continue;
                }
            }

            mix_alpha = egui_color_alpha_mix(circle_alpha, alpha);
            // check inner circle
            if (row_index >= stroke_width && col_index >= stroke_width)
            {
                egui_alpha_t alpha_inner = egui_canvas_get_circle_corner_value(row_index - stroke_width, col_index - stroke_width, info_inner);
                // if here get 0xff, means inner circle is full, close this work???
                if (alpha_inner != 0)
                {
                    if (alpha_inner == EGUI_ALPHA_100)
                    {
                        // in inner circle, do not draw anything
                        break;
                    }
                    else
                    {
                        mix_alpha = egui_color_alpha_mix(mix_alpha, EGUI_ALPHA_100 - alpha_inner);
                    }
                }
            }

            switch (type)
            {
            case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (-sel_y), color, mix_alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (sel_y), color, mix_alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (-sel_y), color, mix_alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (sel_y), color, mix_alpha);
                break;
            }
        }
    }
}

__EGUI_STATIC_INLINE__ egui_alpha_t arc_edge_smoothstep_alpha(egui_float_t signed_dist)
{
// 1.5-pixel wide AA transition zone with smoothstep for smoother radial edges
// half_transition = 0.75 in Q16.16 = 49152
#define ARC_AA_HALF_TRANSITION 49152

    if (signed_dist >= ARC_AA_HALF_TRANSITION)
    {
        return EGUI_ALPHA_100;
    }
    if (signed_dist <= -ARC_AA_HALF_TRANSITION)
    {
        return EGUI_ALPHA_0;
    }

    // Smoothstep interpolation: map [-0.75, 0.75] to [0, 255]
    // t = (signed_dist + 0.75) / 1.5,  then smoothstep(t) = t^2 * (3 - 2t)
    int32_t coverage = signed_dist + ARC_AA_HALF_TRANSITION; // [0, 1.5] in Q16.16, max = 98304
    // Normalize to [0, 1.0]: t = coverage * (2/3),  2/3 in Q16.16 ≈ 43691
    int32_t t = (int32_t)(((int64_t)coverage * 43691) >> EGUI_FLOAT_FRAC); // [0, 65536]
    if (t < 0)
    {
        t = 0;
    }
    else if (t > (1 << EGUI_FLOAT_FRAC))
    {
        t = (1 << EGUI_FLOAT_FRAC);
    }
    // smoothstep: t^2 * (3 - 2t)
    int32_t t_sq = (int32_t)(((int64_t)t * t) >> EGUI_FLOAT_FRAC);
    int32_t smooth = (int32_t)(((int64_t)t_sq * (3 * (1 << EGUI_FLOAT_FRAC) - 2 * t)) >> EGUI_FLOAT_FRAC);
    egui_alpha_t alpha = (egui_alpha_t)((smooth * EGUI_ALPHA_100) >> EGUI_FLOAT_FRAC);
    if (alpha > EGUI_ALPHA_100)
    {
        alpha = EGUI_ALPHA_100;
    }
    return alpha;
}

__EGUI_STATIC_INLINE__ egui_alpha_t arc_get_point_alpha(egui_dim_t x, egui_dim_t y, egui_float_t start_k, egui_float_t end_k, egui_float_t start_cos,
                                                        egui_float_t end_cos, int16_t start_angle, int16_t end_angle)
{
    egui_float_t px = EGUI_FLOAT_VALUE_INT(x);
    egui_float_t py = EGUI_FLOAT_VALUE_INT(y);
    egui_alpha_t alpha_start = EGUI_ALPHA_100;
    egui_alpha_t alpha_end = EGUI_ALPHA_100;

    // Start edge: line y = start_k * x, inside when y > start_k * x (above the line)
    if (start_angle != 0)
    {
        egui_float_t raw_dist = py - EGUI_FLOAT_MULT(start_k, px);
        egui_float_t signed_dist = EGUI_FLOAT_MULT(raw_dist, start_cos);

        alpha_start = arc_edge_smoothstep_alpha(signed_dist);
        if (alpha_start == EGUI_ALPHA_0)
        {
            return EGUI_ALPHA_0;
        }
    }

    // End edge: line y = end_k * x, inside when y < end_k * x (below the line)
    if (end_angle != 90)
    {
        egui_float_t raw_dist = EGUI_FLOAT_MULT(end_k, px) - py;
        egui_float_t signed_dist = EGUI_FLOAT_MULT(raw_dist, end_cos);

        alpha_end = arc_edge_smoothstep_alpha(signed_dist);
        if (alpha_end == EGUI_ALPHA_0)
        {
            return EGUI_ALPHA_0;
        }
    }

    return egui_color_alpha_mix(alpha_start, alpha_end);
}

void egui_canvas_draw_arc_corner_fill(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, int type,
                                      egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    egui_alpha_t circle_alpha;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    egui_region_t region;
    egui_region_t region_intersect;
    switch (type)
    {
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
        egui_region_init(&region, center_x - radius, center_y - radius, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
        egui_region_init(&region, center_x - radius, center_y + 1, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
        egui_region_init(&region, center_x + 1, center_y - radius, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
        egui_region_init(&region, center_x + 1, center_y + 1, radius, radius);
        break;
    }
    egui_region_intersect(&region, &self->base_view_work_region, &region_intersect);
    if (egui_region_is_empty(&region_intersect))
    {
        return;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return;
    }

    if (end_angle > 90)
    {
        end_angle = 90;
    }
    if (start_angle < 0)
    {
        start_angle = 0;
    }

    if (start_angle >= end_angle)
    {
        return;
    }

    if (start_angle == 0 && end_angle == 90)
    {
        switch (type)
        {
        case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
            egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
            egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
            egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
            egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);
            break;
        }

        return;
    }

    // Get the start and end row/col index of the arc
    egui_dim_t row_index_start, row_index_end;
    egui_dim_t col_index_start, col_index_end;

    row_index_start = 0;
    row_index_end = radius;

    col_index_start = 0;
    col_index_end = radius;

    egui_dim_t diff_x = region_intersect.location.x - region.location.x;
    egui_dim_t diff_y = region_intersect.location.y - region.location.y;

    switch (type)
    {
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
        row_index_start = diff_y;
        row_index_end = row_index_start + region_intersect.size.height;

        col_index_start = diff_x;
        col_index_end = col_index_start + region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
        row_index_end = radius - diff_y;
        row_index_start = row_index_end - region_intersect.size.height;

        col_index_start = diff_x;
        col_index_end = col_index_start + region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
        row_index_start = diff_y;
        row_index_end = row_index_start + region_intersect.size.height;

        col_index_end = radius - diff_x;
        col_index_start = col_index_end - region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
        row_index_end = radius - diff_y;
        row_index_start = row_index_end - region_intersect.size.height;

        col_index_end = radius - diff_x;
        col_index_start = col_index_end - region_intersect.size.width;
        break;
    }

    egui_float_t start_k = tan_val_list[start_angle];
    egui_float_t end_k = tan_val_list[end_angle];

    egui_float_t start_ck = ctan_val_list[start_angle];
    egui_float_t end_ck = ctan_val_list[end_angle];

    egui_float_t start_cos = cos_val_list[start_angle];
    egui_float_t end_cos = cos_val_list[end_angle];

    egui_dim_t last_start_x;
    egui_dim_t last_end_x;

    egui_dim_t cur_start_x;
    egui_dim_t cur_end_x;

    egui_dim_t next_start_x;
    egui_dim_t next_end_x;

    egui_dim_t x_allow_min;
    egui_dim_t x_allow_max;

    egui_dim_t x_arc_allow_min;
    egui_dim_t x_arc_allow_max;

    // for speed, calculate the first value
    sel_y = radius;
    if (start_angle != 0)
    {
        cur_start_x = EGUI_FLOAT_MULT_LIMIT(sel_y, start_ck);
        next_start_x = cur_start_x;
    }
    else
    {
        cur_start_x = EGUI_DIM_MAX;
        next_start_x = EGUI_DIM_MAX;
    }

    if (end_angle != 0)
    {
        cur_end_x = EGUI_FLOAT_MULT_LIMIT(sel_y, end_ck);
        next_end_x = cur_end_x;
    }
    else
    {
        cur_end_x = EGUI_DIM_MAX;
        next_end_x = EGUI_DIM_MAX;
    }

    last_start_x = cur_start_x;
    last_end_x = cur_end_x;

    for (row_index = row_index_start; row_index < row_index_end; row_index++)
    {
        sel_y = radius - row_index;

        // for speed, calculate the start and end x of the arc
        if (start_angle != 0)
        {
            cur_start_x = next_start_x;

            next_start_x = EGUI_FLOAT_MULT_LIMIT((sel_y - 1), start_ck);
        }

        if (end_angle != 0)
        {
            cur_end_x = next_end_x;

            next_end_x = EGUI_FLOAT_MULT_LIMIT((sel_y - 1), end_ck);
        }

        x_allow_min = next_end_x - 2;
        x_allow_max = last_start_x + 2;

        x_arc_allow_min = next_start_x - 2;
        x_arc_allow_max = last_end_x + 2;

        circle_alpha = EGUI_ALPHA_0;
        col_index = 0;

        // get the start index of the valid data
        if (row_index < info->item_count)
        {
            col_index = ((const egui_circle_item_t *)info->items)[row_index].start_offset;
        }

        for (col_index = EGUI_MAX(col_index, col_index_start); col_index < col_index_end; col_index++)
        {
            sel_x = radius - col_index;

            // For speed, check need x range
            if ((sel_x < x_allow_min) || (sel_x > x_allow_max))
            {
                continue;
            }

            // get the alpha value
            if (circle_alpha != EGUI_ALPHA_100)
            {
                circle_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
                // check if the point is inside the arc
                if (circle_alpha == 0)
                {
                    continue;
                }
            }

            mix_alpha = egui_color_alpha_mix(circle_alpha, alpha);
            // For speed, check need x range
            if (!((sel_x > x_arc_allow_max) && ((next_start_x > radius) || (sel_x < x_arc_allow_min))))
            {
                egui_alpha_t point_alpha = arc_get_point_alpha(sel_x, sel_y, start_k, end_k, start_cos, end_cos, start_angle, end_angle);
                if (point_alpha == 0)
                {
                    continue;
                }
                mix_alpha = egui_color_alpha_mix(mix_alpha, point_alpha);
            }

            switch (type)
            {
            case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (-sel_y), color, mix_alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (sel_y), color, mix_alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (-sel_y), color, mix_alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (sel_y), color, mix_alpha);
                break;
            }
        }

        last_start_x = cur_start_x;
        last_end_x = cur_end_x;
    }
}

/**
 * \brief           Draw filled rectangle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle, egui_canvas_draw_round_rectangle, egui_canvas_draw_round_rectangle_fill
 */
void egui_canvas_draw_rectangle_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    egui_canvas_draw_fillrect(x, y, width, height, color, alpha);
}

/**
 * \brief           Draw rectangle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle_fill, egui_canvas_draw_round_rectangle, egui_canvas_draw_round_rectangle_fill
 */
void egui_canvas_draw_rectangle(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t stroke_width, egui_color_t color,
                                egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    if (width == 0 || height == 0 || stroke_width == 0)
    {
        return;
    }
    if (stroke_width >= (width >> 1) || stroke_width >= (height >> 1))
    {
        egui_canvas_draw_rectangle_fill(x, y, width, height, color, alpha);
        return;
    }

    // draw the left and right rectangles
    egui_canvas_draw_fillrect(x, y, stroke_width, height, color, alpha);
    egui_canvas_draw_fillrect(x + width - stroke_width, y, stroke_width, height, color, alpha);

    // draw the top and bottom rectangles
    egui_canvas_draw_fillrect(x + stroke_width, y, width - (stroke_width << 1), stroke_width, color, alpha);
    egui_canvas_draw_fillrect(x + stroke_width, y + height - stroke_width, width - (stroke_width << 1), stroke_width, color, alpha);
}

/**
 * \brief           Draw filled rectangle with rounded corners
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       r: Corner radius, max value can be r = MIN(width, height) / 2
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle, egui_canvas_draw_rectangle_fill, egui_canvas_draw_round_rectangle
 */
void egui_canvas_draw_round_rectangle_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius, egui_color_t color,
                                           egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    if (radius >= (height >> 1))
    {
        radius = (height >> 1) - 1;
    }
    if (radius >= (width >> 1))
    {
        radius = (width >> 1) - 1;
    }
    if (radius)
    {
        // draw the middle rectangle
        egui_canvas_draw_rectangle_fill(x + radius, y, width - (radius << 1), height, color, alpha);

        // draw the left and right rectangles
        egui_canvas_draw_rectangle_fill(x, y + radius, radius, height - (radius << 1), color, alpha);
        egui_canvas_draw_rectangle_fill(x + width - radius, y + radius, radius, height - (radius << 1), color, alpha);

        // draw the four corners
        egui_canvas_draw_circle_corner_fill(x + radius, y + radius, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
        egui_canvas_draw_circle_corner_fill(x + radius, y + height - radius - 1, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
        egui_canvas_draw_circle_corner_fill(x + width - radius - 1, y + radius, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
        egui_canvas_draw_circle_corner_fill(x + width - radius - 1, y + height - radius - 1, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);
    }
    else
    {
        egui_canvas_draw_rectangle_fill(x, y, width, height, color, alpha);
    }
}

/**
 * \brief           Draw rectangle with rounded corners
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       r: Corner radius, max value can be r = MIN(width, height) / 2
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle, egui_canvas_draw_rectangle_fill, egui_canvas_draw_round_rectangle_fill
 */
void egui_canvas_draw_round_rectangle(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius, egui_dim_t stroke_width,
                                      egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    if (width == 0 || height == 0 || stroke_width == 0)
    {
        return;
    }
    if (stroke_width >= (width >> 1) || stroke_width >= (height >> 1))
    {
        egui_canvas_draw_round_rectangle_fill(x, y, width, height, stroke_width, color, alpha);
        return;
    }
    if (radius >= (height >> 1))
    {
        radius = (height >> 1) - 1;
    }
    if (radius >= (width >> 1))
    {
        radius = (width >> 1) - 1;
    }
    if (radius)
    {
        // Think stroke_width is bigger than radius, in this case, we should adjust stroke_width to radius.
        stroke_width = EGUI_MIN(radius, stroke_width);

        // draw the left and right rectangles
        egui_canvas_draw_rectangle_fill(x, y + radius, stroke_width, height - (radius << 1), color, alpha);
        egui_canvas_draw_rectangle_fill(x + width - stroke_width, y + radius, stroke_width, height - (radius << 1), color, alpha);

        // draw the top and bottom rectangles
        egui_canvas_draw_fillrect(x + radius, y, width - (radius << 1), stroke_width, color, alpha);
        egui_canvas_draw_fillrect(x + radius, y + height - stroke_width, width - (radius << 1), stroke_width, color, alpha);

        // draw the four corners
        egui_canvas_draw_circle_corner(x + radius, y + radius, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
        egui_canvas_draw_circle_corner(x + radius, y + height - radius - 1, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
        egui_canvas_draw_circle_corner(x + width - radius - 1, y + radius, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
        egui_canvas_draw_circle_corner(x + width - radius - 1, y + height - radius - 1, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color,
                                       alpha);
    }
    else
    {
        egui_canvas_draw_rectangle_fill(x, y, width, height, color, alpha);
    }
}

/**
 * \brief           Draw filled rectangle with rounded corners
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       r: Corner radius, max value can be r = MIN(width, height) / 2
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle, egui_canvas_draw_rectangle_fill, egui_canvas_draw_round_rectangle
 */
void egui_canvas_draw_round_rectangle_corners_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius_left_top,
                                                   egui_dim_t radius_left_bottom, egui_dim_t radius_right_top, egui_dim_t radius_right_bottom,
                                                   egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    // left top corner
    if (radius_left_top >= (height >> 1))
    {
        radius_left_top = (height >> 1) - 1;
    }
    if (radius_left_top >= (width >> 1))
    {
        radius_left_top = (width >> 1) - 1;
    }
    if (radius_left_top)
    {
        // draw the corners
        egui_canvas_draw_circle_corner_fill(x + radius_left_top, y + radius_left_top, radius_left_top, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
    }

    // left bottom corner
    if (radius_left_bottom >= (height >> 1))
    {
        radius_left_bottom = (height >> 1) - 1;
    }
    if (radius_left_bottom >= (width >> 1))
    {
        radius_left_bottom = (width >> 1) - 1;
    }
    if (radius_left_bottom)
    {
        // draw the corners
        egui_canvas_draw_circle_corner_fill(x + radius_left_bottom, y + height - radius_left_bottom - 1, radius_left_bottom,
                                            EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
    }

    // right top corner
    if (radius_right_top >= (height >> 1))
    {
        radius_right_top = (height >> 1) - 1;
    }
    if (radius_right_top >= (width >> 1))
    {
        radius_right_top = (width >> 1) - 1;
    }
    if (radius_right_top)
    {
        // draw the corners
        egui_canvas_draw_circle_corner_fill(x + width - radius_right_top - 1, y + radius_right_top, radius_right_top, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color,
                                            alpha);
    }

    // right bottom corner
    if (radius_right_bottom >= (height >> 1))
    {
        radius_right_bottom = (height >> 1) - 1;
    }
    if (radius_right_bottom >= (width >> 1))
    {
        radius_right_bottom = (width >> 1) - 1;
    }
    if (radius_right_bottom)
    {
        // draw the corners
        egui_canvas_draw_circle_corner_fill(x + width - radius_right_bottom - 1, y + height - radius_right_bottom - 1, radius_right_bottom,
                                            EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);
    }

    // draw the middle rectangle
    egui_dim_t left_width = EGUI_MAX(radius_left_top, radius_left_bottom);
    egui_dim_t right_width = EGUI_MAX(radius_right_top, radius_right_bottom);
    egui_dim_t middle_width = width - left_width - right_width;

    egui_canvas_draw_rectangle_fill(x + left_width, y, middle_width, height, color, alpha);

    // draw the left rectangles
    egui_canvas_draw_rectangle_fill(x, y + radius_left_top, left_width, height - (radius_left_top + radius_left_bottom), color, alpha);
    // draw the left small rectangles
    if (radius_left_top < radius_left_bottom)
    {
        egui_canvas_draw_rectangle_fill(x + radius_left_top, y, radius_left_bottom - radius_left_top, radius_left_top, color, alpha);
    }
    else if (radius_left_bottom < radius_left_top)
    {
        egui_canvas_draw_rectangle_fill(x + radius_left_bottom, y + height - radius_left_bottom, radius_left_top - radius_left_bottom, radius_left_bottom,
                                        color, alpha);
    }

    // draw the right rectangles
    egui_canvas_draw_rectangle_fill(x + width - right_width, y + radius_right_top, right_width, height - (radius_right_top + radius_right_bottom), color,
                                    alpha);
    // draw the right small rectangles
    if (radius_right_top < radius_right_bottom)
    {
        egui_canvas_draw_rectangle_fill(x + width - right_width, y, radius_right_bottom - radius_right_top, radius_right_top, color, alpha);
    }
    else if (radius_right_bottom < radius_right_top)
    {
        egui_canvas_draw_rectangle_fill(x + width - right_width, y + height - radius_right_bottom, radius_right_top - radius_right_bottom, radius_right_bottom,
                                        color, alpha);
    }
}

/**
 * \brief           Draw filled rectangle with rounded corners
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       r: Corner radius, max value can be r = MIN(width, height) / 2
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle, egui_canvas_draw_rectangle_fill, egui_canvas_draw_round_rectangle
 */
void egui_canvas_draw_round_rectangle_corners(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius_left_top,
                                              egui_dim_t radius_left_bottom, egui_dim_t radius_right_top, egui_dim_t radius_right_bottom,
                                              egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    if (width == 0 || height == 0 || stroke_width == 0)
    {
        return;
    }
    if (stroke_width >= (width >> 1) || stroke_width >= (height >> 1))
    {
        egui_canvas_draw_round_rectangle_corners_fill(x, y, width, height, radius_left_top, radius_left_bottom, radius_right_top, radius_right_bottom, color,
                                                      alpha);
        return;
    }
    // left top corner
    if (radius_left_top >= (height >> 1))
    {
        radius_left_top = (height >> 1) - 1;
    }
    if (radius_left_top >= (width >> 1))
    {
        radius_left_top = (width >> 1) - 1;
    }
    if (radius_left_top)
    {
        // draw the corners
        egui_canvas_draw_circle_corner(x + radius_left_top, y + radius_left_top, radius_left_top, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
    }

    // left bottom corner
    if (radius_left_bottom >= (height >> 1))
    {
        radius_left_bottom = (height >> 1) - 1;
    }
    if (radius_left_bottom >= (width >> 1))
    {
        radius_left_bottom = (width >> 1) - 1;
    }
    if (radius_left_bottom)
    {
        // draw the corners
        egui_canvas_draw_circle_corner(x + radius_left_bottom, y + height - radius_left_bottom - 1, radius_left_bottom, stroke_width,
                                       EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
    }

    // right top corner
    if (radius_right_top >= (height >> 1))
    {
        radius_right_top = (height >> 1) - 1;
    }
    if (radius_right_top >= (width >> 1))
    {
        radius_right_top = (width >> 1) - 1;
    }
    if (radius_right_top)
    {
        // draw the corners
        egui_canvas_draw_circle_corner(x + width - radius_right_top - 1, y + radius_right_top, radius_right_top, stroke_width,
                                       EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
    }

    // right bottom corner
    if (radius_right_bottom >= (height >> 1))
    {
        radius_right_bottom = (height >> 1) - 1;
    }
    if (radius_right_bottom >= (width >> 1))
    {
        radius_right_bottom = (width >> 1) - 1;
    }
    if (radius_right_bottom)
    {
        // draw the corners
        egui_canvas_draw_circle_corner(x + width - radius_right_bottom - 1, y + height - radius_right_bottom - 1, radius_right_bottom, stroke_width,
                                       EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);
    }

    // draw the left and right rectangles
    egui_canvas_draw_rectangle_fill(x, y + radius_left_top, stroke_width, height - (radius_left_top + radius_left_bottom), color, alpha);
    egui_canvas_draw_rectangle_fill(x + width - stroke_width, y + radius_right_top, stroke_width, height - (radius_right_top + radius_right_bottom), color,
                                    alpha);

    // draw the top and bottom rectangles
    // Think stroke_width is bigger than radius_left_top, radius_left_bottom, radius_right_top, radius_right_bottom
    egui_canvas_draw_fillrect(x + EGUI_MAX(radius_left_top, stroke_width), y,
                              width - (EGUI_MAX(radius_left_top, stroke_width) + EGUI_MAX(radius_right_top, stroke_width)), stroke_width, color, alpha);
    if (radius_left_top < stroke_width)
    {
        egui_canvas_draw_fillrect(x + radius_left_top, y, stroke_width - radius_left_top, radius_left_top, color, alpha);
    }

    if (radius_right_top < stroke_width)
    {
        egui_canvas_draw_fillrect(x + width - stroke_width, y, stroke_width - radius_right_top, radius_right_top, color, alpha);
    }

    egui_canvas_draw_fillrect(x + EGUI_MAX(radius_left_bottom, stroke_width), y + height - stroke_width,
                              width - (EGUI_MAX(radius_left_bottom, stroke_width) + EGUI_MAX(radius_right_bottom, stroke_width)), stroke_width, color, alpha);
    if (radius_left_bottom < stroke_width)
    {
        egui_canvas_draw_fillrect(x + radius_left_bottom, y + height - radius_left_bottom, stroke_width - radius_left_bottom, radius_left_bottom, color, alpha);
    }

    if (radius_right_bottom < stroke_width)
    {
        egui_canvas_draw_fillrect(x + width - stroke_width, y + height - radius_right_bottom, stroke_width - radius_right_bottom, radius_right_bottom, color,
                                  alpha);
    }

    // egui_canvas_draw_fillrect(x + radius_left_top,    y,                      width - (radius_left_top + radius_right_top), stroke_width, color,
    // alpha); egui_canvas_draw_fillrect(x + radius_left_bottom, y + height - stroke_width, width - (radius_left_bottom + radius_right_bottom),
    // stroke_width, color, alpha);
}

/**
 * \brief           Draw filled circle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: X position of circle center
 * \param[in]       y: X position of circle center
 * \param[in]       r: Circle radius
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_circle_basic, egui_canvas_draw_circle_corner, egui_canvas_draw_circle_corner_fill
 */
void egui_canvas_draw_circle_fill_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
    egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
    egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
    egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);

    // draw the center line
    egui_canvas_draw_hline(center_x - radius, center_y, radius, color, alpha);
    egui_canvas_draw_hline(center_x + 1, center_y, radius, color, alpha);
    egui_canvas_draw_vline(center_x, center_y - radius, (radius << 1) + 1, color, alpha);
}

/**
 * \brief           Draw filled circle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: X position of circle center
 * \param[in]       y: X position of circle center
 * \param[in]       r: Circle radius
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_circle_basic, egui_canvas_draw_circle_corner, egui_canvas_draw_circle_corner_fill
 */
void egui_canvas_draw_arc_fill_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_color_t color,
                                     egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    int16_t start_angle_tmp;
    int16_t end_angle_tmp;
    int is_need_middle_point = 0;

    if (start_angle < 0 || end_angle < 0)
    {
        // not support
        return;
    }

    if (start_angle > end_angle)
    {
        // not support
        return;
    }

    if (start_angle == end_angle)
    {
        // do nothing
        return;
    }

    // for some case, arc need over flow 0. can use 360 to fix it.
    if (start_angle > 720 || end_angle > 720)
    {
        // not support
        return;
    }

    // First draw area
    do
    {
        if (start_angle < 90)
        {
            start_angle_tmp = start_angle;
            end_angle_tmp = end_angle;
            egui_canvas_draw_arc_corner_fill(center_x, center_y, radius, start_angle_tmp, end_angle_tmp, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);
        }
        if (start_angle < 180)
        {
            start_angle_tmp = start_angle - 90;
            end_angle_tmp = end_angle - 90;
            egui_canvas_draw_arc_corner_fill(center_x, center_y, radius, 90 - end_angle_tmp, 90 - start_angle_tmp, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color,
                                             alpha);
        }

        if (start_angle < 270)
        {
            start_angle_tmp = start_angle - 180;
            end_angle_tmp = end_angle - 180;
            egui_canvas_draw_arc_corner_fill(center_x, center_y, radius, start_angle_tmp, end_angle_tmp, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
        }

        if (start_angle < 360)
        {
            start_angle_tmp = start_angle - 270;
            end_angle_tmp = end_angle - 270;
            egui_canvas_draw_arc_corner_fill(center_x, center_y, radius, 90 - end_angle_tmp, 90 - start_angle_tmp, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color,
                                             alpha);
        }

        // draw the line
        if (start_angle <= 0 && end_angle >= 0)
        {
            egui_canvas_draw_hline(center_x + 1, center_y, radius, color, alpha);
            is_need_middle_point = 1;
        }
        if (start_angle <= 90 && end_angle >= 90)
        {
            egui_canvas_draw_vline(center_x, center_y + 1, radius, color, alpha);
            is_need_middle_point = 1;
        }
        if (start_angle <= 180 && end_angle >= 180)
        {
            egui_canvas_draw_hline(center_x - radius, center_y, radius, color, alpha);
            is_need_middle_point = 1;
        }
        if (start_angle <= 270 && end_angle >= 270)
        {
            egui_canvas_draw_vline(center_x, center_y - radius, radius, color, alpha);
            is_need_middle_point = 1;
        }

        start_angle = start_angle - 360;
        if (start_angle < 0)
        {
            start_angle = 0;
        }
        end_angle = end_angle - 360;
    } while (end_angle > 0);

    if (is_need_middle_point)
    {
        egui_canvas_draw_point(center_x, center_y, color, alpha);
    }
}

/**
 * \brief           Draw circle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: X position of circle center
 * \param[in]       y: X position of circle center
 * \param[in]       r: Circle radius
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_circle_fill_basic, egui_canvas_draw_circle_corner, egui_canvas_draw_circle_corner_fill
 */
void egui_canvas_draw_circle_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    // if radius <= stroke_width, draw a filled circle
    if (radius <= stroke_width)
    {
        egui_canvas_draw_circle_fill_basic(center_x, center_y, radius, color, alpha);
        return;
    }

    egui_canvas_draw_circle_corner(center_x, center_y, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
    egui_canvas_draw_circle_corner(center_x, center_y, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
    egui_canvas_draw_circle_corner(center_x, center_y, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
    egui_canvas_draw_circle_corner(center_x, center_y, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);

    // draw the center line
    egui_canvas_draw_hline(center_x - radius, center_y, stroke_width, color, alpha);
    egui_canvas_draw_hline(center_x + radius - stroke_width + 1, center_y, stroke_width, color, alpha);

    egui_canvas_draw_vline(center_x, center_y - radius, stroke_width, color, alpha);
    egui_canvas_draw_vline(center_x, center_y + radius - stroke_width + 1, stroke_width, color, alpha);
}

void egui_canvas_draw_arc_corner(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, int stroke_width,
                                 int type, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    // egui_dim_t len;
    // uint16_t start_offset;
    // uint16_t valid_count;
    // uint16_t data_value_offset;
    egui_alpha_t circle_alpha;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    egui_region_t region;
    egui_region_t region_intersect;
    switch (type)
    {
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
        egui_region_init(&region, center_x - radius, center_y - radius, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
        egui_region_init(&region, center_x - radius, center_y + 1, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
        egui_region_init(&region, center_x + 1, center_y - radius, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
        egui_region_init(&region, center_x + 1, center_y + 1, radius, radius);
        break;
    }
    egui_region_intersect(&region, &self->base_view_work_region, &region_intersect);
    if (egui_region_is_empty(&region_intersect))
    {
        return;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return;
    }

    egui_dim_t radius_inner = radius - stroke_width;
    const egui_circle_info_t *info_inner = egui_canvas_get_circle_item(radius_inner);
    if (info_inner == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius_inner);
        return;
    }

    if (end_angle > 90)
    {
        end_angle = 90;
    }
    if (start_angle < 0)
    {
        start_angle = 0;
    }

    if (start_angle >= end_angle)
    {
        return;
    }

    if (start_angle == 0 && end_angle == 90)
    {
        egui_canvas_draw_circle_corner(center_x, center_y, radius, stroke_width, type, color, alpha);

        return;
    }

    // Get the start and end row/col index of the arc
    egui_dim_t row_index_start, row_index_end;
    egui_dim_t col_index_start, col_index_end;

    row_index_start = 0;
    row_index_end = radius;

    col_index_start = 0;
    col_index_end = radius;

    egui_dim_t diff_x = region_intersect.location.x - region.location.x;
    egui_dim_t diff_y = region_intersect.location.y - region.location.y;

    switch (type)
    {
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
        row_index_start = diff_y;
        row_index_end = row_index_start + region_intersect.size.height;

        col_index_start = diff_x;
        col_index_end = col_index_start + region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
        row_index_end = radius - diff_y;
        row_index_start = row_index_end - region_intersect.size.height;

        col_index_start = diff_x;
        col_index_end = col_index_start + region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
        row_index_start = diff_y;
        row_index_end = row_index_start + region_intersect.size.height;

        col_index_end = radius - diff_x;
        col_index_start = col_index_end - region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
        row_index_end = radius - diff_y;
        row_index_start = row_index_end - region_intersect.size.height;

        col_index_end = radius - diff_x;
        col_index_start = col_index_end - region_intersect.size.width;
        break;
    }

    egui_float_t start_k = tan_val_list[start_angle];
    egui_float_t end_k = tan_val_list[end_angle];

    egui_float_t start_ck = ctan_val_list[start_angle];
    egui_float_t end_ck = ctan_val_list[end_angle];

    egui_float_t start_cos = cos_val_list[start_angle];
    egui_float_t end_cos = cos_val_list[end_angle];

    egui_dim_t last_start_x;
    egui_dim_t last_end_x;

    egui_dim_t cur_start_x;
    egui_dim_t cur_end_x;

    egui_dim_t next_start_x;
    egui_dim_t next_end_x;

    egui_dim_t x_allow_min;
    egui_dim_t x_allow_max;

    egui_dim_t x_arc_allow_min;
    egui_dim_t x_arc_allow_max;

    // for speed, calculate the first value
    sel_y = radius;
    if (start_angle != 0)
    {
        cur_start_x = EGUI_FLOAT_MULT_LIMIT(sel_y, start_ck);
        next_start_x = cur_start_x;
    }
    else
    {
        cur_start_x = EGUI_DIM_MAX;
        next_start_x = EGUI_DIM_MAX;
    }

    if (end_angle != 0)
    {
        cur_end_x = EGUI_FLOAT_MULT_LIMIT(sel_y, end_ck);
        next_end_x = cur_end_x;
    }
    else
    {
        cur_end_x = EGUI_DIM_MAX;
        next_end_x = EGUI_DIM_MAX;
    }

    last_start_x = cur_start_x;
    last_end_x = cur_end_x;

    for (row_index = row_index_start; row_index < row_index_end; row_index++)
    {
        sel_y = radius - row_index;

        // for speed, calculate the start and end x of the arc
        if (start_angle != 0)
        {
            cur_start_x = next_start_x;

            next_start_x = EGUI_FLOAT_MULT_LIMIT((sel_y - 1), start_ck);
        }

        if (end_angle != 0)
        {
            cur_end_x = next_end_x;

            next_end_x = EGUI_FLOAT_MULT_LIMIT((sel_y - 1), end_ck);
        }

        x_allow_min = next_end_x - 2;
        x_allow_max = last_start_x + 2;

        x_arc_allow_min = next_start_x - 2;
        x_arc_allow_max = last_end_x + 2;

        circle_alpha = EGUI_ALPHA_0;
        col_index = 0;

        // get the start index of the valid data
        if (row_index < info->item_count)
        {
            col_index = ((const egui_circle_item_t *)info->items)[row_index].start_offset;
        }

        for (col_index = EGUI_MAX(col_index, col_index_start); col_index < col_index_end; col_index++)
        {
            sel_x = radius - col_index;

            // For speed, check need x range
            if ((sel_x < x_allow_min) || (sel_x > x_allow_max))
            {
                continue;
            }

            // get the alpha value
            if (circle_alpha != EGUI_ALPHA_100)
            {
                circle_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
                // check if the point is inside the arc
                if (circle_alpha == 0)
                {
                    continue;
                }
            }

            mix_alpha = egui_color_alpha_mix(circle_alpha, alpha);
            // check inner circle
            if (row_index >= stroke_width && col_index >= stroke_width)
            {
                egui_alpha_t alpha_inner = egui_canvas_get_circle_corner_value(row_index - stroke_width, col_index - stroke_width, info_inner);
                // if here get 0xff, means inner circle is full, close this work???
                if (alpha_inner != 0)
                {
                    if (alpha_inner == EGUI_ALPHA_100)
                    {
                        // in inner circle, do not draw anything
                        break;
                    }
                    else
                    {
                        mix_alpha = egui_color_alpha_mix(mix_alpha, EGUI_ALPHA_100 - alpha_inner);
                    }
                }
            }

            // For speed, check need x range
            if (!((sel_x > x_arc_allow_max) && ((next_start_x > radius) || (sel_x < x_arc_allow_min))))
            {
                egui_alpha_t point_alpha = arc_get_point_alpha(sel_x, sel_y, start_k, end_k, start_cos, end_cos, start_angle, end_angle);
                if (point_alpha == 0)
                {
                    continue;
                }
                mix_alpha = egui_color_alpha_mix(mix_alpha, point_alpha);
            }

            switch (type)
            {
            case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (-sel_y), color, mix_alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (sel_y), color, mix_alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (-sel_y), color, mix_alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (sel_y), color, mix_alpha);
                break;
            }
        }

        last_start_x = cur_start_x;
        last_end_x = cur_end_x;
    }
}

/**
 * \brief           Draw filled circle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: X position of circle center
 * \param[in]       y: X position of circle center
 * \param[in]       r: Circle radius
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_circle_basic, egui_canvas_draw_circle_corner, egui_canvas_draw_circle_corner_fill
 */
void egui_canvas_draw_arc_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_dim_t stroke_width,
                                egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    int16_t start_angle_tmp;
    int16_t end_angle_tmp;

    // if radius <= stroke_width, draw a filled circle
    if (radius <= stroke_width)
    {
        egui_canvas_draw_arc_fill_basic(center_x, center_y, radius, start_angle, end_angle, color, alpha);
        return;
    }

    if (start_angle < 0 || end_angle < 0)
    {
        // not support
        return;
    }

    if (start_angle > end_angle)
    {
        // not support
        return;
    }

    if (start_angle == end_angle)
    {
        // do nothing
        return;
    }

    // for some case, arc need over flow 0. can use 360 to fix it.
    if (start_angle > 720 || end_angle > 720)
    {
        // not support
        return;
    }

    // First draw area
    do
    {
        if (start_angle < 90)
        {
            start_angle_tmp = start_angle;
            end_angle_tmp = end_angle;
            egui_canvas_draw_arc_corner(center_x, center_y, radius, start_angle_tmp, end_angle_tmp, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color,
                                        alpha);
        }
        if (start_angle < 180)
        {
            start_angle_tmp = start_angle - 90;
            end_angle_tmp = end_angle - 90;
            egui_canvas_draw_arc_corner(center_x, center_y, radius, 90 - end_angle_tmp, 90 - start_angle_tmp, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM,
                                        color, alpha);
        }

        if (start_angle < 270)
        {
            start_angle_tmp = start_angle - 180;
            end_angle_tmp = end_angle - 180;
            egui_canvas_draw_arc_corner(center_x, center_y, radius, start_angle_tmp, end_angle_tmp, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color,
                                        alpha);
        }

        if (start_angle < 360)
        {
            start_angle_tmp = start_angle - 270;
            end_angle_tmp = end_angle - 270;
            egui_canvas_draw_arc_corner(center_x, center_y, radius, 90 - end_angle_tmp, 90 - start_angle_tmp, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP,
                                        color, alpha);
        }

        // draw the line
        if (start_angle <= 0 && end_angle >= 0)
        {
            egui_canvas_draw_hline(center_x + radius - stroke_width + 1, center_y, stroke_width, color, alpha);
        }
        if (start_angle <= 90 && end_angle >= 90)
        {
            egui_canvas_draw_vline(center_x, center_y + radius - stroke_width + 1, stroke_width, color, alpha);
        }
        if (start_angle <= 180 && end_angle >= 180)
        {
            egui_canvas_draw_hline(center_x - radius, center_y, stroke_width, color, alpha);
        }
        if (start_angle <= 270 && end_angle >= 270)
        {
            egui_canvas_draw_vline(center_x, center_y - radius, stroke_width, color, alpha);
        }

        start_angle = start_angle - 360;
        if (start_angle < 0)
        {
            start_angle = 0;
        }
        end_angle = end_angle - 360;
    } while (end_angle > 0);
}

#if TEST_CANVAS_TEST_WOKR
void egui_canvas_debug_print_data_array(void)
{
    EGUI_LOG_DBG("      ");
    for (int i = 0; i < self->pfb_region.size.width; i++)
    {
        EGUI_LOG_DBG("[%02d] ", i);
    }
    // next line
    EGUI_LOG_DBG("\n");

    // print data array
    for (int j = 0; j < self->pfb_region.size.height; j++)
    {
        EGUI_LOG_DBG("[%02d] ", j);
        for (int i = 0; i < self->pfb_region.size.width; i++)
        {
            EGUI_LOG_DBG(" %03d ", self->pfb[j * self->pfb_region.size.width + i] & 0xff);
        }
        // next line
        EGUI_LOG_DBG("\n");
    }
}

void egui_canvas_test_circle(void)
{
    egui_canvas_t canvas;
    egui_color_int_t pfb_arr[30 * 30];

    memset(pfb_arr, 0, sizeof(pfb_arr));

    canvas.pfb = pfb_arr;
    canvas.pfb_region.size.width = 30;
    canvas.pfb_region.size.height = 30;
    canvas.pfb_region.location.x = 0;
    canvas.pfb_region.location.y = 0;

    canvas.base_view_work_region.size.width = 30;
    canvas.base_view_work_region.size.height = 30;
    canvas.base_view_work_region.location.x = 0;
    canvas.base_view_work_region.location.y = 0;

    canvas.pfb_location_in_base_view.x = 0;
    canvas.pfb_location_in_base_view.y = 0;

    canvas.alpha = 0xff;

    canvas.mask = NULL;

    egui_canvas_debug_print_data_array(&canvas);

    egui_dim_t radius = 12;
    // egui_dim_t central_x = 12;
    // egui_dim_t central_y = 12;

    egui_dim_t central_x = radius;
    egui_dim_t central_y = radius;

    // egui_canvas_draw_circle_corner_left_top(&canvas, central_x, central_y, radius, radius, EGUI_COLOR_WHITE, 0xff);
    // egui_canvas_draw_circle_corner_left_bottom(&canvas, central_x, central_y, radius, 3, EGUI_COLOR_WHITE, 0xff);
    // egui_canvas_draw_circle_corner_right_top(&canvas, central_x, central_y, radius, 3, EGUI_COLOR_WHITE, 0xff);
    // egui_canvas_draw_circle_corner_right_bottom(&canvas, central_x, central_y, radius, 3, EGUI_COLOR_WHITE, 0xff);

    // draw the center line
    // egui_canvas_draw_hline(&canvas, central_x - radius, central_y,          2 * radius + 1, EGUI_COLOR_WHITE, 0xFF);
    // egui_canvas_draw_vline(&canvas, central_x,          central_y - radius, 2 * radius + 1, EGUI_COLOR_WHITE, 0xFF);

    egui_canvas_draw_arc_corner_fill_right_bottom(&canvas, central_x, central_y, radius, 0, 30, EGUI_COLOR_WHITE, 0xff);
    // egui_canvas_draw_arc_corner_fill_right_bottom(&canvas, central_x, central_y, radius, 0, 45, EGUI_COLOR_WHITE, 0xff);

    egui_canvas_debug_print_data_array(&canvas);
    // while(1) {
    //     egui_api_delay(1000);
    // };
}
#endif

void egui_canvas_draw_text(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    font->api->draw_string(font, string, x, y, color, alpha);
}

void egui_canvas_draw_text_in_rect_with_line_space(const egui_font_t *font, const void *string, egui_region_t *rect, uint8_t align_type, egui_dim_t line_space,
                                                   egui_color_t color, egui_alpha_t alpha)
{
    egui_font_draw_string_in_rect(font, string, rect, align_type, line_space, color, alpha);
}

void egui_canvas_draw_text_in_rect(const egui_font_t *font, const void *string, egui_region_t *rect, uint8_t align_type, egui_color_t color, egui_alpha_t alpha)
{
    egui_font_draw_string_in_rect(font, string, rect, align_type, 0, color, alpha);
}

void egui_canvas_draw_image(const egui_image_t *img, egui_dim_t x, egui_dim_t y)
{
    img->api->draw_image(img, x, y);
}

void egui_canvas_draw_image_resize(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    img->api->draw_image_resize(img, x, y, width, height);
}

void egui_canvas_draw_image_color(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_image_std_draw_image_color(img, x, y, color, alpha);
}

void egui_canvas_draw_image_resize_color(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                         egui_alpha_t alpha)
{
    egui_image_std_draw_image_resize_color(img, x, y, width, height, color, alpha);
}

void egui_canvas_calc_work_region(egui_region_t *base_region)
{
    egui_canvas_t *self = &canvas_data;

    egui_region_t *region = &self->base_view_work_region;

    // in screen coordinate.
    egui_region_intersect(base_region, &self->pfb_region, region);

    // change to base_region coordinate.
    region->location.x -= base_region->location.x;
    region->location.y -= base_region->location.y;

    // calculate pfb_location_in_base_view in base_region coordinate.
    self->pfb_location_in_base_view.x = self->pfb_region.location.x - base_region->location.x;
    self->pfb_location_in_base_view.y = self->pfb_region.location.y - base_region->location.y;
}

void egui_canvas_register_spec_circle_info(uint16_t res_circle_info_count_spec, const egui_circle_info_t *res_circle_info_spec_arr)
{
    egui_canvas_t *self = &canvas_data;

    self->res_circle_info_count_spec = res_circle_info_count_spec;
    self->res_circle_info_spec_arr = res_circle_info_spec_arr;
}

void egui_canvas_init(egui_color_int_t *pfb, egui_region_t *region)
{
    egui_canvas_t *self = &canvas_data;

    self->pfb = pfb;
    egui_region_copy(&self->pfb_region, region);

    self->alpha = EGUI_ALPHA_100;
}
