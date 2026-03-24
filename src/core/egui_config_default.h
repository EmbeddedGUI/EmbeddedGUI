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

/* ---- Reduce code/ram size ---- */

/**
 * Reduce code size options.
 * Use generic get_pixel function pointer instead of per-format specialized code.
 */
#ifndef EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE
#define EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE 0
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
