/**
 * @file egui_font_ttf.c
 * @brief Runtime TTF font implementation using stb_truetype.
 */

#include "config/egui_config.h"

#if EGUI_CONFIG_FUNCTION_FONT_TTF

#include <string.h>
#include "egui_font_ttf.h"
#include "font/egui_font.h"
#include "canvas/egui_canvas.h"

/*
 * Include stb_truetype implementation.
 *
 * When EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG=1, plutovg-font.c already
 * compiles stb_truetype with STBTT_STATIC (translation-unit-local symbols).
 * Our copy here uses external linkage, so there is no linker conflict; the
 * binary will contain two copies of the stbtt code in that configuration.
 * For all other builds only this copy is present.
 */
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../third_party/plutovg/source/plutovg-stb-truetype.h"
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/* Sanity-check: opaque buffer must be large enough for stbtt_fontinfo.
 * _Static_assert is a C11 extension; guard it to avoid warnings in strict
 * C99 builds.  If this fires, increase EGUI_FONT_TTF_STB_INFO_OPAQUE_SIZE. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(stbtt_fontinfo) <= EGUI_FONT_TTF_STB_INFO_OPAQUE_SIZE, "Increase EGUI_FONT_TTF_STB_INFO_OPAQUE_SIZE to fit stbtt_fontinfo");
#endif

/* ------------------------------------------------------------------ */
/* Private helpers                                                     */
/* ------------------------------------------------------------------ */

/** Cast the opaque byte array to a stbtt_fontinfo pointer. */
static stbtt_fontinfo *priv_info(egui_font_ttf_t *f)
{
    return (stbtt_fontinfo *)(void *)f->_stb_info_opaque;
}

/** Find a cached glyph by codepoint, or return NULL on miss. */
static egui_font_ttf_glyph_t *priv_find_cached(egui_font_ttf_t *f, uint32_t cp)
{
    int i;
    for (i = 0; i < EGUI_CONFIG_FONT_TTF_GLYPH_CACHE_SLOTS; i++)
    {
        if (f->_cache[i].codepoint == cp)
        {
            return &f->_cache[i];
        }
    }
    return NULL;
}

/**
 * Rasterise codepoint into the next ring-eviction cache slot.
 *
 * @return  New cache entry, or NULL if the glyph does not exist in the TTF.
 */
static egui_font_ttf_glyph_t *priv_rasterize(egui_font_ttf_t *f, uint32_t cp)
{
    stbtt_fontinfo *info = priv_info(f);
    float scale = stbtt_ScaleForPixelHeight(info, (float)f->pixel_height);

    int glyph_idx = stbtt_FindGlyphIndex(info, (int)cp);
    if (glyph_idx == 0)
    {
        return NULL; /* Glyph absent from this TTF. */
    }

    int adv_raw, lsb;
    stbtt_GetGlyphHMetrics(info, glyph_idx, &adv_raw, &lsb);
    int advance_px = (int)(adv_raw * scale + 0.5f);

    int bx0, by0, bx1, by1;
    stbtt_GetGlyphBitmapBox(info, glyph_idx, scale, scale, &bx0, &by0, &bx1, &by1);
    int bw = bx1 - bx0;
    int bh = by1 - by0;

    /* Evict the oldest slot (ring buffer). */
    int slot = (int)f->_cache_next;
    f->_cache_next = (uint8_t)((slot + 1) % EGUI_CONFIG_FONT_TTF_GLYPH_CACHE_SLOTS);

    egui_font_ttf_glyph_t *g = &f->_cache[slot];
    g->codepoint = cp;
    g->advance_px = advance_px;
    g->x0 = bx0;
    g->y0 = by0;

    if (bw <= 0 || bh <= 0 || bw > EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_W || bh > EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_H)
    {
        /* Whitespace or oversized glyph – cache metrics only, no bitmap. */
        g->bw = 0;
        g->bh = 0;
        return g;
    }

    g->bw = bw;
    g->bh = bh;
    memset(g->bitmap, 0, (size_t)(bw * bh));
    stbtt_MakeGlyphBitmap(info, g->bitmap, bw, bh, bw, scale, scale, glyph_idx);
    return g;
}

/** Return a cached glyph entry, rasterising it if not yet cached. */
static egui_font_ttf_glyph_t *priv_get_glyph(egui_font_ttf_t *f, uint32_t cp)
{
    egui_font_ttf_glyph_t *g = priv_find_cached(f, cp);
    if (g)
    {
        return g;
    }
    return priv_rasterize(f, cp);
}

/** Encode a Unicode codepoint as a null-terminated UTF-8 string (max 5 bytes). */
static void priv_encode_utf8(uint32_t cp, char out[5])
{
    if (cp < 0x80u)
    {
        out[0] = (char)cp;
        out[1] = '\0';
    }
    else if (cp < 0x800u)
    {
        out[0] = (char)(0xC0u | (cp >> 6));
        out[1] = (char)(0x80u | (cp & 0x3Fu));
        out[2] = '\0';
    }
    else if (cp < 0x10000u)
    {
        out[0] = (char)(0xE0u | (cp >> 12));
        out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (char)(0x80u | (cp & 0x3Fu));
        out[3] = '\0';
    }
    else
    {
        out[0] = (char)(0xF0u | (cp >> 18));
        out[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
        out[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[3] = (char)(0x80u | (cp & 0x3Fu));
        out[4] = '\0';
    }
}

/* ------------------------------------------------------------------ */
/* vtable implementations                                              */
/* ------------------------------------------------------------------ */

static int egui_font_ttf_draw_string(const egui_font_t *self, egui_canvas_t *canvas, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color,
                                     egui_alpha_t alpha)
{
    egui_font_ttf_t *font = (egui_font_ttf_t *)self;
    const char *s = (const char *)string;

    if (!font->initialized || s == NULL || canvas == NULL)
    {
        return 0;
    }

    stbtt_fontinfo *info = priv_info(font);
    float scale = stbtt_ScaleForPixelHeight(info, (float)font->pixel_height);

    /* y is the top of the cell; derive baseline. */
    int ascent_raw;
    stbtt_GetFontVMetrics(info, &ascent_raw, NULL, NULL);
    int baseline = y + (int)(ascent_raw * scale + 0.5f);

    int draw_x = (int)x;
    int char_cnt = 0;

    while (*s != '\0')
    {
        uint32_t cp;
        int bytes = egui_font_get_utf8_code(s, &cp);
        if (bytes <= 0)
        {
            break;
        }

        if (cp == '\n')
        {
            s += bytes;
            continue; /* Multi-line handled by canvas helper above us. */
        }

        egui_font_ttf_glyph_t *g = priv_get_glyph(font, cp);
        if (g == NULL)
        {
            /* Glyph absent from TTF – try fallback. */
            if (font->fallback != NULL && font->fallback->api != NULL)
            {
                char ch_str[5];
                priv_encode_utf8(cp, ch_str);
                egui_dim_t fw = 0, fh = 0;
                egui_font_get_str_size_with_canvas(font->fallback, canvas, ch_str, 0, 0, &fw, &fh);
                font->fallback->api->draw_string(font->fallback, canvas, ch_str, (egui_dim_t)draw_x, y, color, alpha);
                draw_x += (int)fw;
            }
            else
            {
                /* No fallback – skip with half-em placeholder advance. */
                draw_x += (int)font->pixel_height / 2;
            }
        }
        else
        {
            /* Render cached glyph bitmap pixel by pixel. */
            if (g->bw > 0 && g->bh > 0)
            {
                int gy, gx;
                for (gy = 0; gy < g->bh; gy++)
                {
                    for (gx = 0; gx < g->bw; gx++)
                    {
                        uint8_t bval = g->bitmap[gy * g->bw + gx];
                        if (bval > 0)
                        {
                            egui_alpha_t pa = (egui_alpha_t)((uint32_t)bval * (uint32_t)alpha / 255u);
                            egui_canvas_draw_point(canvas, (egui_dim_t)(draw_x + g->x0 + gx), (egui_dim_t)(baseline + g->y0 + gy), color, pa);
                        }
                    }
                }
            }
            draw_x += g->advance_px;
        }

        s += bytes;
        char_cnt++;
    }

    return char_cnt;
}

static int egui_font_ttf_get_str_size_with_context(const egui_font_t *self, egui_core_t *core, const void *string, uint8_t is_multi_line, egui_dim_t line_space,
                                                   egui_dim_t *width, egui_dim_t *height)
{
    egui_font_ttf_t *font = (egui_font_ttf_t *)self;
    const char *s = (const char *)string;

    if (width != NULL)
        *width = 0;
    if (height != NULL)
        *height = 0;

    if (!font->initialized || s == NULL)
    {
        return 0;
    }

    stbtt_fontinfo *info = priv_info(font);
    float scale = stbtt_ScaleForPixelHeight(info, (float)font->pixel_height);

    int ascent_raw, descent_raw;
    stbtt_GetFontVMetrics(info, &ascent_raw, &descent_raw, NULL);
    int line_h = (int)((ascent_raw - descent_raw) * scale + 0.5f);
    if (line_h < (int)font->pixel_height)
    {
        line_h = (int)font->pixel_height;
    }

    int total_w = 0;
    int cur_w = 0;
    int lines = 1;

    while (*s != '\0')
    {
        uint32_t cp;
        int bytes = egui_font_get_utf8_code(s, &cp);
        if (bytes <= 0)
        {
            break;
        }
        s += bytes;

        if (cp == '\n')
        {
            if (is_multi_line)
            {
                if (cur_w > total_w)
                    total_w = cur_w;
                cur_w = 0;
                lines++;
            }
            continue;
        }

        int adv_px = 0;
        int glyph_idx = stbtt_FindGlyphIndex(info, (int)cp);
        if (glyph_idx != 0)
        {
            int adv_raw, lsb;
            stbtt_GetGlyphHMetrics(info, glyph_idx, &adv_raw, &lsb);
            adv_px = (int)(adv_raw * scale + 0.5f);
        }
        else if (font->fallback != NULL && font->fallback->api != NULL)
        {
            char ch_str[5];
            priv_encode_utf8(cp, ch_str);
            egui_dim_t fw = 0, fh = 0;
            egui_font_get_str_size_with_core(font->fallback, core, ch_str, 0, 0, &fw, &fh);
            adv_px = (int)fw;
        }
        else
        {
            adv_px = (int)font->pixel_height / 2;
        }
        cur_w += adv_px;
    }

    if (cur_w > total_w)
        total_w = cur_w;

    if (width != NULL)
        *width = (egui_dim_t)total_w;
    if (height != NULL)
    {
        int total_h = lines * line_h;
        if (lines > 1)
        {
            total_h += (lines - 1) * (int)line_space;
        }
        *height = (egui_dim_t)total_h;
    }
    return 0;
}

int egui_font_ttf_get_str_size_with_core(const egui_font_t *self, egui_core_t *core, const void *string, uint8_t is_multi_line, egui_dim_t line_space,
                                         egui_dim_t *width, egui_dim_t *height)
{
    return egui_font_ttf_get_str_size_with_context(self, core, string, is_multi_line, line_space, width, height);
}

static int egui_font_ttf_get_str_size(const egui_font_t *self, const void *string, uint8_t is_multi_line, egui_dim_t line_space, egui_dim_t *width,
                                      egui_dim_t *height)
{
    return egui_font_ttf_get_str_size_with_context(self, NULL, string, is_multi_line, line_space, width, height);
}

/* ------------------------------------------------------------------ */
/* Vtable                                                              */
/* ------------------------------------------------------------------ */

const egui_font_api_t egui_font_ttf_t_api_table = {
        .draw_string = egui_font_ttf_draw_string,
        .get_str_size = egui_font_ttf_get_str_size,
};

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

int egui_font_ttf_init(egui_font_ttf_t *self, const uint8_t *ttf_data, uint32_t ttf_size, uint16_t pixel_height)
{
    if (self == NULL || ttf_data == NULL || ttf_size == 0 || pixel_height == 0)
    {
        return -1;
    }

    memset(self, 0, sizeof(*self));
    self->base.api = &egui_font_ttf_t_api_table;
    self->base.res = (const void *)ttf_data;
    self->ttf_data = ttf_data;
    self->ttf_size = ttf_size;
    self->pixel_height = pixel_height;

    stbtt_fontinfo *info = priv_info(self);
    int offset = stbtt_GetFontOffsetForIndex(ttf_data, 0);
    if (offset < 0)
    {
        return -1;
    }
    if (!stbtt_InitFont(info, ttf_data, offset))
    {
        return -1;
    }

    self->initialized = 1;
    return 0;
}

void egui_font_ttf_set_fallback(egui_font_ttf_t *self, const egui_font_t *fallback)
{
    if (self != NULL)
    {
        self->fallback = fallback;
    }
}

#endif /* EGUI_CONFIG_FUNCTION_FONT_TTF */
