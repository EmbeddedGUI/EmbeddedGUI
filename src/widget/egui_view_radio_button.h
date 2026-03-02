#ifndef _EGUI_VIEW_RADIO_BUTTON_H_
#define _EGUI_VIEW_RADIO_BUTTON_H_

#include "egui_view.h"
#include "core/egui_theme.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_radio_changed_listener_t)(egui_view_t *self, int index);

// ============== Radio Group ==============
typedef struct egui_view_radio_group egui_view_radio_group_t;
struct egui_view_radio_group
{
    egui_slist_t buttons;
    egui_view_on_radio_changed_listener_t on_changed;
};

void egui_view_radio_group_init(egui_view_radio_group_t *group);
void egui_view_radio_group_add(egui_view_radio_group_t *group, egui_view_t *button);
void egui_view_radio_group_set_on_changed_listener(egui_view_radio_group_t *group, egui_view_on_radio_changed_listener_t listener);

// ============== Radio Button ==============
typedef struct egui_view_radio_button egui_view_radio_button_t;
struct egui_view_radio_button
{
    egui_view_t base;

    egui_snode_t group_node;
    egui_view_radio_group_t *group;

    uint8_t is_checked;
    egui_alpha_t alpha;
    egui_color_t circle_color;
    egui_color_t dot_color;
    const char *text;
    const egui_font_t *font;
    egui_color_t text_color;
    egui_dim_t text_gap;
};

// ============== Radio Button Params ==============
typedef struct egui_view_radio_button_params egui_view_radio_button_params_t;
struct egui_view_radio_button_params
{
    egui_region_t region;
    uint8_t is_checked;
    const char *text;
};

#define EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT(_name, _x, _y, _w, _h, _checked)                                                                                    \
    static const egui_view_radio_button_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .is_checked = (_checked), .text = NULL}

#define EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(_name, _x, _y, _w, _h, _checked, _text)                                                                   \
    static const egui_view_radio_button_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .is_checked = (_checked), .text = (_text)}

void egui_view_radio_button_apply_params(egui_view_t *self, const egui_view_radio_button_params_t *params);
void egui_view_radio_button_init_with_params(egui_view_t *self, const egui_view_radio_button_params_t *params);

void egui_view_radio_button_set_checked(egui_view_t *self, uint8_t is_checked);
void egui_view_radio_button_set_text(egui_view_t *self, const char *text);
void egui_view_radio_button_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_radio_button_set_text_color(egui_view_t *self, egui_color_t color);
void egui_view_radio_button_on_draw(egui_view_t *self);
void egui_view_radio_button_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_RADIO_BUTTON_H_ */
