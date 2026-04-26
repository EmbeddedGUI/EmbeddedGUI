#ifndef _EGUI_CONFIG_FAST_PATH_DEFAULT_H_
#define _EGUI_CONFIG_FAST_PATH_DEFAULT_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* ---- Image codec / cache policy ---- */

/* Maximum bytes per pixel for image decode buffers. */
#ifndef EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE
#define EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE 2
#endif

/*
 * Image codec fast-draw mode.
 * 0: Disable codec row-cache fast path.
 * 1: Enable full row-band cache fast path.
 * 2: Enable tail-row cache fast path
 * (implies row-band cache path).
 *
 * HelloPerformance/qemu/cortex-m3 recheck on 2026-04-26, Arm GNU 12.2,
 * with EGUI_TEST_CONFIG_IMAGE_565=1:
 * - 0 -> 1:
 * text +4332B, data +0B, bss +16B. The filtered QOI/RLE
 *   run mainly improves EXTERN_IMAGE_QOI_565_8 by about 30%; historical
 *   full-scene A/B also shows
 * tiled QOI/RLE cliffs when row cache is off.
 * - 1 -> 2: text +9376B, data +0B, bss +0B. This is above the 2KB
 *   code-size threshold for folding mode 2
 * into mode 1. Dropping mode 2
 *   back to mode 1 regresses the filtered QOI/RLE alpha subset by about
 *   +59% total, with IMAGE_QOI_565_8 around +159%.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_CODEC_FAST_DRAW
#define EGUI_CONFIG_FUNCTION_IMAGE_CODEC_FAST_DRAW 0
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_CODEC_FAST_DRAW > 2
#error "EGUI_CONFIG_FUNCTION_IMAGE_CODEC_FAST_DRAW must be 0, 1 or 2"
#endif

#define EGUI_IMAGE_CODEC_FAST_DRAW_ROW_CACHE_ENABLED      (EGUI_CONFIG_FUNCTION_IMAGE_CODEC_FAST_DRAW >= 1)
#define EGUI_IMAGE_CODEC_FAST_DRAW_TAIL_ROW_CACHE_ENABLED (EGUI_CONFIG_FUNCTION_IMAGE_CODEC_FAST_DRAW >= 2)

/* Optional full-image persistent cache budget for compressed images. */
#ifndef EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES 0
#endif

/* Optional full-image persistent cache budget for external standard images. */
#ifndef EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES 0
#endif

/* RLE external-resource I/O window size. */
#ifndef EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE
#define EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE 1024
#endif

/*
 * Std-font fast-draw switch.
 * 0: Disable std-font fast draw.
 * 1: Enable std-font fast draw and its small lookup caches.
 */
#ifndef EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW
#define EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW 1
#endif

#if (EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW != 0) && (EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW != 1)
#error "EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW must be 0 or 1"
#endif

/*
 * Font transform fast-draw mode.
 * When 1, enable shared prepare/layout/dimension caches for text transform.
 * Defaults to the std-font fast-draw switch so disabling std-font fast draw
 * also disables its transform caches.
 */
#ifndef EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW
#define EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW (EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW ? 1 : 0)
#endif

#if EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW > 1
#error "EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW must be 0 or 1"
#endif

#if EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW && !EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW
#error "EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW requires EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW"
#endif

/* ---- Text transform cache policy ---- */

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE 1
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT 0
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT 0
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES
#define EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES 4096
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_STACK_MAX_BYTES
#define EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_STACK_MAX_BYTES 0
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE
#define EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE 0
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_GLYPHS
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_GLYPHS 0
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_LINES
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_LINES 0
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS
#define EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS 128
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_LINES
#define EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_LINES 32
#endif

/* Remaining fast-path toggles now stay next to the implementation units that
 * consume them, with optional app-side override bridges. This shared default
 * header keeps only common codec/cache policy knobs.
 */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_FAST_PATH_DEFAULT_H_ */
