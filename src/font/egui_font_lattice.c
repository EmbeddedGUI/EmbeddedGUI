#include <stdio.h>
#include <assert.h>

#include "egui_font_lattice.h"
#include "core/egui_api.h"

#define FONT_ERROR_LATTICE_SIZE(_height) (_height >> 1)

static const LATTICE *egui_font_lattice_get_font(const LATTICE_FONT_INFO *font, uint32_t utf8_code)
{
    int first = 0;
    int last = font->count - 1;
    int middle = (first + last) / 2;

    while (first <= last)
    {
        if (font->lattice_array[middle].utf8_code < utf8_code)
            first = middle + 1;
        else if (font->lattice_array[middle].utf8_code == utf8_code)
        {
            return &font->lattice_array[middle];
        }
        else
        {
            last = middle - 1;
        }
        middle = (first + last) / 2;
    }
    return 0;
}

static void egui_font_lattice_draw(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                   egui_color_t color, egui_alpha_t alpha)
{
    uint8_t blk_value = *p_data++;
    uint8_t blk_cnt = *p_data++;
    alpha = blk_value;
    for (int y_ = 0; y_ < height; y_++)
    {
        for (int x_ = 0; x_ < width; x_++)
        {
            EGUI_ASSERT(blk_cnt);
            egui_canvas_draw_point((x + x_), (y + y_), color, alpha);
            if (--blk_cnt == 0)
            {
                blk_value = *p_data++;
                blk_cnt = *p_data++;
                alpha = blk_value;
            }
        }
    }
}

static int egui_font_lattice_draw_single_char(const egui_font_t *self, uint32_t utf8_code, egui_dim_t x, egui_dim_t y, egui_color_t color,
                                              egui_alpha_t alpha)
{
    LATTICE_FONT_INFO *font = (LATTICE_FONT_INFO *)self->res;
    if (font)
    {
        const LATTICE *p_lattice = egui_font_lattice_get_font(font, utf8_code);
        if (p_lattice)
        {
            egui_font_lattice_draw(x, y, p_lattice->width, font->height, p_lattice->pixel_buffer, color, alpha);
            return p_lattice->width;
        }
    }

    egui_canvas_draw_rectangle(x, y, FONT_ERROR_LATTICE_SIZE(font->height) - 2, font->height, 1, color, alpha);
    return FONT_ERROR_LATTICE_SIZE(font->height);
}

void egui_font_lattice_draw_string(const egui_font_t *self, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color,
                                   egui_alpha_t alpha)
{
    const char *s = (const char *)string;
    if (0 == s)
    {
        return;
    }

    int offset = 0;
    uint32_t utf8_code;
    while (*s)
    {
        s += egui_font_get_utf8_code(s, &utf8_code);
        offset += egui_font_lattice_draw_single_char(self, utf8_code, (x + offset), y, color, alpha);
    }
}

int egui_font_lattice_get_str_size(const egui_font_t *self, const void *string, egui_dim_t *width, egui_dim_t *height)
{
    const char *s = (const char *)string;
    LATTICE_FONT_INFO *font = (LATTICE_FONT_INFO *)self->res;
    if (NULL == s || NULL == font)
    {
        *width = *height = 0;
        return -1;
    }

    int lattice_width = 0;
    uint32_t utf8_code;
    int utf8_bytes;
    while (*s)
    {
        utf8_bytes = egui_font_get_utf8_code(s, &utf8_code);
        const LATTICE *p_lattice = egui_font_lattice_get_font(font, utf8_code);
        lattice_width += p_lattice ? p_lattice->width : FONT_ERROR_LATTICE_SIZE(font->height);
        s += utf8_bytes;
    }
    *width = lattice_width;
    *height = font->height;
    return 0;
}

const egui_font_api_t egui_font_lattice_t_api_table = {
        .draw_string = egui_font_lattice_draw_string,
        .draw_string_in_rect = egui_font_draw_string_in_rect,
        .get_str_size = egui_font_lattice_get_str_size,
        .get_str_size_with_limit = egui_font_lattice_get_str_size,
        .get_string_pos = egui_font_get_string_pos,
};

void egui_font_lattice_init(egui_font_t *self, const void *res)
{
    egui_font_lattice_t *local = (egui_font_lattice_t *)self;
    // call super init.
    egui_font_init(self, res);

    // update api.
    self->api = &egui_font_lattice_t_api_table;
}
