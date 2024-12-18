#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// #define EGUI_CONFIG_PFB_WIDTH EGUI_CONFIG_SCEEN_WIDTH
// #define EGUI_CONFIG_PFB_HEIGHT EGUI_CONFIG_SCEEN_HEIGHT

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

#define EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER 1
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 1

#define EGUI_CONFIG_DEBUG_PFB_REFRESH          0
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH 0
#define EGUI_CONFIG_DEBUG_INFO_SHOW            1

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */