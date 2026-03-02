#include <stdio.h>
#include <assert.h>

#include "egui_view_linearlayout.h"
#include "font/egui_font.h"

void egui_view_linearlayout_set_align_type(egui_view_t *self, uint8_t align_type)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    local->align_type = align_type;
}

void egui_view_linearlayout_set_auto_width(egui_view_t *self, uint8_t is_auto_width)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    local->is_auto_width = is_auto_width;
}

void egui_view_linearlayout_set_auto_height(egui_view_t *self, uint8_t is_auto_height)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    local->is_auto_height = is_auto_height;
}

void egui_view_linearlayout_set_orientation(egui_view_t *self, uint8_t is_horizontal)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    local->is_orientation_horizontal = is_horizontal;
}

uint8_t egui_view_linearlayout_is_align_type(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->align_type;
}

uint8_t egui_view_linearlayout_is_auto_width(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->is_auto_width;
}

uint8_t egui_view_linearlayout_is_auto_height(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->is_auto_height;
}

uint8_t egui_view_linearlayout_is_orientation_horizontal(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->is_orientation_horizontal;
}

void egui_view_linearlayout_layout_childs(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);

    egui_view_group_layout_childs(self, local->is_orientation_horizontal, local->is_auto_width, local->is_auto_height, local->align_type);
}

void egui_view_linearlayout_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_linearlayout_t);
    // call super init.
    egui_view_group_init(self);

    // init local data.
    local->is_orientation_horizontal = 0;
    local->is_auto_width = 0;
    local->is_auto_height = 0;
    local->align_type = 0;

    egui_view_set_view_name(self, "egui_view_linearlayout");
}

void egui_view_linearlayout_apply_params(egui_view_t *self, const egui_view_linearlayout_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);

    self->region = params->region;

    local->align_type = params->align_type;
    local->is_orientation_horizontal = params->is_orientation_horizontal;

    egui_view_invalidate(self);
}

void egui_view_linearlayout_init_with_params(egui_view_t *self, const egui_view_linearlayout_params_t *params)
{
    egui_view_linearlayout_init(self);
    egui_view_linearlayout_apply_params(self, params);
}
