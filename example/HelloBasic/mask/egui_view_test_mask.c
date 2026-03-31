#include <stdio.h>
#include <assert.h>

#include "egui_view_test_mask.h"
#include "egui.h"

void egui_view_test_mask_set_mask(egui_view_t *self, egui_mask_t *mask)
{
    egui_view_test_mask_t *local = (egui_view_test_mask_t *)self;

    local->mask = mask; // set mask.
}

void egui_view_test_mask_set_image_transform(egui_view_t *self, int16_t angle_deg, int16_t scale_q8, egui_color_t background_color,
                                             egui_alpha_t background_alpha)
{
    egui_view_test_mask_t *local = (egui_view_test_mask_t *)self;

    local->draw_mode = EGUI_VIEW_TEST_MASK_DRAW_IMAGE_TRANSFORM;
    local->angle_deg = angle_deg;
    local->scale_q8 = scale_q8;
    local->background_color = background_color;
    local->background_alpha = background_alpha;
    local->font = NULL;
    local->text = NULL;
}

void egui_view_test_mask_set_text_transform(egui_view_t *self, const egui_font_t *font, const char *text, int16_t angle_deg, int16_t scale_q8,
                                            egui_color_t color, egui_alpha_t alpha, egui_color_t background_color, egui_alpha_t background_alpha)
{
    egui_view_test_mask_t *local = (egui_view_test_mask_t *)self;

    local->draw_mode = EGUI_VIEW_TEST_MASK_DRAW_TEXT_TRANSFORM;
    local->font = font;
    local->text = text;
    local->angle_deg = angle_deg;
    local->scale_q8 = scale_q8;
    local->draw_color = color;
    local->draw_alpha = alpha;
    local->background_color = background_color;
    local->background_alpha = background_alpha;
}

void egui_view_test_mask_on_draw(egui_view_t *self)
{
    egui_view_test_mask_t *local = (egui_view_test_mask_t *)self;
    egui_region_t region;
    egui_dim_t cx;
    egui_dim_t cy;

    egui_view_get_work_region(self, &region);
    egui_canvas_set_mask(local->mask);

    if (local->background_alpha > 0)
    {
        egui_canvas_draw_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, local->background_color,
                                        local->background_alpha);
    }

    cx = region.location.x + (region.size.width >> 1);
    cy = region.location.y + (region.size.height >> 1);

    switch (local->draw_mode)
    {
    case EGUI_VIEW_TEST_MASK_DRAW_IMAGE_TRANSFORM:
        if (local->base.image != NULL)
        {
            egui_canvas_draw_image_transform(local->base.image, cx, cy, local->angle_deg, local->scale_q8);
        }
        break;

    case EGUI_VIEW_TEST_MASK_DRAW_TEXT_TRANSFORM:
        if (local->font != NULL && local->text != NULL)
        {
            egui_canvas_draw_text_transform(local->font, local->text, cx, cy, local->angle_deg, local->scale_q8, local->draw_color, local->draw_alpha);
        }
        break;

    case EGUI_VIEW_TEST_MASK_DRAW_IMAGE:
    default:
        egui_view_image_on_draw(self);
        break;
    }

    // clear mask for canvas.
    egui_canvas_clear_mask();
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_test_mask_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_test_mask_on_draw, // changed
        .on_detach_from_window = egui_view_on_detach_from_window,
};

void egui_view_test_mask_init(egui_view_t *self)
{
    egui_view_test_mask_t *local = (egui_view_test_mask_t *)self;
    // call super init.
    egui_view_image_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_test_mask_t);

    // init local data.
    local->mask = NULL;
    local->font = NULL;
    local->text = NULL;
    local->draw_color = EGUI_COLOR_WHITE;
    local->background_color = EGUI_COLOR_BLACK;
    local->draw_alpha = EGUI_ALPHA_100;
    local->background_alpha = 0;
    local->angle_deg = 45;
    local->scale_q8 = 256;
    local->draw_mode = EGUI_VIEW_TEST_MASK_DRAW_IMAGE;
}
