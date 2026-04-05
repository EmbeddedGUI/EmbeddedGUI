#ifndef _EGUI_CONFIG_DEFAULT_H_
#define _EGUI_CONFIG_DEFAULT_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* ---- Screen ---- */

/* Width of the screen <8-32767> */
#ifndef EGUI_CONFIG_SCEEN_WIDTH
#define EGUI_CONFIG_SCEEN_WIDTH 240
#endif

/* Height of the screen <8-32767> */
#ifndef EGUI_CONFIG_SCEEN_HEIGHT
#define EGUI_CONFIG_SCEEN_HEIGHT 320
#endif

/* Select the screen colour depth */
#ifndef EGUI_CONFIG_COLOR_DEPTH
#define EGUI_CONFIG_COLOR_DEPTH 16
#endif

/**
 * Color options.
 * Swap the 2 bytes of RGB565 color. Useful if the display has a 8 bit interface (e.g. SPI)
 * and the hardware SPI/DMA controller does NOT support automatic byte-swap.
 *
 * When set to 1, the byte-swap is performed as a bulk in-place pass over the PFB tile
 * inside egui_pfb_manager_start_flush(), immediately before draw_area() is called.
 * This keeps all internal rendering (egui_rgb_mix, EGUI_COLOR_MAKE, etc.) in the
 * normal RGB565 layout, eliminating the green-field split and per-pixel overhead
 * that the old approach (swapped internal layout) imposed.
 *
 * Cost: (PFB_WIDTH * PFB_HEIGHT / 2) uint32 ops per tile flush — typically <0.1 ms
 * on a 100 MHz Cortex-M0 for a 60x60 PFB.
 */
#ifndef EGUI_CONFIG_COLOR_16_SWAP
#define EGUI_CONFIG_COLOR_16_SWAP 0
#endif

/* ---- PFB (Partial Frame Buffer) ---- */

/* For speed, we assume that the PFB width and height are multiples of the screen width and height */

/* Width of the PFB block, suggest be a divisor of EGUI_CONFIG_SCEEN_WIDTH */
#ifndef EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_WIDTH (EGUI_CONFIG_SCEEN_WIDTH / 8)
#endif

/* Height of the PFB block, suggest be a divisor of EGUI_CONFIG_SCEEN_HEIGHT */
#ifndef EGUI_CONFIG_PFB_HEIGHT
#define EGUI_CONFIG_PFB_HEIGHT (EGUI_CONFIG_SCEEN_HEIGHT / 8)
#endif

/* Default-off logical PFB probe for perf/RAM experiments. */
#ifndef EGUI_CONFIG_CORE_LOGICAL_PFB_PROBE_ENABLE
#define EGUI_CONFIG_CORE_LOGICAL_PFB_PROBE_ENABLE 0
#endif

/* Preferred logical tile width when the probe above is enabled. */
#ifndef EGUI_CONFIG_CORE_LOGICAL_PFB_PROBE_TARGET_WIDTH
#define EGUI_CONFIG_CORE_LOGICAL_PFB_PROBE_TARGET_WIDTH EGUI_CONFIG_PFB_WIDTH
#endif

/**
 * PFB multi-buffer count.
 * Controls how many PFB buffers the ring queue uses:
 *   1 = single buffer, synchronous draw (no DMA overlap)
 *   2 = double buffer, CPU/DMA pipeline depth 1 (default)
 *   3 = triple buffer, CPU can run 2 tiles ahead of DMA
 *   4 = quad buffer, CPU can run 3 tiles ahead of DMA
 *
 * More buffers smooth out CPU time variance at the cost of RAM.
 * Each buffer costs PFB_WIDTH * PFB_HEIGHT * COLOR_BYTES.
 * Port provides buffers by declaring:
 *   egui_color_int_t pfb[EGUI_CONFIG_PFB_BUFFER_COUNT][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT]
 * and passing it to egui_init(pfb).
 *
 * Requires display driver to implement draw_area for count >= 2.
 * DMA completion ISR must call egui_pfb_notify_flush_complete().
 */
#ifndef EGUI_CONFIG_PFB_BUFFER_COUNT
#define EGUI_CONFIG_PFB_BUFFER_COUNT 2
#endif

/**
 * Software rotation support.
 * When enabled, core can rotate PFB output in software if hardware does not support it.
 * Requires a rotation scratch buffer of PFB size for 90/270 degree rotation.
 */
#ifndef EGUI_CONFIG_SOFTWARE_ROTATION
#define EGUI_CONFIG_SOFTWARE_ROTATION 0
#endif

/* ---- Timing & refresh ---- */

/* Set the maximum FPS. Limit the maximum FPS to reduce the CPU usage. */
#ifndef EGUI_CONFIG_MAX_FPS
#define EGUI_CONFIG_MAX_FPS 60
#endif

/* Set the dirty area count. Limit is 1, use the buffer to reduce need refresh area. */
#ifndef EGUI_CONFIG_DIRTY_AREA_COUNT
#define EGUI_CONFIG_DIRTY_AREA_COUNT 5
#endif

/* Print per-frame dirty region rectangles. */
#ifndef EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL 0
#endif

/* Print dirty source and merge trace details for debugging. */
#ifndef EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE 0
#endif

/* ---- Input ---- */

/* Set the motion cache count, use to save the motion of the input device. */
#ifndef EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT
#define EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT 5
#endif

/**
 * Key input cache count.
 * Number of key events that can be queued.
 */
#ifndef EGUI_CONFIG_INPUT_KEY_CACHE_COUNT
#define EGUI_CONFIG_INPUT_KEY_CACHE_COUNT 5
#endif

/* ---- Params ---- */

/**
 * Params options.
 * For toast default show time.
 */
#ifndef EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME
#define EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME 1000
#endif

/* ---- Performance ---- */

/**
 * Performance options.
 * In some cpu, float is faster than int, so you can use float to improve the performance.
 */
#ifndef EGUI_CONFIG_PERFORMANCE_USE_FLOAT
#define EGUI_CONFIG_PERFORMANCE_USE_FLOAT 0
#endif

/* ---- Function switches ---- */

/**
 * Function options.
 * Select support shadow effect. if 0, disable shadow.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW 0
#endif

/**
 * Function options.
 * Select support touch. if 0, disable touch.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH 1
#endif

/**
 * Function options.
 * Select support multi-touch (pinch-to-zoom, scroll wheel). if 0, disable multi-touch.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#define EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH 0
#endif

/**
 * Touch dispatch options.
 * Maximum captured view depth tracked for an active touch sequence.
 */
#ifndef EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX
#define EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX 32
#endif

/* Multi-touch requires single-touch */
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#undef EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH 1
#endif

/**
 * Core state options.
 * Select whether to keep a separate user-root wrapper under the top-level root group.
 * Default: 0 (disabled). Auto-enabled when activity or dialog is enabled.
 * Debug info no longer needs this because it is drawn directly as an overlay.
 */
#if EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY || EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG
#undef EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
#define EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE 1
#endif

#ifndef EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
#define EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE 0
#endif

/**
 * Function options.
 * Select support key event. if 0, disable key event.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY 0
#endif

/**
 * Function options.
 * Select support focus system. if 0, disable focus.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#define EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS 0
#endif

/* Focus requires key support */
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#undef EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY 1
#endif

/**
 * Function options.
 * Select support view layer for z-ordering. if 0, disable layer (all views draw in insertion order).
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
#define EGUI_CONFIG_FUNCTION_SUPPORT_LAYER 0
#endif

/**
 * Function options.
 * Select support mask module. if 0, disable mask-related APIs and runtime paths by default.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_MASK
#define EGUI_CONFIG_FUNCTION_SUPPORT_MASK 0
#endif

/**
 * Function options.
 * Select support scrollbar indicator for scrollable views. if 0, disable scrollbar.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
#define EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR 1
#endif

/* ---- Resource management ---- */

/**
 * Function options.
 * Select support external resource manager.
 */
#ifndef EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 0
#endif

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
/* If external resource is enabled, must enable resource manager. */
#undef EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
#define EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER 1
#endif /* EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE */

/**
 * Function options.
 * Select support app resource manager.
 */
#ifndef EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
#define EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER 0
#endif

/* ---- Image format switches ---- */

/**
 * Image format options.
 * Enable/disable specific image format support to reduce code size.
 * Keep only RGB565 and RGB565_4 enabled by default.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32 0
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1 0
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2 0
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8 0
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1 0
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2 0
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4 0
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8 0
#endif

/* ---- Image codec (compression) ---- */

/**
 * Enable QOI (Quite OK Image) codec for compressed image decoding.
 * QOI supports RGB565 and RGB32 formats with optional alpha.
 */
#ifndef EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE 0
#endif

/**
 * Enable RLE (Run-Length Encoding) codec for compressed image decoding.
 * RLE supports RGB565, RGB32 and GRAY8 formats with optional alpha.
 */
#ifndef EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE 0
#endif

/**
 * Maximum bytes per pixel for image decode buffers.
 * Keep at 4 for RGB32-capable apps. RGB565-only apps can set this to 2 to save RAM.
 */
#ifndef EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE
#define EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE 2
#endif

/**
 * Decode row buffer width (pixels). Used by compressed image codecs
 * as temporary storage for one decoded row. Default = screen width.
 */
#ifndef EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH
#define EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH EGUI_CONFIG_SCEEN_WIDTH
#endif

/**
 * Enable row-band decode cache for compressed image codecs.
 * When enabled, the first PFB tile in a row band decodes all rows into
 * a cache; subsequent horizontal tiles blend from cache without re-decoding.
 * Eliminates N-1 redundant decode passes (N = screen_width / pfb_width).
 *
 * RAM cost: PFB_HEIGHT * DECODE_ROW_BUF_WIDTH * (4 + 1) bytes.
 * Example: PFB_H=30, W=240 → 30×240×5 = 36,000 bytes.
 * Only enable on platforms with sufficient RAM.
 */
#ifndef EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE 0
#endif

/**
 * Codec image-mask row-block fast path options.
 * When 1, QOI/RLE masked decode paths may batch image-mask RGB565 /
 * RGB565_8 rows through the dedicated mask-image row-block helpers.
 * When 0, codec image-mask draws fall back to the existing per-row
 * image-mask blend path while keeping the rest of the codec pipeline
 * unchanged.
 */
#ifndef EGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE 1
#endif

/**
 * Reuse the first compressed-image row-cache row as the temporary
 * "all opaque alpha" scratch buffer for masked RGB565 blend fallbacks.
 */
#ifndef EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE
#define EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE 0
#endif

/**
 * QOI decoder checkpoint count.
 * Each slot stores a full decoder state for restoring recent row bands.
 * Must be a power of two.
 */
#ifndef EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT
#define EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT 2
#endif

/**
 * Optional persistent full-image cache for compressed image codecs.
 * Stores one fully decoded compressed image across refreshes and repeated draws.
 * Total RAM budget in bytes for pixel + alpha buffers; 0 disables the feature.
 */
#ifndef EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES 0
#endif

/**
 * Optional persistent full-image cache for external standard images.
 * Stores one external raw image in RAM across refreshes and repeated draws.
 * Total RAM budget in bytes for data + alpha buffers; 0 disables the feature.
 */
#ifndef EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES 0
#endif

/**
 * External raw-image row cache data budget in bytes.
 * Shared by the standard external image draw/resize path and the transform path.
 * Default: 2 rows of RGB565 data.
 */
#ifndef EGUI_CONFIG_IMAGE_EXTERNAL_DATA_CACHE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_EXTERNAL_DATA_CACHE_MAX_BYTES (EGUI_CONFIG_SCEEN_WIDTH * 2 * 2)
#endif

/**
 * External raw-image row cache alpha budget in bytes.
 * Shared by the standard external image draw/resize path and the transform path.
 * Default: 2 rows of alpha data.
 */
#ifndef EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES (EGUI_CONFIG_SCEEN_WIDTH * 2)
#endif

/* ---- Reduce code/ram size ---- */

/**
 * Reduce code size options.
 * Use generic get_pixel function pointer instead of per-format specialized code.
 */
#ifndef EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE
#define EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE 0
#endif

/**
 * Image draw options.
 * When 1, keep the specialized std-image draw/resize fast paths.
 * When 0, fall back to the generic get_pixel-based path to reduce code size.
 *
 * Backward compatibility:
 * EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE=1 implies this macro defaults to 0
 * unless the application overrides it explicitly.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE
#if EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE
#define EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE 0
#else
#define EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE 1
#endif
#endif

/**
 * Image direct-draw alpha options.
 * When 1, keep the specialized std-image direct draw fast paths for
 * RGB565_1/2/4/8 alpha images.
 * When 0, alpha direct draw falls back to the generic get_pixel-based path
 * while raw RGB565 direct draw fast paths remain available.
 *
 * This macro is only effective when EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE=1.
 * By default it follows the std-image fast draw switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE
#define EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE
#endif

/**
 * Image direct-draw alpha8 options.
 * When 1, keep the specialized std-image direct draw fast path for
 * RGB565_8 alpha images.
 * When 0, RGB565_8 direct draw falls back to the generic get_pixel-based path
 * while raw RGB565 and RGB565_1/2/4 direct-draw fast paths remain available.
 *
 * This macro is only effective when EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE=1.
 * By default it follows the std-image alpha direct-draw switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA8_ENABLE
#define EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA8_ENABLE EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE
#endif

/**
 * Image direct-draw packed-alpha options.
 * When 1, keep the specialized std-image direct draw fast paths for
 * RGB565_1/2/4 packed-alpha images.
 * When 0, packed-alpha direct draw falls back to the generic get_pixel-based
 * path while raw RGB565 and RGB565_8 direct draw fast paths remain available.
 *
 * This macro is only effective when EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE=1.
 * By default it follows the std-image alpha direct-draw switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_FAST_DRAW_PACKED_ALPHA_ENABLE
#define EGUI_CONFIG_IMAGE_STD_FAST_DRAW_PACKED_ALPHA_ENABLE EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE
#endif

/**
 * RGB565 opaque-source check options.
 * When 1, std-image scans RGB565 alpha data and promotes fully opaque sources
 * back to the raw RGB565 fast paths.
 * When 0, only RGB565 images without alpha data are treated as opaque sources.
 *
 * This option also affects `egui_image_std_rgb565_is_opaque_source()` callers
 * outside the direct draw path, such as transform helpers.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_RGB565_OPAQUE_SOURCE_CHECK_ENABLE
#define EGUI_CONFIG_IMAGE_STD_RGB565_OPAQUE_SOURCE_CHECK_ENABLE 1
#endif

/**
 * External alpha fast path options.
 * When 1, the child switches below default to on for external RGB565_8 and
 * packed-alpha std-image fast paths.
 * When 0, the child switches below default to off unless the application
 * overrides them explicitly.
 *
 * This option is only effective when the corresponding std-image alpha fast
 * path is enabled and external resources are enabled.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE 1
#endif

/**
 * External alpha8 fast path options.
 * When 1, keep the specialized std-image external-resource fast paths for
 * RGB565_8 direct draw and resize.
 * When 0, external RGB565_8 draw/resize falls back to the generic
 * get_pixel-based path while packed-alpha external fast paths can still stay
 * enabled independently.
 *
 * By default this follows the umbrella external-alpha switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA8_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA8_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE
#endif

/**
 * External packed-alpha fast path options.
 * When 1, keep the specialized std-image external-resource fast paths for
 * RGB565_1/2/4 direct draw and resize.
 * When 0, external RGB565_1/2/4 draw/resize falls back to the generic
 * get_pixel-based path while internal packed-alpha and external RGB565_8 fast
 * paths remain available.
 *
 * By default this follows the umbrella external-alpha switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE
#endif

/**
 * External packed-alpha resize fast path options.
 * When 1, keep the specialized std-image external-resource resize fast paths
 * for RGB565_1/2/4 images.
 * When 0, external RGB565_1/2/4 resize falls back to the generic
 * get_pixel-based path while direct-draw fast paths can still stay enabled
 * independently.
 *
 * By default this follows the umbrella external packed-alpha switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_RESIZE_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_RESIZE_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE
#endif

/**
 * Mask-shape image fast path umbrella options.
 * When 1, circle / round-rectangle std-image mask fast paths default to on.
 * When 0, the child switches below default to off unless overridden
 * explicitly by the application.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE 1
#endif

/**
 * Circle mask image fast path options.
 * When 1, keep the specialized std-image masked fast paths for circle masks.
 * These helpers are also shared by QOI/RLE decode rows.
 * When 0, circle-masked image draw/resize paths fall back to the generic
 * mask_point / row-range based path to reduce code size.
 *
 * By default this follows the umbrella mask-shape switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE
#endif

/**
 * Circle mask image resize fast path options.
 * When 1, keep the specialized std-image resize fast paths for circle masks.
 * When 0, circle-masked image resize falls back to the generic row-range /
 * mask-point based resize path while direct-draw circle fast paths can still
 * stay enabled independently.
 *
 * By default this follows the circle mask-image switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE
#endif

/**
 * Circle mask image alpha8 resize fast path options.
 * When 1, keep the specialized std-image resize fast path for
 * circle-masked RGB565_8 sources.
 * When 0, raw RGB565 circle resize fast paths can still stay enabled
 * independently while alpha8 resize falls back to the generic
 * masked-row / mapped-row path.
 *
 * By default this follows the circle resize switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE
#endif

/**
 * Circle mask image rgb565 resize fast path options.
 * When 1, keep the specialized std-image resize fast path for
 * raw RGB565 circle-masked sources.
 * When 0, raw RGB565 circle resize falls back to the generic
 * masked-row / mapped-row path while alpha8 resize fast paths can still
 * stay enabled independently.
 *
 * By default this follows the circle resize switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_RGB565_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_RGB565_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE
#endif

/**
 * Circle mask image direct-draw fast path options.
 * When 1, keep the specialized std-image direct-draw masked-row fast paths
 * for circle masks, including row/block helpers and direct-draw segment
 * handling.
 * When 0, circle-masked direct draw falls back to the generic masked-row /
 * masked-point path while resize fast paths can still stay enabled
 * independently.
 *
 * By default this follows the circle mask-image switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE
#endif

/**
 * Circle mask image RGB565 direct-draw fast path options.
 * When 1, keep the specialized std-image direct-draw masked-row fast paths
 * for non-alpha8 circle RGB565 sources, including row/block helpers and
 * direct-draw segment handling.
 * When 0, alpha8 circle direct-draw fast paths can still stay enabled
 * independently while non-alpha8 direct draw falls back to the generic
 * masked-row / masked-point path.
 *
 * By default this follows the circle direct-draw switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_RGB565_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_RGB565_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE
#endif

/**
 * Circle mask image alpha8 direct-draw fast path options.
 * When 1, keep the specialized std-image direct-draw fast path for
 * circle-masked RGB565_8 sources.
 * When 0, raw RGB565 circle direct-draw fast paths can still stay enabled
 * independently while alpha8 direct draw falls back to the generic
 * masked-row / masked-point path.
 *
 * By default this follows the circle direct-draw switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_ALPHA8_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_ALPHA8_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE
#endif

/**
 * Round-rectangle mask image fast path options.
 * When 1, keep the specialized std-image masked fast paths for
 * round-rectangle masks. These helpers are also shared by QOI/RLE decode
 * rows.
 * When 0, round-rectangle masked image draw/resize paths fall back to the
 * generic mask_point / row-range based path to reduce code size.
 *
 * By default this follows the umbrella mask-shape switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE
#endif

/**
 * Round-rectangle mask image resize fast path options.
 * When 1, keep the specialized std-image resize fast paths for
 * round-rectangle masks.
 * When 0, round-rectangle masked image resize falls back to the generic
 * row-range / mask-point based resize path while direct-draw round-rect fast
 * paths can still stay enabled independently.
 *
 * By default this follows the round-rectangle mask-image switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE
#endif

/**
 * Round-rectangle mask image alpha8 resize fast path options.
 * When 1, keep the specialized std-image resize fast path for
 * round-rectangle RGB565_8 sources.
 * When 0, raw RGB565 round-rectangle resize fast paths can still stay
 * enabled independently while alpha8 resize falls back to the generic
 * masked-row / mapped-row path.
 *
 * By default this follows the round-rectangle resize switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_ALPHA8_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_ALPHA8_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE
#endif

/**
 * Round-rectangle mask image rgb565 resize fast path options.
 * When 1, keep the specialized std-image resize fast path for
 * round-rectangle raw RGB565 sources.
 * When 0, round-rectangle raw RGB565 resize falls back to the generic
 * masked-row / mapped-row path while alpha8 resize fast paths can still
 * stay enabled independently.
 *
 * By default this follows the round-rectangle resize switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_RGB565_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_RGB565_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE
#endif

/**
 * Round-rectangle mask image direct-draw fast path options.
 * When 1, keep the specialized std-image direct-draw masked-row fast paths
 * for round-rectangle masks, including row/block helpers and direct-draw
 * segment handling.
 * When 0, round-rectangle masked direct draw falls back to the generic
 * masked-row / masked-point path while resize fast paths can still stay
 * enabled independently.
 *
 * By default this follows the round-rectangle mask-image switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE
#endif

/**
 * Round-rectangle mask image RGB565 direct-draw fast path options.
 * When 1, keep the specialized std-image direct-draw masked-row fast paths
 * for non-alpha8 round-rectangle RGB565 sources, including row/block helpers
 * and direct-draw segment handling.
 * When 0, alpha8 round-rectangle direct-draw fast paths can still stay
 * enabled independently while non-alpha8 direct draw falls back to the
 * generic masked-row / masked-point path.
 *
 * By default this follows the round-rectangle direct-draw switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_RGB565_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_RGB565_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE
#endif

/**
 * Round-rectangle mask image alpha8 direct-draw fast path options.
 * When 1, keep the specialized std-image direct-draw masked-row fast paths
 * for round-rectangle RGB565_8 sources, including row/block alpha8 helpers.
 * When 0, raw RGB565 round-rectangle direct-draw fast paths can still stay
 * enabled independently while alpha8 direct draw falls back to the generic
 * masked-row / masked-point path.
 *
 * By default this follows the round-rectangle direct-draw switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE
#endif

/**
 * Row-overlay mask fast path options.
 * When 1, std-image keeps specialized branches for masks that expose
 * `mask_get_row_overlay()` (for example vertical gradient overlays).
 * When 0, these branches fall back to the generic per-point mask path to
 * reduce code size.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE 1
#endif

/**
 * Non-raw row-overlay mask fast path options.
 * When 1, std-image keeps the row-overlay fast paths used by non-raw RGB565
 * branches such as generic get-pixel draw/resize and RGB565_8 alpha
 * draw/resize.
 * When 0, these branches fall back to the generic per-point mask path while
 * the raw RGB565 row-overlay fast paths can still stay enabled independently.
 *
 * By default this follows the row-overlay umbrella switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE
#endif

/**
 * Alpha8 row-overlay mask fast path options.
 * When 1, std-image keeps the row-overlay fast paths used by RGB565_8 alpha
 * draw/resize helpers.
 * When 0, these branches fall back to the generic per-point mask path while
 * generic get-pixel row-overlay fast paths can still stay enabled independently
 * under the parent non-rgb565 switch.
 *
 * By default this follows the non-rgb565 row-overlay switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_ALPHA8_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_ALPHA8_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE
#endif

/**
 * Raw RGB565 row-overlay mask fast path options.
 * When 1, std-image keeps the row-overlay fast paths used by raw RGB565
 * direct draw/resize branches.
 * When 0, these branches fall back to the generic per-point mask path while
 * non-raw row-overlay fast paths can still stay enabled independently.
 *
 * By default this follows the row-overlay umbrella switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_RGB565_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_RGB565_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE
#endif

/**
 * Mask row-range fast path umbrella options.
 * When 1, std-image row-range masked draw/resize paths default to keeping the
 * child switches below on for `PARTIAL` and `INSIDE` rows reported by
 * `mask_get_row_range()`.
 * When 0, the child switches below default to off unless overridden
 * explicitly by the application.
 *
 * `OUTSIDE` row skip remains available because it is behavior-neutral and has
 * very low code-size cost.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_RANGE_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_RANGE_FAST_PATH_ENABLE 1
#endif

/**
 * Mask row-partial fast path options.
 * When 1, keep the partial-row acceleration used after `mask_get_row_range()`
 * reports a row with masked edge spans plus an unmasked middle segment.
 * When 0, these partial rows fall back to the generic per-pixel masked image
 * path while `OUTSIDE` skip and optional `INSIDE` acceleration can still stay
 * on.
 *
 * By default this follows the std-image masked row-range umbrella switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROW_RANGE_FAST_PATH_ENABLE
#endif

/**
 * Mask row-partial draw child options.
 * When 1, keep the partial-row acceleration used by masked direct-draw image
 * paths after `mask_get_row_range()` reports masked edge spans plus an
 * unmasked middle segment.
 * When 0, only the draw-side partial-row helpers fall back to the generic
 * per-pixel masked draw path while resize-side partial-row acceleration can
 * stay on.
 *
 * By default this follows the std-image masked row-partial switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE
#endif

/**
 * Mask row-partial alpha8 draw child options.
 * When 1, keep the partial-row acceleration used by masked RGB565_8 direct
 * draw image paths after `mask_get_row_range()` reports masked edge spans plus
 * an unmasked middle segment.
 * When 0, only the RGB565_8 draw-side partial-row helpers fall back to the
 * generic per-pixel masked draw path while packed-alpha draw partial-row
 * acceleration can still stay enabled independently.
 *
 * By default this follows the std-image masked row-partial draw switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_ALPHA8_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_ALPHA8_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE
#endif

/**
 * Mask row-partial packed-alpha draw child options.
 * When 1, keep the partial-row acceleration used by masked packed-alpha
 * RGB565_4 / RGB565_2 / RGB565_1 direct draw image paths after
 * `mask_get_row_range()` reports masked edge spans plus an unmasked middle
 * segment.
 * When 0, only the packed-alpha draw-side partial-row helpers fall back to the
 * generic per-pixel masked draw path while RGB565_8 alpha8 draw partial-row
 * acceleration can still stay enabled independently.
 *
 * By default this follows the std-image masked row-partial draw switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_PACKED_ALPHA_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_PACKED_ALPHA_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE
#endif

/**
 * Mask row-partial resize child options.
 * When 1, keep the partial-row acceleration used by masked image-resize paths
 * after `mask_get_row_range()` reports masked edge spans plus an unmasked
 * middle segment.
 * When 0, only the resize-side partial-row helpers fall back to the generic
 * masked resize path while draw-side partial-row acceleration can stay on.
 *
 * By default this follows the std-image masked row-partial switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE
#endif

/**
 * Mask row-partial alpha8 resize child options.
 * When 1, keep the partial-row acceleration used by masked RGB565_8 image
 * resize paths after `mask_get_row_range()` reports masked edge spans plus an
 * unmasked middle segment.
 * When 0, only the RGB565_8 resize-side partial-row helpers fall back to the
 * generic masked resize path while raw RGB565 / packed-alpha partial-row
 * resize acceleration can still stay enabled independently.
 *
 * By default this follows the std-image masked row-partial resize switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE
#endif

/**
 * Mask row-partial packed-alpha resize child options.
 * When 1, keep the partial-row acceleration used by masked packed-alpha
 * RGB565_4 / RGB565_2 / RGB565_1 image-resize paths after
 * `mask_get_row_range()` reports masked edge spans plus an unmasked middle
 * segment.
 * When 0, only the packed-alpha resize-side partial-row helpers fall back to
 * the generic masked resize path while raw RGB565 / RGB565_8 alpha8 resize
 * partial-row acceleration can still stay enabled independently.
 *
 * By default this follows the std-image masked row-partial resize switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE
#endif

/**
 * Mask row-partial raw RGB565 resize child options.
 * When 1, keep the partial-row acceleration used by masked raw RGB565 image
 * resize paths after `mask_get_row_range()` reports masked edge spans plus an
 * unmasked middle segment.
 * When 0, only the raw RGB565 resize-side partial-row helpers fall back to
 * the generic masked resize path while RGB565_8 alpha8 / packed-alpha
 * partial-row resize acceleration can still stay enabled independently.
 *
 * By default this follows the std-image masked row-partial resize switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE
#endif

/**
 * Mask row-inside fast path options.
 * When 1, keep the direct row-blend path used after `mask_get_row_range()`
 * reports the requested row segment is fully inside the mask.
 * When 0, these fully-inside rows fall back to the generic per-pixel masked
 * image path while `OUTSIDE` skip and optional `PARTIAL` acceleration can
 * still stay on.
 *
 * By default this follows the std-image masked row-range umbrella switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROW_RANGE_FAST_PATH_ENABLE
#endif

/**
 * Mask row-inside resize child options.
 * When 1, keep the inside-row acceleration used by masked image-resize paths
 * after `mask_get_row_range()` reports the requested row segment is fully
 * inside the mask.
 * When 0, only the resize-side inside-row helpers fall back to the generic
 * masked resize path while draw-side inside-row acceleration can stay on.
 *
 * By default this follows the std-image masked row-inside switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE
#endif

/**
 * Mask row-inside alpha8 resize child options.
 * When 1, keep the inside-row acceleration used by masked RGB565_8 image
 * resize paths after `mask_get_row_range()` reports the requested row segment
 * is fully inside the mask.
 * When 0, only the RGB565_8 resize-side inside-row helpers fall back to the
 * generic masked resize path while raw RGB565 / packed-alpha inside-row resize
 * acceleration can still stay enabled independently.
 *
 * By default this follows the std-image masked row-inside resize switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE
#endif

/**
 * Mask visible-range fast path options.
 * When 1, std-image partial-row masked draw/resize paths query
 * `mask_get_row_visible_range()` to skip fully invisible edge spans for masks
 * that can report a visible row segment.
 * When 0, these partial-row paths still keep `mask_get_row_range()` based
 * outside/inside acceleration, but fall back to scanning the full requested
 * edge spans to reduce code size.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_FAST_PATH_ENABLE 1
#endif

/**
 * Mask-image identity-scale fast path umbrella options.
 * When 1, the child switches below default to on for the specialized
 * identity-scale helpers used by image-mask solid fill, masked blend segment
 * paths, and masked blend row-block paths.
 * When 0, the child switches below default to off unless the application
 * overrides them explicitly.
 */
#ifndef EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_FAST_PATH_ENABLE
#define EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_FAST_PATH_ENABLE 1
#endif

/**
 * Mask-image identity-scale blend fast path options.
 * When 1, keep the specialized identity-scale helpers used by masked
 * RGB565 / RGB565+alpha segment blend paths and internal RGB565/QOI/RLE
 * row-block blend paths.
 * When 0, these masked-image blend paths fall back to the generic
 * scaled/mapped or per-row segment loops while solid-fill identity helpers
 * can still stay enabled.
 *
 * By default this follows the mask-image identity-scale umbrella switch.
 */
#ifndef EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_BLEND_FAST_PATH_ENABLE
#define EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_BLEND_FAST_PATH_ENABLE EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_FAST_PATH_ENABLE
#endif

/**
 * Image resize options.
 * When 1, keep the specialized std-image resize fast paths.
 * When 0, std-image resize falls back to the generic get_pixel-based path
 * while direct draw fast paths remain available.
 *
 * This macro is only effective when EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE=1.
 * By default it follows the std-image fast draw switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE
#define EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE
#endif

/**
 * Image resize alpha options.
 * When 1, keep the specialized std-image alpha resize fast paths.
 * When 0, alpha resize falls back to the generic get_pixel-based path while
 * raw RGB565 resize fast paths remain available.
 *
 * This macro is only effective when EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE=1.
 * By default it follows the std-image resize fast draw switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE
#define EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE
#endif

/**
 * Image resize alpha8 options.
 * When 1, keep the specialized std-image resize fast path for
 * RGB565_8 alpha images.
 * When 0, RGB565_8 resize falls back to the generic get_pixel-based path
 * while raw RGB565 and RGB565_1/2/4 resize fast paths remain available.
 *
 * This macro is only effective when EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE=1.
 * By default it follows the std-image alpha resize switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA8_ENABLE
#define EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA8_ENABLE EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE
#endif

/**
 * Image resize packed-alpha options.
 * When 1, keep the specialized std-image resize fast paths for
 * RGB565_1/2/4 packed-alpha images.
 * When 0, packed-alpha resize falls back to the generic get_pixel-based path
 * while raw RGB565 and RGB565_8 resize fast paths remain available.
 *
 * This macro is only effective when EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE=1.
 * By default it follows the std-image alpha resize switch.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_PACKED_ALPHA_ENABLE
#define EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_PACKED_ALPHA_ENABLE EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE
#endif

/**
 * Alpha-color image draw options.
 * When 1, keep the specialized std-image draw/resize fast paths used by
 * alpha-only images rendered with a solid tint color.
 * When 0, these paths fall back to generic get_point/get_point_resize loops to
 * reduce code size while raw RGB565 image draw/resize fast paths remain
 * available.
 */
#ifndef EGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE
#define EGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE 1
#endif

/**
 * Reduce code size options.
 * Select support view margin/padding APIs. if 0, setters become no-op and layout stays at zero margin/padding by default.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
#define EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING 0
#endif

/**
 * Reduce code/ram size options.
 * Select limit margin/padding size, max to -128~127. outherwise, use the egui_dim_t type.
 */
#ifndef EGUI_CONFIG_REDUCE_MARGIN_PADDING_SIZE
#define EGUI_CONFIG_REDUCE_MARGIN_PADDING_SIZE 1
#endif

/* ---- Recording ---- */

/**
 * Recording test options.
 * Enable auto-click simulation during GIF recording for demo purposes.
 * Each app can define custom click positions by implementing egui_port_get_recording_click().
 */
#ifndef EGUI_CONFIG_RECORDING_TEST
#define EGUI_CONFIG_RECORDING_TEST 0
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_DEFAULT_H_ */
