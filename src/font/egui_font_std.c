#include <stdio.h>
#include <assert.h>

#include "egui_font_std.h"
#include "core/egui_api.h"

#define FONT_ERROR_LATTICE_SIZE(_height) (_height >> 1)

#define EGUI_FONT_ALPHA_VALUE(_val, _bit_size) ((255 * (_val)) / ((1 << (_bit_size)) - 1))

#define EGUI_FONT_ALPHA_VALUE_2(_val) EGUI_FONT_ALPHA_VALUE(_val, 2)
// we don't use multiple alpha value, so we don't need to calculate it.
const uint8_t egui_font_alpha_change_table_2[] = {
        EGUI_FONT_ALPHA_VALUE_2(0x00),
        EGUI_FONT_ALPHA_VALUE_2(0x01),
        EGUI_FONT_ALPHA_VALUE_2(0x02),
        EGUI_FONT_ALPHA_VALUE_2(0x03),
};

#define EGUI_FONT_ALPHA_VALUE_4(_val) EGUI_FONT_ALPHA_VALUE(_val, 4)
// we don't use multiple alpha value, so we don't need to calculate it.
const uint8_t egui_font_alpha_change_table_4[] = {
        EGUI_FONT_ALPHA_VALUE_4(0x00), EGUI_FONT_ALPHA_VALUE_4(0x01), EGUI_FONT_ALPHA_VALUE_4(0x02), EGUI_FONT_ALPHA_VALUE_4(0x03),
        EGUI_FONT_ALPHA_VALUE_4(0x04), EGUI_FONT_ALPHA_VALUE_4(0x05), EGUI_FONT_ALPHA_VALUE_4(0x06), EGUI_FONT_ALPHA_VALUE_4(0x07),
        EGUI_FONT_ALPHA_VALUE_4(0x08), EGUI_FONT_ALPHA_VALUE_4(0x09), EGUI_FONT_ALPHA_VALUE_4(0x0a), EGUI_FONT_ALPHA_VALUE_4(0x0b),
        EGUI_FONT_ALPHA_VALUE_4(0x0c), EGUI_FONT_ALPHA_VALUE_4(0x0d), EGUI_FONT_ALPHA_VALUE_4(0x0e), EGUI_FONT_ALPHA_VALUE_4(0x0f),
};

static const egui_font_std_char_descriptor_t *egui_font_std_get_desc(const egui_font_std_info_t *font, uint32_t utf8_code)
{
    int first = 0;
    int last = font->count;
    int middle = (first + last) / 2;

    while (first <= last)
    {
        if (font->char_array[middle].code < utf8_code)
        {
            first = middle + 1;
        }
        else if (font->char_array[middle].code == utf8_code)
        {
            return &font->char_array[middle];
        }
        else
        {
            last = middle - 1;
        }
        middle = (first + last) / 2;
    }
    return 0;
}

static void egui_font_std_draw_1(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                 egui_color_t color, egui_alpha_t alpha)
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

static void egui_font_std_draw_2(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                 egui_color_t color, egui_alpha_t alpha)
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
                egui_canvas_draw_point((x + x_), (y + y_), color, egui_font_alpha_change_table_2[sel_value]);
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

static void egui_font_std_draw_4(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                 egui_color_t color, egui_alpha_t alpha)
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
                egui_canvas_draw_point((x + x_), (y + y_), color, egui_font_alpha_change_table_4[sel_value]);
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

static void egui_font_std_draw_8(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                 egui_color_t color, egui_alpha_t alpha)
{
    uint16_t sel_value;
    for (egui_dim_t y_ = 0; y_ < height; y_++)
    {
        for (egui_dim_t x_ = 0; x_ < width; x_++)
        {
            sel_value = *p_data++;

            if (sel_value)
            {
                egui_canvas_draw_point((x + x_), (y + y_), color, sel_value);
            }
        }
    }
}

typedef void(egui_font_std_draw)(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                 egui_color_t color, egui_alpha_t alpha);
static int egui_font_std_draw_single_char(const egui_font_t *self, uint32_t utf8_code, egui_dim_t x, egui_dim_t y, egui_color_t color,
                                          egui_alpha_t alpha)
{
    egui_font_std_info_t *font = (egui_font_std_info_t *)self->res;
    if (font)
    {
        const egui_font_std_char_descriptor_t *p_char_desc = egui_font_std_get_desc(font, utf8_code);
        if (p_char_desc)
        {
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
                draw_func(x + p_char_desc->off_x, y + p_char_desc->off_y, p_char_desc->box_w, p_char_desc->box_h, font->pixel_buffer + p_char_desc->idx,
                          color, alpha);
            }

            return p_char_desc->adv;
        }
    }

    egui_canvas_draw_rectangle(x, y, FONT_ERROR_LATTICE_SIZE(font->height) - 2, font->height, 1, color, alpha);
    return FONT_ERROR_LATTICE_SIZE(font->height);
}

void egui_font_std_draw_string(const egui_font_t *self, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    const char *s = (const char *)string;
    if (0 == s)
    {
        return;
    }

    int offset = x;
    uint32_t utf8_code;

    egui_font_std_info_t *font = (egui_font_std_info_t *)self->res;
    egui_region_t* base_region = egui_canvas_get_base_view_work_region();
    // check if the string is in the canvas region.
    if (y > (base_region->location.y + base_region->size.height) ||
        (y + font->height) < base_region->location.y)
    {
        return;
    }

    // printf("draw string: region(%d, %d, %d, %d)\n", base_region->location.x, base_region->location.y,
    // base_region->size.width, base_region->size.height);
    while (*s)
    {
        s += egui_font_get_utf8_code(s, &utf8_code);

        // printf("draw char: %c, offset(%d)\n", utf8_code, offset);
        // only check x-axis intersection, since y-axis is fixed.
        if (offset > base_region->location.x + base_region->size.width)
        {
            break;
        }
        else if ((offset + font->height) < base_region->location.x) // max font width is font->height
        {
            const egui_font_std_char_descriptor_t *p_char_desc = egui_font_std_get_desc(font, utf8_code);
            offset += p_char_desc ? p_char_desc->adv : FONT_ERROR_LATTICE_SIZE(font->height);
        }
        else
        {
            offset += egui_font_std_draw_single_char(self, utf8_code, offset, y, color, alpha);
        }
    }
}

int egui_font_std_get_str_size(const egui_font_t *self, const void *string, egui_dim_t *width, egui_dim_t *height)
{
    const char *s = (const char *)string;
    egui_font_std_info_t *font = (egui_font_std_info_t *)self->res;
    if (NULL == s || NULL == font)
    {
        *width = *height = 0;
        return -1;
    }

    int font_width = 0;
    uint32_t utf8_code;
    int utf8_bytes;
    const egui_font_std_char_descriptor_t *p_char_desc;
    while (*s)
    {
        utf8_bytes = egui_font_get_utf8_code(s, &utf8_code);
        p_char_desc = egui_font_std_get_desc(font, utf8_code);
        font_width += p_char_desc ? p_char_desc->adv : FONT_ERROR_LATTICE_SIZE(font->height);
        s += utf8_bytes;
    }
    *width = font_width;
    *height = font->height;
    return 0;
}

int egui_font_std_get_str_size_with_limit(const egui_font_t *self, const void *string, egui_dim_t *width, egui_dim_t *height)
{
    const char *s = (const char *)string;
    egui_font_std_info_t *font = (egui_font_std_info_t *)self->res;
    if (NULL == s || NULL == font)
    {
        *width = *height = 0;
        return -1;
    }

    int font_width = 0;
    uint32_t utf8_code;
    int utf8_bytes;
    const egui_font_std_char_descriptor_t *p_char_desc;
    while (*s)
    {
        utf8_bytes = egui_font_get_utf8_code(s, &utf8_code);
        p_char_desc = egui_font_std_get_desc(font, utf8_code);
        font_width += p_char_desc ? p_char_desc->adv : FONT_ERROR_LATTICE_SIZE(font->height);
        s += utf8_bytes;

        if (font_width >= *width)
        {
            font_width = *width;
            break;
        }
    }
    *width = font_width;
    *height = font->height;
    return 0;
}

const egui_font_api_t egui_font_std_t_api_table = {
        .draw_string = egui_font_std_draw_string,
        .draw_string_in_rect = egui_font_draw_string_in_rect,
        .get_str_size = egui_font_std_get_str_size,
        .get_str_size_with_limit = egui_font_std_get_str_size_with_limit,
        .get_string_pos = egui_font_get_string_pos,
};

void egui_font_std_init(egui_font_t *self, const void *res)
{
    egui_font_std_t *local = (egui_font_std_t *)self;
    // call super init.
    egui_font_init(self, res);

    // update api.
    self->api = &egui_font_std_t_api_table;
}
