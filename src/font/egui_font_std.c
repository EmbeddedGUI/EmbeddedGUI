#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_font_std.h"
#include "core/egui_api.h"



#define FONT_ERROR_FONT_SIZE(_height) (_height >> 1)

static egui_font_std_char_descriptor_t g_selected_char_desc;

static int egui_font_std_get_code_index(const egui_font_std_info_t *font, uint32_t utf8_code)
{
    int first = 0;
    int last = font->count;
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
    const void *p_pixer_buffer;
    void *data_buf;
    egui_font_std_char_descriptor_t *p_char_desc = &g_selected_char_desc;
    // uint8_t ext_data_buf[EGUI_FONT_STD_EXT_CHAR_DESC_ITEM_SIZE];
    
    int code_index = egui_font_std_get_code_index(font, utf8_code);
    if (code_index < 0)
    {
        return NULL;
    }

    if(font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        egui_memcpy(p_char_desc, &font->char_array[code_index], sizeof(egui_font_std_char_descriptor_t));
    }
    else
    {
        egui_api_load_external_resource(p_char_desc, (uint32_t)font->char_array, code_index * sizeof(egui_font_std_char_descriptor_t), sizeof(egui_font_std_char_descriptor_t));
        // egui_api_load_external_resource(ext_data_buf, (uint32_t)font->char_array, code_index * EGUI_FONT_STD_EXT_CHAR_DESC_ITEM_SIZE, EGUI_FONT_STD_EXT_CHAR_DESC_ITEM_SIZE);
        // p_char_desc->idx = *(uint32_t *)ext_data_buf;
        // p_char_desc->size = *(uint16_t *)(ext_data_buf + 4);
        // p_char_desc->box_w = *(uint8_t *)(ext_data_buf + 6);
        // p_char_desc->box_h = *(uint8_t *)(ext_data_buf + 7);
        // p_char_desc->adv = *(uint8_t *)(ext_data_buf + 8);
        // p_char_desc->off_x = *(int8_t *)(ext_data_buf + 9);
        // p_char_desc->off_y = *(int8_t *)(ext_data_buf + 10);
        
        // EGUI_LOG_INF("utf8_code: 0x%08x, idx:%d, size:%d, box_w:%d, box_h:%d, adv:%d, off_x:%d, off_y:%d\r\n"
        //              , utf8_code, p_char_desc->idx, p_char_desc->size, p_char_desc->box_w, p_char_desc->box_h, p_char_desc->adv, p_char_desc->off_x, p_char_desc->off_y);
    }

    return p_char_desc;
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
                egui_canvas_draw_point((x + x_), (y + y_), color, egui_alpha_change_table_2[sel_value]);
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
                egui_canvas_draw_point((x + x_), (y + y_), color, egui_alpha_change_table_4[sel_value]);
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
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            const void *p_pixer_buffer;
            void *data_buf = egui_malloc(p_char_desc->size);
            if(data_buf == NULL)
            {
                EGUI_ASSERT(0);
                return 0;
            }
            p_pixer_buffer = data_buf;
            if(font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
            {
                memcpy(data_buf, font->pixel_buffer + p_char_desc->idx, p_char_desc->size);
            }
            else
            {
                egui_api_load_external_resource(data_buf, (uint32_t)(font->pixel_buffer), p_char_desc->idx, p_char_desc->size);
                // EGUI_LOG_INF("load external resource, idx:%d, size:%d, data_buf:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n", p_char_desc->idx, p_char_desc->size
                //     , ((uint8_t *)data_buf)[0], ((uint8_t *)data_buf)[1], ((uint8_t *)data_buf)[2], ((uint8_t *)data_buf)[3], ((uint8_t *)data_buf)[4], ((uint8_t *)data_buf)[5], ((uint8_t *)data_buf)[6], ((uint8_t *)data_buf)[7]);
            }
#else // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            const void *p_pixer_buffer = (const void *)(font->pixel_buffer + p_char_desc->idx);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
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
                draw_func(x + p_char_desc->off_x, y + p_char_desc->off_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer,
                          color, alpha);
            }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            egui_free(data_buf);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            return p_char_desc->adv;
        }
    }

    egui_canvas_draw_rectangle(x, y, FONT_ERROR_FONT_SIZE(font->height) - 2, font->height, 1, color, alpha);
    return FONT_ERROR_FONT_SIZE(font->height);
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
            offset += p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
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
        font_width += p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
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
        font_width += p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
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
