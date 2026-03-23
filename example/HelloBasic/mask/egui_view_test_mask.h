#ifndef _EGUI_VIEW_TEST_MASK_H_
#define _EGUI_VIEW_TEST_MASK_H_

#include "widget/egui_view_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_test_mask egui_view_test_mask_t;
typedef enum
{
    EGUI_VIEW_TEST_MASK_DRAW_IMAGE = 0,
    EGUI_VIEW_TEST_MASK_DRAW_IMAGE_TRANSFORM,
    EGUI_VIEW_TEST_MASK_DRAW_TEXT_TRANSFORM,
} egui_view_test_mask_draw_mode_t;

struct egui_view_test_mask
{
    egui_view_image_t base;

    egui_mask_t *mask;
    const egui_font_t *font;
    const char *text;
    egui_color_t draw_color;
    egui_color_t background_color;
    egui_alpha_t draw_alpha;
    egui_alpha_t background_alpha;
    int16_t angle_deg;
    int16_t scale_q8;
    uint8_t draw_mode;
    uint8_t use_buffered_transform;
};

void egui_view_test_mask_set_mask(egui_view_t *self, egui_mask_t *mask);
void egui_view_test_mask_set_image_transform(egui_view_t *self, int16_t angle_deg, int16_t scale_q8, egui_color_t background_color,
                                             egui_alpha_t background_alpha);
void egui_view_test_mask_set_text_transform(egui_view_t *self, const egui_font_t *font, const char *text, int is_buffered, int16_t angle_deg, int16_t scale_q8,
                                            egui_color_t color, egui_alpha_t alpha, egui_color_t background_color, egui_alpha_t background_alpha);
void egui_view_test_mask_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TEST_MASK_H_ */
