#include <stdio.h>
#include <assert.h>

#include "egui_view_gridlayout.h"

void egui_view_gridlayout_set_col_count(egui_view_t *self, uint8_t col_count)
{
    EGUI_LOCAL_INIT(egui_view_gridlayout_t);
    if (col_count == 0)
    {
        col_count = 1;
    }
    local->col_count = col_count;
}

void egui_view_gridlayout_set_align_type(egui_view_t *self, uint8_t align_type)
{
    EGUI_LOCAL_INIT(egui_view_gridlayout_t);
    local->align_type = align_type;
}

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

        // Start a new row
        if (col == 0 && index > 0)
        {
            row_y += row_max_h;
            row_max_h = 0;
        }

        // Center child within the cell horizontally
        egui_dim_t child_w = child->region.size.width + child->margin.left + child->margin.right;
        egui_dim_t child_h = child->region.size.height + child->margin.top + child->margin.bottom;

        egui_dim_t child_x = col * cell_w + (cell_w - child_w) / 2 + child->margin.left;
        egui_dim_t child_y = row_y + child->margin.top;

        egui_view_set_position(child, child_x, child_y);

        if (child_h > row_max_h)
        {
            row_max_h = child_h;
        }

        index++;
    }
}

void egui_view_gridlayout_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_gridlayout_t);
    // call super init.
    egui_view_group_init(self);

    // init local data.
    local->col_count = 2;
    local->align_type = 0;

    egui_view_set_view_name(self, "egui_view_gridlayout");
}

void egui_view_gridlayout_apply_params(egui_view_t *self, const egui_view_gridlayout_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_gridlayout_t);

    self->region = params->region;

    local->col_count = params->col_count;
    local->align_type = params->align_type;

    egui_view_invalidate(self);
}

void egui_view_gridlayout_init_with_params(egui_view_t *self, const egui_view_gridlayout_params_t *params)
{
    egui_view_gridlayout_init(self);
    egui_view_gridlayout_apply_params(self, params);
}
