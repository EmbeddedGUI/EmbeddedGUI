#ifndef _EGUI_VIEW_AUTOCOMPLETE_H_
#define _EGUI_VIEW_AUTOCOMPLETE_H_

#include "egui_view_combobox.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef egui_view_on_combobox_selected_listener_t egui_view_on_autocomplete_selected_listener_t;

typedef struct egui_view_autocomplete egui_view_autocomplete_t;
struct egui_view_autocomplete
{
    egui_view_combobox_t combobox;
};

typedef struct egui_view_autocomplete_params egui_view_autocomplete_params_t;
struct egui_view_autocomplete_params
{
    egui_region_t region;
    const char **suggestions;
    uint8_t suggestion_count;
    uint8_t current_index;
};

#define EGUI_VIEW_AUTOCOMPLETE_PARAMS_INIT(_name, _x, _y, _w, _h, _suggestions, _count, _index)                                                            \
    static const egui_view_autocomplete_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .suggestions = (_suggestions),                           \
                                                           .suggestion_count = (_count), .current_index = (_index)}

void egui_view_autocomplete_apply_params(egui_view_t *self, const egui_view_autocomplete_params_t *params);
void egui_view_autocomplete_init_with_params(egui_view_t *self, const egui_view_autocomplete_params_t *params);

void egui_view_autocomplete_set_suggestions(egui_view_t *self, const char **suggestions, uint8_t count);
uint8_t egui_view_autocomplete_get_suggestion_count(egui_view_t *self);
void egui_view_autocomplete_set_current_index(egui_view_t *self, uint8_t index);
uint8_t egui_view_autocomplete_get_current_index(egui_view_t *self);
const char *egui_view_autocomplete_get_current_text(egui_view_t *self);
void egui_view_autocomplete_set_max_visible_items(egui_view_t *self, uint8_t max_items);
void egui_view_autocomplete_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_autocomplete_expand(egui_view_t *self);
void egui_view_autocomplete_collapse(egui_view_t *self);
uint8_t egui_view_autocomplete_is_expanded(egui_view_t *self);
void egui_view_autocomplete_set_on_selected_listener(egui_view_t *self, egui_view_on_autocomplete_selected_listener_t listener);
void egui_view_autocomplete_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_AUTOCOMPLETE_H_ */
