#include "egui_view_ring_text_basic.h"
#include "core/egui_common.h"
#include "resource/egui_resource.h"

static const egui_font_t *egui_view_ring_text_get_default_font(void)
{
    return (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
}

static uint8_t egui_view_ring_text_measure_default_font(const char *text, egui_dim_t *out_width, egui_dim_t *out_height)
{
    const egui_font_t *font = egui_view_ring_text_get_default_font();

    if (font == NULL || text == NULL || text[0] == '\0' || font->api == NULL || font->api->get_str_size == NULL)
    {
        return 0;
    }

    font->api->get_str_size(font, text, 0, 0, out_width, out_height);
    return 1;
}

static egui_dim_t egui_view_ring_text_get_default_box_height(egui_dim_t inner_r)
{
    if (inner_r >= 40)
    {
        return 18;
    }
    if (inner_r >= 28)
    {
        return 16;
    }
    if (inner_r >= 18)
    {
        return 12;
    }
    return 0;
}

void egui_view_ring_text_format_value(uint8_t value, uint8_t append_percent, char *buf, uint32_t buf_size)
{
    uint32_t len = 0;
    uint32_t min_size = append_percent ? 3U : 2U;

    if (buf == NULL || buf_size < min_size)
    {
        return;
    }

    if (value >= 100)
    {
        if (buf_size < 4)
        {
            return;
        }
        buf[len++] = '1';
        buf[len++] = '0';
        buf[len++] = '0';
    }
    else if (value >= 10)
    {
        buf[len++] = (char)('0' + value / 10);
        buf[len++] = (char)('0' + value % 10);
    }
    else
    {
        buf[len++] = (char)('0' + value);
    }

    if (append_percent)
    {
        if (len + 1 >= buf_size)
        {
            buf[0] = '\0';
            return;
        }
        buf[len++] = '%';
    }

    buf[len] = '\0';
}

static uint8_t egui_view_ring_text_get_box_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t inner_r, egui_dim_t top_offset, uint8_t center_box,
                                                 const char *text, egui_region_t *text_box)
{
    egui_dim_t box_width;
    egui_dim_t box_height;

    if (text_box == NULL || text == NULL || text[0] == '\0')
    {
        return 0;
    }

    box_width = inner_r * 2 - 4;
    if (box_width <= 0)
    {
        return 0;
    }

    box_height = egui_view_ring_text_get_default_box_height(inner_r);
    if (box_height <= 0)
    {
        return 0;
    }

    text_box->location.x = center_x - box_width / 2;
    text_box->location.y = center_box ? (center_y - box_height / 2) : (center_y + top_offset);
    text_box->size.width = box_width;
    text_box->size.height = box_height;

    return egui_region_is_empty(text_box) ? 0 : 1;
}

uint8_t egui_view_ring_text_get_region_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t inner_r, egui_dim_t top_offset, uint8_t center_box,
                                             uint8_t value, uint8_t append_percent, egui_region_t *text_region)
{
    egui_region_t text_box;
    egui_dim_t text_w;
    egui_dim_t text_h;
    egui_dim_t offset_x;
    egui_dim_t offset_y;
    char text_buf[8];

    if (text_region == NULL)
    {
        return 0;
    }

    egui_view_ring_text_format_value(value, append_percent, text_buf, sizeof(text_buf));
    if (!egui_view_ring_text_get_box_basic(center_x, center_y, inner_r, top_offset, center_box, text_buf, &text_box))
    {
        return 0;
    }

    if (!egui_view_ring_text_measure_default_font(text_buf, &text_w, &text_h) || text_w <= 0 || text_h <= 0)
    {
        egui_region_copy(text_region, &text_box);
        return 1;
    }

    text_w = EGUI_MIN(text_w, text_box.size.width);
    text_h = EGUI_MIN(text_h, text_box.size.height);
    egui_common_align_get_x_y(text_box.size.width, text_box.size.height, text_w, text_h, EGUI_ALIGN_CENTER, &offset_x, &offset_y);

    text_region->location.x = text_box.location.x + offset_x;
    text_region->location.y = text_box.location.y + offset_y;
    text_region->size.width = text_w;
    text_region->size.height = text_h;

    if (text_region->location.x > text_box.location.x)
    {
        text_region->location.x--;
        text_region->size.width++;
    }
    if (text_region->location.y > text_box.location.y)
    {
        text_region->location.y--;
        text_region->size.height++;
    }
    if (text_region->location.x + text_region->size.width < text_box.location.x + text_box.size.width)
    {
        text_region->size.width++;
    }
    if (text_region->location.y + text_region->size.height < text_box.location.y + text_box.size.height)
    {
        text_region->size.height++;
    }

    egui_region_intersect(text_region, &text_box, text_region);
    return egui_region_is_empty(text_region) ? 0 : 1;
}

void egui_view_ring_text_draw_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t inner_r, egui_dim_t top_offset, uint8_t center_box, uint8_t value,
                                    uint8_t append_percent, egui_color_t color, egui_alpha_t alpha)
{
    char text_buf[8];
    egui_region_t text_rect;
    const egui_font_t *font = egui_view_ring_text_get_default_font();

    egui_view_ring_text_format_value(value, append_percent, text_buf, sizeof(text_buf));
    if (font != NULL && egui_view_ring_text_get_box_basic(center_x, center_y, inner_r, top_offset, center_box, text_buf, &text_rect))
    {
        egui_canvas_draw_text_in_rect(font, text_buf, &text_rect, EGUI_ALIGN_CENTER, color, alpha);
    }
}
