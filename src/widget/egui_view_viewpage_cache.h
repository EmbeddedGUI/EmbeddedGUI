#ifndef _EGUI_VIEW_VIEWPAGE_CACHE_H_
#define _EGUI_VIEW_VIEWPAGE_CACHE_H_

#include "egui_view_group.h"
#include "core/egui_scroller.h"
#include "font/egui_font.h"

#include "egui_view_linearlayout.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Cache at most left/current/right logical pages at the same time. */
#define EGUI_VIEW_VIEWPAGE_CACHE_MAX_PAGE_CNT 3
/** Slot offset of the current page inside the three-page cache window. */
#define EGUI_VIEW_VIEWPAGE_CACHE_BASE_OFFSET  ((EGUI_VIEW_VIEWPAGE_CACHE_MAX_PAGE_CNT) / 2)

/** Fired after the active logical page changes. Animated scrolls report after snapping settles on a page. */
typedef void (*egui_view_viewpage_cache_on_page_changed_listener_t)(egui_view_t *self, int current_page_index);

/**
 * Create or load the page view for one logical page index.
 * Return the view pointer that should be inserted into the cache. The same pointer is later passed to the free listener.
 */
typedef void *(*egui_view_viewpage_cache_on_page_load_listener_t)(egui_view_t *self, int current_page_index);

/** Release one page previously returned by the load listener after it has been removed from the internal container. */
typedef void (*egui_view_viewpage_cache_on_page_free_listener_t)(egui_view_t *self, int current_page_index, egui_view_t *page);

typedef struct egui_view_viewpage_cache egui_view_viewpage_cache_t;
struct egui_view_viewpage_cache
{
    egui_view_group_t base;

    egui_view_linearlayout_t container; // container in view scroll

    egui_dim_t touch_slop;
    egui_dim_t last_motion_x;

    uint8_t is_begin_dragged;
    uint8_t current_page_index;
    uint8_t total_page_cnt;

    // egui_view_t* page_cache[EGUI_VIEW_VIEWPAGE_CACHE_MAX_PAGE_CNT]; // only cache left, center, right page

    egui_view_viewpage_cache_on_page_changed_listener_t listener;
    egui_view_viewpage_cache_on_page_load_listener_t load_listener;
    egui_view_viewpage_cache_on_page_free_listener_t free_listener;

    egui_scroller_t scroller;
};

// ============== ViewPage Cache Params ==============
typedef struct egui_view_viewpage_cache_params egui_view_viewpage_cache_params_t;
struct egui_view_viewpage_cache_params
{
    egui_region_t region;
};

#define EGUI_VIEW_VIEWPAGE_CACHE_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                            \
    static const egui_view_viewpage_cache_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

/** Apply a simple region-based parameter block to a cached viewpage. */
void egui_view_viewpage_cache_apply_params(egui_view_t *self, const egui_view_viewpage_cache_params_t *params);
/** Initialize a cached viewpage and immediately apply its parameter block. */
void egui_view_viewpage_cache_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_viewpage_cache_params_t *params);

/** Set the logical page count and resize the internal strip so page bounds match the new count. */
void egui_view_viewpage_cache_set_child_total_cnt(egui_view_t *self, int cnt);
/** Return the logical page count. Returns 0 when self is NULL. */
int egui_view_viewpage_cache_get_child_total_cnt(egui_view_t *self);

/** Register the callback fired when the active logical page changes. */
void egui_view_viewpage_cache_set_on_page_changed_listener(egui_view_t *self, egui_view_viewpage_cache_on_page_changed_listener_t listener);
/** Return the registered page-change callback, or NULL when unset or self is NULL. */
egui_view_viewpage_cache_on_page_changed_listener_t egui_view_viewpage_cache_get_on_page_changed_listener(egui_view_t *self);
/** Register the loader used to create nearby pages on demand. */
void egui_view_viewpage_cache_set_on_page_load_listener(egui_view_t *self, egui_view_viewpage_cache_on_page_load_listener_t listener);
/** Return the registered page-load callback, or NULL when unset or self is NULL. */
egui_view_viewpage_cache_on_page_load_listener_t egui_view_viewpage_cache_get_on_page_load_listener(egui_view_t *self);
/** Register the cleanup callback for pages previously produced by the load listener. */
void egui_view_viewpage_cache_set_on_page_free_listener(egui_view_t *self, egui_view_viewpage_cache_on_page_free_listener_t listener);
/** Return the registered page-free callback, or NULL when unset or self is NULL. */
egui_view_viewpage_cache_on_page_free_listener_t egui_view_viewpage_cache_get_on_page_free_listener(egui_view_t *self);

/** Release every page currently cached around the active center page. */
void egui_view_viewpage_cache_on_paged_free_all(egui_view_t *self);
/** Low-level helper that inserts a page view into the internal horizontal container. */
void egui_view_viewpage_cache_add_child(egui_view_t *self, egui_view_t *child);
/** Low-level helper that removes a page view from the internal horizontal container. */
void egui_view_viewpage_cache_remove_child(egui_view_t *self, egui_view_t *child);
/** Update the internal strip width from the logical page count. This does not load pages by itself. */
void egui_view_viewpage_cache_layout_childs(egui_view_t *self);
/** Set the viewport size and keep the cached page height in sync with the viewport. */
void egui_view_viewpage_cache_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height);
/** Move the internal page strip by a clamped X offset. Mainly used by drag and scroller code. */
void egui_view_viewpage_cache_start_container_scroll(egui_view_t *self, int diff_x);
/** Start inertial horizontal scrolling inside the current cached window. */
void egui_view_viewpage_cache_fling(egui_view_t *self, egui_float_t velocity_x);
/** Animate to a logical page index. The page-changed listener runs after the animation lands on a page. */
void egui_view_viewpage_cache_scroll_to_page(egui_view_t *self, int page_index);
/** Snap to the nearest logical page based on the current drag offset. */
void egui_view_viewpage_cache_slow_scroll_to_page(egui_view_t *self);
/** Jump to a logical page immediately, reload the nearby cache window, and notify listeners if the page changed. */
void egui_view_viewpage_cache_set_current_page(egui_view_t *self, int page_index);
/** Return the active logical page, or -1 when no page has been loaded yet. Returns 0 when self is NULL. */
int egui_view_viewpage_cache_get_current_page(egui_view_t *self);
/** Advance scrolling animation and trigger page reload once the strip settles on an exact page. */
void egui_view_viewpage_cache_compute_scroll(egui_view_t *self);
/** Promote the current gesture to a horizontal drag after movement exceeds touch slop. */
void egui_view_viewpage_cache_check_begin_dragged(egui_view_t *self, egui_dim_t delta);
/** Intercept touch when this cached viewpage should own the current swipe gesture. */
int egui_view_viewpage_cache_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Handle drag, fling, and page snapping for the cached page strip. */
int egui_view_viewpage_cache_on_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Initialize the cached horizontal page container and its lazy-loading state. */
void egui_view_viewpage_cache_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIEWPAGE_CACHE_H_ */
