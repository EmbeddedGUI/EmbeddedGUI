#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_font_std.h"
#include "core/egui_api.h"

#define FONT_ERROR_FONT_SIZE(_height) (_height >> 1)

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static egui_font_std_char_descriptor_t g_selected_char_desc;
#define EGUI_FONT_STD_EXTERNAL_DESC_CACHE_SIZE  2048
#define EGUI_FONT_STD_EXTERNAL_DESC_CACHE_COUNT (EGUI_FONT_STD_EXTERNAL_DESC_CACHE_SIZE / sizeof(egui_font_std_char_descriptor_t))
static const egui_font_std_info_t *g_cached_char_desc_font = NULL;
static egui_font_std_char_descriptor_t g_cached_char_desc_array[EGUI_FONT_STD_EXTERNAL_DESC_CACHE_COUNT];
#define EGUI_FONT_STD_EXTERNAL_PIXEL_CACHE_SIZE 1024
static uint8_t g_selected_char_pixel_data[EGUI_FONT_STD_EXTERNAL_PIXEL_CACHE_SIZE];
#endif

static int egui_font_std_get_code_index(const egui_font_std_info_t *font, uint32_t utf8_code)
{
    int first = 0;
    int last = font->count - 1;
    int middle = (first + last) / 2;

    while (first <= last)
    {
        if (font->code_array[middle].code < utf8_code)
        {
            first = middle + 1;
        }
        else if (font->code_array[middle].code == utf8_code)
        {
            return middle;
        }
        else
        {
            last = middle - 1;
        }
        middle = (first + last) / 2;
    }
    return -1;
}

static const egui_font_std_char_descriptor_t *egui_font_std_get_desc(const egui_font_std_info_t *font, uint32_t utf8_code)
{
    int code_index = egui_font_std_get_code_index(font, utf8_code);
    if (code_index < 0)
    {
        return NULL;
    }

    if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        return &font->char_array[code_index];
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    {
        if (font->count <= EGUI_FONT_STD_EXTERNAL_DESC_CACHE_COUNT)
        {
            if (g_cached_char_desc_font != font)
            {
                egui_api_load_external_resource(g_cached_char_desc_array, (egui_uintptr_t)font->char_array, 0,
                                                font->count * sizeof(egui_font_std_char_descriptor_t));
                g_cached_char_desc_font = font;
            }

            if (g_cached_char_desc_array[code_index].size == 0)
            {
                return NULL;
            }

            return &g_cached_char_desc_array[code_index];
        }

        egui_font_std_char_descriptor_t *p_char_desc = &g_selected_char_desc;

        egui_api_load_external_resource(p_char_desc, (egui_uintptr_t)font->char_array, code_index * sizeof(egui_font_std_char_descriptor_t),
                                        sizeof(egui_font_std_char_descriptor_t));
        if (p_char_desc->size == 0)
        {
            return NULL;
        }

        return p_char_desc;
    }
#else
    {
        return NULL;
    }
#endif
}

__EGUI_STATIC_INLINE__ void egui_font_std_blend_pixel(egui_color_int_t *dst, egui_color_t color, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        *dst = color.full;
    }
    else
    {
        egui_rgb_mix_ptr((egui_color_t *)dst, &color, (egui_color_t *)dst, alpha);
    }
}

static int egui_font_std_can_fast_draw(const egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    const egui_region_t *region = egui_canvas_get_base_view_work_region();

    if (canvas->mask != NULL || width <= 0 || height <= 0)
    {
        return 0;
    }

    if (x < region->location.x || y < region->location.y)
    {
        return 0;
    }

    if (x + width > region->location.x + region->size.width || y + height > region->location.y + region->size.height)
    {
        return 0;
    }

    // Extra safety: verify the PFB index is within bounds
    egui_dim_t pfb_x = x - canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_y = y - canvas->pfb_location_in_base_view.y;
    egui_dim_t pfb_w = canvas->pfb_region.size.width;
    egui_dim_t pfb_h = canvas->pfb_region.size.height;
    if (pfb_x < 0 || pfb_y < 0 || pfb_x + width > pfb_w || pfb_y + height > pfb_h)
    {
        return 0;
    }

    return 1;
}

static void egui_font_std_draw_fast_1(const egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                      egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t pfb_x = x - canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_y = y - canvas->pfb_location_in_base_view.y;
    uint16_t row_stride = (width + 7) >> 3;

    for (egui_dim_t row = 0; row < height; row++)
    {
        const uint8_t *src = p_data + row * row_stride;
        egui_color_int_t *dst = &canvas->pfb[(pfb_y + row) * pfb_width + pfb_x];

        for (egui_dim_t col = 0; col < width; col++)
        {
            if ((src[col >> 3] >> (col & 0x07)) & 0x01)
            {
                egui_font_std_blend_pixel(&dst[col], color, alpha);
            }
        }
    }
}

static void egui_font_std_draw_fast_2(const egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                      egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t pfb_x = x - canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_y = y - canvas->pfb_location_in_base_view.y;
    uint16_t row_stride = (width + 3) >> 2;

    for (egui_dim_t row = 0; row < height; row++)
    {
        const uint8_t *src = p_data + row * row_stride;
        egui_color_int_t *dst = &canvas->pfb[(pfb_y + row) * pfb_width + pfb_x];

        for (egui_dim_t col = 0; col < width; col++)
        {
            uint8_t sel_value = (src[col >> 2] >> ((col & 0x03) << 1)) & 0x03;
            egui_alpha_t pixel_alpha = egui_alpha_change_table_2[sel_value];

            if (pixel_alpha != 0)
            {
                egui_font_std_blend_pixel(&dst[col], color, egui_color_alpha_mix(alpha, pixel_alpha));
            }
        }
    }
}

static void egui_font_std_draw_fast_4(const egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                      egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t pfb_x = x - canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_y = y - canvas->pfb_location_in_base_view.y;
    uint16_t row_stride = (width + 1) >> 1;

    for (egui_dim_t row = 0; row < height; row++)
    {
        const uint8_t *src = p_data + row * row_stride;
        egui_color_int_t *dst = &canvas->pfb[(pfb_y + row) * pfb_width + pfb_x];

        for (egui_dim_t col = 0; col < width; col++)
        {
            uint8_t sel_value = (src[col >> 1] >> ((col & 0x01) << 2)) & 0x0F;
            egui_alpha_t pixel_alpha = egui_alpha_change_table_4[sel_value];

            if (pixel_alpha != 0)
            {
                egui_font_std_blend_pixel(&dst[col], color, egui_color_alpha_mix(alpha, pixel_alpha));
            }
        }
    }
}

static void egui_font_std_draw_fast_8(const egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                      egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t pfb_x = x - canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_y = y - canvas->pfb_location_in_base_view.y;

    for (egui_dim_t row = 0; row < height; row++)
    {
        const uint8_t *src = p_data + row * width;
        egui_color_int_t *dst = &canvas->pfb[(pfb_y + row) * pfb_width + pfb_x];

        for (egui_dim_t col = 0; col < width; col++)
        {
            if (src[col] != 0)
            {
                egui_font_std_blend_pixel(&dst[col], color, egui_color_alpha_mix(alpha, src[col]));
            }
        }
    }
}

static int egui_font_std_draw_fast(const egui_font_std_info_t *font, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                   egui_color_t color, egui_alpha_t alpha)
{
    const egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_alpha_t draw_alpha = egui_color_alpha_mix(canvas->alpha, alpha);

    if (!egui_font_std_can_fast_draw(canvas, x, y, width, height) || draw_alpha == 0)
    {
        return 0;
    }

    switch (font->font_bit_mode)
    {
    case 1:
        egui_font_std_draw_fast_1(canvas, x, y, width, height, p_data, color, draw_alpha);
        return 1;
    case 2:
        egui_font_std_draw_fast_2(canvas, x, y, width, height, p_data, color, draw_alpha);
        return 1;
    case 4:
        egui_font_std_draw_fast_4(canvas, x, y, width, height, p_data, color, draw_alpha);
        return 1;
    case 8:
        egui_font_std_draw_fast_8(canvas, x, y, width, height, p_data, color, draw_alpha);
        return 1;
    default:
        break;
    }

    return 0;
}

// Fast path for font rendering with masks that support row-level color blend
// (e.g., LINEAR_VERTICAL gradient masks). Avoids per-pixel mask_point vtable dispatch.
// Handles clipping to PFB work region, so partially-visible glyphs are covered too.
static int egui_font_std_draw_fast_mask(const egui_font_std_info_t *font, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height,
                                        const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    const egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_mask_t *mask = canvas->mask;

    if (mask == NULL || mask->api->mask_blend_row_color == NULL)
    {
        return 0;
    }

    egui_alpha_t draw_alpha = egui_color_alpha_mix(canvas->alpha, alpha);
    if (draw_alpha == 0)
    {
        return 0;
    }

    if (width <= 0 || height <= 0)
    {
        return 0;
    }

    // Clip glyph rect to PFB work region
    const egui_region_t *region = egui_canvas_get_base_view_work_region();
    egui_dim_t vis_x0 = x;
    egui_dim_t vis_y0 = y;
    egui_dim_t vis_x1 = x + width;
    egui_dim_t vis_y1 = y + height;

    if (vis_x0 < region->location.x)
    {
        vis_x0 = region->location.x;
    }
    if (vis_y0 < region->location.y)
    {
        vis_y0 = region->location.y;
    }
    if (vis_x1 > region->location.x + region->size.width)
    {
        vis_x1 = region->location.x + region->size.width;
    }
    if (vis_y1 > region->location.y + region->size.height)
    {
        vis_y1 = region->location.y + region->size.height;
    }

    if (vis_x0 >= vis_x1 || vis_y0 >= vis_y1)
    {
        return 1; // fully clipped, nothing to draw
    }

    // Glyph data offsets for the visible portion
    egui_dim_t col0 = vis_x0 - x;
    egui_dim_t row0 = vis_y0 - y;
    egui_dim_t col1 = vis_x1 - x;
    egui_dim_t row1 = vis_y1 - y;

    // PFB coordinates for the visible top-left
    egui_dim_t pfb_x0 = vis_x0 - canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_y0 = vis_y0 - canvas->pfb_location_in_base_view.y;
    egui_dim_t pfb_w = canvas->pfb_region.size.width;

    // Probe: check if mask supports row-level blend for this configuration
    egui_color_t probe = color;
    if (!mask->api->mask_blend_row_color(mask, vis_y0, &probe))
    {
        return 0;
    }

    switch (font->font_bit_mode)
    {
    case 1:
    {
        uint16_t row_stride = (width + 7) >> 3;
        for (egui_dim_t row = row0; row < row1; row++)
        {
            egui_color_t row_color = color;
            mask->api->mask_blend_row_color(mask, y + row, &row_color);
            const uint8_t *src = p_data + row * row_stride;
            egui_color_int_t *dst = &canvas->pfb[(pfb_y0 + row - row0) * pfb_w + pfb_x0];
            for (egui_dim_t col = col0; col < col1; col++)
            {
                if (src[col >> 3] & (1 << (col & 7)))
                {
                    egui_font_std_blend_pixel(&dst[col - col0], row_color, draw_alpha);
                }
            }
        }
        return 1;
    }
    case 2:
    {
        uint16_t row_stride = (width + 3) >> 2;
        for (egui_dim_t row = row0; row < row1; row++)
        {
            egui_color_t row_color = color;
            mask->api->mask_blend_row_color(mask, y + row, &row_color);
            const uint8_t *src = p_data + row * row_stride;
            egui_color_int_t *dst = &canvas->pfb[(pfb_y0 + row - row0) * pfb_w + pfb_x0];
            for (egui_dim_t col = col0; col < col1; col++)
            {
                uint8_t sel_value = (src[col >> 2] >> ((col & 0x03) << 1)) & 0x03;
                egui_alpha_t pixel_alpha = egui_alpha_change_table_2[sel_value];
                if (pixel_alpha != 0)
                {
                    egui_font_std_blend_pixel(&dst[col - col0], row_color, egui_color_alpha_mix(draw_alpha, pixel_alpha));
                }
            }
        }
        return 1;
    }
    case 4:
    {
        uint16_t row_stride = (width + 1) >> 1;
        for (egui_dim_t row = row0; row < row1; row++)
        {
            egui_color_t row_color = color;
            mask->api->mask_blend_row_color(mask, y + row, &row_color);
            const uint8_t *src = p_data + row * row_stride;
            egui_color_int_t *dst = &canvas->pfb[(pfb_y0 + row - row0) * pfb_w + pfb_x0];
            for (egui_dim_t col = col0; col < col1; col++)
            {
                uint8_t sel_value = (src[col >> 1] >> ((col & 0x01) << 2)) & 0x0F;
                egui_alpha_t pixel_alpha = egui_alpha_change_table_4[sel_value];
                if (pixel_alpha != 0)
                {
                    egui_font_std_blend_pixel(&dst[col - col0], row_color, egui_color_alpha_mix(draw_alpha, pixel_alpha));
                }
            }
        }
        return 1;
    }
    case 8:
    {
        for (egui_dim_t row = row0; row < row1; row++)
        {
            egui_color_t row_color = color;
            mask->api->mask_blend_row_color(mask, y + row, &row_color);
            const uint8_t *src = p_data + row * width;
            egui_color_int_t *dst = &canvas->pfb[(pfb_y0 + row - row0) * pfb_w + pfb_x0];
            for (egui_dim_t col = col0; col < col1; col++)
            {
                if (src[col] != 0)
                {
                    egui_font_std_blend_pixel(&dst[col - col0], row_color, egui_color_alpha_mix(draw_alpha, src[col]));
                }
            }
        }
        return 1;
    }
    default:
        return 0;
    }
}

static void egui_font_std_draw_1(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    uint8_t bit_pos;
    uint8_t sel_data;
    for (egui_dim_t y_ = 0; y_ < height; y_++)
    {
        bit_pos = 0;
        for (egui_dim_t x_ = 0; x_ < width; x_++)
        {
            // update bytes position.
            if (bit_pos == 0)
            {
                sel_data = *p_data++;
            }

            if (sel_data & (1 << bit_pos))
            {
                egui_canvas_draw_point((x + x_), (y + y_), color, alpha);
            }

            // update bit position.
            bit_pos++;
            if (bit_pos == 8)
            {
                bit_pos = 0;
            }
        }
    }
}

static void egui_font_std_draw_2(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    uint8_t bit_pos;
    uint8_t sel_data;
    uint8_t sel_value;
    for (egui_dim_t y_ = 0; y_ < height; y_++)
    {
        bit_pos = 0;
        for (egui_dim_t x_ = 0; x_ < width; x_++)
        {
            // update bytes position.
            if (bit_pos == 0)
            {
                sel_data = *p_data++;
            }

            sel_value = (sel_data >> bit_pos) & 0x03;
            if (sel_value)
            {
                if (alpha == EGUI_ALPHA_100)
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, egui_alpha_change_table_2[sel_value]);
                }
                else
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, egui_alpha_change_table_2[sel_value] * alpha / EGUI_ALPHA_100);
                }
            }

            // update bit position.
            bit_pos += 2;
            if (bit_pos == 8)
            {
                bit_pos = 0;
            }
        }
    }
}

static void egui_font_std_draw_4(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    uint8_t bit_pos;
    uint8_t sel_data;
    uint8_t sel_value;
    for (egui_dim_t y_ = 0; y_ < height; y_++)
    {
        bit_pos = 0;
        for (egui_dim_t x_ = 0; x_ < width; x_++)
        {
            // update bytes position.
            if (bit_pos == 0)
            {
                sel_data = *p_data++;
            }

            sel_value = (sel_data >> bit_pos) & 0x0F;
            if (sel_value)
            {
                if (alpha == EGUI_ALPHA_100)
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, egui_alpha_change_table_4[sel_value]);
                }
                else
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, egui_alpha_change_table_4[sel_value] * alpha / EGUI_ALPHA_100);
                }
            }

            // update bit position.
            bit_pos += 4;
            if (bit_pos == 8)
            {
                bit_pos = 0;
            }
        }
    }
}

static void egui_font_std_draw_8(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    uint16_t sel_value;
    for (egui_dim_t y_ = 0; y_ < height; y_++)
    {
        for (egui_dim_t x_ = 0; x_ < width; x_++)
        {
            sel_value = *p_data++;

            if (sel_value)
            {
                if (alpha == EGUI_ALPHA_100)
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, sel_value);
                }
                else
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, sel_value * alpha / EGUI_ALPHA_100);
                }
            }
        }
    }
}

typedef void(egui_font_std_draw)(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color,
                                 egui_alpha_t alpha);
static int egui_font_std_draw_single_char(const egui_font_t *self, uint32_t utf8_code, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_font_std_info_t *font = (egui_font_std_info_t *)self->res;
    if (font)
    {
        const egui_font_std_char_descriptor_t *p_char_desc = egui_font_std_get_desc(font, utf8_code);

        if (p_char_desc)
        {
            const uint8_t *p_pixer_buffer = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            void *data_buf = NULL;
#endif

            if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
            {
                p_pixer_buffer = font->pixel_buffer + p_char_desc->idx;
            }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            else
            {
                if (p_char_desc->size <= EGUI_FONT_STD_EXTERNAL_PIXEL_CACHE_SIZE)
                {
                    data_buf = g_selected_char_pixel_data;
                }
                else
                {
                    data_buf = egui_malloc(p_char_desc->size);
                    if (data_buf == NULL)
                    {
                        EGUI_ASSERT(0);
                        return 0;
                    }
                }

                egui_api_load_external_resource(data_buf, (egui_uintptr_t)(font->pixel_buffer), p_char_desc->idx, p_char_desc->size);
                p_pixer_buffer = data_buf;
            }
#endif
            egui_font_std_draw *draw_func = NULL;
            switch (font->font_bit_mode)
            {
            case 1:
                draw_func = egui_font_std_draw_1;
                break;
            case 2:
                draw_func = egui_font_std_draw_2;
                break;
            case 4:
                draw_func = egui_font_std_draw_4;
                break;
            case 8:
                draw_func = egui_font_std_draw_8;
                break;
            default:
                EGUI_ASSERT(0);
                break;
            }
            if (draw_func)
            {
                egui_dim_t draw_x = x + p_char_desc->off_x;
                egui_dim_t draw_y = y + p_char_desc->off_y;

                if (!egui_font_std_draw_fast(font, draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha))
                {
                    if (!egui_font_std_draw_fast_mask(font, draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha))
                    {
                        draw_func(draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha);
                    }
                }
            }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            if (data_buf != NULL && data_buf != g_selected_char_pixel_data)
            {
                egui_free(data_buf);
            }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            return p_char_desc->adv;
        }
    }

    egui_canvas_draw_rectangle(x, y, FONT_ERROR_FONT_SIZE(font->height) - 2, font->height, 1, color, alpha);
    return FONT_ERROR_FONT_SIZE(font->height);
}

int egui_font_std_draw_string(const egui_font_t *self, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    const char *s = (const char *)string;
    int str_cnt = 0;
    if (0 == s)
    {
        return 0;
    }

    int offset = x;
    uint32_t utf8_code;
    int char_bytes;

    egui_font_std_info_t *font = (egui_font_std_info_t *)self->res;
    egui_region_t *base_region = egui_canvas_get_base_view_work_region();
    // check if the string is in the canvas region.
    if (y > (base_region->location.y + base_region->size.height) || (y + font->height) < base_region->location.y)
    {
        const char *next_line = strchr(s, '\n');
        if (next_line)
        {
            str_cnt += (next_line - s) + 1;
        }
        else
        {
            str_cnt += strlen(s);
        }
        return str_cnt;
    }

    while ((*s != '\0'))
    {
        if (*s == '\r')
        {
            s++;
            str_cnt++;
            continue;
        }
        else if (*s == '\n')
        {
            str_cnt++;
            break;
        }
        char_bytes = egui_font_get_utf8_code(s, &utf8_code);
        s += char_bytes;
        str_cnt += char_bytes;

        // printf("draw char: %c, offset(%d)\n", utf8_code, offset);
        // only check x-axis intersection, since y-axis is fixed.
        if (offset > base_region->location.x + base_region->size.width)
        {
            const char *next_line = strchr(s, '\n');
            if (next_line)
            {
                str_cnt += (next_line - s) + 1;
            }
            else
            {
                str_cnt += strlen(s);
            }
            break;
        }
        else if ((offset + font->height) < base_region->location.x) // max font width is font->height
        {
            const egui_font_std_char_descriptor_t *p_char_desc = egui_font_std_get_desc(font, utf8_code);
            offset += p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
        }
        else
        {
            offset += egui_font_std_draw_single_char(self, utf8_code, offset, y, color, alpha);
        }
    }

    return str_cnt;
}

int egui_font_std_get_str_size(const egui_font_t *self, const void *string, uint8_t is_multi_line, egui_dim_t line_space, egui_dim_t *width, egui_dim_t *height)
{
    const char *s = (const char *)string;
    egui_font_std_info_t *font = (egui_font_std_info_t *)self->res;
    if (NULL == s || NULL == font)
    {
        *width = *height = 0;
        return -1;
    }

    int font_width = 0;
    int font_height = font->height;
    int font_max_width = 0;
    uint32_t utf8_code;
    int utf8_bytes;
    const egui_font_std_char_descriptor_t *p_char_desc;

    int is_line_full = 0;
    while ((*s != '\0'))
    {
        if (*s == '\r')
        {
            s++;
            continue;
        }
        else if (*s == '\n')
        {
            if (is_multi_line == 0)
            {
                break;
            }
            // update max width.
            if (font_max_width < font_width)
            {
                font_max_width = font_width;
            }
            font_height += font->height + line_space;
            font_width = 0;

            is_line_full = 0;
        }

        utf8_bytes = egui_font_get_utf8_code(s, &utf8_code);
        s += utf8_bytes;

        // caculate font width.
        if (!is_line_full)
        {
            p_char_desc = egui_font_std_get_desc(font, utf8_code);
            font_width += p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);

            if (*width != 0)
            {
                if (font_width > *width)
                {
                    font_width = *width;
                    is_line_full = 1;
                    continue;
                }
            }
        }
    }

    // update max width.
    if (font_max_width < font_width)
    {
        font_max_width = font_width;
    }

    *width = font_max_width;
    *height = font_height;
    return 0;
}

const egui_font_api_t egui_font_std_t_api_table = {
        .draw_string = egui_font_std_draw_string,
        .get_str_size = egui_font_std_get_str_size,
};

void egui_font_std_init(egui_font_t *self, const void *res)
{
    EGUI_LOCAL_INIT(egui_font_std_t);
    // call super init.
    egui_font_init(self, res);

    // update api.
    self->api = &egui_font_std_t_api_table;
}
