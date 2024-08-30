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

// For speed, we assume that the PFB width and height are multiples of the screen width and height

// Width of the PFB block
// The width of your PFB block size, must be a multiple of EGUI_CONFIG_SCEEN_WIDTH
#ifndef EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_WIDTH (EGUI_CONFIG_SCEEN_WIDTH / 8)
#endif

// Height of the PFB block
// The height of your PFB block size, must be a multiple of EGUI_CONFIG_SCEEN_HEIGHT
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
 * Select support touch. if 0, disable touch.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH 1
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
 * Force refresh all, for debug performance.
 */
#ifndef EGUI_CONFIG_DEBUG_FORCE_REFRESH_ALL
#define EGUI_CONFIG_DEBUG_FORCE_REFRESH_ALL 0
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
 * Delay for checking the refresh of the PFB.
 * /ref EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
 */
#ifndef EGUI_CONFIG_DEBUG_PFB_REFRESH_DELAY
#define EGUI_CONFIG_DEBUG_PFB_REFRESH_DELAY 1
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
 * Delay for checking the refresh of the Dirty Region.
 * /ref EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
 */
#ifndef EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH_DELAY
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH_DELAY 10
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

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_DEFAULT_H_ */
