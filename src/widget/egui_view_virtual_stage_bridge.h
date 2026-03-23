#ifndef _EGUI_VIEW_VIRTUAL_STAGE_BRIDGE_H_
#define _EGUI_VIEW_VIRTUAL_STAGE_BRIDGE_H_

#include "egui_view_virtual_stage.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_virtual_stage_array_bridge egui_view_virtual_stage_array_bridge_t;

void egui_core_add_user_root_view(egui_view_t *view);

struct egui_view_virtual_stage_array_bridge
{
    egui_view_virtual_stage_array_adapter_t adapter;
    egui_view_virtual_stage_array_setup_t setup;
};

#define EGUI_VIEW_VIRTUAL_STAGE_PARAMS_INIT_WITH_LIMIT(_name, _x, _y, _w, _h, _live_slot_limit)                                                                \
    static const egui_view_virtual_stage_params_t _name = {                                                                                                    \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .live_slot_limit = (_live_slot_limit),                                                                                                             \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_PARAMS_INIT_SCREEN_WITH_LIMIT(_name, _live_slot_limit)                                                                         \
    EGUI_VIEW_VIRTUAL_STAGE_PARAMS_INIT_WITH_LIMIT(_name, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, _live_slot_limit)

#define EGUI_VIEW_VIRTUAL_STAGE_ARRAY_BRIDGE_INIT(_name, _params, _node_source, _ops, _user_context)                                                           \
    static egui_view_virtual_stage_array_bridge_t _name = {                                                                                                    \
            .setup =                                                                                                                                           \
                    {                                                                                                                                          \
                            .params = (_params),                                                                                                               \
                            .node_source = (_node_source),                                                                                                     \
                            .ops = (_ops),                                                                                                                     \
                            .user_context = (_user_context),                                                                                                   \
                    },                                                                                                                                         \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_SIMPLE_INIT(_name, _create_view, _destroy_view, _bind_view, _draw_node)                                              \
    static const egui_view_virtual_stage_array_ops_t _name = {                                                                                                 \
            .create_view = (_create_view),                                                                                                                     \
            .destroy_view = (_destroy_view),                                                                                                                   \
            .bind_view = (_bind_view),                                                                                                                         \
            .draw_node = (_draw_node),                                                                                                                         \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_SIMPLE_CONST_INIT(_name, _create_view, _destroy_view, _bind_view, _draw_node)                                        \
    const egui_view_virtual_stage_array_ops_t _name = {                                                                                                        \
            .create_view = (_create_view),                                                                                                                     \
            .destroy_view = (_destroy_view),                                                                                                                   \
            .bind_view = (_bind_view),                                                                                                                         \
            .draw_node = (_draw_node),                                                                                                                         \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_INTERACTIVE_INIT(_name, _create_view, _destroy_view, _bind_view, _draw_node, _hit_test, _should_keep_alive)          \
    static const egui_view_virtual_stage_array_ops_t _name = {                                                                                                 \
            .create_view = (_create_view),                                                                                                                     \
            .destroy_view = (_destroy_view),                                                                                                                   \
            .bind_view = (_bind_view),                                                                                                                         \
            .draw_node = (_draw_node),                                                                                                                         \
            .hit_test = (_hit_test),                                                                                                                           \
            .should_keep_alive = (_should_keep_alive),                                                                                                         \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_INTERACTIVE_CONST_INIT(_name, _create_view, _destroy_view, _bind_view, _draw_node, _hit_test, _should_keep_alive)    \
    const egui_view_virtual_stage_array_ops_t _name = {                                                                                                        \
            .create_view = (_create_view),                                                                                                                     \
            .destroy_view = (_destroy_view),                                                                                                                   \
            .bind_view = (_bind_view),                                                                                                                         \
            .draw_node = (_draw_node),                                                                                                                         \
            .hit_test = (_hit_test),                                                                                                                           \
            .should_keep_alive = (_should_keep_alive),                                                                                                         \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_STATEFUL_INIT(_name, _create_view, _destroy_view, _bind_view, _save_state, _restore_state, _draw_node, _hit_test,    \
                                                        _should_keep_alive)                                                                                    \
    static const egui_view_virtual_stage_array_ops_t _name = {                                                                                                 \
            .create_view = (_create_view),                                                                                                                     \
            .destroy_view = (_destroy_view),                                                                                                                   \
            .bind_view = (_bind_view),                                                                                                                         \
            .save_state = (_save_state),                                                                                                                       \
            .restore_state = (_restore_state),                                                                                                                 \
            .draw_node = (_draw_node),                                                                                                                         \
            .hit_test = (_hit_test),                                                                                                                           \
            .should_keep_alive = (_should_keep_alive),                                                                                                         \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_STATEFUL_CONST_INIT(_name, _create_view, _destroy_view, _bind_view, _save_state, _restore_state, _draw_node,         \
                                                              _hit_test, _should_keep_alive)                                                                   \
    const egui_view_virtual_stage_array_ops_t _name = {                                                                                                        \
            .create_view = (_create_view),                                                                                                                     \
            .destroy_view = (_destroy_view),                                                                                                                   \
            .bind_view = (_bind_view),                                                                                                                         \
            .save_state = (_save_state),                                                                                                                       \
            .restore_state = (_restore_state),                                                                                                                 \
            .draw_node = (_draw_node),                                                                                                                         \
            .hit_test = (_hit_test),                                                                                                                           \
            .should_keep_alive = (_should_keep_alive),                                                                                                         \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_DESC_ARRAY_BRIDGE_INIT(_name, _params, _items, _ops, _user_context)                                                            \
    EGUI_VIEW_VIRTUAL_STAGE_DESC_ARRAY_SOURCE_INIT(_name##_node_source, _items);                                                                               \
    EGUI_VIEW_VIRTUAL_STAGE_ARRAY_BRIDGE_INIT(_name, _params, &_name##_node_source, _ops, _user_context)

#define EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT(_name, _params, _items, _item_type, _desc_member, _ops, _user_context)                                  \
    EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SOURCE_INIT(_name##_node_source, _items, _item_type, _desc_member);                                                     \
    EGUI_VIEW_VIRTUAL_STAGE_ARRAY_BRIDGE_INIT(_name, _params, &_name##_node_source, _ops, _user_context)

#define EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT_WITH_LIMIT(_name, _x, _y, _w, _h, _live_slot_limit, _items, _item_type, _desc_member, _ops,             \
                                                                  _user_context)                                                                               \
    EGUI_VIEW_VIRTUAL_STAGE_PARAMS_INIT_WITH_LIMIT(_name##_params, _x, _y, _w, _h, _live_slot_limit);                                                          \
    EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT(_name, &_name##_params, _items, _item_type, _desc_member, _ops, _user_context)

#define EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SCREEN_BRIDGE_INIT_WITH_LIMIT(_name, _live_slot_limit, _items, _item_type, _desc_member, _ops, _user_context)       \
    EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT_WITH_LIMIT(_name, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, _live_slot_limit, _items,        \
                                                              _item_type, _desc_member, _ops, _user_context)

#define EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SIMPLE_BRIDGE_INIT_WITH_LIMIT(_name, _x, _y, _w, _h, _live_slot_limit, _items, _item_type, _desc_member,            \
                                                                         _create_view, _destroy_view, _bind_view, _draw_node, _user_context)                   \
    EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_SIMPLE_INIT(_name##_ops, _create_view, _destroy_view, _bind_view, _draw_node);                                           \
    EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT_WITH_LIMIT(_name, _x, _y, _w, _h, _live_slot_limit, _items, _item_type, _desc_member, &_name##_ops,         \
                                                              _user_context)

#define EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(_name, _x, _y, _w, _h, _live_slot_limit, _items, _item_type, _desc_member,       \
                                                                              _create_view, _destroy_view, _bind_view, _draw_node, _hit_test,                  \
                                                                              _should_keep_alive, _user_context)                                               \
    EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_INTERACTIVE_INIT(_name##_ops, _create_view, _destroy_view, _bind_view, _draw_node, _hit_test, _should_keep_alive);       \
    EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT_WITH_LIMIT(_name, _x, _y, _w, _h, _live_slot_limit, _items, _item_type, _desc_member, &_name##_ops,         \
                                                              _user_context)

#define EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_STATEFUL_BRIDGE_INIT_WITH_LIMIT(_name, _x, _y, _w, _h, _live_slot_limit, _items, _item_type, _desc_member,          \
                                                                           _create_view, _destroy_view, _bind_view, _save_state, _restore_state, _draw_node,   \
                                                                           _hit_test, _should_keep_alive, _user_context)                                       \
    EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_STATEFUL_INIT(_name##_ops, _create_view, _destroy_view, _bind_view, _save_state, _restore_state, _draw_node, _hit_test,  \
                                                    _should_keep_alive);                                                                                       \
    EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT_WITH_LIMIT(_name, _x, _y, _w, _h, _live_slot_limit, _items, _item_type, _desc_member, &_name##_ops,         \
                                                              _user_context)

#define EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SCREEN_SIMPLE_BRIDGE_INIT_WITH_LIMIT(_name, _live_slot_limit, _items, _item_type, _desc_member, _create_view,       \
                                                                                _destroy_view, _bind_view, _draw_node, _user_context)                          \
    EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SIMPLE_BRIDGE_INIT_WITH_LIMIT(_name, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, _live_slot_limit, _items, \
                                                                     _item_type, _desc_member, _create_view, _destroy_view, _bind_view, _draw_node,            \
                                                                     _user_context)

#define EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SCREEN_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(_name, _live_slot_limit, _items, _item_type, _desc_member, _create_view,  \
                                                                                     _destroy_view, _bind_view, _draw_node, _hit_test, _should_keep_alive,     \
                                                                                     _user_context)                                                            \
    EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(_name, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, _live_slot_limit,    \
                                                                          _items, _item_type, _desc_member, _create_view, _destroy_view, _bind_view,           \
                                                                          _draw_node, _hit_test, _should_keep_alive, _user_context)

#define EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SCREEN_STATEFUL_BRIDGE_INIT_WITH_LIMIT(_name, _live_slot_limit, _items, _item_type, _desc_member, _create_view,     \
                                                                                  _destroy_view, _bind_view, _save_state, _restore_state, _draw_node,          \
                                                                                  _hit_test, _should_keep_alive, _user_context)                                \
    EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_STATEFUL_BRIDGE_INIT_WITH_LIMIT(_name, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, _live_slot_limit,       \
                                                                       _items, _item_type, _desc_member, _create_view, _destroy_view, _bind_view, _save_state, \
                                                                       _restore_state, _draw_node, _hit_test, _should_keep_alive, _user_context)

static inline void egui_view_virtual_stage_apply_array_bridge(egui_view_t *self, egui_view_virtual_stage_array_bridge_t *bridge)
{
    egui_view_virtual_stage_apply_array_setup(self, &bridge->adapter, &bridge->setup);
}

static inline void egui_view_virtual_stage_init_with_array_bridge(egui_view_t *self, egui_view_virtual_stage_array_bridge_t *bridge)
{
    egui_view_virtual_stage_init_with_array_setup(self, &bridge->adapter, &bridge->setup);
}

#define EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage) EGUI_VIEW_OF(_stage)

#define EGUI_VIEW_VIRTUAL_STAGE_APPLY_ARRAY_BRIDGE(_stage, _bridge)                                                                                            \
    egui_view_virtual_stage_apply_array_bridge(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_bridge))

#define EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(_stage, _bridge)                                                                                             \
    egui_view_virtual_stage_init_with_array_bridge(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_bridge))

#define EGUI_VIEW_VIRTUAL_STAGE_SET_BACKGROUND(_stage, _background) egui_view_set_background(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_background))

#define EGUI_VIEW_VIRTUAL_STAGE_OVERRIDE_ON_TOUCH(_stage, _api, _listener)                                                                                     \
    egui_view_override_api_on_touch(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_api), (_listener))

#define EGUI_VIEW_VIRTUAL_STAGE_SET_ON_CLICK(_stage, _listener) egui_view_set_on_click_listener(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_listener))

#define EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(_stage) egui_core_add_user_root_view(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage))

#define EGUI_VIEW_VIRTUAL_STAGE_INVALIDATE(_stage) egui_view_invalidate(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage))

#define EGUI_VIEW_VIRTUAL_STAGE_REQUEST_LAYOUT(_stage) egui_view_request_layout(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage))

#define EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(_stage) EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage)->api->calculate_layout(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage))

#define EGUI_VIEW_VIRTUAL_STAGE_SCREEN_REGION(_stage) (&EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage)->region_screen)

#define EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_DATA(_stage) egui_view_virtual_stage_notify_data_changed(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage))

#define EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(_stage, _stable_id)                                                                                                \
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_stable_id))

#define EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE_BOUNDS(_stage, _stable_id)                                                                                         \
    egui_view_virtual_stage_notify_node_bounds_changed(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_stable_id))

#define EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES(_stage, _stable_ids)                                                                                              \
    egui_view_virtual_stage_notify_nodes_changed(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_stable_ids), EGUI_ARRAY_SIZE(_stable_ids))

#define EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(_stage, ...)                                                                                                        \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        const uint32_t _egui_virtual_stage_stable_ids[] = {__VA_ARGS__};                                                                                       \
        egui_view_virtual_stage_notify_nodes_changed(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), _egui_virtual_stage_stable_ids,                                  \
                                                     EGUI_ARRAY_SIZE(_egui_virtual_stage_stable_ids));                                                         \
    } while (0)

#define EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES_BOUNDS(_stage, _stable_ids)                                                                                       \
    egui_view_virtual_stage_notify_nodes_bounds_changed(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_stable_ids), EGUI_ARRAY_SIZE(_stable_ids))

#define EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_BOUNDS_IDS(_stage, ...)                                                                                                 \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        const uint32_t _egui_virtual_stage_stable_ids[] = {__VA_ARGS__};                                                                                       \
        egui_view_virtual_stage_notify_nodes_bounds_changed(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), _egui_virtual_stage_stable_ids,                           \
                                                            EGUI_ARRAY_SIZE(_egui_virtual_stage_stable_ids));                                                  \
    } while (0)

#define EGUI_VIEW_VIRTUAL_STAGE_PIN(_stage, _stable_id) egui_view_virtual_stage_pin_node(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_stable_id))

#define EGUI_VIEW_VIRTUAL_STAGE_PIN_IDS(_stage, ...)                                                                                                           \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        const uint32_t _egui_virtual_stage_stable_ids[] = {__VA_ARGS__};                                                                                       \
        uint32_t _egui_virtual_stage_index;                                                                                                                    \
        for (_egui_virtual_stage_index = 0U; _egui_virtual_stage_index < EGUI_ARRAY_SIZE(_egui_virtual_stage_stable_ids); _egui_virtual_stage_index++)         \
        {                                                                                                                                                      \
            (void)egui_view_virtual_stage_pin_node(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), _egui_virtual_stage_stable_ids[_egui_virtual_stage_index]);        \
        }                                                                                                                                                      \
    } while (0)

#define EGUI_VIEW_VIRTUAL_STAGE_UNPIN(_stage, _stable_id) egui_view_virtual_stage_unpin_node(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_stable_id))

#define EGUI_VIEW_VIRTUAL_STAGE_UNPIN_IDS(_stage, ...)                                                                                                         \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        const uint32_t _egui_virtual_stage_stable_ids[] = {__VA_ARGS__};                                                                                       \
        uint32_t _egui_virtual_stage_index;                                                                                                                    \
        for (_egui_virtual_stage_index = 0U; _egui_virtual_stage_index < EGUI_ARRAY_SIZE(_egui_virtual_stage_stable_ids); _egui_virtual_stage_index++)         \
        {                                                                                                                                                      \
            egui_view_virtual_stage_unpin_node(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), _egui_virtual_stage_stable_ids[_egui_virtual_stage_index]);            \
        }                                                                                                                                                      \
    } while (0)

#define EGUI_VIEW_VIRTUAL_STAGE_IS_PINNED(_stage, _stable_id) egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_stable_id))

#define EGUI_VIEW_VIRTUAL_STAGE_TOGGLE_PIN(_stage, _stable_id) egui_view_virtual_stage_toggle_pin_node(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_stable_id))

#define EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(_stage) egui_view_virtual_stage_get_slot_count(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage))

#define EGUI_VIEW_VIRTUAL_STAGE_GET_SLOT(_stage, _slot_index) egui_view_virtual_stage_get_slot(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_slot_index))

#define EGUI_VIEW_VIRTUAL_STAGE_FIND_SLOT_BY_ID(_stage, _stable_id)                                                                                            \
    egui_view_virtual_stage_find_slot_by_stable_id(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_stable_id))

#define EGUI_VIEW_VIRTUAL_STAGE_FIND_VIEW_BY_ID(_stage, _stable_id)                                                                                            \
    egui_view_virtual_stage_find_view_by_stable_id(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_stable_id))

#define EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_NODE_BY_VIEW(_stage, _view, _out_entry)                                                                                \
    egui_view_virtual_stage_resolve_node_by_view(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_view), (_out_entry))

#define EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_ID_BY_VIEW(_stage, _view, _out_stable_id)                                                                              \
    egui_view_virtual_stage_resolve_stable_id_by_view(EGUI_VIEW_VIRTUAL_STAGE_AS_VIEW(_stage), (_view), (_out_stable_id))

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_STAGE_BRIDGE_H_ */
