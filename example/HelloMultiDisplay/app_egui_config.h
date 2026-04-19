#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Enable multi-display: 2 displays */
#define EGUI_CONFIG_MAX_DISPLAY_COUNT 2

/* Both displays use Activity */
#define EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY 1
#define EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG   1

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
