#include <stdio.h>

#include "egui_view_flexlayout.h"

#if EGUI_CONFIG_FUNCTION_FLEXLAYOUT

#include "core/egui_api.h"

/**
 * @file egui_view_flexlayout.c
 * @brief CSS Flexbox-style layout container.
 *
 * Supports direction (row/column), wrap, justify-content, align-items,
 * align-content, row/col gap, and per-child flex_grow. All arithmetic is
 * integer-only; no heap allocation is used — a fixed-size stack array of
 * egui_flex_line_t records (up to EGUI_CONFIG_FLEXLAYOUT_MAX_LINES) is used
 * during the layout pass.
 */

/* --------------------------------------------------------------------------
 * Internal line descriptor
 * -------------------------------------------------------------------------- */

/* Maximum children tracked per line. Reuses EGUI_CONFIG_FLEXLAYOUT_MAX_LINES
 * as an upper bound for items per line as well — in practice a single line
 * can hold many children but the common case is far fewer. We over-provision
 * with a separate items pool to keep the struct small. */
#define FLEX_MAX_ITEMS_PER_LINE EGUI_CONFIG_FLEXLAYOUT_MAX_LINES

typedef struct
{
    egui_view_t *items[FLEX_MAX_ITEMS_PER_LINE];
    int count;
    egui_dim_t main_size;  /* occupied main-axis length incl. gaps between items */
    egui_dim_t cross_size; /* tallest/widest item in this line */
} egui_flex_line_t;

/* --------------------------------------------------------------------------
 * Helpers: item main/cross dimension with margin
 * -------------------------------------------------------------------------- */

static egui_dim_t item_main(egui_view_t *child, uint8_t is_row)
{
    if (is_row)
    {
        return child->region.size.width + child->margin.left + child->margin.right;
    }
    return child->region.size.height + child->margin.top + child->margin.bottom;
}

static egui_dim_t item_cross(egui_view_t *child, uint8_t is_row)
{
    if (is_row)
    {
        return child->region.size.height + child->margin.top + child->margin.bottom;
    }
    return child->region.size.width + child->margin.left + child->margin.right;
}

/* --------------------------------------------------------------------------
 * Phase 1: Break children into lines
 * -------------------------------------------------------------------------- */

static int flex_build_lines(egui_view_group_t *group, uint8_t is_row, uint8_t do_wrap, egui_dim_t main_limit, egui_dim_t gap, egui_flex_line_t *lines,
                            int max_lines)
{
    int line_idx = 0;
    egui_dim_t cursor = 0;
    int first_item = 1;

    egui_api_memset(lines, 0, (int)(sizeof(egui_flex_line_t) * (unsigned int)max_lines));

    egui_dnode_t *p_head;
    EGUI_DLIST_FOR_EACH_NODE(&group->childs, p_head)
    {
        egui_view_t *child = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

        if (child->is_gone)
        {
            continue;
        }

        egui_dim_t m = item_main(child, is_row);
        egui_dim_t c = item_cross(child, is_row);

        /* Decide whether to wrap */
        if (!first_item && do_wrap && (cursor + gap + m > main_limit))
        {
            line_idx++;
            if (line_idx >= max_lines)
            {
                /* Out of line budget — clamp to last line */
                line_idx = max_lines - 1;
            }
            cursor = 0;
            first_item = 1;
        }

        egui_flex_line_t *line = &lines[line_idx];

        if (!first_item)
        {
            cursor += gap;
            line->main_size += gap;
        }

        /* Store item if room */
        if (line->count < FLEX_MAX_ITEMS_PER_LINE)
        {
            line->items[line->count] = child;
            line->count++;
        }

        cursor += m;
        line->main_size += m;

        if (c > line->cross_size)
        {
            line->cross_size = c;
        }

        first_item = 0;
    }

    return line_idx + 1;
}

/* --------------------------------------------------------------------------
 * Phase 2: Distribute free space via flex_grow
 * -------------------------------------------------------------------------- */

static void flex_apply_grow(egui_flex_line_t *line, uint8_t is_row, egui_dim_t container_main, egui_dim_t gap)
{
    egui_dim_t free_space = container_main - line->main_size;

    if (free_space <= 0 || line->count == 0)
    {
        return;
    }

    int total_grow = 0;

    for (int i = 0; i < line->count; i++)
    {
        total_grow += (int)egui_view_get_flex_grow(line->items[i]);
    }

    if (total_grow == 0)
    {
        return;
    }

    for (int i = 0; i < line->count; i++)
    {
        uint8_t grow = egui_view_get_flex_grow(line->items[i]);

        if (grow == 0)
        {
            continue;
        }

        egui_dim_t extra = (egui_dim_t)((int)free_space * (int)grow / total_grow);
        egui_view_t *child = line->items[i];

        if (is_row)
        {
            egui_view_set_size(child, child->region.size.width + extra, child->region.size.height);
        }
        else
        {
            egui_view_set_size(child, child->region.size.width, child->region.size.height + extra);
        }

        line->main_size += extra;
    }

    EGUI_UNUSED(gap);
}

/* --------------------------------------------------------------------------
 * Phase 3: Compute justify_content spacing
 * -------------------------------------------------------------------------- */

typedef struct
{
    egui_dim_t leading; /* offset before first item */
    egui_dim_t between; /* gap between items */
} egui_flex_spacing_t;

static egui_flex_spacing_t flex_compute_justify(uint8_t justify, egui_dim_t free_space, int count)
{
    egui_flex_spacing_t sp;
    sp.leading = 0;
    sp.between = 0;

    if (free_space <= 0 || count <= 0)
    {
        return sp;
    }

    switch (justify)
    {
    case EGUI_FLEX_JUSTIFY_END:
        sp.leading = free_space;
        break;

    case EGUI_FLEX_JUSTIFY_CENTER:
        sp.leading = free_space / 2;
        break;

    case EGUI_FLEX_JUSTIFY_SPACE_BETWEEN:
        if (count > 1)
        {
            sp.between = free_space / (count - 1);
        }
        else
        {
            sp.leading = free_space / 2;
        }
        break;

    case EGUI_FLEX_JUSTIFY_SPACE_AROUND:
        sp.between = free_space / count;
        sp.leading = sp.between / 2;
        break;

    case EGUI_FLEX_JUSTIFY_SPACE_EVENLY:
        sp.between = free_space / (count + 1);
        sp.leading = sp.between;
        break;

    default: /* EGUI_FLEX_JUSTIFY_START */
        break;
    }

    return sp;
}

/* --------------------------------------------------------------------------
 * Phase 4 & 5: Place items (main + cross)
 * -------------------------------------------------------------------------- */

static void flex_place_line(egui_flex_line_t *line, uint8_t is_row, uint8_t justify, uint8_t align, egui_dim_t container_main, egui_dim_t gap,
                            egui_dim_t origin_main, egui_dim_t origin_cross)
{
    egui_dim_t free_space = container_main - line->main_size;
    egui_flex_spacing_t sp = flex_compute_justify(justify, free_space, line->count);

    egui_dim_t cursor = origin_main + sp.leading;

    for (int i = 0; i < line->count; i++)
    {
        egui_view_t *child = line->items[i];

        egui_dim_t m = item_main(child, is_row);
        egui_dim_t c = item_cross(child, is_row);
        egui_dim_t cx = 0; /* cross-axis offset within the line */

        switch (align)
        {
        case EGUI_FLEX_ALIGN_END:
            cx = line->cross_size - c;
            break;

        case EGUI_FLEX_ALIGN_CENTER:
            cx = (line->cross_size - c) / 2;
            break;

        case EGUI_FLEX_ALIGN_STRETCH:
            /* Resize item to fill line cross-axis */
            if (is_row)
            {
                egui_dim_t target_h = line->cross_size - child->margin.top - child->margin.bottom;

                if (target_h > 0)
                {
                    egui_view_set_size(child, child->region.size.width, target_h);
                }
            }
            else
            {
                egui_dim_t target_w = line->cross_size - child->margin.left - child->margin.right;

                if (target_w > 0)
                {
                    egui_view_set_size(child, target_w, child->region.size.height);
                }
            }
            cx = 0;
            break;

        default: /* EGUI_FLEX_ALIGN_START */
            cx = 0;
            break;
        }

        egui_dim_t pos_main = cursor;
        egui_dim_t pos_cross = origin_cross + cx;

        if (is_row)
        {
            egui_view_set_position(child, pos_main + child->margin.left, pos_cross + child->margin.top);
        }
        else
        {
            egui_view_set_position(child, pos_cross + child->margin.left, pos_main + child->margin.top);
        }

        cursor += m;
        if (i < line->count - 1)
        {
            cursor += gap + sp.between;
        }
    }
}

/* --------------------------------------------------------------------------
 * Public: layout_childs
 * -------------------------------------------------------------------------- */

void egui_view_flexlayout_layout_childs(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);

    egui_view_group_t *group = (egui_view_group_t *)self;

    if (egui_dlist_is_empty(&group->childs))
    {
        return;
    }

    uint8_t is_row = (local->direction == EGUI_FLEX_DIRECTION_ROW);
    egui_dim_t pad_l = self->padding.left;
    egui_dim_t pad_t = self->padding.top;
    egui_dim_t pad_r = self->padding.right;
    egui_dim_t pad_b = self->padding.bottom;
    egui_dim_t content_w = self->region.size.width - pad_l - pad_r;
    egui_dim_t content_h = self->region.size.height - pad_t - pad_b;
    egui_dim_t main_limit = is_row ? content_w : content_h;
    egui_dim_t cross_avail = is_row ? content_h : content_w;
    egui_dim_t main_gap = is_row ? local->col_gap : local->row_gap;
    egui_dim_t cross_gap = is_row ? local->row_gap : local->col_gap;

    egui_flex_line_t lines[EGUI_CONFIG_FLEXLAYOUT_MAX_LINES];
    int line_count = flex_build_lines(group, is_row, local->wrap, main_limit, main_gap, lines, EGUI_CONFIG_FLEXLAYOUT_MAX_LINES);

    /* Grow pass */
    for (int li = 0; li < line_count; li++)
    {
        flex_apply_grow(&lines[li], is_row, main_limit, main_gap);
    }

    /* Total cross size for align_content distribution */
    egui_dim_t total_cross = 0;

    for (int li = 0; li < line_count; li++)
    {
        total_cross += lines[li].cross_size;
    }

    if (line_count > 1)
    {
        total_cross += cross_gap * (egui_dim_t)(line_count - 1);
    }

    /* Compute line cross origins using align_content */
    egui_dim_t cross_free = cross_avail - total_cross;
    egui_flex_spacing_t line_sp = flex_compute_justify(local->align_content, cross_free, line_count);
    egui_dim_t cross_cursor = (is_row ? pad_t : pad_l) + line_sp.leading;

    /* Place each line */
    egui_dim_t origin_main = is_row ? pad_l : pad_t;

    for (int li = 0; li < line_count; li++)
    {
        flex_place_line(&lines[li], is_row, local->justify_content, local->align_items, main_limit, main_gap, origin_main, cross_cursor);

        cross_cursor += lines[li].cross_size;

        if (li < line_count - 1)
        {
            cross_cursor += line_sp.between + cross_gap;
        }
    }
}

/* --------------------------------------------------------------------------
 * Public: setters + init
 * -------------------------------------------------------------------------- */

void egui_view_flexlayout_set_direction(egui_view_t *self, uint8_t direction)
{
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    local->direction = direction;
}

uint8_t egui_view_flexlayout_get_direction(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    return local->direction;
}

void egui_view_flexlayout_set_wrap(egui_view_t *self, uint8_t wrap)
{
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    local->wrap = wrap;
}

uint8_t egui_view_flexlayout_get_wrap(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    return local->wrap;
}

void egui_view_flexlayout_set_justify_content(egui_view_t *self, uint8_t justify)
{
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    local->justify_content = justify;
}

uint8_t egui_view_flexlayout_get_justify_content(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    return local->justify_content;
}

void egui_view_flexlayout_set_align_items(egui_view_t *self, uint8_t align)
{
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    local->align_items = align;
}

uint8_t egui_view_flexlayout_get_align_items(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    return local->align_items;
}

void egui_view_flexlayout_set_align_content(egui_view_t *self, uint8_t align)
{
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    local->align_content = align;
}

uint8_t egui_view_flexlayout_get_align_content(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    return local->align_content;
}

void egui_view_flexlayout_set_gap(egui_view_t *self, egui_dim_t row_gap, egui_dim_t col_gap)
{
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    local->row_gap = row_gap;
    local->col_gap = col_gap;
}

egui_dim_t egui_view_flexlayout_get_row_gap(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    return local->row_gap;
}

egui_dim_t egui_view_flexlayout_get_col_gap(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    return local->col_gap;
}

void egui_view_flexlayout_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_LOCAL_INIT(egui_view_flexlayout_t);
    egui_view_group_init(self, core);
    local->direction = EGUI_FLEX_DIRECTION_ROW;
    local->wrap = EGUI_FLEX_WRAP_NOWRAP;
    local->justify_content = EGUI_FLEX_JUSTIFY_START;
    local->align_items = EGUI_FLEX_ALIGN_STRETCH;
    local->align_content = EGUI_FLEX_JUSTIFY_START;
    local->row_gap = 0;
    local->col_gap = 0;
}

#endif /* EGUI_CONFIG_FUNCTION_FLEXLAYOUT */
