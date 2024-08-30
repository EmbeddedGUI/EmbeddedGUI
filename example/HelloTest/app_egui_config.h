#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_SCEEN_WIDTH  240
#define EGUI_CONFIG_SCEEN_HEIGHT 320
#define EGUI_CONFIG_COLOR_DEPTH  16

#define EGUI_CONFIG_PFB_WIDTH  24
#define EGUI_CONFIG_PFB_HEIGHT 32

// debug options
#define EGUI_CONFIG_DEBUG_PFB_REFRESH          0
#define EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH 0
#define EGUI_CONFIG_DEBUG_INFO_SHOW            1

#define EGUI_CONFIG_FONT_DEFAULT &egui_res_font_montserrat_18_4

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */