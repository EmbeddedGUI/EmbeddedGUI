#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#ifdef EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
#undef EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
#endif
#define EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING 0

#ifdef EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
#undef EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
#endif
#define EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE 0

#endif /* _APP_EGUI_CONFIG_H_ */
