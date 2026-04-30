#ifndef _EGUI_VIEW_H_
#define _EGUI_VIEW_H_

#include "core/egui_region.h"
#include "utils/egui_dlist.h"
#include "canvas/egui_canvas.h"
#include "core/egui_timer.h"
#include "core/egui_motion_event.h"
#include "background/egui_background.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#include "core/egui_key_event.h"
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_api egui_view_api_t;

/** Custom touch listener. Return non-zero when the event is consumed. */
typedef int (*egui_view_on_touch_listener_t)(egui_view_t *self, egui_motion_event_t *event);
/** Click listener invoked after the built-in pressed/click flow succeeds. */
typedef void (*egui_view_on_click_listener_t)(egui_view_t *self);

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
/** Custom key listener. Return non-zero when the event is consumed. */
typedef int (*egui_view_on_key_listener_t)(egui_view_t *self, egui_key_event_t *event);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
/** Focus-change listener for custom widgets. */
typedef void (*egui_view_on_focus_change_listener_t)(egui_view_t *self, int is_focused);
#endif

/** Virtual-method table used by every widget type. Custom widgets usually copy and override selected entries. */
struct egui_view_api
{
    int (*dispatch_touch_event)(egui_view_t *self, egui_motion_event_t *event);
    int (*on_touch_event)(egui_view_t *self, egui_motion_event_t *event);
    // to reduce code size, just put the function pointer here, and implement it in egui_view_group.c
    int (*on_intercept_touch_event)(egui_view_t *self, egui_motion_event_t *event);
    void (*compute_scroll)(egui_view_t *self);
    void (*calculate_layout)(egui_view_t *self);
    void (*request_layout)(egui_view_t *self);
    void (*draw)(egui_view_t *self);
    void (*on_attach_to_window)(egui_view_t *self);
    void (*on_draw)(egui_view_t *self);
    void (*on_detach_from_window)(egui_view_t *self);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    int (*on_touch)(egui_view_t *self, egui_motion_event_t *event);
    int (*perform_click)(egui_view_t *self);
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    int (*on_key)(egui_view_t *self, egui_key_event_t *event);
    int (*dispatch_key_event)(egui_view_t *self, egui_key_event_t *event);
    int (*on_key_event)(egui_view_t *self, egui_key_event_t *event);
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    void (*on_focus_changed)(egui_view_t *self, int is_focused);
#endif
};

#define EGUI_VIEW_API_TABLE_NAME(_name) _name##_api_table

typedef struct egui_view_padding egui_view_padding_t;
/** Per-side inner spacing consumed inside the widget bounds. */
struct egui_view_padding
{
    egui_dim_margin_padding_t left;
    egui_dim_margin_padding_t right;
    egui_dim_margin_padding_t top;
    egui_dim_margin_padding_t bottom;
};

typedef struct egui_view_margin egui_view_margin_t;
/** Per-side outer spacing consumed by parent layout containers. */
struct egui_view_margin
{
    egui_dim_margin_padding_t left;
    egui_dim_margin_padding_t right;
    egui_dim_margin_padding_t top;
    egui_dim_margin_padding_t bottom;
};

/** Common base object shared by all widgets. */
struct egui_view
{
#if EGUI_CONFIG_DEBUG_VIEW_ID
    // id is used to identify the view, it should be unique in the view tree, and it is only for debugging purpose.
    uint16_t id; // ID of the view
#endif

    uint8_t is_enable : 1;         // whether the view is enabled
    uint8_t is_visible : 1;        // whether the view is visible
    uint8_t is_gone : 1;           // whether the view is gone
    uint8_t is_pressed : 1;        // whether the view is pressed
    uint8_t is_clickable : 1;      // whether the view is clickable
    uint8_t is_request_layout : 1; // whether the view is requested to layout
    uint8_t is_attached_to_window : 1;
#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
    uint8_t is_dirty_passthrough : 1; // structural container only: emit per-child dirty rects, not self
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    uint8_t is_focusable : 1;      // whether the view can receive focus
    uint8_t is_focused : 1;        // whether the view currently has focus
    uint8_t is_no_focus_clear : 1; // when touched, do not clear other views' focus (used by keyboard keys)
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
    uint8_t layer; // layer for z-ordering within parent (higher = on top)
#endif

    egui_alpha_t alpha; // alpha of the view

    egui_view_padding_t padding;
    egui_view_margin_t margin;

    egui_dnode_t node; // used for linked list

    egui_view_group_t *parent; // parent view

    egui_region_t region; // size of the region

    egui_region_t region_screen; // size of the region in screen coordinate
    uint32_t last_dirty_epoch;

    egui_background_t *background; // background

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_view_on_click_listener_t on_click_listener; // click listener is dense, keep it inline
#endif                                               // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    const egui_shadow_t *shadow; // shadow effect
#endif

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    const char *name; // name of the view
#endif

    egui_core_t *core; // owning core instance, set during init

    const egui_view_api_t *api; // api of the view
};

/* Dirty-region helpers. Sub-regions are expressed in the view's local coordinates. */
/** Mark the whole view dirty and request layout recalculation on the next frame. */
void egui_view_invalidate(egui_view_t *self);
/** Mark the current full local bounds dirty. Useful for pressed/background changes. */
void egui_view_invalidate_full(egui_view_t *self);
/** Mark only one local sub-region dirty to reduce redraw cost. */
void egui_view_invalidate_region(egui_view_t *self, const egui_region_t *dirty_region);
/** Mark this visible subtree dirty, using child regions for dirty-passthrough containers without their own background. */
void egui_view_invalidate_visible_tree(egui_view_t *self);
/** Return whether the owning core still has pending dirty work to flush. */
uint8_t egui_view_has_pending_dirty(egui_view_t *self);

/* Context helpers for reaching the owning core, canvas, scene objects, and input state. */
/** Return the core pointer bound during init, or NULL when the view is not initialized. */
egui_core_t *egui_view_get_core(egui_view_t *self);
/** Return the current dirty-epoch counter from the owning core, or 0 when unavailable. */
uint32_t egui_view_get_dirty_epoch(egui_view_t *self);
/** Merge one screen-space dirty region into the owning core. */
void egui_view_update_region_dirty(egui_view_t *self, egui_region_t *region_dirty);
/** Return the canvas owned by the same core as this view, or NULL when unavailable. */
egui_canvas_t *egui_view_get_canvas(egui_view_t *self);
/** Return the view currently focused on the same core, or NULL. */
egui_view_t *egui_view_get_focused_view(egui_view_t *self);
/** Clear focus on the owning core, if focus support is enabled. */
void egui_view_clear_focus(egui_view_t *self);
/** Replace the active theme on the owning core. Passing NULL is ignored. */
void egui_view_set_theme(egui_view_t *self, const egui_theme_t *theme);
/** Return the activity that currently owns this view, or NULL when it is not inside one. */
egui_activity_t *egui_view_get_activity(egui_view_t *self);
/** Return the dialog currently managed by the owning core, or NULL. */
egui_dialog_t *egui_view_get_dialog(egui_view_t *self);
/** Return the toast controller currently managed by the owning core, or NULL. */
egui_toast_t *egui_view_get_toast(egui_view_t *self);
/** Show an info toast through the core toast controller for an explicit duration. */
void egui_view_show_toast_info_with_duration(egui_view_t *self, const char *text, uint16_t duration);
/** Show an info toast through the core toast controller using the default duration. */
void egui_view_show_toast_info(egui_view_t *self, const char *text);
/** Return the latest tracked horizontal input velocity from the owning core. */
egui_float_t egui_view_get_velocity_x(egui_view_t *self);
/** Return the latest tracked vertical input velocity from the owning core. */
egui_float_t egui_view_get_velocity_y(egui_view_t *self);
/** Start a timer on the same core as this view. */
int egui_view_start_timer(egui_view_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period);
/** Stop one timer previously started on the same core as this view. */
void egui_view_stop_timer(egui_view_t *self, egui_timer_t *handle);
/** Return whether one timer handle is currently active on the owning core. */
int egui_view_check_timer_start(egui_view_t *self, egui_timer_t *handle);
/** Add the view to the core root tree. Useful for overlays managed outside normal containers. */
void egui_view_add_to_root(egui_view_t *self);
/** Remove the view from the user root tree when it was attached there manually. */
void egui_view_remove_from_user_root(egui_view_t *self);
/** Relayout the children currently attached to the user root tree. */
void egui_view_layout_user_root(egui_view_t *self, uint8_t is_orientation_horizontal, uint8_t align_type);
/** Override tile scan order for advanced PFB flush scenarios. */
void egui_view_set_pfb_scan_direction(egui_view_t *self, uint8_t reverse_x, uint8_t reverse_y);
/** Restore the default PFB scan direction on the owning core. */
void egui_view_reset_pfb_scan_direction(egui_view_t *self);

typedef struct egui_sub_region
{
    egui_region_t region;
} egui_sub_region_t;

typedef struct egui_sub_region_table
{
    egui_sub_region_t *regions;
    uint16_t count;
} egui_sub_region_table_t;

/** Mark one precomputed sub-region dirty by index. */
void egui_view_invalidate_sub_region(egui_view_t *self, const egui_sub_region_table_t *table, uint16_t index);

/* Geometry, drawing, and subclassing helpers. */
/** Replace the background object pointer borrowed by this view. */
void egui_view_set_background(egui_view_t *self, egui_background_t *background);
/* Usually managed by container widgets instead of application code. */
/** Rebind the parent pointer only. Containers normally call this for you. */
void egui_view_set_parent(egui_view_t *self, egui_view_group_t *parent);
/** Set the view alpha mixed into all subsequent drawing for this widget. */
void egui_view_set_alpha(egui_view_t *self, egui_alpha_t alpha);
/** Accumulate the local position through the parent chain into screen-like coordinates. */
void egui_view_get_raw_pos(egui_view_t *self, egui_location_t *location);
/** Replace the full local region and request a relayout/redraw. */
void egui_view_layout(egui_view_t *self, egui_region_t *region);
/** Move the local origin to the given position. Used by scrolling containers. */
void egui_view_scroll_to(egui_view_t *self, egui_dim_t x, egui_dim_t y);
/** Offset the local origin relative to the current position. */
void egui_view_scroll_by(egui_view_t *self, egui_dim_t x, egui_dim_t y);
/** Return the drawable content area after subtracting padding. */
void egui_view_get_work_region(egui_view_t *self, egui_region_t *region);
/** Copy the current API table so a custom widget can override selected callbacks. */
void egui_view_copy_api(egui_view_t *self, egui_view_api_t *api);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/** Convenience helper for overriding only the touch callback in a copied API table. */
void egui_view_override_api_on_touch(egui_view_t *self, egui_view_api_t *api, egui_view_on_touch_listener_t listener);
#endif

/** Register a click listener and make the view clickable. */
void egui_view_set_on_click_listener(egui_view_t *self, egui_view_on_click_listener_t listener);
/** Return the currently installed click listener. */
egui_view_on_click_listener_t egui_view_get_on_click_listener(egui_view_t *self);

/* Basic state and geometry setters. Most of them invalidate the view when the value changes. */
/** Enable or disable the view. Disabled views stop handling touch/key events. */
void egui_view_set_enable(egui_view_t *self, int is_enable);
/** Return whether the view is enabled. */
int egui_view_get_enable(egui_view_t *self);
/** Enable or disable click handling for the built-in touch/key flow. */
void egui_view_set_clickable(egui_view_t *self, int is_clickable);
/** Return whether the view participates in the built-in click flow. */
int egui_view_get_clickable(egui_view_t *self);
/** Move the local origin and mark the view dirty. */
void egui_view_set_position(egui_view_t *self, egui_dim_t x, egui_dim_t y);
/** Resize the local bounds and mark the view dirty. */
void egui_view_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height);
/** Update the pressed state and redraw the whole widget when it changes. */
void egui_view_set_pressed(egui_view_t *self, int is_pressed);
/** Update the pressed state and only redraw the supplied local dirty region when possible. */
int egui_view_set_pressed_with_region(egui_view_t *self, int is_pressed, const egui_region_t *dirty_region);
/** Return whether the view is currently marked pressed. */
int egui_view_get_pressed(egui_view_t *self);
/** Hide or show the view without removing its layout slot. */
void egui_view_set_visible(egui_view_t *self, int is_visible);
/** Return whether the view is visible. */
int egui_view_get_visible(egui_view_t *self);
/**
 * Mark this view as a structural container that does not paint pixels in self space.
 * Layout-caused dirty marking is delegated to direct children:
 *
 * Pure translation emits swept old/new child rects; other layout causes request child layout.
 * The flag does not change background, shadow, visible, alpha,
 * or pressed dirty paths.
 * Requires EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH=1. When disabled, this API is a no-op and the getter returns 0.
 */
void egui_view_set_dirty_passthrough(egui_view_t *self, int on);
int egui_view_get_dirty_passthrough(egui_view_t *self);
/** Mark the view as gone so layout containers can skip its occupied space. */
void egui_view_set_gone(egui_view_t *self, int is_gone);
/** Return whether the view is currently marked gone. */
int egui_view_get_gone(egui_view_t *self);
/** Set per-side inner padding used by text/layout helpers. */
void egui_view_set_padding(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top,
                           egui_dim_margin_padding_t bottom);
/** Set the same inner padding on all four sides. */
void egui_view_set_padding_all(egui_view_t *self, egui_dim_margin_padding_t padding);
/** Set per-side outer margin consumed by parent layout containers. */
void egui_view_set_margin(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top,
                          egui_dim_margin_padding_t bottom);
/** Set the same outer margin on all four sides. */
void egui_view_set_margin_all(egui_view_t *self, egui_dim_margin_padding_t margin);
/** Replace the optional shadow descriptor borrowed by this view. */
void egui_view_set_shadow(egui_view_t *self, const egui_shadow_t *shadow);

/** Set the debug name shown by tracing/log helpers. */
#if EGUI_CONFIG_DEBUG_CLASS_NAME
void egui_view_set_view_name(egui_view_t *self, const char *name);
#else
#define egui_view_set_view_name(_self, _name) EGUI_UNUSED(_self)
#endif

/* Event-pipeline entry points. Custom widgets usually override these through api tables. */
/** Default intercept hook for base views. It always returns 0. */
int egui_view_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Entry point for touch dispatch. It runs any override hook before falling back to `on_touch_event`. */
int egui_view_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Fire the optional custom click hook and registered click listener. */
int egui_view_perform_click(egui_view_t *self);
/** Default touch handler implementing the base pressed/click behavior. */
int egui_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event);

/* Lifecycle and rendering hooks used by the framework and custom widgets. */
/** Attach this view to a live window once. Usually called by parent containers or the core. */
void egui_view_dispatch_attach_to_window(egui_view_t *self);
/** Detach this view from a live window once and clear focus if needed. */
void egui_view_dispatch_detach_from_window(egui_view_t *self);
/** Default attach hook for subclasses. */
void egui_view_on_attach_to_window(egui_view_t *self);
/** Default content-draw hook for subclasses after background/shadow setup. */
void egui_view_on_draw(egui_view_t *self);
/** Default detach hook for subclasses. */
void egui_view_on_detach_from_window(egui_view_t *self);
/** Common draw pipeline that handles visibility, alpha, shadow, background, and clipping. */
void egui_view_draw(egui_view_t *self);
/** Mark the view so `calculate_layout` recomputes `region_screen` on the next frame. */
void egui_view_request_layout(egui_view_t *self);
/** Default scroll-compute hook for non-scrollable base views. */
void egui_view_compute_scroll(egui_view_t *self);
/** Recompute screen-space layout from local geometry and parent padding. */
void egui_view_calculate_layout(egui_view_t *self);
/** Initialize the common base fields of a view. Call this first in custom widget init functions. */
void egui_view_init(egui_view_t *self, egui_core_t *core);

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
/* Keyboard helpers for widgets that participate in the key event pipeline. */
/** Entry point for key dispatch. It runs any override hook before falling back to `on_key_event`. */
int egui_view_dispatch_key_event(egui_view_t *self, egui_key_event_t *event);
/** Default key handler. Clickable views treat ENTER up as a click. */
int egui_view_on_key_event(egui_view_t *self, egui_key_event_t *event);
/** Convenience helper for overriding only the key callback in a copied API table. */
void egui_view_override_api_on_key(egui_view_t *self, egui_view_api_t *api, egui_view_on_key_listener_t listener);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
/** Enable or disable focus participation for this view. */
void egui_view_set_focusable(egui_view_t *self, int is_focusable);
/** Return whether the view may receive focus. */
int egui_view_get_focusable(egui_view_t *self);
/** Request focus for this view if it is visible, enabled, and focusable. */
void egui_view_request_focus(egui_view_t *self);
/** Convenience helper for overriding only the focus-change callback. */
void egui_view_override_api_on_focus_changed(egui_view_t *self, egui_view_api_t *api, egui_view_on_focus_change_listener_t listener);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
#define EGUI_VIEW_LAYER_BACKGROUND 0
#define EGUI_VIEW_LAYER_DEFAULT    0
#define EGUI_VIEW_LAYER_CONTENT    64
#define EGUI_VIEW_LAYER_OVERLAY    192
#define EGUI_VIEW_LAYER_TOP        255

/** Set the child layer used by layer-aware parents and reorder the child when needed. */
void egui_view_set_layer(egui_view_t *self, uint8_t layer);
/** Return the current layer value. */
uint8_t egui_view_get_layer(egui_view_t *self);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_H_ */
