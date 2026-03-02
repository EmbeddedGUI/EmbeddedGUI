#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_SCEEN_WIDTH  480
#define EGUI_CONFIG_SCEEN_HEIGHT 480

#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 120

// Required by egui_background_gradient.c (uses canvas gradient draw APIs)
#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT 1

// Enable auto-click simulation for GIF recording
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
