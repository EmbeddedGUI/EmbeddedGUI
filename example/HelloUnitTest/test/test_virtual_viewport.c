#include "test_virtual_viewport.h"

#include <stdio.h>
#include <string.h>

#include "egui.h"

#define TEST_VIRTUAL_VIEWPORT_MAX_ITEMS 1280

typedef struct test_virtual_viewport_context test_virtual_viewport_context_t;
struct test_virtual_viewport_context
{
    uint32_t item_count;
    uint32_t keep_alive_ids[4];
    uint16_t bind_count;
    uint16_t unbind_count;
    uint16_t keep_alive_save_count;
    uint16_t keep_alive_unbind_count;
    uint16_t state_save_count;
    uint16_t state_restore_count;
    uint8_t created_count;
    uint8_t keep_alive_count;
    uint8_t use_state_cache_callbacks;
    uint32_t stable_ids[TEST_VIRTUAL_VIEWPORT_MAX_ITEMS];
    int32_t item_heights[TEST_VIRTUAL_VIEWPORT_MAX_ITEMS];
    egui_view_label_t labels[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
    char texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][24];
    uint8_t view_state[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
};

static egui_view_virtual_viewport_t test_viewport;
static egui_view_virtual_list_t test_list;
static egui_view_virtual_page_t test_page;
static test_virtual_viewport_context_t test_context;

static int find_pool_index(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        if (view == EGUI_VIEW_OF(&test_context.labels[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static uint8_t is_keep_alive_id(test_virtual_viewport_context_t *ctx, uint32_t stable_id)
{
    uint8_t i;

    for (i = 0; i < ctx->keep_alive_count; i++)
    {
        if (ctx->keep_alive_ids[i] == stable_id)
        {
            return 1;
        }
    }

    return 0;
}

static uint32_t adapter_get_count(void *adapter_context)
{
    return ((test_virtual_viewport_context_t *)adapter_context)->item_count;
}

static uint32_t adapter_get_stable_id(void *adapter_context, uint32_t index)
{
    test_virtual_viewport_context_t *ctx = (test_virtual_viewport_context_t *)adapter_context;
    return ctx->stable_ids[index];
}

static int32_t adapter_find_index_by_stable_id(void *adapter_context, uint32_t stable_id)
{
    test_virtual_viewport_context_t *ctx = (test_virtual_viewport_context_t *)adapter_context;
    uint32_t index;

    for (index = 0; index < ctx->item_count; index++)
    {
        if (ctx->stable_ids[index] == stable_id)
        {
            return (int32_t)index;
        }
    }

    return -1;
}

static uint16_t adapter_get_view_type(void *adapter_context, uint32_t index)
{
    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(index);
    return 0;
}

static int32_t adapter_measure_main_size(void *adapter_context, uint32_t index, int32_t cross_size_hint)
{
    test_virtual_viewport_context_t *ctx = (test_virtual_viewport_context_t *)adapter_context;
    EGUI_UNUSED(cross_size_hint);
    return ctx->item_heights[index];
}

static egui_view_t *adapter_create_view(void *adapter_context, uint16_t view_type)
{
    test_virtual_viewport_context_t *ctx = (test_virtual_viewport_context_t *)adapter_context;
    egui_view_label_t *label;

    EGUI_UNUSED(view_type);

    if (ctx->created_count >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
    {
        return NULL;
    }

    label = &ctx->labels[ctx->created_count++];
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), EGUI_ALIGN_CENTER);
    return EGUI_VIEW_OF(label);
}

static void adapter_bind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    test_virtual_viewport_context_t *ctx = (test_virtual_viewport_context_t *)adapter_context;
    int pool_index = find_pool_index(view);

    ctx->bind_count++;
    if (pool_index >= 0)
    {
        snprintf(ctx->texts[pool_index], sizeof(ctx->texts[pool_index]), "Item %lu", (unsigned long)index);
        egui_view_label_set_text(view, ctx->texts[pool_index]);
    }

    EGUI_UNUSED(stable_id);
}

static void adapter_unbind_view(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    test_virtual_viewport_context_t *ctx = (test_virtual_viewport_context_t *)adapter_context;

    ctx->unbind_count++;
    if (is_keep_alive_id(ctx, stable_id))
    {
        ctx->keep_alive_unbind_count++;
    }

    EGUI_UNUSED(view);
}

static uint8_t adapter_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    test_virtual_viewport_context_t *ctx = (test_virtual_viewport_context_t *)adapter_context;

    EGUI_UNUSED(view);
    return is_keep_alive_id(ctx, stable_id);
}

static void adapter_save_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    test_virtual_viewport_context_t *ctx = (test_virtual_viewport_context_t *)adapter_context;
    int pool_index = find_pool_index(view);

    if (is_keep_alive_id(ctx, stable_id))
    {
        ctx->keep_alive_save_count++;
    }

    if (ctx->use_state_cache_callbacks && pool_index >= 0)
    {
        if (egui_view_virtual_viewport_write_state_for_view(view, stable_id, &ctx->view_state[pool_index], sizeof(ctx->view_state[pool_index])))
        {
            ctx->state_save_count++;
        }
    }

    EGUI_UNUSED(view);
}

static void adapter_restore_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    test_virtual_viewport_context_t *ctx = (test_virtual_viewport_context_t *)adapter_context;
    int pool_index = find_pool_index(view);
    uint8_t value = 0;

    if (!ctx->use_state_cache_callbacks || pool_index < 0)
    {
        return;
    }

    if (egui_view_virtual_viewport_read_state_for_view(view, stable_id, &value, sizeof(value)) == sizeof(value))
    {
        ctx->view_state[pool_index] = value;
        ctx->state_restore_count++;
    }
    else
    {
        ctx->view_state[pool_index] = 0;
    }
}

static const egui_view_virtual_viewport_adapter_t test_adapter = {
        .get_count = adapter_get_count,
        .get_stable_id = adapter_get_stable_id,
        .find_index_by_stable_id = adapter_find_index_by_stable_id,
        .get_view_type = adapter_get_view_type,
        .measure_main_size = adapter_measure_main_size,
        .create_view = adapter_create_view,
        .destroy_view = NULL,
        .bind_view = adapter_bind_view,
        .unbind_view = adapter_unbind_view,
        .should_keep_alive = adapter_should_keep_alive,
        .save_state = adapter_save_state,
        .restore_state = adapter_restore_state,
};

static const egui_view_virtual_list_data_source_t test_list_data_source = {
        .get_count = adapter_get_count,
        .get_stable_id = adapter_get_stable_id,
        .find_index_by_stable_id = NULL,
        .get_view_type = NULL,
        .measure_item_height = NULL,
        .create_item_view = adapter_create_view,
        .destroy_item_view = NULL,
        .bind_item_view = adapter_bind_view,
        .unbind_item_view = adapter_unbind_view,
        .should_keep_alive = NULL,
        .save_item_state = NULL,
        .restore_item_state = NULL,
        .default_view_type = 0,
};

static const egui_view_virtual_list_data_source_t test_list_measured_data_source = {
        .get_count = adapter_get_count,
        .get_stable_id = adapter_get_stable_id,
        .find_index_by_stable_id = NULL,
        .get_view_type = NULL,
        .measure_item_height = adapter_measure_main_size,
        .create_item_view = adapter_create_view,
        .destroy_item_view = NULL,
        .bind_item_view = adapter_bind_view,
        .unbind_item_view = adapter_unbind_view,
        .should_keep_alive = NULL,
        .save_item_state = NULL,
        .restore_item_state = NULL,
        .default_view_type = 0,
};

static const egui_view_virtual_page_data_source_t test_page_data_source = {
        .get_count = adapter_get_count,
        .get_stable_id = adapter_get_stable_id,
        .find_index_by_stable_id = NULL,
        .get_section_type = NULL,
        .measure_section_height = NULL,
        .create_section_view = adapter_create_view,
        .destroy_section_view = NULL,
        .bind_section_view = adapter_bind_view,
        .unbind_section_view = adapter_unbind_view,
        .should_keep_alive = NULL,
        .save_section_state = NULL,
        .restore_section_state = NULL,
        .default_section_type = 0,
};

static const egui_view_virtual_page_data_source_t test_page_measured_data_source = {
        .get_count = adapter_get_count,
        .get_stable_id = adapter_get_stable_id,
        .find_index_by_stable_id = NULL,
        .get_section_type = NULL,
        .measure_section_height = adapter_measure_main_size,
        .create_section_view = adapter_create_view,
        .destroy_section_view = NULL,
        .bind_section_view = adapter_bind_view,
        .unbind_section_view = adapter_unbind_view,
        .should_keep_alive = NULL,
        .save_section_state = NULL,
        .restore_section_state = NULL,
        .default_section_type = 0,
};

static void reset_test_items(uint32_t item_count, int32_t item_height)
{
    uint32_t i;

    if (item_count > TEST_VIRTUAL_VIEWPORT_MAX_ITEMS)
    {
        item_count = TEST_VIRTUAL_VIEWPORT_MAX_ITEMS;
    }

    test_context.item_count = item_count;
    for (i = 0; i < item_count; i++)
    {
        test_context.stable_ids[i] = 1000U + i;
        test_context.item_heights[i] = item_height;
    }
}

static void insert_test_item(uint32_t index, uint32_t stable_id, int32_t item_height)
{
    uint32_t i;

    if (test_context.item_count >= TEST_VIRTUAL_VIEWPORT_MAX_ITEMS)
    {
        return;
    }
    if (index > test_context.item_count)
    {
        index = test_context.item_count;
    }

    for (i = test_context.item_count; i > index; i--)
    {
        test_context.stable_ids[i] = test_context.stable_ids[i - 1];
        test_context.item_heights[i] = test_context.item_heights[i - 1];
    }

    test_context.stable_ids[index] = stable_id;
    test_context.item_heights[index] = item_height;
    test_context.item_count++;
}

static void layout_viewport(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_view_layout(EGUI_VIEW_OF(&test_viewport), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_viewport)->region_screen, &region);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));
}

static void layout_list(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_view_layout(EGUI_VIEW_OF(&test_list), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_list)->region_screen, &region);
    EGUI_VIEW_OF(&test_list)->api->calculate_layout(EGUI_VIEW_OF(&test_list));
}

static void setup_viewport(uint32_t item_count, uint32_t keep_alive_id)
{
    memset(&test_viewport, 0, sizeof(test_viewport));
    memset(&test_context, 0, sizeof(test_context));

    reset_test_items(item_count, 20);
    if (keep_alive_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        test_context.keep_alive_ids[0] = keep_alive_id;
        test_context.keep_alive_count = 1;
    }

    egui_view_virtual_viewport_init(EGUI_VIEW_OF(&test_viewport));
    egui_view_virtual_viewport_set_adapter(EGUI_VIEW_OF(&test_viewport), &test_adapter, &test_context);
    egui_view_virtual_viewport_set_overscan(EGUI_VIEW_OF(&test_viewport), 1, 1);
    egui_view_virtual_viewport_set_estimated_item_extent(EGUI_VIEW_OF(&test_viewport), 20);
    layout_viewport(0, 0, 100, 60);
}

static void setup_list_with_data_source_config(uint32_t item_count, const egui_view_virtual_list_data_source_t *data_source)
{
    memset(&test_list, 0, sizeof(test_list));
    memset(&test_context, 0, sizeof(test_context));

    reset_test_items(item_count, 20);

    egui_view_virtual_list_init(EGUI_VIEW_OF(&test_list));
    egui_view_virtual_list_set_data_source(EGUI_VIEW_OF(&test_list), data_source, &test_context);
    egui_view_virtual_list_set_overscan(EGUI_VIEW_OF(&test_list), 1, 1);
    egui_view_virtual_list_set_estimated_item_height(EGUI_VIEW_OF(&test_list), 20);
    layout_list(0, 0, 100, 60);
}

static void setup_list_with_data_source(uint32_t item_count)
{
    setup_list_with_data_source_config(item_count, &test_list_data_source);
}

static void setup_page_with_data_source_config(uint32_t item_count, const egui_view_virtual_page_data_source_t *data_source)
{
    EGUI_REGION_DEFINE(region, 0, 0, 100, 60);

    memset(&test_page, 0, sizeof(test_page));
    memset(&test_context, 0, sizeof(test_context));

    reset_test_items(item_count, 20);

    egui_view_virtual_page_init(EGUI_VIEW_OF(&test_page));
    egui_view_virtual_page_set_data_source(EGUI_VIEW_OF(&test_page), data_source, &test_context);
    egui_view_virtual_page_set_overscan(EGUI_VIEW_OF(&test_page), 1, 1);
    egui_view_virtual_page_set_estimated_section_height(EGUI_VIEW_OF(&test_page), 20);
    egui_view_layout(EGUI_VIEW_OF(&test_page), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_page)->region_screen, &region);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
}

static void setup_page_with_data_source(uint32_t item_count)
{
    setup_page_with_data_source_config(item_count, &test_page_data_source);
}

static int count_slots_with_state(uint8_t state)
{
    uint8_t i;
    int count = 0;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        const egui_view_virtual_viewport_slot_t *slot = egui_view_virtual_viewport_get_slot(EGUI_VIEW_OF(&test_viewport), i);
        if (slot != NULL && slot->state == state)
        {
            count++;
        }
    }

    return count;
}

static const egui_view_virtual_viewport_slot_t *find_slot_by_stable_id(uint32_t stable_id)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        const egui_view_virtual_viewport_slot_t *slot = egui_view_virtual_viewport_get_slot(EGUI_VIEW_OF(&test_viewport), i);
        if (slot != NULL && slot->stable_id == stable_id)
        {
            return slot;
        }
    }

    return NULL;
}

static int count_slots_by_stable_id(uint32_t stable_id)
{
    uint8_t i;
    int count = 0;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        const egui_view_virtual_viewport_slot_t *slot = egui_view_virtual_viewport_get_slot(EGUI_VIEW_OF(&test_viewport), i);
        if (slot != NULL && slot->stable_id == stable_id)
        {
            count++;
        }
    }

    return count;
}

static void test_virtual_viewport_initial_window_uses_bounded_slots(void)
{
    setup_viewport(1200, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);

    EGUI_TEST_ASSERT_EQUAL_INT(4, count_slots_with_state(EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE));
    EGUI_TEST_ASSERT_EQUAL_INT(4, test_context.created_count);
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(1000));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(1001));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(1002));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(1003));
}

static void test_virtual_viewport_scroll_reuses_slot_pool(void)
{
    setup_viewport(1200, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);

    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&test_viewport), 200);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));
    EGUI_TEST_ASSERT_EQUAL_INT(5, count_slots_with_state(EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE));
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_context.created_count);

    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&test_viewport), 400);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    EGUI_TEST_ASSERT_EQUAL_INT(5, count_slots_with_state(EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE));
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_context.created_count);
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(1019));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(1023));
}

static void test_virtual_viewport_keepalive_preserves_view_instance(void)
{
    setup_viewport(1200, 1000);

    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&test_viewport), 200);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(1000));
    EGUI_TEST_ASSERT_EQUAL_INT(1, count_slots_with_state(EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_context.keep_alive_save_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_context.keep_alive_unbind_count);
}

static void test_virtual_viewport_variable_height_resize_keeps_anchor(void)
{
    setup_viewport(40, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);

    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&test_viewport), 407);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    EGUI_TEST_ASSERT_EQUAL_INT(400, egui_view_virtual_viewport_get_item_main_origin(EGUI_VIEW_OF(&test_viewport), 20));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_viewport_get_item_main_size(EGUI_VIEW_OF(&test_viewport), 20));

    test_context.item_heights[2] = 35;
    egui_view_virtual_viewport_notify_item_resized(EGUI_VIEW_OF(&test_viewport), 2);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    EGUI_TEST_ASSERT_EQUAL_INT(415, egui_view_virtual_viewport_get_item_main_origin(EGUI_VIEW_OF(&test_viewport), 20));
    EGUI_TEST_ASSERT_EQUAL_INT(422, egui_view_virtual_viewport_get_logical_offset(EGUI_VIEW_OF(&test_viewport)));
}

static void test_virtual_viewport_insert_before_anchor_preserves_scroll_position(void)
{
    setup_viewport(40, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);

    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&test_viewport), 407);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    insert_test_item(0, 900, 30);
    egui_view_virtual_viewport_notify_item_inserted(EGUI_VIEW_OF(&test_viewport), 0, 1);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    EGUI_TEST_ASSERT_EQUAL_INT(437, egui_view_virtual_viewport_get_logical_offset(EGUI_VIEW_OF(&test_viewport)));
    EGUI_TEST_ASSERT_EQUAL_INT(430, egui_view_virtual_viewport_get_item_main_origin(EGUI_VIEW_OF(&test_viewport), 21));
}

static void test_virtual_viewport_keepalive_limit_trims_oldest_slot(void)
{
    setup_viewport(1200, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);

    test_context.keep_alive_ids[0] = 1000;
    test_context.keep_alive_ids[1] = 1001;
    test_context.keep_alive_count = 2;

    egui_view_virtual_viewport_set_keepalive_limit(EGUI_VIEW_OF(&test_viewport), 1);
    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&test_viewport), 120);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    EGUI_TEST_ASSERT_EQUAL_INT(1, count_slots_with_state(EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE));
    EGUI_TEST_ASSERT_NULL(find_slot_by_stable_id(1000));
    EGUI_TEST_ASSERT_NOT_NULL(find_slot_by_stable_id(1001));
}

static void test_virtual_viewport_state_cache_write_read_and_trim(void)
{
    uint8_t value = 0;

    setup_viewport(40, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    egui_view_virtual_viewport_set_state_cache_limits(EGUI_VIEW_OF(&test_viewport), 2, 2);

    value = 11;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_write_state(EGUI_VIEW_OF(&test_viewport), 1000, &value, sizeof(value)));
    value = 22;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_write_state(EGUI_VIEW_OF(&test_viewport), 1001, &value, sizeof(value)));

    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_viewport_read_state(EGUI_VIEW_OF(&test_viewport), 1000, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(11, value);

    value = 33;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_write_state(EGUI_VIEW_OF(&test_viewport), 1002, &value, sizeof(value)));

    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_viewport_read_state(EGUI_VIEW_OF(&test_viewport), 1000, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(11, value);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_viewport_read_state(EGUI_VIEW_OF(&test_viewport), 1001, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_viewport_read_state(EGUI_VIEW_OF(&test_viewport), 1002, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(33, value);

    egui_view_virtual_viewport_remove_state_by_stable_id(EGUI_VIEW_OF(&test_viewport), 1000);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_viewport_read_state(EGUI_VIEW_OF(&test_viewport), 1000, &value, sizeof(value)));

    egui_view_virtual_viewport_clear_state_cache(EGUI_VIEW_OF(&test_viewport));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_viewport_read_state(EGUI_VIEW_OF(&test_viewport), 1002, &value, sizeof(value)));
}

static void test_virtual_viewport_state_cache_restores_recycled_slot(void)
{
    const egui_view_virtual_viewport_slot_t *slot;
    uint8_t saved_value = 0;
    int pool_index;

    setup_viewport(1200, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    test_context.use_state_cache_callbacks = 1;
    egui_view_virtual_viewport_set_state_cache_limits(EGUI_VIEW_OF(&test_viewport), 8, 16);

    slot = find_slot_by_stable_id(1000);
    EGUI_TEST_ASSERT_NOT_NULL(slot);

    pool_index = find_pool_index(slot->view);
    EGUI_TEST_ASSERT_TRUE(pool_index >= 0);
    test_context.view_state[pool_index] = 77;

    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&test_viewport), 200);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    EGUI_TEST_ASSERT_TRUE(test_context.state_save_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_viewport_read_state(EGUI_VIEW_OF(&test_viewport), 1000, &saved_value, sizeof(saved_value)));
    EGUI_TEST_ASSERT_EQUAL_INT(77, saved_value);

    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&test_viewport), 0);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    slot = find_slot_by_stable_id(1000);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    pool_index = find_pool_index(slot->view);
    EGUI_TEST_ASSERT_TRUE(pool_index >= 0);
    EGUI_TEST_ASSERT_TRUE(test_context.state_restore_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(77, test_context.view_state[pool_index]);
}

static void test_virtual_viewport_duplicate_keepalive_slot_is_recycled(void)
{
    egui_view_virtual_viewport_slot_t *duplicate_slot;

    setup_viewport(1200, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);

    duplicate_slot = &test_viewport.slots[1];
    duplicate_slot->stable_id = 1000;
    duplicate_slot->state = EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE;

    egui_view_virtual_viewport_notify_item_changed(EGUI_VIEW_OF(&test_viewport), 0);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    EGUI_TEST_ASSERT_EQUAL_INT(1, count_slots_by_stable_id(1000));
    EGUI_TEST_ASSERT_TRUE(duplicate_slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE);
}

static void test_virtual_list_data_source_defaults_bridge_viewport_adapter(void)
{
    setup_list_with_data_source(40);

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_list_get_data_source(EGUI_VIEW_OF(&test_list)) == &test_list_data_source);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_list_get_data_source_context(EGUI_VIEW_OF(&test_list)) == &test_context);

    egui_view_virtual_list_set_scroll_y(EGUI_VIEW_OF(&test_list), 407);
    EGUI_VIEW_OF(&test_list)->api->calculate_layout(EGUI_VIEW_OF(&test_list));

    insert_test_item(0, 900, 20);
    egui_view_virtual_list_notify_item_inserted(EGUI_VIEW_OF(&test_list), 0, 1);
    EGUI_VIEW_OF(&test_list)->api->calculate_layout(EGUI_VIEW_OF(&test_list));

    EGUI_TEST_ASSERT_EQUAL_INT(427, egui_view_virtual_list_get_scroll_y(EGUI_VIEW_OF(&test_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(420, egui_view_virtual_list_get_item_y(EGUI_VIEW_OF(&test_list), 21));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_list_get_item_height(EGUI_VIEW_OF(&test_list), 21));
}

static void test_virtual_list_stable_id_helpers_fallback_lookup(void)
{
    setup_list_with_data_source_config(40, &test_list_measured_data_source);

    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_list_find_index_by_stable_id(EGUI_VIEW_OF(&test_list), 1020));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_list_find_index_by_stable_id(EGUI_VIEW_OF(&test_list), 999999));

    egui_view_virtual_list_scroll_to_stable_id(EGUI_VIEW_OF(&test_list), 1020, 7);
    EGUI_VIEW_OF(&test_list)->api->calculate_layout(EGUI_VIEW_OF(&test_list));
    EGUI_TEST_ASSERT_EQUAL_INT(407, egui_view_virtual_list_get_scroll_y(EGUI_VIEW_OF(&test_list)));

    test_context.item_heights[2] = 35;
    egui_view_virtual_list_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&test_list), 1002);
    EGUI_VIEW_OF(&test_list)->api->calculate_layout(EGUI_VIEW_OF(&test_list));

    EGUI_TEST_ASSERT_EQUAL_INT(415, egui_view_virtual_list_get_item_y(EGUI_VIEW_OF(&test_list), 20));
    EGUI_TEST_ASSERT_EQUAL_INT(422, egui_view_virtual_list_get_scroll_y(EGUI_VIEW_OF(&test_list)));
}

static void test_virtual_list_state_cache_helpers_bridge_viewport(void)
{
    const egui_view_virtual_list_slot_t *slot;
    uint8_t value = 0;

    setup_list_with_data_source(40);
    egui_view_virtual_list_set_state_cache_limits(EGUI_VIEW_OF(&test_list), 2, 4);

    value = 61;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_list_write_item_state(EGUI_VIEW_OF(&test_list), 1002, &value, sizeof(value)));
    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_list_read_item_state(EGUI_VIEW_OF(&test_list), 1002, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(61, value);

    slot = egui_view_virtual_list_get_slot(EGUI_VIEW_OF(&test_list), 0);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    value = 73;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_list_write_item_state_for_view(slot->view, 1000, &value, sizeof(value)));
    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_list_read_item_state_for_view(slot->view, 1000, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(73, value);

    egui_view_virtual_list_remove_item_state_by_stable_id(EGUI_VIEW_OF(&test_list), 1002);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_list_read_item_state(EGUI_VIEW_OF(&test_list), 1002, &value, sizeof(value)));
}

static void test_virtual_page_data_source_defaults_bridge_viewport_adapter(void)
{
    setup_page_with_data_source(40);

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_page_get_data_source(EGUI_VIEW_OF(&test_page)) == &test_page_data_source);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_page_get_data_source_context(EGUI_VIEW_OF(&test_page)) == &test_context);

    egui_view_virtual_page_set_scroll_y(EGUI_VIEW_OF(&test_page), 407);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    insert_test_item(0, 900, 20);
    egui_view_virtual_page_notify_section_inserted(EGUI_VIEW_OF(&test_page), 0, 1);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(427, egui_view_virtual_page_get_scroll_y(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_EQUAL_INT(420, egui_view_virtual_page_get_section_y(EGUI_VIEW_OF(&test_page), 21));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_page_get_section_height(EGUI_VIEW_OF(&test_page), 21));
}

static void test_virtual_page_stable_id_helpers_fallback_lookup(void)
{
    setup_page_with_data_source_config(40, &test_page_measured_data_source);

    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_page_find_section_index_by_stable_id(EGUI_VIEW_OF(&test_page), 1020));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_page_find_section_index_by_stable_id(EGUI_VIEW_OF(&test_page), 999999));

    egui_view_virtual_page_scroll_to_section_by_stable_id(EGUI_VIEW_OF(&test_page), 1020, 7);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
    EGUI_TEST_ASSERT_EQUAL_INT(407, egui_view_virtual_page_get_scroll_y(EGUI_VIEW_OF(&test_page)));

    test_context.item_heights[2] = 35;
    egui_view_virtual_page_notify_section_resized_by_stable_id(EGUI_VIEW_OF(&test_page), 1002);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_EQUAL_INT(415, egui_view_virtual_page_get_section_y(EGUI_VIEW_OF(&test_page), 20));
    EGUI_TEST_ASSERT_EQUAL_INT(422, egui_view_virtual_page_get_scroll_y(EGUI_VIEW_OF(&test_page)));
}

static void test_virtual_page_state_cache_helpers_bridge_viewport(void)
{
    const egui_view_virtual_page_slot_t *slot;
    uint8_t value = 0;

    setup_page_with_data_source(40);
    egui_view_virtual_page_set_state_cache_limits(EGUI_VIEW_OF(&test_page), 2, 4);

    value = 41;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_page_write_section_state(EGUI_VIEW_OF(&test_page), 1002, &value, sizeof(value)));
    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_page_read_section_state(EGUI_VIEW_OF(&test_page), 1002, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(41, value);

    slot = egui_view_virtual_page_get_slot(EGUI_VIEW_OF(&test_page), 0);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    value = 55;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_page_write_section_state_for_view(slot->view, 1000, &value, sizeof(value)));
    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_page_read_section_state_for_view(slot->view, 1000, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(55, value);

    egui_view_virtual_page_remove_section_state_by_stable_id(EGUI_VIEW_OF(&test_page), 1002);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_page_read_section_state(EGUI_VIEW_OF(&test_page), 1002, &value, sizeof(value)));
}

void test_virtual_viewport_run(void)
{
    EGUI_TEST_SUITE_BEGIN(virtual_viewport);
    EGUI_TEST_RUN(test_virtual_viewport_initial_window_uses_bounded_slots);
    EGUI_TEST_RUN(test_virtual_viewport_scroll_reuses_slot_pool);
    EGUI_TEST_RUN(test_virtual_viewport_keepalive_preserves_view_instance);
    EGUI_TEST_RUN(test_virtual_viewport_variable_height_resize_keeps_anchor);
    EGUI_TEST_RUN(test_virtual_viewport_insert_before_anchor_preserves_scroll_position);
    EGUI_TEST_RUN(test_virtual_viewport_keepalive_limit_trims_oldest_slot);
    EGUI_TEST_RUN(test_virtual_viewport_state_cache_write_read_and_trim);
    EGUI_TEST_RUN(test_virtual_viewport_state_cache_restores_recycled_slot);
    EGUI_TEST_RUN(test_virtual_viewport_duplicate_keepalive_slot_is_recycled);
    EGUI_TEST_RUN(test_virtual_list_data_source_defaults_bridge_viewport_adapter);
    EGUI_TEST_RUN(test_virtual_list_stable_id_helpers_fallback_lookup);
    EGUI_TEST_RUN(test_virtual_list_state_cache_helpers_bridge_viewport);
    EGUI_TEST_RUN(test_virtual_page_data_source_defaults_bridge_viewport_adapter);
    EGUI_TEST_RUN(test_virtual_page_stable_id_helpers_fallback_lookup);
    EGUI_TEST_RUN(test_virtual_page_state_cache_helpers_bridge_viewport);
    EGUI_TEST_SUITE_END();
}
