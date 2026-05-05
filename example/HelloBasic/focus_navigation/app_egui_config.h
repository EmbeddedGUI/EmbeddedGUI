#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH 0

#define EGUI_CONFIG_FUNCTION_SUPPORT_LAYER 1

#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#define EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS 1
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
