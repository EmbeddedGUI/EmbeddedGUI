#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

#define EGUI_CONFIG_DIRTY_AREA_COUNT 16

#define EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY 1

#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH 1

#define EGUI_CONFIG_DEBUG_PFB_DIRTY_REGION_CLEAR 0

#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
#define EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH 1
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
