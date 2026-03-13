#ifndef _EGUI_CONFIG_DEFAULT_H_
#define _EGUI_CONFIG_DEFAULT_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// Width of the screen <8-32767>
// The width of your screen.
#ifndef EGUI_CONFIG_SCEEN_WIDTH
#define EGUI_CONFIG_SCEEN_WIDTH 240
#endif
// Height of the screen <8-32767>
// The height of your screen.
#ifndef EGUI_CONFIG_SCEEN_HEIGHT
#define EGUI_CONFIG_SCEEN_HEIGHT 320
#endif
// Select the screen colour depth
// The colour depth of your LCD.
#ifndef EGUI_CONFIG_COLOR_DEPTH
#define EGUI_CONFIG_COLOR_DEPTH 16
#endif

/**
 * Color options.
 * Swap the 2 bytes of RGB565 color. Useful if the display has a 8 bit interface (e.g. SPI).
 */
#ifndef EGUI_CONFIG_COLOR_16_SWAP
#define EGUI_CONFIG_COLOR_16_SWAP 0
#endif

/**
 * Color options.
 * Swap the 2 bytes of RGB565 color. Useful if the image is 16bit.
 */
#ifndef EGUI_CONFIG_COLOR_16_SWAP_IMG565
#define EGUI_CONFIG_COLOR_16_SWAP_IMG565 0
#endif

/**
 * SDL display options (PC simulator only).
 * When enabled and color depth is 16, SDL uses native RGB565 texture
 * to accurately simulate embedded display color quality.
 * When 0 (default), always converts to ARGB8888 for display.
 */
#ifndef EGUI_CONFIG_SDL_NATIVE_COLOR
#define EGUI_CONFIG_SDL_NATIVE_COLOR 0
#endif

// For speed, we assume that the PFB width and height are multiples of the screen width and height

// Width of the PFB block
// The width of your PFB block size, suggest be a multiple of EGUI_CONFIG_SCEEN_WIDTH
#ifndef EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_WIDTH (EGUI_CONFIG_SCEEN_WIDTH / 8)
#endif

// Height of the PFB block
// The height of your PFB block size, suggest be a multiple of EGUI_CONFIG_SCEEN_HEIGHT
#ifndef EGUI_CONFIG_PFB_HEIGHT
#define EGUI_CONFIG_PFB_HEIGHT (EGUI_CONFIG_SCEEN_HEIGHT / 8)
#endif

// Set the maximum FPS
// Limit the maximum FPS to reduce the CPU usage.
#ifndef EGUI_CONFIG_MAX_FPS
#define EGUI_CONFIG_MAX_FPS 60
#endif

// Set the dirty area count
// Limit is 1, use the buffer to reduce need refresh area.
#ifndef EGUI_CONFIG_DIRTY_AREA_COUNT
#define EGUI_CONFIG_DIRTY_AREA_COUNT 5
#endif

// Set the motion cache count
// use to save the motion of the input device.
#ifndef EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT
#define EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT 5
#endif

/**
 * Params options.
 * For toast default show time.
 */
#ifndef EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME
#define EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME 1000
#endif

/**
 * Performance options.
 * In some cpu, float is faster than int, so you can use float to improve the performance.
 */
#ifndef EGUI_CONFIG_PERFORMANCE_USE_FLOAT
#define EGUI_CONFIG_PERFORMANCE_USE_FLOAT 0
#endif

///////////////////// UI Config /////////////////////

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

// Multi-touch requires single-touch
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

// Focus requires key support
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#undef EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY 1
#endif

/**
 * Function options.
 * Select support view layer for z-ordering. if 0, disable layer (all views draw in insertion order).
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
#define EGUI_CONFIG_FUNCTION_SUPPORT_LAYER 1
#endif

/**
 * Function options.
 * Select support scrollbar indicator for scrollable views. if 0, disable scrollbar.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
#define EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR 1
#endif

/**
 * Key input cache count.
 * Number of key events that can be queued.
 */
#ifndef EGUI_CONFIG_INPUT_KEY_CACHE_COUNT
#define EGUI_CONFIG_INPUT_KEY_CACHE_COUNT 5
#endif

/**
 * Text input default max length.
 */
#ifndef EGUI_CONFIG_TEXTINPUT_MAX_LENGTH
#define EGUI_CONFIG_TEXTINPUT_MAX_LENGTH 32
#endif

/**
 * Cursor blink interval in milliseconds.
 */
#ifndef EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS
#define EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS 500
#endif

/**
 * Textblock edit buffer max length.
 */
#ifndef EGUI_CONFIG_TEXTBLOCK_EDIT_MAX_LENGTH
#define EGUI_CONFIG_TEXTBLOCK_EDIT_MAX_LENGTH 256
#endif

/**
 * Textblock cursor blink interval in milliseconds.
 */
#ifndef EGUI_CONFIG_TEXTBLOCK_CURSOR_BLINK_MS
#define EGUI_CONFIG_TEXTBLOCK_CURSOR_BLINK_MS 500
#endif

/**
 * Function options.
 * Select support image rgb32.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32 1
#endif

/**
 * Function options.
 * Select support image rgb565.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565 1
#endif

/**
 * Function options.
 * Select support image rgb565_1.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1 1
#endif

/**
 * Function options.
 * Select support image rgb565_2.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2 1
#endif

/**
 * Function options.
 * Select support image rgb565_4.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4 1
#endif

/**
 * Function options.
 * Select support image rgb565_8.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8 1
#endif

/**
 * Function options.
 * Select support image alpha_1 (alpha-only, 1-bit).
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1 1
#endif

/**
 * Function options.
 * Select support image alpha_2 (alpha-only, 2-bit).
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2 1
#endif

/**
 * Function options.
 * Select support image alpha_4 (alpha-only, 4-bit).
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4 1
#endif

/**
 * Function options.
 * Select support image alpha_8 (alpha-only, 8-bit).
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8 1
#endif

/**
 * Function options.
 * Select support external resource manager.
 */
#ifndef EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 0
#endif

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
// If external resource is enabled, must enable resource manager.
#undef EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
#define EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER 1
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

/**
 * Function options.
 * Select support app resource manager.
 */
#ifndef EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
#define EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER 0
#endif

/**
 * Resource options.
 * Select the default font
 */
#ifndef EGUI_CONFIG_FONT_DEFAULT
#define EGUI_CONFIG_FONT_DEFAULT &egui_res_font_montserrat_14_4
#endif

/**
 * Resource options.
 * select support circle radius range.
 */
#ifndef EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE
#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 50
#endif

/**
 * Canvas draw options.
 * Enable ellipse drawing support.
 */
#ifndef EGUI_CONFIG_FUNCTION_CANVAS_DRAW_ELLIPSE
#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_ELLIPSE 1
#endif

/**
 * Canvas draw options.
 * Enable polygon and polyline drawing support.
 */
#ifndef EGUI_CONFIG_FUNCTION_CANVAS_DRAW_POLYGON
#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_POLYGON 1
#endif

/**
 * Canvas draw options.
 * Enable bezier curve drawing support (quadratic and cubic).
 */
#ifndef EGUI_CONFIG_FUNCTION_CANVAS_DRAW_BEZIER
#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_BEZIER 1
#endif

/**
 * Canvas draw options.
 * Enable high-quality circle/arc drawing support (runtime sub-pixel sampling).
 * When enabled, egui_canvas_draw_circle_hq() etc. are available.
 * Better visual quality but slower than basic (lookup table) algorithm.
 */
#ifndef EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ
#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ 1
#endif

/**
 * Canvas draw options.
 * When 1, default circle/arc APIs use HQ algorithm (better quality, slower).
 * When 0, default circle/arc APIs use basic algorithm (faster, lookup table).
 * Users can always explicitly call _basic or _hq variants regardless of this setting.
 */
#ifndef EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ
#define EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ 0
#endif

// If HQ is the default, it must be enabled
#if EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ
#undef EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ
#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ 1
#endif

/**
 * Canvas draw options.
 * Enable gradient fill support for shapes (rectangle, round rectangle, circle, etc.).
 * When enabled, egui_canvas_draw_*_fill_gradient() functions are available.
 * Supports linear (vertical/horizontal) and radial gradients with multi-stop colors.
 */
#ifndef EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT
#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT 0
#endif

/**
 * Enhanced widget drawing mode.
 * When enabled, widgets use gradient fills instead of flat colors for a more polished look.
 * Automatically enables EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT when active.
 * Default: 0 (off) - zero overhead when disabled.
 */
#ifndef EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#define EGUI_CONFIG_WIDGET_ENHANCED_DRAW 0
#endif

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW && !EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT
#undef EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT
#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT 1
#endif

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW && !EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#undef EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
#define EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING 0
#endif

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW && !EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
#undef EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
#define EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING 1
#endif

/**
 * Canvas draw options.
 * When 1, HQ circle uses int32_t only (no 64-bit math in hot path).
 * Max radius limited to EGUI_CONFIG_CIRCLE_HQ_MAX_RADIUS (default 8191).
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

/**
 * Canvas draw options.
 * Enable high-quality line/polyline drawing support (runtime sub-pixel sampling).
 * When enabled, egui_canvas_draw_line_hq() and egui_canvas_draw_polyline_hq() are available.
 * Better visual quality but slower than basic (distance-field) algorithm.
 */
#ifndef EGUI_CONFIG_FUNCTION_CANVAS_DRAW_LINE_HQ
#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_LINE_HQ 1
#endif

/**
 * Canvas draw options.
 * When 1, HQ line uses 2x2 (4-point) sub-pixel sampling for faster performance.
 * When 0 (default), uses 4x4 (16-point) sampling for best quality.
 */
#ifndef EGUI_CONFIG_LINE_HQ_SAMPLE_2X2
#define EGUI_CONFIG_LINE_HQ_SAMPLE_2X2 0
#endif

/**
 * Reduce code/ram size options.
 * Image code size reduce, becareful to use this option, it will reduce the code size of the image, but it will increase the CPU usage.
 */
#ifndef EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE
#define EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE 0
#endif

/**
 * Reduce code/ram size options.
 * Select limit margin/padding size, max to -128~127. outherwise, use the egui_dim_t type.
 */
#ifndef EGUI_CONFIG_REDUCE_MARGIN_PADDING_SIZE
#define EGUI_CONFIG_REDUCE_MARGIN_PADDING_SIZE 1
#endif

/**
 * Debug options.
 * Skip gui draw to reduce cpu usage, for debug GUI performance.
 */
#ifndef EGUI_CONFIG_DEBUG_SKIP_DRAW_ALL
#define EGUI_CONFIG_DEBUG_SKIP_DRAW_ALL 0
#endif

/**
 * Debug options.
 * For checking the refresh of the PFB.
 */
#ifndef EGUI_CONFIG_DEBUG_PFB_REFRESH
#define EGUI_CONFIG_DEBUG_PFB_REFRESH 0
#endif

/**
 * Debug options.
 * For checking the refresh of the Dirty Region.
 */
#ifndef EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH 0
#endif

/**
 * Debug options.
 * Delay for checking the refresh of the PFB/Dirty Region.
 * /ref EGUI_CONFIG_DEBUG_PFB_REFRESH and EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
 */
#ifndef EGUI_CONFIG_DEBUG_REFRESH_DELAY
#define EGUI_CONFIG_DEBUG_REFRESH_DELAY 1
#endif

/**
 * Debug options.
 * For show debug info on the screen.
 */
#ifndef EGUI_CONFIG_DEBUG_INFO_SHOW
#define EGUI_CONFIG_DEBUG_INFO_SHOW 0
#endif

/**
 * Debug options.
 * For debug the class name.
 */
#ifndef EGUI_CONFIG_DEBUG_CLASS_NAME
#define EGUI_CONFIG_DEBUG_CLASS_NAME 0
#endif

/**
 * Debug options.
 * For log level. EGUI_LOG_IMPL_LEVEL_NONE, EGUI_LOG_IMPL_LEVEL_ERR, EGUI_LOG_IMPL_LEVEL_WRN, EGUI_LOG_IMPL_LEVEL_INF, EGUI_LOG_IMPL_LEVEL_DBG
 */
#ifndef EGUI_CONFIG_DEBUG_LOG_LEVEL
#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_NONE
#endif

/**
 * Debug options.
 * For log info print.
 */
#ifndef EGUI_CONFIG_DEBUG_LOG_SIMPLE
#define EGUI_CONFIG_DEBUG_LOG_SIMPLE 1
#endif

/**
 * Recording test options.
 * Enable auto-click simulation during GIF recording for demo purposes.
 * Each app can define custom click positions by implementing egui_port_get_recording_click().
 */
#ifndef EGUI_CONFIG_RECORDING_TEST
#define EGUI_CONFIG_RECORDING_TEST 0
#endif

/**
 * PFB double buffering (legacy, kept for backward compatibility).
 * Prefer using EGUI_CONFIG_PFB_BUFFER_COUNT >= 2 instead.
 */
#ifndef EGUI_CONFIG_PFB_DOUBLE_BUFFER
#define EGUI_CONFIG_PFB_DOUBLE_BUFFER 0
#endif

/**
 * PFB multi-buffer count.
 * Controls how many PFB buffers the ring queue uses:
 *   1 = single buffer, synchronous draw (default, no DMA overlap)
 *   2 = double buffer, CPU/DMA pipeline depth 1
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
#if EGUI_CONFIG_PFB_DOUBLE_BUFFER
#define EGUI_CONFIG_PFB_BUFFER_COUNT 2
#else
#define EGUI_CONFIG_PFB_BUFFER_COUNT 1
#endif
#endif

/**
 * Software rotation support.
 * When enabled, core can rotate PFB output in software if hardware does not support it.
 * Requires a rotation scratch buffer of PFB size for 90/270 degree rotation.
 */
#ifndef EGUI_CONFIG_SOFTWARE_ROTATION
#define EGUI_CONFIG_SOFTWARE_ROTATION 0
#endif

// -----------------------------------------------------------------------------
// Default Visual Theme Colors (Modern Round inspired)
// -----------------------------------------------------------------------------

#ifndef EGUI_THEME_PRIMARY
#define EGUI_THEME_PRIMARY EGUI_COLOR_MAKE(0x25, 0x63, 0xEB) // Blue 600
#endif

#ifndef EGUI_THEME_PRIMARY_DARK
#define EGUI_THEME_PRIMARY_DARK EGUI_COLOR_MAKE(0x1D, 0x4E, 0xD8) // Blue 700 (Pressed)
#endif

#ifndef EGUI_THEME_SECONDARY
#define EGUI_THEME_SECONDARY EGUI_COLOR_MAKE(0x14, 0xB8, 0xA6) // Teal 500
#endif

#ifndef EGUI_THEME_SUCCESS
#define EGUI_THEME_SUCCESS EGUI_COLOR_MAKE(0x16, 0xA3, 0x4A) // Green 600
#endif

#ifndef EGUI_THEME_WARNING
#define EGUI_THEME_WARNING EGUI_COLOR_MAKE(0xF5, 0x9E, 0x0B) // Amber 500
#endif

#ifndef EGUI_THEME_DANGER
#define EGUI_THEME_DANGER EGUI_COLOR_MAKE(0xDC, 0x26, 0x26) // Red 600
#endif

#ifndef EGUI_THEME_TRACK_BG
#define EGUI_THEME_TRACK_BG EGUI_COLOR_MAKE(0xE2, 0xE8, 0xF0) // Slate 200
#endif

#ifndef EGUI_THEME_TRACK_OFF
#define EGUI_THEME_TRACK_OFF EGUI_COLOR_MAKE(0xCB, 0xD5, 0xE1) // Slate 300
#endif

#ifndef EGUI_THEME_DISABLED
#define EGUI_THEME_DISABLED EGUI_COLOR_MAKE(0xD1, 0xD5, 0xDB) // Gray 300
#endif

#ifndef EGUI_THEME_THUMB
#define EGUI_THEME_THUMB EGUI_COLOR_WHITE // White (Slider/Switch knobs)
#endif

#ifndef EGUI_THEME_TEXT
#define EGUI_THEME_TEXT EGUI_COLOR_WHITE // White text on primary
#endif

#ifndef EGUI_THEME_TEXT_PRIMARY
#define EGUI_THEME_TEXT_PRIMARY EGUI_COLOR_MAKE(0x11, 0x18, 0x27) // Gray 900
#endif

#ifndef EGUI_THEME_TEXT_SECONDARY
#define EGUI_THEME_TEXT_SECONDARY EGUI_COLOR_MAKE(0x6B, 0x72, 0x80) // Gray 500
#endif

#ifndef EGUI_THEME_SURFACE
#define EGUI_THEME_SURFACE EGUI_COLOR_MAKE(0xFF, 0xFF, 0xFF) // White surface
#endif

#ifndef EGUI_THEME_SURFACE_VARIANT
#define EGUI_THEME_SURFACE_VARIANT EGUI_COLOR_MAKE(0xF8, 0xFA, 0xFC) // Slate 50
#endif

#ifndef EGUI_THEME_BORDER
#define EGUI_THEME_BORDER EGUI_COLOR_MAKE(0xE2, 0xE8, 0xF0) // Slate 200
#endif

#ifndef EGUI_THEME_FOCUS
#define EGUI_THEME_FOCUS EGUI_THEME_PRIMARY // Focus ring color
#endif

#ifndef EGUI_THEME_PRESS_OVERLAY
#define EGUI_THEME_PRESS_OVERLAY EGUI_COLOR_BLACK // Press overlay color
#endif

#ifndef EGUI_THEME_PRESS_OVERLAY_ALPHA
#define EGUI_THEME_PRESS_OVERLAY_ALPHA EGUI_ALPHA_10 // Press overlay alpha
#endif

#ifndef EGUI_THEME_RADIUS_SM
#define EGUI_THEME_RADIUS_SM 6
#endif

#ifndef EGUI_THEME_RADIUS_MD
#define EGUI_THEME_RADIUS_MD 10
#endif

#ifndef EGUI_THEME_RADIUS_LG
#define EGUI_THEME_RADIUS_LG 14
#endif

#ifndef EGUI_THEME_STROKE_WIDTH
#define EGUI_THEME_STROKE_WIDTH 1
#endif

#ifndef EGUI_THEME_TRACK_THICKNESS
#define EGUI_THEME_TRACK_THICKNESS 6
#endif

// Scrollbar theme
#ifndef EGUI_THEME_SCROLLBAR_COLOR
#define EGUI_THEME_SCROLLBAR_COLOR EGUI_COLOR_MAKE(0x94, 0xA3, 0xB8) // Slate 400
#endif

#ifndef EGUI_THEME_SCROLLBAR_ALPHA
#define EGUI_THEME_SCROLLBAR_ALPHA EGUI_ALPHA_50
#endif

#ifndef EGUI_THEME_SCROLLBAR_THICKNESS
#define EGUI_THEME_SCROLLBAR_THICKNESS 4
#endif

#ifndef EGUI_THEME_SCROLLBAR_MIN_LENGTH
#define EGUI_THEME_SCROLLBAR_MIN_LENGTH 8
#endif

#ifndef EGUI_THEME_SCROLLBAR_MARGIN
#define EGUI_THEME_SCROLLBAR_MARGIN 2
#endif

#ifndef EGUI_THEME_SCROLLBAR_RADIUS
#define EGUI_THEME_SCROLLBAR_RADIUS 2
#endif

#ifndef EGUI_THEME_SCROLLBAR_TOUCH_WIDTH
#define EGUI_THEME_SCROLLBAR_TOUCH_WIDTH 15
#endif

// Enhanced draw theme colors (lighter/darker variants for gradients)
#ifndef EGUI_THEME_PRIMARY_LIGHT
#define EGUI_THEME_PRIMARY_LIGHT EGUI_COLOR_MAKE(0x3B, 0x82, 0xF6) // Blue 500
#endif

#ifndef EGUI_THEME_TRACK_BG_DARK
#define EGUI_THEME_TRACK_BG_DARK EGUI_COLOR_MAKE(0xCB, 0xD5, 0xE1) // Slate 300
#endif

// Enhanced draw shadow theme
#ifndef EGUI_THEME_SHADOW_WIDTH_SM
#define EGUI_THEME_SHADOW_WIDTH_SM 3
#endif

#ifndef EGUI_THEME_SHADOW_WIDTH_MD
#define EGUI_THEME_SHADOW_WIDTH_MD 4
#endif

#ifndef EGUI_THEME_SHADOW_WIDTH_LG
#define EGUI_THEME_SHADOW_WIDTH_LG 6
#endif

#ifndef EGUI_THEME_SHADOW_OFS_Y_SM
#define EGUI_THEME_SHADOW_OFS_Y_SM 1
#endif

#ifndef EGUI_THEME_SHADOW_OFS_Y_MD
#define EGUI_THEME_SHADOW_OFS_Y_MD 2
#endif

#ifndef EGUI_THEME_SHADOW_OFS_Y_LG
#define EGUI_THEME_SHADOW_OFS_Y_LG 3
#endif

#ifndef EGUI_THEME_SHADOW_OPA
#define EGUI_THEME_SHADOW_OPA 80
#endif

#ifndef EGUI_THEME_ENHANCED_BORDER_WIDTH
#define EGUI_THEME_ENHANCED_BORDER_WIDTH 1
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_DEFAULT_H_ */


