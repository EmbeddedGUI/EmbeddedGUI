#include <stdio.h>
#include <assert.h>

#include "egui_view_gridlayout.h"

/**
 * @file egui_view_gridlayout.c
 * @brief Fixed-column grid container that places children row by row.
 *
 * The gridlayout keeps its policy small and explicit: visible children are
 * distributed in append order, each column gets the same width, and each row
 * advances by the tallest child assigned to that row.
 */

/** Update the column count, normalizing zero to one column. */
void egui_view_gridlayout_set_col_count(egui_view_t *self, uint8_t col_count)
{
    EGUI_LOCAL_INIT(egui_view_gridlayout_t);
    if (col_count == 0)
    {
        col_count = 1;
    }
    local->col_count = col_count;
}

uint8_t egui_view_gridlayout_get_col_count(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_gridlayout_t);
    return local->col_count;
}

/** Store an alignment hint for the grid layout. */
void egui_view_gridlayout_set_align_type(egui_view_t *self, uint8_t align_type)
{
    EGUI_LOCAL_INIT(egui_view_gridlayout_t);
    local->align_type = align_type;
}

uint8_t egui_view_gridlayout_get_align_type(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_gridlayout_t);
    return local->align_type;
}

/** Place visible children into equal-width columns and row-height buckets. */
void egui_view_gridlayout_layout_childs(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_gridlayout_t);

    egui_view_group_t *group = (egui_view_group_t *)self;

    if (egui_dlist_is_empty(&group->childs) || local->col_count == 0)
    {
        return;
    }

    egui_dim_t container_w = self->region.size.width;
    egui_dim_t cell_w = container_w / local->col_count;

    egui_dnode_t *p_head;
    egui_view_t *child;

    uint8_t index = 0;
    egui_dim_t row_y = 0;
    egui_dim_t row_max_h = 0;

    EGUI_DLIST_FOR_EACH_NODE(&group->childs, p_head)
    {
        child = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

        if (child->is_gone)
        {
            continue;
        }

        uint8_t col = index % local->col_count;

        // Start a new row whenever the current item wraps back to column zero.
        if (col == 0 && index > 0)
        {
            row_y += row_max_h;
            row_max_h = 0;
        }

        // Include margins in the occupied cell footprint so spacing remains consistent.
        egui_dim_t child_w = child->region.size.width + child->margin.left + child->margin.right;
        egui_dim_t child_h = child->region.size.height + child->margin.top + child->margin.bottom;

        // The current helper centers each child horizontally inside its column cell.
        egui_dim_t child_x = col * cell_w + (cell_w - child_w) / 2 + child->margin.left;
        egui_dim_t child_y = row_y + child->margin.top;

        egui_view_set_position(child, child_x, child_y);

        // Track the tallest child in the row so the next row starts below it.
        if (child_h > row_max_h)
        {
            row_max_h = child_h;
        }

        index++;
    }
}

/** Initialize the grid container with a stock two-column layout. */
void egui_view_gridlayout_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_gridlayout_t);
    // call super init.
    egui_view_group_init(self, core);

    // init local data.
    local->col_count = 2;
    local->align_type = 0;

    egui_view_set_view_name(self, "egui_view_gridlayout");
}

/** Apply geometry, column count, and alignment hint from one parameter block. */
void egui_view_gridlayout_apply_params(egui_view_t *self, const egui_view_gridlayout_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_gridlayout_t);

    self->region = params->region;

    local->col_count = params->col_count;
    local->align_type = params->align_type;

    egui_view_invalidate(self);
}

/** Convenience helper that initializes the gridlayout before applying params. */
void egui_view_gridlayout_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_gridlayout_params_t *params)
{
    egui_view_gridlayout_init(self, core);
    egui_view_gridlayout_apply_params(self, params);
}
