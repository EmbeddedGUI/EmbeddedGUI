#ifndef _EGUI_VIEW_PATTERN_LOCK_H_
#define _EGUI_VIEW_PATTERN_LOCK_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_PATTERN_LOCK_GRID_SIZE 3
#define EGUI_VIEW_PATTERN_LOCK_MAX_NODES (EGUI_VIEW_PATTERN_LOCK_GRID_SIZE * EGUI_VIEW_PATTERN_LOCK_GRID_SIZE)

/** Listener fired after a gesture ends with enough nodes to count as a valid pattern. */
typedef void (*egui_view_on_pattern_complete_listener_t)(egui_view_t *self, uint8_t node_count);
/** Listener fired when the gesture ends. `nodes` points to the internal node-order buffer and `valid` reports the min-node check result. */
typedef void (*egui_view_on_pattern_finish_listener_t)(egui_view_t *self, const uint8_t *nodes, uint8_t node_count, uint8_t valid);

typedef struct egui_view_pattern_lock egui_view_pattern_lock_t;
struct egui_view_pattern_lock
{
    egui_view_t base;

    uint8_t min_nodes;
    uint8_t node_count;
    uint8_t nodes[EGUI_VIEW_PATTERN_LOCK_MAX_NODES];
    uint8_t selected[EGUI_VIEW_PATTERN_LOCK_MAX_NODES];
    uint8_t is_tracking;
    uint8_t is_completed;
    uint8_t show_error;
    uint8_t touch_expand;

    egui_dim_t cursor_x;
    egui_dim_t cursor_y;

    egui_alpha_t alpha;
    egui_color_t bg_color;
    egui_color_t border_color;
    egui_color_t node_color;
    egui_color_t active_node_color;
    egui_color_t line_color;
    egui_color_t error_color;

    egui_view_on_pattern_complete_listener_t on_pattern_complete;
    egui_view_on_pattern_finish_listener_t on_pattern_finish;
};

typedef struct egui_view_pattern_lock_params egui_view_pattern_lock_params_t;
struct egui_view_pattern_lock_params
{
    egui_region_t region;
    uint8_t min_nodes;
    uint8_t touch_expand;
};

#define EGUI_VIEW_PATTERN_LOCK_PARAMS_INIT(_name, _x, _y, _w, _h, _min_nodes, _touch_expand)                                                                   \
    static const egui_view_pattern_lock_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .min_nodes = (_min_nodes), .touch_expand = (_touch_expand)}

/** Apply region, minimum-node count, and touch hit expansion from one parameter block. */
void egui_view_pattern_lock_apply_params(egui_view_t *self, const egui_view_pattern_lock_params_t *params);
/** Initialize a pattern-lock view and immediately apply its parameter block. */
void egui_view_pattern_lock_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_pattern_lock_params_t *params);

/** Set the minimum number of nodes required for a valid pattern. Values clamp to the 3x3 grid capacity. */
void egui_view_pattern_lock_set_min_nodes(egui_view_t *self, uint8_t min_nodes);
/** Return the minimum node count currently required for validity. */
uint8_t egui_view_pattern_lock_get_min_nodes(egui_view_t *self);
/** Return how many nodes are currently stored in the active or finished pattern. */
uint8_t egui_view_pattern_lock_get_node_count(egui_view_t *self);
/** Return the internal node-order buffer. The pointer stays owned by the widget and may change after reset or new input. */
const uint8_t *egui_view_pattern_lock_get_nodes(egui_view_t *self);
/** Return one stored node by order index, or 0xFF when the order index is invalid. */
uint8_t egui_view_pattern_lock_get_node(egui_view_t *self, uint8_t index);
/** Return whether a grid node is selected in the active or finished pattern. */
uint8_t egui_view_pattern_lock_get_selected(egui_view_t *self, uint8_t node);
/** Return whether a pointer gesture is currently tracking a pattern. */
uint8_t egui_view_pattern_lock_get_is_tracking(egui_view_t *self);
/** Return whether the current stored pattern has finished. */
uint8_t egui_view_pattern_lock_get_is_completed(egui_view_t *self);
/** Return whether the current stored pattern is marked invalid for display. */
uint8_t egui_view_pattern_lock_get_show_error(egui_view_t *self);
/** Clear the tracked pattern, error state, and pressed state. */
void egui_view_pattern_lock_clear_pattern(egui_view_t *self);
/** Expand the circular hit area used for touch selection. Values clamp to an internal maximum. */
void egui_view_pattern_lock_set_touch_expand(egui_view_t *self, uint8_t touch_expand);
/** Return the configured touch hit-area expansion in pixels. */
uint8_t egui_view_pattern_lock_get_touch_expand(egui_view_t *self);

/** Register the listener fired only when the finished pattern is valid. */
void egui_view_pattern_lock_set_on_pattern_complete_listener(egui_view_t *self, egui_view_on_pattern_complete_listener_t listener);
/** Return the listener fired only when the finished pattern is valid. */
egui_view_on_pattern_complete_listener_t egui_view_pattern_lock_get_on_pattern_complete_listener(egui_view_t *self);
/** Register the listener fired whenever the gesture ends, including invalid patterns. */
void egui_view_pattern_lock_set_on_pattern_finish_listener(egui_view_t *self, egui_view_on_pattern_finish_listener_t listener);
/** Return the listener fired whenever the gesture ends. */
egui_view_on_pattern_finish_listener_t egui_view_pattern_lock_get_on_pattern_finish_listener(egui_view_t *self);
/** Set the background color of the pattern-lock panel. */
void egui_view_pattern_lock_set_bg_color(egui_view_t *self, egui_color_t color);
/** Return the background color of the pattern-lock panel. */
egui_color_t egui_view_pattern_lock_get_bg_color(egui_view_t *self);
/** Set the border color of the panel and selected nodes. */
void egui_view_pattern_lock_set_border_color(egui_view_t *self, egui_color_t color);
/** Return the border color of the panel and selected nodes. */
egui_color_t egui_view_pattern_lock_get_border_color(egui_view_t *self);
/** Set the outline color used by idle nodes. */
void egui_view_pattern_lock_set_node_color(egui_view_t *self, egui_color_t color);
/** Return the outline color used by idle nodes. */
egui_color_t egui_view_pattern_lock_get_node_color(egui_view_t *self);
/** Set the fill color used by selected nodes in a normal pattern. */
void egui_view_pattern_lock_set_active_node_color(egui_view_t *self, egui_color_t color);
/** Return the fill color used by selected nodes in a normal pattern. */
egui_color_t egui_view_pattern_lock_get_active_node_color(egui_view_t *self);
/** Set the line color used to connect selected nodes in a normal pattern. */
void egui_view_pattern_lock_set_line_color(egui_view_t *self, egui_color_t color);
/** Return the line color used to connect selected nodes in a normal pattern. */
egui_color_t egui_view_pattern_lock_get_line_color(egui_view_t *self);
/** Set the color used when the finished pattern is invalid. */
void egui_view_pattern_lock_set_error_color(egui_view_t *self, egui_color_t color);
/** Return the color used when the finished pattern is invalid. */
egui_color_t egui_view_pattern_lock_get_error_color(egui_view_t *self);

/** Default draw hook used by the pattern-lock API table. */
void egui_view_pattern_lock_on_draw(egui_view_t *self);
/** Initialize the 3x3 pattern-lock widget. Dragging across two-step gaps automatically inserts the bridge node. */
void egui_view_pattern_lock_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_PATTERN_LOCK_H_ */
