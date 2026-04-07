#include "test_virtual_stage.h"

#include <string.h>

#include "egui.h"
#include "port_api.h"

#define TEST_VIRTUAL_STAGE_MAX_NODES 4

enum
{
    TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON = 1,
    TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT = 2,
    TEST_VIRTUAL_STAGE_VIEW_TYPE_LIST = 3,
    TEST_VIRTUAL_STAGE_VIEW_TYPE_SPINNER = 4,
};

typedef struct test_virtual_stage_node_data test_virtual_stage_node_data_t;
typedef struct test_virtual_stage_context test_virtual_stage_context_t;
typedef struct test_virtual_stage_array_node_data test_virtual_stage_array_node_data_t;
typedef struct test_virtual_stage_array_context test_virtual_stage_array_context_t;

struct test_virtual_stage_node_data
{
    egui_virtual_stage_node_desc_t desc;
    char text[EGUI_CONFIG_TEXTINPUT_MAX_LENGTH + 1];
    char label[16];
};

struct test_virtual_stage_context
{
    uint32_t node_count;
    uint32_t keep_alive_stable_id;
    uint32_t last_clicked_stable_id;
    uint32_t hit_test_stable_id;
    uint8_t keep_alive_enabled;
    uint8_t hit_test_enabled;
    egui_dim_t hit_test_min_x_offset;
    uint16_t bind_count;
    uint16_t save_count;
    uint16_t restore_count;
    uint16_t destroy_count;
    uint16_t button_click_count;
    test_virtual_stage_node_data_t nodes[TEST_VIRTUAL_STAGE_MAX_NODES];
};

struct test_virtual_stage_array_node_data
{
    egui_virtual_stage_node_desc_t desc;
    char label[16];
};

struct test_virtual_stage_array_context
{
    uint32_t last_clicked_stable_id;
    uint16_t bind_count;
    uint16_t destroy_count;
    uint16_t button_click_count;
};

static egui_view_virtual_stage_t test_page;
static test_virtual_stage_context_t test_context;
static egui_view_virtual_stage_array_adapter_t test_array_adapter;
static test_virtual_stage_array_context_t test_array_context;
static test_virtual_stage_array_node_data_t test_array_nodes[2];

EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SOURCE_INIT(test_array_node_source, test_array_nodes, test_virtual_stage_array_node_data_t, desc);

static test_virtual_stage_node_data_t *find_node_by_stable_id(uint32_t stable_id)
{
    uint32_t i;

    for (i = 0; i < test_context.node_count; i++)
    {
        if (test_context.nodes[i].desc.stable_id == stable_id)
        {
            return &test_context.nodes[i];
        }
    }

    return NULL;
}

static uint32_t adapter_get_count(void *adapter_context)
{
    return ((test_virtual_stage_context_t *)adapter_context)->node_count;
}

static uint8_t adapter_get_desc(void *adapter_context, uint32_t index, egui_virtual_stage_node_desc_t *out_desc)
{
    test_virtual_stage_context_t *ctx = (test_virtual_stage_context_t *)adapter_context;

    if (index >= ctx->node_count)
    {
        return 0;
    }

    *out_desc = ctx->nodes[index].desc;
    return 1;
}

static void button_click_cb(egui_view_t *self)
{
    egui_view_virtual_stage_entry_t entry;

    if (egui_view_virtual_stage_resolve_node_by_view(EGUI_VIEW_OF(&test_page), self, &entry))
    {
        test_context.last_clicked_stable_id = entry.stable_id;
    }
    test_context.button_click_count++;
}

static void array_button_click_cb(egui_view_t *self)
{
    uint32_t stable_id;

    if (egui_view_virtual_stage_resolve_stable_id_by_view(EGUI_VIEW_OF(&test_page), self, &stable_id))
    {
        test_array_context.last_clicked_stable_id = stable_id;
    }
    test_array_context.button_click_count++;
}

static egui_view_t *adapter_create_view(void *adapter_context, uint16_t view_type)
{
    EGUI_UNUSED(adapter_context);

    if (view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON)
    {
        egui_view_button_t *button = (egui_view_button_t *)egui_malloc(sizeof(egui_view_button_t));
        if (button == NULL)
        {
            return NULL;
        }

        egui_view_button_init(EGUI_VIEW_OF(button));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(button), button_click_cb);
        return EGUI_VIEW_OF(button);
    }

    if (view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT)
    {
        egui_view_textinput_t *textinput = (egui_view_textinput_t *)egui_malloc(sizeof(egui_view_textinput_t));
        if (textinput == NULL)
        {
            return NULL;
        }

        egui_view_textinput_init(EGUI_VIEW_OF(textinput));
        egui_view_textinput_set_font(EGUI_VIEW_OF(textinput), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
        return EGUI_VIEW_OF(textinput);
    }

    if (view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_LIST)
    {
        egui_view_list_t *list = (egui_view_list_t *)egui_malloc(sizeof(egui_view_list_t));
        if (list == NULL)
        {
            return NULL;
        }

        egui_view_list_init(EGUI_VIEW_OF(list));
        return EGUI_VIEW_OF(list);
    }

    if (view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_SPINNER)
    {
        egui_view_spinner_t *spinner = (egui_view_spinner_t *)egui_malloc(sizeof(egui_view_spinner_t));
        if (spinner == NULL)
        {
            return NULL;
        }

        egui_view_spinner_init(EGUI_VIEW_OF(spinner));
        egui_view_spinner_start(EGUI_VIEW_OF(spinner));
        return EGUI_VIEW_OF(spinner);
    }

    return NULL;
}

static void adapter_destroy_view(void *adapter_context, egui_view_t *view, uint16_t view_type)
{
    test_virtual_stage_context_t *ctx = (test_virtual_stage_context_t *)adapter_context;
    EGUI_UNUSED(view_type);
    ctx->destroy_count++;
    egui_free(view);
}

static egui_view_t *array_adapter_create_view(void *user_context, uint16_t view_type)
{
    EGUI_UNUSED(user_context);

    if (view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON)
    {
        egui_view_button_t *button = (egui_view_button_t *)egui_malloc(sizeof(egui_view_button_t));
        if (button == NULL)
        {
            return NULL;
        }

        egui_view_button_init(EGUI_VIEW_OF(button));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(button), array_button_click_cb);
        return EGUI_VIEW_OF(button);
    }

    return NULL;
}

static void array_adapter_destroy_view(void *user_context, egui_view_t *view, uint16_t view_type)
{
    test_virtual_stage_array_context_t *ctx = (test_virtual_stage_array_context_t *)user_context;
    EGUI_UNUSED(view_type);

    ctx->destroy_count++;
    egui_free(view);
}

static void adapter_bind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    test_virtual_stage_context_t *ctx = (test_virtual_stage_context_t *)adapter_context;
    test_virtual_stage_node_data_t *node = &ctx->nodes[index];
    EGUI_UNUSED(stable_id);

    ctx->bind_count++;
    if (node->desc.view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON)
    {
        egui_view_label_set_text(view, node->label);
        return;
    }

    if (node->desc.view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_LIST)
    {
        static const char *items[] = {"A", "B", "C", "D", "E", "F"};
        egui_view_list_t *list = (egui_view_list_t *)view;
        uint8_t i;

        egui_view_scroll_set_size(view, desc->region.size.width, desc->region.size.height);
        if (list->item_count == 0U)
        {
            for (i = 0; i < EGUI_ARRAY_SIZE(items); i++)
            {
                egui_view_list_add_item(view, items[i]);
            }
        }
        for (i = 0; i < list->item_count; i++)
        {
            egui_view_set_clickable(EGUI_VIEW_OF(&list->items[i]), 0);
        }
        egui_view_scroll_layout_childs(view);
    }
}

static void array_adapter_bind_view(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    test_virtual_stage_array_context_t *ctx = (test_virtual_stage_array_context_t *)user_context;

    EGUI_UNUSED(stable_id);
    EGUI_UNUSED(desc);

    ctx->bind_count++;
    egui_view_label_set_text(view, test_array_nodes[index].label);
}

static void adapter_save_state(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    test_virtual_stage_context_t *ctx = (test_virtual_stage_context_t *)adapter_context;
    test_virtual_stage_node_data_t *node = find_node_by_stable_id(stable_id);

    EGUI_UNUSED(index);

    if (node != NULL && desc != NULL && desc->view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT)
    {
        const char *text = egui_view_textinput_get_text(view);
        strncpy(node->text, text, sizeof(node->text) - 1);
        node->text[sizeof(node->text) - 1] = '\0';
        ctx->save_count++;
    }
}

static void adapter_restore_state(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    test_virtual_stage_context_t *ctx = (test_virtual_stage_context_t *)adapter_context;
    test_virtual_stage_node_data_t *node = find_node_by_stable_id(stable_id);

    EGUI_UNUSED(index);

    if (node != NULL && desc != NULL && desc->view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT)
    {
        egui_view_textinput_set_text(view, node->text);
        ctx->restore_count++;
    }
}

static uint8_t adapter_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id,
                                         const egui_virtual_stage_node_desc_t *desc)
{
    test_virtual_stage_context_t *ctx = (test_virtual_stage_context_t *)adapter_context;

    EGUI_UNUSED(view);
    EGUI_UNUSED(index);
    EGUI_UNUSED(desc);

    return ctx->keep_alive_enabled && ctx->keep_alive_stable_id == stable_id ? 1U : 0U;
}

static uint8_t adapter_hit_test(void *adapter_context, uint32_t index, const egui_virtual_stage_node_desc_t *desc, const egui_region_t *screen_region,
                                egui_dim_t screen_x, egui_dim_t screen_y)
{
    test_virtual_stage_context_t *ctx = (test_virtual_stage_context_t *)adapter_context;

    EGUI_UNUSED(index);

    if (!egui_region_pt_in_rect(screen_region, screen_x, screen_y))
    {
        return 0;
    }
    if (!ctx->hit_test_enabled || desc->stable_id != ctx->hit_test_stable_id)
    {
        return 1;
    }

    return screen_x >= (egui_dim_t)(screen_region->location.x + ctx->hit_test_min_x_offset) ? 1U : 0U;
}

static const egui_view_virtual_stage_adapter_t test_adapter = {
        .get_count = adapter_get_count,
        .get_desc = adapter_get_desc,
        .create_view = adapter_create_view,
        .destroy_view = adapter_destroy_view,
        .bind_view = adapter_bind_view,
        .unbind_view = NULL,
        .save_state = adapter_save_state,
        .restore_state = adapter_restore_state,
        .draw_node = NULL,
        .hit_test = adapter_hit_test,
        .should_keep_alive = adapter_should_keep_alive,
};

static const egui_view_virtual_stage_adapter_t test_pooling_adapter = {
        .get_count = adapter_get_count,
        .get_desc = adapter_get_desc,
        .create_view = adapter_create_view,
        .destroy_view = NULL,
        .bind_view = adapter_bind_view,
        .unbind_view = NULL,
        .save_state = adapter_save_state,
        .restore_state = adapter_restore_state,
        .draw_node = NULL,
        .hit_test = adapter_hit_test,
        .should_keep_alive = adapter_should_keep_alive,
};

static const egui_view_virtual_stage_array_ops_t test_array_ops = {
        .create_view = array_adapter_create_view,
        .destroy_view = array_adapter_destroy_view,
        .bind_view = array_adapter_bind_view,
        .unbind_view = NULL,
        .save_state = NULL,
        .restore_state = NULL,
        .draw_node = NULL,
        .hit_test = NULL,
        .should_keep_alive = NULL,
};

static const egui_view_virtual_stage_array_setup_t test_array_setup = {
        .params = NULL,
        .node_source = &test_array_node_source,
        .ops = &test_array_ops,
        .user_context = &test_array_context,
};

static void layout_page(void)
{
    EGUI_REGION_DEFINE(region, 0, 0, 160, 120);
    egui_view_layout(EGUI_VIEW_OF(&test_page), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_page)->region_screen, &region);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
}

static void collect_dirty_union(egui_region_t *out_region, uint8_t *out_count)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();
    uint8_t i;
    uint8_t count = 0;

    egui_region_init_empty(out_region);
    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if (egui_region_is_empty(&arr[i]))
        {
            continue;
        }

        if (count == 0)
        {
            egui_region_copy(out_region, &arr[i]);
        }
        else
        {
            egui_region_union(out_region, &arr[i], out_region);
        }
        count++;
    }

    if (out_count != NULL)
    {
        *out_count = count;
    }
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_page)->api->dispatch_touch_event(EGUI_VIEW_OF(&test_page), &event);
}

static const egui_view_virtual_stage_slot_t *find_slot_by_stable_id(uint32_t stable_id)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS; i++)
    {
        const egui_view_virtual_stage_slot_t *slot = egui_view_virtual_stage_get_slot(EGUI_VIEW_OF(&test_page), i);
        if (slot != NULL && slot->state != EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_UNUSED && slot->stable_id == stable_id)
        {
            return slot;
        }
    }

    return NULL;
}

static void reset_page_with_adapter(uint8_t live_slot_limit, const egui_view_virtual_stage_adapter_t *adapter, void *adapter_context)
{
    egui_focus_manager_clear_focus();
    memset(&test_page, 0, sizeof(test_page));
    memset(&test_context, 0, sizeof(test_context));
    egui_view_virtual_stage_init(EGUI_VIEW_OF(&test_page));
    egui_view_virtual_stage_set_live_slot_limit(EGUI_VIEW_OF(&test_page), live_slot_limit);
    egui_view_virtual_stage_set_adapter(EGUI_VIEW_OF(&test_page), adapter, adapter_context);
}

static void reset_page(uint8_t live_slot_limit)
{
    reset_page_with_adapter(live_slot_limit, &test_adapter, &test_context);
}

static void reset_page_with_array_adapter(uint8_t live_slot_limit)
{
    egui_focus_manager_clear_focus();
    memset(&test_page, 0, sizeof(test_page));
    memset(&test_context, 0, sizeof(test_context));
    memset(&test_array_context, 0, sizeof(test_array_context));
    memset(test_array_nodes, 0, sizeof(test_array_nodes));
    egui_view_virtual_stage_init(EGUI_VIEW_OF(&test_page));
    egui_view_virtual_stage_set_live_slot_limit(EGUI_VIEW_OF(&test_page), live_slot_limit);
    egui_view_virtual_stage_apply_array_setup(EGUI_VIEW_OF(&test_page), &test_array_adapter, &test_array_setup);
    egui_view_virtual_stage_set_live_slot_limit(EGUI_VIEW_OF(&test_page), live_slot_limit);
}

static void configure_button_node(uint32_t index, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, uint32_t stable_id, uint8_t flags,
                                  const char *label)
{
    test_virtual_stage_node_data_t *node = &test_context.nodes[index];

    memset(node, 0, sizeof(*node));
    node->desc.region.location.x = x;
    node->desc.region.location.y = y;
    node->desc.region.size.width = width;
    node->desc.region.size.height = height;
    node->desc.stable_id = stable_id;
    node->desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON;
    node->desc.flags = flags;
    strncpy(node->label, label, sizeof(node->label) - 1);
    node->label[sizeof(node->label) - 1] = '\0';
}

static void configure_button_node_in_context(test_virtual_stage_context_t *ctx, uint32_t index, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height,
                                             uint32_t stable_id, uint8_t flags, const char *label)
{
    test_virtual_stage_node_data_t *node = &ctx->nodes[index];

    memset(node, 0, sizeof(*node));
    node->desc.region.location.x = x;
    node->desc.region.location.y = y;
    node->desc.region.size.width = width;
    node->desc.region.size.height = height;
    node->desc.stable_id = stable_id;
    node->desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON;
    node->desc.flags = flags;
    strncpy(node->label, label, sizeof(node->label) - 1);
    node->label[sizeof(node->label) - 1] = '\0';
}

static void configure_textinput_node(uint32_t index, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, uint32_t stable_id, uint8_t flags)
{
    test_virtual_stage_node_data_t *node = &test_context.nodes[index];

    memset(node, 0, sizeof(*node));
    node->desc.region.location.x = x;
    node->desc.region.location.y = y;
    node->desc.region.size.width = width;
    node->desc.region.size.height = height;
    node->desc.stable_id = stable_id;
    node->desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT;
    node->desc.flags = flags;
}

static void configure_list_node(uint32_t index, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, uint32_t stable_id, uint8_t flags)
{
    test_virtual_stage_node_data_t *node = &test_context.nodes[index];

    memset(node, 0, sizeof(*node));
    node->desc.region.location.x = x;
    node->desc.region.location.y = y;
    node->desc.region.size.width = width;
    node->desc.region.size.height = height;
    node->desc.stable_id = stable_id;
    node->desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_LIST;
    node->desc.flags = flags;
}

static void configure_spinner_node(uint32_t index, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, uint32_t stable_id, uint8_t flags)
{
    test_virtual_stage_node_data_t *node = &test_context.nodes[index];

    memset(node, 0, sizeof(*node));
    node->desc.region.location.x = x;
    node->desc.region.location.y = y;
    node->desc.region.size.width = width;
    node->desc.region.size.height = height;
    node->desc.stable_id = stable_id;
    node->desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_SPINNER;
    node->desc.flags = flags;
}

static void configure_render_only_node(uint32_t index, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, uint32_t stable_id, int16_t z_order)
{
    test_virtual_stage_node_data_t *node = &test_context.nodes[index];

    memset(node, 0, sizeof(*node));
    node->desc.region.location.x = x;
    node->desc.region.location.y = y;
    node->desc.region.size.width = width;
    node->desc.region.size.height = height;
    node->desc.stable_id = stable_id;
    node->desc.view_type = EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE;
    node->desc.z_order = z_order;
}

static void configure_array_button_node(uint32_t index, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, uint32_t stable_id, uint8_t flags,
                                        const char *label)
{
    test_virtual_stage_array_node_data_t *node = &test_array_nodes[index];

    memset(node, 0, sizeof(*node));
    node->desc.region.location.x = x;
    node->desc.region.location.y = y;
    node->desc.region.size.width = width;
    node->desc.region.size.height = height;
    node->desc.stable_id = stable_id;
    node->desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON;
    node->desc.flags = flags;
    strncpy(node->label, label, sizeof(node->label) - 1);
    node->label[sizeof(node->label) - 1] = '\0';
}

static void configure_array_hidden_node(uint32_t index, uint32_t stable_id)
{
    test_virtual_stage_array_node_data_t *node = &test_array_nodes[index];

    memset(node, 0, sizeof(*node));
    node->desc.stable_id = stable_id;
    node->desc.flags = EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN;
}

static void test_virtual_stage_render_only_node_keeps_zero_slots(void)
{
    reset_page(2);

    test_context.node_count = 1;
    test_context.nodes[0].desc.region.location.x = 8;
    test_context.nodes[0].desc.region.location.y = 8;
    test_context.nodes[0].desc.region.size.width = 48;
    test_context.nodes[0].desc.region.size.height = 24;
    test_context.nodes[0].desc.stable_id = 100;
    test_context.nodes[0].desc.view_type = EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE;
    layout_page();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
}

static void test_virtual_stage_render_only_interactive_flag_does_not_consume_touch(void)
{
    reset_page(1);

    test_context.node_count = 1;
    test_context.nodes[0].desc.region.location.x = 8;
    test_context.nodes[0].desc.region.location.y = 8;
    test_context.nodes[0].desc.region.size.width = 48;
    test_context.nodes[0].desc.region.size.height = 24;
    test_context.nodes[0].desc.stable_id = 101;
    test_context.nodes[0].desc.view_type = EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE;
    test_context.nodes[0].desc.flags = EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE;
    layout_page();

    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 12, 12));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_context.button_click_count);
}

static void test_virtual_stage_touch_button_materializes_and_releases(void)
{
    reset_page(1);

    test_context.node_count = 1;
    test_context.nodes[0].desc.region.location.x = 10;
    test_context.nodes[0].desc.region.location.y = 10;
    test_context.nodes[0].desc.region.size.width = 64;
    test_context.nodes[0].desc.region.size.height = 28;
    test_context.nodes[0].desc.stable_id = 200;
    test_context.nodes[0].desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON;
    test_context.nodes[0].desc.flags = EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE;
    strcpy(test_context.nodes[0].label, "Tap");
    layout_page();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 20, 20));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 20, 20));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(1, test_context.button_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
}

static void test_virtual_stage_pin_and_unpin_respects_limit(void)
{
    reset_page(1);

    test_context.node_count = 2;

    test_context.nodes[0].desc.region.location.x = 8;
    test_context.nodes[0].desc.region.location.y = 8;
    test_context.nodes[0].desc.region.size.width = 48;
    test_context.nodes[0].desc.region.size.height = 24;
    test_context.nodes[0].desc.stable_id = 300;
    test_context.nodes[0].desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON;
    test_context.nodes[0].desc.flags = EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE;
    strcpy(test_context.nodes[0].label, "A");

    test_context.nodes[1].desc.region.location.x = 64;
    test_context.nodes[1].desc.region.location.y = 8;
    test_context.nodes[1].desc.region.size.width = 48;
    test_context.nodes[1].desc.region.size.height = 24;
    test_context.nodes[1].desc.stable_id = 301;
    test_context.nodes[1].desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON;
    test_context.nodes[1].desc.flags = EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE;
    strcpy(test_context.nodes[1].label, "B");

    layout_page();

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 300));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 301));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    egui_view_virtual_stage_unpin_node(EGUI_VIEW_OF(&test_page), 300);
    egui_view_virtual_stage_unpin_node(EGUI_VIEW_OF(&test_page), 301);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
}

static void test_virtual_stage_double_pin_requires_matching_unpin(void)
{
    reset_page(1);

    test_context.node_count = 1;
    test_context.nodes[0].desc.region.location.x = 12;
    test_context.nodes[0].desc.region.location.y = 12;
    test_context.nodes[0].desc.region.size.width = 64;
    test_context.nodes[0].desc.region.size.height = 28;
    test_context.nodes[0].desc.stable_id = 320;
    test_context.nodes[0].desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON;
    test_context.nodes[0].desc.flags = EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE;
    strcpy(test_context.nodes[0].label, "Pin");
    layout_page();

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 320));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 320));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    egui_view_virtual_stage_unpin_node(EGUI_VIEW_OF(&test_page), 320);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    egui_view_virtual_stage_unpin_node(EGUI_VIEW_OF(&test_page), 320);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
}

static void test_virtual_stage_textinput_save_and_restore_state(void)
{
    const egui_view_virtual_stage_slot_t *slot;

    reset_page(1);

    test_context.node_count = 1;
    test_context.nodes[0].desc.region.location.x = 10;
    test_context.nodes[0].desc.region.location.y = 10;
    test_context.nodes[0].desc.region.size.width = 100;
    test_context.nodes[0].desc.region.size.height = 28;
    test_context.nodes[0].desc.stable_id = 400;
    test_context.nodes[0].desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT;
    test_context.nodes[0].desc.flags = EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE;
    layout_page();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 20, 20));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 20, 20));
    slot = find_slot_by_stable_id(400);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    egui_view_textinput_set_text(slot->view, "abc");

    egui_focus_manager_clear_focus();
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_TRUE(test_context.save_count > 0);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 20, 20));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 20, 20));
    slot = find_slot_by_stable_id(400);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(strcmp(egui_view_textinput_get_text(slot->view), "abc") == 0);
    EGUI_TEST_ASSERT_TRUE(test_context.restore_count > 0);
}

static void test_virtual_stage_adapter_keep_alive_retains_slot_until_cleared(void)
{
    reset_page(1);

    test_context.node_count = 1;
    test_context.keep_alive_enabled = 1;
    test_context.keep_alive_stable_id = 500;
    test_context.nodes[0].desc.region.location.x = 16;
    test_context.nodes[0].desc.region.location.y = 16;
    test_context.nodes[0].desc.region.size.width = 72;
    test_context.nodes[0].desc.region.size.height = 28;
    test_context.nodes[0].desc.stable_id = 500;
    test_context.nodes[0].desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON;
    test_context.nodes[0].desc.flags = EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE;
    strcpy(test_context.nodes[0].label, "Keep");
    layout_page();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 24, 24));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 24, 24));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    test_context.keep_alive_enabled = 0;
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
}

static void test_virtual_stage_over_limit_keep_alive_slots_do_not_thrash_on_idle_layout(void)
{
    uint16_t bind_count_after_first_layout;
    uint16_t destroy_count_after_first_layout;
    uint32_t kept_stable_id = 0;

    reset_page(1);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 510, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_button_node(1, 72, 8, 56, 24, 511, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "B");
    layout_page();

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    if (find_slot_by_stable_id(510) != NULL)
    {
        kept_stable_id = 510;
    }
    else
    {
        EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(511));
        kept_stable_id = 511;
    }

    bind_count_after_first_layout = test_context.bind_count;
    destroy_count_after_first_layout = test_context.destroy_count;

    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(kept_stable_id));
    EGUI_TEST_ASSERT_EQUAL_INT(bind_count_after_first_layout, test_context.bind_count);
    EGUI_TEST_ASSERT_EQUAL_INT(destroy_count_after_first_layout, test_context.destroy_count);
}

static void test_virtual_stage_notify_data_changed_releases_removed_slot_and_rebinds_remaining_node(void)
{
    reset_page(2);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 520, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_button_node(1, 72, 8, 56, 24, 521, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "B");
    layout_page();

    EGUI_TEST_ASSERT_EQUAL_INT(2, test_context.bind_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    test_context.nodes[0] = test_context.nodes[1];
    memset(&test_context.nodes[1], 0, sizeof(test_context.nodes[1]));
    test_context.node_count = 1;
    egui_view_virtual_stage_notify_data_changed(EGUI_VIEW_OF(&test_page));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(520));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(521));
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_context.bind_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_context.destroy_count);
}

static void test_virtual_stage_notify_data_changed_prunes_removed_pin_state(void)
{
    reset_page(2);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 522, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_button_node(1, 72, 8, 56, 24, 523, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "B");
    layout_page();

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 522));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 522));

    test_context.nodes[0] = test_context.nodes[1];
    memset(&test_context.nodes[1], 0, sizeof(test_context.nodes[1]));
    test_context.node_count = 1;
    egui_view_virtual_stage_notify_data_changed(EGUI_VIEW_OF(&test_page));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 522));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 999999));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(522));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(523));
}

static void test_virtual_stage_notify_data_changed_reuses_cache_capacity_without_heap_churn(void)
{
    egui_port_alloc_stats_t stats;
    void *node_cache_before;
    uint32_t *draw_order_before;

    reset_page(2);

    test_context.node_count = 2;
    configure_render_only_node(0, 8, 8, 40, 20, 5241, 0);
    configure_render_only_node(1, 56, 8, 40, 20, 5242, 1);

    egui_port_reset_alloc_stats();
    layout_page();
    egui_port_get_alloc_stats(&stats);

    EGUI_TEST_ASSERT_EQUAL_INT(2, stats.alloc_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, stats.free_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_page.cached_capacity);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_page.cached_node_count);

    node_cache_before = test_page.node_cache;
    draw_order_before = test_page.draw_order;

    egui_port_reset_alloc_stats();
    egui_view_virtual_stage_notify_data_changed(EGUI_VIEW_OF(&test_page));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    egui_port_get_alloc_stats(&stats);

    EGUI_TEST_ASSERT_EQUAL_INT(0, stats.alloc_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, stats.free_count);
    EGUI_TEST_ASSERT_TRUE(test_page.node_cache == node_cache_before);
    EGUI_TEST_ASSERT_TRUE(test_page.draw_order == draw_order_before);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_page.cached_capacity);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_page.cached_node_count);

    test_context.node_count = 3;
    configure_render_only_node(2, 104, 8, 40, 20, 5243, 2);

    egui_port_reset_alloc_stats();
    egui_view_virtual_stage_notify_data_changed(EGUI_VIEW_OF(&test_page));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    egui_port_get_alloc_stats(&stats);

    EGUI_TEST_ASSERT_EQUAL_INT(2, stats.alloc_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, stats.free_count);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_page.cached_capacity);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_page.cached_node_count);

    node_cache_before = test_page.node_cache;
    draw_order_before = test_page.draw_order;

    test_context.node_count = 2;
    egui_port_reset_alloc_stats();
    egui_view_virtual_stage_notify_data_changed(EGUI_VIEW_OF(&test_page));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    egui_port_get_alloc_stats(&stats);

    EGUI_TEST_ASSERT_EQUAL_INT(0, stats.alloc_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, stats.free_count);
    EGUI_TEST_ASSERT_TRUE(test_page.node_cache == node_cache_before);
    EGUI_TEST_ASSERT_TRUE(test_page.draw_order == draw_order_before);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_page.cached_capacity);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_page.cached_node_count);
}

static void test_virtual_stage_reduce_live_slot_limit_trims_oldest_keepalive_slot(void)
{
    uint16_t bind_count_after_trim;
    uint16_t destroy_count_after_trim;

    reset_page(2);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 530, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_button_node(1, 72, 8, 56, 24, 531, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "B");
    layout_page();

    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    egui_view_virtual_stage_set_live_slot_limit(EGUI_VIEW_OF(&test_page), 1);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(530));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(531));

    bind_count_after_trim = test_context.bind_count;
    destroy_count_after_trim = test_context.destroy_count;
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(530));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(531));
    EGUI_TEST_ASSERT_EQUAL_INT(bind_count_after_trim, test_context.bind_count);
    EGUI_TEST_ASSERT_EQUAL_INT(destroy_count_after_trim, test_context.destroy_count);
}

static void test_virtual_stage_reduce_live_slot_limit_to_zero_releases_captured_keepalive_after_up(void)
{
    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 535, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "Hold");
    layout_page();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 16, 16));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(535));

    egui_view_virtual_stage_set_live_slot_limit(EGUI_VIEW_OF(&test_page), 0);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 16, 16));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(535));
}

static void test_virtual_stage_set_adapter_clears_old_slots_and_pin_state(void)
{
    test_virtual_stage_context_t other_context;

    reset_page(2);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 540, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_button_node(1, 72, 8, 56, 24, 541, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "B");
    layout_page();
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 540));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    memset(&other_context, 0, sizeof(other_context));
    other_context.node_count = 1;
    configure_button_node_in_context(&other_context, 0, 12, 12, 72, 28, 542, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "C");

    egui_view_virtual_stage_set_adapter(EGUI_VIEW_OF(&test_page), &test_adapter, &other_context);

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_get_adapter_context(EGUI_VIEW_OF(&test_page)) == &other_context);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 540));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_context.destroy_count);

    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(540));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(542));
    EGUI_TEST_ASSERT_EQUAL_INT(1, other_context.bind_count);
}

static void test_virtual_stage_notify_node_changed_rebinds_only_target_slot(void)
{
    reset_page(2);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 600, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_button_node(1, 72, 8, 56, 24, 601, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "B");
    layout_page();

    EGUI_TEST_ASSERT_EQUAL_INT(2, test_context.bind_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    strcpy(test_context.nodes[0].label, "A+");
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 600);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(3, test_context.bind_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_context.destroy_count);
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(600));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(601));
}

static void test_virtual_stage_notify_bounds_changed_updates_only_target_slot_region(void)
{
    const egui_view_virtual_stage_slot_t *slot_a;
    const egui_view_virtual_stage_slot_t *slot_b;

    reset_page(2);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 610, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_button_node(1, 72, 8, 56, 24, 611, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "B");
    layout_page();

    test_context.nodes[0].desc.region.location.x = 16;
    test_context.nodes[0].desc.region.location.y = 20;
    test_context.nodes[0].desc.region.size.width = 64;
    test_context.nodes[0].desc.region.size.height = 28;
    egui_view_virtual_stage_notify_node_bounds_changed(EGUI_VIEW_OF(&test_page), 610);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    slot_a = find_slot_by_stable_id(610);
    slot_b = find_slot_by_stable_id(611);
    EGUI_TEST_ASSERT_NOT_NULL(slot_a);
    EGUI_TEST_ASSERT_NOT_NULL(slot_b);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_context.bind_count);
    EGUI_TEST_ASSERT_EQUAL_INT(16, slot_a->render_region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(20, slot_a->render_region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(64, slot_a->render_region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(28, slot_a->render_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(72, slot_b->render_region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(8, slot_b->render_region.location.y);
}

static void test_virtual_stage_notify_bounds_changed_invalidates_old_and_new_region(void)
{
    egui_region_t dirty_union;
    egui_region_t expected_union;
    uint8_t dirty_count = 0;

    reset_page(2);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 6100, 0, "Move");
    layout_page();

    egui_core_clear_region_dirty();
    test_context.nodes[0].desc.region.location.x = 24;
    egui_view_virtual_stage_notify_node_bounds_changed(EGUI_VIEW_OF(&test_page), 6100);

    collect_dirty_union(&dirty_union, &dirty_count);
    egui_region_init(&expected_union, 8, 8, 72, 24);
    EGUI_TEST_ASSERT_TRUE(dirty_count > 0);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected_union, &dirty_union);
}

static void test_virtual_stage_notify_bounds_changed_materializes_keepalive_node_when_becoming_visible(void)
{
    const egui_view_virtual_stage_slot_t *slot;

    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 0, 0, 612, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "Grow");
    layout_page();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    test_context.nodes[0].desc.region.location.x = 20;
    test_context.nodes[0].desc.region.location.y = 18;
    test_context.nodes[0].desc.region.size.width = 60;
    test_context.nodes[0].desc.region.size.height = 26;
    egui_view_virtual_stage_notify_node_bounds_changed(EGUI_VIEW_OF(&test_page), 612);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    slot = find_slot_by_stable_id(612);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_EQUAL_INT(20, slot->render_region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(18, slot->render_region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(60, slot->render_region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(26, slot->render_region.size.height);
}

static void test_virtual_stage_notify_node_changed_visible_to_hidden_invalidates_old_region_only(void)
{
    egui_region_t dirty_union;
    egui_region_t expected_region;
    uint8_t dirty_count = 0;

    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 6180, 0, "Hide");
    layout_page();

    egui_core_clear_region_dirty();
    test_context.nodes[0].desc.flags |= EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN;
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 6180);

    collect_dirty_union(&dirty_union, &dirty_count);
    egui_region_init(&expected_region, 8, 8, 56, 24);
    EGUI_TEST_ASSERT_TRUE(dirty_count > 0);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected_region, &dirty_union);
}

static void test_virtual_stage_hidden_node_releases_only_target_slot(void)
{
    reset_page(2);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 620, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_button_node(1, 72, 8, 56, 24, 621, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "B");
    layout_page();

    EGUI_TEST_ASSERT_EQUAL_INT(2, test_context.bind_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    test_context.nodes[0].desc.flags |= EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN;
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 620);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(2, test_context.bind_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(620));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(621));
}

static void test_virtual_stage_notify_node_changed_hidden_to_visible_invalidates_new_region_only(void)
{
    egui_region_t dirty_union;
    egui_region_t expected_region;
    uint8_t dirty_count = 0;

    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 6240, EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN, "Show");
    layout_page();

    egui_core_clear_region_dirty();
    test_context.nodes[0].desc.flags &= (uint8_t)(~EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN);
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 6240);

    collect_dirty_union(&dirty_union, &dirty_count);
    egui_region_init(&expected_region, 8, 8, 56, 24);
    EGUI_TEST_ASSERT_TRUE(dirty_count > 0);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected_region, &dirty_union);
}

static void test_virtual_stage_notify_node_changed_materializes_new_keepalive_node_from_idle(void)
{
    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 625, 0, "Idle");
    layout_page();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    test_context.nodes[0].desc.flags |= EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE;
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 625);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(625));
}

static void test_virtual_stage_notify_node_changed_restores_pinned_node_after_unhide(void)
{
    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 626, 0, "Pin");
    layout_page();

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 626));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(626));

    test_context.nodes[0].desc.flags |= EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN;
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 626);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(626));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 626));

    test_context.nodes[0].desc.flags &= (uint8_t)(~EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN);
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 626);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(626));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 626));
}

static void test_virtual_stage_focus_slot_replaces_pinned_slot_and_restores_after_blur(void)
{
    reset_page(1);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 630, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "Pin");
    configure_textinput_node(1, 8, 44, 96, 28, 631, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    layout_page();

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 630));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(630));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 16, 52));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 16, 52));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(630));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(631));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    egui_focus_manager_clear_focus();
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(630));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(631));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
}

static void test_virtual_stage_same_type_eviction_prefers_button_slot_over_textinput_slot(void)
{
    reset_page(2);

    test_context.node_count = 3;
    configure_button_node(0, 8, 8, 56, 24, 640, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_textinput_node(1, 8, 44, 96, 28, 641, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE);
    configure_button_node(2, 72, 8, 56, 24, 642, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "B");
    layout_page();

    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(640));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(641));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 80, 16));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(640));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(641));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(642));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 80, 16));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(640));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(641));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(642));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
}

static void test_virtual_stage_overlap_hit_prefers_higher_z_order_node(void)
{
    reset_page(1);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 645, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "Low");
    configure_button_node(1, 8, 8, 56, 24, 646, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "High");
    test_context.nodes[0].desc.z_order = 0;
    test_context.nodes[1].desc.z_order = 1;
    layout_page();

    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(645));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(646));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 16, 16));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(645));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(646));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 16, 16));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_context.button_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(646, test_context.last_clicked_stable_id);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));

    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(645));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(646));
}

static void test_virtual_stage_notify_node_changed_updates_touch_target_after_z_order_change(void)
{
    reset_page(2);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 6450, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "Low");
    configure_button_node(1, 8, 8, 56, 24, 6451, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "High");
    test_context.nodes[0].desc.z_order = 1;
    test_context.nodes[1].desc.z_order = 0;
    layout_page();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 16, 16));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 16, 16));
    EGUI_TEST_ASSERT_EQUAL_INT(6450, test_context.last_clicked_stable_id);

    test_context.button_click_count = 0;
    test_context.last_clicked_stable_id = 0;
    test_context.nodes[1].desc.z_order = 2;
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 6451);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 16, 16));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 16, 16));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_context.button_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(6451, test_context.last_clicked_stable_id);
}

static void test_virtual_stage_custom_hit_test_falls_through_to_lower_node(void)
{
    reset_page(1);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 647, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "Low");
    configure_button_node(1, 8, 8, 56, 24, 648, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "High");
    test_context.nodes[0].desc.z_order = 0;
    test_context.nodes[1].desc.z_order = 1;
    test_context.hit_test_enabled = 1;
    test_context.hit_test_stable_id = 648;
    test_context.hit_test_min_x_offset = 28;
    layout_page();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 16, 16));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(647));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(648));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 16, 16));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_context.button_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(647, test_context.last_clicked_stable_id);

    test_context.hit_test_enabled = 0;
}

static void test_virtual_stage_notify_node_changed_noninteractive_during_capture_cancels_click(void)
{
    const egui_view_virtual_stage_slot_t *slot;

    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 6475, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "Tap");
    layout_page();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 16, 16));
    slot = find_slot_by_stable_id(6475);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_pressed(slot->view));

    test_context.nodes[0].desc.flags &= (uint8_t)(~EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 6475);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 16, 16));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_context.button_click_count);
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(6475));
}

static void test_virtual_stage_notify_data_changed_during_capture_cancels_pooled_view_before_rebind(void)
{
    const egui_view_virtual_stage_slot_t *slot;

    reset_page_with_adapter(1, &test_pooling_adapter, &test_context);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 6476, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "Old");
    layout_page();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 16, 16));
    slot = find_slot_by_stable_id(6476);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_pressed(slot->view));

    configure_button_node(0, 8, 8, 56, 24, 6477, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "New");
    egui_view_virtual_stage_notify_data_changed(EGUI_VIEW_OF(&test_page));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    slot = find_slot_by_stable_id(6477);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_FALSE(egui_view_get_pressed(slot->view));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 16, 16));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_context.button_click_count);
}

static void test_virtual_stage_custom_hit_test_followup_move_outside_cancels_click(void)
{
    const egui_view_virtual_stage_slot_t *slot;

    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 6480, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "Edge");
    test_context.hit_test_enabled = 1;
    test_context.hit_test_stable_id = 6480;
    test_context.hit_test_min_x_offset = 28;
    layout_page();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 40, 16));
    slot = find_slot_by_stable_id(6480);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_pressed(slot->view));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 16, 16));
    slot = find_slot_by_stable_id(6480);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_FALSE(egui_view_get_pressed(slot->view));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 16, 16));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_context.button_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_context.last_clicked_stable_id);

    test_context.hit_test_enabled = 0;
}

static void test_virtual_stage_custom_hit_test_followup_move_back_inside_restores_click(void)
{
    const egui_view_virtual_stage_slot_t *slot;

    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 6481, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "Edge");
    test_context.hit_test_enabled = 1;
    test_context.hit_test_stable_id = 6481;
    test_context.hit_test_min_x_offset = 28;
    layout_page();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 40, 16));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 16, 16));
    slot = find_slot_by_stable_id(6481);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_FALSE(egui_view_get_pressed(slot->view));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 40, 16));
    slot = find_slot_by_stable_id(6481);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_pressed(slot->view));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 40, 16));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_context.button_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(6481, test_context.last_clicked_stable_id);

    test_context.hit_test_enabled = 0;
}

static void test_virtual_stage_notify_node_changed_recreates_view_when_type_changes(void)
{
    const egui_view_virtual_stage_slot_t *slot;

    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 80, 28, 649, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "Swap");
    layout_page();

    slot = find_slot_by_stable_id(649);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON, slot->view_type);

    strcpy(test_context.nodes[0].text, "restore-me");
    test_context.nodes[0].desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT;
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 649);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    slot = find_slot_by_stable_id(649);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT, slot->view_type);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_context.destroy_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_context.restore_count);
    EGUI_TEST_ASSERT_TRUE(strcmp(egui_view_textinput_get_text(slot->view), "restore-me") == 0);
}

static void test_virtual_stage_notify_node_changed_restores_keepalive_textinput_after_render_only_round_trip(void)
{
    const egui_view_virtual_stage_slot_t *slot;
    uint16_t save_count_before_hide;
    uint16_t restore_count_before_show;

    reset_page(1);

    test_context.node_count = 1;
    configure_textinput_node(0, 8, 8, 96, 28, 6490, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE);
    layout_page();

    slot = find_slot_by_stable_id(6490);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    egui_view_textinput_set_text(slot->view, "abc");

    save_count_before_hide = test_context.save_count;
    restore_count_before_show = test_context.restore_count;

    test_context.nodes[0].desc.view_type = EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE;
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 6490);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(6490));
    EGUI_TEST_ASSERT_EQUAL_INT(save_count_before_hide + 1, test_context.save_count);

    test_context.nodes[0].desc.view_type = TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT;
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 6490);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    slot = find_slot_by_stable_id(6490);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT, slot->view_type);
    EGUI_TEST_ASSERT_EQUAL_INT(restore_count_before_show + 1, test_context.restore_count);
    EGUI_TEST_ASSERT_TRUE(strcmp(egui_view_textinput_get_text(slot->view), "abc") == 0);
}

static void test_virtual_stage_timer_view_stops_on_detach_and_restarts_on_reattach(void)
{
    const egui_view_virtual_stage_slot_t *slot;
    egui_view_t *spinner_view;
    egui_view_spinner_t *spinner;

    reset_page_with_adapter(1, &test_pooling_adapter, &test_context);
    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&test_page));

    test_context.node_count = 1;
    configure_spinner_node(0, 8, 8, 36, 36, 6492, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE);
    layout_page();

    slot = find_slot_by_stable_id(6492);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_VIRTUAL_STAGE_VIEW_TYPE_SPINNER, slot->view_type);
    spinner_view = slot->view;
    spinner = (egui_view_spinner_t *)spinner_view;
    EGUI_TEST_ASSERT_TRUE(spinner_view->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&spinner->spin_timer));

    test_context.nodes[0].desc.flags = EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE;
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 6492);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(6492));
    EGUI_TEST_ASSERT_FALSE(spinner_view->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&spinner->spin_timer));

    test_context.nodes[0].desc.flags = EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE;
    egui_view_virtual_stage_notify_node_changed(EGUI_VIEW_OF(&test_page), 6492);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    slot = find_slot_by_stable_id(6492);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(slot->view == spinner_view);
    EGUI_TEST_ASSERT_TRUE(spinner_view->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&spinner->spin_timer));

    egui_view_dispatch_detach_from_window(EGUI_VIEW_OF(&test_page));
}

static void test_virtual_stage_nested_list_drag_keeps_internal_touch_capture(void)
{
    const egui_view_virtual_stage_slot_t *slot;
    egui_view_t *container;

    reset_page(1);
    test_context.keep_alive_enabled = 1U;
    test_context.keep_alive_stable_id = 651U;
    test_context.node_count = 1;
    configure_list_node(0, 8, 8, 64, 48, 651U, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE);
    layout_page();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 24, 44));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 24, 20));

    slot = find_slot_by_stable_id(651U);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_NOT_NULL(slot->view);

    container = EGUI_VIEW_OF(&((egui_view_list_t *)slot->view)->base.container);
    EGUI_TEST_ASSERT_TRUE(container->region.location.y < 0);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 24, 20));
}

static void test_virtual_stage_array_adapter_bridge_exposes_count_and_desc(void)
{
    egui_virtual_stage_node_desc_t desc;

    reset_page_with_array_adapter(2);
    configure_array_button_node(0, 8, 8, 64, 28, 6496, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_array_button_node(1, 72, 8, 64, 28, 6497, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "B");

    EGUI_TEST_ASSERT_EQUAL_INT(2, test_array_adapter.adapter.get_count(&test_array_adapter));
    EGUI_TEST_ASSERT_TRUE(test_array_adapter.adapter.get_desc(&test_array_adapter, 1, &desc));
    EGUI_TEST_ASSERT_EQUAL_INT(6497, desc.stable_id);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON, desc.view_type);
    EGUI_TEST_ASSERT_FALSE(test_array_adapter.adapter.get_desc(&test_array_adapter, 2, &desc));
}

static void test_virtual_stage_array_adapter_default_hit_test_falls_back_to_rect(void)
{
    reset_page_with_array_adapter(1);
    configure_array_button_node(0, 10, 10, 64, 28, 6498, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "Tap");
    configure_array_hidden_node(1, 6499);
    layout_page();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 22, 20));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 22, 20));

    EGUI_TEST_ASSERT_EQUAL_INT(1, test_array_context.bind_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_array_context.button_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(6498, test_array_context.last_clicked_stable_id);
}

static void test_virtual_stage_init_with_setup_applies_params_and_adapter(void)
{
    static const egui_view_virtual_stage_params_t setup_params = {
            .region = {{4, 6}, {132, 88}},
            .live_slot_limit = 2,
    };
    static const egui_view_virtual_stage_setup_t setup = {
            .params = &setup_params,
            .adapter = &test_adapter,
            .adapter_context = &test_context,
    };

    memset(&test_page, 0, sizeof(test_page));
    memset(&test_context, 0, sizeof(test_context));
    egui_view_virtual_stage_init_with_setup(EGUI_VIEW_OF(&test_page), &setup);

    EGUI_TEST_ASSERT_EQUAL_INT(4, EGUI_VIEW_OF(&test_page)->region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(6, EGUI_VIEW_OF(&test_page)->region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(132, EGUI_VIEW_OF(&test_page)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(88, EGUI_VIEW_OF(&test_page)->region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_stage_get_live_slot_limit(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_get_adapter(EGUI_VIEW_OF(&test_page)) == &test_adapter);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_get_adapter_context(EGUI_VIEW_OF(&test_page)) == &test_context);
}

static void test_virtual_stage_init_with_array_setup_applies_params_and_bridge_adapter(void)
{
    static const egui_view_virtual_stage_params_t setup_params = {
            .region = {{6, 8}, {128, 84}},
            .live_slot_limit = 3,
    };
    static const egui_view_virtual_stage_array_setup_t setup = {
            .params = &setup_params,
            .node_source = &test_array_node_source,
            .ops = &test_array_ops,
            .user_context = &test_array_context,
    };

    memset(&test_page, 0, sizeof(test_page));
    memset(&test_array_adapter, 0, sizeof(test_array_adapter));
    memset(&test_array_context, 0, sizeof(test_array_context));
    memset(test_array_nodes, 0, sizeof(test_array_nodes));

    egui_view_virtual_stage_init_with_array_setup(EGUI_VIEW_OF(&test_page), &test_array_adapter, &setup);

    EGUI_TEST_ASSERT_EQUAL_INT(6, EGUI_VIEW_OF(&test_page)->region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(8, EGUI_VIEW_OF(&test_page)->region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(128, EGUI_VIEW_OF(&test_page)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(84, EGUI_VIEW_OF(&test_page)->region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_virtual_stage_get_live_slot_limit(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_get_adapter(EGUI_VIEW_OF(&test_page)) == &test_array_adapter.adapter);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_get_adapter_context(EGUI_VIEW_OF(&test_page)) == &test_array_adapter);
}

static void test_virtual_stage_public_find_helpers_resolve_live_slot(void)
{
    const egui_view_virtual_stage_slot_t *slot;
    egui_view_t *view;

    reset_page(2);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 64, 28, 650, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "Find");
    layout_page();

    slot = egui_view_virtual_stage_find_slot_by_stable_id(EGUI_VIEW_OF(&test_page), 650);
    view = egui_view_virtual_stage_find_view_by_stable_id(EGUI_VIEW_OF(&test_page), 650);

    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_NOT_NULL(view);
    EGUI_TEST_ASSERT_TRUE(slot->view == view);
    EGUI_TEST_ASSERT_EQUAL_INT(650, slot->stable_id);
}

static void test_virtual_stage_public_resolve_node_by_view_returns_entry(void)
{
    const egui_view_virtual_stage_slot_t *slot;
    egui_view_virtual_stage_entry_t entry;

    reset_page(2);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 64, 28, 660, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "Node");
    layout_page();

    slot = egui_view_virtual_stage_find_slot_by_stable_id(EGUI_VIEW_OF(&test_page), 660);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_resolve_node_by_view(EGUI_VIEW_OF(&test_page), slot->view, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(0, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(660, entry.stable_id);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON, entry.desc.view_type);
}

static void test_virtual_stage_public_resolve_stable_id_by_view_returns_stable_id(void)
{
    const egui_view_virtual_stage_slot_t *slot;
    uint32_t stable_id = 0;

    reset_page(2);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 64, 28, 665, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "Node");
    layout_page();

    slot = egui_view_virtual_stage_find_slot_by_stable_id(EGUI_VIEW_OF(&test_page), 665);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_resolve_stable_id_by_view(EGUI_VIEW_OF(&test_page), slot->view, &stable_id));
    EGUI_TEST_ASSERT_EQUAL_INT(665, stable_id);
}

static void test_virtual_stage_public_is_node_pinned_tracks_pin_state(void)
{
    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 670, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "Pin");
    layout_page();

    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 670));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 670));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 670));
    egui_view_virtual_stage_unpin_node(EGUI_VIEW_OF(&test_page), 670);
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 670));
}

static void test_virtual_stage_public_toggle_pin_node_toggles_pin_state(void)
{
    reset_page(1);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 56, 24, 675, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, "Pin");
    layout_page();

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_toggle_pin_node(EGUI_VIEW_OF(&test_page), 675));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 675));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_toggle_pin_node(EGUI_VIEW_OF(&test_page), 675));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 675));
}

static void test_virtual_stage_notify_nodes_changed_rebinds_each_target_slot(void)
{
    const egui_view_virtual_stage_slot_t *slot_a;
    const egui_view_virtual_stage_slot_t *slot_b;
    const uint32_t changed_ids[] = {676, 677};
    uint16_t bind_count_before;

    reset_page(2);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 676, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_button_node(1, 72, 8, 56, 24, 677, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "B");
    layout_page();

    strcpy(test_context.nodes[0].label, "A+");
    strcpy(test_context.nodes[1].label, "B+");
    bind_count_before = test_context.bind_count;

    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES_CHANGED(EGUI_VIEW_OF(&test_page), changed_ids);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    slot_a = find_slot_by_stable_id(676);
    slot_b = find_slot_by_stable_id(677);
    EGUI_TEST_ASSERT_NOT_NULL(slot_a);
    EGUI_TEST_ASSERT_NOT_NULL(slot_b);
    EGUI_TEST_ASSERT_EQUAL_INT(bind_count_before + 2, test_context.bind_count);
    EGUI_TEST_ASSERT_TRUE(strcmp(((egui_view_button_t *)slot_a->view)->base.text, "A+") == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp(((egui_view_button_t *)slot_b->view)->base.text, "B+") == 0);
}

static void test_virtual_stage_notify_nodes_bounds_changed_updates_each_target_slot_region(void)
{
    const egui_view_virtual_stage_slot_t *slot_a;
    const egui_view_virtual_stage_slot_t *slot_b;
    const uint32_t changed_ids[] = {678, 679};

    reset_page(2);

    test_context.node_count = 2;
    configure_button_node(0, 8, 8, 56, 24, 678, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "A");
    configure_button_node(1, 72, 8, 56, 24, 679, EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE, "B");
    layout_page();

    test_context.nodes[0].desc.region.location.x = 16;
    test_context.nodes[0].desc.region.location.y = 20;
    test_context.nodes[0].desc.region.size.width = 64;
    test_context.nodes[0].desc.region.size.height = 28;
    test_context.nodes[1].desc.region.location.x = 84;
    test_context.nodes[1].desc.region.location.y = 16;
    test_context.nodes[1].desc.region.size.width = 52;
    test_context.nodes[1].desc.region.size.height = 20;

    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE_BOUNDS_CHANGED(EGUI_VIEW_OF(&test_page), changed_ids);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    slot_a = find_slot_by_stable_id(678);
    slot_b = find_slot_by_stable_id(679);
    EGUI_TEST_ASSERT_NOT_NULL(slot_a);
    EGUI_TEST_ASSERT_NOT_NULL(slot_b);
    EGUI_TEST_ASSERT_EQUAL_INT(16, slot_a->render_region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(20, slot_a->render_region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(64, slot_a->render_region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(28, slot_a->render_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(84, slot_b->render_region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(16, slot_b->render_region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(52, slot_b->render_region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(20, slot_b->render_region.size.height);
}

static void test_virtual_stage_pin_node_rejects_render_only_and_missing_node(void)
{
    reset_page(2);

    test_context.node_count = 1;
    test_context.nodes[0].desc.region.location.x = 8;
    test_context.nodes[0].desc.region.location.y = 8;
    test_context.nodes[0].desc.region.size.width = 64;
    test_context.nodes[0].desc.region.size.height = 28;
    test_context.nodes[0].desc.stable_id = 680;
    test_context.nodes[0].desc.view_type = EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE;
    layout_page();

    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 680));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 680));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 681));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 681));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
}

static void test_virtual_stage_pin_node_rejects_hidden_node(void)
{
    reset_page(2);

    test_context.node_count = 1;
    configure_button_node(0, 8, 8, 64, 28, 690, EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN, "Hide");
    layout_page();

    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_pin_node(EGUI_VIEW_OF(&test_page), 690));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_stage_is_node_pinned(EGUI_VIEW_OF(&test_page), 690));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_stage_get_slot_count(EGUI_VIEW_OF(&test_page)));
}

void test_virtual_stage_run(void)
{
    EGUI_TEST_SUITE_BEGIN(virtual_stage);
    EGUI_TEST_RUN(test_virtual_stage_render_only_node_keeps_zero_slots);
    EGUI_TEST_RUN(test_virtual_stage_render_only_interactive_flag_does_not_consume_touch);
    EGUI_TEST_RUN(test_virtual_stage_touch_button_materializes_and_releases);
    EGUI_TEST_RUN(test_virtual_stage_pin_and_unpin_respects_limit);
    EGUI_TEST_RUN(test_virtual_stage_double_pin_requires_matching_unpin);
    EGUI_TEST_RUN(test_virtual_stage_textinput_save_and_restore_state);
    EGUI_TEST_RUN(test_virtual_stage_nested_list_drag_keeps_internal_touch_capture);
    EGUI_TEST_RUN(test_virtual_stage_adapter_keep_alive_retains_slot_until_cleared);
    EGUI_TEST_RUN(test_virtual_stage_over_limit_keep_alive_slots_do_not_thrash_on_idle_layout);
    EGUI_TEST_RUN(test_virtual_stage_notify_data_changed_releases_removed_slot_and_rebinds_remaining_node);
    EGUI_TEST_RUN(test_virtual_stage_notify_data_changed_prunes_removed_pin_state);
    EGUI_TEST_RUN(test_virtual_stage_notify_data_changed_reuses_cache_capacity_without_heap_churn);
    EGUI_TEST_RUN(test_virtual_stage_reduce_live_slot_limit_trims_oldest_keepalive_slot);
    EGUI_TEST_RUN(test_virtual_stage_reduce_live_slot_limit_to_zero_releases_captured_keepalive_after_up);
    EGUI_TEST_RUN(test_virtual_stage_set_adapter_clears_old_slots_and_pin_state);
    EGUI_TEST_RUN(test_virtual_stage_notify_node_changed_rebinds_only_target_slot);
    EGUI_TEST_RUN(test_virtual_stage_notify_bounds_changed_updates_only_target_slot_region);
    EGUI_TEST_RUN(test_virtual_stage_notify_bounds_changed_invalidates_old_and_new_region);
    EGUI_TEST_RUN(test_virtual_stage_notify_bounds_changed_materializes_keepalive_node_when_becoming_visible);
    EGUI_TEST_RUN(test_virtual_stage_notify_node_changed_visible_to_hidden_invalidates_old_region_only);
    EGUI_TEST_RUN(test_virtual_stage_hidden_node_releases_only_target_slot);
    EGUI_TEST_RUN(test_virtual_stage_notify_node_changed_hidden_to_visible_invalidates_new_region_only);
    EGUI_TEST_RUN(test_virtual_stage_notify_node_changed_materializes_new_keepalive_node_from_idle);
    EGUI_TEST_RUN(test_virtual_stage_notify_node_changed_restores_pinned_node_after_unhide);
    EGUI_TEST_RUN(test_virtual_stage_focus_slot_replaces_pinned_slot_and_restores_after_blur);
    EGUI_TEST_RUN(test_virtual_stage_same_type_eviction_prefers_button_slot_over_textinput_slot);
    EGUI_TEST_RUN(test_virtual_stage_overlap_hit_prefers_higher_z_order_node);
    EGUI_TEST_RUN(test_virtual_stage_notify_node_changed_updates_touch_target_after_z_order_change);
    EGUI_TEST_RUN(test_virtual_stage_custom_hit_test_falls_through_to_lower_node);
    EGUI_TEST_RUN(test_virtual_stage_notify_node_changed_noninteractive_during_capture_cancels_click);
    EGUI_TEST_RUN(test_virtual_stage_notify_data_changed_during_capture_cancels_pooled_view_before_rebind);
    EGUI_TEST_RUN(test_virtual_stage_custom_hit_test_followup_move_outside_cancels_click);
    EGUI_TEST_RUN(test_virtual_stage_custom_hit_test_followup_move_back_inside_restores_click);
    EGUI_TEST_RUN(test_virtual_stage_notify_node_changed_recreates_view_when_type_changes);
    EGUI_TEST_RUN(test_virtual_stage_notify_node_changed_restores_keepalive_textinput_after_render_only_round_trip);
    EGUI_TEST_RUN(test_virtual_stage_timer_view_stops_on_detach_and_restarts_on_reattach);
    EGUI_TEST_RUN(test_virtual_stage_array_adapter_bridge_exposes_count_and_desc);
    EGUI_TEST_RUN(test_virtual_stage_array_adapter_default_hit_test_falls_back_to_rect);
    EGUI_TEST_RUN(test_virtual_stage_init_with_setup_applies_params_and_adapter);
    EGUI_TEST_RUN(test_virtual_stage_init_with_array_setup_applies_params_and_bridge_adapter);
    EGUI_TEST_RUN(test_virtual_stage_public_find_helpers_resolve_live_slot);
    EGUI_TEST_RUN(test_virtual_stage_public_resolve_node_by_view_returns_entry);
    EGUI_TEST_RUN(test_virtual_stage_public_resolve_stable_id_by_view_returns_stable_id);
    EGUI_TEST_RUN(test_virtual_stage_public_is_node_pinned_tracks_pin_state);
    EGUI_TEST_RUN(test_virtual_stage_public_toggle_pin_node_toggles_pin_state);
    EGUI_TEST_RUN(test_virtual_stage_notify_nodes_changed_rebinds_each_target_slot);
    EGUI_TEST_RUN(test_virtual_stage_notify_nodes_bounds_changed_updates_each_target_slot_region);
    EGUI_TEST_RUN(test_virtual_stage_pin_node_rejects_render_only_and_missing_node);
    EGUI_TEST_RUN(test_virtual_stage_pin_node_rejects_hidden_node);
    EGUI_TEST_SUITE_END();
}
