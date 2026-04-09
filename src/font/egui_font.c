#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_font.h"
#include "core/egui_api.h"
#include "font/egui_font_std.h"

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
        *output_utf8_code = *us;
        utf8_bytes = 1;
        break;
    }
    return utf8_bytes;
}

void egui_font_get_string_pos(const egui_font_t *self, const void *string, egui_region_t *rect, uint8_t align_type, uint8_t is_multi_line,
                              egui_dim_t line_space, egui_dim_t *x, egui_dim_t *y)
{
    egui_dim_t height = rect->size.height;
    egui_dim_t width = rect->size.width;
    egui_dim_t x_size = width, y_size = height;

    *x = *y = 0;

    // Only calculate string size if needed for alignment
    if (((align_type & EGUI_ALIGN_HMASK) != EGUI_ALIGN_LEFT && (align_type & EGUI_ALIGN_HMASK) != 0) ||
        ((align_type & EGUI_ALIGN_VMASK) != EGUI_ALIGN_TOP && (align_type & EGUI_ALIGN_VMASK) != 0))
    {
        self->api->get_str_size(self, string, is_multi_line, line_space, &x_size, &y_size);
    }

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

static egui_dim_t egui_font_get_line_height(const egui_font_t *self)
{
    egui_dim_t x_size;
    egui_dim_t y_size;

    if (egui_font_std_try_get_line_height(self, &y_size))
    {
        return y_size;
    }

    self->api->get_str_size(self, "t", 0, 0, &x_size, &y_size);
    return y_size;
}

void egui_font_draw_string_in_rect(const egui_font_t *self, const void *string, egui_region_t *rect, uint8_t align_type, egui_dim_t line_space,
                                   egui_color_t color, egui_alpha_t alpha)
{
    const char *s = (const char *)string;
    if (NULL == s)
    {
        return;
    }

    egui_dim_t x, y;
    int str_bytes;
    egui_dim_t y_size;
    egui_region_t tmp_rect = *rect;
    egui_dim_t draw_y;
    uint8_t h_align = align_type & EGUI_ALIGN_HMASK;
    uint8_t v_align = align_type & EGUI_ALIGN_VMASK;
    const egui_region_t *work_region = egui_canvas_get_base_view_work_region();
    egui_dim_t work_y0 = work_region->location.y;
    egui_dim_t work_y1 = work_y0 + work_region->size.height;
    egui_dim_t line_step;

    y_size = egui_font_get_line_height(self);
    line_step = y_size + line_space;

    if ((h_align == 0 || h_align == EGUI_ALIGN_LEFT) && (v_align == 0 || v_align == EGUI_ALIGN_TOP))
    {
        draw_y = tmp_rect.location.y;
    }
    else
    {
        egui_font_get_string_pos(self, s, rect, align_type, 1, line_space, &x, &y);
        draw_y = tmp_rect.location.y + y;
    }

    if (h_align == 0 || h_align == EGUI_ALIGN_LEFT)
    {
        tmp_rect.location.y = draw_y;

        if (line_step > 0)
        {
            while (*s && draw_y + y_size <= work_y0)
            {
                const char *next_line = strchr(s, '\n');

                if (next_line == NULL)
                {
                    return;
                }

                s = next_line + 1;
                draw_y += line_step;
            }
        }

        while (*s)
        {
            if (line_step > 0 && draw_y >= work_y1)
            {
                break;
            }

            str_bytes = self->api->draw_string(self, s, tmp_rect.location.x, draw_y, color, alpha);
            if (str_bytes <= 0)
            {
                break;
            }

            s += str_bytes;
            draw_y += y_size + line_space;
        }
        return;
    }

    tmp_rect.location.y = draw_y;
    align_type &= ~EGUI_ALIGN_VMASK;

    while (*s)
    {
        egui_font_get_string_pos(self, s, rect, align_type, 0, line_space, &x, &y);

        str_bytes = self->api->draw_string(self, s, tmp_rect.location.x + x, tmp_rect.location.y + y, color, alpha);

        s += str_bytes;
        tmp_rect.location.y += y_size + line_space;

        // EGUI_LOG_INF("egui_font_draw_string_in_rect. string:%s, rect:%d,%d,%d,%d, x:%d, y:%d, str_bytes:%d\n"
        //     , string, tmp_rect.location.x, tmp_rect.location.y, tmp_rect.size.width, tmp_rect.size.height, x, y, str_bytes);
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
