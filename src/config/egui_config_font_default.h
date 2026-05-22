#ifndef _EGUI_CONFIG_FONT_DEFAULT_H_
#define _EGUI_CONFIG_FONT_DEFAULT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Runtime TTF font ---- */

/**
 * Runtime TTF font support backed by stb_truetype.
 * When disabled, egui_font_ttf.h / egui_font_ttf.c compile to no feature code.
 */
#ifndef EGUI_CONFIG_FUNCTION_FONT_TTF
#define EGUI_CONFIG_FUNCTION_FONT_TTF 0
#endif

/**
 * Number of rasterized glyphs kept in the egui_font_ttf_t cache.
 */
#ifndef EGUI_CONFIG_FONT_TTF_GLYPH_CACHE_SLOTS
#define EGUI_CONFIG_FONT_TTF_GLYPH_CACHE_SLOTS 8
#endif

/**
 * Maximum cached TTF glyph bitmap width in pixels.
 */
#ifndef EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_W
#define EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_W 20
#endif

/**
 * Maximum cached TTF glyph bitmap height in pixels.
 */
#ifndef EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_H
#define EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_H 32
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_FONT_DEFAULT_H_ */
