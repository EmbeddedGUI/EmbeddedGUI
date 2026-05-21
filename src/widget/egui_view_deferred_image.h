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
    /** No request is scheduled or running. */
    EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE = 0,
    /** Waiting for `load_delay_ms` before starting the loader. */
    EGUI_VIEW_DEFERRED_IMAGE_STATUS_DELAY,
    /** The loader request has started and is being polled. */
    EGUI_VIEW_DEFERRED_IMAGE_STATUS_LOADING,
    /** The requested image finished loading and is now displayed. */
    EGUI_VIEW_DEFERRED_IMAGE_STATUS_READY,
    /** The last load attempt failed. */
    EGUI_VIEW_DEFERRED_IMAGE_STATUS_FAILED,
} egui_view_deferred_image_status_t;

typedef enum
{
    /** Keep polling; the request has not reached a terminal state yet. */
    EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_PENDING = 0,
    /** The request finished successfully. */
    EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_SUCCESS,
    /** The request finished with an error. */
    EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_FAILED,
} egui_view_deferred_image_loader_poll_result_t;

struct egui_view_deferred_image_loader
{
    void *user_data;
    /** Start loading `source_uri` and optionally write to `cache_path`. Return 0 on success and fill `request_handle`. */
    int (*start)(void *user_data, const char *source_uri, const char *cache_path, void **request_handle);
    /** Poll the outstanding request until it reports success or failure. */
    egui_view_deferred_image_loader_poll_result_t (*poll)(void *user_data, void *request_handle);
    /** Cancel or finalize one request handle. This must be safe for both explicit cancel and terminal-state cleanup. */
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

/** Apply the region from one parameter block. */
void egui_view_deferred_image_apply_params(egui_view_t *self, const egui_view_deferred_image_params_t *params);
/** Initialize a deferred-image view and immediately apply its parameter block. */
void egui_view_deferred_image_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_deferred_image_params_t *params);

/** Copy a new source URI, reset back to the placeholder, and return to `IDLE`. Auto-start may schedule a new load. */
void egui_view_deferred_image_set_source_uri(egui_view_t *self, const char *source_uri);
/** Return the copied source URI string, or NULL when unset or self is NULL. */
const char *egui_view_deferred_image_get_source_uri(const egui_view_t *self);
/** Copy a new cache path, reset back to the placeholder, and return to `IDLE`. */
void egui_view_deferred_image_set_cache_path(egui_view_t *self, const char *cache_path);
/** Return the copied cache path string, or NULL when unset or self is NULL. */
const char *egui_view_deferred_image_get_cache_path(const egui_view_t *self);
/** Set the in-memory placeholder image shown while idle, delayed, loading, or failed before a real image is ready. */
void egui_view_deferred_image_set_placeholder_image(egui_view_t *self, const egui_image_t *image);
/** Return the in-memory placeholder image pointer, or NULL when unset or self is NULL. */
const egui_image_t *egui_view_deferred_image_get_placeholder_image(const egui_view_t *self);
/** Set the placeholder forwarded to the internal file-image loader for file decode fallback states. */
void egui_view_deferred_image_set_file_placeholder(egui_view_t *self, const egui_image_t *image);
/** Return the file-image placeholder pointer, or NULL when unset or self is NULL. */
const egui_image_t *egui_view_deferred_image_get_file_placeholder(const egui_view_t *self);
/** Borrow a loader vtable, reset the current state to the placeholder, and optionally auto-start a new request. */
void egui_view_deferred_image_set_loader(egui_view_t *self, const egui_view_deferred_image_loader_t *loader);
/** Return the borrowed loader vtable pointer, or NULL when unset or self is NULL. */
const egui_view_deferred_image_loader_t *egui_view_deferred_image_get_loader(const egui_view_t *self);
/** Enable or disable automatic loading when the view is attached. Enabling it may start loading immediately from `IDLE`. */
void egui_view_deferred_image_set_auto_start_on_attach(egui_view_t *self, int auto_start_on_attach);
/** Return whether loading is automatically scheduled on attach. Returns 0 when self is NULL. */
uint8_t egui_view_deferred_image_get_auto_start_on_attach(const egui_view_t *self);
/** Set the delay before the loader starts after scheduling. */
void egui_view_deferred_image_set_load_delay_ms(egui_view_t *self, uint16_t delay_ms);
/** Return the delay before the loader starts after scheduling. Returns 0 when self is NULL. */
uint16_t egui_view_deferred_image_get_load_delay_ms(const egui_view_t *self);
/** Return the image currently displayed by draw(), or NULL when the built-in placeholder is used or self is NULL. */
const egui_image_t *egui_view_deferred_image_get_display_image(const egui_view_t *self);
/** Reset to the placeholder, return to `IDLE`, and then schedule a fresh load attempt. */
void egui_view_deferred_image_reload(egui_view_t *self);
/** Cancel the active request, stop timers, and return to the placeholder `IDLE` state. */
void egui_view_deferred_image_cancel(egui_view_t *self);
/** Return the current deferred-load status. */
egui_view_deferred_image_status_t egui_view_deferred_image_get_status(const egui_view_t *self);
/** Release timers, request handles, copied strings, and the internal file-image object. */
void egui_view_deferred_image_deinit(egui_view_t *self);
/** Initialize the deferred-image view with auto-start enabled and default 50 ms delay/poll intervals. */
void egui_view_deferred_image_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* EGUI_CONFIG_FUNCTION_IMAGE_FILE */

#endif /* _EGUI_VIEW_DEFERRED_IMAGE_H_ */
