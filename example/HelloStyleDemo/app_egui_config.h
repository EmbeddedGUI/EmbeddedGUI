#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_SCEEN_WIDTH  240
#define EGUI_CONFIG_SCEEN_HEIGHT 320

#define EGUI_CONFIG_PFB_WIDTH  (EGUI_CONFIG_SCEEN_WIDTH / 4)
#define EGUI_CONFIG_PFB_HEIGHT (EGUI_CONFIG_SCEEN_HEIGHT / 4)

#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 120

#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW        1
#define EGUI_CONFIG_WIDGET_ENHANCED_DRAW           1
#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_DITHERING 1

#define EGUI_THEME_TRACK_THICKNESS         10
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY   1
#define EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS 1
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
