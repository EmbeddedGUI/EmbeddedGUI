#include <stdio.h>

#include "egui_focus.h"
#include "egui_core.h"
#include "widget/egui_view.h"
#include "widget/egui_view_group.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

static egui_focus_manager_t focus_manager;

void egui_focus_manager_init(void)
{
    focus_manager.focused_view = NULL;
}

static void egui_focus_set_view_focused(egui_view_t *view, int is_focused)
{
    view->is_focused = is_focused;
    if (view->on_focus_change_listener != NULL)
    {
        view->on_focus_change_listener(view, is_focused);
    }
    egui_view_invalidate(view);
}

void egui_focus_manager_set_focus(egui_view_t *view)
{
    if (focus_manager.focused_view == view)
    {
        return;
    }

    // Clear old focus
    if (focus_manager.focused_view != NULL)
    {
        egui_focus_set_view_focused(focus_manager.focused_view, 0);
    }

    // Set new focus
    focus_manager.focused_view = view;
    if (view != NULL)
    {
        egui_focus_set_view_focused(view, 1);
    }
}

void egui_focus_manager_clear_focus(void)
{
    egui_focus_manager_set_focus(NULL);
}

egui_view_t *egui_focus_manager_get_focused_view(void)
{
    return focus_manager.focused_view;
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

// Simple stack-based DFS using a fixed-size stack
// For embedded systems, we limit the depth
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

        // If this view is focusable, add it to the list
        if (egui_focus_view_is_focusable(current))
        {
            list[count++] = current;
        }

        // If this is a view_group, push children in reverse order so first child is processed first
        // We detect view_group by checking if the draw function is egui_view_group_draw or similar
        // A simpler approach: check if the view has the view_group dispatch functions
        if (current->api->draw == egui_view_group_draw)
        {
            egui_view_group_t *group = (egui_view_group_t *)current;
            egui_dnode_t *p_head;

            // Push children in reverse order onto stack
            // First collect them, then push in reverse
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

            // Push in reverse order so first child is on top of stack
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

void egui_focus_manager_move_focus_next(void)
{
    egui_view_t *focusable_list[FOCUS_MAX_FOCUSABLE_VIEWS];
    egui_view_group_t *root = egui_core_get_root_view();

    int count = egui_focus_collect_focusable_views((egui_view_t *)root, focusable_list, FOCUS_MAX_FOCUSABLE_VIEWS);

    if (count == 0)
    {
        return;
    }

    // Find current focused view in the list
    int current_idx = -1;
    if (focus_manager.focused_view != NULL)
    {
        for (int i = 0; i < count; i++)
        {
            if (focusable_list[i] == focus_manager.focused_view)
            {
                current_idx = i;
                break;
            }
        }
    }

    // Move to next, wrap around
    int next_idx = (current_idx + 1) % count;
    egui_focus_manager_set_focus(focusable_list[next_idx]);
}

void egui_focus_manager_move_focus_prev(void)
{
    egui_view_t *focusable_list[FOCUS_MAX_FOCUSABLE_VIEWS];
    egui_view_group_t *root = egui_core_get_root_view();

    int count = egui_focus_collect_focusable_views((egui_view_t *)root, focusable_list, FOCUS_MAX_FOCUSABLE_VIEWS);

    if (count == 0)
    {
        return;
    }

    // Find current focused view in the list
    int current_idx = -1;
    if (focus_manager.focused_view != NULL)
    {
        for (int i = 0; i < count; i++)
        {
            if (focusable_list[i] == focus_manager.focused_view)
            {
                current_idx = i;
                break;
            }
        }
    }

    // Move to previous, wrap around
    int prev_idx;
    if (current_idx <= 0)
    {
        prev_idx = count - 1;
    }
    else
    {
        prev_idx = current_idx - 1;
    }
    egui_focus_manager_set_focus(focusable_list[prev_idx]);
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
