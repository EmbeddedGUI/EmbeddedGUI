#include <stdio.h>

#include "egui_focus.h"
#include "egui_core.h"
#include "egui_key_event.h"
#include "widget/egui_view.h"
#include "widget/egui_view_group.h"
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
#include "egui_event.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

/**
 * @file egui_focus.c
 * @brief Minimal focus-management helpers for keyboard and accessibility-style navigation.
 */

/** Initialize focus tracking with no view currently selected. */
void egui_focus_manager_init(egui_core_t *core)
{
    core->system.focus_manager.focused_view = NULL;
}

/**
 * Update one view's focused flag, fire the optional callback, and invalidate
 * the view so focus visuals can be redrawn.
 */
static void egui_focus_set_view_focused(egui_view_t *view, int is_focused)
{
    view->is_focused = is_focused;
    if (!is_focused && view->is_pressed)
    {
        egui_view_set_pressed(view, false);
    }
    if (view->api->on_focus_changed != NULL)
    {
        view->api->on_focus_changed(view, is_focused);
    }
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    egui_view_send_event(view, is_focused ? EGUI_EVENT_FOCUSED : EGUI_EVENT_DEFOCUSED, NULL);
#endif
    egui_view_invalidate(view);
    egui_view_invalidate_focus_region(view);
}

/** Replace the current focused view with a new one, clearing the old target first. */
void egui_focus_manager_set_focus(egui_core_t *core, egui_view_t *view)
{
    if (core == NULL)
    {
        return;
    }

    if (view != NULL && !egui_focus_view_is_focusable(view))
    {
        view = NULL;
    }

    if (core->system.focus_manager.focused_view == view)
    {
        return;
    }

    egui_view_t *old_view = core->system.focus_manager.focused_view;

    // Publish the new target before callbacks so blur handlers can tell whether
    // focus moved into a related subtree such as an on-screen keyboard.
    core->system.focus_manager.focused_view = view;

    // Clear the previously focused view before activating the new target.
    if (old_view != NULL)
    {
        egui_focus_set_view_focused(old_view, 0);
    }

    // Activate the new focus target, if one was requested.
    if (view != NULL)
    {
        egui_focus_set_view_focused(view, 1);
    }
}

/** Clear the current focus selection, if any. */
void egui_focus_manager_clear_focus(egui_core_t *core)
{
    egui_focus_manager_set_focus(core, NULL);
}

/** Return the view that currently owns focus in this core. */
egui_view_t *egui_focus_manager_get_focused_view(egui_core_t *core)
{
    return core->system.focus_manager.focused_view;
}

/**
 * Check if a view can receive focus.
 */
int egui_focus_view_is_focusable(egui_view_t *view)
{
    egui_view_t *current;

    if (view == NULL || !view->is_focusable)
    {
        return 0;
    }

    current = view;
    while (current != NULL)
    {
        if (!current->is_enable || !current->is_visible || current->is_gone)
        {
            return 0;
        }

        current = (egui_view_t *)current->parent;
    }

    return 1;
}

/**
 * Build a flat list of focusable views by DFS traversal of the view tree.
 * Returns the count of focusable views found.
 * max_count limits the array size.
 */
static int egui_focus_collect_focusable_views(egui_view_t *root, egui_view_t **list, int max_count)
{
    int count = 0;

    // Simple stack-based DFS using a fixed-size stack to avoid heap allocation.
    // Depth and result count are intentionally capped for embedded use.
    egui_view_t *stack[EGUI_CONFIG_FOCUS_DFS_MAX_DEPTH];
    int stack_top = 0;

    stack[stack_top++] = root;

    while (stack_top > 0 && count < max_count)
    {
        egui_view_t *current = stack[--stack_top];

        if (!current->is_enable || !current->is_visible || current->is_gone)
        {
            continue;
        }

        // Record focusable views in traversal order.
        if (egui_focus_view_is_focusable(current))
        {
            list[count++] = current;
        }

        // Push children in reverse order so DFS still visits the first child first.
        // Some derived containers reuse `egui_view_group` storage while overriding behavior.
        if (current->api->draw == egui_view_group_draw || current->api->request_layout == egui_view_group_request_layout)
        {
            egui_view_group_t *group = (egui_view_group_t *)current;
            egui_dnode_t *p_head;

            // Collect children first, then push in reverse order.
            egui_view_t *children[EGUI_CONFIG_FOCUS_DFS_MAX_DEPTH];
            int child_count = 0;

            if (!egui_dlist_is_empty(&group->childs))
            {
                EGUI_DLIST_FOR_EACH_NODE(&group->childs, p_head)
                {
                    if (child_count < EGUI_CONFIG_FOCUS_DFS_MAX_DEPTH)
                    {
                        children[child_count++] = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);
                    }
                }
            }

            // Reverse push keeps the original child order when popping from the stack.
            for (int i = child_count - 1; i >= 0; i--)
            {
                if (stack_top < EGUI_CONFIG_FOCUS_DFS_MAX_DEPTH)
                {
                    stack[stack_top++] = children[i];
                }
            }
        }
    }

    return count;
}

static egui_dim_t egui_focus_region_center_x(const egui_region_t *region)
{
    return region->location.x + region->size.width / 2;
}

static egui_dim_t egui_focus_region_center_y(const egui_region_t *region)
{
    return region->location.y + region->size.height / 2;
}

static int egui_focus_key_is_direction(uint8_t key_code)
{
    return key_code == EGUI_KEY_CODE_UP || key_code == EGUI_KEY_CODE_DOWN || key_code == EGUI_KEY_CODE_LEFT || key_code == EGUI_KEY_CODE_RIGHT;
}

static int egui_focus_candidate_is_in_direction(uint8_t key_code, egui_dim_t current_x, egui_dim_t current_y, egui_dim_t candidate_x, egui_dim_t candidate_y)
{
    switch (key_code)
    {
    case EGUI_KEY_CODE_UP:
        return candidate_y < current_y;
    case EGUI_KEY_CODE_DOWN:
        return candidate_y > current_y;
    case EGUI_KEY_CODE_LEFT:
        return candidate_x < current_x;
    case EGUI_KEY_CODE_RIGHT:
        return candidate_x > current_x;
    default:
        return 0;
    }
}

static uint32_t egui_focus_get_spatial_score(uint8_t key_code, egui_dim_t current_x, egui_dim_t current_y, egui_dim_t candidate_x, egui_dim_t candidate_y)
{
    uint32_t primary;
    uint32_t secondary;

    if (key_code == EGUI_KEY_CODE_LEFT || key_code == EGUI_KEY_CODE_RIGHT)
    {
        primary = (uint32_t)EGUI_ABS(candidate_x - current_x);
        secondary = (uint32_t)EGUI_ABS(candidate_y - current_y);
    }
    else
    {
        primary = (uint32_t)EGUI_ABS(candidate_y - current_y);
        secondary = (uint32_t)EGUI_ABS(candidate_x - current_x);
    }

    return primary * 1024U + secondary;
}

static int egui_focus_ranges_overlap(egui_dim_t a_start, egui_dim_t a_end, egui_dim_t b_start, egui_dim_t b_end)
{
    return a_start < b_end && b_start < a_end;
}

static int egui_focus_candidate_is_in_beam(uint8_t key_code, const egui_region_t *current_region, const egui_region_t *candidate_region)
{
    if (key_code == EGUI_KEY_CODE_LEFT || key_code == EGUI_KEY_CODE_RIGHT)
    {
        egui_dim_t current_top = current_region->location.y;
        egui_dim_t current_bottom = current_region->location.y + current_region->size.height;
        egui_dim_t candidate_top = candidate_region->location.y;
        egui_dim_t candidate_bottom = candidate_region->location.y + candidate_region->size.height;

        return egui_focus_ranges_overlap(current_top, current_bottom, candidate_top, candidate_bottom);
    }

    if (key_code == EGUI_KEY_CODE_UP || key_code == EGUI_KEY_CODE_DOWN)
    {
        egui_dim_t current_left = current_region->location.x;
        egui_dim_t current_right = current_region->location.x + current_region->size.width;
        egui_dim_t candidate_left = candidate_region->location.x;
        egui_dim_t candidate_right = candidate_region->location.x + candidate_region->size.width;

        return egui_focus_ranges_overlap(current_left, current_right, candidate_left, candidate_right);
    }

    return 0;
}

static uint32_t egui_focus_axis_rank(egui_dim_t value)
{
    return (uint32_t)((int32_t)value + (int32_t)EGUI_DIM_MAX);
}

static uint32_t egui_focus_get_wrap_score(uint8_t key_code, const egui_region_t *screen_region, egui_dim_t current_x, egui_dim_t current_y)
{
    egui_dim_t candidate_x = egui_focus_region_center_x(screen_region);
    egui_dim_t candidate_y = egui_focus_region_center_y(screen_region);
    uint32_t secondary;
    uint32_t edge;
    uint32_t max_rank = (uint32_t)EGUI_DIM_MAX * 2U + 1U;

    if (key_code == EGUI_KEY_CODE_LEFT || key_code == EGUI_KEY_CODE_RIGHT)
    {
        secondary = (uint32_t)EGUI_ABS(candidate_y - current_y);
        edge = (key_code == EGUI_KEY_CODE_RIGHT) ? egui_focus_axis_rank(candidate_x) : (max_rank - egui_focus_axis_rank(candidate_x));
    }
    else
    {
        secondary = (uint32_t)EGUI_ABS(candidate_x - current_x);
        edge = (key_code == EGUI_KEY_CODE_DOWN) ? egui_focus_axis_rank(candidate_y) : (max_rank - egui_focus_axis_rank(candidate_y));
    }

    return edge * 1024U + secondary;
}

/** Move focus to the next focusable view in traversal order, wrapping around at the end. */
void egui_focus_manager_move_focus_next(egui_core_t *core)
{
    egui_view_t *focusable_list[EGUI_CONFIG_FOCUS_MAX_FOCUSABLE_VIEWS];
    egui_view_group_t *root = egui_core_get_root_view(core);

    int count = egui_focus_collect_focusable_views((egui_view_t *)root, focusable_list, EGUI_CONFIG_FOCUS_MAX_FOCUSABLE_VIEWS);

    if (count == 0)
    {
        return;
    }

    // Find the current focus target inside the flattened traversal order.
    int current_idx = -1;
    if (core->system.focus_manager.focused_view != NULL)
    {
        for (int i = 0; i < count; i++)
        {
            if (focusable_list[i] == core->system.focus_manager.focused_view)
            {
                current_idx = i;
                break;
            }
        }
    }

    // Advance to the next focus target, wrapping back to the first entry.
    int next_idx = (current_idx + 1) % count;
    egui_focus_manager_set_focus(core, focusable_list[next_idx]);
}

/** Move focus to the previous focusable view in traversal order, wrapping around at the beginning. */
void egui_focus_manager_move_focus_prev(egui_core_t *core)
{
    egui_view_t *focusable_list[EGUI_CONFIG_FOCUS_MAX_FOCUSABLE_VIEWS];
    egui_view_group_t *root = egui_core_get_root_view(core);

    int count = egui_focus_collect_focusable_views((egui_view_t *)root, focusable_list, EGUI_CONFIG_FOCUS_MAX_FOCUSABLE_VIEWS);

    if (count == 0)
    {
        return;
    }

    // Find the current focus target inside the flattened traversal order.
    int current_idx = -1;
    if (core->system.focus_manager.focused_view != NULL)
    {
        for (int i = 0; i < count; i++)
        {
            if (focusable_list[i] == core->system.focus_manager.focused_view)
            {
                current_idx = i;
                break;
            }
        }
    }

    // Move backward one step, wrapping to the last focusable view when needed.
    int prev_idx;
    if (current_idx <= 0)
    {
        prev_idx = count - 1;
    }
    else
    {
        prev_idx = current_idx - 1;
    }
    egui_focus_manager_set_focus(core, focusable_list[prev_idx]);
}

int egui_focus_manager_move_focus_direction(egui_core_t *core, uint8_t key_code)
{
    egui_view_t *focusable_list[EGUI_CONFIG_FOCUS_MAX_FOCUSABLE_VIEWS];
    egui_view_group_t *root = egui_core_get_root_view(core);
    egui_view_t *current;
    egui_view_t *best = NULL;
    egui_view_t *wrap_best = NULL;
    int count;
    egui_dim_t current_x = 0;
    egui_dim_t current_y = 0;
    uint32_t best_score = UINT32_MAX;
    uint32_t wrap_score = UINT32_MAX;
    int current_found = 0;
    int best_is_in_beam = 0;

    if (!egui_focus_key_is_direction(key_code))
    {
        return 0;
    }

    count = egui_focus_collect_focusable_views((egui_view_t *)root, focusable_list, EGUI_CONFIG_FOCUS_MAX_FOCUSABLE_VIEWS);
    if (count == 0)
    {
        return 0;
    }

    current = core->system.focus_manager.focused_view;
    if (current != NULL && egui_focus_view_is_focusable(current))
    {
        for (int i = 0; i < count; i++)
        {
            if (focusable_list[i] == current)
            {
                current_found = 1;
                break;
            }
        }
    }

    if (current == NULL || !current_found)
    {
        egui_focus_manager_set_focus(core, focusable_list[0]);
        return 1;
    }

    current_x = egui_focus_region_center_x(&current->region_screen);
    current_y = egui_focus_region_center_y(&current->region_screen);

    for (int i = 0; i < count; i++)
    {
        egui_view_t *candidate = focusable_list[i];
        egui_dim_t candidate_x;
        egui_dim_t candidate_y;
        uint32_t score;

        if (candidate == current || egui_region_is_empty(&candidate->region_screen))
        {
            continue;
        }

        candidate_x = egui_focus_region_center_x(&candidate->region_screen);
        candidate_y = egui_focus_region_center_y(&candidate->region_screen);
        if (egui_focus_candidate_is_in_direction(key_code, current_x, current_y, candidate_x, candidate_y))
        {
            int candidate_is_in_beam = egui_focus_candidate_is_in_beam(key_code, &current->region_screen, &candidate->region_screen);

            score = egui_focus_get_spatial_score(key_code, current_x, current_y, candidate_x, candidate_y);
            if (best == NULL || (candidate_is_in_beam && !best_is_in_beam) || (candidate_is_in_beam == best_is_in_beam && score < best_score))
            {
                best_score = score;
                best = candidate;
                best_is_in_beam = candidate_is_in_beam;
            }
        }

        score = egui_focus_get_wrap_score(key_code, &candidate->region_screen, current_x, current_y);
        if (score < wrap_score)
        {
            wrap_score = score;
            wrap_best = candidate;
        }
    }

    if (best == NULL)
    {
        best = wrap_best;
    }

    if (best == NULL)
    {
        return 0;
    }

    egui_focus_manager_set_focus(core, best);
    return 1;
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
