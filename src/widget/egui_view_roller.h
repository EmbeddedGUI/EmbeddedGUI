#ifndef _EGUI_VIEW_ROLLER_H_
#define _EGUI_VIEW_ROLLER_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired when touch interaction finishes with a selected roller index. */
typedef void (*egui_view_on_roller_selected_listener_t)(egui_view_t *self, uint8_t index);

/** Wheel-style selector that keeps the current item in the center highlight row. */
typedef struct egui_view_roller egui_view_roller_t;
struct egui_view_roller
{
    egui_view_t base;

    egui_view_on_roller_selected_listener_t on_selected; /* Notified when the user releases after a drag. */
    const char **items;                                  /* Text array shown by the roller rows. */
    uint8_t item_count;                                  /* Number of valid entries in items. */
    uint8_t current_index;                               /* Item aligned to the center highlight row. */
    uint8_t visible_count;                               /* Number of rows drawn at once, usually an odd count. */
    egui_dim_t scroll_offset;                            /* Temporary pixel offset while dragging between rows. */
    egui_dim_t last_touch_y;                             /* Previous touch y used to accumulate drag distance. */
    uint8_t is_dragging;                                 /* Non-zero while the roller owns the drag gesture. */
    egui_color_t text_color;                             /* Color for non-selected rows. */
    egui_color_t selected_text_color;                    /* Color for the centered selected row. */
    egui_color_t highlight_color;                        /* Background color for the center selection band. */
    const egui_font_t *font;                             /* Font used for every visible item. */
};

// ============== Roller Params ==============
typedef struct egui_view_roller_params egui_view_roller_params_t;
/** Construction-time parameters for a roller widget. */
struct egui_view_roller_params
{
    egui_region_t region;
    const char **items;
    uint8_t item_count;
    uint8_t current_index;
};

#define EGUI_VIEW_ROLLER_PARAMS_INIT(_name, _x, _y, _w, _h, _items, _count, _index)                                                                            \
    static const egui_view_roller_params_t _name = {                                                                                                           \
            .region = {{(_x), (_y)}, {(_w), (_h)}}, .items = (_items), .item_count = (_count), .current_index = (_index)}

/** Apply a roller parameter block after initialization. */
void egui_view_roller_apply_params(egui_view_t *self, const egui_view_roller_params_t *params);
/** Initialize a roller and immediately apply its parameter block. */
void egui_view_roller_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_roller_params_t *params);

/** Register the callback fired after drag selection completes. */
void egui_view_roller_set_on_selected_listener(egui_view_t *self, egui_view_on_roller_selected_listener_t listener);
/** Return the callback fired after drag selection completes. */
egui_view_on_roller_selected_listener_t egui_view_roller_get_on_selected_listener(egui_view_t *self);
/** Replace the item array. The current index is clamped if needed. */
void egui_view_roller_set_items(egui_view_t *self, const char **items, uint8_t count);
/** Return the item array currently used by the roller. */
const char **egui_view_roller_get_items(egui_view_t *self);
/** Set the current selection programmatically and reset any drag offset. */
void egui_view_roller_set_current_index(egui_view_t *self, uint8_t index);
/** Return the current selected item index. */
uint8_t egui_view_roller_get_current_index(egui_view_t *self);
/** Return the text of the currently selected item, or NULL when unset or self is NULL. */
const char *egui_view_roller_get_selected_text(egui_view_t *self);
/** Return the number of items in the roller. Returns 0 when self is NULL. */
uint8_t egui_view_roller_get_item_count(egui_view_t *self);
/** Return the color used for non-selected item text. */
egui_color_t egui_view_roller_get_text_color(egui_view_t *self);
/** Return the background color of the center highlight band. */
egui_color_t egui_view_roller_get_highlight_color(egui_view_t *self);
/** Return the color used for the selected (center) item text. */
egui_color_t egui_view_roller_get_selected_text_color(egui_view_t *self);
/** Return the number of visible rows drawn at once. */
uint8_t egui_view_roller_get_visible_count(egui_view_t *self);
/** Return the font used for all item rows. */
const egui_font_t *egui_view_roller_get_font(egui_view_t *self);
/** Default draw hook used by the roller API table. */
void egui_view_roller_on_draw(egui_view_t *self);
/** Initialize the touch-driven roller widget. */
void egui_view_roller_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ROLLER_H_ */
