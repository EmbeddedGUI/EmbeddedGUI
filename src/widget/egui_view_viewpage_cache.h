#ifndef _EGUI_VIEW_VIEWPAGE_CACHE_H_
#define _EGUI_VIEW_VIEWPAGE_CACHE_H_

#include "egui_view_group.h"
#include "core/egui_theme.h"
#include "core/egui_scroller.h"
#include "font/egui_font.h"

#include "egui_view_linearlayout.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_VIEWPAGE_CACHE_MAX_PAGE_CNT    3
#define EGUI_VIEW_VIEWPAGE_CACHE_BASE_OFFSET    ((EGUI_VIEW_VIEWPAGE_CACHE_MAX_PAGE_CNT) / 2)

typedef void (*egui_view_viewpage_cache_on_page_changed_listener_t)(egui_view_t *self, int current_page_index);

typedef void* (*egui_view_viewpage_cache_on_page_load_listener_t)(egui_view_t *self, int current_page_index);

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

void egui_view_viewpage_cache_set_child_total_cnt(egui_view_t *self, int cnt);

void egui_view_viewpage_cache_set_on_page_changed_listener(egui_view_t *self, egui_view_viewpage_cache_on_page_changed_listener_t listener);
void egui_view_viewpage_cache_set_on_page_load_listener(egui_view_t *self, egui_view_viewpage_cache_on_page_load_listener_t listener);
void egui_view_viewpage_cache_set_on_page_free_listener(egui_view_t *self, egui_view_viewpage_cache_on_page_free_listener_t listener);

void egui_view_viewpage_cache_on_paged_free_all(egui_view_t *self);
void egui_view_viewpage_cache_add_child(egui_view_t *self, egui_view_t *child);
void egui_view_viewpage_cache_remove_child(egui_view_t *self, egui_view_t *child);
void egui_view_viewpage_cache_layout_childs(egui_view_t *self);
void egui_view_viewpage_cache_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height);
void egui_view_viewpage_cache_start_container_scroll(egui_view_t *self, int diff_x);
void egui_view_viewpage_cache_fling(egui_view_t *self, egui_float_t velocity_x);
void egui_view_viewpage_cache_scroll_to_page(egui_view_t *self, int page_index);
void egui_view_viewpage_cache_slow_scroll_to_page(egui_view_t *self);
void egui_view_viewpage_cache_set_current_page(egui_view_t *self, int page_index);
void egui_view_viewpage_cache_compute_scroll(egui_view_t *self);
void egui_view_viewpage_cache_check_begin_dragged(egui_view_t *self, egui_dim_t delta);
int egui_view_viewpage_cache_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event);
int egui_view_viewpage_cache_on_touch_event(egui_view_t *self, egui_motion_event_t *event);
void egui_view_viewpage_cache_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIEWPAGE_CACHE_H_ */
