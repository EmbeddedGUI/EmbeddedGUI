#include "egui_canvas_compact.h"

#define EGUI_CANVAS_COMPACT_TEXT_ROWS   5
#define EGUI_CANVAS_COMPACT_NUMBER_ROWS 5
#define EGUI_CANVAS_COMPACT_NUMBER_COLS 3

typedef struct egui_canvas_compact_glyph egui_canvas_compact_glyph_t;
struct egui_canvas_compact_glyph
{
    char ch;
    uint8_t width;
    uint8_t rows[EGUI_CANVAS_COMPACT_TEXT_ROWS];
};

typedef enum
{
    EGUI_CANVAS_COMPACT_BITMAP_KIND_TEXT = 0,
    EGUI_CANVAS_COMPACT_BITMAP_KIND_NUMBER,
} egui_canvas_compact_bitmap_kind_t;

typedef struct
{
    uint8_t width;
    const uint8_t *rows;
} egui_canvas_compact_bitmap_glyph_t;

typedef struct
{
    egui_dim_t scale;
    egui_dim_t gap;
    egui_dim_t width;
    egui_dim_t height;
} egui_canvas_compact_bitmap_layout_t;

#define EGUI_COMPACT_ROWS(_r0, _r1, _r2, _r3, _r4) {_r0, _r1, _r2, _r3, _r4}

static const egui_canvas_compact_glyph_t egui_canvas_compact_glyphs[] = {
        {' ', 3, EGUI_COMPACT_ROWS(0x0, 0x0, 0x0, 0x0, 0x0)},      {'%', 5, EGUI_COMPACT_ROWS(0x19, 0x12, 0x04, 0x09, 0x13)},
        {'+', 5, EGUI_COMPACT_ROWS(0x04, 0x04, 0x1F, 0x04, 0x04)}, {'-', 5, EGUI_COMPACT_ROWS(0x00, 0x00, 0x1F, 0x00, 0x00)},
        {'.', 2, EGUI_COMPACT_ROWS(0x0, 0x0, 0x0, 0x0, 0x2)},      {'/', 5, EGUI_COMPACT_ROWS(0x01, 0x02, 0x04, 0x08, 0x10)},
        {'0', 4, EGUI_COMPACT_ROWS(0x6, 0x9, 0x9, 0x9, 0x6)},      {'1', 3, EGUI_COMPACT_ROWS(0x2, 0x6, 0x2, 0x2, 0x7)},
        {'2', 4, EGUI_COMPACT_ROWS(0xE, 0x1, 0x6, 0x8, 0xF)},      {'3', 4, EGUI_COMPACT_ROWS(0xE, 0x1, 0x6, 0x1, 0xE)},
        {'4', 4, EGUI_COMPACT_ROWS(0x9, 0x9, 0xF, 0x1, 0x1)},      {'5', 4, EGUI_COMPACT_ROWS(0xF, 0x8, 0xE, 0x1, 0xE)},
        {'6', 4, EGUI_COMPACT_ROWS(0x6, 0x8, 0xE, 0x9, 0x6)},      {'7', 4, EGUI_COMPACT_ROWS(0xF, 0x1, 0x2, 0x4, 0x4)},
        {'8', 4, EGUI_COMPACT_ROWS(0x6, 0x9, 0x6, 0x9, 0x6)},      {'9', 4, EGUI_COMPACT_ROWS(0x6, 0x9, 0x7, 0x1, 0x6)},
        {'?', 5, EGUI_COMPACT_ROWS(0x0E, 0x11, 0x06, 0x00, 0x04)}, {'A', 5, EGUI_COMPACT_ROWS(0x0E, 0x11, 0x1F, 0x11, 0x11)},
        {'B', 5, EGUI_COMPACT_ROWS(0x1E, 0x11, 0x1E, 0x11, 0x1E)}, {'C', 5, EGUI_COMPACT_ROWS(0x0F, 0x10, 0x10, 0x10, 0x0F)},
        {'D', 5, EGUI_COMPACT_ROWS(0x1E, 0x11, 0x11, 0x11, 0x1E)}, {'E', 5, EGUI_COMPACT_ROWS(0x1F, 0x10, 0x1E, 0x10, 0x1F)},
        {'F', 5, EGUI_COMPACT_ROWS(0x1F, 0x10, 0x1E, 0x10, 0x10)}, {'G', 5, EGUI_COMPACT_ROWS(0x0F, 0x10, 0x17, 0x11, 0x0E)},
        {'H', 5, EGUI_COMPACT_ROWS(0x11, 0x11, 0x1F, 0x11, 0x11)}, {'I', 3, EGUI_COMPACT_ROWS(0x7, 0x2, 0x2, 0x2, 0x7)},
        {'K', 5, EGUI_COMPACT_ROWS(0x11, 0x12, 0x1C, 0x12, 0x11)}, {'L', 5, EGUI_COMPACT_ROWS(0x10, 0x10, 0x10, 0x10, 0x1F)},
        {'M', 5, EGUI_COMPACT_ROWS(0x11, 0x1B, 0x15, 0x11, 0x11)}, {'N', 5, EGUI_COMPACT_ROWS(0x11, 0x19, 0x15, 0x13, 0x11)},
        {'O', 5, EGUI_COMPACT_ROWS(0x0E, 0x11, 0x11, 0x11, 0x0E)}, {'P', 5, EGUI_COMPACT_ROWS(0x1E, 0x11, 0x1E, 0x10, 0x10)},
        {'R', 5, EGUI_COMPACT_ROWS(0x1E, 0x11, 0x1E, 0x12, 0x11)}, {'S', 5, EGUI_COMPACT_ROWS(0x0F, 0x10, 0x0E, 0x01, 0x1E)},
        {'T', 5, EGUI_COMPACT_ROWS(0x1F, 0x04, 0x04, 0x04, 0x04)}, {'U', 5, EGUI_COMPACT_ROWS(0x11, 0x11, 0x11, 0x11, 0x0E)},
        {'V', 5, EGUI_COMPACT_ROWS(0x11, 0x11, 0x11, 0x0A, 0x04)}, {'Y', 5, EGUI_COMPACT_ROWS(0x11, 0x0A, 0x04, 0x04, 0x04)},
};

static const uint8_t egui_canvas_compact_number_glyphs[12][EGUI_CANVAS_COMPACT_NUMBER_ROWS] = {
        {0x7, 0x5, 0x5, 0x5, 0x7}, {0x2, 0x6, 0x2, 0x2, 0x7}, {0x7, 0x1, 0x7, 0x4, 0x7}, {0x7, 0x1, 0x7, 0x1, 0x7},
        {0x5, 0x5, 0x7, 0x1, 0x1}, {0x7, 0x4, 0x7, 0x1, 0x7}, {0x7, 0x4, 0x7, 0x5, 0x7}, {0x7, 0x1, 0x1, 0x1, 0x1},
        {0x7, 0x5, 0x7, 0x5, 0x7}, {0x7, 0x5, 0x7, 0x1, 0x7}, {0x0, 0x2, 0x7, 0x2, 0x0}, {0x5, 0x1, 0x2, 0x4, 0x5},
};

static const egui_canvas_compact_glyph_t *egui_canvas_compact_text_get_glyph(char ch)
{
    uint8_t code = (uint8_t)ch;

    if (code >= (uint8_t)'a' && code <= (uint8_t)'z')
    {
        code = (uint8_t)(code - ((uint8_t)'a' - (uint8_t)'A'));
    }

    for (uint32_t i = 0; i < EGUI_ARRAY_SIZE(egui_canvas_compact_glyphs); i++)
    {
        if ((uint8_t)egui_canvas_compact_glyphs[i].ch == code)
        {
            return &egui_canvas_compact_glyphs[i];
        }
    }

    if (code >= 32U && code <= 126U)
    {
        return egui_canvas_compact_text_get_glyph('?');
    }

    return NULL;
}

static const uint8_t *egui_canvas_compact_number_get_glyph(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return egui_canvas_compact_number_glyphs[(uint8_t)(ch - '0')];
    }

    if (ch == '+')
    {
        return egui_canvas_compact_number_glyphs[10];
    }

    if (ch == '%')
    {
        return egui_canvas_compact_number_glyphs[11];
    }

    return NULL;
}

static uint8_t egui_canvas_compact_bitmap_get_glyph(egui_canvas_compact_bitmap_kind_t kind, char ch, egui_canvas_compact_bitmap_glyph_t *glyph)
{
    if (glyph == NULL)
    {
        return 0;
    }

    if (kind == EGUI_CANVAS_COMPACT_BITMAP_KIND_TEXT)
    {
        const egui_canvas_compact_glyph_t *text_glyph = egui_canvas_compact_text_get_glyph(ch);

        if (text_glyph == NULL)
        {
            return 0;
        }

        glyph->width = text_glyph->width;
        glyph->rows = text_glyph->rows;
        return 1;
    }

    {
        const uint8_t *number_glyph = egui_canvas_compact_number_get_glyph(ch);

        if (number_glyph == NULL)
        {
            return 0;
        }

        glyph->width = EGUI_CANVAS_COMPACT_NUMBER_COLS;
        glyph->rows = number_glyph;
        return 1;
    }
}

static egui_dim_t egui_canvas_compact_bitmap_get_gap(egui_canvas_compact_bitmap_kind_t kind, egui_dim_t scale, egui_dim_t glyph_count)
{
    if (glyph_count <= 1)
    {
        return 0;
    }

    if (kind == EGUI_CANVAS_COMPACT_BITMAP_KIND_TEXT)
    {
        return scale;
    }

    return EGUI_MAX(1, scale - 1);
}

static uint8_t egui_canvas_compact_bitmap_measure_internal(egui_canvas_compact_bitmap_kind_t kind, const char *text, egui_dim_t max_width,
                                                           egui_dim_t max_height, egui_canvas_compact_bitmap_layout_t *layout)
{
    egui_canvas_compact_bitmap_glyph_t glyph;
    egui_dim_t glyph_count = 0;
    egui_dim_t base_units = 0;
    egui_dim_t glyph_units = 0;
    egui_dim_t scale;
    const char *cursor = text;

    if (text == NULL || text[0] == '\0' || max_width <= 0 || max_height <= 0)
    {
        return 0;
    }

    while (*cursor != '\0')
    {
        if (!egui_canvas_compact_bitmap_get_glyph(kind, *cursor, &glyph))
        {
            return 0;
        }

        if (glyph_count > 0)
        {
            base_units += 1;
        }
        base_units += glyph.width;
        glyph_units += glyph.width;
        glyph_count++;
        cursor++;
    }

    if (glyph_count == 0 || base_units <= 0)
    {
        return 0;
    }

    scale = EGUI_MIN(max_width / base_units, max_height / EGUI_CANVAS_COMPACT_TEXT_ROWS);
    if (scale <= 0)
    {
        scale = 1;
    }

    if (layout != NULL)
    {
        egui_dim_t gap = egui_canvas_compact_bitmap_get_gap(kind, scale, glyph_count);

        layout->scale = scale;
        layout->gap = gap;
        layout->width = glyph_units * scale + ((glyph_count > 1) ? (glyph_count - 1) * gap : 0);
        layout->height = EGUI_CANVAS_COMPACT_TEXT_ROWS * scale;
    }

    return 1;
}

static void egui_canvas_compact_bitmap_draw_glyph(const egui_canvas_compact_bitmap_glyph_t *glyph, egui_dim_t x, egui_dim_t y, egui_dim_t scale,
                                                  egui_color_t color, egui_alpha_t alpha)
{
    for (egui_dim_t row = 0; row < EGUI_CANVAS_COMPACT_TEXT_ROWS; row++)
    {
        uint8_t bits = glyph->rows[row];

        for (egui_dim_t col = 0; col < glyph->width; col++)
        {
            uint8_t mask = (uint8_t)(1U << (glyph->width - 1 - col));

            if ((bits & mask) == 0)
            {
                continue;
            }

            egui_canvas_draw_fillrect(x + col * scale, y + row * scale, scale, scale, color, alpha);
        }
    }
}

static uint8_t egui_canvas_compact_bitmap_is_supported(egui_canvas_compact_bitmap_kind_t kind, const char *text)
{
    egui_canvas_compact_bitmap_glyph_t glyph;
    const char *cursor = text;

    if (text == NULL || text[0] == '\0')
    {
        return 0;
    }

    while (*cursor != '\0')
    {
        if (!egui_canvas_compact_bitmap_get_glyph(kind, *cursor, &glyph))
        {
            return 0;
        }
        cursor++;
    }

    return 1;
}

static uint8_t egui_canvas_compact_bitmap_measure_with_font_internal(egui_canvas_compact_bitmap_kind_t kind, const egui_font_t *font, const char *text,
                                                                     egui_dim_t max_width, egui_dim_t max_height, egui_dim_t *out_width, egui_dim_t *out_height)
{
    if (text == NULL || text[0] == '\0')
    {
        return 0;
    }

    if (font != NULL)
    {
        egui_dim_t width = 0;
        egui_dim_t height = 0;

        if (font->api == NULL || font->api->get_str_size == NULL)
        {
            return 0;
        }

        font->api->get_str_size(font, text, 0, 0, &width, &height);
        if (out_width != NULL)
        {
            *out_width = width;
        }
        if (out_height != NULL)
        {
            *out_height = height;
        }
        return 1;
    }

    {
        egui_canvas_compact_bitmap_layout_t layout;

        if (!egui_canvas_compact_bitmap_measure_internal(kind, text, max_width, max_height, &layout))
        {
            return 0;
        }

        if (out_width != NULL)
        {
            *out_width = layout.width;
        }
        if (out_height != NULL)
        {
            *out_height = layout.height;
        }
    }

    return 1;
}

static uint8_t egui_canvas_compact_bitmap_draw_internal(egui_canvas_compact_bitmap_kind_t kind, const char *text, const egui_region_t *region,
                                                        uint8_t align_type, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_compact_bitmap_layout_t layout;
    egui_canvas_compact_bitmap_glyph_t glyph;
    egui_dim_t draw_x;
    egui_dim_t draw_y;
    const char *cursor = text;

    if (region == NULL || !egui_canvas_compact_bitmap_measure_internal(kind, text, region->size.width, region->size.height, &layout))
    {
        return 0;
    }

    switch (align_type & EGUI_ALIGN_HMASK)
    {
    case EGUI_ALIGN_LEFT:
        draw_x = region->location.x;
        break;
    case EGUI_ALIGN_RIGHT:
        draw_x = region->location.x + region->size.width - layout.width;
        break;
    default:
        draw_x = region->location.x + (region->size.width - layout.width) / 2;
        break;
    }

    switch (align_type & EGUI_ALIGN_VMASK)
    {
    case EGUI_ALIGN_TOP:
        draw_y = region->location.y;
        break;
    case EGUI_ALIGN_BOTTOM:
        draw_y = region->location.y + region->size.height - layout.height;
        break;
    default:
        draw_y = region->location.y + (region->size.height - layout.height) / 2;
        break;
    }

    while (*cursor != '\0')
    {
        if (!egui_canvas_compact_bitmap_get_glyph(kind, *cursor, &glyph))
        {
            return 0;
        }

        egui_canvas_compact_bitmap_draw_glyph(&glyph, draw_x, draw_y, layout.scale, color, alpha);
        draw_x += glyph.width * layout.scale;
        if (cursor[1] != '\0')
        {
            draw_x += layout.gap;
        }
        cursor++;
    }

    return 1;
}

static uint8_t egui_canvas_compact_bitmap_draw_with_font_internal(egui_canvas_compact_bitmap_kind_t kind, const egui_font_t *font, const char *text,
                                                                  const egui_region_t *region, uint8_t align_type, egui_dim_t line_space,
                                                                  uint8_t use_line_space, egui_color_t color, egui_alpha_t alpha)
{
    if (text == NULL || text[0] == '\0' || region == NULL)
    {
        return 0;
    }

    if (font != NULL)
    {
        egui_region_t draw_region = *region;

        if (use_line_space)
        {
            egui_canvas_draw_text_in_rect_with_line_space(font, text, &draw_region, align_type, line_space, color, alpha);
        }
        else
        {
            egui_canvas_draw_text_in_rect(font, text, &draw_region, align_type, color, alpha);
        }
        return 1;
    }

    return egui_canvas_compact_bitmap_draw_internal(kind, text, region, align_type, color, alpha);
}

uint8_t egui_canvas_compact_text_is_supported(const char *text)
{
    return egui_canvas_compact_bitmap_is_supported(EGUI_CANVAS_COMPACT_BITMAP_KIND_TEXT, text);
}

uint8_t egui_canvas_compact_text_measure(const char *text, egui_dim_t max_width, egui_dim_t max_height, egui_canvas_compact_text_layout_t *layout)
{
    return egui_canvas_compact_bitmap_measure_internal(EGUI_CANVAS_COMPACT_BITMAP_KIND_TEXT, text, max_width, max_height,
                                                       (egui_canvas_compact_bitmap_layout_t *)layout);
}

uint8_t egui_canvas_compact_text_measure_with_font(const egui_font_t *font, const char *text, egui_dim_t max_width, egui_dim_t max_height,
                                                   egui_dim_t *out_width, egui_dim_t *out_height)
{
    return egui_canvas_compact_bitmap_measure_with_font_internal(EGUI_CANVAS_COMPACT_BITMAP_KIND_TEXT, font, text, max_width, max_height, out_width,
                                                                 out_height);
}

uint8_t egui_canvas_compact_text_draw(const char *text, const egui_region_t *region, uint8_t align_type, egui_color_t color, egui_alpha_t alpha)
{
    return egui_canvas_compact_bitmap_draw_internal(EGUI_CANVAS_COMPACT_BITMAP_KIND_TEXT, text, region, align_type, color, alpha);
}

uint8_t egui_canvas_compact_text_draw_with_font(const egui_font_t *font, const char *text, const egui_region_t *region, uint8_t align_type,
                                                egui_dim_t line_space, uint8_t use_line_space, egui_color_t color, egui_alpha_t alpha)
{
    return egui_canvas_compact_bitmap_draw_with_font_internal(EGUI_CANVAS_COMPACT_BITMAP_KIND_TEXT, font, text, region, align_type, line_space, use_line_space,
                                                              color, alpha);
}

uint8_t egui_canvas_compact_number_is_supported(const char *text)
{
    return egui_canvas_compact_bitmap_is_supported(EGUI_CANVAS_COMPACT_BITMAP_KIND_NUMBER, text);
}

uint8_t egui_canvas_compact_number_measure(const char *text, egui_dim_t max_width, egui_dim_t max_height, egui_canvas_compact_number_layout_t *layout)
{
    return egui_canvas_compact_bitmap_measure_internal(EGUI_CANVAS_COMPACT_BITMAP_KIND_NUMBER, text, max_width, max_height,
                                                       (egui_canvas_compact_bitmap_layout_t *)layout);
}

uint8_t egui_canvas_compact_number_measure_with_font(const egui_font_t *font, const char *text, egui_dim_t max_width, egui_dim_t max_height,
                                                     egui_dim_t *out_width, egui_dim_t *out_height)
{
    return egui_canvas_compact_bitmap_measure_with_font_internal(EGUI_CANVAS_COMPACT_BITMAP_KIND_NUMBER, font, text, max_width, max_height, out_width,
                                                                 out_height);
}

uint8_t egui_canvas_compact_number_draw(const char *text, const egui_region_t *region, egui_color_t color, egui_alpha_t alpha)
{
    return egui_canvas_compact_bitmap_draw_internal(EGUI_CANVAS_COMPACT_BITMAP_KIND_NUMBER, text, region, EGUI_ALIGN_CENTER, color, alpha);
}

uint8_t egui_canvas_compact_number_draw_with_font(const egui_font_t *font, const char *text, const egui_region_t *region, egui_color_t color,
                                                  egui_alpha_t alpha)
{
    return egui_canvas_compact_bitmap_draw_with_font_internal(EGUI_CANVAS_COMPACT_BITMAP_KIND_NUMBER, font, text, region, EGUI_ALIGN_CENTER, 0, 0, color,
                                                              alpha);
}

void egui_canvas_draw_line_round_cap_compact(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color,
                                             egui_alpha_t alpha)
{
    egui_dim_t cap_radius;

    if (stroke_width <= 0)
    {
        return;
    }

    if (stroke_width <= 1)
    {
        egui_canvas_draw_line(x1, y1, x2, y2, stroke_width, color, alpha);
        return;
    }

    cap_radius = stroke_width >> 1;
    egui_canvas_draw_line(x1, y1, x2, y2, stroke_width, color, alpha);
    egui_canvas_draw_circle_fill_basic(x1, y1, cap_radius, color, alpha);
    if (x1 != x2 || y1 != y2)
    {
        egui_canvas_draw_circle_fill_basic(x2, y2, cap_radius, color, alpha);
    }
}
