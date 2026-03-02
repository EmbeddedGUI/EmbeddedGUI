#ifndef _UICODE_H_
#define _UICODE_H_

#include "egui.h"
#include "app_egui_resource_generate.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

void uicode_create_ui(void);
void uicode_init_page_smarthome(egui_view_t *parent);
void uicode_init_page_music(egui_view_t *parent);
void uicode_init_page_dashboard(egui_view_t *parent);
void uicode_init_page_watch(egui_view_t *parent);
void uicode_toggle_theme(void);
uint8_t uicode_is_dark_theme(void);

// Page entry animation triggers
void uicode_page_smarthome_on_enter(void);
void uicode_page_music_on_enter(void);
void uicode_page_dashboard_on_enter(void);
void uicode_page_watch_on_enter(void);

// Theme icon update for all pages
void uicode_update_theme_icons(void);
void uicode_page_smarthome_update_theme_icon(void);
void uicode_page_dashboard_update_theme_icon(void);
void uicode_page_watch_update_theme_icon(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _UICODE_H_ */
