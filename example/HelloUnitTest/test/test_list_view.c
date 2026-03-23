#include "test_list_view.h"

#include <string.h>

#include "egui.h"

#define TEST_LIST_VIEW_MAX_ITEMS 40

typedef struct test_list_view_holder test_list_view_holder_t;
typedef struct test_list_view_context test_list_view_context_t;

struct test_list_view_holder
{
    egui_view_list_view_holder_t base;
    egui_view_group_t root;
    egui_view_label_t label;
};

struct test_list_view_context
{
    uint32_t item_count;
    uint16_t bind_count;
    uint16_t destroy_count;
    uint8_t created_count;
    uint32_t stable_ids[TEST_LIST_VIEW_MAX_ITEMS];
    int32_t heights[TEST_LIST_VIEW_MAX_ITEMS];
    test_list_view_holder_t holders[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
    char texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][24];
};

static egui_view_list_view_t test_list_view;
static test_list_view_context_t test_context;

static int find_holder_index(const egui_view_list_view_holder_t *holder)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        if (holder == &test_context.holders[i].base)
        {
            return (int)i;
        }
    }

    return -1;
}

static void layout_list_view(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_view_layout(EGUI_VIEW_OF(&test_list_view), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_list_view)->region_screen, &region);
    EGUI_VIEW_OF(&test_list_view)->api->calculate_layout(EGUI_VIEW_OF(&test_list_view));
}

static void reset_list_items(void)
{
    uint32_t index;

    memset(&test_context, 0, sizeof(test_context));
    test_context.item_count = TEST_LIST_VIEW_MAX_ITEMS;

    for (index = 0; index < test_context.item_count; index++)
    {
        test_context.stable_ids[index] = 9100U + index;
        test_context.heights[index] = (index % 3U) == 0U ? 18 : 24;
    }
}

static uint32_t list_view_get_count(void *data_model_context)
{
    return ((test_list_view_context_t *)data_model_context)->item_count;
}

static uint32_t list_view_get_stable_id(void *data_model_context, uint32_t index)
{
    return ((test_list_view_context_t *)data_model_context)->stable_ids[index];
}

static int32_t list_view_find_index_by_stable_id(void *data_model_context, uint32_t stable_id)
{
    test_list_view_context_t *ctx = (test_list_view_context_t *)data_model_context;
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

static int32_t list_view_measure_item_height(void *data_model_context, uint32_t index, int32_t width_hint)
{
    EGUI_UNUSED(width_hint);
    return ((test_list_view_context_t *)data_model_context)->heights[index];
}

static egui_view_list_view_holder_t *list_view_create_holder(void *data_model_context, uint16_t view_type)
{
    test_list_view_context_t *ctx = (test_list_view_context_t *)data_model_context;
    test_list_view_holder_t *holder;

    EGUI_UNUSED(view_type);

    if (ctx->created_count >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
    {
        return NULL;
    }

    holder = &ctx->holders[ctx->created_count++];
    memset(holder, 0, sizeof(*holder));

    egui_view_group_init(EGUI_VIEW_OF(&holder->root));
    egui_view_label_init(EGUI_VIEW_OF(&holder->label));
    egui_view_label_set_align_type(EGUI_VIEW_OF(&holder->label), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&holder->root), EGUI_VIEW_OF(&holder->label));

    holder->base.item_view = EGUI_VIEW_OF(&holder->root);
    return &holder->base;
}

static void list_view_destroy_holder(void *data_model_context, egui_view_list_view_holder_t *holder, uint16_t view_type)
{
    EGUI_UNUSED(holder);
    EGUI_UNUSED(view_type);
    ((test_list_view_context_t *)data_model_context)->destroy_count++;
}

static void list_view_bind_holder(void *data_model_context, egui_view_list_view_holder_t *holder, uint32_t index, uint32_t stable_id)
{
    test_list_view_context_t *ctx = (test_list_view_context_t *)data_model_context;
    test_list_view_holder_t *typed_holder = (test_list_view_holder_t *)holder;
    int pool_index = find_holder_index(holder);

    ctx->bind_count++;
    if (pool_index < 0)
    {
        return;
    }

    snprintf(ctx->texts[pool_index], sizeof(ctx->texts[pool_index]), "Item %lu:%lu", (unsigned long)index, (unsigned long)stable_id);
    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->root), holder->host_view->region.size.width, holder->host_view->region.size.height);
    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->label), holder->host_view->region.size.width, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&typed_holder->label), ctx->texts[pool_index]);
}

static const egui_view_list_view_data_model_t test_data_model = {
        .get_count = list_view_get_count,
        .get_stable_id = list_view_get_stable_id,
        .find_index_by_stable_id = list_view_find_index_by_stable_id,
        .get_view_type = NULL,
        .measure_item_height = list_view_measure_item_height,
        .default_view_type = 0,
};

static const egui_view_list_view_holder_ops_t test_holder_ops = {
        .create_holder = list_view_create_holder,
        .destroy_holder = list_view_destroy_holder,
        .bind_holder = list_view_bind_holder,
        .unbind_holder = NULL,
        .should_keep_alive = NULL,
        .save_holder_state = NULL,
        .restore_holder_state = NULL,
};

static void setup_list_view(void)
{
    const egui_view_list_view_params_t params = {
            .region = {{0, 0}, {100, 60}},
            .overscan_before = 1,
            .overscan_after = 1,
            .max_keepalive_slots = 2,
            .estimated_item_height = 20,
    };
    const egui_view_list_view_setup_t setup = {
            .params = &params,
            .data_model = &test_data_model,
            .holder_ops = &test_holder_ops,
            .data_model_context = &test_context,
            .state_cache_max_entries = 4,
            .state_cache_max_bytes = 16,
    };

    memset(&test_list_view, 0, sizeof(test_list_view));
    reset_list_items();

    egui_view_list_view_init_with_setup(EGUI_VIEW_OF(&test_list_view), &setup);
    layout_list_view(0, 0, 100, 60);
}

static void test_list_view_bridge_and_lookup_helpers(void)
{
    egui_view_list_view_entry_t entry;
    egui_view_list_view_holder_t *holder;
    egui_view_list_view_holder_t *resolved_holder;
    egui_view_t *item_view;
    test_list_view_holder_t *typed_holder;

    setup_list_view();

    EGUI_TEST_ASSERT_TRUE(egui_view_list_view_get_data_model(EGUI_VIEW_OF(&test_list_view)) == &test_data_model);
    EGUI_TEST_ASSERT_TRUE(egui_view_list_view_get_holder_ops(EGUI_VIEW_OF(&test_list_view)) == &test_holder_ops);
    EGUI_TEST_ASSERT_TRUE(egui_view_list_view_get_data_model_context(EGUI_VIEW_OF(&test_list_view)) == &test_context);
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_list_view_get_state_cache_entry_limit(EGUI_VIEW_OF(&test_list_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(16, egui_view_list_view_get_state_cache_byte_limit(EGUI_VIEW_OF(&test_list_view)));
    EGUI_TEST_ASSERT_TRUE(test_context.created_count > 0);
    EGUI_TEST_ASSERT_TRUE(test_context.bind_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_LIST_VIEW_MAX_ITEMS, egui_view_list_view_get_item_count(EGUI_VIEW_OF(&test_list_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_list_view_find_index_by_stable_id(EGUI_VIEW_OF(&test_list_view), 9104));

    EGUI_TEST_ASSERT_TRUE(egui_view_list_view_resolve_item_by_stable_id(EGUI_VIEW_OF(&test_list_view), 9104, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(4, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(9104, entry.stable_id);

    EGUI_TEST_ASSERT_TRUE(egui_view_list_view_find_holder_by_stable_id(EGUI_VIEW_OF(&test_list_view), 9104) == NULL);

    egui_view_list_view_scroll_to_stable_id(EGUI_VIEW_OF(&test_list_view), 9104, 0);
    layout_list_view(0, 0, 100, 60);

    holder = egui_view_list_view_find_holder_by_stable_id(EGUI_VIEW_OF(&test_list_view), 9104);
    EGUI_TEST_ASSERT_NOT_NULL(holder);
    item_view = egui_view_list_view_find_item_view_by_stable_id(EGUI_VIEW_OF(&test_list_view), 9104);
    EGUI_TEST_ASSERT_TRUE(item_view == holder->item_view);

    EGUI_TEST_ASSERT_TRUE(egui_view_list_view_resolve_holder_by_view(EGUI_VIEW_OF(&test_list_view), item_view, &resolved_holder, &entry));
    EGUI_TEST_ASSERT_TRUE(resolved_holder == holder);
    EGUI_TEST_ASSERT_EQUAL_INT(4, entry.index);

    typed_holder = (test_list_view_holder_t *)holder;
    EGUI_TEST_ASSERT_TRUE(
            egui_view_list_view_resolve_holder_by_view(EGUI_VIEW_OF(&test_list_view), EGUI_VIEW_OF(&typed_holder->label), &resolved_holder, &entry));
    EGUI_TEST_ASSERT_TRUE(resolved_holder == holder);
    EGUI_TEST_ASSERT_EQUAL_INT(9104, entry.stable_id);

    egui_view_list_view_scroll_to_stable_id(EGUI_VIEW_OF(&test_list_view), 9110, 2);
    layout_list_view(0, 0, 100, 60);
    EGUI_TEST_ASSERT_TRUE(egui_view_list_view_get_scroll_y(EGUI_VIEW_OF(&test_list_view)) > 0);
}

static void test_list_view_resize_helpers_bridge_virtual_list(void)
{
    int32_t before_y;

    setup_list_view();
    before_y = egui_view_list_view_get_item_y(EGUI_VIEW_OF(&test_list_view), 4);

    test_context.heights[1] += 8;
    egui_view_list_view_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&test_list_view), 9101);
    layout_list_view(0, 0, 100, 60);

    EGUI_TEST_ASSERT_EQUAL_INT(before_y + 8, egui_view_list_view_get_item_y(EGUI_VIEW_OF(&test_list_view), 4));
}

void test_list_view_run(void)
{
    EGUI_TEST_SUITE_BEGIN(list_view);
    EGUI_TEST_RUN(test_list_view_bridge_and_lookup_helpers);
    EGUI_TEST_RUN(test_list_view_resize_helpers_bridge_virtual_list);
}
