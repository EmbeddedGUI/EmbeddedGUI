#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

// Large showcase canvas
#define HELLO_SHOWCASE_CANVAS_WIDTH  1280
#define HELLO_SHOWCASE_CANVAS_HEIGHT 1024

// #define EGUI_CONFIG_SCREEN_WIDTH 240
// #define EGUI_CONFIG_SCREEN_HEIGHT 320

#ifndef EGUI_CONFIG_SCREEN_WIDTH
#define EGUI_CONFIG_SCREEN_WIDTH HELLO_SHOWCASE_CANVAS_WIDTH
#endif

#ifndef EGUI_CONFIG_SCREEN_HEIGHT
#define EGUI_CONFIG_SCREEN_HEIGHT HELLO_SHOWCASE_CANVAS_HEIGHT
#endif

#ifndef EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_WIDTH ((EGUI_CONFIG_SCREEN_WIDTH + 3) / 4)
#endif

#ifndef EGUI_CONFIG_PFB_HEIGHT
#define EGUI_CONFIG_PFB_HEIGHT ((EGUI_CONFIG_SCREEN_HEIGHT + 31) / 32)
#endif

// Showcase often updates 9-10 distant widgets in the same frame.
// Use a few more slots here to avoid large fallback unions without
// affecting the global default or materially changing SRAM usage.
#define EGUI_CONFIG_DIRTY_AREA_COUNT 10

#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 150

// Enable enhanced rendering features
#define EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW 1
#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW       1
#define EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS        1
#define EGUI_CONFIG_FUNCTION_SUPPORT_LAYER        1
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY          1
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOAST        1
#define EGUI_CONFIG_FOCUS_MAX_FOCUSABLE_VIEWS     96
#define EGUI_CONFIG_FOCUS_DFS_MAX_DEPTH           128
#define EGUI_CONFIG_INPUT_KEY_CACHE_COUNT         16

// debug
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH 0

#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH 0

#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
