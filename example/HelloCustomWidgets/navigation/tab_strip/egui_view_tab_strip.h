#ifndef _EGUI_VIEW_TAB_STRIP_H_
#define _EGUI_VIEW_TAB_STRIP_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TAB_STRIP_MAX_TABS 6

typedef void (*egui_view_on_tab_strip_changed_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_tab_strip egui_view_tab_strip_t;
struct egui_view_tab_strip
{
    egui_view_t base;
    egui_view_on_tab_strip_changed_listener_t on_tab_changed;
    const char **tab_texts;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    uint8_t tab_count;
    uint8_t current_index;
    uint8_t compact_mode;
    uint8_t locked_mode;
    uint8_t pressed_index;
};

void egui_view_tab_strip_init(egui_view_t *self);
void egui_view_tab_strip_set_tabs(egui_view_t *self, const char **tab_texts, uint8_t tab_count);
void egui_view_tab_strip_set_current_index(egui_view_t *self, uint8_t index);
uint8_t egui_view_tab_strip_get_current_index(egui_view_t *self);
void egui_view_tab_strip_set_on_tab_changed_listener(egui_view_t *self, egui_view_on_tab_strip_changed_listener_t listener);
void egui_view_tab_strip_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_tab_strip_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_tab_strip_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_tab_strip_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                     egui_color_t muted_text_color, egui_color_t accent_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TAB_STRIP_H_ */
