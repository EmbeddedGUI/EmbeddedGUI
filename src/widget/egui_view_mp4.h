#ifndef _EGUI_VIEW_MP4_H_
#define _EGUI_VIEW_MP4_H_

#include "widget/egui_view.h"

#include "core/egui_timer.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_mp4 egui_view_mp4_t;

typedef void (*egui_view_mp4_callback_func)(egui_view_mp4_t *, int is_end);

struct egui_view_mp4
{
    egui_view_t base;

    egui_timer_t anim_timer;

    egui_view_mp4_callback_func callback;

    uint8_t align_type;

    const egui_image_t **mp4_image_list;
    uint16_t mp4_image_index;
    uint16_t mp4_image_count;
};

// ============== MP4 Params ==============
typedef struct egui_view_mp4_params egui_view_mp4_params_t;
struct egui_view_mp4_params
{
    egui_region_t region;
};

#define EGUI_VIEW_MP4_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_mp4_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

void egui_view_mp4_apply_params(egui_view_t *self, const egui_view_mp4_params_t *params);
void egui_view_mp4_init_with_params(egui_view_t *self, const egui_view_mp4_params_t *params);

void egui_view_mp4_set_align_type(egui_view_t *self, uint8_t align_type);
void egui_view_mp4_set_callback(egui_view_t *self, egui_view_mp4_callback_func callback);
void egui_view_mp4_set_mp4_image_list(egui_view_t *self, const egui_image_t **mp4_image_list, uint16_t mp4_image_count);
void egui_view_mp4_start_work(egui_view_t *self, int interval_ms);
void egui_view_mp4_stop_work(egui_view_t *self);
void egui_view_mp4_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_MP4_H_ */
