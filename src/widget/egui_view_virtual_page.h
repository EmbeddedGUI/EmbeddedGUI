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
typedef struct egui_view_virtual_page_params egui_view_virtual_page_params_t;

struct egui_view_virtual_page
{
    egui_view_virtual_viewport_t base;
    const egui_view_virtual_page_data_source_t *data_source;
    void *data_source_context;
    egui_view_virtual_page_adapter_t data_source_adapter;
};

struct egui_view_virtual_page_params
{
    egui_region_t region;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    int32_t estimated_section_height;
};

#define EGUI_VIEW_VIRTUAL_PAGE_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                              \
    static const egui_view_virtual_page_params_t _name = {                                                                                                     \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .overscan_before = 1,                                                                                                                              \
            .overscan_after = 1,                                                                                                                               \
            .max_keepalive_slots = 2,                                                                                                                          \
            .estimated_section_height = 40,                                                                                                                    \
    }

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

void egui_view_virtual_page_apply_params(egui_view_t *self, const egui_view_virtual_page_params_t *params);
void egui_view_virtual_page_init_with_params(egui_view_t *self, const egui_view_virtual_page_params_t *params);

void egui_view_virtual_page_set_adapter(egui_view_t *self, const egui_view_virtual_page_adapter_t *adapter, void *adapter_context);
const egui_view_virtual_page_adapter_t *egui_view_virtual_page_get_adapter(egui_view_t *self);
void *egui_view_virtual_page_get_adapter_context(egui_view_t *self);

void egui_view_virtual_page_set_data_source(egui_view_t *self, const egui_view_virtual_page_data_source_t *data_source, void *data_source_context);
const egui_view_virtual_page_data_source_t *egui_view_virtual_page_get_data_source(egui_view_t *self);
void *egui_view_virtual_page_get_data_source_context(egui_view_t *self);

void egui_view_virtual_page_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
uint8_t egui_view_virtual_page_get_overscan_before(egui_view_t *self);
uint8_t egui_view_virtual_page_get_overscan_after(egui_view_t *self);

void egui_view_virtual_page_set_estimated_section_height(egui_view_t *self, int32_t height);
int32_t egui_view_virtual_page_get_estimated_section_height(egui_view_t *self);

void egui_view_virtual_page_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
uint8_t egui_view_virtual_page_get_keepalive_limit(egui_view_t *self);

void egui_view_virtual_page_set_scroll_y(egui_view_t *self, int32_t offset);
void egui_view_virtual_page_scroll_by(egui_view_t *self, int32_t delta);
void egui_view_virtual_page_scroll_to_section(egui_view_t *self, uint32_t index, int32_t section_offset);
int32_t egui_view_virtual_page_get_scroll_y(egui_view_t *self);
int32_t egui_view_virtual_page_get_section_y(egui_view_t *self, uint32_t index);
int32_t egui_view_virtual_page_get_section_height(egui_view_t *self, uint32_t index);

void egui_view_virtual_page_notify_data_changed(egui_view_t *self);
void egui_view_virtual_page_notify_section_changed(egui_view_t *self, uint32_t index);
void egui_view_virtual_page_notify_section_inserted(egui_view_t *self, uint32_t index, uint32_t count);
void egui_view_virtual_page_notify_section_removed(egui_view_t *self, uint32_t index, uint32_t count);
void egui_view_virtual_page_notify_section_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
void egui_view_virtual_page_notify_section_resized(egui_view_t *self, uint32_t index);

uint8_t egui_view_virtual_page_get_slot_count(egui_view_t *self);
const egui_view_virtual_page_slot_t *egui_view_virtual_page_get_slot(egui_view_t *self, uint8_t slot_index);

void egui_view_virtual_page_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_PAGE_H_ */
