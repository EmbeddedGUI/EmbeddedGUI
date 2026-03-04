#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// Enable software rotation
#define EGUI_CONFIG_SOFTWARE_ROTATION 1

// PFB size must be common divisors of both 240 and 320 for rotation compatibility
#define EGUI_CONFIG_PFB_WIDTH  20
#define EGUI_CONFIG_PFB_HEIGHT 20

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
