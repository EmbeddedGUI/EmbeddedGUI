#ifndef _EGUI_VIEW_VIRTUAL_PAGE_H_
#define _EGUI_VIEW_VIRTUAL_PAGE_H_

#include "egui_view_virtual_viewport.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef egui_view_virtual_viewport_adapter_t egui_view_virtual_page_adapter_t;
typedef egui_view_virtual_viewport_slot_t egui_view_virtual_page_slot_t;

typedef struct egui_view_virtual_page egui_view_virtual_page_t;
typedef struct egui_view_virtual_page_data_source egui_view_virtual_page_data_source_t;
typedef struct egui_view_virtual_page_entry egui_view_virtual_page_entry_t;
typedef struct egui_view_virtual_page_params egui_view_virtual_page_params_t;
typedef struct egui_view_virtual_page_setup egui_view_virtual_page_setup_t;
/** Predicate used by `egui_view_virtual_page_find_first_visible_section_view`. */
typedef uint8_t (*egui_view_virtual_page_visible_section_matcher_t)(egui_view_t *self, const egui_view_virtual_page_slot_t *slot,
                                                                    const egui_view_virtual_page_entry_t *entry, egui_view_t *section_view, void *context);
/** Visitor used by `egui_view_virtual_page_visit_visible_sections`. Return 0 to stop early. */
typedef uint8_t (*egui_view_virtual_page_visible_section_visitor_t)(egui_view_t *self, const egui_view_virtual_page_slot_t *slot,
                                                                    const egui_view_virtual_page_entry_t *entry, egui_view_t *section_view, void *context);

/** Page-style wrapper around the generic virtual viewport. */
struct egui_view_virtual_page
{
    egui_view_virtual_viewport_t base;
    const egui_view_virtual_page_data_source_t *data_source;
    void *data_source_context;
    egui_view_virtual_page_adapter_t data_source_adapter;
};

/** One-shot parameter block for page geometry and virtualization behavior. */
struct egui_view_virtual_page_params
{
    egui_region_t region;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    int32_t estimated_section_height;
};

/** Resolved metadata for one logical page section. */
struct egui_view_virtual_page_entry
{
    uint32_t index;
    uint32_t stable_id;
};

#define EGUI_VIEW_VIRTUAL_PAGE_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                              \
    static const egui_view_virtual_page_params_t _name = {                                                                                                     \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .overscan_before = 1,                                                                                                                              \
            .overscan_after = 1,                                                                                                                               \
            .max_keepalive_slots = 2,                                                                                                                          \
            .estimated_section_height = 40,                                                                                                                    \
    }

/** One-shot bundle that combines params, data-source hooks, and state-cache limits. */
struct egui_view_virtual_page_setup
{
    const egui_view_virtual_page_params_t *params;
    const egui_view_virtual_page_data_source_t *data_source;
    void *data_source_context;
    uint16_t state_cache_max_entries;
    uint32_t state_cache_max_bytes;
};

#define EGUI_VIEW_VIRTUAL_PAGE_SETUP_INIT(_name, _params, _data_source, _data_source_context)                                                                  \
    static const egui_view_virtual_page_setup_t _name = {                                                                                                      \
            .params = (_params),                                                                                                                               \
            .data_source = (_data_source),                                                                                                                     \
            .data_source_context = (_data_source_context),                                                                                                     \
            .state_cache_max_entries = 0,                                                                                                                      \
            .state_cache_max_bytes = 0,                                                                                                                        \
    }

/** Page-style data-source hooks that are bridged onto the generic virtual viewport adapter. */
struct egui_view_virtual_page_data_source
{
    uint32_t (*get_count)(void *data_source_context);
    uint32_t (*get_stable_id)(void *data_source_context, uint32_t index);
    int32_t (*find_index_by_stable_id)(void *data_source_context, uint32_t stable_id);
    uint16_t (*get_section_type)(void *data_source_context, uint32_t index);
    int32_t (*measure_section_height)(void *data_source_context, uint32_t index, int32_t width_hint);
    egui_view_t *(*create_section_view)(void *data_source_context, uint16_t section_type);
    void (*destroy_section_view)(void *data_source_context, egui_view_t *view, uint16_t section_type);
    void (*bind_section_view)(void *data_source_context, egui_view_t *view, uint32_t index, uint32_t stable_id);
    void (*unbind_section_view)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint8_t (*should_keep_alive)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*save_section_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*restore_section_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint16_t default_section_type;
};

#define EGUI_VIEW_VIRTUAL_PAGE_DATA_SOURCE_INIT(_name)                                                                                                         \
    static const egui_view_virtual_page_data_source_t _name = {                                                                                                \
            .get_count = NULL,                                                                                                                                 \
            .get_stable_id = NULL,                                                                                                                             \
            .find_index_by_stable_id = NULL,                                                                                                                   \
            .get_section_type = NULL,                                                                                                                          \
            .measure_section_height = NULL,                                                                                                                    \
            .create_section_view = NULL,                                                                                                                       \
            .destroy_section_view = NULL,                                                                                                                      \
            .bind_section_view = NULL,                                                                                                                         \
            .unbind_section_view = NULL,                                                                                                                       \
            .should_keep_alive = NULL,                                                                                                                         \
            .save_section_state = NULL,                                                                                                                        \
            .restore_section_state = NULL,                                                                                                                     \
            .default_section_type = 0,                                                                                                                         \
    }

/** Apply a page parameter block after initialization. */
void egui_view_virtual_page_apply_params(egui_view_t *self, const egui_view_virtual_page_params_t *params);
/** Initialize the page and immediately apply its parameter block. */
void egui_view_virtual_page_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_virtual_page_params_t *params);
/** Apply params, data-source hooks, and state-cache limits from one setup bundle. */
void egui_view_virtual_page_apply_setup(egui_view_t *self, const egui_view_virtual_page_setup_t *setup);
/** Initialize the page and immediately apply a setup bundle. */
void egui_view_virtual_page_init_with_setup(egui_view_t *self, egui_core_t *core, const egui_view_virtual_page_setup_t *setup);

/** Attach a raw viewport adapter instead of a page-style data source. This clears any current data source. */
void egui_view_virtual_page_set_adapter(egui_view_t *self, const egui_view_virtual_page_adapter_t *adapter, void *adapter_context);
/** Return the currently attached raw adapter, or NULL. */
const egui_view_virtual_page_adapter_t *egui_view_virtual_page_get_adapter(egui_view_t *self);
/** Return the opaque context pointer passed back to the active raw adapter. */
void *egui_view_virtual_page_get_adapter_context(egui_view_t *self);

/** Attach a page data source. This installs an internal viewport-adapter bridge. Pass NULL to detach it. */
void egui_view_virtual_page_set_data_source(egui_view_t *self, const egui_view_virtual_page_data_source_t *data_source, void *data_source_context);
/** Return the currently attached page data source, or NULL. */
const egui_view_virtual_page_data_source_t *egui_view_virtual_page_get_data_source(egui_view_t *self);
/** Return the opaque context pointer passed to the active page data source. */
void *egui_view_virtual_page_get_data_source_context(egui_view_t *self);
/** Return the current section count reported by the active adapter or data source. */
uint32_t egui_view_virtual_page_get_section_count(egui_view_t *self);

/** Keep additional sections realized before and after the viewport to reduce recycle churn while scrolling. */
void egui_view_virtual_page_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
/** Return the leading overscan section count. */
uint8_t egui_view_virtual_page_get_overscan_before(egui_view_t *self);
/** Return the trailing overscan section count. */
uint8_t egui_view_virtual_page_get_overscan_after(egui_view_t *self);

/** Set the fallback section height used before real measurements are known. */
void egui_view_virtual_page_set_estimated_section_height(egui_view_t *self, int32_t height);
/** Return the fallback section height used by the virtual page. */
int32_t egui_view_virtual_page_get_estimated_section_height(egui_view_t *self);

/** Limit how many off-screen section views may stay alive for quick rebinding. */
void egui_view_virtual_page_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
/** Return the current keepalive-section limit. */
uint8_t egui_view_virtual_page_get_keepalive_limit(egui_view_t *self);
/** Configure the per-section state cache. Both limits must be non-zero or the cache is disabled and cleared. */
void egui_view_virtual_page_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes);
/** Return the maximum number of cached section-state entries. */
uint16_t egui_view_virtual_page_get_state_cache_entry_limit(egui_view_t *self);
/** Return the maximum byte budget for cached section state. */
uint32_t egui_view_virtual_page_get_state_cache_byte_limit(egui_view_t *self);
/** Drop all cached per-section state. */
void egui_view_virtual_page_clear_section_state_cache(egui_view_t *self);
/** Remove cached state for one stable section id. */
void egui_view_virtual_page_remove_section_state_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Store custom state bytes for one stable section id. Passing `size == 0` removes the entry. */
uint8_t egui_view_virtual_page_write_section_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size);
/** Read cached state for one stable section id. Returns the stored size even if `capacity` is smaller. */
uint16_t egui_view_virtual_page_read_section_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity);
/** Convenience helper that stores state for the realized section root view passed by the data source. */
uint8_t egui_view_virtual_page_write_section_state_for_view(egui_view_t *section_view, uint32_t stable_id, const void *data, uint16_t size);
/** Convenience helper that reads state for the realized section root view passed by the data source. */
uint16_t egui_view_virtual_page_read_section_state_for_view(egui_view_t *section_view, uint32_t stable_id, void *data, uint16_t capacity);

/** Jump the vertical scroll offset to an absolute Y position. */
void egui_view_virtual_page_set_scroll_y(egui_view_t *self, int32_t offset);
/** Scroll by a signed Y delta. */
void egui_view_virtual_page_scroll_by(egui_view_t *self, int32_t delta);
/** Scroll so the target section starts near `-section_offset` inside the viewport. */
void egui_view_virtual_page_scroll_to_section(egui_view_t *self, uint32_t index, int32_t section_offset);
/** Scroll to a section by stable id instead of transient index. */
void egui_view_virtual_page_scroll_to_section_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t section_offset);
/** Return the current vertical scroll offset. */
int32_t egui_view_virtual_page_get_scroll_y(egui_view_t *self);
/** Resolve a stable id back to the current section index. */
int32_t egui_view_virtual_page_find_section_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve section metadata for a stable id. */
uint8_t egui_view_virtual_page_resolve_section_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_page_entry_t *entry);
/** Resolve section metadata from the realized section root view returned by the data source. */
uint8_t egui_view_virtual_page_resolve_section_by_view(egui_view_t *self, egui_view_t *section_view, egui_view_virtual_page_entry_t *entry);
/** Return the top Y position of one section. */
int32_t egui_view_virtual_page_get_section_y(egui_view_t *self, uint32_t index);
/** Return the measured or estimated height of one section. */
int32_t egui_view_virtual_page_get_section_height(egui_view_t *self, uint32_t index);
/** Return the top Y position of a section found by stable id. */
int32_t egui_view_virtual_page_get_section_y_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the height of a section found by stable id. */
int32_t egui_view_virtual_page_get_section_height_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Scroll if needed so the target stable id becomes visible within the requested inset tolerance. */
uint8_t egui_view_virtual_page_ensure_section_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset);

/** Tell the virtual page that the whole section set changed and visible sections may need rebuilding. */
void egui_view_virtual_page_notify_data_changed(egui_view_t *self);
/** Tell the virtual page that one section changed in place. */
void egui_view_virtual_page_notify_section_changed(egui_view_t *self, uint32_t index);
/** Notify one in-place section change by stable id. */
void egui_view_virtual_page_notify_section_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Notify that sections were inserted so cached measurements and bindings should be refreshed. */
void egui_view_virtual_page_notify_section_inserted(egui_view_t *self, uint32_t index, uint32_t count);
/** Notify that sections were removed so cached measurements and bindings should be refreshed. */
void egui_view_virtual_page_notify_section_removed(egui_view_t *self, uint32_t index, uint32_t count);
/** Notify that one section moved to a new index. */
void egui_view_virtual_page_notify_section_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
/** Notify that one section's measured height changed. */
void egui_view_virtual_page_notify_section_resized(egui_view_t *self, uint32_t index);
/** Notify that one section's measured height changed, addressed by stable id. */
void egui_view_virtual_page_notify_section_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);

/** Return the number of tracked slot records currently owned by the page. */
uint8_t egui_view_virtual_page_get_slot_count(egui_view_t *self);
/** Access one tracked slot. Check `slot->state` before using it. */
const egui_view_virtual_page_slot_t *egui_view_virtual_page_get_slot(egui_view_t *self, uint8_t slot_index);
/** Return the tracked slot index for an active stable id, or -1 if it is not realized. */
int32_t egui_view_virtual_page_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the active slot for a stable id, or NULL if the section is not currently realized. */
const egui_view_virtual_page_slot_t *egui_view_virtual_page_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the realized root view for a stable id, or NULL if the section is not currently realized. */
egui_view_t *egui_view_virtual_page_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve a tracked slot back to the current section index and stable id. */
uint8_t egui_view_virtual_page_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_page_entry_t *entry);
/** Visit currently visible realized sections. Returns the number of sections passed to the visitor. */
uint8_t egui_view_virtual_page_visit_visible_sections(egui_view_t *self, egui_view_virtual_page_visible_section_visitor_t visitor, void *context);
/** Return the lowest-index visible section that matches the predicate, optionally filling `entry_out`. */
egui_view_t *egui_view_virtual_page_find_first_visible_section_view(egui_view_t *self, egui_view_virtual_page_visible_section_matcher_t matcher, void *context,
                                                                    egui_view_virtual_page_entry_t *entry_out);

/** Initialize the page-style virtual viewport wrapper. */
void egui_view_virtual_page_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_PAGE_H_ */
