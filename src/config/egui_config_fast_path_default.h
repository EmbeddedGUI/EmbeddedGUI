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
 * 2: Enable tail-row cache fast path (implies row-band cache path).
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
 * Std-font fast-draw mode.
 * 0: Disable std-font fast draw.
 * 1: Enable std-font fast draw only.
 * 2: Enable std-font fast draw and ASCII direct lookup cache.
 */
#ifndef EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW
#define EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW 1
#endif

#if EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW > 2
#error "EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW must be 0, 1 or 2"
#endif

#define EGUI_FONT_STD_FAST_DRAW_ENABLED              (EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW >= 1)
#define EGUI_FONT_STD_FAST_DRAW_ASCII_LOOKUP_ENABLED (EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW >= 2)

/*
 * Font transform fast-draw mode.
 * When 1, enable shared prepare/layout/dimension caches for text transform.
 * This is an effective child of EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW.
 */
#ifndef EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW
#define EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW 1
#endif

#if EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW > 1
#error "EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW must be 0 or 1"
#endif

#define EGUI_FONT_TRANSFORM_FAST_DRAW_ENABLED (EGUI_FONT_STD_FAST_DRAW_ENABLED && EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW)

/*
 * Per-frame cache release hooks.
 * Keep these enabled unless the application never touches the corresponding
 * subsystem and wants to trim the frame-end cleanup path.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_STD_FRAME_CACHE_RELEASE
#define EGUI_CONFIG_FUNCTION_IMAGE_STD_FRAME_CACHE_RELEASE 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_CANVAS_TRANSFORM_FRAME_CACHE_RELEASE
#define EGUI_CONFIG_FUNCTION_CANVAS_TRANSFORM_FRAME_CACHE_RELEASE 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_FONT_STD_FRAME_CACHE_RELEASE
#define EGUI_CONFIG_FUNCTION_FONT_STD_FRAME_CACHE_RELEASE 1
#endif

/*
 * Optional lookup cache sub-toggles.
 * Keep these at their defaults unless doing targeted framework A/B work.
 * Font cache sub-toggles are effective only when std-font fast draw is enabled.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_STD_RGB565_ALPHA_OPAQUE_CACHE
#define EGUI_CONFIG_FUNCTION_IMAGE_STD_RGB565_ALPHA_OPAQUE_CACHE 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_FONT_STD_CODE_LOOKUP_CACHE
#define EGUI_CONFIG_FUNCTION_FONT_STD_CODE_LOOKUP_CACHE 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_TEXT_TRANSFORM_SIZE_CACHE
#define EGUI_CONFIG_FUNCTION_TEXT_TRANSFORM_SIZE_CACHE 1
#endif

#define EGUI_FONT_STD_CODE_LOOKUP_CACHE_ENABLED (EGUI_FONT_STD_FAST_DRAW_ENABLED && EGUI_CONFIG_FUNCTION_FONT_STD_CODE_LOOKUP_CACHE)
#define EGUI_TEXT_TRANSFORM_SIZE_CACHE_ENABLED  (EGUI_FONT_STD_FAST_DRAW_ENABLED && EGUI_CONFIG_FUNCTION_TEXT_TRANSFORM_SIZE_CACHE)

/* Remaining fast-path toggles now stay next to the implementation units that
 * consume them, with optional app-side override bridges. This shared default
 * header keeps only common codec/cache policy knobs.
 */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_FAST_PATH_DEFAULT_H_ */
