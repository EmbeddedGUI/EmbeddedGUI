#include "test_virtual_stage.h"

#include <string.h>

#include "egui.h"

#define TEST_VIRTUAL_STAGE_MAX_NODES 4

enum
{
    TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON = 1,
    TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT = 2,
};

typedef struct test_virtual_stage_node_data test_virtual_stage_node_data_t;
typedef struct test_virtual_stage_context test_virtual_stage_context_t;

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
    uint8_t keep_alive_enabled;
    uint16_t bind_count;
    uint16_t save_count;
    uint16_t restore_count;
    uint16_t destroy_count;
    uint16_t button_click_count;
    test_virtual_stage_node_data_t nodes[TEST_VIRTUAL_STAGE_MAX_NODES];
};

static egui_view_virtual_stage_t test_page;
static test_virtual_stage_context_t test_context;

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
    EGUI_UNUSED(self);
    test_context.button_click_count++;
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

    return NULL;
}

static void adapter_destroy_view(void *adapter_context, egui_view_t *view, uint16_t view_type)
{
    test_virtual_stage_context_t *ctx = (test_virtual_stage_context_t *)adapter_context;
    EGUI_UNUSED(view_type);
    ctx->destroy_count++;
    egui_free(view);
}

static void adapter_bind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    test_virtual_stage_context_t *ctx = (test_virtual_stage_context_t *)adapter_context;
    test_virtual_stage_node_data_t *node = &ctx->nodes[index];
    EGUI_UNUSED(stable_id);
    EGUI_UNUSED(desc);

    ctx->bind_count++;
    if (node->desc.view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_BUTTON)
    {
        egui_view_label_set_text(view, node->label);
    }
}

static void adapter_save_state(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    test_virtual_stage_context_t *ctx = (test_virtual_stage_context_t *)adapter_context;
    test_virtual_stage_node_data_t *node = find_node_by_stable_id(stable_id);

    EGUI_UNUSED(index);
    EGUI_UNUSED(desc);

    if (node != NULL && node->desc.view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT)
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
    EGUI_UNUSED(desc);

    if (node != NULL && node->desc.view_type == TEST_VIRTUAL_STAGE_VIEW_TYPE_TEXTINPUT)
    {
        egui_view_textinput_set_text(view, node->text);
        ctx->restore_count++;
    }
}

static uint8_t adapter_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    test_virtual_stage_context_t *ctx = (test_virtual_stage_context_t *)adapter_context;

    EGUI_UNUSED(view);
    EGUI_UNUSED(index);
    EGUI_UNUSED(desc);

    return ctx->keep_alive_enabled && ctx->keep_alive_stable_id == stable_id ? 1U : 0U;
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
        .hit_test = NULL,
        .should_keep_alive = adapter_should_keep_alive,
};

static void layout_page(void)
{
    EGUI_REGION_DEFINE(region, 0, 0, 160, 120);
    egui_view_layout(EGUI_VIEW_OF(&test_page), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_page)->region_screen, &region);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
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

static void reset_page(uint8_t live_slot_limit)
{
    egui_focus_manager_clear_focus();
    memset(&test_page, 0, sizeof(test_page));
    memset(&test_context, 0, sizeof(test_context));
    egui_view_virtual_stage_init(EGUI_VIEW_OF(&test_page));
    egui_view_virtual_stage_set_live_slot_limit(EGUI_VIEW_OF(&test_page), live_slot_limit);
    egui_view_virtual_stage_set_adapter(EGUI_VIEW_OF(&test_page), &test_adapter, &test_context);
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

void test_virtual_stage_run(void)
{
    EGUI_TEST_SUITE_BEGIN(virtual_stage);
    EGUI_TEST_RUN(test_virtual_stage_render_only_node_keeps_zero_slots);
    EGUI_TEST_RUN(test_virtual_stage_touch_button_materializes_and_releases);
    EGUI_TEST_RUN(test_virtual_stage_pin_and_unpin_respects_limit);
    EGUI_TEST_RUN(test_virtual_stage_double_pin_requires_matching_unpin);
    EGUI_TEST_RUN(test_virtual_stage_textinput_save_and_restore_state);
    EGUI_TEST_RUN(test_virtual_stage_adapter_keep_alive_retains_slot_until_cleared);
    EGUI_TEST_RUN(test_virtual_stage_notify_node_changed_rebinds_only_target_slot);
    EGUI_TEST_RUN(test_virtual_stage_notify_bounds_changed_updates_only_target_slot_region);
    EGUI_TEST_RUN(test_virtual_stage_hidden_node_releases_only_target_slot);
    EGUI_TEST_RUN(test_virtual_stage_focus_slot_replaces_pinned_slot_and_restores_after_blur);
    EGUI_TEST_RUN(test_virtual_stage_same_type_eviction_prefers_button_slot_over_textinput_slot);
    EGUI_TEST_SUITE_END();
}
