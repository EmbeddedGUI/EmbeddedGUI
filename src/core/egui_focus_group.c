#include "egui_focus_group.h"

#if EGUI_CONFIG_FUNCTION_FOCUS_GROUP

#include "egui_focus.h"
#include "widget/egui_view.h"

static int egui_focus_group_view_can_focus(egui_view_t *view)
{
    return egui_focus_view_is_focusable(view);
}

void egui_focus_group_init(egui_focus_group_t *group, egui_core_t *core)
{
    uint8_t i;

    if (group == NULL)
    {
        return;
    }

    group->core = core;
    group->count = 0;
    group->index = 0;
    group->wrap_mode = EGUI_FOCUS_GROUP_WRAP;
    for (i = 0; i < EGUI_CONFIG_FOCUS_GROUP_MAX_VIEWS; i++)
    {
        group->views[i] = NULL;
    }
}

int egui_focus_group_add_view(egui_focus_group_t *group, egui_view_t *view)
{
    uint8_t i;

    if (group == NULL || view == NULL || group->count >= EGUI_CONFIG_FOCUS_GROUP_MAX_VIEWS)
    {
        return -1;
    }

    for (i = 0; i < group->count; i++)
    {
        if (group->views[i] == view)
        {
            return 0;
        }
    }

    group->views[group->count] = view;
    group->count++;
    return 0;
}

int egui_focus_group_remove_view(egui_focus_group_t *group, egui_view_t *view)
{
    uint8_t i;

    if (group == NULL || view == NULL)
    {
        return -1;
    }

    for (i = 0; i < group->count; i++)
    {
        if (group->views[i] == view)
        {
            uint8_t tail;
            for (tail = i; tail + 1 < group->count; tail++)
            {
                group->views[tail] = group->views[tail + 1];
            }
            group->count--;
            group->views[group->count] = NULL;
            if (group->count == 0)
            {
                group->index = 0;
            }
            else if (group->index >= group->count)
            {
                group->index = (uint8_t)(group->count - 1);
            }
            return 0;
        }
    }

    return -1;
}

void egui_focus_group_clear(egui_focus_group_t *group)
{
    uint8_t i;

    if (group == NULL)
    {
        return;
    }

    for (i = 0; i < EGUI_CONFIG_FOCUS_GROUP_MAX_VIEWS; i++)
    {
        group->views[i] = NULL;
    }
    group->count = 0;
    group->index = 0;
}

void egui_focus_group_set_wrap_mode(egui_focus_group_t *group, egui_focus_group_wrap_t mode)
{
    if (group == NULL)
    {
        return;
    }

    group->wrap_mode = (uint8_t)mode;
}

int egui_focus_group_focus_index(egui_focus_group_t *group, uint8_t index)
{
    egui_view_t *view;

    if (group == NULL || index >= group->count)
    {
        return -1;
    }

    view = group->views[index];
    if (!egui_focus_group_view_can_focus(view))
    {
        return -1;
    }

    group->index = index;
    egui_focus_manager_set_focus(group->core != NULL ? group->core : egui_view_get_core(view), view);
    return 0;
}

static int egui_focus_group_move(egui_focus_group_t *group, int step)
{
    uint8_t tries;
    uint8_t next;

    if (group == NULL || group->count == 0)
    {
        return -1;
    }

    next = group->index;
    for (tries = 0; tries < group->count; tries++)
    {
        if (step > 0)
        {
            if (next + 1 >= group->count)
            {
                if (group->wrap_mode == EGUI_FOCUS_GROUP_CLAMP)
                {
                    next = (uint8_t)(group->count - 1);
                }
                else
                {
                    next = 0;
                }
            }
            else
            {
                next++;
            }
        }
        else
        {
            if (next == 0)
            {
                if (group->wrap_mode == EGUI_FOCUS_GROUP_CLAMP)
                {
                    next = 0;
                }
                else
                {
                    next = (uint8_t)(group->count - 1);
                }
            }
            else
            {
                next--;
            }
        }

        if (egui_focus_group_focus_index(group, next) == 0)
        {
            return 0;
        }

        if (group->wrap_mode == EGUI_FOCUS_GROUP_CLAMP && (next == 0 || next + 1 >= group->count))
        {
            break;
        }
    }

    return -1;
}

int egui_focus_group_focus_next(egui_focus_group_t *group)
{
    return egui_focus_group_move(group, 1);
}

int egui_focus_group_focus_prev(egui_focus_group_t *group)
{
    return egui_focus_group_move(group, -1);
}

egui_view_t *egui_focus_group_get_focused_view(egui_focus_group_t *group)
{
    if (group == NULL || group->count == 0 || group->index >= group->count)
    {
        return NULL;
    }

    return group->views[group->index];
}

uint8_t egui_focus_group_get_count(egui_focus_group_t *group)
{
    if (group == NULL)
    {
        return 0;
    }

    return group->count;
}

uint8_t egui_focus_group_get_index(egui_focus_group_t *group)
{
    if (group == NULL)
    {
        return 0;
    }

    return group->index;
}

#endif /* EGUI_CONFIG_FUNCTION_FOCUS_GROUP */
