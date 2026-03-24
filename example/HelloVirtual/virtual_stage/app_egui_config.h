#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY   1
#define EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS 1

#define HELLO_VIRTUAL_STAGE_CANVAS_WIDTH  800
#define HELLO_VIRTUAL_STAGE_CANVAS_HEIGHT 800

#ifndef EGUI_CONFIG_SCEEN_WIDTH
#define EGUI_CONFIG_SCEEN_WIDTH HELLO_VIRTUAL_STAGE_CANVAS_WIDTH
#endif

#ifndef EGUI_CONFIG_SCEEN_HEIGHT
#define EGUI_CONFIG_SCEEN_HEIGHT HELLO_VIRTUAL_STAGE_CANVAS_HEIGHT
#endif

#ifndef EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_WIDTH ((EGUI_CONFIG_SCEEN_WIDTH + 7) / 8)
#endif

#ifndef EGUI_CONFIG_PFB_HEIGHT
#define EGUI_CONFIG_PFB_HEIGHT ((EGUI_CONFIG_SCEEN_HEIGHT + 7) / 8)
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
