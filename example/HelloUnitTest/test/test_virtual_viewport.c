#include "test_virtual_viewport.h"

#include <stdio.h>
#include <string.h>

#include "egui.h"

#define TEST_VIRTUAL_VIEWPORT_MAX_ITEMS        1280
#define TEST_VIRTUAL_SECTION_LIST_MAX_SECTIONS 8
#define TEST_VIRTUAL_SECTION_LIST_MAX_ITEMS    8
#define TEST_VIRTUAL_GRID_MAX_POOL             (EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS * EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS)
#define TEST_VIRTUAL_TREE_MAX_ROOTS            4
#define TEST_VIRTUAL_TREE_MAX_CHILDREN         4
#define TEST_VIRTUAL_TREE_MAX_NODES            12

typedef struct test_virtual_viewport_context test_virtual_viewport_context_t;
typedef struct test_virtual_section_list_context test_virtual_section_list_context_t;
typedef struct test_virtual_grid_context test_virtual_grid_context_t;
typedef struct test_virtual_tree_node test_virtual_tree_node_t;
typedef struct test_virtual_tree_context test_virtual_tree_context_t;
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

struct test_virtual_section_list_context
{
    uint32_t section_count;
    uint16_t bind_count;
    uint8_t created_count;
    uint32_t section_ids[TEST_VIRTUAL_SECTION_LIST_MAX_SECTIONS];
    uint32_t item_counts[TEST_VIRTUAL_SECTION_LIST_MAX_SECTIONS];
    uint32_t item_ids[TEST_VIRTUAL_SECTION_LIST_MAX_SECTIONS][TEST_VIRTUAL_SECTION_LIST_MAX_ITEMS];
    int32_t header_heights[TEST_VIRTUAL_SECTION_LIST_MAX_SECTIONS];
    int32_t item_heights[TEST_VIRTUAL_SECTION_LIST_MAX_SECTIONS][TEST_VIRTUAL_SECTION_LIST_MAX_ITEMS];
    egui_view_label_t labels[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
    char texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][32];
};

struct test_virtual_tree_node
{
    uint32_t stable_id;
    uint32_t parent_stable_id;
    uint32_t child_ids[TEST_VIRTUAL_TREE_MAX_CHILDREN];
    uint32_t child_count;
    int32_t height;
    uint8_t expanded;
};

struct test_virtual_tree_context
{
    uint32_t root_count;
    uint16_t bind_count;
    uint8_t created_count;
    uint32_t root_ids[TEST_VIRTUAL_TREE_MAX_ROOTS];
    test_virtual_tree_node_t nodes[TEST_VIRTUAL_TREE_MAX_NODES];
    egui_view_label_t labels[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
    char texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][32];
};

struct test_virtual_grid_context
{
    uint32_t item_count;
    uint16_t bind_count;
    uint8_t created_count;
    uint32_t stable_ids[TEST_VIRTUAL_VIEWPORT_MAX_ITEMS];
    int32_t item_heights[TEST_VIRTUAL_VIEWPORT_MAX_ITEMS];
    egui_view_label_t labels[TEST_VIRTUAL_GRID_MAX_POOL];
    char texts[TEST_VIRTUAL_GRID_MAX_POOL][32];
};

static egui_view_virtual_viewport_t test_viewport;
static egui_view_virtual_list_t test_list;
static egui_view_virtual_strip_t test_strip;
static egui_view_virtual_page_t test_page;
static egui_view_virtual_grid_t test_grid;
static egui_view_virtual_section_list_t test_section_list;
static egui_view_virtual_tree_t test_tree;
static test_virtual_viewport_context_t test_context;
static test_virtual_section_list_context_t test_section_list_context;
static test_virtual_grid_context_t test_grid_context;
static test_virtual_tree_context_t test_tree_context;

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

static const egui_view_virtual_strip_data_source_t test_strip_data_source = {
        .get_count = adapter_get_count,
        .get_stable_id = adapter_get_stable_id,
        .find_index_by_stable_id = adapter_find_index_by_stable_id,
        .get_item_view_type = NULL,
        .measure_item_width = adapter_measure_main_size,
        .create_item_view = adapter_create_view,
        .destroy_item_view = NULL,
        .bind_item_view = adapter_bind_view,
        .unbind_item_view = adapter_unbind_view,
        .should_keep_alive = NULL,
        .save_item_state = NULL,
        .restore_item_state = NULL,
        .default_item_view_type = 0,
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

static int find_section_list_pool_index(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        if (view == EGUI_VIEW_OF(&test_section_list_context.labels[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static uint32_t test_section_list_get_section_count(void *data_source_context)
{
    return ((test_virtual_section_list_context_t *)data_source_context)->section_count;
}

static uint32_t test_section_list_get_section_stable_id(void *data_source_context, uint32_t section_index)
{
    test_virtual_section_list_context_t *ctx = (test_virtual_section_list_context_t *)data_source_context;
    return ctx->section_ids[section_index];
}

static uint32_t test_section_list_get_item_count(void *data_source_context, uint32_t section_index)
{
    test_virtual_section_list_context_t *ctx = (test_virtual_section_list_context_t *)data_source_context;
    return ctx->item_counts[section_index];
}

static uint32_t test_section_list_get_item_stable_id(void *data_source_context, uint32_t section_index, uint32_t item_index)
{
    test_virtual_section_list_context_t *ctx = (test_virtual_section_list_context_t *)data_source_context;
    return ctx->item_ids[section_index][item_index];
}

static int32_t test_section_list_measure_header_height(void *data_source_context, uint32_t section_index, int32_t width_hint)
{
    test_virtual_section_list_context_t *ctx = (test_virtual_section_list_context_t *)data_source_context;
    EGUI_UNUSED(width_hint);
    return ctx->header_heights[section_index];
}

static int32_t test_section_list_measure_item_height(void *data_source_context, uint32_t section_index, uint32_t item_index, int32_t width_hint)
{
    test_virtual_section_list_context_t *ctx = (test_virtual_section_list_context_t *)data_source_context;
    EGUI_UNUSED(width_hint);
    return ctx->item_heights[section_index][item_index];
}

static egui_view_t *test_section_list_create_view(void *data_source_context, uint16_t view_type)
{
    test_virtual_section_list_context_t *ctx = (test_virtual_section_list_context_t *)data_source_context;
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

static void test_section_list_bind_header_view(void *data_source_context, egui_view_t *view, uint32_t section_index, uint32_t stable_id)
{
    test_virtual_section_list_context_t *ctx = (test_virtual_section_list_context_t *)data_source_context;
    int pool_index = find_section_list_pool_index(view);

    ctx->bind_count++;
    if (pool_index >= 0)
    {
        snprintf(ctx->texts[pool_index], sizeof(ctx->texts[pool_index]), "H%lu:%lu", (unsigned long)section_index, (unsigned long)stable_id);
        egui_view_label_set_text(view, ctx->texts[pool_index]);
    }
}

static void test_section_list_bind_item_view(void *data_source_context, egui_view_t *view, uint32_t section_index, uint32_t item_index, uint32_t stable_id)
{
    test_virtual_section_list_context_t *ctx = (test_virtual_section_list_context_t *)data_source_context;
    int pool_index = find_section_list_pool_index(view);

    ctx->bind_count++;
    if (pool_index >= 0)
    {
        snprintf(ctx->texts[pool_index], sizeof(ctx->texts[pool_index]), "I%lu:%lu:%lu", (unsigned long)section_index, (unsigned long)item_index,
                 (unsigned long)stable_id);
        egui_view_label_set_text(view, ctx->texts[pool_index]);
    }
}

static const egui_view_virtual_section_list_data_source_t test_section_list_data_source = {
        .get_section_count = test_section_list_get_section_count,
        .get_section_stable_id = test_section_list_get_section_stable_id,
        .find_section_index_by_stable_id = NULL,
        .get_item_count = test_section_list_get_item_count,
        .get_item_stable_id = test_section_list_get_item_stable_id,
        .find_item_position_by_stable_id = NULL,
        .get_section_header_view_type = NULL,
        .get_item_view_type = NULL,
        .measure_section_header_height = test_section_list_measure_header_height,
        .measure_item_height = test_section_list_measure_item_height,
        .create_section_header_view = test_section_list_create_view,
        .create_item_view = test_section_list_create_view,
        .destroy_section_header_view = NULL,
        .destroy_item_view = NULL,
        .bind_section_header_view = test_section_list_bind_header_view,
        .bind_item_view = test_section_list_bind_item_view,
        .unbind_section_header_view = NULL,
        .unbind_item_view = NULL,
        .should_keep_section_header_alive = NULL,
        .should_keep_item_alive = NULL,
        .save_section_header_state = NULL,
        .save_item_state = NULL,
        .restore_section_header_state = NULL,
        .restore_item_state = NULL,
        .default_section_header_view_type = 0,
        .default_item_view_type = 0,
};

static int find_grid_pool_index(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < TEST_VIRTUAL_GRID_MAX_POOL; i++)
    {
        if (view == EGUI_VIEW_OF(&test_grid_context.labels[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static uint32_t test_grid_get_count(void *data_source_context)
{
    return ((test_virtual_grid_context_t *)data_source_context)->item_count;
}

static uint32_t test_grid_get_stable_id(void *data_source_context, uint32_t index)
{
    test_virtual_grid_context_t *ctx = (test_virtual_grid_context_t *)data_source_context;
    return ctx->stable_ids[index];
}

static int32_t test_grid_find_index_by_stable_id(void *data_source_context, uint32_t stable_id)
{
    test_virtual_grid_context_t *ctx = (test_virtual_grid_context_t *)data_source_context;
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

static int32_t test_grid_measure_item_height(void *data_source_context, uint32_t index, int32_t width_hint)
{
    test_virtual_grid_context_t *ctx = (test_virtual_grid_context_t *)data_source_context;

    EGUI_UNUSED(width_hint);
    return ctx->item_heights[index];
}

static egui_view_t *test_grid_create_item_view(void *data_source_context, uint16_t view_type)
{
    test_virtual_grid_context_t *ctx = (test_virtual_grid_context_t *)data_source_context;
    egui_view_label_t *label;

    EGUI_UNUSED(view_type);

    if (ctx->created_count >= TEST_VIRTUAL_GRID_MAX_POOL)
    {
        return NULL;
    }

    label = &ctx->labels[ctx->created_count++];
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), EGUI_ALIGN_CENTER);
    return EGUI_VIEW_OF(label);
}

static void test_grid_bind_item_view(void *data_source_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    test_virtual_grid_context_t *ctx = (test_virtual_grid_context_t *)data_source_context;
    int pool_index = find_grid_pool_index(view);

    ctx->bind_count++;
    if (pool_index >= 0)
    {
        snprintf(ctx->texts[pool_index], sizeof(ctx->texts[pool_index]), "G%lu:%lu", (unsigned long)index, (unsigned long)stable_id);
        egui_view_label_set_text(view, ctx->texts[pool_index]);
    }
}

static const egui_view_virtual_grid_data_source_t test_grid_data_source = {
        .get_count = test_grid_get_count,
        .get_stable_id = test_grid_get_stable_id,
        .find_index_by_stable_id = test_grid_find_index_by_stable_id,
        .get_item_view_type = NULL,
        .measure_item_height = test_grid_measure_item_height,
        .create_item_view = test_grid_create_item_view,
        .destroy_item_view = NULL,
        .bind_item_view = test_grid_bind_item_view,
        .unbind_item_view = NULL,
        .should_keep_alive = NULL,
        .save_item_state = NULL,
        .restore_item_state = NULL,
        .default_item_view_type = 0,
};

static int find_tree_pool_index(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        if (view == EGUI_VIEW_OF(&test_tree_context.labels[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static test_virtual_tree_node_t *find_tree_node(uint32_t stable_id)
{
    uint32_t i;

    for (i = 0; i < TEST_VIRTUAL_TREE_MAX_NODES; i++)
    {
        if (test_tree_context.nodes[i].stable_id == stable_id)
        {
            return &test_tree_context.nodes[i];
        }
    }

    return NULL;
}

static uint32_t test_tree_get_root_count(void *data_source_context)
{
    return ((test_virtual_tree_context_t *)data_source_context)->root_count;
}

static uint32_t test_tree_get_root_stable_id(void *data_source_context, uint32_t root_index)
{
    test_virtual_tree_context_t *ctx = (test_virtual_tree_context_t *)data_source_context;
    return root_index < ctx->root_count ? ctx->root_ids[root_index] : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static uint32_t test_tree_get_child_count(void *data_source_context, uint32_t stable_id)
{
    test_virtual_tree_node_t *node;

    EGUI_UNUSED(data_source_context);

    node = find_tree_node(stable_id);
    return node != NULL ? node->child_count : 0;
}

static uint32_t test_tree_get_child_stable_id(void *data_source_context, uint32_t stable_id, uint32_t child_index)
{
    test_virtual_tree_node_t *node;

    EGUI_UNUSED(data_source_context);

    node = find_tree_node(stable_id);
    if (node == NULL || child_index >= node->child_count)
    {
        return EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    return node->child_ids[child_index];
}

static uint8_t test_tree_is_node_expanded(void *data_source_context, uint32_t stable_id)
{
    test_virtual_tree_node_t *node;

    EGUI_UNUSED(data_source_context);

    node = find_tree_node(stable_id);
    return node != NULL ? node->expanded : 0;
}

static int32_t test_tree_measure_node_height(void *data_source_context, const egui_view_virtual_tree_entry_t *entry, int32_t width_hint)
{
    test_virtual_tree_node_t *node;

    EGUI_UNUSED(data_source_context);
    EGUI_UNUSED(width_hint);

    node = find_tree_node(entry->stable_id);
    return node != NULL ? node->height : 18;
}

static egui_view_t *test_tree_create_node_view(void *data_source_context, uint16_t view_type)
{
    test_virtual_tree_context_t *ctx = (test_virtual_tree_context_t *)data_source_context;
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

static void test_tree_bind_node_view(void *data_source_context, egui_view_t *view, const egui_view_virtual_tree_entry_t *entry)
{
    test_virtual_tree_context_t *ctx = (test_virtual_tree_context_t *)data_source_context;
    int pool_index = find_tree_pool_index(view);

    ctx->bind_count++;
    if (pool_index >= 0)
    {
        snprintf(ctx->texts[pool_index], sizeof(ctx->texts[pool_index]), "T%lu:%u", (unsigned long)entry->stable_id, (unsigned)entry->depth);
        egui_view_label_set_text(view, ctx->texts[pool_index]);
    }
}

static const egui_view_virtual_tree_data_source_t test_tree_data_source = {
        .get_root_count = test_tree_get_root_count,
        .get_root_stable_id = test_tree_get_root_stable_id,
        .get_child_count = test_tree_get_child_count,
        .get_child_stable_id = test_tree_get_child_stable_id,
        .is_node_expanded = test_tree_is_node_expanded,
        .get_node_view_type = NULL,
        .measure_node_height = test_tree_measure_node_height,
        .create_node_view = test_tree_create_node_view,
        .destroy_node_view = NULL,
        .bind_node_view = test_tree_bind_node_view,
        .unbind_node_view = NULL,
        .should_keep_alive = NULL,
        .save_node_state = NULL,
        .restore_node_state = NULL,
        .default_view_type = 0,
};

static void reset_test_section_list_items(void)
{
    memset(&test_section_list_context, 0, sizeof(test_section_list_context));

    test_section_list_context.section_count = 3;
    test_section_list_context.section_ids[0] = 2100;
    test_section_list_context.section_ids[1] = 2200;
    test_section_list_context.section_ids[2] = 2300;

    test_section_list_context.item_counts[0] = 2;
    test_section_list_context.item_counts[1] = 3;
    test_section_list_context.item_counts[2] = 1;

    test_section_list_context.header_heights[0] = 18;
    test_section_list_context.header_heights[1] = 22;
    test_section_list_context.header_heights[2] = 20;

    test_section_list_context.item_ids[0][0] = 3100;
    test_section_list_context.item_ids[0][1] = 3101;
    test_section_list_context.item_ids[1][0] = 3200;
    test_section_list_context.item_ids[1][1] = 3201;
    test_section_list_context.item_ids[1][2] = 3202;
    test_section_list_context.item_ids[2][0] = 3300;

    test_section_list_context.item_heights[0][0] = 12;
    test_section_list_context.item_heights[0][1] = 14;
    test_section_list_context.item_heights[1][0] = 16;
    test_section_list_context.item_heights[1][1] = 18;
    test_section_list_context.item_heights[1][2] = 20;
    test_section_list_context.item_heights[2][0] = 24;
}

static void reset_test_tree_items(void)
{
    memset(&test_tree_context, 0, sizeof(test_tree_context));

    test_tree_context.root_count = 2;
    test_tree_context.root_ids[0] = 5100;
    test_tree_context.root_ids[1] = 5400;

    test_tree_context.nodes[0].stable_id = 5100;
    test_tree_context.nodes[0].parent_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    test_tree_context.nodes[0].child_ids[0] = 5200;
    test_tree_context.nodes[0].child_ids[1] = 5300;
    test_tree_context.nodes[0].child_count = 2;
    test_tree_context.nodes[0].height = 18;
    test_tree_context.nodes[0].expanded = 1;

    test_tree_context.nodes[1].stable_id = 5200;
    test_tree_context.nodes[1].parent_stable_id = 5100;
    test_tree_context.nodes[1].child_ids[0] = 5210;
    test_tree_context.nodes[1].child_ids[1] = 5220;
    test_tree_context.nodes[1].child_count = 2;
    test_tree_context.nodes[1].height = 22;
    test_tree_context.nodes[1].expanded = 1;

    test_tree_context.nodes[2].stable_id = 5210;
    test_tree_context.nodes[2].parent_stable_id = 5200;
    test_tree_context.nodes[2].child_count = 0;
    test_tree_context.nodes[2].height = 16;
    test_tree_context.nodes[2].expanded = 0;

    test_tree_context.nodes[3].stable_id = 5220;
    test_tree_context.nodes[3].parent_stable_id = 5200;
    test_tree_context.nodes[3].child_count = 0;
    test_tree_context.nodes[3].height = 14;
    test_tree_context.nodes[3].expanded = 0;

    test_tree_context.nodes[4].stable_id = 5300;
    test_tree_context.nodes[4].parent_stable_id = 5100;
    test_tree_context.nodes[4].child_ids[0] = 5310;
    test_tree_context.nodes[4].child_count = 1;
    test_tree_context.nodes[4].height = 20;
    test_tree_context.nodes[4].expanded = 0;

    test_tree_context.nodes[5].stable_id = 5310;
    test_tree_context.nodes[5].parent_stable_id = 5300;
    test_tree_context.nodes[5].child_count = 0;
    test_tree_context.nodes[5].height = 12;
    test_tree_context.nodes[5].expanded = 0;

    test_tree_context.nodes[6].stable_id = 5400;
    test_tree_context.nodes[6].parent_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    test_tree_context.nodes[6].child_ids[0] = 5410;
    test_tree_context.nodes[6].child_ids[1] = 5420;
    test_tree_context.nodes[6].child_count = 2;
    test_tree_context.nodes[6].height = 18;
    test_tree_context.nodes[6].expanded = 0;

    test_tree_context.nodes[7].stable_id = 5410;
    test_tree_context.nodes[7].parent_stable_id = 5400;
    test_tree_context.nodes[7].child_count = 0;
    test_tree_context.nodes[7].height = 16;
    test_tree_context.nodes[7].expanded = 0;

    test_tree_context.nodes[8].stable_id = 5420;
    test_tree_context.nodes[8].parent_stable_id = 5400;
    test_tree_context.nodes[8].child_count = 0;
    test_tree_context.nodes[8].height = 14;
    test_tree_context.nodes[8].expanded = 0;
}

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

static void layout_strip(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_view_layout(EGUI_VIEW_OF(&test_strip), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_strip)->region_screen, &region);
    EGUI_VIEW_OF(&test_strip)->api->calculate_layout(EGUI_VIEW_OF(&test_strip));
}

static void setup_viewport(uint32_t item_count, uint32_t keep_alive_id)
{
    const egui_view_virtual_viewport_params_t params = {
            .region = {{0, 0}, {100, 60}},
            .orientation = EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL,
            .overscan_before = 1,
            .overscan_after = 1,
            .max_keepalive_slots = 2,
            .estimated_item_extent = 20,
    };
    const egui_view_virtual_viewport_setup_t setup = {
            .params = &params,
            .adapter = &test_adapter,
            .adapter_context = &test_context,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };

    memset(&test_viewport, 0, sizeof(test_viewport));
    memset(&test_context, 0, sizeof(test_context));

    reset_test_items(item_count, 20);
    if (keep_alive_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        test_context.keep_alive_ids[0] = keep_alive_id;
        test_context.keep_alive_count = 1;
    }

    egui_view_virtual_viewport_init_with_setup(EGUI_VIEW_OF(&test_viewport), &setup);
    layout_viewport(0, 0, 100, 60);
}

static void setup_list_with_data_source_config(uint32_t item_count, const egui_view_virtual_list_data_source_t *data_source)
{
    const egui_view_virtual_list_params_t params = {
            .region = {{0, 0}, {100, 60}},
            .overscan_before = 1,
            .overscan_after = 1,
            .max_keepalive_slots = 2,
            .estimated_item_height = 20,
    };
    const egui_view_virtual_list_setup_t setup = {
            .params = &params,
            .data_source = data_source,
            .data_source_context = &test_context,
            .state_cache_max_entries = 3,
            .state_cache_max_bytes = 12,
    };

    memset(&test_list, 0, sizeof(test_list));
    memset(&test_context, 0, sizeof(test_context));

    reset_test_items(item_count, 20);

    egui_view_virtual_list_init_with_setup(EGUI_VIEW_OF(&test_list), &setup);
    layout_list(0, 0, 100, 60);
}

static void setup_list_with_data_source(uint32_t item_count)
{
    setup_list_with_data_source_config(item_count, &test_list_data_source);
}

static int find_list_slot_index_by_stable_id(uint32_t stable_id)
{
    uint8_t i;
    uint8_t slot_count = egui_view_virtual_list_get_slot_count(EGUI_VIEW_OF(&test_list));

    for (i = 0; i < slot_count; i++)
    {
        const egui_view_virtual_list_slot_t *slot = egui_view_virtual_list_get_slot(EGUI_VIEW_OF(&test_list), i);
        if (slot != NULL && slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->stable_id == stable_id)
        {
            return (int)i;
        }
    }

    return -1;
}

static void setup_strip_with_data_source(void)
{
    const egui_view_virtual_strip_params_t params = {
            .region = {{0, 0}, {60, 36}},
            .overscan_before = 1,
            .overscan_after = 1,
            .max_keepalive_slots = 2,
            .estimated_item_width = 18,
    };
    const egui_view_virtual_strip_setup_t setup = {
            .params = &params,
            .data_source = &test_strip_data_source,
            .data_source_context = &test_context,
            .state_cache_max_entries = 4,
            .state_cache_max_bytes = 16,
    };

    memset(&test_strip, 0, sizeof(test_strip));
    memset(&test_context, 0, sizeof(test_context));

    reset_test_items(6, 18);
    test_context.stable_ids[0] = 8100;
    test_context.stable_ids[1] = 8101;
    test_context.stable_ids[2] = 8102;
    test_context.stable_ids[3] = 8103;
    test_context.stable_ids[4] = 8104;
    test_context.stable_ids[5] = 8105;
    test_context.item_heights[0] = 18;
    test_context.item_heights[1] = 22;
    test_context.item_heights[2] = 14;
    test_context.item_heights[3] = 26;
    test_context.item_heights[4] = 20;
    test_context.item_heights[5] = 16;

    egui_view_virtual_strip_init_with_setup(EGUI_VIEW_OF(&test_strip), &setup);
    layout_strip(0, 0, 60, 36);
}

static int find_strip_slot_index_by_stable_id(uint32_t stable_id)
{
    uint8_t i;
    uint8_t slot_count = egui_view_virtual_strip_get_slot_count(EGUI_VIEW_OF(&test_strip));

    for (i = 0; i < slot_count; i++)
    {
        const egui_view_virtual_strip_slot_t *slot = egui_view_virtual_strip_get_slot(EGUI_VIEW_OF(&test_strip), i);
        if (slot != NULL && slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->stable_id == stable_id)
        {
            return (int)i;
        }
    }

    return -1;
}

static void setup_page_with_data_source_config(uint32_t item_count, const egui_view_virtual_page_data_source_t *data_source)
{
    EGUI_REGION_DEFINE(region, 0, 0, 100, 60);
    const egui_view_virtual_page_params_t params = {
            .region = {{0, 0}, {100, 60}},
            .overscan_before = 1,
            .overscan_after = 1,
            .max_keepalive_slots = 2,
            .estimated_section_height = 20,
    };
    const egui_view_virtual_page_setup_t setup = {
            .params = &params,
            .data_source = data_source,
            .data_source_context = &test_context,
            .state_cache_max_entries = 5,
            .state_cache_max_bytes = 20,
    };

    memset(&test_page, 0, sizeof(test_page));
    memset(&test_context, 0, sizeof(test_context));

    reset_test_items(item_count, 20);

    egui_view_virtual_page_init_with_setup(EGUI_VIEW_OF(&test_page), &setup);
    egui_view_layout(EGUI_VIEW_OF(&test_page), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_page)->region_screen, &region);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));
}

static void setup_page_with_data_source(uint32_t item_count)
{
    setup_page_with_data_source_config(item_count, &test_page_data_source);
}

static int find_page_slot_index_by_stable_id(uint32_t stable_id)
{
    uint8_t i;
    uint8_t slot_count = egui_view_virtual_page_get_slot_count(EGUI_VIEW_OF(&test_page));

    for (i = 0; i < slot_count; i++)
    {
        const egui_view_virtual_page_slot_t *slot = egui_view_virtual_page_get_slot(EGUI_VIEW_OF(&test_page), i);
        if (slot != NULL && slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->stable_id == stable_id)
        {
            return (int)i;
        }
    }

    return -1;
}

static void layout_section_list(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_view_layout(EGUI_VIEW_OF(&test_section_list), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_section_list)->region_screen, &region);
    EGUI_VIEW_OF(&test_section_list)->api->calculate_layout(EGUI_VIEW_OF(&test_section_list));
}

static void setup_section_list_with_data_source(void)
{
    const egui_view_virtual_section_list_params_t params = {
            .region = {{0, 0}, {100, 60}},
            .overscan_before = 1,
            .overscan_after = 1,
            .max_keepalive_slots = 2,
            .estimated_entry_height = 18,
    };
    const egui_view_virtual_section_list_setup_t setup = {
            .params = &params,
            .data_source = &test_section_list_data_source,
            .data_source_context = &test_section_list_context,
            .state_cache_max_entries = 6,
            .state_cache_max_bytes = 24,
    };

    memset(&test_section_list, 0, sizeof(test_section_list));
    reset_test_section_list_items();

    egui_view_virtual_section_list_init_with_setup(EGUI_VIEW_OF(&test_section_list), &setup);
    layout_section_list(0, 0, 100, 60);
}

static void layout_grid(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_view_layout(EGUI_VIEW_OF(&test_grid), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_grid)->region_screen, &region);
    EGUI_VIEW_OF(&test_grid)->api->calculate_layout(EGUI_VIEW_OF(&test_grid));
}

static void reset_test_grid_items(void)
{
    memset(&test_grid_context, 0, sizeof(test_grid_context));
    test_grid_context.item_count = 8;

    test_grid_context.stable_ids[0] = 7100;
    test_grid_context.stable_ids[1] = 7101;
    test_grid_context.stable_ids[2] = 7102;
    test_grid_context.stable_ids[3] = 7103;
    test_grid_context.stable_ids[4] = 7104;
    test_grid_context.stable_ids[5] = 7105;
    test_grid_context.stable_ids[6] = 7106;
    test_grid_context.stable_ids[7] = 7107;

    test_grid_context.item_heights[0] = 20;
    test_grid_context.item_heights[1] = 12;
    test_grid_context.item_heights[2] = 16;
    test_grid_context.item_heights[3] = 18;
    test_grid_context.item_heights[4] = 22;
    test_grid_context.item_heights[5] = 14;
    test_grid_context.item_heights[6] = 26;
    test_grid_context.item_heights[7] = 10;
}

static void setup_grid_with_data_source(void)
{
    const egui_view_virtual_grid_params_t params = {
            .region = {{0, 0}, {100, 40}},
            .column_count = 3,
            .overscan_before = 1,
            .overscan_after = 1,
            .max_keepalive_slots = 2,
            .column_spacing = 4,
            .row_spacing = 5,
            .estimated_item_height = 18,
    };
    const egui_view_virtual_grid_setup_t setup = {
            .params = &params,
            .data_source = &test_grid_data_source,
            .data_source_context = &test_grid_context,
            .state_cache_max_entries = 7,
            .state_cache_max_bytes = 28,
    };

    memset(&test_grid, 0, sizeof(test_grid));
    reset_test_grid_items();

    egui_view_virtual_grid_init_with_setup(EGUI_VIEW_OF(&test_grid), &setup);
    layout_grid(0, 0, 100, 40);
}

static int find_grid_slot_index_by_row_index(uint32_t row_index)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        const egui_view_virtual_grid_slot_t *slot = egui_view_virtual_grid_get_slot(EGUI_VIEW_OF(&test_grid), i);

        if (slot == NULL || slot->view == NULL)
        {
            continue;
        }
        if (slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE && slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE)
        {
            continue;
        }
        if (slot->index == row_index)
        {
            return (int)i;
        }
    }

    return -1;
}

static void layout_tree(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_view_layout(EGUI_VIEW_OF(&test_tree), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_tree)->region_screen, &region);
    EGUI_VIEW_OF(&test_tree)->api->calculate_layout(EGUI_VIEW_OF(&test_tree));
}

static void setup_tree_with_data_source(void)
{
    const egui_view_virtual_tree_params_t params = {
            .region = {{0, 0}, {100, 60}},
            .overscan_before = 1,
            .overscan_after = 1,
            .max_keepalive_slots = 2,
            .estimated_node_height = 18,
    };
    const egui_view_virtual_tree_setup_t setup = {
            .params = &params,
            .data_source = &test_tree_data_source,
            .data_source_context = &test_tree_context,
            .state_cache_max_entries = 8,
            .state_cache_max_bytes = 32,
    };

    memset(&test_tree, 0, sizeof(test_tree));
    reset_test_tree_items();

    egui_view_virtual_tree_init_with_setup(EGUI_VIEW_OF(&test_tree), &setup);
    layout_tree(0, 0, 100, 60);
}

static int find_tree_slot_index_by_stable_id(uint32_t stable_id)
{
    uint8_t i;
    uint8_t slot_count = egui_view_virtual_tree_get_slot_count(EGUI_VIEW_OF(&test_tree));

    for (i = 0; i < slot_count; i++)
    {
        const egui_view_virtual_tree_slot_t *slot = egui_view_virtual_tree_get_slot(EGUI_VIEW_OF(&test_tree), i);
        if (slot != NULL && slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->stable_id == stable_id)
        {
            return (int)i;
        }
    }

    return -1;
}

static int find_section_list_slot_index_by_stable_id(uint32_t stable_id)
{
    uint8_t i;
    uint8_t slot_count = egui_view_virtual_section_list_get_slot_count(EGUI_VIEW_OF(&test_section_list));

    for (i = 0; i < slot_count; i++)
    {
        const egui_view_virtual_section_list_slot_t *slot = egui_view_virtual_section_list_get_slot(EGUI_VIEW_OF(&test_section_list), i);
        if (slot != NULL && slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->stable_id == stable_id)
        {
            return (int)i;
        }
    }

    return -1;
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
    return egui_view_virtual_viewport_find_slot_by_stable_id(EGUI_VIEW_OF(&test_viewport), stable_id);
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

typedef struct test_visible_visit_context
{
    egui_view_t *expected_self;
    uint8_t stop_after;
    uint8_t visited;
    uint8_t saw_bad_self;
    uint8_t saw_bad_slot_state;
    uint8_t saw_null_view;
    uint8_t saw_bad_mapping;
    uint8_t saw_duplicate_stable_id;
    uint32_t stable_ids[TEST_VIRTUAL_GRID_MAX_POOL];
} test_visible_visit_context_t;

static void test_visible_visit_record(test_visible_visit_context_t *ctx, egui_view_t *self, uint8_t slot_is_visible, egui_view_t *view, uint32_t stable_id)
{
    uint8_t i;

    if (ctx == NULL)
    {
        return;
    }

    if (self != ctx->expected_self)
    {
        ctx->saw_bad_self = 1;
    }
    if (!slot_is_visible)
    {
        ctx->saw_bad_slot_state = 1;
    }
    if (view == NULL)
    {
        ctx->saw_null_view = 1;
    }

    for (i = 0; i < ctx->visited; i++)
    {
        if (ctx->stable_ids[i] == stable_id)
        {
            ctx->saw_duplicate_stable_id = 1;
            break;
        }
    }

    if (ctx->visited < TEST_VIRTUAL_GRID_MAX_POOL)
    {
        ctx->stable_ids[ctx->visited] = stable_id;
    }
    ctx->visited++;
}

static uint8_t test_visible_visit_should_continue(test_visible_visit_context_t *ctx)
{
    if (ctx == NULL || ctx->stop_after == 0)
    {
        return 1;
    }

    return ctx->visited < ctx->stop_after;
}

static void test_visible_visit_assert_ok(const test_visible_visit_context_t *ctx)
{
    EGUI_TEST_ASSERT_FALSE(ctx->saw_bad_self);
    EGUI_TEST_ASSERT_FALSE(ctx->saw_bad_slot_state);
    EGUI_TEST_ASSERT_FALSE(ctx->saw_null_view);
    EGUI_TEST_ASSERT_FALSE(ctx->saw_bad_mapping);
    EGUI_TEST_ASSERT_FALSE(ctx->saw_duplicate_stable_id);
}

static uint8_t count_visible_list_items_manual(void)
{
    uint8_t slot_count = egui_view_virtual_list_get_slot_count(EGUI_VIEW_OF(&test_list));
    uint8_t slot_index;
    uint8_t count = 0;

    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_list_entry_t entry;
        const egui_view_virtual_list_slot_t *slot = egui_view_virtual_list_get_slot(EGUI_VIEW_OF(&test_list), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
        {
            continue;
        }
        if (!egui_view_virtual_list_get_slot_entry(EGUI_VIEW_OF(&test_list), slot_index, &entry))
        {
            continue;
        }

        count++;
    }

    return count;
}

static uint8_t count_visible_viewport_items_manual(void)
{
    uint8_t slot_count = egui_view_virtual_viewport_get_slot_count(EGUI_VIEW_OF(&test_viewport));
    uint8_t slot_index;
    uint8_t count = 0;

    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_viewport_entry_t entry;
        const egui_view_virtual_viewport_slot_t *slot = egui_view_virtual_viewport_get_slot(EGUI_VIEW_OF(&test_viewport), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
        {
            continue;
        }
        if (!egui_view_virtual_viewport_get_slot_entry(EGUI_VIEW_OF(&test_viewport), slot_index, &entry))
        {
            continue;
        }

        count++;
    }

    return count;
}

static uint8_t count_visible_strip_items_manual(void)
{
    uint8_t slot_count = egui_view_virtual_strip_get_slot_count(EGUI_VIEW_OF(&test_strip));
    uint8_t slot_index;
    uint8_t count = 0;

    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_strip_entry_t entry;
        const egui_view_virtual_strip_slot_t *slot = egui_view_virtual_strip_get_slot(EGUI_VIEW_OF(&test_strip), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
        {
            continue;
        }
        if (!egui_view_virtual_strip_get_slot_entry(EGUI_VIEW_OF(&test_strip), slot_index, &entry))
        {
            continue;
        }

        count++;
    }

    return count;
}

static uint8_t count_visible_page_sections_manual(void)
{
    uint8_t slot_count = egui_view_virtual_page_get_slot_count(EGUI_VIEW_OF(&test_page));
    uint8_t slot_index;
    uint8_t count = 0;

    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_page_entry_t entry;
        const egui_view_virtual_page_slot_t *slot = egui_view_virtual_page_get_slot(EGUI_VIEW_OF(&test_page), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
        {
            continue;
        }
        if (!egui_view_virtual_page_get_slot_entry(EGUI_VIEW_OF(&test_page), slot_index, &entry))
        {
            continue;
        }

        count++;
    }

    return count;
}

static uint8_t count_visible_section_list_entries_manual(void)
{
    uint8_t slot_count = egui_view_virtual_section_list_get_slot_count(EGUI_VIEW_OF(&test_section_list));
    uint8_t slot_index;
    uint8_t count = 0;

    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_section_list_entry_t entry;
        const egui_view_virtual_section_list_slot_t *slot = egui_view_virtual_section_list_get_slot(EGUI_VIEW_OF(&test_section_list), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
        {
            continue;
        }
        if (!egui_view_virtual_section_list_get_slot_entry(EGUI_VIEW_OF(&test_section_list), slot_index, &entry))
        {
            continue;
        }

        count++;
    }

    return count;
}

static uint8_t count_visible_grid_items_manual(void)
{
    uint8_t slot_count = egui_view_virtual_grid_get_slot_count(EGUI_VIEW_OF(&test_grid));
    uint8_t slot_index;
    uint8_t count = 0;

    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        uint8_t item_count;
        uint8_t column_index;
        const egui_view_virtual_grid_slot_t *slot = egui_view_virtual_grid_get_slot(EGUI_VIEW_OF(&test_grid), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE)
        {
            continue;
        }

        item_count = egui_view_virtual_grid_get_slot_item_count(EGUI_VIEW_OF(&test_grid), slot_index);
        for (column_index = 0; column_index < item_count; column_index++)
        {
            egui_view_virtual_grid_entry_t entry;
            egui_view_t *item_view = egui_view_virtual_grid_get_slot_item_view(EGUI_VIEW_OF(&test_grid), slot_index, column_index);

            if (item_view == NULL)
            {
                continue;
            }
            if (!egui_view_virtual_grid_get_slot_entry(EGUI_VIEW_OF(&test_grid), slot_index, column_index, &entry))
            {
                continue;
            }

            count++;
        }
    }

    return count;
}

static uint8_t count_visible_tree_nodes_manual(void)
{
    uint8_t slot_count = egui_view_virtual_tree_get_slot_count(EGUI_VIEW_OF(&test_tree));
    uint8_t slot_index;
    uint8_t count = 0;

    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_tree_entry_t entry;
        const egui_view_virtual_tree_slot_t *slot = egui_view_virtual_tree_get_slot(EGUI_VIEW_OF(&test_tree), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
        {
            continue;
        }
        if (!egui_view_virtual_tree_get_slot_entry(EGUI_VIEW_OF(&test_tree), slot_index, &entry))
        {
            continue;
        }

        count++;
    }

    return count;
}

static uint8_t test_virtual_viewport_visit_visible_item(egui_view_t *self, const egui_view_virtual_viewport_slot_t *slot,
                                                        const egui_view_virtual_viewport_entry_t *entry, egui_view_t *item_view, void *context)
{
    test_visible_visit_context_t *ctx = (test_visible_visit_context_t *)context;
    egui_view_virtual_viewport_entry_t resolved;

    if (slot == NULL || entry == NULL || slot->view != item_view || !egui_view_virtual_viewport_resolve_item_by_view(self, item_view, &resolved) ||
        resolved.index != entry->index || resolved.stable_id != entry->stable_id || resolved.view_type != entry->view_type)
    {
        ctx->saw_bad_mapping = 1;
    }

    test_visible_visit_record(ctx, self, slot != NULL && slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE, item_view,
                              entry != NULL ? entry->stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    return test_visible_visit_should_continue(ctx);
}

static uint8_t test_virtual_list_visit_visible_item(egui_view_t *self, const egui_view_virtual_list_slot_t *slot, const egui_view_virtual_list_entry_t *entry,
                                                    egui_view_t *item_view, void *context)
{
    test_visible_visit_context_t *ctx = (test_visible_visit_context_t *)context;
    egui_view_virtual_list_entry_t resolved;

    if (slot == NULL || entry == NULL || slot->view != item_view || !egui_view_virtual_list_resolve_item_by_view(self, item_view, &resolved) ||
        resolved.index != entry->index || resolved.stable_id != entry->stable_id)
    {
        ctx->saw_bad_mapping = 1;
    }

    test_visible_visit_record(ctx, self, slot != NULL && slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE, item_view,
                              entry != NULL ? entry->stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    return test_visible_visit_should_continue(ctx);
}

static uint8_t test_virtual_strip_visit_visible_item(egui_view_t *self, const egui_view_virtual_strip_slot_t *slot,
                                                     const egui_view_virtual_strip_entry_t *entry, egui_view_t *item_view, void *context)
{
    test_visible_visit_context_t *ctx = (test_visible_visit_context_t *)context;
    egui_view_virtual_strip_entry_t resolved;

    if (slot == NULL || entry == NULL || slot->view != item_view || !egui_view_virtual_strip_resolve_item_by_view(self, item_view, &resolved) ||
        resolved.index != entry->index || resolved.stable_id != entry->stable_id)
    {
        ctx->saw_bad_mapping = 1;
    }

    test_visible_visit_record(ctx, self, slot != NULL && slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE, item_view,
                              entry != NULL ? entry->stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    return test_visible_visit_should_continue(ctx);
}

static uint8_t test_virtual_page_visit_visible_section(egui_view_t *self, const egui_view_virtual_page_slot_t *slot,
                                                       const egui_view_virtual_page_entry_t *entry, egui_view_t *section_view, void *context)
{
    test_visible_visit_context_t *ctx = (test_visible_visit_context_t *)context;
    egui_view_virtual_page_entry_t resolved;

    if (slot == NULL || entry == NULL || slot->view != section_view || !egui_view_virtual_page_resolve_section_by_view(self, section_view, &resolved) ||
        resolved.index != entry->index || resolved.stable_id != entry->stable_id)
    {
        ctx->saw_bad_mapping = 1;
    }

    test_visible_visit_record(ctx, self, slot != NULL && slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE, section_view,
                              entry != NULL ? entry->stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    return test_visible_visit_should_continue(ctx);
}

static uint8_t test_virtual_section_list_visit_visible_entry(egui_view_t *self, const egui_view_virtual_section_list_slot_t *slot,
                                                             const egui_view_virtual_section_list_entry_t *entry, egui_view_t *entry_view, void *context)
{
    test_visible_visit_context_t *ctx = (test_visible_visit_context_t *)context;
    egui_view_virtual_section_list_entry_t resolved;

    if (slot == NULL || entry == NULL || slot->view != entry_view || !egui_view_virtual_section_list_resolve_entry_by_view(self, entry_view, &resolved) ||
        resolved.stable_id != entry->stable_id || resolved.section_index != entry->section_index || resolved.item_index != entry->item_index ||
        resolved.is_section_header != entry->is_section_header)
    {
        ctx->saw_bad_mapping = 1;
    }

    test_visible_visit_record(ctx, self, slot != NULL && slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE, entry_view,
                              entry != NULL ? entry->stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    return test_visible_visit_should_continue(ctx);
}

static uint8_t test_virtual_grid_visit_visible_item(egui_view_t *self, const egui_view_virtual_grid_slot_t *slot, const egui_view_virtual_grid_entry_t *entry,
                                                    egui_view_t *item_view, void *context)
{
    test_visible_visit_context_t *ctx = (test_visible_visit_context_t *)context;
    egui_view_virtual_grid_entry_t resolved;

    if (slot == NULL || entry == NULL || !egui_view_virtual_grid_resolve_item_by_view(self, item_view, &resolved) || resolved.index != entry->index ||
        resolved.row_index != entry->row_index || resolved.column_index != entry->column_index || resolved.stable_id != entry->stable_id)
    {
        ctx->saw_bad_mapping = 1;
    }

    test_visible_visit_record(ctx, self, slot != NULL && slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE, item_view,
                              entry != NULL ? entry->stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    return test_visible_visit_should_continue(ctx);
}

static uint8_t test_virtual_tree_visit_visible_node(egui_view_t *self, const egui_view_virtual_tree_slot_t *slot, const egui_view_virtual_tree_entry_t *entry,
                                                    egui_view_t *node_view, void *context)
{
    test_visible_visit_context_t *ctx = (test_visible_visit_context_t *)context;
    egui_view_virtual_tree_entry_t resolved;

    if (slot == NULL || entry == NULL || slot->view != node_view || !egui_view_virtual_tree_resolve_node_by_view(self, node_view, &resolved) ||
        resolved.stable_id != entry->stable_id || resolved.visible_index != entry->visible_index || resolved.depth != entry->depth ||
        resolved.has_children != entry->has_children || resolved.is_expanded != entry->is_expanded)
    {
        ctx->saw_bad_mapping = 1;
    }

    test_visible_visit_record(ctx, self, slot != NULL && slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE, node_view,
                              entry != NULL ? entry->stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    return test_visible_visit_should_continue(ctx);
}

typedef struct test_min_index_match_context
{
    uint32_t min_index;
} test_min_index_match_context_t;

typedef struct test_section_entry_match_context
{
    uint32_t min_section_index;
    uint8_t want_header;
} test_section_entry_match_context_t;

typedef struct test_tree_node_match_context
{
    uint32_t min_visible_index;
    uint8_t want_branch;
    uint8_t min_depth;
} test_tree_node_match_context_t;

static uint8_t test_virtual_viewport_match_item(egui_view_t *self, const egui_view_virtual_viewport_slot_t *slot,
                                                const egui_view_virtual_viewport_entry_t *entry, egui_view_t *item_view, void *context)
{
    test_min_index_match_context_t *ctx = (test_min_index_match_context_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(slot);
    EGUI_UNUSED(item_view);
    return (uint8_t)(entry != NULL && entry->index >= ctx->min_index);
}

static uint8_t test_virtual_list_match_item(egui_view_t *self, const egui_view_virtual_list_slot_t *slot, const egui_view_virtual_list_entry_t *entry,
                                            egui_view_t *item_view, void *context)
{
    test_min_index_match_context_t *ctx = (test_min_index_match_context_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(slot);
    EGUI_UNUSED(item_view);
    return (uint8_t)(entry != NULL && entry->index >= ctx->min_index);
}

static uint8_t test_virtual_strip_match_item(egui_view_t *self, const egui_view_virtual_strip_slot_t *slot, const egui_view_virtual_strip_entry_t *entry,
                                             egui_view_t *item_view, void *context)
{
    test_min_index_match_context_t *ctx = (test_min_index_match_context_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(slot);
    EGUI_UNUSED(item_view);
    return (uint8_t)(entry != NULL && entry->index >= ctx->min_index);
}

static uint8_t test_virtual_page_match_section(egui_view_t *self, const egui_view_virtual_page_slot_t *slot, const egui_view_virtual_page_entry_t *entry,
                                               egui_view_t *section_view, void *context)
{
    test_min_index_match_context_t *ctx = (test_min_index_match_context_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(slot);
    EGUI_UNUSED(section_view);
    return (uint8_t)(entry != NULL && entry->index >= ctx->min_index);
}

static uint8_t test_virtual_section_list_match_entry(egui_view_t *self, const egui_view_virtual_section_list_slot_t *slot,
                                                     const egui_view_virtual_section_list_entry_t *entry, egui_view_t *entry_view, void *context)
{
    test_section_entry_match_context_t *ctx = (test_section_entry_match_context_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(slot);
    EGUI_UNUSED(entry_view);
    return (uint8_t)(entry != NULL && entry->is_section_header == ctx->want_header && entry->section_index >= ctx->min_section_index);
}

static uint8_t test_virtual_grid_match_item(egui_view_t *self, const egui_view_virtual_grid_slot_t *slot, const egui_view_virtual_grid_entry_t *entry,
                                            egui_view_t *item_view, void *context)
{
    test_min_index_match_context_t *ctx = (test_min_index_match_context_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(slot);
    EGUI_UNUSED(item_view);
    return (uint8_t)(entry != NULL && entry->index >= ctx->min_index);
}

static uint8_t test_virtual_tree_match_node(egui_view_t *self, const egui_view_virtual_tree_slot_t *slot, const egui_view_virtual_tree_entry_t *entry,
                                            egui_view_t *node_view, void *context)
{
    test_tree_node_match_context_t *ctx = (test_tree_node_match_context_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(slot);
    EGUI_UNUSED(node_view);
    return (uint8_t)(entry != NULL && entry->visible_index >= ctx->min_visible_index && entry->depth >= ctx->min_depth &&
                     entry->has_children == ctx->want_branch);
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

static void test_virtual_viewport_slot_visibility_helpers(void)
{
    const egui_view_virtual_viewport_slot_t *slot;

    setup_viewport(40, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);

    slot = find_slot_by_stable_id(1000);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_is_slot_center_visible(EGUI_VIEW_OF(&test_viewport), slot));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_is_slot_fully_visible(EGUI_VIEW_OF(&test_viewport), slot, 0));

    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&test_viewport), 10);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    slot = find_slot_by_stable_id(1000);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_is_slot_center_visible(EGUI_VIEW_OF(&test_viewport), slot));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_viewport_is_slot_fully_visible(EGUI_VIEW_OF(&test_viewport), slot, 0));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_is_slot_fully_visible(EGUI_VIEW_OF(&test_viewport), slot, 10));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_is_main_span_center_visible(EGUI_VIEW_OF(&test_viewport), -10, 20));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_viewport_is_main_span_fully_visible(EGUI_VIEW_OF(&test_viewport), -10, 20, 0));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_is_main_span_fully_visible(EGUI_VIEW_OF(&test_viewport), -10, 20, 10));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_viewport_is_main_span_center_visible(EGUI_VIEW_OF(&test_viewport), 80, 20));

    setup_strip_with_data_source();

    slot = egui_view_virtual_strip_find_slot_by_stable_id(EGUI_VIEW_OF(&test_strip), 8102);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_is_slot_center_visible(EGUI_VIEW_OF(&test_strip), slot));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_is_slot_fully_visible(EGUI_VIEW_OF(&test_strip), slot, 0));

    slot = egui_view_virtual_strip_find_slot_by_stable_id(EGUI_VIEW_OF(&test_strip), 8104);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_viewport_is_slot_center_visible(EGUI_VIEW_OF(&test_strip), slot));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_viewport_is_slot_fully_visible(EGUI_VIEW_OF(&test_strip), slot, 0));
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

static void test_virtual_viewport_setup_and_lookup_helpers(void)
{
    egui_view_virtual_viewport_entry_t entry;
    egui_view_virtual_viewport_setup_t setup;
    const egui_view_virtual_viewport_slot_t *slot;
    egui_view_t *item_view;
    int32_t slot_index;

    EGUI_VIEW_VIRTUAL_VIEWPORT_PARAMS_INIT(test_viewport_params, 0, 0, 100, 60);
    EGUI_VIEW_VIRTUAL_VIEWPORT_SETUP_INIT(test_viewport_setup_defaults, &test_viewport_params, &test_adapter, &test_context);

    memset(&test_viewport, 0, sizeof(test_viewport));
    memset(&test_context, 0, sizeof(test_context));
    reset_test_items(40, 20);

    setup = test_viewport_setup_defaults;
    setup.state_cache_max_entries = 5;
    setup.state_cache_max_bytes = 20;

    egui_view_virtual_viewport_init_with_setup(EGUI_VIEW_OF(&test_viewport), &setup);
    layout_viewport(0, 0, 100, 60);

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_get_adapter(EGUI_VIEW_OF(&test_viewport)) == &test_adapter);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_get_adapter_context(EGUI_VIEW_OF(&test_viewport)) == &test_context);
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_virtual_viewport_get_state_cache_entry_limit(EGUI_VIEW_OF(&test_viewport)));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_viewport_get_state_cache_byte_limit(EGUI_VIEW_OF(&test_viewport)));

    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_viewport_find_item_index_by_stable_id(EGUI_VIEW_OF(&test_viewport), 1020));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_resolve_item_by_stable_id(EGUI_VIEW_OF(&test_viewport), 1020, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(20, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1020, entry.stable_id);
    EGUI_TEST_ASSERT_EQUAL_INT(0, entry.view_type);
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_viewport_resolve_item_by_stable_id(EGUI_VIEW_OF(&test_viewport), 999999, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, entry.stable_id);

    slot_index = egui_view_virtual_viewport_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_viewport), 1000);
    EGUI_TEST_ASSERT_TRUE(slot_index >= 0);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_viewport_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_viewport), 999999));

    slot = egui_view_virtual_viewport_find_slot_by_stable_id(EGUI_VIEW_OF(&test_viewport), 1000);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(slot == egui_view_virtual_viewport_get_slot(EGUI_VIEW_OF(&test_viewport), (uint8_t)slot_index));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_viewport_find_slot_by_stable_id(EGUI_VIEW_OF(&test_viewport), 999999));

    item_view = egui_view_virtual_viewport_find_view_by_stable_id(EGUI_VIEW_OF(&test_viewport), 1000);
    EGUI_TEST_ASSERT_TRUE(item_view == slot->view);
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_viewport_find_view_by_stable_id(EGUI_VIEW_OF(&test_viewport), 999999));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_get_slot_entry(EGUI_VIEW_OF(&test_viewport), (uint8_t)slot_index, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(0, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1000, entry.stable_id);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_resolve_item_by_view(EGUI_VIEW_OF(&test_viewport), item_view, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(0, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1000, entry.stable_id);
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_viewport_resolve_item_by_view(EGUI_VIEW_OF(&test_viewport), NULL, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, entry.stable_id);
}

static void test_virtual_viewport_visit_visible_items_helper(void)
{
    test_visible_visit_context_t ctx;
    uint8_t manual_count;

    setup_viewport(40, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&test_viewport), 55);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    manual_count = count_visible_viewport_items_manual();
    EGUI_TEST_ASSERT_TRUE(manual_count >= 2);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_viewport_visit_visible_items(EGUI_VIEW_OF(&test_viewport), NULL, NULL));

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_viewport);
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count,
                               egui_view_virtual_viewport_visit_visible_items(EGUI_VIEW_OF(&test_viewport), test_virtual_viewport_visit_visible_item, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count, ctx.visited);
    test_visible_visit_assert_ok(&ctx);

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_viewport);
    ctx.stop_after = 2;
    EGUI_TEST_ASSERT_EQUAL_INT(2,
                               egui_view_virtual_viewport_visit_visible_items(EGUI_VIEW_OF(&test_viewport), test_virtual_viewport_visit_visible_item, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(2, ctx.visited);
    test_visible_visit_assert_ok(&ctx);
}

static void test_virtual_viewport_find_first_visible_item_view_helper(void)
{
    egui_view_virtual_viewport_entry_t expected_entry = {EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, 0};
    egui_view_virtual_viewport_entry_t actual_entry;
    egui_view_t *expected_view = NULL;
    egui_view_t *actual_view;
    test_min_index_match_context_t match_ctx = {.min_index = 3};
    uint8_t slot_count;
    uint8_t slot_index;

    setup_viewport(40, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&test_viewport), 55);
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    slot_count = egui_view_virtual_viewport_get_slot_count(EGUI_VIEW_OF(&test_viewport));
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_viewport_entry_t entry;
        const egui_view_virtual_viewport_slot_t *slot = egui_view_virtual_viewport_get_slot(EGUI_VIEW_OF(&test_viewport), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_viewport_get_slot_entry(EGUI_VIEW_OF(&test_viewport), slot_index, &entry))
        {
            continue;
        }
        if (expected_view == NULL || entry.index < expected_entry.index)
        {
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_viewport_find_first_visible_item_view(EGUI_VIEW_OF(&test_viewport), NULL, NULL, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.index, actual_entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.view_type, actual_entry.view_type);

    expected_view = NULL;
    expected_entry.index = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    expected_entry.stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    expected_entry.view_type = 0;
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_viewport_entry_t entry;
        const egui_view_virtual_viewport_slot_t *slot = egui_view_virtual_viewport_get_slot(EGUI_VIEW_OF(&test_viewport), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_viewport_get_slot_entry(EGUI_VIEW_OF(&test_viewport), slot_index, &entry) || entry.index < match_ctx.min_index)
        {
            continue;
        }
        if (expected_view == NULL || entry.index < expected_entry.index)
        {
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_viewport_find_first_visible_item_view(EGUI_VIEW_OF(&test_viewport), test_virtual_viewport_match_item, &match_ctx,
                                                                          &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.index, actual_entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.view_type, actual_entry.view_type);
}

static void test_virtual_viewport_ensure_visible_helper(void)
{
    setup_viewport(40, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);

    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_viewport_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_viewport), 999999, 7));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_viewport), 1020, 7));
    EGUI_TEST_ASSERT_EQUAL_INT(407, egui_view_virtual_viewport_get_logical_offset(EGUI_VIEW_OF(&test_viewport)));
    EGUI_VIEW_OF(&test_viewport)->api->calculate_layout(EGUI_VIEW_OF(&test_viewport));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_viewport_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_viewport), 1020, 7));
    EGUI_TEST_ASSERT_EQUAL_INT(407, egui_view_virtual_viewport_get_logical_offset(EGUI_VIEW_OF(&test_viewport)));
}

static void test_virtual_list_data_source_defaults_bridge_viewport_adapter(void)
{
    setup_list_with_data_source(40);

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_list_get_data_source(EGUI_VIEW_OF(&test_list)) == &test_list_data_source);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_list_get_data_source_context(EGUI_VIEW_OF(&test_list)) == &test_context);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_virtual_list_get_state_cache_entry_limit(EGUI_VIEW_OF(&test_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(12, egui_view_virtual_list_get_state_cache_byte_limit(EGUI_VIEW_OF(&test_list)));

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
    egui_view_virtual_list_entry_t entry;
    const egui_view_virtual_list_slot_t *slot;
    int slot_index;

    setup_list_with_data_source_config(40, &test_list_measured_data_source);

    EGUI_TEST_ASSERT_EQUAL_INT(40, egui_view_virtual_list_get_item_count(EGUI_VIEW_OF(&test_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_list_find_index_by_stable_id(EGUI_VIEW_OF(&test_list), 1020));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_list_find_index_by_stable_id(EGUI_VIEW_OF(&test_list), 999999));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_list_resolve_item_by_stable_id(EGUI_VIEW_OF(&test_list), 1020, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(20, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1020, entry.stable_id);
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_list_resolve_item_by_stable_id(EGUI_VIEW_OF(&test_list), 999999, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, entry.stable_id);
    EGUI_TEST_ASSERT_EQUAL_INT(400, egui_view_virtual_list_get_item_y_by_stable_id(EGUI_VIEW_OF(&test_list), 1020));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_list_get_item_height_by_stable_id(EGUI_VIEW_OF(&test_list), 1020));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_list_get_item_y_by_stable_id(EGUI_VIEW_OF(&test_list), 999999));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_list_get_item_height_by_stable_id(EGUI_VIEW_OF(&test_list), 999999));

    slot_index = find_list_slot_index_by_stable_id(1000);
    EGUI_TEST_ASSERT_TRUE(slot_index >= 0);
    EGUI_TEST_ASSERT_EQUAL_INT(slot_index, egui_view_virtual_list_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_list), 1000));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_list_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_list), 999999));
    slot = egui_view_virtual_list_get_slot(EGUI_VIEW_OF(&test_list), (uint8_t)slot_index);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(slot == egui_view_virtual_list_find_slot_by_stable_id(EGUI_VIEW_OF(&test_list), 1000));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_list_find_slot_by_stable_id(EGUI_VIEW_OF(&test_list), 999999));
    EGUI_TEST_ASSERT_TRUE(slot->view == egui_view_virtual_list_find_view_by_stable_id(EGUI_VIEW_OF(&test_list), 1000));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_list_find_view_by_stable_id(EGUI_VIEW_OF(&test_list), 999999));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_list_get_slot_entry(EGUI_VIEW_OF(&test_list), (uint8_t)slot_index, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(0, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1000, entry.stable_id);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_list_resolve_item_by_view(EGUI_VIEW_OF(&test_list), slot->view, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(0, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1000, entry.stable_id);

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

static void test_virtual_list_visit_visible_items_helper(void)
{
    test_visible_visit_context_t ctx;
    uint8_t manual_count;

    setup_list_with_data_source(40);
    egui_view_virtual_list_set_scroll_y(EGUI_VIEW_OF(&test_list), 55);
    EGUI_VIEW_OF(&test_list)->api->calculate_layout(EGUI_VIEW_OF(&test_list));

    manual_count = count_visible_list_items_manual();
    EGUI_TEST_ASSERT_TRUE(manual_count >= 2);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_list_visit_visible_items(EGUI_VIEW_OF(&test_list), NULL, NULL));

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_list);
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count, egui_view_virtual_list_visit_visible_items(EGUI_VIEW_OF(&test_list), test_virtual_list_visit_visible_item, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count, ctx.visited);
    test_visible_visit_assert_ok(&ctx);

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_list);
    ctx.stop_after = 2;
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_list_visit_visible_items(EGUI_VIEW_OF(&test_list), test_virtual_list_visit_visible_item, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(2, ctx.visited);
    test_visible_visit_assert_ok(&ctx);
}

static void test_virtual_list_find_first_visible_item_view_helper(void)
{
    egui_view_virtual_list_entry_t expected_entry = {EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID};
    egui_view_virtual_list_entry_t actual_entry;
    egui_view_t *expected_view = NULL;
    egui_view_t *actual_view;
    test_min_index_match_context_t match_ctx = {.min_index = 3};
    uint8_t slot_count;
    uint8_t slot_index;

    setup_list_with_data_source(40);
    egui_view_virtual_list_set_scroll_y(EGUI_VIEW_OF(&test_list), 55);
    EGUI_VIEW_OF(&test_list)->api->calculate_layout(EGUI_VIEW_OF(&test_list));

    slot_count = egui_view_virtual_list_get_slot_count(EGUI_VIEW_OF(&test_list));
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_list_entry_t entry;
        const egui_view_virtual_list_slot_t *slot = egui_view_virtual_list_get_slot(EGUI_VIEW_OF(&test_list), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_list_get_slot_entry(EGUI_VIEW_OF(&test_list), slot_index, &entry))
        {
            continue;
        }
        if (expected_view == NULL || entry.index < expected_entry.index)
        {
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_list_find_first_visible_item_view(EGUI_VIEW_OF(&test_list), NULL, NULL, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.index, actual_entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);

    expected_view = NULL;
    expected_entry.index = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    expected_entry.stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_list_entry_t entry;
        const egui_view_virtual_list_slot_t *slot = egui_view_virtual_list_get_slot(EGUI_VIEW_OF(&test_list), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_list_get_slot_entry(EGUI_VIEW_OF(&test_list), slot_index, &entry) || entry.index < match_ctx.min_index)
        {
            continue;
        }
        if (expected_view == NULL || entry.index < expected_entry.index)
        {
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_list_find_first_visible_item_view(EGUI_VIEW_OF(&test_list), test_virtual_list_match_item, &match_ctx, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.index, actual_entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);
}

static void test_virtual_list_ensure_visible_helper(void)
{
    setup_list_with_data_source_config(40, &test_list_measured_data_source);

    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_list_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_list), 999999, 7));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_list_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_list), 1020, 7));
    EGUI_TEST_ASSERT_EQUAL_INT(407, egui_view_virtual_list_get_scroll_y(EGUI_VIEW_OF(&test_list)));
    EGUI_VIEW_OF(&test_list)->api->calculate_layout(EGUI_VIEW_OF(&test_list));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_list_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_list), 1020, 7));
    EGUI_TEST_ASSERT_EQUAL_INT(407, egui_view_virtual_list_get_scroll_y(EGUI_VIEW_OF(&test_list)));
}

static void test_virtual_strip_bridge_and_lookup_helpers(void)
{
    egui_view_virtual_strip_entry_t entry;
    const egui_view_virtual_strip_slot_t *slot;
    egui_view_t *item_view;
    int slot_index;

    setup_strip_with_data_source();

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_strip_get_data_source(EGUI_VIEW_OF(&test_strip)) == &test_strip_data_source);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_strip_get_data_source_context(EGUI_VIEW_OF(&test_strip)) == &test_context);
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_virtual_strip_get_state_cache_entry_limit(EGUI_VIEW_OF(&test_strip)));
    EGUI_TEST_ASSERT_EQUAL_INT(16, egui_view_virtual_strip_get_state_cache_byte_limit(EGUI_VIEW_OF(&test_strip)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, egui_view_virtual_strip_get_item_count(EGUI_VIEW_OF(&test_strip)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_virtual_strip_find_index_by_stable_id(EGUI_VIEW_OF(&test_strip), 8104));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_strip_find_index_by_stable_id(EGUI_VIEW_OF(&test_strip), 999999));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_strip_resolve_item_by_stable_id(EGUI_VIEW_OF(&test_strip), 8104, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(4, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(8104, entry.stable_id);
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_strip_resolve_item_by_stable_id(EGUI_VIEW_OF(&test_strip), 999999, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, entry.stable_id);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_strip_get_item_x(EGUI_VIEW_OF(&test_strip), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(18, egui_view_virtual_strip_get_item_x(EGUI_VIEW_OF(&test_strip), 1));
    EGUI_TEST_ASSERT_EQUAL_INT(40, egui_view_virtual_strip_get_item_x(EGUI_VIEW_OF(&test_strip), 2));
    EGUI_TEST_ASSERT_EQUAL_INT(26, egui_view_virtual_strip_get_item_width(EGUI_VIEW_OF(&test_strip), 3));
    EGUI_TEST_ASSERT_EQUAL_INT(80, egui_view_virtual_strip_get_item_x_by_stable_id(EGUI_VIEW_OF(&test_strip), 8104));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_strip_get_item_width_by_stable_id(EGUI_VIEW_OF(&test_strip), 8104));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_strip_get_item_x_by_stable_id(EGUI_VIEW_OF(&test_strip), 999999));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_strip_get_item_width_by_stable_id(EGUI_VIEW_OF(&test_strip), 999999));

    item_view = egui_view_virtual_strip_find_view_by_stable_id(EGUI_VIEW_OF(&test_strip), 8102);
    EGUI_TEST_ASSERT_NOT_NULL(item_view);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_strip_resolve_item_by_view(EGUI_VIEW_OF(&test_strip), item_view, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(2, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(8102, entry.stable_id);

    slot_index = find_strip_slot_index_by_stable_id(8102);
    EGUI_TEST_ASSERT_TRUE(slot_index >= 0);
    EGUI_TEST_ASSERT_EQUAL_INT(slot_index, egui_view_virtual_strip_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_strip), 8102));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_strip_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_strip), 999999));
    slot = egui_view_virtual_strip_get_slot(EGUI_VIEW_OF(&test_strip), (uint8_t)slot_index);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(slot == egui_view_virtual_strip_find_slot_by_stable_id(EGUI_VIEW_OF(&test_strip), 8102));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_strip_find_slot_by_stable_id(EGUI_VIEW_OF(&test_strip), 999999));
    EGUI_TEST_ASSERT_TRUE(slot->view == egui_view_virtual_strip_find_view_by_stable_id(EGUI_VIEW_OF(&test_strip), 8102));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_strip_get_slot_entry(EGUI_VIEW_OF(&test_strip), (uint8_t)slot_index, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(2, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(8102, entry.stable_id);

    egui_view_virtual_strip_scroll_to_stable_id(EGUI_VIEW_OF(&test_strip), 8102, 6);
    EGUI_VIEW_OF(&test_strip)->api->calculate_layout(EGUI_VIEW_OF(&test_strip));
    EGUI_TEST_ASSERT_EQUAL_INT(46, egui_view_virtual_strip_get_scroll_x(EGUI_VIEW_OF(&test_strip)));
}

static void test_virtual_strip_resize_helpers_bridge_viewport(void)
{
    setup_strip_with_data_source();

    egui_view_virtual_strip_scroll_to_stable_id(EGUI_VIEW_OF(&test_strip), 8102, 6);
    EGUI_VIEW_OF(&test_strip)->api->calculate_layout(EGUI_VIEW_OF(&test_strip));
    EGUI_TEST_ASSERT_EQUAL_INT(80, egui_view_virtual_strip_get_item_x(EGUI_VIEW_OF(&test_strip), 4));

    test_context.item_heights[0] = 26;
    egui_view_virtual_strip_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&test_strip), 8100);
    EGUI_VIEW_OF(&test_strip)->api->calculate_layout(EGUI_VIEW_OF(&test_strip));

    EGUI_TEST_ASSERT_EQUAL_INT(88, egui_view_virtual_strip_get_item_x(EGUI_VIEW_OF(&test_strip), 4));
    EGUI_TEST_ASSERT_EQUAL_INT(26, egui_view_virtual_strip_get_item_width_by_stable_id(EGUI_VIEW_OF(&test_strip), 8100));
    EGUI_TEST_ASSERT_EQUAL_INT(54, egui_view_virtual_strip_get_scroll_x(EGUI_VIEW_OF(&test_strip)));
}

static void test_virtual_strip_state_cache_helpers_bridge_viewport(void)
{
    const egui_view_virtual_strip_slot_t *slot;
    uint8_t value = 0;

    setup_strip_with_data_source();
    egui_view_virtual_strip_set_state_cache_limits(EGUI_VIEW_OF(&test_strip), 2, 4);

    value = 41;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_strip_write_item_state(EGUI_VIEW_OF(&test_strip), 8103, &value, sizeof(value)));
    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_strip_read_item_state(EGUI_VIEW_OF(&test_strip), 8103, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(41, value);

    slot = egui_view_virtual_strip_get_slot(EGUI_VIEW_OF(&test_strip), 0);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    value = 53;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_strip_write_item_state_for_view(slot->view, slot->stable_id, &value, sizeof(value)));
    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_strip_read_item_state_for_view(slot->view, slot->stable_id, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(53, value);

    egui_view_virtual_strip_remove_item_state_by_stable_id(EGUI_VIEW_OF(&test_strip), 8103);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_strip_read_item_state(EGUI_VIEW_OF(&test_strip), 8103, &value, sizeof(value)));
}

static void test_virtual_strip_visit_visible_items_helper(void)
{
    test_visible_visit_context_t ctx;
    uint8_t manual_count;

    setup_strip_with_data_source();
    egui_view_virtual_strip_set_scroll_x(EGUI_VIEW_OF(&test_strip), 28);
    EGUI_VIEW_OF(&test_strip)->api->calculate_layout(EGUI_VIEW_OF(&test_strip));

    manual_count = count_visible_strip_items_manual();
    EGUI_TEST_ASSERT_TRUE(manual_count >= 2);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_strip_visit_visible_items(EGUI_VIEW_OF(&test_strip), NULL, NULL));

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_strip);
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count,
                               egui_view_virtual_strip_visit_visible_items(EGUI_VIEW_OF(&test_strip), test_virtual_strip_visit_visible_item, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count, ctx.visited);
    test_visible_visit_assert_ok(&ctx);

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_strip);
    ctx.stop_after = 2;
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_strip_visit_visible_items(EGUI_VIEW_OF(&test_strip), test_virtual_strip_visit_visible_item, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(2, ctx.visited);
    test_visible_visit_assert_ok(&ctx);
}

static void test_virtual_strip_find_first_visible_item_view_helper(void)
{
    egui_view_virtual_strip_entry_t expected_entry = {EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID};
    egui_view_virtual_strip_entry_t actual_entry;
    egui_view_t *expected_view = NULL;
    egui_view_t *actual_view;
    test_min_index_match_context_t match_ctx = {.min_index = 3};
    uint8_t slot_count;
    uint8_t slot_index;

    setup_strip_with_data_source();
    egui_view_virtual_strip_set_scroll_x(EGUI_VIEW_OF(&test_strip), 28);
    EGUI_VIEW_OF(&test_strip)->api->calculate_layout(EGUI_VIEW_OF(&test_strip));

    slot_count = egui_view_virtual_strip_get_slot_count(EGUI_VIEW_OF(&test_strip));
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_strip_entry_t entry;
        const egui_view_virtual_strip_slot_t *slot = egui_view_virtual_strip_get_slot(EGUI_VIEW_OF(&test_strip), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_strip_get_slot_entry(EGUI_VIEW_OF(&test_strip), slot_index, &entry))
        {
            continue;
        }
        if (expected_view == NULL || entry.index < expected_entry.index)
        {
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_strip_find_first_visible_item_view(EGUI_VIEW_OF(&test_strip), NULL, NULL, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.index, actual_entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);

    expected_view = NULL;
    expected_entry.index = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    expected_entry.stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_strip_entry_t entry;
        const egui_view_virtual_strip_slot_t *slot = egui_view_virtual_strip_get_slot(EGUI_VIEW_OF(&test_strip), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_strip_get_slot_entry(EGUI_VIEW_OF(&test_strip), slot_index, &entry) || entry.index < match_ctx.min_index)
        {
            continue;
        }
        if (expected_view == NULL || entry.index < expected_entry.index)
        {
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_strip_find_first_visible_item_view(EGUI_VIEW_OF(&test_strip), test_virtual_strip_match_item, &match_ctx, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.index, actual_entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);
}

static void test_virtual_strip_ensure_visible_helper(void)
{
    setup_strip_with_data_source();

    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_strip_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_strip), 999999, 6));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_strip_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_strip), 8104, 6));
    EGUI_TEST_ASSERT_EQUAL_INT(56, egui_view_virtual_strip_get_scroll_x(EGUI_VIEW_OF(&test_strip)));
    EGUI_VIEW_OF(&test_strip)->api->calculate_layout(EGUI_VIEW_OF(&test_strip));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_strip_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_strip), 8104, 6));
    EGUI_TEST_ASSERT_EQUAL_INT(56, egui_view_virtual_strip_get_scroll_x(EGUI_VIEW_OF(&test_strip)));
}

static void test_virtual_page_data_source_defaults_bridge_viewport_adapter(void)
{
    setup_page_with_data_source(40);

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_page_get_data_source(EGUI_VIEW_OF(&test_page)) == &test_page_data_source);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_page_get_data_source_context(EGUI_VIEW_OF(&test_page)) == &test_context);
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_virtual_page_get_state_cache_entry_limit(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_page_get_state_cache_byte_limit(EGUI_VIEW_OF(&test_page)));

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
    egui_view_virtual_page_entry_t entry;
    const egui_view_virtual_page_slot_t *slot;
    int slot_index;

    setup_page_with_data_source_config(40, &test_page_measured_data_source);

    EGUI_TEST_ASSERT_EQUAL_INT(40, egui_view_virtual_page_get_section_count(EGUI_VIEW_OF(&test_page)));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_page_find_section_index_by_stable_id(EGUI_VIEW_OF(&test_page), 1020));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_page_find_section_index_by_stable_id(EGUI_VIEW_OF(&test_page), 999999));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_page_resolve_section_by_stable_id(EGUI_VIEW_OF(&test_page), 1020, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(20, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1020, entry.stable_id);
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_page_resolve_section_by_stable_id(EGUI_VIEW_OF(&test_page), 999999, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, entry.stable_id);
    EGUI_TEST_ASSERT_EQUAL_INT(400, egui_view_virtual_page_get_section_y_by_stable_id(EGUI_VIEW_OF(&test_page), 1020));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_virtual_page_get_section_height_by_stable_id(EGUI_VIEW_OF(&test_page), 1020));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_page_get_section_y_by_stable_id(EGUI_VIEW_OF(&test_page), 999999));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_page_get_section_height_by_stable_id(EGUI_VIEW_OF(&test_page), 999999));

    slot_index = find_page_slot_index_by_stable_id(1000);
    EGUI_TEST_ASSERT_TRUE(slot_index >= 0);
    EGUI_TEST_ASSERT_EQUAL_INT(slot_index, egui_view_virtual_page_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_page), 1000));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_page_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_page), 999999));
    slot = egui_view_virtual_page_get_slot(EGUI_VIEW_OF(&test_page), (uint8_t)slot_index);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(slot == egui_view_virtual_page_find_slot_by_stable_id(EGUI_VIEW_OF(&test_page), 1000));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_page_find_slot_by_stable_id(EGUI_VIEW_OF(&test_page), 999999));
    EGUI_TEST_ASSERT_TRUE(slot->view == egui_view_virtual_page_find_view_by_stable_id(EGUI_VIEW_OF(&test_page), 1000));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_page_find_view_by_stable_id(EGUI_VIEW_OF(&test_page), 999999));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_page_get_slot_entry(EGUI_VIEW_OF(&test_page), (uint8_t)slot_index, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(0, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1000, entry.stable_id);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_page_resolve_section_by_view(EGUI_VIEW_OF(&test_page), slot->view, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(0, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1000, entry.stable_id);

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

static void test_virtual_page_visit_visible_sections_helper(void)
{
    test_visible_visit_context_t ctx;
    uint8_t manual_count;

    setup_page_with_data_source(40);
    egui_view_virtual_page_set_scroll_y(EGUI_VIEW_OF(&test_page), 55);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    manual_count = count_visible_page_sections_manual();
    EGUI_TEST_ASSERT_TRUE(manual_count >= 2);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_page_visit_visible_sections(EGUI_VIEW_OF(&test_page), NULL, NULL));

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_page);
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count,
                               egui_view_virtual_page_visit_visible_sections(EGUI_VIEW_OF(&test_page), test_virtual_page_visit_visible_section, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count, ctx.visited);
    test_visible_visit_assert_ok(&ctx);

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_page);
    ctx.stop_after = 2;
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_page_visit_visible_sections(EGUI_VIEW_OF(&test_page), test_virtual_page_visit_visible_section, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(2, ctx.visited);
    test_visible_visit_assert_ok(&ctx);
}

static void test_virtual_page_find_first_visible_section_view_helper(void)
{
    egui_view_virtual_page_entry_t expected_entry = {EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID};
    egui_view_virtual_page_entry_t actual_entry;
    egui_view_t *expected_view = NULL;
    egui_view_t *actual_view;
    test_min_index_match_context_t match_ctx = {.min_index = 3};
    uint8_t slot_count;
    uint8_t slot_index;

    setup_page_with_data_source(40);
    egui_view_virtual_page_set_scroll_y(EGUI_VIEW_OF(&test_page), 55);
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    slot_count = egui_view_virtual_page_get_slot_count(EGUI_VIEW_OF(&test_page));
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_page_entry_t entry;
        const egui_view_virtual_page_slot_t *slot = egui_view_virtual_page_get_slot(EGUI_VIEW_OF(&test_page), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_page_get_slot_entry(EGUI_VIEW_OF(&test_page), slot_index, &entry))
        {
            continue;
        }
        if (expected_view == NULL || entry.index < expected_entry.index)
        {
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_page_find_first_visible_section_view(EGUI_VIEW_OF(&test_page), NULL, NULL, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.index, actual_entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);

    expected_view = NULL;
    expected_entry.index = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    expected_entry.stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_page_entry_t entry;
        const egui_view_virtual_page_slot_t *slot = egui_view_virtual_page_get_slot(EGUI_VIEW_OF(&test_page), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_page_get_slot_entry(EGUI_VIEW_OF(&test_page), slot_index, &entry) || entry.index < match_ctx.min_index)
        {
            continue;
        }
        if (expected_view == NULL || entry.index < expected_entry.index)
        {
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_page_find_first_visible_section_view(EGUI_VIEW_OF(&test_page), test_virtual_page_match_section, &match_ctx, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.index, actual_entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);
}

static void test_virtual_page_ensure_visible_helper(void)
{
    setup_page_with_data_source_config(40, &test_page_measured_data_source);

    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&test_page), 999999, 7));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&test_page), 1020, 7));
    EGUI_TEST_ASSERT_EQUAL_INT(407, egui_view_virtual_page_get_scroll_y(EGUI_VIEW_OF(&test_page)));
    EGUI_VIEW_OF(&test_page)->api->calculate_layout(EGUI_VIEW_OF(&test_page));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&test_page), 1020, 7));
    EGUI_TEST_ASSERT_EQUAL_INT(407, egui_view_virtual_page_get_scroll_y(EGUI_VIEW_OF(&test_page)));
}

static void test_virtual_section_list_bridge_and_lookup_helpers(void)
{
    egui_view_virtual_section_list_entry_t entry;
    const egui_view_virtual_section_list_slot_t *slot;
    uint32_t section_index = 0;
    uint32_t item_index = 0;
    int slot_index;

    setup_section_list_with_data_source();

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_get_data_source(EGUI_VIEW_OF(&test_section_list)) == &test_section_list_data_source);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_get_data_source_context(EGUI_VIEW_OF(&test_section_list)) == &test_section_list_context);
    EGUI_TEST_ASSERT_EQUAL_INT(6, egui_view_virtual_section_list_get_state_cache_entry_limit(EGUI_VIEW_OF(&test_section_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(24, egui_view_virtual_section_list_get_state_cache_byte_limit(EGUI_VIEW_OF(&test_section_list)));
    EGUI_TEST_ASSERT_TRUE(test_section_list_context.created_count <= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS);
    EGUI_TEST_ASSERT_TRUE(test_section_list_context.bind_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_virtual_section_list_get_section_count(EGUI_VIEW_OF(&test_section_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_virtual_section_list_get_item_count(EGUI_VIEW_OF(&test_section_list), 1));

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_section_list_find_section_index_by_stable_id(EGUI_VIEW_OF(&test_section_list), 2200));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_find_item_position_by_stable_id(EGUI_VIEW_OF(&test_section_list), 3201, &section_index, &item_index));
    EGUI_TEST_ASSERT_EQUAL_INT(1, section_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, item_index);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_resolve_entry_by_stable_id(EGUI_VIEW_OF(&test_section_list), 2200, &entry));
    EGUI_TEST_ASSERT_TRUE(entry.is_section_header);
    EGUI_TEST_ASSERT_EQUAL_INT(1, entry.section_index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX, entry.item_index);
    EGUI_TEST_ASSERT_EQUAL_INT(2200, entry.stable_id);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_resolve_entry_by_stable_id(EGUI_VIEW_OF(&test_section_list), 3201, &entry));
    EGUI_TEST_ASSERT_FALSE(entry.is_section_header);
    EGUI_TEST_ASSERT_EQUAL_INT(1, entry.section_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, entry.item_index);
    EGUI_TEST_ASSERT_EQUAL_INT(3201, entry.stable_id);
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_section_list_resolve_entry_by_stable_id(EGUI_VIEW_OF(&test_section_list), 999999, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, entry.stable_id);

    EGUI_TEST_ASSERT_EQUAL_INT(44, egui_view_virtual_section_list_get_section_header_y(EGUI_VIEW_OF(&test_section_list), 1));
    EGUI_TEST_ASSERT_EQUAL_INT(22, egui_view_virtual_section_list_get_section_header_height(EGUI_VIEW_OF(&test_section_list), 1));
    EGUI_TEST_ASSERT_EQUAL_INT(82, egui_view_virtual_section_list_get_item_y(EGUI_VIEW_OF(&test_section_list), 1, 1));
    EGUI_TEST_ASSERT_EQUAL_INT(18, egui_view_virtual_section_list_get_item_height(EGUI_VIEW_OF(&test_section_list), 1, 1));
    EGUI_TEST_ASSERT_EQUAL_INT(44, egui_view_virtual_section_list_get_entry_y_by_stable_id(EGUI_VIEW_OF(&test_section_list), 2200));
    EGUI_TEST_ASSERT_EQUAL_INT(22, egui_view_virtual_section_list_get_entry_height_by_stable_id(EGUI_VIEW_OF(&test_section_list), 2200));
    EGUI_TEST_ASSERT_EQUAL_INT(82, egui_view_virtual_section_list_get_entry_y_by_stable_id(EGUI_VIEW_OF(&test_section_list), 3201));
    EGUI_TEST_ASSERT_EQUAL_INT(18, egui_view_virtual_section_list_get_entry_height_by_stable_id(EGUI_VIEW_OF(&test_section_list), 3201));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_section_list_get_entry_y_by_stable_id(EGUI_VIEW_OF(&test_section_list), 999999));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_section_list_get_entry_height_by_stable_id(EGUI_VIEW_OF(&test_section_list), 999999));

    slot_index = find_section_list_slot_index_by_stable_id(2200);
    EGUI_TEST_ASSERT_TRUE(slot_index >= 0);
    EGUI_TEST_ASSERT_EQUAL_INT(slot_index, egui_view_virtual_section_list_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_section_list), 2200));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_section_list_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_section_list), 999999));
    slot = egui_view_virtual_section_list_get_slot(EGUI_VIEW_OF(&test_section_list), (uint8_t)slot_index);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(slot == egui_view_virtual_section_list_find_slot_by_stable_id(EGUI_VIEW_OF(&test_section_list), 2200));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_section_list_find_slot_by_stable_id(EGUI_VIEW_OF(&test_section_list), 999999));
    EGUI_TEST_ASSERT_TRUE(slot->view == egui_view_virtual_section_list_find_view_by_stable_id(EGUI_VIEW_OF(&test_section_list), 2200));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_section_list_find_view_by_stable_id(EGUI_VIEW_OF(&test_section_list), 999999));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_get_slot_entry(EGUI_VIEW_OF(&test_section_list), (uint8_t)slot_index, &entry));
    EGUI_TEST_ASSERT_TRUE(entry.is_section_header);
    EGUI_TEST_ASSERT_EQUAL_INT(1, entry.section_index);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_resolve_entry_by_view(EGUI_VIEW_OF(&test_section_list), slot->view, &entry));
    EGUI_TEST_ASSERT_TRUE(entry.is_section_header);
    EGUI_TEST_ASSERT_EQUAL_INT(2200, entry.stable_id);

    egui_view_virtual_section_list_scroll_to_item_by_stable_id(EGUI_VIEW_OF(&test_section_list), 3201, 6);
    EGUI_VIEW_OF(&test_section_list)->api->calculate_layout(EGUI_VIEW_OF(&test_section_list));
    EGUI_TEST_ASSERT_EQUAL_INT(88, egui_view_virtual_section_list_get_scroll_y(EGUI_VIEW_OF(&test_section_list)));
}

static void test_virtual_section_list_resize_helpers_bridge_viewport(void)
{
    setup_section_list_with_data_source();

    egui_view_virtual_section_list_set_scroll_y(EGUI_VIEW_OF(&test_section_list), 96);
    EGUI_VIEW_OF(&test_section_list)->api->calculate_layout(EGUI_VIEW_OF(&test_section_list));
    EGUI_TEST_ASSERT_EQUAL_INT(100, egui_view_virtual_section_list_get_item_y(EGUI_VIEW_OF(&test_section_list), 1, 2));

    test_section_list_context.item_heights[0][0] = 20;
    egui_view_virtual_section_list_notify_item_resized(EGUI_VIEW_OF(&test_section_list), 0, 0);
    EGUI_VIEW_OF(&test_section_list)->api->calculate_layout(EGUI_VIEW_OF(&test_section_list));

    EGUI_TEST_ASSERT_EQUAL_INT(108, egui_view_virtual_section_list_get_item_y(EGUI_VIEW_OF(&test_section_list), 1, 2));
    EGUI_TEST_ASSERT_EQUAL_INT(104, egui_view_virtual_section_list_get_scroll_y(EGUI_VIEW_OF(&test_section_list)));
}

static void test_virtual_section_list_state_cache_helpers_bridge_viewport(void)
{
    const egui_view_virtual_section_list_slot_t *slot;
    uint8_t value = 0;

    setup_section_list_with_data_source();
    egui_view_virtual_section_list_set_state_cache_limits(EGUI_VIEW_OF(&test_section_list), 2, 4);

    value = 19;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_write_entry_state(EGUI_VIEW_OF(&test_section_list), 2200, &value, sizeof(value)));
    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_section_list_read_entry_state(EGUI_VIEW_OF(&test_section_list), 2200, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(19, value);

    value = 27;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_write_entry_state(EGUI_VIEW_OF(&test_section_list), 3201, &value, sizeof(value)));
    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_section_list_read_entry_state(EGUI_VIEW_OF(&test_section_list), 3201, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(27, value);

    slot = egui_view_virtual_section_list_get_slot(EGUI_VIEW_OF(&test_section_list), 0);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    value = 33;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_write_entry_state_for_view(slot->view, slot->stable_id, &value, sizeof(value)));
    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_section_list_read_entry_state_for_view(slot->view, slot->stable_id, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(33, value);

    egui_view_virtual_section_list_remove_entry_state_by_stable_id(EGUI_VIEW_OF(&test_section_list), 3201);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_section_list_read_entry_state(EGUI_VIEW_OF(&test_section_list), 3201, &value, sizeof(value)));
}

static void test_virtual_section_list_visit_visible_entries_helper(void)
{
    test_visible_visit_context_t ctx;
    uint8_t manual_count;

    setup_section_list_with_data_source();
    egui_view_virtual_section_list_set_scroll_y(EGUI_VIEW_OF(&test_section_list), 44);
    EGUI_VIEW_OF(&test_section_list)->api->calculate_layout(EGUI_VIEW_OF(&test_section_list));

    manual_count = count_visible_section_list_entries_manual();
    EGUI_TEST_ASSERT_TRUE(manual_count >= 2);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_section_list_visit_visible_entries(EGUI_VIEW_OF(&test_section_list), NULL, NULL));

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_section_list);
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count, egui_view_virtual_section_list_visit_visible_entries(EGUI_VIEW_OF(&test_section_list),
                                                                                                  test_virtual_section_list_visit_visible_entry, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count, ctx.visited);
    test_visible_visit_assert_ok(&ctx);

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_section_list);
    ctx.stop_after = 2;
    EGUI_TEST_ASSERT_EQUAL_INT(
            2, egui_view_virtual_section_list_visit_visible_entries(EGUI_VIEW_OF(&test_section_list), test_virtual_section_list_visit_visible_entry, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(2, ctx.visited);
    test_visible_visit_assert_ok(&ctx);
}

static void test_virtual_section_list_find_first_visible_entry_view_helper(void)
{
    egui_view_virtual_section_list_entry_t expected_entry = {0, EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX, EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX,
                                                             EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID};
    egui_view_virtual_section_list_entry_t actual_entry;
    egui_view_t *expected_view = NULL;
    egui_view_t *actual_view;
    test_section_entry_match_context_t match_ctx = {.min_section_index = 1, .want_header = 1};
    uint32_t best_flat_index = EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX;
    uint8_t slot_count;
    uint8_t slot_index;

    setup_section_list_with_data_source();
    egui_view_virtual_section_list_set_scroll_y(EGUI_VIEW_OF(&test_section_list), 44);
    EGUI_VIEW_OF(&test_section_list)->api->calculate_layout(EGUI_VIEW_OF(&test_section_list));

    slot_count = egui_view_virtual_section_list_get_slot_count(EGUI_VIEW_OF(&test_section_list));
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_section_list_entry_t entry;
        const egui_view_virtual_section_list_slot_t *slot = egui_view_virtual_section_list_get_slot(EGUI_VIEW_OF(&test_section_list), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_section_list_get_slot_entry(EGUI_VIEW_OF(&test_section_list), slot_index, &entry))
        {
            continue;
        }
        if (expected_view == NULL || slot->index < best_flat_index)
        {
            best_flat_index = slot->index;
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_section_list_find_first_visible_entry_view(EGUI_VIEW_OF(&test_section_list), NULL, NULL, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.section_index, actual_entry.section_index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.item_index, actual_entry.item_index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.is_section_header, actual_entry.is_section_header);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);

    expected_view = NULL;
    best_flat_index = EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX;
    expected_entry.stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_section_list_entry_t entry;
        const egui_view_virtual_section_list_slot_t *slot = egui_view_virtual_section_list_get_slot(EGUI_VIEW_OF(&test_section_list), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_section_list_get_slot_entry(EGUI_VIEW_OF(&test_section_list), slot_index, &entry) ||
            entry.is_section_header != match_ctx.want_header || entry.section_index < match_ctx.min_section_index)
        {
            continue;
        }
        if (expected_view == NULL || slot->index < best_flat_index)
        {
            best_flat_index = slot->index;
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_section_list_find_first_visible_entry_view(EGUI_VIEW_OF(&test_section_list), test_virtual_section_list_match_entry,
                                                                               &match_ctx, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.section_index, actual_entry.section_index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.item_index, actual_entry.item_index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.is_section_header, actual_entry.is_section_header);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);
}

static void test_virtual_section_list_ensure_visible_helper(void)
{
    setup_section_list_with_data_source();

    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(EGUI_VIEW_OF(&test_section_list), 999999, 6));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(EGUI_VIEW_OF(&test_section_list), 3201, 6));
    EGUI_TEST_ASSERT_EQUAL_INT(88, egui_view_virtual_section_list_get_scroll_y(EGUI_VIEW_OF(&test_section_list)));
    EGUI_VIEW_OF(&test_section_list)->api->calculate_layout(EGUI_VIEW_OF(&test_section_list));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(EGUI_VIEW_OF(&test_section_list), 3201, 6));
    EGUI_TEST_ASSERT_EQUAL_INT(88, egui_view_virtual_section_list_get_scroll_y(EGUI_VIEW_OF(&test_section_list)));
}

static void test_virtual_grid_bridge_and_lookup_helpers(void)
{
    egui_view_virtual_grid_entry_t entry;
    const egui_view_virtual_grid_slot_t *slot;
    egui_view_t *item_view;
    int slot_index;

    setup_grid_with_data_source();

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_grid_get_data_source(EGUI_VIEW_OF(&test_grid)) == &test_grid_data_source);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_grid_get_data_source_context(EGUI_VIEW_OF(&test_grid)) == &test_grid_context);
    EGUI_TEST_ASSERT_EQUAL_INT(7, egui_view_virtual_grid_get_state_cache_entry_limit(EGUI_VIEW_OF(&test_grid)));
    EGUI_TEST_ASSERT_EQUAL_INT(28, egui_view_virtual_grid_get_state_cache_byte_limit(EGUI_VIEW_OF(&test_grid)));
    EGUI_TEST_ASSERT_TRUE(test_grid_context.created_count > 0);
    EGUI_TEST_ASSERT_TRUE(test_grid_context.bind_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(8, egui_view_virtual_grid_get_item_count(EGUI_VIEW_OF(&test_grid)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_virtual_grid_get_row_count(EGUI_VIEW_OF(&test_grid)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_virtual_grid_find_index_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_grid_find_index_by_stable_id(EGUI_VIEW_OF(&test_grid), 999999));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_grid_resolve_item_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(4, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, entry.row_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, entry.column_index);
    EGUI_TEST_ASSERT_EQUAL_INT(7104, entry.stable_id);
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_grid_resolve_item_by_stable_id(EGUI_VIEW_OF(&test_grid), 999999, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, entry.stable_id);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_grid_get_item_x(EGUI_VIEW_OF(&test_grid), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(34, egui_view_virtual_grid_get_item_x(EGUI_VIEW_OF(&test_grid), 1));
    EGUI_TEST_ASSERT_EQUAL_INT(68, egui_view_virtual_grid_get_item_x(EGUI_VIEW_OF(&test_grid), 2));
    EGUI_TEST_ASSERT_EQUAL_INT(25, egui_view_virtual_grid_get_item_y(EGUI_VIEW_OF(&test_grid), 4));
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_virtual_grid_get_item_width(EGUI_VIEW_OF(&test_grid), 1));
    EGUI_TEST_ASSERT_EQUAL_INT(32, egui_view_virtual_grid_get_item_width(EGUI_VIEW_OF(&test_grid), 2));
    EGUI_TEST_ASSERT_EQUAL_INT(22, egui_view_virtual_grid_get_item_height(EGUI_VIEW_OF(&test_grid), 4));
    EGUI_TEST_ASSERT_EQUAL_INT(34, egui_view_virtual_grid_get_item_x_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104));
    EGUI_TEST_ASSERT_EQUAL_INT(25, egui_view_virtual_grid_get_item_y_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104));
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_virtual_grid_get_item_width_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104));
    EGUI_TEST_ASSERT_EQUAL_INT(22, egui_view_virtual_grid_get_item_height_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_grid_get_item_x_by_stable_id(EGUI_VIEW_OF(&test_grid), 999999));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_grid_get_item_y_by_stable_id(EGUI_VIEW_OF(&test_grid), 999999));

    item_view = egui_view_virtual_grid_find_view_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104);
    EGUI_TEST_ASSERT_NOT_NULL(item_view);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_grid_resolve_item_by_view(EGUI_VIEW_OF(&test_grid), item_view, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(4, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(7104, entry.stable_id);
    slot_index = find_grid_slot_index_by_row_index(1);
    EGUI_TEST_ASSERT_TRUE(slot_index >= 0);
    EGUI_TEST_ASSERT_EQUAL_INT(slot_index, egui_view_virtual_grid_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_grid_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_grid), 999999));
    slot = egui_view_virtual_grid_get_slot(EGUI_VIEW_OF(&test_grid), (uint8_t)slot_index);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(slot == egui_view_virtual_grid_find_slot_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_grid_find_slot_by_stable_id(EGUI_VIEW_OF(&test_grid), 999999));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_virtual_grid_get_slot_item_count(EGUI_VIEW_OF(&test_grid), (uint8_t)slot_index));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_grid_get_slot_entry(EGUI_VIEW_OF(&test_grid), (uint8_t)slot_index, 1, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(4, entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, entry.row_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, entry.column_index);
    EGUI_TEST_ASSERT_EQUAL_INT(7104, entry.stable_id);
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_grid_get_slot_entry(EGUI_VIEW_OF(&test_grid), (uint8_t)slot_index, 3, &entry));
    EGUI_TEST_ASSERT_TRUE(item_view == egui_view_virtual_grid_get_slot_item_view(EGUI_VIEW_OF(&test_grid), (uint8_t)slot_index, 1));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_grid_get_slot_item_view(EGUI_VIEW_OF(&test_grid), (uint8_t)slot_index, 3));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_grid_find_view_by_stable_id(EGUI_VIEW_OF(&test_grid), 999999));

    egui_view_virtual_grid_scroll_to_item_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104, 4);
    EGUI_VIEW_OF(&test_grid)->api->calculate_layout(EGUI_VIEW_OF(&test_grid));
    EGUI_TEST_ASSERT_EQUAL_INT(29, egui_view_virtual_grid_get_scroll_y(EGUI_VIEW_OF(&test_grid)));
}

static void test_virtual_grid_resize_helpers_bridge_viewport(void)
{
    setup_grid_with_data_source();

    egui_view_virtual_grid_set_scroll_y(EGUI_VIEW_OF(&test_grid), 28);
    EGUI_VIEW_OF(&test_grid)->api->calculate_layout(EGUI_VIEW_OF(&test_grid));
    EGUI_TEST_ASSERT_EQUAL_INT(25, egui_view_virtual_grid_get_item_y(EGUI_VIEW_OF(&test_grid), 4));

    test_grid_context.item_heights[1] = 30;
    egui_view_virtual_grid_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&test_grid), 7101);
    EGUI_VIEW_OF(&test_grid)->api->calculate_layout(EGUI_VIEW_OF(&test_grid));

    EGUI_TEST_ASSERT_EQUAL_INT(35, egui_view_virtual_grid_get_item_y(EGUI_VIEW_OF(&test_grid), 4));
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_virtual_grid_get_item_height_by_stable_id(EGUI_VIEW_OF(&test_grid), 7101));
    EGUI_TEST_ASSERT_EQUAL_INT(38, egui_view_virtual_grid_get_scroll_y(EGUI_VIEW_OF(&test_grid)));
}

static void test_virtual_grid_visit_visible_items_helper(void)
{
    test_visible_visit_context_t ctx;
    uint8_t manual_count;

    setup_grid_with_data_source();
    egui_view_virtual_grid_set_scroll_y(EGUI_VIEW_OF(&test_grid), 18);
    EGUI_VIEW_OF(&test_grid)->api->calculate_layout(EGUI_VIEW_OF(&test_grid));

    manual_count = count_visible_grid_items_manual();
    EGUI_TEST_ASSERT_TRUE(manual_count >= 2);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_grid_visit_visible_items(EGUI_VIEW_OF(&test_grid), NULL, NULL));

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_grid);
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count, egui_view_virtual_grid_visit_visible_items(EGUI_VIEW_OF(&test_grid), test_virtual_grid_visit_visible_item, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count, ctx.visited);
    test_visible_visit_assert_ok(&ctx);

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_grid);
    ctx.stop_after = 2;
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_grid_visit_visible_items(EGUI_VIEW_OF(&test_grid), test_virtual_grid_visit_visible_item, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(2, ctx.visited);
    test_visible_visit_assert_ok(&ctx);
}

static void test_virtual_grid_find_first_visible_item_view_helper(void)
{
    egui_view_virtual_grid_entry_t expected_entry = {EGUI_VIEW_VIRTUAL_GRID_INVALID_INDEX, EGUI_VIEW_VIRTUAL_GRID_INVALID_INDEX, 0,
                                                     EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID};
    egui_view_virtual_grid_entry_t actual_entry;
    egui_view_t *expected_view = NULL;
    egui_view_t *actual_view;
    test_min_index_match_context_t match_ctx = {.min_index = 4};
    uint8_t slot_count;
    uint8_t slot_index;

    setup_grid_with_data_source();
    egui_view_virtual_grid_set_scroll_y(EGUI_VIEW_OF(&test_grid), 18);
    EGUI_VIEW_OF(&test_grid)->api->calculate_layout(EGUI_VIEW_OF(&test_grid));

    slot_count = egui_view_virtual_grid_get_slot_count(EGUI_VIEW_OF(&test_grid));
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        uint8_t item_count;
        uint8_t column_index;
        const egui_view_virtual_grid_slot_t *slot = egui_view_virtual_grid_get_slot(EGUI_VIEW_OF(&test_grid), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE)
        {
            continue;
        }

        item_count = egui_view_virtual_grid_get_slot_item_count(EGUI_VIEW_OF(&test_grid), slot_index);
        for (column_index = 0; column_index < item_count; column_index++)
        {
            egui_view_virtual_grid_entry_t entry;
            egui_view_t *item_view;

            if (!egui_view_virtual_grid_get_slot_entry(EGUI_VIEW_OF(&test_grid), slot_index, column_index, &entry))
            {
                continue;
            }
            item_view = egui_view_virtual_grid_get_slot_item_view(EGUI_VIEW_OF(&test_grid), slot_index, column_index);
            if (item_view == NULL)
            {
                continue;
            }
            if (expected_view == NULL || entry.index < expected_entry.index)
            {
                expected_entry = entry;
                expected_view = item_view;
            }
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_grid_find_first_visible_item_view(EGUI_VIEW_OF(&test_grid), NULL, NULL, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.index, actual_entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.row_index, actual_entry.row_index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.column_index, actual_entry.column_index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);

    expected_view = NULL;
    expected_entry.index = EGUI_VIEW_VIRTUAL_GRID_INVALID_INDEX;
    expected_entry.stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        uint8_t item_count;
        uint8_t column_index;
        const egui_view_virtual_grid_slot_t *slot = egui_view_virtual_grid_get_slot(EGUI_VIEW_OF(&test_grid), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE)
        {
            continue;
        }

        item_count = egui_view_virtual_grid_get_slot_item_count(EGUI_VIEW_OF(&test_grid), slot_index);
        for (column_index = 0; column_index < item_count; column_index++)
        {
            egui_view_virtual_grid_entry_t entry;
            egui_view_t *item_view;

            if (!egui_view_virtual_grid_get_slot_entry(EGUI_VIEW_OF(&test_grid), slot_index, column_index, &entry) || entry.index < match_ctx.min_index)
            {
                continue;
            }
            item_view = egui_view_virtual_grid_get_slot_item_view(EGUI_VIEW_OF(&test_grid), slot_index, column_index);
            if (item_view == NULL)
            {
                continue;
            }
            if (expected_view == NULL || entry.index < expected_entry.index)
            {
                expected_entry = entry;
                expected_view = item_view;
            }
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_grid_find_first_visible_item_view(EGUI_VIEW_OF(&test_grid), test_virtual_grid_match_item, &match_ctx, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.index, actual_entry.index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.row_index, actual_entry.row_index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.column_index, actual_entry.column_index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);
}

static void test_virtual_grid_ensure_visible_helper(void)
{
    setup_grid_with_data_source();

    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_grid_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_grid), 999999, 4));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_grid_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104, 4));
    EGUI_TEST_ASSERT_EQUAL_INT(29, egui_view_virtual_grid_get_scroll_y(EGUI_VIEW_OF(&test_grid)));
    EGUI_VIEW_OF(&test_grid)->api->calculate_layout(EGUI_VIEW_OF(&test_grid));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_grid_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&test_grid), 7104, 4));
    EGUI_TEST_ASSERT_EQUAL_INT(29, egui_view_virtual_grid_get_scroll_y(EGUI_VIEW_OF(&test_grid)));
}

static void test_virtual_tree_bridge_and_lookup_helpers(void)
{
    egui_view_virtual_tree_entry_t entry;
    const egui_view_virtual_tree_slot_t *slot;
    int slot_index;

    setup_tree_with_data_source();

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_tree_get_data_source(EGUI_VIEW_OF(&test_tree)) == &test_tree_data_source);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_tree_get_data_source_context(EGUI_VIEW_OF(&test_tree)) == &test_tree_context);
    EGUI_TEST_ASSERT_EQUAL_INT(8, egui_view_virtual_tree_get_state_cache_entry_limit(EGUI_VIEW_OF(&test_tree)));
    EGUI_TEST_ASSERT_EQUAL_INT(32, egui_view_virtual_tree_get_state_cache_byte_limit(EGUI_VIEW_OF(&test_tree)));
    EGUI_TEST_ASSERT_TRUE(test_tree_context.created_count <= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS);
    EGUI_TEST_ASSERT_TRUE(test_tree_context.bind_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_tree_get_root_count(EGUI_VIEW_OF(&test_tree)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, egui_view_virtual_tree_get_visible_node_count(EGUI_VIEW_OF(&test_tree)));

    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_virtual_tree_find_visible_index_by_stable_id(EGUI_VIEW_OF(&test_tree), 5220));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_tree_find_visible_index_by_stable_id(EGUI_VIEW_OF(&test_tree), 5310));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_tree_resolve_node_by_stable_id(EGUI_VIEW_OF(&test_tree), 5220, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(3, entry.visible_index);
    EGUI_TEST_ASSERT_EQUAL_INT(5200, entry.parent_stable_id);
    EGUI_TEST_ASSERT_EQUAL_INT(2, entry.depth);
    EGUI_TEST_ASSERT_FALSE(entry.has_children);
    EGUI_TEST_ASSERT_FALSE(entry.is_expanded);

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_tree_resolve_node_by_stable_id(EGUI_VIEW_OF(&test_tree), 5310, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX, entry.visible_index);
    EGUI_TEST_ASSERT_EQUAL_INT(5300, entry.parent_stable_id);
    EGUI_TEST_ASSERT_EQUAL_INT(2, entry.depth);
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_tree_resolve_node_by_stable_id(EGUI_VIEW_OF(&test_tree), 999999, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, entry.stable_id);

    EGUI_TEST_ASSERT_EQUAL_INT(56, egui_view_virtual_tree_get_node_y_by_stable_id(EGUI_VIEW_OF(&test_tree), 5220));
    EGUI_TEST_ASSERT_EQUAL_INT(14, egui_view_virtual_tree_get_node_height_by_stable_id(EGUI_VIEW_OF(&test_tree), 5220));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_tree_get_node_y_by_stable_id(EGUI_VIEW_OF(&test_tree), 5310));

    slot_index = find_tree_slot_index_by_stable_id(5100);
    EGUI_TEST_ASSERT_TRUE(slot_index >= 0);
    EGUI_TEST_ASSERT_EQUAL_INT(slot_index, egui_view_virtual_tree_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_tree), 5100));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_tree_find_slot_index_by_stable_id(EGUI_VIEW_OF(&test_tree), 999999));
    slot = egui_view_virtual_tree_get_slot(EGUI_VIEW_OF(&test_tree), (uint8_t)slot_index);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    EGUI_TEST_ASSERT_TRUE(slot == egui_view_virtual_tree_find_slot_by_stable_id(EGUI_VIEW_OF(&test_tree), 5100));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_tree_find_slot_by_stable_id(EGUI_VIEW_OF(&test_tree), 999999));
    EGUI_TEST_ASSERT_TRUE(slot->view == egui_view_virtual_tree_find_view_by_stable_id(EGUI_VIEW_OF(&test_tree), 5100));
    EGUI_TEST_ASSERT_NULL(egui_view_virtual_tree_find_view_by_stable_id(EGUI_VIEW_OF(&test_tree), 999999));
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_tree_get_slot_entry(EGUI_VIEW_OF(&test_tree), (uint8_t)slot_index, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(0, entry.visible_index);
    EGUI_TEST_ASSERT_EQUAL_INT(5100, entry.stable_id);
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_tree_resolve_node_by_view(EGUI_VIEW_OF(&test_tree), slot->view, &entry));
    EGUI_TEST_ASSERT_EQUAL_INT(0, entry.visible_index);
    EGUI_TEST_ASSERT_EQUAL_INT(5100, entry.stable_id);

    egui_view_virtual_tree_scroll_to_node_by_stable_id(EGUI_VIEW_OF(&test_tree), 5220, 6);
    EGUI_VIEW_OF(&test_tree)->api->calculate_layout(EGUI_VIEW_OF(&test_tree));
    EGUI_TEST_ASSERT_EQUAL_INT(48, egui_view_virtual_tree_get_scroll_y(EGUI_VIEW_OF(&test_tree)));
}

static void test_virtual_tree_data_changed_updates_visible_mapping(void)
{
    setup_tree_with_data_source();

    test_tree_context.nodes[1].expanded = 0;
    egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&test_tree));
    EGUI_VIEW_OF(&test_tree)->api->calculate_layout(EGUI_VIEW_OF(&test_tree));

    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_virtual_tree_get_visible_node_count(EGUI_VIEW_OF(&test_tree)));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_virtual_tree_find_visible_index_by_stable_id(EGUI_VIEW_OF(&test_tree), 5220));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_tree_find_visible_index_by_stable_id(EGUI_VIEW_OF(&test_tree), 5300));
    EGUI_TEST_ASSERT_EQUAL_INT(40, egui_view_virtual_tree_get_node_y_by_stable_id(EGUI_VIEW_OF(&test_tree), 5300));

    test_tree_context.nodes[6].expanded = 1;
    egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&test_tree));
    EGUI_VIEW_OF(&test_tree)->api->calculate_layout(EGUI_VIEW_OF(&test_tree));

    EGUI_TEST_ASSERT_EQUAL_INT(6, egui_view_virtual_tree_get_visible_node_count(EGUI_VIEW_OF(&test_tree)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_virtual_tree_find_visible_index_by_stable_id(EGUI_VIEW_OF(&test_tree), 5400));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_virtual_tree_find_visible_index_by_stable_id(EGUI_VIEW_OF(&test_tree), 5410));
}

static void test_virtual_tree_state_cache_helpers_bridge_viewport(void)
{
    const egui_view_virtual_tree_slot_t *slot;
    uint8_t value = 0;

    setup_tree_with_data_source();
    egui_view_virtual_tree_set_state_cache_limits(EGUI_VIEW_OF(&test_tree), 2, 4);

    value = 29;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_tree_write_node_state(EGUI_VIEW_OF(&test_tree), 5220, &value, sizeof(value)));
    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_tree_read_node_state(EGUI_VIEW_OF(&test_tree), 5220, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(29, value);

    slot = egui_view_virtual_tree_get_slot(EGUI_VIEW_OF(&test_tree), 0);
    EGUI_TEST_ASSERT_NOT_NULL(slot);
    value = 37;
    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_tree_write_node_state_for_view(slot->view, slot->stable_id, &value, sizeof(value)));
    value = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_virtual_tree_read_node_state_for_view(slot->view, slot->stable_id, &value, sizeof(value)));
    EGUI_TEST_ASSERT_EQUAL_INT(37, value);

    egui_view_virtual_tree_remove_node_state_by_stable_id(EGUI_VIEW_OF(&test_tree), 5220);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_tree_read_node_state(EGUI_VIEW_OF(&test_tree), 5220, &value, sizeof(value)));
}

static void test_virtual_tree_visit_visible_nodes_helper(void)
{
    test_visible_visit_context_t ctx;
    uint8_t manual_count;

    setup_tree_with_data_source();
    egui_view_virtual_tree_set_scroll_y(EGUI_VIEW_OF(&test_tree), 24);
    EGUI_VIEW_OF(&test_tree)->api->calculate_layout(EGUI_VIEW_OF(&test_tree));

    manual_count = count_visible_tree_nodes_manual();
    EGUI_TEST_ASSERT_TRUE(manual_count >= 2);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_virtual_tree_visit_visible_nodes(EGUI_VIEW_OF(&test_tree), NULL, NULL));

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_tree);
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count, egui_view_virtual_tree_visit_visible_nodes(EGUI_VIEW_OF(&test_tree), test_virtual_tree_visit_visible_node, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(manual_count, ctx.visited);
    test_visible_visit_assert_ok(&ctx);

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected_self = EGUI_VIEW_OF(&test_tree);
    ctx.stop_after = 2;
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_virtual_tree_visit_visible_nodes(EGUI_VIEW_OF(&test_tree), test_virtual_tree_visit_visible_node, &ctx));
    EGUI_TEST_ASSERT_EQUAL_INT(2, ctx.visited);
    test_visible_visit_assert_ok(&ctx);
}

static void test_virtual_tree_find_first_visible_node_view_helper(void)
{
    egui_view_virtual_tree_entry_t expected_entry = {
            EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, 0, 0, 0, 0};
    egui_view_virtual_tree_entry_t actual_entry;
    egui_view_t *expected_view = NULL;
    egui_view_t *actual_view;
    test_tree_node_match_context_t match_ctx = {.min_visible_index = 1, .want_branch = 1, .min_depth = 1};
    uint8_t slot_count;
    uint8_t slot_index;

    setup_tree_with_data_source();
    egui_view_virtual_tree_set_scroll_y(EGUI_VIEW_OF(&test_tree), 24);
    EGUI_VIEW_OF(&test_tree)->api->calculate_layout(EGUI_VIEW_OF(&test_tree));

    slot_count = egui_view_virtual_tree_get_slot_count(EGUI_VIEW_OF(&test_tree));
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_tree_entry_t entry;
        const egui_view_virtual_tree_slot_t *slot = egui_view_virtual_tree_get_slot(EGUI_VIEW_OF(&test_tree), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_tree_get_slot_entry(EGUI_VIEW_OF(&test_tree), slot_index, &entry))
        {
            continue;
        }
        if (expected_view == NULL || entry.visible_index < expected_entry.visible_index)
        {
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_tree_find_first_visible_node_view(EGUI_VIEW_OF(&test_tree), NULL, NULL, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.visible_index, actual_entry.visible_index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.depth, actual_entry.depth);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.has_children, actual_entry.has_children);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);

    expected_view = NULL;
    expected_entry.visible_index = EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX;
    expected_entry.stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_tree_entry_t entry;
        const egui_view_virtual_tree_slot_t *slot = egui_view_virtual_tree_get_slot(EGUI_VIEW_OF(&test_tree), slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL ||
            !egui_view_virtual_tree_get_slot_entry(EGUI_VIEW_OF(&test_tree), slot_index, &entry) || entry.visible_index < match_ctx.min_visible_index ||
            entry.depth < match_ctx.min_depth || entry.has_children != match_ctx.want_branch)
        {
            continue;
        }
        if (expected_view == NULL || entry.visible_index < expected_entry.visible_index)
        {
            expected_entry = entry;
            expected_view = slot->view;
        }
    }

    EGUI_TEST_ASSERT_NOT_NULL(expected_view);
    actual_view = egui_view_virtual_tree_find_first_visible_node_view(EGUI_VIEW_OF(&test_tree), test_virtual_tree_match_node, &match_ctx, &actual_entry);
    EGUI_TEST_ASSERT_TRUE(actual_view == expected_view);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.visible_index, actual_entry.visible_index);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.depth, actual_entry.depth);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.has_children, actual_entry.has_children);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_entry.stable_id, actual_entry.stable_id);
}

static void test_virtual_tree_ensure_visible_helper(void)
{
    setup_tree_with_data_source();

    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&test_tree), 999999, 6));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&test_tree), 5220, 6));
    EGUI_TEST_ASSERT_EQUAL_INT(48, egui_view_virtual_tree_get_scroll_y(EGUI_VIEW_OF(&test_tree)));
    EGUI_VIEW_OF(&test_tree)->api->calculate_layout(EGUI_VIEW_OF(&test_tree));

    EGUI_TEST_ASSERT_TRUE(egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&test_tree), 5220, 6));
    EGUI_TEST_ASSERT_EQUAL_INT(48, egui_view_virtual_tree_get_scroll_y(EGUI_VIEW_OF(&test_tree)));
    EGUI_TEST_ASSERT_FALSE(egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&test_tree), 5310, 6));
}

void test_virtual_viewport_run(void)
{
    EGUI_TEST_SUITE_BEGIN(virtual_viewport);
    EGUI_TEST_RUN(test_virtual_viewport_initial_window_uses_bounded_slots);
    EGUI_TEST_RUN(test_virtual_viewport_scroll_reuses_slot_pool);
    EGUI_TEST_RUN(test_virtual_viewport_slot_visibility_helpers);
    EGUI_TEST_RUN(test_virtual_viewport_keepalive_preserves_view_instance);
    EGUI_TEST_RUN(test_virtual_viewport_variable_height_resize_keeps_anchor);
    EGUI_TEST_RUN(test_virtual_viewport_insert_before_anchor_preserves_scroll_position);
    EGUI_TEST_RUN(test_virtual_viewport_keepalive_limit_trims_oldest_slot);
    EGUI_TEST_RUN(test_virtual_viewport_state_cache_write_read_and_trim);
    EGUI_TEST_RUN(test_virtual_viewport_state_cache_restores_recycled_slot);
    EGUI_TEST_RUN(test_virtual_viewport_duplicate_keepalive_slot_is_recycled);
    EGUI_TEST_RUN(test_virtual_viewport_setup_and_lookup_helpers);
    EGUI_TEST_RUN(test_virtual_viewport_visit_visible_items_helper);
    EGUI_TEST_RUN(test_virtual_viewport_find_first_visible_item_view_helper);
    EGUI_TEST_RUN(test_virtual_viewport_ensure_visible_helper);
    EGUI_TEST_RUN(test_virtual_list_data_source_defaults_bridge_viewport_adapter);
    EGUI_TEST_RUN(test_virtual_list_stable_id_helpers_fallback_lookup);
    EGUI_TEST_RUN(test_virtual_list_state_cache_helpers_bridge_viewport);
    EGUI_TEST_RUN(test_virtual_list_visit_visible_items_helper);
    EGUI_TEST_RUN(test_virtual_list_find_first_visible_item_view_helper);
    EGUI_TEST_RUN(test_virtual_list_ensure_visible_helper);
    EGUI_TEST_RUN(test_virtual_strip_bridge_and_lookup_helpers);
    EGUI_TEST_RUN(test_virtual_strip_resize_helpers_bridge_viewport);
    EGUI_TEST_RUN(test_virtual_strip_state_cache_helpers_bridge_viewport);
    EGUI_TEST_RUN(test_virtual_strip_visit_visible_items_helper);
    EGUI_TEST_RUN(test_virtual_strip_find_first_visible_item_view_helper);
    EGUI_TEST_RUN(test_virtual_strip_ensure_visible_helper);
    EGUI_TEST_RUN(test_virtual_page_data_source_defaults_bridge_viewport_adapter);
    EGUI_TEST_RUN(test_virtual_page_stable_id_helpers_fallback_lookup);
    EGUI_TEST_RUN(test_virtual_page_state_cache_helpers_bridge_viewport);
    EGUI_TEST_RUN(test_virtual_page_visit_visible_sections_helper);
    EGUI_TEST_RUN(test_virtual_page_find_first_visible_section_view_helper);
    EGUI_TEST_RUN(test_virtual_page_ensure_visible_helper);
    EGUI_TEST_RUN(test_virtual_section_list_bridge_and_lookup_helpers);
    EGUI_TEST_RUN(test_virtual_section_list_resize_helpers_bridge_viewport);
    EGUI_TEST_RUN(test_virtual_section_list_state_cache_helpers_bridge_viewport);
    EGUI_TEST_RUN(test_virtual_section_list_visit_visible_entries_helper);
    EGUI_TEST_RUN(test_virtual_section_list_find_first_visible_entry_view_helper);
    EGUI_TEST_RUN(test_virtual_section_list_ensure_visible_helper);
    EGUI_TEST_RUN(test_virtual_grid_bridge_and_lookup_helpers);
    EGUI_TEST_RUN(test_virtual_grid_resize_helpers_bridge_viewport);
    EGUI_TEST_RUN(test_virtual_grid_visit_visible_items_helper);
    EGUI_TEST_RUN(test_virtual_grid_find_first_visible_item_view_helper);
    EGUI_TEST_RUN(test_virtual_grid_ensure_visible_helper);
    EGUI_TEST_RUN(test_virtual_tree_bridge_and_lookup_helpers);
    EGUI_TEST_RUN(test_virtual_tree_data_changed_updates_visible_mapping);
    EGUI_TEST_RUN(test_virtual_tree_state_cache_helpers_bridge_viewport);
    EGUI_TEST_RUN(test_virtual_tree_visit_visible_nodes_helper);
    EGUI_TEST_RUN(test_virtual_tree_find_first_visible_node_view_helper);
    EGUI_TEST_RUN(test_virtual_tree_ensure_visible_helper);
    EGUI_TEST_SUITE_END();
}
