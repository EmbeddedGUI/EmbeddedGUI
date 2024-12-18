#include <stdio.h>
#include <assert.h>

#include "egui_view_test_mask.h"
#include "egui.h"

void egui_view_test_mask_set_mask(egui_view_t *self, egui_mask_t *mask)
{
    egui_view_test_mask_t *local = (egui_view_test_mask_t *)self;

    local->mask = mask; // set mask.
}

void egui_view_test_mask_on_draw(egui_view_t *self)
{
    egui_view_test_mask_t *local = (egui_view_test_mask_t *)self;

    egui_canvas_set_mask(local->mask);
    // call super draw.
    egui_view_image_on_draw(self);
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
}
