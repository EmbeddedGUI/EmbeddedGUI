#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_SCEEN_WIDTH  240
#define EGUI_CONFIG_SCEEN_HEIGHT 320
#define EGUI_CONFIG_COLOR_DEPTH  16

// PFB options - horizontal refresh
// #define EGUI_CONFIG_PFB_WIDTH  240
// #define EGUI_CONFIG_PFB_HEIGHT 8

// PFB options - vertical refresh
#define EGUI_CONFIG_PFB_WIDTH  8
#define EGUI_CONFIG_PFB_HEIGHT 320

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

// debug options
#define EGUI_CONFIG_DEBUG_PFB_REFRESH          1
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH 1
#define EGUI_CONFIG_DEBUG_INFO_SHOW            0

#define EGUI_CONFIG_DEBUG_REFRESH_DELAY 50

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
