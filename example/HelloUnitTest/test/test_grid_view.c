#include "test_grid_view.h"

#include <string.h>

#include "egui.h"

#define TEST_GRID_VIEW_MAX_ITEMS 8
#define TEST_GRID_VIEW_MAX_POOL  (EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS * EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS)

typedef struct test_grid_view_holder test_grid_view_holder_t;
typedef struct test_grid_view_context test_grid_view_context_t;

struct test_grid_view_holder
{
    egui_view_grid_view_holder_t base;
    egui_view_group_t root;
    egui_view_label_t label;
};

struct test_grid_view_context
{
    uint32_t item_count;
    uint16_t bind_count;
    uint16_t destroy_count;
    uint8_t created_count;
    uint32_t stable_ids[TEST_GRID_VIEW_MAX_ITEMS];
    int32_t heights[TEST_GRID_VIEW_MAX_ITEMS];
    test_grid_view_holder_t holders[TEST_GRID_VIEW_MAX_POOL];
    char texts[TEST_GRID_VIEW_MAX_POOL][24];
};

static egui_view_grid_view_t test_grid_view;
static test_grid_view_context_t test_context;

static int find_holder_index(const egui_view_grid_view_holder_t *holder)
{
    uint8_t i;

    for (i = 0; i < TEST_GRID_VIEW_MAX_POOL; i++)
    {
        if (holder == &test_context.holders[i].base)
        {
            return (int)i;
        }
    }

    return -1;
}

static void layout_grid_view(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_view_layout(EGUI_VIEW_OF(&test_grid_view), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_grid_view)->region_screen, &region);
    EGUI_VIEW_OF(&test_grid_view)->api->calculate_layout(EGUI_VIEW_OF(&test_grid_view));
}

static void reset_grid_items(void)
{
    memset(&test_context, 0, sizeof(test_context));
    test_context.item_count = TEST_GRID_VIEW_MAX_ITEMS;

    test_context.stable_ids[0] = 9300;
    test_context.stable_ids[1] = 9301;
    test_context.stable_ids[2] = 9302;
    test_context.stable_ids[3] = 9303;
    test_context.stable_ids[4] = 9304;
    test_context.stable_ids[5] = 9305;
    test_context.stable_ids[6] = 9306;
    test_context.stable_ids[7] = 9307;

    test_context.heights[0] = 20;
    test_context.heights[1] = 12;
    test_context.heights[2] = 16;
    test_context.heights[3] = 18;
    test_context.heights[4] = 22;
    test_context.heights[5] = 14;
    test_context.heights[6] = 26;
    test_context.heights[7] = 10;
}

static uint32_t grid_view_get_count(void *data_model_context)
{
    return ((test_grid_view_context_t *)data_model_context)->item_count;
}

static uint32_t grid_view_get_stable_id(void *data_model_context, uint32_t index)
{
    return ((test_grid_view_context_t *)data_model_context)->stable_ids[index];
}

static int32_t grid_view_find_index_by_stable_id(void *data_model_context, uint32_t stable_id)
{
    test_grid_view_context_t *ctx = (test_grid_view_context_t *)data_model_context;
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

static int32_t grid_view_measure_item_height(void *data_model_context, uint32_t index, int32_t width_hint)
{
    EGUI_UNUSED(width_hint);
    return ((test_grid_view_context_t *)data_model_context)->heights[index];
}

static egui_view_grid_view_holder_t *grid_view_create_holder(void *data_model_context, uint16_t view_type)
{
    test_grid_view_context_t *ctx = (test_grid_view_context_t *)data_model_context;
    test_grid_view_holder_t *holder;

    EGUI_UNUSED(view_type);

    if (ctx->created_count >= TEST_GRID_VIEW_MAX_POOL)
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

static void grid_view_destroy_holder(void *data_model_context, egui_view_grid_view_holder_t *holder, uint16_t view_type)
{
    EGUI_UNUSED(holder);
    EGUI_UNUSED(view_type);
    ((test_grid_view_context_t *)data_model_context)->destroy_count++;
}

static void grid_view_bind_holder(void *data_model_context, egui_view_grid_view_holder_t *holder, uint32_t index, uint32_t stable_id)
{
    test_grid_view_context_t *ctx = (test_grid_view_context_t *)data_model_context;
    test_grid_view_holder_t *typed_holder = (test_grid_view_holder_t *)holder;
    int pool_index = find_holder_index(holder);

    ctx->bind_count++;
    if (pool_index < 0)
    {
        return;
    }

    snprintf(ctx->texts[pool_index], sizeof(ctx->texts[pool_index]), "Tile %lu", (unsigned long)index);
    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->root), holder->host_view->region.size.width, holder->host_view->region.size.height);
    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->label), holder->host_view->region.size.width, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&typed_holder->label), ctx->texts[pool_index]);
    EGUI_UNUSED(stable_id);
}

static const egui_view_grid_view_data_model_t test_data_model = {
        .get_count = grid_view_get_count,
        .get_stable_id = grid_view_get_stable_id,
        .find_index_by_stable_id = grid_view_find_index_by_stable_id,
        .get_view_type = NULL,
        .measure_item_height = grid_view_measure_item_height,
        .default_view_type = 0,
};

static const egui_view_grid_view_holder_ops_t test_holder_ops = {
        .create_holder = grid_view_create_holder,
        .destroy_holder = grid_view_destroy_holder,
        .bind_holder = grid_view_bind_holder,
        .unbind_holder = NULL,
        .should_keep_alive = NULL,
        .save_holder_state = NULL,
        .restore_holder_state = NULL,
};

static void setup_grid_view(void)
{
    const egui_view_grid_view_params_t params = {
            .region = {{0, 0}, {100, 40}},
            .column_count = 3,
            .overscan_before = 1,
            .overscan_after = 1,
            .max_keepalive_slots = 2,
            .column_spacing = 4,
            .row_spacing = 5,
            .estimated_item_height = 18,
    };
    const egui_view_grid_view_setup_t setup = {
            .params = &params,
            .data_model = &test_data_model,
            .holder_ops = &test_holder_ops,
            .data_model_context = &test_context,
            .state_cache_max_entries = 5,
            .state_cache_max_bytes = 20,
    };

    memset(&test_grid_view, 0, sizeof(test_grid_view));
    reset_grid_items();

    egui_view_grid_view_init_with_setup(EGUI_VIEW_OF(&test_grid_view), &setup);
    layout_grid_view(0, 0, 100, 40);
}

static void test_grid_view_bridge_and_lookup_helpers(void)
{
    egui_view_grid_view_entry_t entry;
    egui_view_grid_view_holder_t *holder;
    egui_view_grid_view_holder_t *resolved_holder;
    egui_view_t *item_view;
    test_grid_view_holder_t *typed_holder;

    setup_grid_view();

    EGUI_TEST_ASSERT_TRUE(egui_view_grid_view_get_data_model(EGUI_VIEW_OF(&test_grid_view)) == &test_data_model);
    EGUI_TEST_ASSERT_TRUE(egui_view_grid_view_get_holder_ops(EGUI_VIEW_OF(&test_grid_view)) == &test_holder_ops);
    EGUI_TEST_ASSERT_TRUE(egui_view_grid_view_get_data_model_context(EGUI_VIEW_OF(&test_grid_view)) == &test_context);
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_grid_view_get_state_cache_entry_limit(EGUI_VIEW_OF(&test_grid_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_grid_view_get_state_cache_byte_limit(EGUI_VIEW_OF(&test_grid_view)));
    EGUI_TEST_ASSERT_TRUE(test_context.created_count > 0);
    EGUI_TEST_ASSERT_TRUE(test_context.bind_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_GRID_VIEW_MAX_ITEMS, egui_view_grid_view_get_item_count(EGUI_VIEW_OF(&test_grid_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_grid_view_get_row_count(EGUI_VIEW_OF(&test_grid_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_grid_view_find_index_by_stable_id(EGUI_VIEW_OF(&test_grid_view), 9304));

    EGUI_TEST_ASSERT_TRUE(egui_view_grid_view_resolve_item_by_stable_id(EGUI_VIEW_OF(&test_grid_view), 9304, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(4, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, entry.row_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, entry.column_index);
    EGUI_TEST_ASSERT_EQUAL_INT(9304, entry.stable_id);

    holder = egui_view_grid_view_find_holder_by_stable_id(EGUI_VIEW_OF(&test_grid_view), 9304);
    EGUI_TEST_ASSERT_NOT_NULL(holder);
    item_view = egui_view_grid_view_find_item_view_by_stable_id(EGUI_VIEW_OF(&test_grid_view), 9304);
    EGUI_TEST_ASSERT_TRUE(item_view == holder->item_view);

    EGUI_TEST_ASSERT_TRUE(egui_view_grid_view_resolve_holder_by_view(EGUI_VIEW_OF(&test_grid_view), item_view, &resolved_holder, &entry));
    EGUI_TEST_ASSERT_TRUE(resolved_holder == holder);
    EGUI_TEST_ASSERT_EQUAL_INT(4, entry.index);

    typed_holder = (test_grid_view_holder_t *)holder;
    EGUI_TEST_ASSERT_TRUE(
            egui_view_grid_view_resolve_holder_by_view(EGUI_VIEW_OF(&test_grid_view), EGUI_VIEW_OF(&typed_holder->label), &resolved_holder, &entry));
    EGUI_TEST_ASSERT_TRUE(resolved_holder == holder);
    EGUI_TEST_ASSERT_EQUAL_INT(9304, entry.stable_id);

    EGUI_TEST_ASSERT_EQUAL_INT(34, egui_view_grid_view_get_item_x(EGUI_VIEW_OF(&test_grid_view), 1));
    EGUI_TEST_ASSERT_EQUAL_INT(25, egui_view_grid_view_get_item_y(EGUI_VIEW_OF(&test_grid_view), 4));
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_grid_view_get_item_width(EGUI_VIEW_OF(&test_grid_view), 1));
    EGUI_TEST_ASSERT_EQUAL_INT(22, egui_view_grid_view_get_item_height(EGUI_VIEW_OF(&test_grid_view), 4));

    egui_view_grid_view_scroll_to_stable_id(EGUI_VIEW_OF(&test_grid_view), 9304, 4);
    layout_grid_view(0, 0, 100, 40);
    EGUI_TEST_ASSERT_EQUAL_INT(29, egui_view_grid_view_get_scroll_y(EGUI_VIEW_OF(&test_grid_view)));
}

static void test_grid_view_resize_helpers_bridge_virtual_grid(void)
{
    int32_t before_y;

    setup_grid_view();
    before_y = egui_view_grid_view_get_item_y(EGUI_VIEW_OF(&test_grid_view), 4);

    test_context.heights[1] = 30;
    egui_view_grid_view_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&test_grid_view), 9301);
    layout_grid_view(0, 0, 100, 40);

    EGUI_TEST_ASSERT_EQUAL_INT(before_y + 10, egui_view_grid_view_get_item_y(EGUI_VIEW_OF(&test_grid_view), 4));
}

void test_grid_view_run(void)
{
    EGUI_TEST_SUITE_BEGIN(grid_view);
    EGUI_TEST_RUN(test_grid_view_bridge_and_lookup_helpers);
    EGUI_TEST_RUN(test_grid_view_resize_helpers_bridge_virtual_grid);
}
