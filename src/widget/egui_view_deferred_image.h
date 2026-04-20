#ifndef _EGUI_VIEW_DEFERRED_IMAGE_H_
#define _EGUI_VIEW_DEFERRED_IMAGE_H_

#include "widget/egui_view.h"
#include "core/egui_timer.h"

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE
#include "image/egui_image_file.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_deferred_image egui_view_deferred_image_t;
typedef struct egui_view_deferred_image_loader egui_view_deferred_image_loader_t;

typedef enum
{
    EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE = 0,
    EGUI_VIEW_DEFERRED_IMAGE_STATUS_DELAY,
    EGUI_VIEW_DEFERRED_IMAGE_STATUS_LOADING,
    EGUI_VIEW_DEFERRED_IMAGE_STATUS_READY,
    EGUI_VIEW_DEFERRED_IMAGE_STATUS_FAILED,
} egui_view_deferred_image_status_t;

typedef enum
{
    EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_PENDING = 0,
    EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_SUCCESS,
    EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_FAILED,
} egui_view_deferred_image_loader_poll_result_t;

struct egui_view_deferred_image_loader
{
    void *user_data;
    int (*start)(void *user_data, const char *source_uri, const char *cache_path, void **request_handle);
    egui_view_deferred_image_loader_poll_result_t (*poll)(void *user_data, void *request_handle);
    /* Must be safe for explicit cancellation and terminal-state cleanup. */
    void (*cancel)(void *user_data, void *request_handle);
};

struct egui_view_deferred_image
{
    egui_view_t base;

    char *source_uri;
    char *cache_path;

    const egui_image_t *placeholder_image;
    const egui_image_t *file_placeholder;
    const egui_image_t *display_image;

    egui_image_file_t loaded_image;
    const egui_view_deferred_image_loader_t *loader;
    void *request_handle;

    egui_timer_t delay_timer;
    egui_timer_t poll_timer;

    uint16_t load_delay_ms;
    uint16_t poll_interval_ms;

    uint8_t status;
    uint8_t auto_start_on_attach;
};

typedef struct egui_view_deferred_image_params egui_view_deferred_image_params_t;
struct egui_view_deferred_image_params
{
    egui_region_t region;
};

#define EGUI_VIEW_DEFERRED_IMAGE_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                            \
    static const egui_view_deferred_image_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

void egui_view_deferred_image_apply_params(egui_view_t *self, const egui_view_deferred_image_params_t *params);
void egui_view_deferred_image_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_deferred_image_params_t *params);

void egui_view_deferred_image_set_source_uri(egui_view_t *self, const char *source_uri);
void egui_view_deferred_image_set_cache_path(egui_view_t *self, const char *cache_path);
void egui_view_deferred_image_set_placeholder_image(egui_view_t *self, const egui_image_t *image);
void egui_view_deferred_image_set_file_placeholder(egui_view_t *self, const egui_image_t *image);
void egui_view_deferred_image_set_loader(egui_view_t *self, const egui_view_deferred_image_loader_t *loader);
void egui_view_deferred_image_set_auto_start_on_attach(egui_view_t *self, int auto_start_on_attach);
void egui_view_deferred_image_set_load_delay_ms(egui_view_t *self, uint16_t delay_ms);
void egui_view_deferred_image_reload(egui_view_t *self);
void egui_view_deferred_image_cancel(egui_view_t *self);
egui_view_deferred_image_status_t egui_view_deferred_image_get_status(const egui_view_t *self);
void egui_view_deferred_image_deinit(egui_view_t *self);
void egui_view_deferred_image_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* EGUI_CONFIG_FUNCTION_IMAGE_FILE */

#endif /* _EGUI_VIEW_DEFERRED_IMAGE_H_ */
