#include <stdio.h>
#include <assert.h>

#include "egui_view_linearlayout.h"
#include "font/egui_font.h"

/**
 * @file egui_view_linearlayout.c
 * @brief One-dimensional layout container driven by shared group-layout helpers.
 *
 * The widget itself stores only a few policy flags. Actual child placement is
 * delegated to `egui_view_group_layout_childs`, which interprets direction,
 * auto-size behavior, and alignment in one shared code path.
 */

/** Store the alignment flags consumed by the next layout pass. */
void egui_view_linearlayout_set_align_type(egui_view_t *self, uint8_t align_type)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    local->align_type = align_type;
}

/** Enable or disable automatic width growth based on child content. */
void egui_view_linearlayout_set_auto_width(egui_view_t *self, uint8_t is_auto_width)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    local->is_auto_width = is_auto_width;
}

/** Enable or disable automatic height growth based on child content. */
void egui_view_linearlayout_set_auto_height(egui_view_t *self, uint8_t is_auto_height)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    local->is_auto_height = is_auto_height;
}

/** Switch between vertical and horizontal child flow. */
void egui_view_linearlayout_set_orientation(egui_view_t *self, uint8_t is_horizontal)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    local->is_orientation_horizontal = is_horizontal;
}

/** Return the currently stored alignment flags. */
uint8_t egui_view_linearlayout_is_align_type(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->align_type;
}

uint8_t egui_view_linearlayout_get_align_type(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->align_type;
}

/** Report whether automatic width growth is enabled. */
uint8_t egui_view_linearlayout_is_auto_width(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->is_auto_width;
}

uint8_t egui_view_linearlayout_get_auto_width(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->is_auto_width;
}

/** Report whether automatic height growth is enabled. */
uint8_t egui_view_linearlayout_is_auto_height(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->is_auto_height;
}

uint8_t egui_view_linearlayout_get_auto_height(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->is_auto_height;
}

/** Report whether child flow is currently horizontal. */
uint8_t egui_view_linearlayout_is_orientation_horizontal(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->is_orientation_horizontal;
}

uint8_t egui_view_linearlayout_get_orientation(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);
    return local->is_orientation_horizontal;
}

/** Recalculate child positions by forwarding all policy flags to the shared group helper. */
void egui_view_linearlayout_layout_childs(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);

    egui_view_group_layout_childs(self, local->is_orientation_horizontal, local->is_auto_width, local->is_auto_height, local->align_type);
}

/** Initialize the linearlayout container with vertical, fixed-size defaults. */
void egui_view_linearlayout_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_linearlayout_t);
    // call super init.
    egui_view_group_init(self, core);

    // init local data.
    local->is_orientation_horizontal = 0;
    local->is_auto_width = 0;
    local->is_auto_height = 0;
    local->align_type = 0;

    egui_view_set_view_name(self, "egui_view_linearlayout");
}

/** Apply geometry, alignment, and orientation from one parameter block. */
void egui_view_linearlayout_apply_params(egui_view_t *self, const egui_view_linearlayout_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_linearlayout_t);

    self->region = params->region;

    local->align_type = params->align_type;
    local->is_orientation_horizontal = params->is_orientation_horizontal;

    egui_view_invalidate(self);
}

/** Convenience helper that initializes the linearlayout before applying params. */
void egui_view_linearlayout_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_linearlayout_params_t *params)
{
    egui_view_linearlayout_init(self, core);
    egui_view_linearlayout_apply_params(self, params);
}
