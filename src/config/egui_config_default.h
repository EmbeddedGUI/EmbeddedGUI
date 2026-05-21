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
 * FlexLayout container support.
 * When enabled, adds egui_view_flexlayout_t and a uint8_t flex_grow field
 * to egui_view_t. Disable to save ~1 byte per view instance.
 */
#ifndef EGUI_CONFIG_FUNCTION_FLEXLAYOUT
#define EGUI_CONFIG_FUNCTION_FLEXLAYOUT 0
#endif

/**
 * Style Cascade: enable a small per-view style stack that lets multiple shared
 * egui_view_style_t objects be applied to a view in priority order.  When disabled
 * (default) there is zero overhead.  When enabled each view gains a pointer
 * array of size EGUI_CONFIG_STYLE_MAX_PER_VIEW and a uint8_t counter.
 */
#ifndef EGUI_CONFIG_FUNCTION_STYLE_CASCADE
#define EGUI_CONFIG_FUNCTION_STYLE_CASCADE 0
#endif

/**
 * R3-5: View state styles — analogue of LVGL lv_style_t per-state.
 *
 * When enabled (requires EGUI_CONFIG_FUNCTION_STYLE_CASCADE=1), each
 * egui_view_style_t gains a state_mask field.  During style cascade only
 * styles whose state_mask is 0 (DEFAULT, always applied) OR whose bits are
 * ALL present in the view's current computed state are considered.
 *
 * Current state bits computed automatically from existing view flags:
 *   EGUI_VIEW_STATE_PRESSED  ← view->is_pressed
 *   EGUI_VIEW_STATE_FOCUSED  ← view->is_focused (needs SUPPORT_FOCUS)
 *   EGUI_VIEW_STATE_DISABLED ← !view->is_enable
 *   EGUI_VIEW_STATE_CHECKED  ← set via egui_view_set_state_checked()
 *
 * Opt-in because it adds one byte to egui_view_style_t and one byte to
 * egui_view_t.  Default 0 (disabled).
 */
#ifndef EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
#define EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES 0
#endif

/**
 * Subject-Observer (data binding): enable egui_subject_t / egui_observer_t.
 * When disabled (default) there is zero overhead — no data added to any struct.
 * When enabled, egui_subject.h / egui_subject.c are compiled in.
 * Maximum observers per subject is controlled by
 * EGUI_CONFIG_SUBJECT_MAX_OBSERVERS (default 4, in egui_config_widget_default.h).
 */
#ifndef EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER
#define EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER 0
#endif

/**
 * Runtime TTF font (Tiny TTF): enable egui_font_ttf_t backed by stb_truetype.
 * When disabled (default) there is zero overhead.
 * When enabled, egui_font_ttf.h / egui_font_ttf.c are compiled in.
 * Cache capacity and bitmap dimensions are controlled by:
 *   EGUI_CONFIG_FONT_TTF_GLYPH_CACHE_SLOTS    (default 8)
 *   EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_W   (default 20)
 *   EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_H   (default 32)
 * in egui_config_widget_default.h.
 *
 * Note: stb_truetype is a single-file header library included in the
 * third_party/plutovg/source directory.  This feature pulls in that header
 * and approximately doubles the binary size vs. bitmap-only builds.
 */
#ifndef EGUI_CONFIG_FUNCTION_FONT_TTF
#define EGUI_CONFIG_FUNCTION_FONT_TTF 0
#endif

/**
 * Lightweight event listener support.
 * When enabled, each view gains a small fixed listener table. No heap is used.
 */
#ifndef EGUI_CONFIG_FUNCTION_EVENT_LITE
#define EGUI_CONFIG_FUNCTION_EVENT_LITE 0
#endif

/**
 * Maximum event listeners that can be registered on one view when
 * EGUI_CONFIG_FUNCTION_EVENT_LITE is enabled.
 */
#ifndef EGUI_CONFIG_EVENT_MAX_LISTENERS_PER_VIEW
#define EGUI_CONFIG_EVENT_MAX_LISTENERS_PER_VIEW 4
#endif

/**
 * Standard message box helper built on top of egui_dialog_t.
 * Requires EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG.
 */
#ifndef EGUI_CONFIG_FUNCTION_MSGBOX
#define EGUI_CONFIG_FUNCTION_MSGBOX 0
#endif

/**
 * Maximum number of buttons one message box instance can expose.
 */
#ifndef EGUI_CONFIG_MSGBOX_MAX_BUTTONS
#define EGUI_CONFIG_MSGBOX_MAX_BUTTONS 3
#endif

/**
 * Explicit focus group helper for encoder/key UIs that want a small caller-owned
 * focus ring instead of full-tree traversal.
 */
#ifndef EGUI_CONFIG_FUNCTION_FOCUS_GROUP
#define EGUI_CONFIG_FUNCTION_FOCUS_GROUP 0
#endif

/**
 * Maximum number of views in one focus group.
 */
#ifndef EGUI_CONFIG_FOCUS_GROUP_MAX_VIEWS
#define EGUI_CONFIG_FOCUS_GROUP_MAX_VIEWS 8
#endif

/**
 * Generic property access helpers for tooling and small dynamic UIs.
 */
#ifndef EGUI_CONFIG_FUNCTION_PROPERTY_LITE
#define EGUI_CONFIG_FUNCTION_PROPERTY_LITE 0
#endif

/* ---- Round 4 features ---- */

/**
 * R4-1: Animation delay.
 * When enabled, egui_animation_t gains a delay_ms field.
 * egui_animation_set_delay(anim, delay_ms) defers the start of an animation
 * after egui_animation_start() is called.  Default 0 (disabled).
 */
#ifndef EGUI_CONFIG_FUNCTION_ANIM_DELAY
#define EGUI_CONFIG_FUNCTION_ANIM_DELAY 0
#endif

/**
 * R4-6: Animation on-complete callback.
 * When enabled, egui_animation_t gains on_complete_cb + on_complete_user_data.
 * egui_animation_set_on_complete(anim, cb, ud) registers a callback invoked
 * once when the animation ends (after any fill-after update).  Default 0.
 */
#ifndef EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB
#define EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB 0
#endif

/**
 * R7-1: Animation pause / resume.
 * When enabled, egui_animation_t gains is_paused + pause_elapsed fields.
 * egui_animation_pause(), egui_animation_resume(), and egui_animation_is_paused()
 * let callers freeze an in-progress animation and restore it mid-flight.
 * Default 0 (disabled).
 */
#ifndef EGUI_CONFIG_FUNCTION_ANIM_PAUSE_RESUME
#define EGUI_CONFIG_FUNCTION_ANIM_PAUSE_RESUME 0
#endif

/**
 * R4-3: Scroll horizontal mode.
 * When enabled, egui_view_scroll_t gains is_horizontal bit and
 * egui_view_scroll_set_horizontal() / egui_view_scroll_get_scroll_x().
 * Default 0 (disabled).
 */
#ifndef EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
#define EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL 0
#endif

/**
 * R4-4: Label inline recolor.
 * When enabled, egui_view_label_t gains a recolor bit and the renderer
 * parses '#RRGGBB text#' inline color tags.
 * Default 0 (disabled).
 */
#ifndef EGUI_CONFIG_FUNCTION_LABEL_RECOLOR
#define EGUI_CONFIG_FUNCTION_LABEL_RECOLOR 0
#endif

/**
 * R5-1: View user data.
 * When enabled, egui_view_t gains a void *user_data field.
 * egui_view_set_user_data() / egui_view_get_user_data() provide access.
 * Default 0 (disabled).
 */
#ifndef EGUI_CONFIG_FUNCTION_VIEW_USER_DATA
#define EGUI_CONFIG_FUNCTION_VIEW_USER_DATA 0
#endif

/**
 * R5-2: Label letter spacing.
 * When enabled, egui_view_label_t gains a letter_space field.
 * egui_view_label_set_letter_space() sets extra pixel gap between characters.
 * Default 0 (disabled).
 */
#ifndef EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE
#define EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE 0
#endif

/**
 * R5-5: Animation timeline.
 * Provides egui_animation_timeline_t to sequence multiple animations with
 * independent start-time offsets when EGUI_CONFIG_FUNCTION_ANIM_DELAY is enabled.
 */
#ifndef EGUI_CONFIG_ANIM_TIMELINE_MAX_ENTRIES
#define EGUI_CONFIG_ANIM_TIMELINE_MAX_ENTRIES 8
#endif

/**
 * R5-6: Label word wrap (LONG_WRAP mode).
 * When enabled, EGUI_LABEL_LONG_WRAP (value 2) is available as a long_mode
 * setting that automatically wraps text at word boundaries.
 * Requires EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE=1.  Default 0 (disabled).
 */
#ifndef EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP
#define EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP 0
#endif

/**
 * R6-5: Label printf-style formatting (set_text_fmt).
 * When enabled, egui_view_label_set_text_fmt() formats text into an
 * internal buffer of EGUI_CONFIG_LABEL_FMT_BUF_SIZE bytes before displaying.
 * Default 0 (disabled).
 */
#ifndef EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT
#define EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT 0
#endif

#ifndef EGUI_CONFIG_LABEL_FMT_BUF_SIZE
#define EGUI_CONFIG_LABEL_FMT_BUF_SIZE 64
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
 * Function options.
 * Rotary encoder driver support.  When enabled, egui_encoder_polling_work()
 * is called from egui_polling_work() to translate encoder ticks and button
 * presses into key events.  Key support is automatically enabled.
 */
#ifndef EGUI_CONFIG_FUNCTION_ENCODER
#define EGUI_CONFIG_FUNCTION_ENCODER 0
#endif

/**
 * Encoder long-press threshold in milliseconds.
 * The push-button must be held for this duration before a LONG_PRESS key
 * event is emitted.  Only used when EGUI_CONFIG_FUNCTION_ENCODER is enabled.
 */
#ifndef EGUI_CONFIG_ENCODER_PRESS_LONG_MS
#define EGUI_CONFIG_ENCODER_PRESS_LONG_MS 500
#endif

/* Encoder requires key support */
#if EGUI_CONFIG_FUNCTION_ENCODER
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
 * Extended click area: allow each view to expand its touch hit region beyond its visual bounds.
 * When enabled, egui_view_set_ext_click_area(view, px) expands the hit-test rect on all four
 * sides by `px` pixels without changing the layout or drawn size.
 * Each view gains one uint8_t field (1 byte per instance).  Disable to save that byte.
 */
#ifndef EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA
#define EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA 0
#endif

/**
 * Long-press listener for touch views.
 * When enabled, egui_view_set_on_long_press_listener(view, cb) arms a long-press callback that
 * fires once after the finger has been held inside the view for EGUI_CONFIG_LONG_PRESS_DURATION_MS
 * without lifting or sliding out.  If the long-press fires, the subsequent UP event will NOT
 * trigger the on_click_listener.  Disable to save a function pointer + two timing fields per view.
 */
#ifndef EGUI_CONFIG_FUNCTION_LONG_PRESS
#define EGUI_CONFIG_FUNCTION_LONG_PRESS 0
#endif

/**
 * How long (ms) a finger must be held inside a view before the long-press callback fires.
 * The default 500 ms matches the Android long-press threshold.
 */
#ifndef EGUI_CONFIG_LONG_PRESS_DURATION_MS
#define EGUI_CONFIG_LONG_PRESS_DURATION_MS 500
#endif

/**
 * Swipe direction listener for touch views.
 * When enabled, egui_view_set_on_swipe_listener(view, cb) arms a callback that fires on touch UP
 * when the finger has traveled at least EGUI_CONFIG_SWIPE_MIN_DISPLACEMENT_PX pixels from the
 * initial DOWN position.  The dominant axis (horizontal or vertical) determines the direction.
 * Disable to save a function pointer and two coordinate fields per view.
 */
#ifndef EGUI_CONFIG_FUNCTION_SWIPE_LISTENER
#define EGUI_CONFIG_FUNCTION_SWIPE_LISTENER 0
#endif

/**
 * Minimum finger travel distance in pixels required for a swipe event to fire.
 * Shorter gestures are ignored so accidental micro-drags do not count as swipes.
 */
#ifndef EGUI_CONFIG_SWIPE_MIN_DISPLACEMENT_PX
#define EGUI_CONFIG_SWIPE_MIN_DISPLACEMENT_PX 20
#endif

/**
 * Scroll snap for egui_view_scroll_t.
 * When enabled, egui_view_scroll_set_snap_interval_y(scroll, interval) makes the scroll
 * view animate to the nearest multiple of @p interval pixels after the user lifts the finger,
 * instead of starting a free-form fling.  Set @p interval to 0 (default) to disable snapping
 * on a per-view basis.  Disable this macro to remove snap code entirely.
 */
#ifndef EGUI_CONFIG_FUNCTION_SCROLL_SNAP
#define EGUI_CONFIG_FUNCTION_SCROLL_SNAP 0
#endif

/**
 * Duration in milliseconds of the snap-to-position animation.
 * Shorter values feel snappier; longer values feel smoother.
 */
#ifndef EGUI_CONFIG_SCROLL_SNAP_DURATION_MS
#define EGUI_CONFIG_SCROLL_SNAP_DURATION_MS 200
#endif

/**
 * R2-6: Draggable View — analogue of LVGL LV_OBJ_FLAG_DRAGGABLE.
 *
 * When enabled, call egui_view_set_draggable(view, 1) to make a view freely
 * movable by dragging.  On touch DOWN the view captures the finger; on each
 * MOVE the view is repositioned by the finger delta via egui_view_layout().
 * The feature requires EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH.
 */
#ifndef EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW
#define EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW 0
#endif

/**
 * Image widget lightweight scale.
 * egui_view_image_set_scale() uses the existing resize draw path and does not
 * pull in the full rotate/affine renderer. scale_q8: 256 = 1x.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE
#define EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE 0
#endif

/**
 * Image widget transform (rotation and affine scale).
 * egui_view_image_set_angle() and egui_view_image_set_transform() call the full
 * egui_canvas_draw_image_transform() renderer. Use IMAGE_SCALE_LITE when only
 * axis-aligned scale is needed.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
#define EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM 0
#endif

/**
 * Label long mode: text overflow behaviour when the label text exceeds its width.
 * Mode 0 (EGUI_LABEL_LONG_CLIP): clip silently (default, zero overhead).
 * Mode 1 (EGUI_LABEL_LONG_DOTS): truncate and append "...".
 * The scratch buffer used during truncation is allocated on the stack.
 * EGUI_CONFIG_LABEL_LONG_DOTS_BUF_SIZE controls its size (default 128).
 * Equivalent to lv_label_set_long_mode(DOTS) in LVGL.
 */
#ifndef EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
#define EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE 0
#endif
#ifndef EGUI_CONFIG_LABEL_LONG_DOTS_BUF_SIZE
#define EGUI_CONFIG_LABEL_LONG_DOTS_BUF_SIZE 128
#endif

/**
 * Scroll event listener.
 * When enabled, register a callback with egui_view_scroll_set_on_scroll_listener() to
 * receive the current scroll offset (pixels from top) each time the scroll view moves.
 * Equivalent to LV_EVENT_SCROLL in LVGL.
 */
#ifndef EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
#define EGUI_CONFIG_FUNCTION_SCROLL_LISTENER 0
#endif

/**
 * Recording test options.
 * Enable auto-click simulation during GIF recording for demo purposes.
 * Each app can define custom click positions by implementing egui_port_get_recording_click().
 */
#ifndef EGUI_CONFIG_FUNCTION_RECORDING_TEST
#define EGUI_CONFIG_FUNCTION_RECORDING_TEST 0
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_DEFAULT_H_ */
