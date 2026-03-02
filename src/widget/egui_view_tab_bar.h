#ifndef _EGUI_VIEW_TAB_BAR_H_
#define _EGUI_VIEW_TAB_BAR_H_

#include "egui_view.h"
#include "core/egui_theme.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_tab_changed_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_tab_bar egui_view_tab_bar_t;
struct egui_view_tab_bar
{
    egui_view_t base;

    egui_view_on_tab_changed_listener_t on_tab_changed;
    const char **tab_texts;
    uint8_t tab_count;
    uint8_t current_index;
    egui_alpha_t alpha;
    egui_color_t text_color;
    egui_color_t active_text_color;
    egui_color_t indicator_color;
    const egui_font_t *font;
};

// ============== TabBar Params ==============
typedef struct egui_view_tab_bar_params egui_view_tab_bar_params_t;
struct egui_view_tab_bar_params
{
    egui_region_t region;
    const char **tab_texts;
    uint8_t tab_count;
};

#define EGUI_VIEW_TAB_BAR_PARAMS_INIT(_name, _x, _y, _w, _h, _texts, _count)                                                                                   \
    static const egui_view_tab_bar_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .tab_texts = (_texts), .tab_count = (_count)}

void egui_view_tab_bar_apply_params(egui_view_t *self, const egui_view_tab_bar_params_t *params);
void egui_view_tab_bar_init_with_params(egui_view_t *self, const egui_view_tab_bar_params_t *params);

void egui_view_tab_bar_set_tabs(egui_view_t *self, const char **tab_texts, uint8_t tab_count);
void egui_view_tab_bar_set_current_index(egui_view_t *self, uint8_t index);
void egui_view_tab_bar_set_on_tab_changed_listener(egui_view_t *self, egui_view_on_tab_changed_listener_t listener);
void egui_view_tab_bar_on_draw(egui_view_t *self);
void egui_view_tab_bar_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TAB_BAR_H_ */
