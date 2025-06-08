#include <stdio.h>
#include <assert.h>

#include "egui_font.h"
#include "core/egui_api.h"

static const uint8_t s_utf8_length_table[256] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                                 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1};
/**
 * @brief Get the length of a utf8 character.
 * @param s The utf8 character.
 * @param output_utf8_code The output utf8 code.
 * @return The length of the utf8 character.
 */
int egui_font_get_utf8_code(const char *s, uint32_t *output_utf8_code)
{

    uint8_t *us = (uint8_t *)s;
    int utf8_bytes = s_utf8_length_table[*us];
    switch (utf8_bytes)
    {
    case 1:
        *output_utf8_code = *us;
        break;
    case 2:
        *output_utf8_code = (*us << 8) | (*(us + 1));
        break;
    case 3:
        *output_utf8_code = (*us << 16) | ((*(us + 1)) << 8) | *(us + 2);
        break;
    case 4:
        *output_utf8_code = (*us << 24) | ((*(us + 1)) << 16) | (*(us + 2) << 8) | *(us + 3);
        break;
    default:
        EGUI_ASSERT(false);
        break;
    }
    return utf8_bytes;
}

void egui_font_get_string_pos(const egui_font_t *self, const void *string, egui_region_t *rect, uint8_t align_type, uint8_t is_multi_line, egui_dim_t line_space, egui_dim_t *x, egui_dim_t *y)
{
    egui_dim_t height = rect->size.height;
    egui_dim_t width = rect->size.width;
    egui_dim_t x_size = width, y_size = height;
    // get string size.
    self->api->get_str_size(self, string, is_multi_line, line_space, &x_size, &y_size);
    *x = *y = 0;
    switch (align_type & EGUI_ALIGN_HMASK)
    {
    case EGUI_ALIGN_HCENTER:
        if (width > x_size)
        {
            *x = (width - x_size) >> 1;
        }
        break;
    case EGUI_ALIGN_LEFT:
        *x = 0;
        break;
    case EGUI_ALIGN_RIGHT:
        if (width > x_size)
        {
            *x = width - x_size;
        }
        break;
    default:
        break;
    }
    switch (align_type & EGUI_ALIGN_VMASK)
    {
    case EGUI_ALIGN_VCENTER:
        if (height > y_size)
        {
            *y = (height - y_size) >> 1;
        }
        break;
    case EGUI_ALIGN_TOP:
        *y = 0;
        break;
    case EGUI_ALIGN_BOTTOM:
        if (height > y_size)
        {
            *y = height - y_size;
        }
        break;
    default:
        break;
    }
}

void egui_font_draw_string_in_rect(const egui_font_t *self, const void *string, egui_region_t *rect, uint8_t align_type, egui_dim_t line_space, egui_color_t color,
                                   egui_alpha_t alpha)
{
    const char *s = (const char *)string;
    if (NULL == s)
    {
        return;
    }
    egui_dim_t x, y;
    egui_dim_t start_y;
    int str_bytes;
    egui_dim_t x_size, y_size;
    egui_region_t tmp_rect = *rect;

    // Get all text start position.
    egui_font_get_string_pos(self, s, rect, align_type, 1, 0, &x, &y);
    // Get string height.
    self->api->get_str_size(self, "t", 0, 0, &x_size, &y_size);

    align_type &= ~EGUI_ALIGN_VMASK;
    start_y = y;

    while (*s)
    {
        tmp_rect.location.y = start_y;
        egui_font_get_string_pos(self, s, rect, align_type, 0, line_space, &x, &y);

        str_bytes = self->api->draw_string(self, s, tmp_rect.location.x + x, tmp_rect.location.y + y, color, alpha);

        s += str_bytes;
        start_y += y_size + line_space;
    }

}


int egui_font_draw_string(const egui_font_t *self, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    // implement is sub-class.
    return 0;
}

int egui_font_get_str_size(const egui_font_t *self, const void *string, uint8_t is_multi_line, egui_dim_t line_space, egui_dim_t *width, egui_dim_t *height)
{
    // implement is sub-class.
    return 0;
}

const egui_font_api_t egui_font_t_api_table = {
        .draw_string = egui_font_draw_string,
        .get_str_size = egui_font_get_str_size,
};

void egui_font_init(egui_font_t *self, const void *res)
{
    self->res = res;

    // update api.
    self->api = &egui_font_t_api_table;
}
