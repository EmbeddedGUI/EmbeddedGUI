#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Enable multi-display: main + sub */
#define EGUI_CONFIG_MAX_DISPLAY_COUNT 2

/* Display 1 uses a compact status panel. */
#define EGUI_CONFIG_SCEEN_1_WIDTH  128
#define EGUI_CONFIG_SCEEN_1_HEIGHT 64
#define EGUI_CONFIG_PFB_1_WIDTH    16
#define EGUI_CONFIG_PFB_1_HEIGHT   8

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
