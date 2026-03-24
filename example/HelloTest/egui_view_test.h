#ifndef _EGUI_VIEW_TEST_H_
#define _EGUI_VIEW_TEST_H_

#include "core/egui_timer.h"
#include "mask/egui_mask_circle.h"
#include "widget/egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_test_block egui_view_test_block_t;
struct egui_view_test_block
{
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t size;
    int8_t vx;
    int8_t vy;
    egui_color_t fill_color;
    egui_alpha_t alpha;
};

typedef struct egui_view_test egui_view_test_t;
struct egui_view_test
{
    egui_view_t base;

    egui_timer_t anim_timer;
    egui_mask_circle_t photo_mask;
    egui_dim_t photo_w;
    egui_dim_t photo_h;
    egui_dim_t text_cn_w;
    egui_dim_t text_cn_h;
    egui_dim_t text_en_w;
    egui_dim_t text_en_h;
    uint8_t metrics_ready;
    uint16_t arc_sweep;
    int8_t arc_direction;
    uint16_t popup_phase;
    int16_t popup_angle;
    egui_view_test_block_t blocks[2];
};

void egui_view_test_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TEST_H_ */
