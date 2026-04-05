#ifndef _EGUI_CONFIG_CANVAS_DEFAULT_H_
#define _EGUI_CONFIG_CANVAS_DEFAULT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Default font ---- */

/**
 * Resource options.
 * Select the default font
 */
#ifndef EGUI_CONFIG_FONT_DEFAULT
#define EGUI_CONFIG_FONT_DEFAULT &egui_res_font_montserrat_14_4
#endif

/* ---- Circle radius range ---- */

/**
 * Resource options.
 * select support circle radius range.
 */
#ifndef EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE
#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 50
#endif

/**
 * Canvas state options.
 * When 1, keep the extra clip pointer used by scroll/list/viewport style widgets.
 * Disable only when the application never relies on nested clip regions.
 */
#ifndef EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE
#define EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE 1
#endif

/**
 * Canvas state options.
 * When 1, keep the optional runtime-registered circle LUT list for radii beyond
 * EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE.
 * Disable only when the application never registers extra circle info.
 */
#ifndef EGUI_CONFIG_CANVAS_SPEC_CIRCLE_INFO_ENABLE
#define EGUI_CONFIG_CANVAS_SPEC_CIRCLE_INFO_ENABLE 1
#endif

/* ---- Circle/Arc default algorithm ---- */

/**
 * Canvas draw options.
 * When 1, default circle/arc APIs use HQ algorithm (better quality, slower).
 * When 0, default circle/arc APIs use basic algorithm (faster, lookup table).
 * Users can always explicitly call _basic or _hq variants regardless of this setting.
 */
#ifndef EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ
#define EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ 0
#endif

/* Canvas fast-path defaults now live in egui_config_fast_path_default.h. */

/* ---- Enhanced widget drawing ---- */

/**
 * Enhanced widget drawing mode.
 * When enabled, widgets use gradient fills instead of flat colors for a more polished look.
 * Automatically enables shadow and gradient dithering when active.
 * Default: 0 (off) - zero overhead when disabled.
 */
#ifndef EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#define EGUI_CONFIG_WIDGET_ENHANCED_DRAW 0
#endif

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW && !EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#undef EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW 1
#endif

/* ---- Circle HQ parameters ---- */

/**
 * Canvas draw options.
 * When 1, HQ circle uses int32_t only (no 64-bit math in hot path).
 * Max radius limited to EGUI_CONFIG_CIRCLE_HQ_MAX_RADIUS (default 4095).
 * Recommended for 32-bit MCUs without hardware 64-bit multiply.
 * When 0, uses int64_t for unlimited radius support.
 */
#ifndef EGUI_CONFIG_CIRCLE_HQ_INT32_ONLY
#define EGUI_CONFIG_CIRCLE_HQ_INT32_ONLY 1
#endif

/**
 * Canvas draw options.
 * Maximum radius when EGUI_CONFIG_CIRCLE_HQ_INT32_ONLY is enabled.
 * Derived from int32_t overflow limit with SCALE=8: 2*(8r+11)^2 < INT32_MAX => r < 4095.
 */
#ifndef EGUI_CONFIG_CIRCLE_HQ_MAX_RADIUS
#define EGUI_CONFIG_CIRCLE_HQ_MAX_RADIUS 4095
#endif

/**
 * Canvas draw options.
 * When 1, HQ circle uses 2x2 (4-point) sub-pixel sampling for faster performance.
 * Produces 5 alpha levels (0, 64, 128, 191, 255).
 * When 0 (default), uses 4x4 (16-point) sampling for best quality (17 alpha levels).
 */
#ifndef EGUI_CONFIG_CIRCLE_HQ_SAMPLE_2X2
#define EGUI_CONFIG_CIRCLE_HQ_SAMPLE_2X2 0
#endif

/* ---- Line HQ parameters ---- */

/**
 * Canvas draw options.
 * When 1, HQ line uses 2x2 (4-point) sub-pixel sampling for faster performance.
 * When 0 (default), uses 4x4 (16-point) sampling for best quality.
 */
#ifndef EGUI_CONFIG_LINE_HQ_SAMPLE_2X2
#define EGUI_CONFIG_LINE_HQ_SAMPLE_2X2 0
#endif

/* ---- Gradient dithering ---- */

/**
 * Canvas draw options.
 * Enable ordered dithering for gradient fills to reduce color banding on low-depth displays.
 */
#ifndef EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
#define EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING 0
#endif

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW && !EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
#undef EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
#define EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING 1
#endif

/* Image/font fast-path and cache-policy defaults now live in egui_config_fast_path_default.h. */

/**
 * Font format options.
 * Enable support for 1-bit packed std font bitmaps.
 * Default: 0 (off). Enable only when the application really uses 1bpp fonts.

 */
#ifndef EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
#define EGUI_CONFIG_FUNCTION_FONT_FORMAT_1 0
#endif

/**
 * Font format options.
 * Enable support for 2-bit packed std font bitmaps.
 * Default: 0 (off). Enable only when the application really uses 2bpp fonts.

 */
#ifndef EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
#define EGUI_CONFIG_FUNCTION_FONT_FORMAT_2 0
#endif

/**
 * Font format options.
 * Enable support for 4-bit packed std font bitmaps.
 * Default: 1 (on). This is the default built-in font format.
 */
#ifndef EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
#define EGUI_CONFIG_FUNCTION_FONT_FORMAT_4 1
#endif

/**
 * Font format options.
 * Enable support for 8-bit packed std font bitmaps.
 * Default: 0 (off). Enable only when the application really uses 8bpp fonts.

 */
#ifndef EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
#define EGUI_CONFIG_FUNCTION_FONT_FORMAT_8 0
#endif

/**
 * Font cache options.
 * When 1, use compact uint8_t fields for ASCII code lookup cache (saves ~20 B BSS).
 * Only suitable for pure ASCII fonts (code <=
 * 255).
 * When 0, use uint16_t/uint32_t fields for full Unicode support.
 */
#ifndef EGUI_CONFIG_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT
#define EGUI_CONFIG_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT 0
#endif

/**
 * Font cache options.
 * When 1, build ASCII (0~127) direct lookup table for O(1) glyph access.
 * Allocates ~140 B heap + 8 B BSS on first use, persists across frames.
 * When 0, all ASCII characters use optimized binary search with multi-level cache.
 * Default: 0 (disabled) to save RAM. Enable for pure English UI applications.
 * Performance gain: ~10-20% for pure English UI, ~1-2% for Chinese UI.
 */
#ifndef EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE
#define EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE 0
#endif

/**
 * Font cache options.
 * When 1, use uint8_t for ASCII lookup index (max 255 glyphs, saves ~128 B heap).
 * When 0, use uint16_t (supports up to 65535 glyphs).
 * Only effective when EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE is 1.
 */
#ifndef EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT
#define EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_INDEX_8BIT 0
#endif

/**
 * Font cache options.
 * When 1, cache multi-line text line split results to avoid rescanning '\n' on every draw.
 * Allocates ~164 B heap for recent string split cache.
 * When 0, rescan line breaks on every get_str_size or draw call.
 * Default: 0 (disabled) to save RAM. Enable for UI with multi-line labels.
 */
#ifndef EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE
#define EGUI_CONFIG_FONT_STD_LINE_CACHE_ENABLE 0
#endif

/**
 * Font cache options.
 * Maximum glyphs per cache slot for draw-prefix cache.
 * Draw-prefix cache stores glyph layout metadata (x position, bbox, advance, index) per character.
 * When same string is drawn repeatedly across frames, skips string scan and glyph lookup.
 * Default: 0 (disabled) to save ~2612 B BSS. Enable for static UI text (labels, titles).
 * Not useful for full-frame refresh scenarios.
 * Example: Set to 64 with SLOTS=2 for typical UI applications.
 */
#ifndef EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS 0
#endif

/**
 * Font cache options.
 * Number of cache slots for draw-prefix cache.
 * Each slot can cache one string's glyph layout (up to MAX_GLYPHS characters).
 * Default: 0 (disabled). Must be set together with MAX_GLYPHS.
 * Example: Set to 2 for typical UI applications. Total BSS = SLOTS × MAX_GLYPHS × ~20 bytes.
 */
#ifndef EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS 0
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_CANVAS_DEFAULT_H_ */
