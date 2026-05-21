#ifndef _EGUI_VIEW_AUTOCOMPLETE_H_
#define _EGUI_VIEW_AUTOCOMPLETE_H_

#include "egui_view_combobox.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired when the user commits one suggestion from the dropdown. */
typedef egui_view_on_combobox_selected_listener_t egui_view_on_autocomplete_selected_listener_t;

typedef struct egui_view_autocomplete egui_view_autocomplete_t;
/**
 * @brief Thin wrapper that exposes combobox behavior as autocomplete naming.
 *
 * The current implementation reuses the combobox storage and rendering logic
 * directly. The wrapper mainly gives applications a more task-oriented API
 * when suggestions are driven by text input or command search UIs.
 */
struct egui_view_autocomplete
{
    egui_view_combobox_t combobox;
};

typedef struct egui_view_autocomplete_params egui_view_autocomplete_params_t;
/**
 * @brief Construction-time parameter block for one autocomplete dropdown.
 */
struct egui_view_autocomplete_params
{
    egui_region_t region;
    const char **suggestions;
    uint8_t suggestion_count;
    uint8_t current_index;
};

/** Build a parameter block for one suggestion list and initial selection. */
#define EGUI_VIEW_AUTOCOMPLETE_PARAMS_INIT(_name, _x, _y, _w, _h, _suggestions, _count, _index)                                                                \
    static const egui_view_autocomplete_params_t _name = {                                                                                                     \
            .region = {{(_x), (_y)}, {(_w), (_h)}}, .suggestions = (_suggestions), .suggestion_count = (_count), .current_index = (_index)}

/** Apply region, suggestion array, count, and initial selection from one parameter block. */
void egui_view_autocomplete_apply_params(egui_view_t *self, const egui_view_autocomplete_params_t *params);
/** Initialize the autocomplete wrapper and immediately apply its parameter block. */
void egui_view_autocomplete_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_autocomplete_params_t *params);

/** Borrow the suggestion array from caller storage. The current index is clamped through the underlying combobox logic. */
void egui_view_autocomplete_set_suggestions(egui_view_t *self, const char **suggestions, uint8_t count);
/** Return the borrowed suggestion array currently exposed by the wrapped combobox. */
const char **egui_view_autocomplete_get_suggestions(egui_view_t *self);
/** Return the number of suggestions currently exposed by the wrapped combobox. */
uint8_t egui_view_autocomplete_get_suggestion_count(egui_view_t *self);
/** Change the selected suggestion programmatically. Out-of-range indices are ignored and no listener is fired. */
void egui_view_autocomplete_set_current_index(egui_view_t *self, uint8_t index);
/** Return the current selected suggestion index. */
uint8_t egui_view_autocomplete_get_current_index(egui_view_t *self);
/** Return the currently selected suggestion text, or NULL when there are no suggestions. */
const char *egui_view_autocomplete_get_current_text(egui_view_t *self);
/** Limit how many suggestions may be visible when expanded. Values below 1 clamp to 1. */
void egui_view_autocomplete_set_max_visible_items(egui_view_t *self, uint8_t max_items);
/** Return the configured maximum number of visible suggestions. */
uint8_t egui_view_autocomplete_get_max_visible_items(egui_view_t *self);
/** Override the text font used by the wrapped combobox. Passing NULL is ignored by this wrapper. */
void egui_view_autocomplete_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the font used by the wrapped combobox. */
const egui_font_t *egui_view_autocomplete_get_font(egui_view_t *self);
/** Expand the suggestion dropdown. */
void egui_view_autocomplete_expand(egui_view_t *self);
/** Collapse the suggestion dropdown back to its header row. */
void egui_view_autocomplete_collapse(egui_view_t *self);
/** Return whether the dropdown is currently expanded. */
uint8_t egui_view_autocomplete_is_expanded(egui_view_t *self);
/** Register the listener fired when the user selects one suggestion. */
void egui_view_autocomplete_set_on_selected_listener(egui_view_t *self, egui_view_on_autocomplete_selected_listener_t listener);
/** Return the registered selection listener. */
egui_view_on_autocomplete_selected_listener_t egui_view_autocomplete_get_on_selected_listener(egui_view_t *self);
/** Initialize the autocomplete view as a thin combobox-based wrapper. */
void egui_view_autocomplete_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_AUTOCOMPLETE_H_ */
