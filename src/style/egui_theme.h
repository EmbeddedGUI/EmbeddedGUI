#ifndef _EGUI_STYLE_THEME_H_
#define _EGUI_STYLE_THEME_H_

#include "egui_style.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_theme
{
    const char *name;
    const egui_widget_style_desc_t *button;
    const egui_widget_style_desc_t *label;
    const egui_widget_style_desc_t *switch_ctrl;
    const egui_widget_style_desc_t *slider;
    const egui_widget_style_desc_t *checkbox;
    const egui_widget_style_desc_t *card;
    const egui_widget_style_desc_t *progress_bar;
    const egui_widget_style_desc_t *circular_progress_bar;
} egui_theme_t;

extern const egui_theme_t *egui_current_theme;
extern const egui_theme_t egui_theme_light;
extern const egui_theme_t egui_theme_dark;

void egui_theme_set(const egui_theme_t *theme);
const egui_theme_t *egui_theme_get(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_STYLE_THEME_H_ */
