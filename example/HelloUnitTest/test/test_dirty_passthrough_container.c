#include "test_dirty_passthrough_container.h"

#include "background/egui_background_color.h"
#include "egui.h"
#include "test/egui_test.h"
#include "uicode_disp0.h"

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_dirty_passthrough_bg_param, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_dirty_passthrough_bg_params, &s_dirty_passthrough_bg_param, NULL, NULL);

static egui_background_color_t s_dirty_passthrough_bg;
static int s_dirty_passthrough_bg_ready;

static egui_core_t *test_dirty_passthrough_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void test_dirty_passthrough_ensure_bg(void)
{
    if (s_dirty_passthrough_bg_ready)
    {
        return;
    }

    egui_background_color_init_with_params((egui_background_t *)&s_dirty_passthrough_bg, &s_dirty_passthrough_bg_params);
    s_dirty_passthrough_bg_ready = 1;
}

static int test_dirty_passthrough_count(void)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr(test_dirty_passthrough_get_core());
    int count = 0;

    for (int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if (!egui_region_is_empty(&arr[i]))
        {
            count++;
        }
    }

    return count;
}

static int32_t test_dirty_passthrough_area(const egui_region_t *region)
{
    return (int32_t)region->size.width * region->size.height;
}

static int32_t test_dirty_passthrough_total_area(void)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr(test_dirty_passthrough_get_core());
    int32_t area = 0;

    for (int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if (!egui_region_is_empty(&arr[i]))
        {
            area += test_dirty_passthrough_area(&arr[i]);
        }
    }

    return area;
}

static int test_dirty_passthrough_has_region(const egui_region_t *expected)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr(test_dirty_passthrough_get_core());

    for (int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if (egui_test_region_is_same(expected, &arr[i]))
        {
            return 1;
        }
    }

    return 0;
}

static int test_dirty_passthrough_has_point(egui_dim_t x, egui_dim_t y)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr(test_dirty_passthrough_get_core());

    for (int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if (egui_region_pt_in_rect(&arr[i], x, y))
        {
            return 1;
        }
    }

    return 0;
}

static void test_dirty_passthrough_set_region(egui_view_t *view, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_view_set_position(view, x, y);
    egui_view_set_size(view, width, height);
}

static void test_dirty_passthrough_init_child(egui_view_t *child, egui_core_t *core, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_view_init(child, core);
    test_dirty_passthrough_set_region(child, x, y, width, height);
}

static void test_dirty_passthrough_add_two_children(egui_view_t *parent, egui_view_t *child_a, egui_view_t *child_b, egui_core_t *core)
{
    test_dirty_passthrough_init_child(child_a, core, 0, 20, 100, 20);
    test_dirty_passthrough_init_child(child_b, core, 0, 70, 100, 20);
    egui_view_group_add_child(parent, child_a);
    egui_view_group_add_child(parent, child_b);
}

static void test_dirty_passthrough_assert_two_vertical_sweeps_at(egui_dim_t y_a, egui_dim_t y_b)
{
    egui_region_t expected_a;
    egui_region_t expected_b;

    egui_region_init(&expected_a, 0, y_a, 100, 25);
    egui_region_init(&expected_b, 0, y_b, 100, 25);

    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_b));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(10, 50));
}

static void test_dirty_passthrough_assert_two_vertical_sweeps(void)
{
    test_dirty_passthrough_assert_two_vertical_sweeps_at(20, 70);
}

static void test_dirty_passthrough_assert_two_horizontal_sweeps_at(egui_dim_t x_a, egui_dim_t x_b)
{
    egui_region_t expected_a;
    egui_region_t expected_b;

    egui_region_init(&expected_a, x_a, 0, 25, 100);
    egui_region_init(&expected_b, x_b, 0, 25, 100);

    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_b));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(50, 10));
}

static void test_dirty_passthrough_setup_group(egui_view_group_t *group, egui_view_t *child_a, egui_view_t *child_b)
{
    egui_core_t *core = test_dirty_passthrough_get_core();

    egui_view_group_init(EGUI_VIEW_OF(group), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(group), 0, 0, 100, 120);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(group), 1);
    test_dirty_passthrough_add_two_children(EGUI_VIEW_OF(group), child_a, child_b, core);
    EGUI_VIEW_OF(group)->api->calculate_layout(EGUI_VIEW_OF(group));
    egui_core_clear_region_dirty(core);
}

static void test_dirty_passthrough_setup_spaced_linear_layout(egui_view_linearlayout_t *layout, egui_view_t *child_a, egui_view_t *child_b, egui_core_t *core,
                                                              egui_dim_t layout_width, egui_dim_t layout_height, egui_dim_t child_width, uint8_t align_type)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(layout), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(layout), 0, 0, layout_width, layout_height);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(layout), align_type);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(layout), 1);

    test_dirty_passthrough_init_child(child_a, core, 0, 0, child_width, 20);
    egui_view_set_margin(child_a, 0, 0, 20, 30);
    test_dirty_passthrough_init_child(child_b, core, 0, 0, child_width, 20);
    egui_view_group_add_child(EGUI_VIEW_OF(layout), child_a);
    egui_view_group_add_child(EGUI_VIEW_OF(layout), child_b);
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(layout));
}

static void test_dirty_passthrough_setup_viewpage_scroll_linear(egui_view_viewpage_t *viewpage, egui_view_scroll_t *scroll, egui_view_linearlayout_t *layout,
                                                                egui_view_group_t *empty_page, egui_view_t *child_a, egui_view_t *child_b, egui_core_t *core,
                                                                int scroll_has_background)
{
    egui_view_viewpage_init(EGUI_VIEW_OF(viewpage), core);
    egui_view_set_position(EGUI_VIEW_OF(viewpage), 0, 0);
    egui_view_viewpage_set_size(EGUI_VIEW_OF(viewpage), 120, 100);

    egui_view_scroll_init(EGUI_VIEW_OF(scroll), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(scroll), 0, 0, 120, 100);
    if (scroll_has_background)
    {
        test_dirty_passthrough_ensure_bg();
        egui_view_set_background(EGUI_VIEW_OF(scroll), (egui_background_t *)&s_dirty_passthrough_bg.base);
    }

    test_dirty_passthrough_setup_spaced_linear_layout(layout, child_a, child_b, core, 100, 160, 100, EGUI_ALIGN_TOP_LEFT);
    egui_view_scroll_add_child(EGUI_VIEW_OF(scroll), EGUI_VIEW_OF(layout));
    egui_view_scroll_layout_childs(EGUI_VIEW_OF(scroll));

    egui_view_group_init(EGUI_VIEW_OF(empty_page), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(empty_page), 0, 0, 120, 100);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(empty_page), 1);

    egui_view_viewpage_add_child(EGUI_VIEW_OF(viewpage), EGUI_VIEW_OF(scroll));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(viewpage), EGUI_VIEW_OF(empty_page));
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(viewpage));
}

typedef struct test_dirty_passthrough_leaf_host
{
    egui_view_group_t root;
    egui_view_t leaf;
} test_dirty_passthrough_leaf_host_t;

typedef struct test_dirty_passthrough_virtual_viewport_context
{
    egui_core_t *core;
    test_dirty_passthrough_leaf_host_t hosts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
    uint8_t next_host;
    uint32_t item_count;
    int32_t item_height;
} test_dirty_passthrough_virtual_viewport_context_t;

typedef struct test_dirty_passthrough_list_view_holder
{
    egui_view_list_view_holder_t base;
    test_dirty_passthrough_leaf_host_t host;
} test_dirty_passthrough_list_view_holder_t;

typedef struct test_dirty_passthrough_list_view_context
{
    egui_core_t *core;
    test_dirty_passthrough_list_view_holder_t holders[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
    uint8_t next_holder;
    uint32_t item_count;
    int32_t item_height;
} test_dirty_passthrough_list_view_context_t;

typedef struct test_dirty_passthrough_grid_view_holder
{
    egui_view_grid_view_holder_t base;
    test_dirty_passthrough_leaf_host_t host;
} test_dirty_passthrough_grid_view_holder_t;

typedef struct test_dirty_passthrough_grid_view_context
{
    egui_core_t *core;
    test_dirty_passthrough_grid_view_holder_t holders[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS * EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS];
    uint8_t next_holder;
    uint32_t item_count;
    int32_t item_height;
} test_dirty_passthrough_grid_view_context_t;

static void test_dirty_passthrough_init_leaf_host(test_dirty_passthrough_leaf_host_t *host, egui_core_t *core, egui_dim_t x, egui_dim_t y, egui_dim_t width,
                                                  egui_dim_t height)
{
    egui_view_group_init(EGUI_VIEW_OF(&host->root), core);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&host->root), 1);
    test_dirty_passthrough_init_child(&host->leaf, core, x, y, width, height);
    egui_view_group_add_child(EGUI_VIEW_OF(&host->root), &host->leaf);
}

static uint32_t test_dirty_passthrough_virtual_get_count(void *adapter_context)
{
    test_dirty_passthrough_virtual_viewport_context_t *context = (test_dirty_passthrough_virtual_viewport_context_t *)adapter_context;
    return context->item_count;
}

static uint32_t test_dirty_passthrough_virtual_get_stable_id(void *adapter_context, uint32_t index)
{
    test_dirty_passthrough_virtual_viewport_context_t *context = (test_dirty_passthrough_virtual_viewport_context_t *)adapter_context;
    return index < context->item_count ? 1000U + index : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t test_dirty_passthrough_virtual_find_index_by_stable_id(void *adapter_context, uint32_t stable_id)
{
    test_dirty_passthrough_virtual_viewport_context_t *context = (test_dirty_passthrough_virtual_viewport_context_t *)adapter_context;

    if (stable_id < 1000U || stable_id >= 1000U + context->item_count)
    {
        return -1;
    }
    return (int32_t)(stable_id - 1000U);
}

static int32_t test_dirty_passthrough_virtual_measure_main_size(void *adapter_context, uint32_t index, int32_t cross_size_hint)
{
    test_dirty_passthrough_virtual_viewport_context_t *context = (test_dirty_passthrough_virtual_viewport_context_t *)adapter_context;

    EGUI_UNUSED(index);
    EGUI_UNUSED(cross_size_hint);
    return context->item_height;
}

static egui_view_t *test_dirty_passthrough_virtual_create_view(void *adapter_context, uint16_t view_type)
{
    test_dirty_passthrough_virtual_viewport_context_t *context = (test_dirty_passthrough_virtual_viewport_context_t *)adapter_context;
    test_dirty_passthrough_leaf_host_t *host;

    EGUI_UNUSED(view_type);
    if (context->next_host >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
    {
        return NULL;
    }

    host = &context->hosts[context->next_host++];
    test_dirty_passthrough_init_leaf_host(host, context->core, 10, 10, 40, 10);
    return EGUI_VIEW_OF(&host->root);
}

static const egui_view_virtual_viewport_adapter_t test_dirty_passthrough_virtual_adapter = {
        .get_count = test_dirty_passthrough_virtual_get_count,
        .get_stable_id = test_dirty_passthrough_virtual_get_stable_id,
        .find_index_by_stable_id = test_dirty_passthrough_virtual_find_index_by_stable_id,
        .get_view_type = NULL,
        .measure_main_size = test_dirty_passthrough_virtual_measure_main_size,
        .create_view = test_dirty_passthrough_virtual_create_view,
        .destroy_view = NULL,
        .bind_view = NULL,
        .unbind_view = NULL,
        .should_keep_alive = NULL,
        .save_state = NULL,
        .restore_state = NULL,
};

static uint32_t test_dirty_passthrough_list_view_get_count(void *data_model_context)
{
    test_dirty_passthrough_list_view_context_t *context = (test_dirty_passthrough_list_view_context_t *)data_model_context;
    return context->item_count;
}

static uint32_t test_dirty_passthrough_list_view_get_stable_id(void *data_model_context, uint32_t index)
{
    test_dirty_passthrough_list_view_context_t *context = (test_dirty_passthrough_list_view_context_t *)data_model_context;
    return index < context->item_count ? 2000U + index : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t test_dirty_passthrough_list_view_find_index_by_stable_id(void *data_model_context, uint32_t stable_id)
{
    test_dirty_passthrough_list_view_context_t *context = (test_dirty_passthrough_list_view_context_t *)data_model_context;

    if (stable_id < 2000U || stable_id >= 2000U + context->item_count)
    {
        return -1;
    }
    return (int32_t)(stable_id - 2000U);
}

static int32_t test_dirty_passthrough_list_view_measure_item_height(void *data_model_context, uint32_t index, int32_t width_hint)
{
    test_dirty_passthrough_list_view_context_t *context = (test_dirty_passthrough_list_view_context_t *)data_model_context;

    EGUI_UNUSED(index);
    EGUI_UNUSED(width_hint);
    return context->item_height;
}

static egui_view_list_view_holder_t *test_dirty_passthrough_list_view_create_holder(void *data_model_context, uint16_t view_type)
{
    test_dirty_passthrough_list_view_context_t *context = (test_dirty_passthrough_list_view_context_t *)data_model_context;
    test_dirty_passthrough_list_view_holder_t *holder;

    EGUI_UNUSED(view_type);
    if (context->next_holder >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
    {
        return NULL;
    }

    holder = &context->holders[context->next_holder++];
    egui_api_memset(holder, 0, sizeof(*holder));
    test_dirty_passthrough_init_leaf_host(&holder->host, context->core, 10, 10, 40, 10);
    holder->base.item_view = EGUI_VIEW_OF(&holder->host.root);
    return &holder->base;
}

static void test_dirty_passthrough_list_view_destroy_holder(void *data_model_context, egui_view_list_view_holder_t *holder, uint16_t view_type)
{
    EGUI_UNUSED(data_model_context);
    EGUI_UNUSED(holder);
    EGUI_UNUSED(view_type);
}

static const egui_view_list_view_data_model_t test_dirty_passthrough_list_view_model = {
        .get_count = test_dirty_passthrough_list_view_get_count,
        .get_stable_id = test_dirty_passthrough_list_view_get_stable_id,
        .find_index_by_stable_id = test_dirty_passthrough_list_view_find_index_by_stable_id,
        .get_view_type = NULL,
        .measure_item_height = test_dirty_passthrough_list_view_measure_item_height,
        .default_view_type = 0,
};

static const egui_view_list_view_holder_ops_t test_dirty_passthrough_list_view_holder_ops = {
        .create_holder = test_dirty_passthrough_list_view_create_holder,
        .destroy_holder = test_dirty_passthrough_list_view_destroy_holder,
        .bind_holder = NULL,
        .unbind_holder = NULL,
        .should_keep_alive = NULL,
        .save_holder_state = NULL,
        .restore_holder_state = NULL,
};

static uint32_t test_dirty_passthrough_grid_view_get_count(void *data_model_context)
{
    test_dirty_passthrough_grid_view_context_t *context = (test_dirty_passthrough_grid_view_context_t *)data_model_context;
    return context->item_count;
}

static uint32_t test_dirty_passthrough_grid_view_get_stable_id(void *data_model_context, uint32_t index)
{
    test_dirty_passthrough_grid_view_context_t *context = (test_dirty_passthrough_grid_view_context_t *)data_model_context;
    return index < context->item_count ? 3000U + index : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t test_dirty_passthrough_grid_view_find_index_by_stable_id(void *data_model_context, uint32_t stable_id)
{
    test_dirty_passthrough_grid_view_context_t *context = (test_dirty_passthrough_grid_view_context_t *)data_model_context;

    if (stable_id < 3000U || stable_id >= 3000U + context->item_count)
    {
        return -1;
    }
    return (int32_t)(stable_id - 3000U);
}

static int32_t test_dirty_passthrough_grid_view_measure_item_height(void *data_model_context, uint32_t index, int32_t width_hint)
{
    test_dirty_passthrough_grid_view_context_t *context = (test_dirty_passthrough_grid_view_context_t *)data_model_context;

    EGUI_UNUSED(index);
    EGUI_UNUSED(width_hint);
    return context->item_height;
}

static egui_view_grid_view_holder_t *test_dirty_passthrough_grid_view_create_holder(void *data_model_context, uint16_t view_type)
{
    test_dirty_passthrough_grid_view_context_t *context = (test_dirty_passthrough_grid_view_context_t *)data_model_context;
    test_dirty_passthrough_grid_view_holder_t *holder;

    EGUI_UNUSED(view_type);
    if (context->next_holder >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS * EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS)
    {
        return NULL;
    }

    holder = &context->holders[context->next_holder++];
    egui_api_memset(holder, 0, sizeof(*holder));
    test_dirty_passthrough_init_leaf_host(&holder->host, context->core, 5, 5, 20, 10);
    holder->base.item_view = EGUI_VIEW_OF(&holder->host.root);
    return &holder->base;
}

static void test_dirty_passthrough_grid_view_destroy_holder(void *data_model_context, egui_view_grid_view_holder_t *holder, uint16_t view_type)
{
    EGUI_UNUSED(data_model_context);
    EGUI_UNUSED(holder);
    EGUI_UNUSED(view_type);
}

static const egui_view_grid_view_data_model_t test_dirty_passthrough_grid_view_model = {
        .get_count = test_dirty_passthrough_grid_view_get_count,
        .get_stable_id = test_dirty_passthrough_grid_view_get_stable_id,
        .find_index_by_stable_id = test_dirty_passthrough_grid_view_find_index_by_stable_id,
        .get_view_type = NULL,
        .measure_item_height = test_dirty_passthrough_grid_view_measure_item_height,
        .default_view_type = 0,
};

static const egui_view_grid_view_holder_ops_t test_dirty_passthrough_grid_view_holder_ops = {
        .create_holder = test_dirty_passthrough_grid_view_create_holder,
        .destroy_holder = test_dirty_passthrough_grid_view_destroy_holder,
        .bind_holder = NULL,
        .unbind_holder = NULL,
        .should_keep_alive = NULL,
        .save_holder_state = NULL,
        .restore_holder_state = NULL,
};

static int32_t test_dirty_passthrough_measure_sparse_vertical_group(int dirty_passthrough)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_group_t group;
    egui_view_t child_a;
    egui_view_t child_b;

    egui_view_group_init(EGUI_VIEW_OF(&group), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&group), 0, 0, 100, 160);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&group), dirty_passthrough);
    test_dirty_passthrough_init_child(&child_a, core, 0, 20, 100, 20);
    test_dirty_passthrough_init_child(&child_b, core, 0, 110, 100, 20);
    egui_view_group_add_child(EGUI_VIEW_OF(&group), &child_a);
    egui_view_group_add_child(EGUI_VIEW_OF(&group), &child_b);
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&group), 0, 5);
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));

    return test_dirty_passthrough_total_area();
}

static int32_t test_dirty_passthrough_measure_sparse_horizontal_group(int dirty_passthrough)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_group_t group;
    egui_view_t child_a;
    egui_view_t child_b;

    egui_view_group_init(EGUI_VIEW_OF(&group), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&group), 0, 0, 160, 100);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&group), dirty_passthrough);
    test_dirty_passthrough_init_child(&child_a, core, 20, 0, 20, 100);
    test_dirty_passthrough_init_child(&child_b, core, 110, 0, 20, 100);
    egui_view_group_add_child(EGUI_VIEW_OF(&group), &child_a);
    egui_view_group_add_child(EGUI_VIEW_OF(&group), &child_b);
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&group), 5, 0);
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));

    return test_dirty_passthrough_total_area();
}

static int32_t test_dirty_passthrough_measure_nested_sparse_page(int page_dirty_passthrough)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_group_t parent;
    egui_view_group_t page;
    egui_view_t child_a;
    egui_view_t child_b;

    egui_view_group_init(EGUI_VIEW_OF(&parent), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&parent), 0, 0, 160, 100);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&parent), 1);

    egui_view_group_init(EGUI_VIEW_OF(&page), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&page), 0, 0, 160, 100);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&page), page_dirty_passthrough);

    test_dirty_passthrough_init_child(&child_a, core, 20, 20, 20, 20);
    test_dirty_passthrough_init_child(&child_b, core, 110, 60, 20, 20);
    egui_view_group_add_child(EGUI_VIEW_OF(&page), &child_a);
    egui_view_group_add_child(EGUI_VIEW_OF(&page), &child_b);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent), EGUI_VIEW_OF(&page));
    EGUI_VIEW_OF(&parent)->api->calculate_layout(EGUI_VIEW_OF(&parent));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&parent), 5, 0);
    EGUI_VIEW_OF(&parent)->api->calculate_layout(EGUI_VIEW_OF(&parent));

    return test_dirty_passthrough_total_area();
}

static void test_dirty_passthrough_group_translation_emits_per_child_swept(void)
{
    egui_view_group_t group;
    egui_view_t child_a;
    egui_view_t child_b;

    test_dirty_passthrough_setup_group(&group, &child_a, &child_b);
    egui_view_set_position(EGUI_VIEW_OF(&group), 0, 5);
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));

    test_dirty_passthrough_assert_two_vertical_sweeps();
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_total_area() < test_dirty_passthrough_area(&EGUI_VIEW_OF(&group)->region_screen));
}

static void test_dirty_passthrough_group_size_change_cascades_to_children(void)
{
    egui_view_group_t group;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t expected_a;
    egui_region_t expected_b;

    test_dirty_passthrough_setup_group(&group, &child_a, &child_b);
    egui_view_set_size(EGUI_VIEW_OF(&group), 120, 120);
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));

    egui_region_init(&expected_a, 0, 20, 100, 20);
    egui_region_init(&expected_b, 0, 70, 100, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_b));
}

static void test_dirty_passthrough_group_background_change_still_self_full_rect(void)
{
    egui_view_group_t group;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t *arr = egui_core_get_region_dirty_arr(test_dirty_passthrough_get_core());

    test_dirty_passthrough_setup_group(&group, &child_a, &child_b);
    test_dirty_passthrough_ensure_bg();
    egui_view_set_background(EGUI_VIEW_OF(&group), (egui_background_t *)&s_dirty_passthrough_bg.base);

    EGUI_TEST_ASSERT_EQUAL_INT(1, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_REGION_EQUAL(&EGUI_VIEW_OF(&group)->region_screen, &arr[0]);
}

static void test_dirty_passthrough_group_background_translation_marks_self_swept(void)
{
    egui_view_group_t group;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t expected;

    test_dirty_passthrough_setup_group(&group, &child_a, &child_b);
    test_dirty_passthrough_ensure_bg();
    egui_view_set_background(EGUI_VIEW_OF(&group), (egui_background_t *)&s_dirty_passthrough_bg.base);
    egui_core_clear_region_dirty(test_dirty_passthrough_get_core());

    egui_view_set_position(EGUI_VIEW_OF(&group), 0, 5);
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));

    egui_region_init(&expected, 0, 0, 100, 125);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected));
}

static void test_dirty_passthrough_group_background_size_change_marks_self_swept(void)
{
    egui_view_group_t group;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t expected;

    test_dirty_passthrough_setup_group(&group, &child_a, &child_b);
    test_dirty_passthrough_ensure_bg();
    egui_view_set_background(EGUI_VIEW_OF(&group), (egui_background_t *)&s_dirty_passthrough_bg.base);
    egui_core_clear_region_dirty(test_dirty_passthrough_get_core());

    egui_view_set_size(EGUI_VIEW_OF(&group), 120, 120);
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));

    egui_region_init(&expected, 0, 0, 120, 120);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected));
}

static void test_dirty_passthrough_nested_background_group_translation_marks_child_self_swept(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_group_t parent;
    egui_view_group_t child_group;
    egui_view_t leaf;
    egui_region_t expected;

    egui_view_group_init(EGUI_VIEW_OF(&parent), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&parent), 0, 0, 160, 120);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&parent), 1);

    egui_view_group_init(EGUI_VIEW_OF(&child_group), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&child_group), 10, 10, 80, 40);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&child_group), 1);
    test_dirty_passthrough_ensure_bg();
    egui_view_set_background(EGUI_VIEW_OF(&child_group), (egui_background_t *)&s_dirty_passthrough_bg.base);

    test_dirty_passthrough_init_child(&leaf, core, 0, 0, 20, 20);
    egui_view_group_add_child(EGUI_VIEW_OF(&child_group), &leaf);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent), EGUI_VIEW_OF(&child_group));
    EGUI_VIEW_OF(&parent)->api->calculate_layout(EGUI_VIEW_OF(&parent));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&parent), 5, 0);
    EGUI_VIEW_OF(&parent)->api->calculate_layout(EGUI_VIEW_OF(&parent));

    egui_region_init(&expected, 10, 10, 85, 40);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected));
}

static void test_dirty_passthrough_group_empty_container_no_op(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_group_t group;

    egui_view_group_init(EGUI_VIEW_OF(&group), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&group), 0, 0, 100, 120);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&group), 1);
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&group), 0, 5);
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));

    EGUI_TEST_ASSERT_EQUAL_INT(0, test_dirty_passthrough_count());
}

static void test_dirty_passthrough_group_visibility_change_emits_child_regions(void)
{
    egui_view_group_t group;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t expected_a;
    egui_region_t expected_b;

    test_dirty_passthrough_setup_group(&group, &child_a, &child_b);
    egui_view_set_visible(EGUI_VIEW_OF(&group), 0);

    egui_region_init(&expected_a, 0, 20, 100, 20);
    egui_region_init(&expected_b, 0, 70, 100, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_b));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(10, 50));
}

static void test_dirty_passthrough_linearlayout_translation_emits_leaf_sweeps(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_linearlayout_t layout;
    egui_view_t child_a;
    egui_view_t child_b;

    test_dirty_passthrough_setup_spaced_linear_layout(&layout, &child_a, &child_b, core, 100, 140, 100, EGUI_ALIGN_TOP_LEFT);
    EGUI_VIEW_OF(&layout)->api->calculate_layout(EGUI_VIEW_OF(&layout));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&layout), 0, 5);
    EGUI_VIEW_OF(&layout)->api->calculate_layout(EGUI_VIEW_OF(&layout));

    test_dirty_passthrough_assert_two_vertical_sweeps();
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_total_area() < test_dirty_passthrough_area(&EGUI_VIEW_OF(&layout)->region_screen));
}

static void test_dirty_passthrough_setup_scroll(egui_view_scroll_t *scroll, egui_view_t *child_a, egui_view_t *child_b)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_t *container;

    egui_view_scroll_init(EGUI_VIEW_OF(scroll), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(scroll), 0, 0, 100, 120);
    container = EGUI_VIEW_OF(&scroll->container);
    test_dirty_passthrough_set_region(container, 0, 0, 100, 200);
    test_dirty_passthrough_add_two_children(container, child_a, child_b, core);
    EGUI_VIEW_OF(scroll)->api->calculate_layout(EGUI_VIEW_OF(scroll));
    egui_core_clear_region_dirty(core);
}

static void test_dirty_passthrough_scroll_inner_drag_emits_per_child_swept(void)
{
    egui_view_scroll_t scroll;
    egui_view_t child_a;
    egui_view_t child_b;

    test_dirty_passthrough_setup_scroll(&scroll, &child_a, &child_b);
    egui_view_set_position(EGUI_VIEW_OF(&scroll.container), 0, -5);
    EGUI_VIEW_OF(&scroll)->api->calculate_layout(EGUI_VIEW_OF(&scroll));

    test_dirty_passthrough_assert_two_vertical_sweeps_at(15, 65);
}

static void test_dirty_passthrough_scroll_viewport_clips_inner_drag_sweeps(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_scroll_t scroll;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t expected_a;
    egui_region_t expected_b;

    test_dirty_passthrough_setup_scroll(&scroll, &child_a, &child_b);
    egui_view_set_position(EGUI_VIEW_OF(&scroll), 0, 40);
    EGUI_VIEW_OF(&scroll)->api->calculate_layout(EGUI_VIEW_OF(&scroll));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&scroll.container), 0, -25);
    EGUI_VIEW_OF(&scroll)->api->calculate_layout(EGUI_VIEW_OF(&scroll));

    egui_region_init(&expected_a, 0, 40, 100, 40);
    egui_region_init(&expected_b, 0, 85, 100, 45);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_b));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(10, 39));
}

static void test_dirty_passthrough_scroll_first_attach_cascades(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_scroll_t scroll;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t expected_a;
    egui_region_t expected_b;

    egui_view_scroll_init(EGUI_VIEW_OF(&scroll), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&scroll), 0, 0, 100, 120);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&scroll.container), 0, 0, 100, 200);
    test_dirty_passthrough_add_two_children(EGUI_VIEW_OF(&scroll.container), &child_a, &child_b, core);
    egui_core_clear_region_dirty(core);

    EGUI_VIEW_OF(&scroll)->api->calculate_layout(EGUI_VIEW_OF(&scroll));

    egui_region_init(&expected_a, 0, 20, 100, 20);
    egui_region_init(&expected_b, 0, 70, 100, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_b));
}

static void test_dirty_passthrough_scroll_background_change_self_full_rect(void)
{
    egui_view_scroll_t scroll;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t *arr = egui_core_get_region_dirty_arr(test_dirty_passthrough_get_core());

    test_dirty_passthrough_setup_scroll(&scroll, &child_a, &child_b);
    test_dirty_passthrough_ensure_bg();
    egui_view_set_background(EGUI_VIEW_OF(&scroll), (egui_background_t *)&s_dirty_passthrough_bg.base);

    EGUI_TEST_ASSERT_EQUAL_INT(1, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_REGION_EQUAL(&EGUI_VIEW_OF(&scroll)->region_screen, &arr[0]);
}

static void test_dirty_passthrough_scroll_with_background_stable_drag_emits_child_sweeps(void)
{
    egui_view_scroll_t scroll;
    egui_view_t child_a;
    egui_view_t child_b;

    test_dirty_passthrough_setup_scroll(&scroll, &child_a, &child_b);
    test_dirty_passthrough_ensure_bg();
    egui_view_set_background(EGUI_VIEW_OF(&scroll), (egui_background_t *)&s_dirty_passthrough_bg.base);
    egui_core_clear_region_dirty(test_dirty_passthrough_get_core());

    egui_view_scroll_start_container_scroll(EGUI_VIEW_OF(&scroll), -5);
    EGUI_VIEW_OF(&scroll)->api->calculate_layout(EGUI_VIEW_OF(&scroll));

    test_dirty_passthrough_assert_two_vertical_sweeps_at(15, 65);
}

static void test_dirty_passthrough_scroll_empty_no_op(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_scroll_t scroll;

    egui_view_scroll_init(EGUI_VIEW_OF(&scroll), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&scroll), 0, 0, 100, 120);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&scroll.container), 0, 0, 100, 200);
    EGUI_VIEW_OF(&scroll)->api->calculate_layout(EGUI_VIEW_OF(&scroll));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&scroll.container), 0, 5);
    EGUI_VIEW_OF(&scroll)->api->calculate_layout(EGUI_VIEW_OF(&scroll));

    EGUI_TEST_ASSERT_EQUAL_INT(0, test_dirty_passthrough_count());
}

static void test_dirty_passthrough_scrollbar_thumb_dirty_tracks_scroll(void)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    egui_view_scroll_t scroll;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_dim_t bar_x;
    egui_dim_t old_bar_y;
    egui_dim_t new_bar_y;
    egui_dim_t view_height;
    egui_dim_t content_height;
    egui_dim_t track_length;
    egui_dim_t thumb_length;
    egui_dim_t thumb_travel;
    egui_dim_t max_scroll;

    test_dirty_passthrough_setup_scroll(&scroll, &child_a, &child_b);
    egui_view_scroll_set_scrollbar_enabled(EGUI_VIEW_OF(&scroll), 1);
    egui_core_clear_region_dirty(test_dirty_passthrough_get_core());

    egui_view_scroll_start_container_scroll(EGUI_VIEW_OF(&scroll), -5);

    bar_x = EGUI_VIEW_OF(&scroll)->region_screen.location.x + EGUI_VIEW_OF(&scroll)->region.size.width - EGUI_THEME_SCROLLBAR_THICKNESS -
            EGUI_THEME_SCROLLBAR_MARGIN;
    old_bar_y = EGUI_VIEW_OF(&scroll)->region_screen.location.y + EGUI_THEME_SCROLLBAR_MARGIN;
    view_height = EGUI_VIEW_OF(&scroll)->region.size.height;
    content_height = EGUI_VIEW_OF(&scroll.container)->region.size.height;
    track_length = view_height - 2 * EGUI_THEME_SCROLLBAR_MARGIN;
    thumb_length = (egui_dim_t)(((int32_t)track_length * view_height) / content_height);
    if (thumb_length < EGUI_THEME_SCROLLBAR_MIN_LENGTH)
    {
        thumb_length = EGUI_THEME_SCROLLBAR_MIN_LENGTH;
    }
    if (thumb_length > track_length)
    {
        thumb_length = track_length;
    }
    thumb_travel = track_length - thumb_length;
    max_scroll = content_height - view_height;
    new_bar_y = old_bar_y + (egui_dim_t)(((int32_t)5 * thumb_travel) / max_scroll);

    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(bar_x, old_bar_y));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(bar_x, new_bar_y));
#endif
}

static void test_dirty_passthrough_setup_viewpage(egui_view_viewpage_t *viewpage, egui_view_t *child_a, egui_view_t *child_b)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_t *container;

    egui_view_viewpage_init(EGUI_VIEW_OF(viewpage), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(viewpage), 0, 0, 120, 100);
    container = EGUI_VIEW_OF(&viewpage->container);
    test_dirty_passthrough_set_region(container, 0, 0, 200, 100);
    test_dirty_passthrough_init_child(child_a, core, 20, 0, 20, 100);
    test_dirty_passthrough_init_child(child_b, core, 70, 0, 20, 100);
    egui_view_group_add_child(container, child_a);
    egui_view_group_add_child(container, child_b);
    EGUI_VIEW_OF(viewpage)->api->calculate_layout(EGUI_VIEW_OF(viewpage));
    egui_core_clear_region_dirty(core);
}

static void test_dirty_passthrough_viewpage_page_animation_emits_per_child_swept(void)
{
    egui_view_viewpage_t viewpage;
    egui_view_t child_a;
    egui_view_t child_b;

    test_dirty_passthrough_setup_viewpage(&viewpage, &child_a, &child_b);
    egui_view_set_position(EGUI_VIEW_OF(&viewpage.container), -5, 0);
    EGUI_VIEW_OF(&viewpage)->api->calculate_layout(EGUI_VIEW_OF(&viewpage));

    test_dirty_passthrough_assert_two_horizontal_sweeps_at(15, 65);
}

static void test_dirty_passthrough_viewpage_empty_no_op(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_viewpage_t viewpage;

    egui_view_viewpage_init(EGUI_VIEW_OF(&viewpage), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&viewpage), 0, 0, 120, 100);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&viewpage.container), 0, 0, 200, 100);
    EGUI_VIEW_OF(&viewpage)->api->calculate_layout(EGUI_VIEW_OF(&viewpage));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&viewpage.container), 5, 0);
    EGUI_VIEW_OF(&viewpage)->api->calculate_layout(EGUI_VIEW_OF(&viewpage));

    EGUI_TEST_ASSERT_EQUAL_INT(0, test_dirty_passthrough_count());
}

static void test_dirty_passthrough_viewpage_linear_page_drag_emits_leaf_sweeps(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_viewpage_t viewpage;
    egui_view_linearlayout_t linear_page;
    egui_view_group_t empty_page;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t expected_a;
    egui_region_t expected_b;

    egui_view_viewpage_init(EGUI_VIEW_OF(&viewpage), core);
    egui_view_set_position(EGUI_VIEW_OF(&viewpage), 0, 0);
    egui_view_viewpage_set_size(EGUI_VIEW_OF(&viewpage), 120, 120);

    test_dirty_passthrough_setup_spaced_linear_layout(&linear_page, &child_a, &child_b, core, 120, 120, 80, EGUI_ALIGN_TOP_MID);

    egui_view_group_init(EGUI_VIEW_OF(&empty_page), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&empty_page), 0, 0, 120, 120);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&empty_page), 1);

    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&linear_page));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&empty_page));
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));
    EGUI_VIEW_OF(&viewpage)->api->calculate_layout(EGUI_VIEW_OF(&viewpage));
    egui_core_clear_region_dirty(core);

    egui_view_viewpage_start_container_scroll(EGUI_VIEW_OF(&viewpage), -5);
    EGUI_VIEW_OF(&viewpage)->api->calculate_layout(EGUI_VIEW_OF(&viewpage));

    egui_region_init(&expected_a, 15, 20, 85, 20);
    egui_region_init(&expected_b, 15, 70, 85, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_b));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(105, 20));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(50, 50));
}

static void test_dirty_passthrough_mixed_viewpage_scroll_linearlayout_drag_emits_leaf_regions(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_viewpage_t viewpage;
    egui_view_scroll_t scroll;
    egui_view_linearlayout_t layout;
    egui_view_group_t empty_page;
    egui_view_t child_a;
    egui_view_t child_b;

    test_dirty_passthrough_setup_viewpage_scroll_linear(&viewpage, &scroll, &layout, &empty_page, &child_a, &child_b, core, 1);
    EGUI_VIEW_OF(&viewpage)->api->calculate_layout(EGUI_VIEW_OF(&viewpage));
    egui_core_clear_region_dirty(core);

    egui_view_scroll_start_container_scroll(EGUI_VIEW_OF(&scroll), -5);
    EGUI_VIEW_OF(&viewpage)->api->calculate_layout(EGUI_VIEW_OF(&viewpage));

    test_dirty_passthrough_assert_two_vertical_sweeps_at(15, 65);
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(110, 20));
}

static void test_dirty_passthrough_virtual_viewport_background_scroll_emits_leaf_regions(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_virtual_viewport_t viewport;
    test_dirty_passthrough_virtual_viewport_context_t context;
    egui_view_virtual_viewport_params_t params = {
            .region = {{0, 0}, {100, 100}},
            .orientation = EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL,
            .overscan_before = 0,
            .overscan_after = 0,
            .max_keepalive_slots = 0,
            .estimated_item_extent = 40,
    };
    egui_view_virtual_viewport_setup_t setup = {
            .params = &params,
            .adapter = &test_dirty_passthrough_virtual_adapter,
            .adapter_context = &context,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };
    egui_region_t expected_a;

    egui_api_memset(&context, 0, sizeof(context));
    context.core = core;
    context.item_count = 8;
    context.item_height = 40;

    egui_view_virtual_viewport_init_with_setup(EGUI_VIEW_OF(&viewport), core, &setup);
    test_dirty_passthrough_ensure_bg();
    egui_view_set_background(EGUI_VIEW_OF(&viewport), (egui_background_t *)&s_dirty_passthrough_bg.base);
    EGUI_VIEW_OF(&viewport)->api->calculate_layout(EGUI_VIEW_OF(&viewport));
    egui_core_clear_region_dirty(core);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_dirty_passthrough(EGUI_VIEW_OF(&viewport)));
    EGUI_TEST_ASSERT_TRUE(egui_view_get_dirty_passthrough(egui_view_virtual_viewport_get_content_layer(EGUI_VIEW_OF(&viewport))));

    egui_view_virtual_viewport_scroll_by(EGUI_VIEW_OF(&viewport), 5);
    EGUI_VIEW_OF(&viewport)->api->calculate_layout(EGUI_VIEW_OF(&viewport));

    egui_region_init(&expected_a, 10, 5, 40, 15);
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(10, 45));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(10, 85));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(80, 10));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(10, 30));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_total_area() <= 2800);
}

static void test_dirty_passthrough_list_view_scroll_emits_holder_leaf_regions(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_list_view_t list_view;
    test_dirty_passthrough_list_view_context_t context;
    egui_view_list_view_holder_t *holder;
    egui_view_list_view_params_t params = {
            .region = {{0, 0}, {100, 100}},
            .overscan_before = 0,
            .overscan_after = 0,
            .max_keepalive_slots = 0,
            .estimated_item_height = 40,
    };
    egui_view_list_view_setup_t setup = {
            .params = &params,
            .data_model = &test_dirty_passthrough_list_view_model,
            .holder_ops = &test_dirty_passthrough_list_view_holder_ops,
            .data_model_context = &context,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };
    egui_region_t expected_a;

    egui_api_memset(&context, 0, sizeof(context));
    context.core = core;
    context.item_count = 8;
    context.item_height = 40;

    egui_view_list_view_init_with_setup(EGUI_VIEW_OF(&list_view), core, &setup);
    EGUI_VIEW_OF(&list_view)->api->calculate_layout(EGUI_VIEW_OF(&list_view));
    egui_core_clear_region_dirty(core);

    holder = egui_view_list_view_find_holder_by_stable_id(EGUI_VIEW_OF(&list_view), 2000U);
    EGUI_TEST_ASSERT_NOT_NULL(holder);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_dirty_passthrough(holder->host_view));

    egui_view_list_view_scroll_by(EGUI_VIEW_OF(&list_view), 5);
    EGUI_VIEW_OF(&list_view)->api->calculate_layout(EGUI_VIEW_OF(&list_view));

    egui_region_init(&expected_a, 10, 5, 40, 15);
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(10, 45));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(10, 85));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(70, 10));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(10, 30));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_total_area() <= 2800);
}

static void test_dirty_passthrough_grid_view_scroll_emits_cell_leaf_regions(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_grid_view_t grid_view;
    test_dirty_passthrough_grid_view_context_t context;
    egui_view_grid_view_holder_t *holder;
    egui_view_grid_view_params_t params = {
            .region = {{0, 0}, {100, 90}},
            .column_count = 2,
            .overscan_before = 0,
            .overscan_after = 0,
            .max_keepalive_slots = 0,
            .column_spacing = 10,
            .row_spacing = 10,
            .estimated_item_height = 30,
    };
    egui_view_grid_view_setup_t setup = {
            .params = &params,
            .data_model = &test_dirty_passthrough_grid_view_model,
            .holder_ops = &test_dirty_passthrough_grid_view_holder_ops,
            .data_model_context = &context,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };

    egui_api_memset(&context, 0, sizeof(context));
    context.core = core;
    context.item_count = 8;
    context.item_height = 30;

    egui_view_grid_view_init_with_setup(EGUI_VIEW_OF(&grid_view), core, &setup);
    EGUI_VIEW_OF(&grid_view)->api->calculate_layout(EGUI_VIEW_OF(&grid_view));
    egui_core_clear_region_dirty(core);

    holder = egui_view_grid_view_find_holder_by_stable_id(EGUI_VIEW_OF(&grid_view), 3000U);
    EGUI_TEST_ASSERT_NOT_NULL(holder);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_dirty_passthrough(holder->host_view));

    egui_view_grid_view_scroll_by(EGUI_VIEW_OF(&grid_view), 5);
    EGUI_VIEW_OF(&grid_view)->api->calculate_layout(EGUI_VIEW_OF(&grid_view));

    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(10, 5));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(65, 5));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(10, 45));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(65, 45));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(10, 85));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(40, 10));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(50, 10));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(90, 10));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_total_area() <= 4000);
}

static void test_dirty_passthrough_page_base_content_translation_emits_per_child_swept(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_page_base_t page;
    egui_view_t child_a;
    egui_view_t child_b;

    egui_page_base_init(&page, core);
    test_dirty_passthrough_add_two_children(EGUI_VIEW_OF(&page.root_view), &child_a, &child_b, core);
    EGUI_VIEW_OF(&page.root_view)->api->calculate_layout(EGUI_VIEW_OF(&page.root_view));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&page.root_view), 0, 5);
    EGUI_VIEW_OF(&page.root_view)->api->calculate_layout(EGUI_VIEW_OF(&page.root_view));

    test_dirty_passthrough_assert_two_vertical_sweeps();
}

static void test_dirty_passthrough_page_base_size_change_cascades(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_page_base_t page;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t expected_a;
    egui_region_t expected_b;

    egui_page_base_init(&page, core);
    test_dirty_passthrough_add_two_children(EGUI_VIEW_OF(&page.root_view), &child_a, &child_b, core);
    EGUI_VIEW_OF(&page.root_view)->api->calculate_layout(EGUI_VIEW_OF(&page.root_view));
    egui_core_clear_region_dirty(core);

    egui_view_set_size(EGUI_VIEW_OF(&page.root_view), core->screen_width - 10, core->screen_height);
    EGUI_VIEW_OF(&page.root_view)->api->calculate_layout(EGUI_VIEW_OF(&page.root_view));

    egui_region_init(&expected_a, 0, 20, 100, 20);
    egui_region_init(&expected_b, 0, 70, 100, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_b));
}

static void test_dirty_passthrough_user_root_remove_emits_child_regions(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);
    egui_view_group_t page;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t expected_a;
    egui_region_t expected_b;

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
    egui_view_group_init(EGUI_VIEW_OF(&page), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&page), 0, 0, 100, 120);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&page), 1);
    test_dirty_passthrough_add_two_children(EGUI_VIEW_OF(&page), &child_a, &child_b, core);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&page));
    EGUI_VIEW_OF(user_root)->api->calculate_layout(EGUI_VIEW_OF(user_root));
    egui_core_clear_region_dirty(core);

    egui_view_remove_from_user_root(EGUI_VIEW_OF(&page));

    egui_region_init(&expected_a, 0, 20, 100, 20);
    egui_region_init(&expected_b, 0, 70, 100, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_b));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(10, 50));
}

static void test_dirty_passthrough_user_root_remove_nested_header_emits_leaf_regions(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);
    egui_view_group_t page;
    egui_view_group_t header;
    egui_view_t left;
    egui_view_t right;
    egui_region_t expected_left;
    egui_region_t expected_right;

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
    egui_view_group_init(EGUI_VIEW_OF(&page), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&page), 0, 0, 240, 320);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&page), 1);

    egui_view_group_init(EGUI_VIEW_OF(&header), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&header), 0, 0, 240, 46);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&header), 1);

    test_dirty_passthrough_init_child(&left, core, 4, 10, 30, 20);
    test_dirty_passthrough_init_child(&right, core, 180, 10, 30, 20);
    egui_view_group_add_child(EGUI_VIEW_OF(&header), &left);
    egui_view_group_add_child(EGUI_VIEW_OF(&header), &right);
    egui_view_group_add_child(EGUI_VIEW_OF(&page), EGUI_VIEW_OF(&header));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&page));
    EGUI_VIEW_OF(user_root)->api->calculate_layout(EGUI_VIEW_OF(user_root));
    egui_core_clear_region_dirty(core);

    egui_view_remove_from_user_root(EGUI_VIEW_OF(&page));

    egui_region_init(&expected_left, 4, 10, 30, 20);
    egui_region_init(&expected_right, 180, 10, 30, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_left));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_right));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(50, 20));
}

static void test_dirty_passthrough_user_root_remove_mixed_viewpage_emits_nested_leaf_regions(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);
    egui_view_group_t page;
    egui_view_viewpage_t viewpage;
    egui_view_scroll_t scroll;
    egui_view_linearlayout_t layout;
    egui_view_group_t empty_page;
    egui_view_t child_a;
    egui_view_t child_b;
    egui_region_t expected_a;
    egui_region_t expected_b;

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
    egui_core_clear_region_dirty(core);

    egui_view_group_init(EGUI_VIEW_OF(&page), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&page), 0, 0, 120, 100);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&page), 1);

    test_dirty_passthrough_setup_viewpage_scroll_linear(&viewpage, &scroll, &layout, &empty_page, &child_a, &child_b, core, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&page), EGUI_VIEW_OF(&viewpage));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&page));
    EGUI_VIEW_OF(user_root)->api->calculate_layout(EGUI_VIEW_OF(user_root));
    egui_core_clear_region_dirty(core);

    egui_view_remove_from_user_root(EGUI_VIEW_OF(&page));

    egui_region_init(&expected_a, 0, 20, 100, 20);
    egui_region_init(&expected_b, 0, 70, 100, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_a));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected_b));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(10, 50));
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(110, 20));
}

static void test_dirty_passthrough_activity_root_child_attach_does_not_mark_root_self(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_root_group_t root;
    egui_view_t child;
    egui_region_t expected;

    egui_view_root_group_init(EGUI_VIEW_OF(&root), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&root), 0, 0, 100, 120);
    EGUI_VIEW_OF(&root)->api->calculate_layout(EGUI_VIEW_OF(&root));
    egui_core_clear_region_dirty(core);

    test_dirty_passthrough_init_child(&child, core, 0, 20, 100, 20);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &child);
    EGUI_VIEW_OF(&root)->api->request_layout(EGUI_VIEW_OF(&root));
    EGUI_VIEW_OF(&root)->api->calculate_layout(EGUI_VIEW_OF(&root));

    egui_region_init(&expected, 0, 20, 100, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_region(&expected));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_total_area() < test_dirty_passthrough_area(&EGUI_VIEW_OF(&root)->region_screen));
}

static void test_dirty_passthrough_activity_root_translation_emits_per_child_swept(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_root_group_t root;
    egui_view_t child_a;
    egui_view_t child_b;

    egui_view_root_group_init(EGUI_VIEW_OF(&root), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&root), 0, 0, 100, 120);
    test_dirty_passthrough_add_two_children(EGUI_VIEW_OF(&root), &child_a, &child_b, core);
    EGUI_VIEW_OF(&root)->api->calculate_layout(EGUI_VIEW_OF(&root));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&root), 0, 5);
    EGUI_VIEW_OF(&root)->api->calculate_layout(EGUI_VIEW_OF(&root));

    test_dirty_passthrough_assert_two_vertical_sweeps();
}

static void test_dirty_passthrough_dirty_slot_merge_still_covers_child_sweeps(void)
{
    egui_core_t *core = test_dirty_passthrough_get_core();
    egui_view_group_t group;
    egui_view_t child[4];

    egui_view_group_init(EGUI_VIEW_OF(&group), core);
    test_dirty_passthrough_set_region(EGUI_VIEW_OF(&group), 0, 0, 100, 160);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&group), 1);
    for (int i = 0; i < 4; i++)
    {
        test_dirty_passthrough_init_child(&child[i], core, 0, 10 + i * 35, 100, 10);
        egui_view_group_add_child(EGUI_VIEW_OF(&group), &child[i]);
    }
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));
    egui_core_clear_region_dirty(core);

    egui_view_set_position(EGUI_VIEW_OF(&group), 0, 5);
    EGUI_VIEW_OF(&group)->api->calculate_layout(EGUI_VIEW_OF(&group));

    EGUI_TEST_ASSERT_EQUAL_INT(2, test_dirty_passthrough_count());
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(5, 17));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(5, 52));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(5, 87));
    EGUI_TEST_ASSERT_TRUE(test_dirty_passthrough_has_point(5, 122));
}

static void test_dirty_passthrough_advantage_sparse_vertical_list_reduces_dirty_area(void)
{
    int32_t baseline_area = test_dirty_passthrough_measure_sparse_vertical_group(0);
    int32_t dirty_passthrough_area = test_dirty_passthrough_measure_sparse_vertical_group(1);

    EGUI_TEST_ASSERT_TRUE(dirty_passthrough_area < baseline_area);
    EGUI_TEST_ASSERT_TRUE(dirty_passthrough_area <= baseline_area / 2);
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(10, 80));
}

static void test_dirty_passthrough_advantage_horizontal_pager_reduces_dirty_area(void)
{
    int32_t baseline_area = test_dirty_passthrough_measure_sparse_horizontal_group(0);
    int32_t dirty_passthrough_area = test_dirty_passthrough_measure_sparse_horizontal_group(1);

    EGUI_TEST_ASSERT_TRUE(dirty_passthrough_area < baseline_area);
    EGUI_TEST_ASSERT_TRUE(dirty_passthrough_area <= baseline_area / 2);
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(80, 10));
}

static void test_dirty_passthrough_advantage_nested_structural_page_reduces_dirty_area(void)
{
    int32_t baseline_area = test_dirty_passthrough_measure_nested_sparse_page(0);
    int32_t dirty_passthrough_area = test_dirty_passthrough_measure_nested_sparse_page(1);

    EGUI_TEST_ASSERT_TRUE(dirty_passthrough_area < baseline_area);
    EGUI_TEST_ASSERT_TRUE(dirty_passthrough_area <= baseline_area / 4);
    EGUI_TEST_ASSERT_FALSE(test_dirty_passthrough_has_point(80, 50));
}

void test_dirty_passthrough_container_run(void)
{
    EGUI_TEST_SUITE_BEGIN(dirty_passthrough_container);
    EGUI_TEST_RUN(test_dirty_passthrough_group_translation_emits_per_child_swept);
    EGUI_TEST_RUN(test_dirty_passthrough_group_size_change_cascades_to_children);
    EGUI_TEST_RUN(test_dirty_passthrough_group_background_change_still_self_full_rect);
    EGUI_TEST_RUN(test_dirty_passthrough_group_background_translation_marks_self_swept);
    EGUI_TEST_RUN(test_dirty_passthrough_group_background_size_change_marks_self_swept);
    EGUI_TEST_RUN(test_dirty_passthrough_nested_background_group_translation_marks_child_self_swept);
    EGUI_TEST_RUN(test_dirty_passthrough_group_empty_container_no_op);
    EGUI_TEST_RUN(test_dirty_passthrough_group_visibility_change_emits_child_regions);
    EGUI_TEST_RUN(test_dirty_passthrough_linearlayout_translation_emits_leaf_sweeps);
    EGUI_TEST_RUN(test_dirty_passthrough_scroll_inner_drag_emits_per_child_swept);
    EGUI_TEST_RUN(test_dirty_passthrough_scroll_viewport_clips_inner_drag_sweeps);
    EGUI_TEST_RUN(test_dirty_passthrough_scroll_first_attach_cascades);
    EGUI_TEST_RUN(test_dirty_passthrough_scroll_background_change_self_full_rect);
    EGUI_TEST_RUN(test_dirty_passthrough_scroll_with_background_stable_drag_emits_child_sweeps);
    EGUI_TEST_RUN(test_dirty_passthrough_scroll_empty_no_op);
    EGUI_TEST_RUN(test_dirty_passthrough_scrollbar_thumb_dirty_tracks_scroll);
    EGUI_TEST_RUN(test_dirty_passthrough_viewpage_page_animation_emits_per_child_swept);
    EGUI_TEST_RUN(test_dirty_passthrough_viewpage_empty_no_op);
    EGUI_TEST_RUN(test_dirty_passthrough_viewpage_linear_page_drag_emits_leaf_sweeps);
    EGUI_TEST_RUN(test_dirty_passthrough_mixed_viewpage_scroll_linearlayout_drag_emits_leaf_regions);
    EGUI_TEST_RUN(test_dirty_passthrough_virtual_viewport_background_scroll_emits_leaf_regions);
    EGUI_TEST_RUN(test_dirty_passthrough_list_view_scroll_emits_holder_leaf_regions);
    EGUI_TEST_RUN(test_dirty_passthrough_grid_view_scroll_emits_cell_leaf_regions);
    EGUI_TEST_RUN(test_dirty_passthrough_page_base_content_translation_emits_per_child_swept);
    EGUI_TEST_RUN(test_dirty_passthrough_page_base_size_change_cascades);
    EGUI_TEST_RUN(test_dirty_passthrough_user_root_remove_emits_child_regions);
    EGUI_TEST_RUN(test_dirty_passthrough_user_root_remove_nested_header_emits_leaf_regions);
    EGUI_TEST_RUN(test_dirty_passthrough_user_root_remove_mixed_viewpage_emits_nested_leaf_regions);
    EGUI_TEST_RUN(test_dirty_passthrough_activity_root_child_attach_does_not_mark_root_self);
    EGUI_TEST_RUN(test_dirty_passthrough_activity_root_translation_emits_per_child_swept);
    EGUI_TEST_RUN(test_dirty_passthrough_dirty_slot_merge_still_covers_child_sweeps);
    EGUI_TEST_RUN(test_dirty_passthrough_advantage_sparse_vertical_list_reduces_dirty_area);
    EGUI_TEST_RUN(test_dirty_passthrough_advantage_horizontal_pager_reduces_dirty_area);
    EGUI_TEST_RUN(test_dirty_passthrough_advantage_nested_structural_page_reduces_dirty_area);
    EGUI_TEST_SUITE_END();
}
