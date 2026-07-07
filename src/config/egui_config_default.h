#ifndef _EGUI_CONFIG_DEFAULT_H_
#define _EGUI_CONFIG_DEFAULT_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* ---- Screen ---- */

/* Width of the screen <8-32767> */
#ifndef EGUI_CONFIG_SCREEN_WIDTH
#define EGUI_CONFIG_SCREEN_WIDTH 240
#endif

/* Height of the screen <8-32767> */
#ifndef EGUI_CONFIG_SCREEN_HEIGHT
#define EGUI_CONFIG_SCREEN_HEIGHT 320
#endif

/* Select the screen colour depth */
#ifndef EGUI_CONFIG_COLOR_DEPTH
#define EGUI_CONFIG_COLOR_DEPTH 16
#endif

/**
 * Color options.
 * Default runtime value for RGB565 byte-swap on the primary display helpers.
 * Useful if the display has a 8 bit interface (e.g. SPI) and the hardware
 * SPI/DMA controller does NOT support automatic byte-swap.
 *
 * When the runtime flag is enabled, the byte-swap is performed as a bulk in-place
 * pass over the PFB tile inside egui_pfb_manager_start_flush(), immediately before
 * draw_area() is called. This keeps all internal rendering (egui_rgb_mix,
 * EGUI_COLOR_MAKE, etc.) in the normal RGB565 layout, eliminating the green-field
 * split and per-pixel overhead that the old approach (swapped internal layout)
 * imposed.
 *
 * Cost: (PFB_WIDTH * PFB_HEIGHT / 2) uint32 ops per tile flush — typically <0.1 ms
 * on a 100 MHz Cortex-M0 for a 60x60 PFB.
 *
 * Multi-display setups can override this per core via egui_display_setup_t.render_config.
 */
#ifndef EGUI_CONFIG_COLOR_16_SWAP
#define EGUI_CONFIG_COLOR_16_SWAP 0
#endif

/* ---- PFB (Partial Frame Buffer) ---- */

/* Divisor-sized PFB tiles are usually the best default, but edge tiles can also be handled safely. */

/* Width of the PFB block, suggested to be a divisor of EGUI_CONFIG_SCREEN_WIDTH */
#ifndef EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_WIDTH (EGUI_CONFIG_SCREEN_WIDTH / 8)
#endif

/* Height of the PFB block, suggested to be a divisor of EGUI_CONFIG_SCREEN_HEIGHT */
#ifndef EGUI_CONFIG_PFB_HEIGHT
#define EGUI_CONFIG_PFB_HEIGHT (EGUI_CONFIG_SCREEN_HEIGHT / 8)
#endif

#include "egui_config_multi_default.h"

/**
 * Optional attribute suffix for the default PFB buffer declaration.
 * Example:
 *   #define EGUI_CONFIG_PFB_BUFFER_SECTION_ATTR __attribute__((section(".bss.pfb_area")))
 */
#ifndef EGUI_CONFIG_PFB_BUFFER_SECTION_ATTR
#define EGUI_CONFIG_PFB_BUFFER_SECTION_ATTR
#endif

/**
 * Default PFB buffer declaration.
 * Users can override this macro for compiler-specific section placement or
 * custom storage class requirements.
 */
#ifndef EGUI_CONFIG_PFB_BUFFER_DECLARE
#define EGUI_CONFIG_PFB_BUFFER_DECLARE(_name)                                                                                                                  \
    static egui_color_int_t _name[EGUI_CONFIG_PFB_BUFFER_COUNT][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] EGUI_CONFIG_PFB_BUFFER_SECTION_ATTR
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
 *   EGUI_CONFIG_PFB_BUFFER_DECLARE(pfb);
 * and passing it to egui_init(pfb).
 *
 * Requires display driver to implement draw_area for count >= 2.
 * DMA completion ISR must call egui_pfb_notify_flush_complete().
 */
#ifndef EGUI_CONFIG_PFB_BUFFER_COUNT
#define EGUI_CONFIG_PFB_BUFFER_COUNT 2
#endif

/**
 * Compile-time gate for software rotation support on the primary display helpers.
 * The same flag is also used as the default runtime value when
 * render_config is NULL.
 * When the runtime flag is enabled, core can rotate PFB output in software if
 * hardware does not support it. A PFB-sized scratch
 * buffer is required for 90/270
 * degree rotation and is allocated on demand unless the caller provides one.
 *
 * Multi-display setups can override this per
 * core via egui_display_setup_t.render_config.
 */
#ifndef EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
#define EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE 0
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

/* ---- Input ---- */

/* Set the motion cache count, use to save the motion of the input device. */
#ifndef EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT
#define EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT 5
#endif

/**
 * Touch input options.
 * Maximum number of simultaneous touch points captured by the core when
 * multi-touch is enabled. The default matches the common
 * HAL touch data limit.
 */
#ifndef EGUI_CONFIG_TOUCH_MAX_POINTS
#define EGUI_CONFIG_TOUCH_MAX_POINTS 5
#endif

/**
 * Input options.
 * Select support velocity tracker for fling/scroll helpers. if 0, return zero
 * velocity and skip the extra per-core history state.
 */
#ifndef EGUI_CONFIG_FUNCTION_INPUT_VELOCITY_TRACKER
#define EGUI_CONFIG_FUNCTION_INPUT_VELOCITY_TRACKER 0
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

/* ---- Platform service hooks ---- */

/**
 * Platform service options.
 * When enabled, egui_api_malloc/free dispatch through registered platform ops.
 */
#ifndef EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
#define EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC 0
#endif

/**
 * When platform custom malloc hooks are enabled, keep the libc malloc/free
 * fallback path available if the port does not provide hooks at runtime.
 * Disable this for ports that always register valid malloc/free callbacks and
 * want to avoid linking the libc allocator into small demos.
 */
#ifndef EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC_LIBC_FALLBACK
#define EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC_LIBC_FALLBACK 1
#endif

/**
 * Platform service options.
 * When enabled, egui_api_log dispatches through registered platform ops.
 */
#ifndef EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF
#define EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF 0
#endif

/**
 * Enable %f support in egui_api_sprintf().
 * Disabled by default to avoid pulling floating-point formatting code into
 * size-sensitive embedded builds.
 */
#ifndef EGUI_CONFIG_PLATFORM_CUSTOM_SPRINTF_FLOAT
#define EGUI_CONFIG_PLATFORM_CUSTOM_SPRINTF_FLOAT 0
#endif

/**
 * Platform service options.
 * When enabled, egui_api_memset/memcpy dispatch through registered platform ops
 * when the corresponding callbacks are provided.
 */
#ifndef EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP
#define EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP 0
#endif

/* ---- Function switches ---- */

/**
 * Function options.
 * Select support activity stack helpers. if 0, disable activity-specific helper bridges in common widgets.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY
#define EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY 0
#endif

/**
 * Function options.
 * Select support dialog helpers. if 0, disable dialog-specific helper bridges in common widgets.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG
#define EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG 0
#endif

/**
 * Dialog helper options.
 * Standard message box helper built on top of egui_dialog_t.
 */
#ifndef EGUI_CONFIG_FUNCTION_MSGBOX
#define EGUI_CONFIG_FUNCTION_MSGBOX 0
#endif

/**
 * Dialog helper options.
 * Maximum number of buttons one message box instance can expose.
 */
#ifndef EGUI_CONFIG_MSGBOX_MAX_BUTTONS
#define EGUI_CONFIG_MSGBOX_MAX_BUTTONS 3
#endif

/**
 * Function options.
 * Select support toast helpers. if 0, disable toast-specific helper bridges in common widgets.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_TOAST
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOAST 0
#endif

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

/**
 * Touch dispatch options.
 * Select full ancestor-path capture tracking for nested/interceptable groups.
 * Set 0 to use a lighter single-target capture path for simple click-only UIs.
 */
#ifndef EGUI_CONFIG_FUNCTION_VIEW_GROUP_TOUCH_CAPTURE_PATH
#define EGUI_CONFIG_FUNCTION_VIEW_GROUP_TOUCH_CAPTURE_PATH 1
#endif

/**
 * Core pre-work options.
 * Select whether each frame should run the scroll prepass before layout.
 * Set 0 for apps that do not use any scroll/fling/viewpage style widgets.
 */
#ifndef EGUI_CONFIG_FUNCTION_CORE_PRE_COMPUTE_SCROLL
#define EGUI_CONFIG_FUNCTION_CORE_PRE_COMPUTE_SCROLL 1
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
#ifndef EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
#if EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY || EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG
#define EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE 1
#else
#define EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE 0
#endif
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
 * Focus traversal options.
 * Maximum number of focusable views collected during one directional or tab navigation pass.
 */
#ifndef EGUI_CONFIG_FOCUS_MAX_FOCUSABLE_VIEWS
#define EGUI_CONFIG_FOCUS_MAX_FOCUSABLE_VIEWS 32
#endif

/**
 * Focus traversal options.
 * Maximum stack depth used by the embedded DFS focus collector.
 */
#ifndef EGUI_CONFIG_FOCUS_DFS_MAX_DEPTH
#define EGUI_CONFIG_FOCUS_DFS_MAX_DEPTH 16
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
 * Enable dirty passthrough support for structural containers. if 0,
 * egui_view_set_dirty_passthrough() is a no-op and normal container dirty
 * regions are used.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
#define EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH 0
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

#ifndef EGUI_CONFIG_EXTERNAL_RESOURCE_SHARED_DISPLAY_BUS
#define EGUI_CONFIG_EXTERNAL_RESOURCE_SHARED_DISPLAY_BUS 1
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

/**
 * Function options.
 * Select support runtime image files decoded by example-side decoders.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FILE
#define EGUI_CONFIG_FUNCTION_IMAGE_FILE 0
#endif

/**
 * Runtime file-image decoder registry capacity.
 * Used only when EGUI_CONFIG_FUNCTION_IMAGE_FILE is enabled.
 */
#ifndef EGUI_CONFIG_IMAGE_FILE_DECODER_MAX_COUNT
#define EGUI_CONFIG_IMAGE_FILE_DECODER_MAX_COUNT 4
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
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_CODEC_QOI
#define EGUI_CONFIG_FUNCTION_IMAGE_CODEC_QOI 0
#endif

/**
 * Enable RLE (Run-Length Encoding) codec for compressed image decoding.
 * RLE supports RGB565, RGB32 and GRAY8 formats with optional alpha.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_CODEC_RLE
#define EGUI_CONFIG_FUNCTION_IMAGE_CODEC_RLE 0
#endif

/**
 * Enable runtime SVG parsing and rendering.
 * The runtime SVG path delegates parsing and rasterization to the
 * vendored PlutoSVG/PlutoVG stack.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG 0
#endif

/*
 * Fast-path and codec/cache policy defaults now live in
 * egui_config_fast_path_default.h. Keep only the shared buffer budgets here.
 */

/**
 * Decode row buffer width (pixels). Used by compressed image codecs
 * as temporary storage for one decoded row. Default = screen width.
 */
#ifndef EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH
#define EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH EGUI_CONFIG_SCREEN_WIDTH
#endif

/**
 * External raw-image row cache data budget in bytes.
 * Shared by the standard external image draw/resize path and the transform path.
 * Default: 2 rows of RGB565 data.
 */
#ifndef EGUI_CONFIG_IMAGE_EXTERNAL_DATA_CACHE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_EXTERNAL_DATA_CACHE_MAX_BYTES (EGUI_CONFIG_SCREEN_WIDTH * 2 * 2)
#endif

/**
 * External raw-image row cache alpha budget in bytes.
 * Shared by the standard external image draw/resize path and the transform path.
 * Default: 2 rows of alpha data.
 */
#ifndef EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES (EGUI_CONFIG_SCREEN_WIDTH * 2)
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
 * When 1, labels render/measure only through the built-in compact ASCII path and
 * ignore runtime font pointers. Enable only for apps that intentionally keep
 * label text ASCII-only and do not need full font rendering in labels.
 */
#ifndef EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_ONLY
#define EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_ONLY 0
#endif

/**
 * Reduce code size options.
 * When 1, labels without a runtime font pointer fall back to the built-in compact
 * ASCII path. Disable only for apps that always provide a real font for labels
 * and want to avoid linking the compact label fallback.
 */
#ifndef EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_FALLBACK
#define EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_FALLBACK 1
#endif

/* Std-image fast-path defaults now live in egui_config_fast_path_default.h. */

/**
 * Layout options.
 * Enable view margin/padding APIs by default. Set to 0 only for size-first
 * builds that never rely on runtime margin/padding setters.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
#define EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING 1
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
 * Each app can define custom click positions by
 * implementing egui_port_get_recording_click().
 */
#ifndef EGUI_CONFIG_FUNCTION_RECORDING_TEST
#define EGUI_CONFIG_FUNCTION_RECORDING_TEST 0
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_DEFAULT_H_ */
