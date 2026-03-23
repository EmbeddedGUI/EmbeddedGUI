#ifndef _EGUI_CONFIG_DEBUG_DEFAULT_H_
#define _EGUI_CONFIG_DEBUG_DEFAULT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Debug options.
 * Allocate a per-view unique id for debugging and tracing only.
 */
#ifndef EGUI_CONFIG_DEBUG_VIEW_ID
#define EGUI_CONFIG_DEBUG_VIEW_ID 0
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
 * For checking the clear of the PFB.
 */
#ifndef EGUI_CONFIG_DEBUG_PFB_DIRTY_REGION_CLEAR
#define EGUI_CONFIG_DEBUG_PFB_DIRTY_REGION_CLEAR 1
#endif

/**
 * Debug options.
 * Delay for checking the refresh of the PFB/Dirty Region.
 * /ref EGUI_CONFIG_DEBUG_PFB_REFRESH and EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
 */
#ifndef EGUI_CONFIG_DEBUG_REFRESH_DELAY
#define EGUI_CONFIG_DEBUG_REFRESH_DELAY 0
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
 * Debug options.
 * Enable dirty region statistics logging for performance analysis. if 0, disable.
 */
#ifndef EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS 0
#endif

/**
 * Debug options.
 * Enable touch trace debug overlay support.
 * When enabled, EGUI automatically records and draws the latest touch trace.
 */
#ifndef EGUI_CONFIG_DEBUG_TOUCH_TRACE
#define EGUI_CONFIG_DEBUG_TOUCH_TRACE 0
#endif

/**
 * Debug options.
 * Maximum points kept per touch trace.
 */
#ifndef EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS
#define EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS 256
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_DEBUG_DEFAULT_H_ */
