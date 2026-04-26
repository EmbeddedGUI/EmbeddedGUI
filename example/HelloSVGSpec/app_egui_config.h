#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG     1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8 1
#ifndef EGUI_CONFIG_SCREEN_WIDTH
#define EGUI_CONFIG_SCREEN_WIDTH 160
#endif
#ifndef EGUI_CONFIG_SCREEN_HEIGHT
#define EGUI_CONFIG_SCREEN_HEIGHT 160
#endif
#ifndef EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_WIDTH 40
#endif
#ifndef EGUI_CONFIG_PFB_HEIGHT
#define EGUI_CONFIG_PFB_HEIGHT 40
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
