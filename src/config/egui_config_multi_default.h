#ifndef _EGUI_CONFIG_MULTI_DEFAULT_H_
#define _EGUI_CONFIG_MULTI_DEFAULT_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* ---- Multi-display ---- */

/* Maximum number of display cores. 1 = single display (default). */
#ifndef EGUI_CONFIG_MAX_DISPLAY_COUNT
#define EGUI_CONFIG_MAX_DISPLAY_COUNT 1
#endif

/* ---- Display 1 ---- */

#ifndef EGUI_CONFIG_SCREEN_1_WIDTH
#define EGUI_CONFIG_SCREEN_1_WIDTH EGUI_CONFIG_SCREEN_WIDTH
#endif

#ifndef EGUI_CONFIG_SCREEN_1_HEIGHT
#define EGUI_CONFIG_SCREEN_1_HEIGHT EGUI_CONFIG_SCREEN_HEIGHT
#endif

#ifndef EGUI_CONFIG_PFB_1_WIDTH
#define EGUI_CONFIG_PFB_1_WIDTH EGUI_CONFIG_PFB_WIDTH
#endif

#ifndef EGUI_CONFIG_PFB_1_HEIGHT
#define EGUI_CONFIG_PFB_1_HEIGHT EGUI_CONFIG_PFB_HEIGHT
#endif

/* ---- Display 2 ---- */

#ifndef EGUI_CONFIG_SCREEN_2_WIDTH
#define EGUI_CONFIG_SCREEN_2_WIDTH EGUI_CONFIG_SCREEN_WIDTH
#endif

#ifndef EGUI_CONFIG_SCREEN_2_HEIGHT
#define EGUI_CONFIG_SCREEN_2_HEIGHT EGUI_CONFIG_SCREEN_HEIGHT
#endif

#ifndef EGUI_CONFIG_PFB_2_WIDTH
#define EGUI_CONFIG_PFB_2_WIDTH EGUI_CONFIG_PFB_WIDTH
#endif

#ifndef EGUI_CONFIG_PFB_2_HEIGHT
#define EGUI_CONFIG_PFB_2_HEIGHT EGUI_CONFIG_PFB_HEIGHT
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_MULTI_DEFAULT_H_ */
