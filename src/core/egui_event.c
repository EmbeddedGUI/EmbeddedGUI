#include "egui_event.h"

#if EGUI_CONFIG_FUNCTION_EVENT_LITE

#include "egui_core.h"
#include "widget/egui_view.h"
#include "widget/egui_view_group.h"

int egui_view_add_event_listener(egui_view_t *self, egui_event_code_t code, egui_event_cb_t cb, void *user_data)
{
    uint8_t i;

    if (self == NULL || cb == NULL)
    {
        return -1;
    }

    for (i = 0; i < self->event_listener_count; i++)
    {
        egui_event_listener_t *listener = &self->event_listeners[i];
        if (listener->code == code && listener->cb == cb && listener->user_data == user_data)
        {
            return 0;
        }
    }

    if (self->event_listener_count >= EGUI_CONFIG_EVENT_MAX_LISTENERS_PER_VIEW)
    {
        return -1;
    }

    self->event_listeners[self->event_listener_count].code = code;
    self->event_listeners[self->event_listener_count].cb = cb;
    self->event_listeners[self->event_listener_count].user_data = user_data;
    self->event_listener_count++;

    return 0;
}

int egui_view_remove_event_listener(egui_view_t *self, egui_event_code_t code, egui_event_cb_t cb, void *user_data)
{
    uint8_t i;

    if (self == NULL || cb == NULL)
    {
        return -1;
    }

    for (i = 0; i < self->event_listener_count; i++)
    {
        egui_event_listener_t *listener = &self->event_listeners[i];
        if (listener->code == code && listener->cb == cb && listener->user_data == user_data)
        {
            uint8_t tail;
            for (tail = i; tail + 1 < self->event_listener_count; tail++)
            {
                self->event_listeners[tail] = self->event_listeners[tail + 1];
            }
            self->event_listener_count--;
            self->event_listeners[self->event_listener_count].code = EGUI_EVENT_ALL;
            self->event_listeners[self->event_listener_count].cb = NULL;
            self->event_listeners[self->event_listener_count].user_data = NULL;
            return 0;
        }
    }

    return -1;
}

void egui_view_clear_event_listeners(egui_view_t *self)
{
    uint8_t i;

    if (self == NULL)
    {
        return;
    }

    for (i = 0; i < EGUI_CONFIG_EVENT_MAX_LISTENERS_PER_VIEW; i++)
    {
        self->event_listeners[i].code = EGUI_EVENT_ALL;
        self->event_listeners[i].cb = NULL;
        self->event_listeners[i].user_data = NULL;
    }
    self->event_listener_count = 0;
}

int egui_view_send_event(egui_view_t *self, egui_event_code_t code, void *param)
{
    uint8_t i;
    int handled = 0;

    if (self == NULL)
    {
        return 0;
    }

    for (i = 0; i < self->event_listener_count; i++)
    {
        egui_event_listener_t *listener = &self->event_listeners[i];
        if (listener->cb != NULL && (listener->code == code || listener->code == EGUI_EVENT_ALL))
        {
            egui_event_t event;
            event.code = code;
            event.target = self;
            event.current_target = self;
            event.param = param;
            event.user_data = listener->user_data;
            listener->cb(&event);
            handled = 1;
        }
    }

    return handled;
}

void egui_view_send_event_to_tree(egui_view_t *root, egui_event_code_t code, void *param)
{
    egui_dnode_t *p_head;

    if (root == NULL)
    {
        return;
    }

    egui_view_send_event(root, code, param);

    if (root->api == NULL || (root->api->draw != egui_view_group_draw && root->api->request_layout != egui_view_group_request_layout))
    {
        return;
    }

    {
        egui_view_group_t *group = (egui_view_group_t *)root;
        if (egui_dlist_is_empty(&group->childs))
        {
            return;
        }

        EGUI_DLIST_FOR_EACH_NODE(&group->childs, p_head)
        {
            egui_view_t *child = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);
            egui_view_send_event_to_tree(child, code, param);
        }
    }
}

void egui_core_send_event_to_tree(egui_core_t *core, egui_event_code_t code, void *param)
{
    egui_view_group_t *root;

    if (core == NULL)
    {
        return;
    }

    root = egui_core_get_root_view(core);
    egui_view_send_event_to_tree((egui_view_t *)root, code, param);
}

#endif /* EGUI_CONFIG_FUNCTION_EVENT_LITE */
