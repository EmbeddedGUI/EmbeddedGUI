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

    const egui_image_t** mp4_image_list;
    uint16_t mp4_image_index;
    uint16_t mp4_image_count;
};

void egui_view_mp4_start_work(egui_view_t *self, int interval_ms);
void egui_view_mp4_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_MP4_H_ */
