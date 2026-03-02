#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_tileview.h"

#define EGUI_VIEW_TILEVIEW_OFFSCREEN_POS (-9999)

static egui_view_t *egui_view_tileview_find_tile(egui_view_tileview_t *local, uint8_t col, uint8_t row)
{
    uint8_t i;
    for (i = 0; i < local->tile_count; i++)
    {
        if (local->tile_positions[i].col == col && local->tile_positions[i].row == row)
        {
            return local->tiles[i];
        }
    }
    return NULL;
}

void egui_view_tileview_add_tile(egui_view_t *self, egui_view_t *tile_view, uint8_t col, uint8_t row)
{
    EGUI_LOCAL_INIT(egui_view_tileview_t);

    if (local->tile_count >= EGUI_VIEW_TILEVIEW_MAX_TILES)
    {
        return;
    }

    local->tiles[local->tile_count] = tile_view;
    local->tile_positions[local->tile_count].col = col;
    local->tile_positions[local->tile_count].row = row;
    local->tile_count++;

    egui_view_group_add_child(self, tile_view);

    // Position tile: current tile at (0,0), others off-screen
    if (col == local->current_col && row == local->current_row)
    {
        egui_view_set_position(tile_view, 0, 0);
    }
    else
    {
        egui_view_set_position(tile_view, EGUI_VIEW_TILEVIEW_OFFSCREEN_POS, EGUI_VIEW_TILEVIEW_OFFSCREEN_POS);
    }

    egui_view_invalidate(self);
}

void egui_view_tileview_set_current(egui_view_t *self, uint8_t col, uint8_t row)
{
    EGUI_LOCAL_INIT(egui_view_tileview_t);

    egui_view_t *target = egui_view_tileview_find_tile(local, col, row);
    if (target == NULL)
    {
        return;
    }

    // Move old current tile off-screen
    egui_view_t *old_tile = egui_view_tileview_find_tile(local, local->current_col, local->current_row);
    if (old_tile != NULL)
    {
        egui_view_set_position(old_tile, EGUI_VIEW_TILEVIEW_OFFSCREEN_POS, EGUI_VIEW_TILEVIEW_OFFSCREEN_POS);
    }

    // Move new tile to (0,0)
    egui_view_set_position(target, 0, 0);

    local->current_col = col;
    local->current_row = row;

    egui_view_invalidate(self);

    if (local->on_changed != NULL)
    {
        local->on_changed(self, col, row);
    }
}

void egui_view_tileview_set_on_changed(egui_view_t *self, egui_view_tileview_changed_cb_t callback)
{
    EGUI_LOCAL_INIT(egui_view_tileview_t);
    local->on_changed = callback;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_tileview_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // Intercept move events so children don't get them during swipe
    if (event->type == EGUI_MOTION_EVENT_ACTION_MOVE)
    {
        EGUI_LOCAL_INIT(egui_view_tileview_t);
        egui_dim_t dx = event->location.x - local->touch_down_x;
        egui_dim_t dy = event->location.y - local->touch_down_y;
        if (EGUI_ABS(dx) > EGUI_VIEW_TILEVIEW_SWIPE_THRESH || EGUI_ABS(dy) > EGUI_VIEW_TILEVIEW_SWIPE_THRESH)
        {
            return 1;
        }
    }
    else if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        EGUI_LOCAL_INIT(egui_view_tileview_t);
        local->touch_down_x = event->location.x;
        local->touch_down_y = event->location.y;
    }

    return egui_view_group_on_intercept_touch_event(self, event);
}

int egui_view_tileview_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_tileview_t);

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        local->touch_down_x = event->location.x;
        local->touch_down_y = event->location.y;
        break;
    case EGUI_MOTION_EVENT_ACTION_UP:
    {
        egui_dim_t dx = event->location.x - local->touch_down_x;
        egui_dim_t dy = event->location.y - local->touch_down_y;
        egui_dim_t abs_dx = EGUI_ABS(dx);
        egui_dim_t abs_dy = EGUI_ABS(dy);

        if (abs_dx > EGUI_VIEW_TILEVIEW_SWIPE_THRESH || abs_dy > EGUI_VIEW_TILEVIEW_SWIPE_THRESH)
        {
            uint8_t target_col = local->current_col;
            uint8_t target_row = local->current_row;

            if (abs_dx > abs_dy)
            {
                // Horizontal swipe
                if (dx < 0 && local->current_col < 255)
                {
                    target_col = local->current_col + 1;
                }
                else if (dx > 0 && local->current_col > 0)
                {
                    target_col = local->current_col - 1;
                }
            }
            else
            {
                // Vertical swipe
                if (dy < 0 && local->current_row < 255)
                {
                    target_row = local->current_row + 1;
                }
                else if (dy > 0 && local->current_row > 0)
                {
                    target_row = local->current_row - 1;
                }
            }

            // Only navigate if target tile exists
            if (target_col != local->current_col || target_row != local->current_row)
            {
                if (egui_view_tileview_find_tile(local, target_col, target_row) != NULL)
                {
                    egui_view_tileview_set_current(self, target_col, target_row);
                }
            }
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    default:
        break;
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_tileview_t) = {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = egui_view_tileview_on_touch_event,
        .on_intercept_touch_event = egui_view_tileview_on_intercept_touch_event,
#else
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = NULL,
        .on_intercept_touch_event = NULL,
#endif
        .compute_scroll = egui_view_group_compute_scroll,
        .calculate_layout = egui_view_group_calculate_layout,
        .request_layout = egui_view_group_request_layout,
        .draw = egui_view_group_draw,
        .on_attach_to_window = egui_view_group_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_group_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_tileview_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_tileview_t);
    // call super init.
    egui_view_group_init(self);

    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_tileview_t);

    // init local data.
    memset(local->tiles, 0, sizeof(local->tiles));
    memset(local->tile_positions, 0, sizeof(local->tile_positions));
    local->tile_count = 0;
    local->current_col = 0;
    local->current_row = 0;
    local->touch_down_x = 0;
    local->touch_down_y = 0;
    local->on_changed = NULL;

    egui_view_set_view_name(self, "egui_view_tileview");
}

void egui_view_tileview_apply_params(egui_view_t *self, const egui_view_tileview_params_t *params)
{
    self->region = params->region;

    egui_view_invalidate(self);
}

void egui_view_tileview_init_with_params(egui_view_t *self, const egui_view_tileview_params_t *params)
{
    egui_view_tileview_init(self);
    egui_view_tileview_apply_params(self, params);
}
