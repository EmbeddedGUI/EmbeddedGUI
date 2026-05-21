#ifndef _EGUI_VIEW_H_
#define _EGUI_VIEW_H_

#include "core/egui_region.h"
#include "utils/egui_dlist.h"
#include "canvas/egui_canvas.h"
#include "core/egui_timer.h"
#include "core/egui_event.h"
#include "core/egui_motion_event.h"
#include "background/egui_background.h"
#include "widget/egui_style.h"

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

#if EGUI_CONFIG_FUNCTION_LONG_PRESS
/** Long-press listener invoked once after the finger has been held inside the view long enough. */
typedef void (*egui_view_on_long_press_listener_t)(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_LONG_PRESS */

#if EGUI_CONFIG_FUNCTION_SWIPE_LISTENER && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/** Direction of a completed swipe gesture. */
typedef enum egui_swipe_dir
{
    EGUI_SWIPE_DIR_LEFT = 0,
    EGUI_SWIPE_DIR_RIGHT = 1,
    EGUI_SWIPE_DIR_UP = 2,
    EGUI_SWIPE_DIR_DOWN = 3,
} egui_swipe_dir_t;
/** Swipe listener invoked on touch UP when the displacement exceeds the configured minimum. */
typedef void (*egui_view_on_swipe_listener_t)(egui_view_t *self, egui_swipe_dir_t dir);
#endif /* EGUI_CONFIG_FUNCTION_SWIPE_LISTENER */

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
/** Custom key listener. Return non-zero when the event is consumed. */
typedef int (*egui_view_on_key_listener_t)(egui_view_t *self, egui_key_event_t *event);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
/** Focus-change listener for custom widgets. */
typedef void (*egui_view_on_focus_change_listener_t)(egui_view_t *self, int is_focused);
/** Custom focus-frame drawing hook. The canvas work region has already been clipped to the focus frame. */
typedef void (*egui_view_on_draw_focus_frame_t)(egui_view_t *self, const egui_region_t *frame_region);
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
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    int (*perform_click)(egui_view_t *self);
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    int (*on_key)(egui_view_t *self, egui_key_event_t *event);
    int (*dispatch_key_event)(egui_view_t *self, egui_key_event_t *event);
    int (*on_key_event)(egui_view_t *self, egui_key_event_t *event);
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    void (*on_focus_changed)(egui_view_t *self, int is_focused);
    void (*on_draw_focus_frame)(egui_view_t *self, const egui_region_t *frame_region);
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

#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
/** View state bit-flags used to filter styles.  Combine with | for multi-condition styles. */
#define EGUI_VIEW_STATE_DEFAULT  0x00u /**< Always active — a style with mask=0 is applied unconditionally. */
#define EGUI_VIEW_STATE_PRESSED  0x01u /**< Set automatically while the view is held down. */
#define EGUI_VIEW_STATE_FOCUSED  0x02u /**< Set automatically while the view holds input focus. */
#define EGUI_VIEW_STATE_DISABLED 0x04u /**< Set automatically when is_enable == 0. */
#define EGUI_VIEW_STATE_CHECKED  0x08u /**< Set by egui_view_set_state_checked(); useful for toggle/checkbox. */
#endif                                 /* EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES */

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
    uint8_t is_focus_frame_visible : 1;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
    uint8_t layer; // layer for z-ordering within parent (higher = on top)
#endif

    egui_alpha_t alpha; // alpha of the view

#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE
    uint8_t has_own_alpha : 1; /* 1 when alpha was explicitly set; cascade skipped if 1 */
#endif
#if EGUI_CONFIG_FUNCTION_FLEXLAYOUT
    uint8_t flex_grow; /* 0 = no grow (default); >0 = proportional share of free space */
#endif

#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
    uint8_t view_state; /**< user-settable state bits (currently only EGUI_VIEW_STATE_CHECKED);
                         *   PRESSED/FOCUSED/DISABLED are derived on-the-fly from existing flags. */
#endif

#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE
    const egui_view_style_t *styles[EGUI_CONFIG_STYLE_MAX_PER_VIEW]; /* style stack, index 0 = lowest priority */
    uint8_t style_count;                                             /* number of styles currently in stack    */
#endif

#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    egui_event_listener_t event_listeners[EGUI_CONFIG_EVENT_MAX_LISTENERS_PER_VIEW];
    uint8_t event_listener_count;
#endif

    egui_view_padding_t padding;
    egui_view_margin_t margin;

    egui_dnode_t node; // used for linked list

    egui_view_group_t *parent; // parent view

    egui_region_t region; // size of the region

    egui_region_t region_screen; // size of the region in screen coordinate
    uint32_t last_dirty_epoch;

    egui_background_t *background; // background

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    egui_view_on_click_listener_t on_click_listener; // click listener is dense, keep it inline
#endif                                               // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if EGUI_CONFIG_FUNCTION_LONG_PRESS && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_view_on_long_press_listener_t on_long_press_listener; /* fired once after holding inside view */
    uint32_t _lp_press_tick;                                   /* egui_timer_get_current_time() when DOWN was captured */
    uint8_t _lp_fired;                                         /* non-zero once the long-press callback has been dispatched */
    uint8_t _lp_active;                                        /* non-zero while tracking a held DOWN (set on DOWN, cleared on UP/CANCEL/slide-out) */
#endif                                                         /* EGUI_CONFIG_FUNCTION_LONG_PRESS */

#if EGUI_CONFIG_FUNCTION_SWIPE_LISTENER && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_view_on_swipe_listener_t on_swipe_listener; /* invoked on UP when displacement >= EGUI_CONFIG_SWIPE_MIN_DISPLACEMENT_PX */
    egui_dim_t _swipe_down_x;                        /* touch DOWN x coordinate for displacement measurement */
    egui_dim_t _swipe_down_y;                        /* touch DOWN y coordinate for displacement measurement */
#endif                                               /* EGUI_CONFIG_FUNCTION_SWIPE_LISTENER */

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    const egui_shadow_t *shadow; // shadow effect
#endif

#if EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA
    uint8_t ext_click_area; /* extra hit-test margin in pixels on all four sides (0 = no extension) */
#endif

#if EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    uint8_t is_draggable : 1;   /* when 1, touch MOVE events translate region.location */
    uint8_t _drag_tracking : 1; /* set on DOWN inside; MOVE only translates while set */
    egui_dim_t _drag_last_x;    /* touch position recorded on last DOWN or MOVE */
    egui_dim_t _drag_last_y;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_dim_t focus_frame_margin;
    egui_dim_t focus_frame_stroke;
    egui_color_t focus_frame_color;
    egui_alpha_t focus_frame_alpha;
#endif

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    const char *name; // name of the view
#endif

#if EGUI_CONFIG_FUNCTION_VIEW_USER_DATA
    void *user_data; /**< Arbitrary pointer for caller use; never touched by the framework. */
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
/** Return whether view is the same as ancestor or belongs to ancestor's subtree. */
int egui_view_is_self_or_descendant_of(egui_view_t *view, egui_view_t *ancestor);
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
/** Return the parent group pointer, or NULL when this view has no parent. */
egui_view_group_t *egui_view_get_parent(egui_view_t *self);
/** Set the view alpha mixed into all subsequent drawing for this widget. */
void egui_view_set_alpha(egui_view_t *self, egui_alpha_t alpha);
/** Return the view's own alpha value (255 = fully opaque). */
egui_alpha_t egui_view_get_alpha(egui_view_t *self);
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
/** Convenience helper for overriding only the touch callback in a copied API table. */
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_view_override_api_on_touch(egui_view_t *self, egui_view_api_t *api, egui_view_on_touch_listener_t listener);
#endif

/** Register a click listener and make the view clickable. */
void egui_view_set_on_click_listener(egui_view_t *self, egui_view_on_click_listener_t listener);
/** Return the currently installed click listener. */
egui_view_on_click_listener_t egui_view_get_on_click_listener(egui_view_t *self);

#if EGUI_CONFIG_FUNCTION_LONG_PRESS && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/**
 * @brief Register a long-press listener.
 *
 * Once set, the view will fire @p listener a single time after the finger has been held
 * inside the view for at least EGUI_CONFIG_LONG_PRESS_DURATION_MS milliseconds without
 * sliding out.  When the long-press fires, the subsequent UP event does NOT trigger the
 * on_click_listener.  Pass NULL to clear the long-press listener.
 * The view does not need to be clickable for the long-press listener to work.
 */
void egui_view_set_on_long_press_listener(egui_view_t *self, egui_view_on_long_press_listener_t listener);
/** Return the currently installed long-press listener, or NULL. */
egui_view_on_long_press_listener_t egui_view_get_on_long_press_listener(egui_view_t *self);
/**
 * @brief Poll one view for long-press expiry.
 *
 * Fires the long-press listener once when the view has been held pressed for at least
 * EGUI_CONFIG_LONG_PRESS_DURATION_MS.  Called automatically by egui_polling_work; exposed
 * here so unit tests can drive long-press detection without the full polling pipeline.
 */
void egui_view_poll_long_press(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_LONG_PRESS */

#if EGUI_CONFIG_FUNCTION_SWIPE_LISTENER && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/**
 * @brief Register a swipe listener.
 *
 * Once set, the view will invoke @p listener on touch UP when the total finger displacement
 * from the DOWN position equals or exceeds EGUI_CONFIG_SWIPE_MIN_DISPLACEMENT_PX.  The
 * dominant axis determines whether EGUI_SWIPE_DIR_LEFT/RIGHT/UP/DOWN is reported.
 * The view must be clickable (is_clickable=1) for swipe detection to operate.
 * Pass NULL to clear the swipe listener.
 */
void egui_view_set_on_swipe_listener(egui_view_t *self, egui_view_on_swipe_listener_t listener);
/** Return the currently installed swipe listener, or NULL. */
egui_view_on_swipe_listener_t egui_view_get_on_swipe_listener(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_SWIPE_LISTENER */

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
/** Return the local X coordinate of this view. */
egui_dim_t egui_view_get_x(egui_view_t *self);
/** Return the local Y coordinate of this view. */
egui_dim_t egui_view_get_y(egui_view_t *self);
/** Resize the local bounds and mark the view dirty. */
void egui_view_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height);
/** Return the width of this view. */
egui_dim_t egui_view_get_width(egui_view_t *self);
/** Return the height of this view. */
egui_dim_t egui_view_get_height(egui_view_t *self);
/** Return the drawable width (width minus left+right padding). Result is clamped to 0. */
egui_dim_t egui_view_get_content_width(egui_view_t *self);
/** Return the drawable height (height minus top+bottom padding). Result is clamped to 0. */
egui_dim_t egui_view_get_content_height(egui_view_t *self);
/** Return the absolute X coordinate of the view in screen space. */
egui_dim_t egui_view_get_screen_x(egui_view_t *self);
/** Return the absolute Y coordinate of the view in screen space. */
egui_dim_t egui_view_get_screen_y(egui_view_t *self);
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

#if EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA
/**
 * @brief Expand the touch hit-test area beyond the visual bounds.
 *
 * The expansion is applied symmetrically on all four sides.
 * Pass 0 to restore the default (hit area == visual region).
 * The maximum value is 127 pixels.
 * Does NOT change the layout size or drawn region.
 */
void egui_view_set_ext_click_area(egui_view_t *self, uint8_t extra_px);
/** Return the current hit-test expansion in pixels. */
uint8_t egui_view_get_ext_click_area(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA */

#if EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/**
 * @brief Enable or disable free dragging for a view.
 *
 * When @p is_draggable is non-zero, the view repositions itself by the finger
 * delta on every MOVE event.  Touch events are consumed (preventing dispatch
 * to parent containers).  Pass 0 to disable.
 */
void egui_view_set_draggable(egui_view_t *self, int is_draggable);
/** Return non-zero when the view is in draggable mode. */
int egui_view_get_draggable(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW */

/** Set per-side inner padding used by text/layout helpers. */
void egui_view_set_padding(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top,
                           egui_dim_margin_padding_t bottom);
/** Set the same inner padding on all four sides. */
void egui_view_set_padding_all(egui_view_t *self, egui_dim_margin_padding_t padding);
/** Return the left inner padding. */
egui_dim_margin_padding_t egui_view_get_padding_left(egui_view_t *self);
/** Return the right inner padding. */
egui_dim_margin_padding_t egui_view_get_padding_right(egui_view_t *self);
/** Return the top inner padding. */
egui_dim_margin_padding_t egui_view_get_padding_top(egui_view_t *self);
/** Return the bottom inner padding. */
egui_dim_margin_padding_t egui_view_get_padding_bottom(egui_view_t *self);
/** Set per-side outer margin consumed by parent layout containers. */
void egui_view_set_margin(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top,
                          egui_dim_margin_padding_t bottom);
/** Set the same outer margin on all four sides. */
void egui_view_set_margin_all(egui_view_t *self, egui_dim_margin_padding_t margin);
/** Return the left outer margin. */
egui_dim_margin_padding_t egui_view_get_margin_left(egui_view_t *self);
/** Return the right outer margin. */
egui_dim_margin_padding_t egui_view_get_margin_right(egui_view_t *self);
/** Return the top outer margin. */
egui_dim_margin_padding_t egui_view_get_margin_top(egui_view_t *self);
/** Return the bottom outer margin. */
egui_dim_margin_padding_t egui_view_get_margin_bottom(egui_view_t *self);
/** Position this view inside its parent content area using EGUI_ALIGN_* flags and offsets. */
void egui_view_align_to_parent(egui_view_t *self, uint8_t align_type, egui_dim_t offset_x, egui_dim_t offset_y);

#if EGUI_CONFIG_FUNCTION_FLEXLAYOUT
/** Set the flex grow factor used by FlexLayout to distribute free space. 0 means no grow. */
void egui_view_set_flex_grow(egui_view_t *self, uint8_t grow);
/** Return the current flex grow factor. */
uint8_t egui_view_get_flex_grow(egui_view_t *self);
#endif

#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE
/**
 * Push a shared style onto this view's style stack (max EGUI_CONFIG_STYLE_MAX_PER_VIEW).
 * Styles added later have higher cascade priority.  The view's own inline
 * background / alpha always win over all styles.
 * Returns 0 on success, -1 if the stack is full or arguments are NULL.
 */
int egui_view_add_style(egui_view_t *self, const egui_view_style_t *style);

/**
 * Remove the first occurrence of a style from this view's style stack.
 * Remaining entries are compacted.  Returns 0 on success, -1 if not found.
 */
int egui_view_remove_style(egui_view_t *self, const egui_view_style_t *style);

/** Remove all styles from this view's style stack. */
void egui_view_clear_styles(egui_view_t *self);

/**
 * Resolve the effective background by cascading: view's inline background
 * first, then styles from highest to lowest priority, then NULL.
 */
const egui_background_t *egui_view_get_effective_background(egui_view_t *self);

/**
 * Resolve the effective alpha: view's own alpha if explicitly set via
 * egui_view_set_alpha, otherwise the highest-priority style alpha, otherwise
 * EGUI_ALPHA_100.
 */
egui_alpha_t egui_view_get_effective_alpha(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_STYLE_CASCADE */

#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
/**
 * @brief Set or clear the CHECKED state bit on a view.
 *
 * Use this for toggle buttons and checkboxes.  The view is invalidated so
 * any state-filtered styles are applied on the next frame.
 */
void egui_view_set_state_checked(egui_view_t *self, int checked);
/** Return non-zero if the view's CHECKED state bit is currently set. */
int egui_view_get_state_checked(egui_view_t *self);
/**
 * @brief Compute the full current state bitmask for a view.
 *
 * Combines is_pressed, is_focused, is_enable, and view_state into a single
 * byte that can be compared against a style's state_mask.
 */
uint8_t egui_view_get_computed_state(const egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES */

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
/** Enable or disable the common focus frame for this view. */
void egui_view_set_focus_frame_visible(egui_view_t *self, int is_visible);
/** Return whether the common focus frame is enabled for this view. */
int egui_view_get_focus_frame_visible(egui_view_t *self);
/** Set the common rectangular focus frame style used by this view. */
void egui_view_set_focus_frame_style(egui_view_t *self, egui_dim_t margin, egui_dim_t stroke, egui_color_t color, egui_alpha_t alpha);
/** Copy the common rectangular focus frame style used by this view. */
void egui_view_get_focus_frame_style(egui_view_t *self, egui_dim_t *margin, egui_dim_t *stroke, egui_color_t *color, egui_alpha_t *alpha);
/** Copy the screen-space rectangle used by the common focus frame. */
void egui_view_get_focus_frame_region(egui_view_t *self, egui_region_t *region);
/** Mark the common focus frame region dirty so focus changes erase/redraw cleanly. */
void egui_view_invalidate_focus_region(egui_view_t *self);
/** Convenience helper for overriding only the focus-change callback. */
void egui_view_override_api_on_focus_changed(egui_view_t *self, egui_view_api_t *api, egui_view_on_focus_change_listener_t listener);
/** Convenience helper for overriding only the focus-frame draw callback. */
void egui_view_override_api_on_draw_focus_frame(egui_view_t *self, egui_view_api_t *api, egui_view_on_draw_focus_frame_t listener);
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

#include "core/egui_timer.h"

/**
 * @brief Start a periodic timer that fires a callback at a fixed interval.
 *
 * This is a thin convenience wrapper around egui_timer_start_timer().
 * The caller owns the @p timer storage and must ensure it remains valid until
 * egui_view_stop_periodic() is called.
 *
 * @param view         The view that owns the periodic action (used to obtain the core).
 * @param timer        Caller-owned egui_timer_t handle (must outlive the timer).
 * @param user_data    Arbitrary pointer passed back in the timer callback.
 * @param callback     Function called each time the interval elapses.
 * @param period_ms    Interval in milliseconds.
 */
void egui_view_start_periodic(egui_view_t *view, egui_timer_t *timer, void *user_data, egui_timer_callback_func callback, uint32_t period_ms);

/**
 * @brief Stop an active periodic timer.
 *
 * @param view   The view whose core was used to start the timer.
 * @param timer  The same handle passed to egui_view_start_periodic().
 */
void egui_view_stop_periodic(egui_view_t *view, egui_timer_t *timer);

#if EGUI_CONFIG_FUNCTION_VIEW_USER_DATA
/** Store an arbitrary pointer on the view for use by the caller; the framework never reads it. */
void egui_view_set_user_data(egui_view_t *self, void *user_data);
/** Retrieve the pointer previously stored by egui_view_set_user_data(); NULL if never set. */
void *egui_view_get_user_data(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_VIEW_USER_DATA */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

/**
 * @brief Test whether a screen point lands inside the view's hit region.
 *
 * When EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA is enabled and the view has a
 * non-zero ext_click_area, the test expands the region on all four sides.
 * Otherwise the test is identical to egui_region_pt_in_rect(&view->region_screen, x, y).
 */
__EGUI_STATIC_INLINE__ int egui_view_hit_test(const egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
#if EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA
    if (self->ext_click_area > 0)
    {
        egui_dim_t e = (egui_dim_t)self->ext_click_area;
        return x >= self->region_screen.location.x - e && x < self->region_screen.location.x + self->region_screen.size.width + e &&
               y >= self->region_screen.location.y - e && y < self->region_screen.location.y + self->region_screen.size.height + e;
    }
#endif
    return egui_region_pt_in_rect(&self->region_screen, x, y);
}

#endif /* _EGUI_VIEW_H_ */
