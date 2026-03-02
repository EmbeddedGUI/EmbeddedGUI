#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_MAX_FPS 1

#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 200

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT 1
#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW       1

// For performance test
// #define EGUI_CONFIG_DEBUG_SKIP_DRAW_ALL 1
// #define EGUI_CONFIG_PFB_WIDTH 12
// #define EGUI_CONFIG_PFB_HEIGHT 8
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
