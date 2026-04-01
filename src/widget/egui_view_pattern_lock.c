#include "egui_view_pattern_lock.h"

#define EGUI_VIEW_PATTERN_LOCK_NODE_NONE 0xFF

typedef struct egui_view_pattern_lock_layout egui_view_pattern_lock_layout_t;
struct egui_view_pattern_lock_layout
{
    egui_region_t region;
    egui_dim_t step_x;
    egui_dim_t step_y;
    egui_dim_t node_radius;
};

static uint8_t egui_view_pattern_lock_clamp_min_nodes(uint8_t min_nodes)
{
    if (min_nodes < 1)
    {
        return 1;
    }
    if (min_nodes > EGUI_VIEW_PATTERN_LOCK_MAX_NODES)
    {
        return EGUI_VIEW_PATTERN_LOCK_MAX_NODES;
    }
    return min_nodes;
}

static uint8_t egui_view_pattern_lock_clamp_touch_expand(uint8_t touch_expand)
{
    if (touch_expand > 24)
    {
        return 24;
    }
    return touch_expand;
}

static void egui_view_pattern_lock_reset_state(egui_view_pattern_lock_t *local)
{
    uint8_t i;
    local->node_count = 0;
    local->is_tracking = 0;
    local->is_completed = 0;
    local->show_error = 0;
    local->cursor_x = 0;
    local->cursor_y = 0;
    for (i = 0; i < EGUI_VIEW_PATTERN_LOCK_MAX_NODES; i++)
    {
        local->nodes[i] = EGUI_VIEW_PATTERN_LOCK_NODE_NONE;
        local->selected[i] = 0;
    }
}

static int egui_view_pattern_lock_build_layout(egui_view_t *self, egui_view_pattern_lock_layout_t *layout)
{
    egui_view_get_work_region(self, &layout->region);
    if (layout->region.size.width <= 20 || layout->region.size.height <= 20)
    {
        return 0;
    }

    layout->step_x = layout->region.size.width / EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;
    layout->step_y = layout->region.size.height / EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;
    if (layout->step_x <= 0 || layout->step_y <= 0)
    {
        return 0;
    }

    layout->node_radius = EGUI_MIN(layout->step_x, layout->step_y) / 4;
    if (layout->node_radius < 4)
    {
        layout->node_radius = 4;
    }
    return 1;
}

static void egui_view_pattern_lock_get_node_center(const egui_view_pattern_lock_layout_t *layout, uint8_t index, egui_dim_t *x, egui_dim_t *y)
{
    egui_dim_t col = index % EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;
    egui_dim_t row = index / EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;
    *x = layout->region.location.x + (layout->step_x / 2) + col * layout->step_x;
    *y = layout->region.location.y + (layout->step_y / 2) + row * layout->step_y;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static uint8_t egui_view_pattern_lock_find_hit_node(egui_view_t *self, egui_dim_t local_x, egui_dim_t local_y)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    egui_view_pattern_lock_layout_t layout;
    if (!egui_view_pattern_lock_build_layout(self, &layout))
    {
        return EGUI_VIEW_PATTERN_LOCK_NODE_NONE;
    }

    uint8_t i;
    int32_t hit_radius = (int32_t)layout.node_radius + (int32_t)local->touch_expand;
    int32_t hit_radius_sq = hit_radius * hit_radius;
    for (i = 0; i < EGUI_VIEW_PATTERN_LOCK_MAX_NODES; i++)
    {
        egui_dim_t cx, cy;
        egui_view_pattern_lock_get_node_center(&layout, i, &cx, &cy);
        int32_t dx = (int32_t)local_x - (int32_t)cx;
        int32_t dy = (int32_t)local_y - (int32_t)cy;
        int32_t distance_sq = dx * dx + dy * dy;
        if (distance_sq <= hit_radius_sq)
        {
            return i;
        }
    }

    return EGUI_VIEW_PATTERN_LOCK_NODE_NONE;
}

static uint8_t egui_view_pattern_lock_calc_bridge_node(uint8_t from_node, uint8_t to_node)
{
    uint8_t from_row = from_node / EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;
    uint8_t from_col = from_node % EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;
    uint8_t to_row = to_node / EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;
    uint8_t to_col = to_node % EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;

    int row_delta = (int)to_row - (int)from_row;
    int col_delta = (int)to_col - (int)from_col;
    if (row_delta < 0)
    {
        row_delta = -row_delta;
    }
    if (col_delta < 0)
    {
        col_delta = -col_delta;
    }

    if ((row_delta == 2 && col_delta == 0) || (row_delta == 0 && col_delta == 2) || (row_delta == 2 && col_delta == 2))
    {
        uint8_t mid_row = (uint8_t)(((int)from_row + (int)to_row) / 2);
        uint8_t mid_col = (uint8_t)(((int)from_col + (int)to_col) / 2);
        return (uint8_t)(mid_row * EGUI_VIEW_PATTERN_LOCK_GRID_SIZE + mid_col);
    }

    return EGUI_VIEW_PATTERN_LOCK_NODE_NONE;
}

static void egui_view_pattern_lock_add_node(egui_view_t *self, uint8_t node)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    egui_view_pattern_lock_layout_t layout;

    if (node >= EGUI_VIEW_PATTERN_LOCK_MAX_NODES || local->selected[node] || local->node_count >= EGUI_VIEW_PATTERN_LOCK_MAX_NODES)
    {
        return;
    }

    local->selected[node] = 1;
    local->nodes[local->node_count] = node;
    local->node_count++;

    if (egui_view_pattern_lock_build_layout(self, &layout))
    {
        egui_view_pattern_lock_get_node_center(&layout, node, &local->cursor_x, &local->cursor_y);
    }
}

static void egui_view_pattern_lock_add_node_with_bridge(egui_view_t *self, uint8_t node)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (local->node_count > 0)
    {
        uint8_t last_node = local->nodes[local->node_count - 1];
        uint8_t bridge = egui_view_pattern_lock_calc_bridge_node(last_node, node);
        if (bridge != EGUI_VIEW_PATTERN_LOCK_NODE_NONE && !local->selected[bridge])
        {
            egui_view_pattern_lock_add_node(self, bridge);
        }
    }
    egui_view_pattern_lock_add_node(self, node);
}
#endif

void egui_view_pattern_lock_set_min_nodes(egui_view_t *self, uint8_t min_nodes)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    uint8_t clamped = egui_view_pattern_lock_clamp_min_nodes(min_nodes);
    if (local->min_nodes == clamped)
    {
        return;
    }
    local->min_nodes = clamped;
    egui_view_invalidate(self);
}

uint8_t egui_view_pattern_lock_get_min_nodes(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    return local->min_nodes;
}

uint8_t egui_view_pattern_lock_get_node_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    return local->node_count;
}

const uint8_t *egui_view_pattern_lock_get_nodes(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    return local->nodes;
}

void egui_view_pattern_lock_clear_pattern(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (local->node_count == 0 && !local->is_tracking && !local->is_completed && !local->show_error)
    {
        return;
    }
    egui_view_pattern_lock_reset_state(local);
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

void egui_view_pattern_lock_set_touch_expand(egui_view_t *self, uint8_t touch_expand)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    uint8_t clamped = egui_view_pattern_lock_clamp_touch_expand(touch_expand);
    if (local->touch_expand == clamped)
    {
        return;
    }
    local->touch_expand = clamped;
    egui_view_invalidate(self);
}

uint8_t egui_view_pattern_lock_get_touch_expand(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    return local->touch_expand;
}

void egui_view_pattern_lock_set_on_pattern_complete_listener(egui_view_t *self, egui_view_on_pattern_complete_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (local->on_pattern_complete == listener)
    {
        return;
    }
    local->on_pattern_complete = listener;
}

void egui_view_pattern_lock_set_on_pattern_finish_listener(egui_view_t *self, egui_view_on_pattern_finish_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (local->on_pattern_finish == listener)
    {
        return;
    }
    local->on_pattern_finish = listener;
}

void egui_view_pattern_lock_set_bg_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (local->bg_color.full == color.full)
    {
        return;
    }
    local->bg_color = color;
    egui_view_invalidate(self);
}

void egui_view_pattern_lock_set_border_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (local->border_color.full == color.full)
    {
        return;
    }
    local->border_color = color;
    egui_view_invalidate(self);
}

void egui_view_pattern_lock_set_node_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (local->node_color.full == color.full)
    {
        return;
    }
    local->node_color = color;
    egui_view_invalidate(self);
}

void egui_view_pattern_lock_set_active_node_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (local->active_node_color.full == color.full)
    {
        return;
    }
    local->active_node_color = color;
    egui_view_invalidate(self);
}

void egui_view_pattern_lock_set_line_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (local->line_color.full == color.full)
    {
        return;
    }
    local->line_color = color;
    egui_view_invalidate(self);
}

void egui_view_pattern_lock_set_error_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (local->error_color.full == color.full)
    {
        return;
    }
    local->error_color = color;
    egui_view_invalidate(self);
}

void egui_view_pattern_lock_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    egui_view_pattern_lock_layout_t layout;
    if (!egui_view_pattern_lock_build_layout(self, &layout))
    {
        return;
    }

    int is_enabled = egui_view_get_enable(self);
    egui_color_t bg_color = local->bg_color;
    egui_color_t border_color = local->border_color;
    egui_color_t node_color = local->node_color;
    egui_color_t active_color = local->active_node_color;
    egui_color_t line_color = local->line_color;
    egui_color_t error_color = local->error_color;

    if (!is_enabled)
    {
        bg_color = egui_rgb_mix(bg_color, EGUI_THEME_DISABLED, EGUI_ALPHA_40);
        border_color = egui_rgb_mix(border_color, EGUI_THEME_DISABLED, EGUI_ALPHA_60);
        node_color = egui_rgb_mix(node_color, EGUI_THEME_DISABLED, EGUI_ALPHA_50);
        active_color = egui_rgb_mix(active_color, EGUI_THEME_DISABLED, EGUI_ALPHA_60);
        line_color = egui_rgb_mix(line_color, EGUI_THEME_DISABLED, EGUI_ALPHA_60);
        error_color = egui_rgb_mix(error_color, EGUI_THEME_DISABLED, EGUI_ALPHA_60);
    }

    egui_canvas_draw_round_rectangle_fill(layout.region.location.x, layout.region.location.y, layout.region.size.width, layout.region.size.height,
                                          EGUI_THEME_RADIUS_MD, bg_color, local->alpha);
    egui_canvas_draw_round_rectangle(layout.region.location.x, layout.region.location.y, layout.region.size.width, layout.region.size.height,
                                     EGUI_THEME_RADIUS_MD, 1, border_color, local->alpha);

    if (local->node_count > 1)
    {
        egui_dim_t path_width = layout.node_radius > 8 ? 4 : 3;
        uint8_t i;
        for (i = 1; i < local->node_count; i++)
        {
            egui_dim_t x1, y1, x2, y2;
            egui_view_pattern_lock_get_node_center(&layout, local->nodes[i - 1], &x1, &y1);
            egui_view_pattern_lock_get_node_center(&layout, local->nodes[i], &x2, &y2);
            egui_canvas_draw_line(x1, y1, x2, y2, path_width, local->show_error ? error_color : line_color, local->alpha);
        }
    }

    if (local->is_tracking && local->node_count > 0)
    {
        egui_dim_t tail_width = layout.node_radius > 8 ? 3 : 2;
        egui_color_t tail_color = local->show_error ? error_color : line_color;
        egui_dim_t x1, y1;
        egui_view_pattern_lock_get_node_center(&layout, local->nodes[local->node_count - 1], &x1, &y1);
        egui_canvas_draw_line(x1, y1, local->cursor_x, local->cursor_y, tail_width, tail_color, local->alpha);
    }

    uint8_t first_node = EGUI_VIEW_PATTERN_LOCK_NODE_NONE;
    uint8_t last_node = EGUI_VIEW_PATTERN_LOCK_NODE_NONE;
    if (local->node_count > 0)
    {
        first_node = local->nodes[0];
        last_node = local->nodes[local->node_count - 1];
    }

    uint8_t i;
    for (i = 0; i < EGUI_VIEW_PATTERN_LOCK_MAX_NODES; i++)
    {
        egui_dim_t cx, cy;
        egui_view_pattern_lock_get_node_center(&layout, i, &cx, &cy);
        if (local->selected[i])
        {
            egui_color_t fill = local->show_error ? error_color : active_color;
            egui_canvas_draw_circle_fill(cx, cy, layout.node_radius, fill, local->alpha);
            egui_canvas_draw_circle(cx, cy, layout.node_radius, 2, border_color, local->alpha);
            if (i == first_node)
            {
                egui_dim_t marker_radius = layout.node_radius > 5 ? (layout.node_radius / 2) : 2;
                egui_canvas_draw_circle_fill(cx, cy, marker_radius, EGUI_COLOR_WHITE, local->alpha);
                egui_canvas_draw_circle(cx, cy, marker_radius, 1, fill, local->alpha);
            }
            else if (i == last_node)
            {
                egui_color_t marker_bg = egui_rgb_mix(bg_color, EGUI_COLOR_WHITE, 18);
                egui_dim_t marker_radius = layout.node_radius > 6 ? (layout.node_radius / 2 - 1) : 2;
                egui_canvas_draw_circle_fill(cx, cy, marker_radius, marker_bg, local->alpha);
                egui_canvas_draw_circle(cx, cy, marker_radius, 1, EGUI_COLOR_WHITE, local->alpha);
            }
        }
        else
        {
            egui_color_t inner = egui_rgb_mix(bg_color, EGUI_COLOR_WHITE, 40);
            egui_dim_t inner_radius = layout.node_radius > 2 ? (layout.node_radius - 2) : layout.node_radius;
            egui_canvas_draw_circle_fill(cx, cy, inner_radius, inner, local->alpha);
            egui_canvas_draw_circle(cx, cy, layout.node_radius, 2, node_color, local->alpha);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_pattern_lock_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (!egui_view_get_enable(self))
    {
        return 0;
    }

    egui_dim_t local_x = event->location.x - self->region_screen.location.x;
    egui_dim_t local_y = event->location.y - self->region_screen.location.y;

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        uint8_t hit_node = egui_view_pattern_lock_find_hit_node(self, local_x, local_y);
        egui_view_pattern_lock_reset_state(local);
        local->cursor_x = local_x;
        local->cursor_y = local_y;
        if (hit_node == EGUI_VIEW_PATTERN_LOCK_NODE_NONE)
        {
            egui_view_set_pressed(self, false);
            egui_view_invalidate(self);
            return 0;
        }

        local->is_tracking = 1;
        egui_view_set_pressed(self, true);
        egui_view_pattern_lock_add_node(self, hit_node);
        egui_view_invalidate(self);
        return 1;
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (!local->is_tracking)
        {
            return 0;
        }
        local->cursor_x = local_x;
        local->cursor_y = local_y;
        {
            uint8_t hit_node = egui_view_pattern_lock_find_hit_node(self, local_x, local_y);
            if (hit_node != EGUI_VIEW_PATTERN_LOCK_NODE_NONE && !local->selected[hit_node])
            {
                egui_view_pattern_lock_add_node_with_bridge(self, hit_node);
            }
        }
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        if (!local->is_tracking)
        {
            return 0;
        }
        local->cursor_x = local_x;
        local->cursor_y = local_y;
        {
            uint8_t hit_node = egui_view_pattern_lock_find_hit_node(self, local_x, local_y);
            if (hit_node != EGUI_VIEW_PATTERN_LOCK_NODE_NONE && !local->selected[hit_node])
            {
                egui_view_pattern_lock_add_node_with_bridge(self, hit_node);
            }
        }
        local->is_tracking = 0;
        local->is_completed = 1;
        local->show_error = (local->node_count < local->min_nodes);
        if (local->on_pattern_finish != NULL)
        {
            local->on_pattern_finish(self, local->nodes, local->node_count, (uint8_t)!local->show_error);
        }
        if (!local->show_error && local->on_pattern_complete != NULL)
        {
            local->on_pattern_complete(self, local->node_count);
        }
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        egui_view_pattern_lock_reset_state(local);
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        break;
    }

    return 0;
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_pattern_lock_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_pattern_lock_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_pattern_lock_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_pattern_lock_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_pattern_lock_t);
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_pattern_lock_t);

    local->min_nodes = 4;
    local->alpha = EGUI_ALPHA_100;
    local->bg_color = EGUI_THEME_SURFACE_VARIANT;
    local->border_color = EGUI_THEME_BORDER;
    local->node_color = EGUI_THEME_TEXT_SECONDARY;
    local->active_node_color = EGUI_THEME_PRIMARY;
    local->line_color = EGUI_THEME_PRIMARY;
    local->error_color = EGUI_THEME_DANGER;
    local->touch_expand = 5;
    local->on_pattern_complete = NULL;
    local->on_pattern_finish = NULL;
    egui_view_pattern_lock_reset_state(local);
    egui_view_set_view_name(self, "egui_view_pattern_lock");
}

void egui_view_pattern_lock_apply_params(egui_view_t *self, const egui_view_pattern_lock_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_pattern_lock_t);
    if (params == NULL)
    {
        return;
    }
    self->region = params->region;
    local->min_nodes = egui_view_pattern_lock_clamp_min_nodes(params->min_nodes);
    local->touch_expand = egui_view_pattern_lock_clamp_touch_expand(params->touch_expand);
    egui_view_pattern_lock_reset_state(local);
    egui_view_invalidate(self);
}

void egui_view_pattern_lock_init_with_params(egui_view_t *self, const egui_view_pattern_lock_params_t *params)
{
    egui_view_pattern_lock_init(self);
    if (params == NULL)
    {
        return;
    }
    egui_view_pattern_lock_apply_params(self, params);
}
