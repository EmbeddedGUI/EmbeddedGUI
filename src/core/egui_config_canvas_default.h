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

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_CANVAS_DEFAULT_H_ */
