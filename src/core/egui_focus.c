#include <stdio.h>

#include "egui_focus.h"
#include "egui_core.h"
#include "widget/egui_view.h"
#include "widget/egui_view_group.h"

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
    if (view->api->on_focus_changed != NULL)
    {
        view->api->on_focus_changed(view, is_focused);
    }
    egui_view_invalidate(view);
}

/** Replace the current focused view with a new one, clearing the old target first. */
void egui_focus_manager_set_focus(egui_core_t *core, egui_view_t *view)
{
    if (core->system.focus_manager.focused_view == view)
    {
        return;
    }

    // Clear the previously focused view before assigning the new target.
    if (core->system.focus_manager.focused_view != NULL)
    {
        egui_focus_set_view_focused(core->system.focus_manager.focused_view, 0);
    }

    // Activate the new focus target, if one was requested.
    core->system.focus_manager.focused_view = view;
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
static int egui_focus_view_is_focusable(egui_view_t *view)
{
    return view->is_focusable && view->is_enable && view->is_visible && !view->is_gone;
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
#define FOCUS_DFS_MAX_DEPTH 16
    egui_view_t *stack[FOCUS_DFS_MAX_DEPTH];
    int stack_top = 0;

    stack[stack_top++] = root;

    while (stack_top > 0 && count < max_count)
    {
        egui_view_t *current = stack[--stack_top];

        if (!current->is_visible || current->is_gone)
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
            egui_view_t *children[FOCUS_DFS_MAX_DEPTH];
            int child_count = 0;

            if (!egui_dlist_is_empty(&group->childs))
            {
                EGUI_DLIST_FOR_EACH_NODE(&group->childs, p_head)
                {
                    if (child_count < FOCUS_DFS_MAX_DEPTH)
                    {
                        children[child_count++] = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);
                    }
                }
            }

            // Reverse push keeps the original child order when popping from the stack.
            for (int i = child_count - 1; i >= 0; i--)
            {
                if (stack_top < FOCUS_DFS_MAX_DEPTH)
                {
                    stack[stack_top++] = children[i];
                }
            }
        }
    }

    return count;
#undef FOCUS_DFS_MAX_DEPTH
}

#define FOCUS_MAX_FOCUSABLE_VIEWS 32

/** Move focus to the next focusable view in traversal order, wrapping around at the end. */
void egui_focus_manager_move_focus_next(egui_core_t *core)
{
    egui_view_t *focusable_list[FOCUS_MAX_FOCUSABLE_VIEWS];
    egui_view_group_t *root = egui_core_get_root_view(core);

    int count = egui_focus_collect_focusable_views((egui_view_t *)root, focusable_list, FOCUS_MAX_FOCUSABLE_VIEWS);

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
    egui_view_t *focusable_list[FOCUS_MAX_FOCUSABLE_VIEWS];
    egui_view_group_t *root = egui_core_get_root_view(core);

    int count = egui_focus_collect_focusable_views((egui_view_t *)root, focusable_list, FOCUS_MAX_FOCUSABLE_VIEWS);

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

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
