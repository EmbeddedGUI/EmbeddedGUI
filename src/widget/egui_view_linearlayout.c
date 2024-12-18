#include <stdio.h>
#include <assert.h>

#include "egui_view_linearlayout.h"
#include "font/egui_font.h"

void egui_view_linearlayout_set_align_type(egui_view_t *self, uint8_t align_type)
{
    egui_view_linearlayout_t *local = (egui_view_linearlayout_t *)self;
    local->align_type = align_type;
}

void egui_view_linearlayout_set_auto_width(egui_view_t *self, uint8_t is_auto_width)
{
    egui_view_linearlayout_t *local = (egui_view_linearlayout_t *)self;
    local->is_auto_width = is_auto_width;
}

void egui_view_linearlayout_set_auto_height(egui_view_t *self, uint8_t is_auto_height)
{
    egui_view_linearlayout_t *local = (egui_view_linearlayout_t *)self;
    local->is_auto_height = is_auto_height;
}

void egui_view_linearlayout_set_orientation(egui_view_t *self, uint8_t is_horizontal)
{
    egui_view_linearlayout_t *local = (egui_view_linearlayout_t *)self;
    local->is_orientation_horizontal = is_horizontal;
}

uint8_t egui_view_linearlayout_is_align_type(egui_view_t *self)
{
    egui_view_linearlayout_t *local = (egui_view_linearlayout_t *)self;
    return local->align_type;
}

uint8_t egui_view_linearlayout_is_auto_width(egui_view_t *self)
{
    egui_view_linearlayout_t *local = (egui_view_linearlayout_t *)self;
    return local->is_auto_width;
}

uint8_t egui_view_linearlayout_is_auto_height(egui_view_t *self)
{
    egui_view_linearlayout_t *local = (egui_view_linearlayout_t *)self;
    return local->is_auto_height;
}

uint8_t egui_view_linearlayout_is_orientation_horizontal(egui_view_t *self)
{
    egui_view_linearlayout_t *local = (egui_view_linearlayout_t *)self;
    return local->is_orientation_horizontal;
}

void egui_view_linearlayout_layout_childs(egui_view_t *self)
{
    egui_view_linearlayout_t *local = (egui_view_linearlayout_t *)self;
    
    egui_view_group_layout_childs(self, local->is_orientation_horizontal, local->is_auto_width, local->is_auto_height, local->align_type);
}

void egui_view_linearlayout_init(egui_view_t *self)
{
    egui_view_linearlayout_t *local = (egui_view_linearlayout_t *)self;
    EGUI_UNUSED(local);
    // call super init.
    egui_view_group_init(self);

    // init local data.
    local->is_orientation_horizontal = 0;
    local->is_auto_width = 0;
    local->is_auto_height = 0;
    local->align_type = 0;

    egui_view_set_view_name(self, "egui_view_linearlayout");
}