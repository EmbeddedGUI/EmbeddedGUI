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

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.

EGUI_VIEW_API_DEFINE(egui_view_test_mask_t, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, egui_view_test_mask_on_draw, NULL);

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
