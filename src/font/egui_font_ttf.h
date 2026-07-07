/**
 * @file egui_font_ttf.h
 * @brief Runtime TTF font backed by stb_truetype.
 *
 * A zero-heap font adapter that wraps stb_truetype to render any Unicode glyph
 * at a chosen pixel height.  Rasterised glyphs are cached in a fixed-capacity
 * ring buffer embedded in the struct (no heap allocation required).
 *
 * Guarded by EGUI_CONFIG_FUNCTION_FONT_TTF (default 0).  Enable in the
 * application's app_egui_config.h:
 *
 *   #define EGUI_CONFIG_FUNCTION_FONT_TTF 1
 *
 * Cache capacity and maximum glyph bitmap dimensions are tunable at compile
 * time via egui_config_font_default.h (or per-app overrides):
 *
 *   EGUI_CONFIG_FONT_TTF_GLYPH_CACHE_SLOTS    – number of cached glyphs
 *   EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_W   – max glyph width  (pixels)
 *   EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_H   – max glyph height (pixels)
 */

#ifndef _EGUI_FONT_TTF_H_
#define _EGUI_FONT_TTF_H_

#include "config/egui_config.h"

#if EGUI_CONFIG_FUNCTION_FONT_TTF

#include <stdint.h>
#include "font/egui_font.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Opaque stb_truetype storage                                         */
/* ------------------------------------------------------------------ */

/**
 * Byte size of the opaque stbtt_fontinfo storage embedded in egui_font_ttf_t.
 *
 * Must be >= sizeof(stbtt_fontinfo):
 *   32-bit build: ~128 bytes
 *   64-bit build: ~208 bytes
 * 256 covers both with margin.
 */
#define EGUI_FONT_TTF_STB_INFO_OPAQUE_SIZE 256

/* ------------------------------------------------------------------ */
/* Glyph cache entry                                                   */
/* ------------------------------------------------------------------ */

/** One rasterised and cached glyph entry. */
typedef struct egui_font_ttf_glyph egui_font_ttf_glyph_t;
struct egui_font_ttf_glyph
{
    uint32_t codepoint; /**< Unicode codepoint.  0 = empty slot. */
    int advance_px;     /**< Horizontal advance in pixels. */
    int x0;             /**< Bitmap x-offset relative to pen position. */
    int y0;             /**< Bitmap y-offset relative to baseline. */
    int bw;             /**< Bitmap width  in pixels (0 for whitespace). */
    int bh;             /**< Bitmap height in pixels (0 for whitespace). */
    /** 8-bit grayscale alpha bitmap, row-major (bw * bh bytes used). */
    uint8_t bitmap[EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_W * EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_H];
};

/* ------------------------------------------------------------------ */
/* Font struct                                                         */
/* ------------------------------------------------------------------ */

/** Runtime TTF font object.  Declare as static or file-scope. */
typedef struct egui_font_ttf egui_font_ttf_t;
struct egui_font_ttf
{
    egui_font_t base; /**< Must be first – cast-compatible with egui_font_t. */

    const uint8_t *ttf_data;     /**< Pointer to TTF binary (must outlive this font). */
    uint32_t ttf_size;           /**< Byte size of ttf_data. */
    uint16_t pixel_height;       /**< Rendered font size in pixels. */
    const egui_font_t *fallback; /**< Optional: font used when TTF lacks a glyph. */
    uint8_t initialized;         /**< Non-zero after successful egui_font_ttf_init(). */
    uint8_t _cache_next;         /**< Ring-eviction write pointer. */

    /** Opaque storage for stbtt_fontinfo (avoids exposing stb_truetype header). */
    uint8_t _stb_info_opaque[EGUI_FONT_TTF_STB_INFO_OPAQUE_SIZE];

    /** Fixed-capacity glyph cache (ring-eviction). */
    egui_font_ttf_glyph_t _cache[EGUI_CONFIG_FONT_TTF_GLYPH_CACHE_SLOTS];
};

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

/**
 * Initialise an egui_font_ttf_t from a TTF binary in memory.
 *
 * @param self          Caller-allocated font object.  Its lifetime must
 *                      exceed all draw calls that use it.
 * @param ttf_data      Pointer to the TTF binary.  Must remain valid for
 *                      the lifetime of the font object.
 * @param ttf_size      Byte length of ttf_data.
 * @param pixel_height  Desired rendered height in pixels (e.g. 16, 24).
 *
 * @return  0 on success, -1 if args are invalid or TTF cannot be parsed.
 */
int egui_font_ttf_init(egui_font_ttf_t *self, const uint8_t *ttf_data, uint32_t ttf_size, uint16_t pixel_height);

/**
 * Attach a fallback font for glyphs absent from the TTF.
 *
 * When draw_string encounters a codepoint not present in the TTF it renders
 * it using @p fallback instead.  Pass NULL to clear the fallback.
 *
 * May be called at any time after egui_font_ttf_init().
 *
 * @param self     Initialised font object.
 * @param fallback Any egui_font_t.  NULL removes the fallback.
 */
void egui_font_ttf_set_fallback(egui_font_ttf_t *self, const egui_font_t *fallback);

/** Measure a TTF string while preserving core context for fallback fonts. */
int egui_font_ttf_get_str_size_with_core(const egui_font_t *self, egui_core_t *core, const void *string, uint8_t is_multi_line, egui_dim_t line_space,
                                         egui_dim_t *width, egui_dim_t *height);

/** Internal vtable reference (public so EGUI_FONT_SUB_DEFINE_* macros compile). */
extern const egui_font_api_t egui_font_ttf_t_api_table;

#ifdef __cplusplus
}
#endif

#endif /* EGUI_CONFIG_FUNCTION_FONT_TTF */
#endif /* _EGUI_FONT_TTF_H_ */
