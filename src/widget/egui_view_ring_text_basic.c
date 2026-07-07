#include "egui_view_ring_text_basic.h"
#include "core/egui_core.h"
#include "core/egui_common.h"
#include "font/egui_font.h"
#include "resource/egui_resource.h"

/**
 * @file egui_view_ring_text_basic.c
 * @brief Default text-placement helpers shared by ring-shaped widgets.
 *
 * The helpers keep formatting, box sizing, drawing, and dirty-region logic in
 * one place so multiple widgets can stay visually consistent.
 * They assume a simple single-line numeric label rendered either in the inner
 * hole of the ring or just below it.
 */

/**
 * @brief Resolve the default font used by the stock ring text renderer.
 */
static const egui_font_t *egui_view_ring_text_get_default_font(void)
{
    return (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
}

/**
 * @brief Measure text using the shared default font when available.
 */
static uint8_t egui_view_ring_text_measure_default_font(egui_core_t *core, const char *text, egui_dim_t *out_width, egui_dim_t *out_height)
{
    const egui_font_t *font = egui_view_ring_text_get_default_font();

    if (font == NULL || text == NULL || text[0] == '\0' || font->api == NULL || font->api->get_str_size == NULL)
    {
        return 0;
    }

    // The helper intentionally uses one shared default font so all stock ring labels measure the same way.
    egui_font_get_str_size_with_core(font, core, text, 0, 0, out_width, out_height);
    return 1;
}

/**
 * @brief Choose a stock text-box height based on the available inner-hole radius.
 *
 * The returned height is deliberately coarse-grained because these helpers are
 * meant for compact percentage/value labels, not arbitrary typography.
 */
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

/**
 * @brief Format a small integer value for the default ring label.
 *
 * `append_percent` requires one extra byte for `%` plus the trailing `\0`.
 */
void egui_view_ring_text_format_value(uint8_t value, uint8_t append_percent, char *buf, uint32_t buf_size)
{
    uint32_t len = 0;
    uint32_t min_size = append_percent ? 3U : 2U;

    if (buf == NULL || buf_size < min_size)
    {
        return;
    }

    // Keep formatting branch-free for the common 0-100 percentage use case.
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

/**
 * @brief Build the default text box inside the ring hole or below it.
 *
 * The width is derived from the ring hole diameter minus a small fixed inset,
 * so the default label never touches the inner edge visually.
 */
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

    // `center_box` selects between centered-in-hole placement and a secondary line below the center.
    text_box->location.x = center_x - box_width / 2;
    text_box->location.y = center_box ? (center_y - box_height / 2) : (center_y + top_offset);
    text_box->size.width = box_width;
    text_box->size.height = box_height;

    return egui_region_is_empty(text_box) ? 0 : 1;
}

/**
 * @brief Estimate the dirty region needed by the default ring text renderer.
 *
 * The returned region is slightly expanded around measured glyph bounds so
 * anti-aliased edges are refreshed cleanly.
 * When font measurement is unavailable, the whole text box is dirtied as a
 * safe fallback.
 */
uint8_t egui_view_ring_text_get_region_basic(egui_core_t *core, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t inner_r, egui_dim_t top_offset,
                                             uint8_t center_box, uint8_t value, uint8_t append_percent, egui_region_t *text_region)
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

    if (!egui_view_ring_text_measure_default_font(core, text_buf, &text_w, &text_h) || text_w <= 0 || text_h <= 0)
    {
        egui_region_copy(text_region, &text_box);
        return 1;
    }

    // Clamp the measured glyph bounds to the text box before center-aligning them.
    text_w = EGUI_MIN(text_w, text_box.size.width);
    text_h = EGUI_MIN(text_h, text_box.size.height);
    egui_common_align_get_x_y(text_box.size.width, text_box.size.height, text_w, text_h, EGUI_ALIGN_CENTER, &offset_x, &offset_y);

    text_region->location.x = text_box.location.x + offset_x;
    text_region->location.y = text_box.location.y + offset_y;
    text_region->size.width = text_w;
    text_region->size.height = text_h;

    // Expand by up to one pixel on each side to catch anti-aliased edges.
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

    // Keep the final dirty region inside the nominal text box.
    egui_region_intersect(text_region, &text_box, text_region);
    return egui_region_is_empty(text_region) ? 0 : 1;
}

/**
 * @brief Draw the default numeric label with centered alignment in the text box.
 *
 * This helper mirrors the placement assumptions used by
 * `egui_view_ring_text_get_region_basic` so drawing and invalidation stay in
 * sync.
 */
void egui_view_ring_text_draw_basic(egui_canvas_t *canvas, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t inner_r, egui_dim_t top_offset,
                                    uint8_t center_box, uint8_t value, uint8_t append_percent, egui_color_t color, egui_alpha_t alpha)
{
    char text_buf[8];
    egui_region_t text_rect;
    const egui_font_t *font = egui_view_ring_text_get_default_font();

    egui_view_ring_text_format_value(value, append_percent, text_buf, sizeof(text_buf));
    if (font != NULL && egui_view_ring_text_get_box_basic(center_x, center_y, inner_r, top_offset, center_box, text_buf, &text_rect))
    {
        egui_canvas_draw_text_in_rect(canvas, font, text_buf, &text_rect, EGUI_ALIGN_CENTER, color, alpha);
    }
}
