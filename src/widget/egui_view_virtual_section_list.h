#ifndef _EGUI_VIEW_VIRTUAL_SECTION_LIST_H_
#define _EGUI_VIEW_VIRTUAL_SECTION_LIST_H_

#include "egui_view_virtual_list.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX 0xFFFFFFFFUL

typedef egui_view_virtual_viewport_slot_t egui_view_virtual_section_list_slot_t;

typedef struct egui_view_virtual_section_list egui_view_virtual_section_list_t;
typedef struct egui_view_virtual_section_list_data_source egui_view_virtual_section_list_data_source_t;
typedef struct egui_view_virtual_section_list_entry egui_view_virtual_section_list_entry_t;
typedef struct egui_view_virtual_section_list_params egui_view_virtual_section_list_params_t;
typedef struct egui_view_virtual_section_list_setup egui_view_virtual_section_list_setup_t;
/** Predicate used by `egui_view_virtual_section_list_find_first_visible_entry_view`. */
typedef uint8_t (*egui_view_virtual_section_list_visible_entry_matcher_t)(egui_view_t *self, const egui_view_virtual_section_list_slot_t *slot,
                                                                          const egui_view_virtual_section_list_entry_t *entry, egui_view_t *entry_view,
                                                                          void *context);
/** Visitor used by `egui_view_virtual_section_list_visit_visible_entries`. Return 0 to stop early. */
typedef uint8_t (*egui_view_virtual_section_list_visible_entry_visitor_t)(egui_view_t *self, const egui_view_virtual_section_list_slot_t *slot,
                                                                          const egui_view_virtual_section_list_entry_t *entry, egui_view_t *entry_view,
                                                                          void *context);

/** Sectioned virtual list that flattens headers and items onto one `egui_view_virtual_list_t`. */
struct egui_view_virtual_section_list
{
    egui_view_virtual_list_t base;
    const egui_view_virtual_section_list_data_source_t *data_source;
    void *data_source_context;
    egui_view_virtual_list_data_source_t flat_data_source;
};

/** One-shot parameter block for section-list geometry and virtualization behavior. */
struct egui_view_virtual_section_list_params
{
    egui_region_t region;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    int32_t estimated_entry_height;
};

/** Resolved metadata for one flattened entry. Headers use `item_index == INVALID_INDEX`. */
struct egui_view_virtual_section_list_entry
{
    uint8_t is_section_header;
    uint32_t section_index;
    uint32_t item_index;
    uint32_t stable_id;
};

#define EGUI_VIEW_VIRTUAL_SECTION_LIST_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                      \
    static const egui_view_virtual_section_list_params_t _name = {                                                                                             \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .overscan_before = 1,                                                                                                                              \
            .overscan_after = 1,                                                                                                                               \
            .max_keepalive_slots = 2,                                                                                                                          \
            .estimated_entry_height = 40,                                                                                                                      \
    }

/** One-shot bundle that combines params, section hooks, and entry-state cache limits. */
struct egui_view_virtual_section_list_setup
{
    const egui_view_virtual_section_list_params_t *params;
    const egui_view_virtual_section_list_data_source_t *data_source;
    void *data_source_context;
    uint16_t state_cache_max_entries;
    uint32_t state_cache_max_bytes;
};

#define EGUI_VIEW_VIRTUAL_SECTION_LIST_SETUP_INIT(_name, _params, _data_source, _data_source_context)                                                          \
    static const egui_view_virtual_section_list_setup_t _name = {                                                                                              \
            .params = (_params),                                                                                                                               \
            .data_source = (_data_source),                                                                                                                     \
            .data_source_context = (_data_source_context),                                                                                                     \
            .state_cache_max_entries = 0,                                                                                                                      \
            .state_cache_max_bytes = 0,                                                                                                                        \
    }

/** Section-list data-source hooks. The widget flattens section headers and items into one virtual list internally. */
struct egui_view_virtual_section_list_data_source
{
    uint32_t (*get_section_count)(void *data_source_context);
    uint32_t (*get_section_stable_id)(void *data_source_context, uint32_t section_index);
    int32_t (*find_section_index_by_stable_id)(void *data_source_context, uint32_t stable_id);
    uint32_t (*get_item_count)(void *data_source_context, uint32_t section_index);
    uint32_t (*get_item_stable_id)(void *data_source_context, uint32_t section_index, uint32_t item_index);
    uint8_t (*find_item_position_by_stable_id)(void *data_source_context, uint32_t stable_id, uint32_t *section_index, uint32_t *item_index);
    uint16_t (*get_section_header_view_type)(void *data_source_context, uint32_t section_index);
    uint16_t (*get_item_view_type)(void *data_source_context, uint32_t section_index, uint32_t item_index);
    int32_t (*measure_section_header_height)(void *data_source_context, uint32_t section_index, int32_t width_hint);
    int32_t (*measure_item_height)(void *data_source_context, uint32_t section_index, uint32_t item_index, int32_t width_hint);
    egui_view_t *(*create_section_header_view)(void *data_source_context, uint16_t view_type);
    egui_view_t *(*create_item_view)(void *data_source_context, uint16_t view_type);
    void (*destroy_section_header_view)(void *data_source_context, egui_view_t *view, uint16_t view_type);
    void (*destroy_item_view)(void *data_source_context, egui_view_t *view, uint16_t view_type);
    void (*bind_section_header_view)(void *data_source_context, egui_view_t *view, uint32_t section_index, uint32_t stable_id);
    void (*bind_item_view)(void *data_source_context, egui_view_t *view, uint32_t section_index, uint32_t item_index, uint32_t stable_id);
    void (*unbind_section_header_view)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*unbind_item_view)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint8_t (*should_keep_section_header_alive)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint8_t (*should_keep_item_alive)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*save_section_header_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*save_item_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*restore_section_header_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*restore_item_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint16_t default_section_header_view_type;
    uint16_t default_item_view_type;
};

#define EGUI_VIEW_VIRTUAL_SECTION_LIST_DATA_SOURCE_INIT(_name)                                                                                                 \
    static const egui_view_virtual_section_list_data_source_t _name = {                                                                                        \
            .get_section_count = NULL,                                                                                                                         \
            .get_section_stable_id = NULL,                                                                                                                     \
            .find_section_index_by_stable_id = NULL,                                                                                                           \
            .get_item_count = NULL,                                                                                                                            \
            .get_item_stable_id = NULL,                                                                                                                        \
            .find_item_position_by_stable_id = NULL,                                                                                                           \
            .get_section_header_view_type = NULL,                                                                                                              \
            .get_item_view_type = NULL,                                                                                                                        \
            .measure_section_header_height = NULL,                                                                                                             \
            .measure_item_height = NULL,                                                                                                                       \
            .create_section_header_view = NULL,                                                                                                                \
            .create_item_view = NULL,                                                                                                                          \
            .destroy_section_header_view = NULL,                                                                                                               \
            .destroy_item_view = NULL,                                                                                                                         \
            .bind_section_header_view = NULL,                                                                                                                  \
            .bind_item_view = NULL,                                                                                                                            \
            .unbind_section_header_view = NULL,                                                                                                                \
            .unbind_item_view = NULL,                                                                                                                          \
            .should_keep_section_header_alive = NULL,                                                                                                          \
            .should_keep_item_alive = NULL,                                                                                                                    \
            .save_section_header_state = NULL,                                                                                                                 \
            .save_item_state = NULL,                                                                                                                           \
            .restore_section_header_state = NULL,                                                                                                              \
            .restore_item_state = NULL,                                                                                                                        \
            .default_section_header_view_type = 0,                                                                                                             \
            .default_item_view_type = 0,                                                                                                                       \
    }

/** Apply a section-list parameter block after initialization. */
void egui_view_virtual_section_list_apply_params(egui_view_t *self, const egui_view_virtual_section_list_params_t *params);
/** Initialize the section list and immediately apply its parameter block. */
void egui_view_virtual_section_list_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_virtual_section_list_params_t *params);
/** Apply params, data-source hooks, and cache limits from one setup bundle. */
void egui_view_virtual_section_list_apply_setup(egui_view_t *self, const egui_view_virtual_section_list_setup_t *setup);
/** Initialize the section list and immediately apply a setup bundle. */
void egui_view_virtual_section_list_init_with_setup(egui_view_t *self, egui_core_t *core, const egui_view_virtual_section_list_setup_t *setup);

/** Attach a section-list data source. This rebuilds the internal flat-list bridge. Pass NULL to detach it. */
void egui_view_virtual_section_list_set_data_source(egui_view_t *self, const egui_view_virtual_section_list_data_source_t *data_source,
                                                    void *data_source_context);
/** Return the currently attached section-list data source, or NULL. */
const egui_view_virtual_section_list_data_source_t *egui_view_virtual_section_list_get_data_source(egui_view_t *self);
/** Return the opaque context pointer passed to the active data source. */
void *egui_view_virtual_section_list_get_data_source_context(egui_view_t *self);
/** Return the current number of logical sections. */
uint32_t egui_view_virtual_section_list_get_section_count(egui_view_t *self);
/** Return the current item count inside one logical section. */
uint32_t egui_view_virtual_section_list_get_item_count(egui_view_t *self, uint32_t section_index);

/** Keep additional flattened entries realized before and after the viewport. */
void egui_view_virtual_section_list_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
/** Return the leading overscan entry count. */
uint8_t egui_view_virtual_section_list_get_overscan_before(egui_view_t *self);
/** Return the trailing overscan entry count. */
uint8_t egui_view_virtual_section_list_get_overscan_after(egui_view_t *self);

/** Set the fallback height used for both section headers and items before real measurements are known. */
void egui_view_virtual_section_list_set_estimated_entry_height(egui_view_t *self, int32_t height);
/** Return the fallback entry height used by the section list. */
int32_t egui_view_virtual_section_list_get_estimated_entry_height(egui_view_t *self);

/** Limit how many off-screen entry views may stay alive for quick rebinding. */
void egui_view_virtual_section_list_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
/** Return the current keepalive-entry limit. */
uint8_t egui_view_virtual_section_list_get_keepalive_limit(egui_view_t *self);

/** Configure the shared per-entry state cache. Both limits must be non-zero or the cache is disabled and cleared. */
void egui_view_virtual_section_list_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes);
/** Return the maximum number of cached entry-state records. */
uint16_t egui_view_virtual_section_list_get_state_cache_entry_limit(egui_view_t *self);
/** Return the maximum byte budget for cached entry state. */
uint32_t egui_view_virtual_section_list_get_state_cache_byte_limit(egui_view_t *self);
/** Drop all cached header and item state. */
void egui_view_virtual_section_list_clear_entry_state_cache(egui_view_t *self);
/** Remove cached state for one entry stable id. */
void egui_view_virtual_section_list_remove_entry_state_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Store custom state bytes for one entry stable id. Passing `size == 0` removes the entry. */
uint8_t egui_view_virtual_section_list_write_entry_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size);
/** Read cached state for one entry stable id. Returns the stored size even if `capacity` is smaller. */
uint16_t egui_view_virtual_section_list_read_entry_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity);
/** Convenience helper that stores state for the realized header/item root view returned by the data source. */
uint8_t egui_view_virtual_section_list_write_entry_state_for_view(egui_view_t *entry_view, uint32_t stable_id, const void *data, uint16_t size);
/** Convenience helper that reads state for the realized header/item root view returned by the data source. */
uint16_t egui_view_virtual_section_list_read_entry_state_for_view(egui_view_t *entry_view, uint32_t stable_id, void *data, uint16_t capacity);

/** Jump the vertical scroll offset to an absolute Y position. */
void egui_view_virtual_section_list_set_scroll_y(egui_view_t *self, int32_t offset);
/** Scroll by a signed Y delta. */
void egui_view_virtual_section_list_scroll_by(egui_view_t *self, int32_t delta);
/** Scroll so the target section header starts near `-section_offset` inside the viewport. */
void egui_view_virtual_section_list_scroll_to_section(egui_view_t *self, uint32_t section_index, int32_t section_offset);
/** Scroll to a section header by stable id instead of transient index. */
void egui_view_virtual_section_list_scroll_to_section_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t section_offset);
/** Scroll so one item starts near `-item_offset` inside the viewport. */
void egui_view_virtual_section_list_scroll_to_item(egui_view_t *self, uint32_t section_index, uint32_t item_index, int32_t item_offset);
/** Scroll to one item by stable id instead of `(section_index, item_index)`. */
void egui_view_virtual_section_list_scroll_to_item_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset);
/** Return the current vertical scroll offset. */
int32_t egui_view_virtual_section_list_get_scroll_y(egui_view_t *self);

/** Resolve a section-header stable id back to the current section index. */
int32_t egui_view_virtual_section_list_find_section_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve an item stable id back to its current section and item indexes. */
uint8_t egui_view_virtual_section_list_find_item_position_by_stable_id(egui_view_t *self, uint32_t stable_id, uint32_t *section_index, uint32_t *item_index);
/** Resolve flattened entry metadata for a stable id. */
uint8_t egui_view_virtual_section_list_resolve_entry_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_section_list_entry_t *entry);
/** Resolve flattened entry metadata from the realized entry root view. Descendants are not matched. */
uint8_t egui_view_virtual_section_list_resolve_entry_by_view(egui_view_t *self, egui_view_t *entry_view, egui_view_virtual_section_list_entry_t *entry);

/** Return the top Y position of one section header. */
int32_t egui_view_virtual_section_list_get_section_header_y(egui_view_t *self, uint32_t section_index);
/** Return the measured or estimated height of one section header. */
int32_t egui_view_virtual_section_list_get_section_header_height(egui_view_t *self, uint32_t section_index);
/** Return the top Y position of one item inside a section. */
int32_t egui_view_virtual_section_list_get_item_y(egui_view_t *self, uint32_t section_index, uint32_t item_index);
/** Return the measured or estimated height of one item inside a section. */
int32_t egui_view_virtual_section_list_get_item_height(egui_view_t *self, uint32_t section_index, uint32_t item_index);
/** Return the top Y position of an entry found by stable id. */
int32_t egui_view_virtual_section_list_get_entry_y_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the height of an entry found by stable id. */
int32_t egui_view_virtual_section_list_get_entry_height_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Scroll if needed so the target entry becomes visible within the requested inset tolerance. */
uint8_t egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset);

/** Tell the section list that the whole flattened data set changed and visible entries may need rebuilding. */
void egui_view_virtual_section_list_notify_data_changed(egui_view_t *self);
/** Tell the section list that one section header changed in place. */
void egui_view_virtual_section_list_notify_section_header_changed(egui_view_t *self, uint32_t section_index);
/** Notify one in-place section-header change by stable id. */
void egui_view_virtual_section_list_notify_section_header_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Notify that one section header changed height. */
void egui_view_virtual_section_list_notify_section_header_resized(egui_view_t *self, uint32_t section_index);
/** Notify one section-header height change by stable id. */
void egui_view_virtual_section_list_notify_section_header_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Tell the section list that one item changed in place. */
void egui_view_virtual_section_list_notify_item_changed(egui_view_t *self, uint32_t section_index, uint32_t item_index);
/** Notify one in-place item change by stable id. */
void egui_view_virtual_section_list_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Notify that one item changed height. */
void egui_view_virtual_section_list_notify_item_resized(egui_view_t *self, uint32_t section_index, uint32_t item_index);
/** Notify one item height change by stable id. */
void egui_view_virtual_section_list_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Notify that sections were inserted. This currently falls back to a full data refresh. */
void egui_view_virtual_section_list_notify_section_inserted(egui_view_t *self, uint32_t section_index, uint32_t count);
/** Notify that sections were removed. This currently falls back to a full data refresh. */
void egui_view_virtual_section_list_notify_section_removed(egui_view_t *self, uint32_t section_index, uint32_t count);
/** Notify that one section moved. This currently falls back to a full data refresh. */
void egui_view_virtual_section_list_notify_section_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
/** Notify that items were inserted. This currently falls back to a full data refresh. */
void egui_view_virtual_section_list_notify_item_inserted(egui_view_t *self, uint32_t section_index, uint32_t item_index, uint32_t count);
/** Notify that items were removed. This currently falls back to a full data refresh. */
void egui_view_virtual_section_list_notify_item_removed(egui_view_t *self, uint32_t section_index, uint32_t item_index, uint32_t count);
/** Notify that one item moved. This currently falls back to a full data refresh. */
void egui_view_virtual_section_list_notify_item_moved(egui_view_t *self, uint32_t from_section_index, uint32_t from_item_index, uint32_t to_section_index,
                                                      uint32_t to_item_index);

/** Return the number of tracked slot records currently owned by the flattened list. */
uint8_t egui_view_virtual_section_list_get_slot_count(egui_view_t *self);
/** Access one tracked slot. Check `slot->state` before using it. */
const egui_view_virtual_section_list_slot_t *egui_view_virtual_section_list_get_slot(egui_view_t *self, uint8_t slot_index);
/** Return the tracked slot index for a realized stable id, or -1 if it is not currently in a slot. */
int32_t egui_view_virtual_section_list_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the tracked slot for a realized stable id, or NULL. */
const egui_view_virtual_section_list_slot_t *egui_view_virtual_section_list_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the realized entry root view for a stable id, or NULL if the entry is not currently realized. */
egui_view_t *egui_view_virtual_section_list_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve a tracked slot back to its current header/item metadata. */
uint8_t egui_view_virtual_section_list_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_section_list_entry_t *entry);
/** Visit currently visible realized headers and items. Returns the number of entries passed to the visitor. */
uint8_t egui_view_virtual_section_list_visit_visible_entries(egui_view_t *self, egui_view_virtual_section_list_visible_entry_visitor_t visitor, void *context);
/** Return the lowest flat-index visible entry that matches the predicate, optionally filling `entry_out`. */
egui_view_t *egui_view_virtual_section_list_find_first_visible_entry_view(egui_view_t *self, egui_view_virtual_section_list_visible_entry_matcher_t matcher,
                                                                          void *context, egui_view_virtual_section_list_entry_t *entry_out);

/** Initialize the section-list wrapper around the generic virtual list. */
void egui_view_virtual_section_list_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_SECTION_LIST_H_ */
