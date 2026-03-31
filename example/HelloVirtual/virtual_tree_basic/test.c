#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "uicode.h"

#define TREE_BASIC_ROOT_COUNT   4U
#define TREE_BASIC_GROUP_COUNT  3U
#define TREE_BASIC_TASK_COUNT   4U
#define TREE_BASIC_MAX_CHILDREN 4U
#define TREE_BASIC_NODE_COUNT                                                                                                                                  \
    (TREE_BASIC_ROOT_COUNT + TREE_BASIC_ROOT_COUNT * TREE_BASIC_GROUP_COUNT + TREE_BASIC_ROOT_COUNT * TREE_BASIC_GROUP_COUNT * TREE_BASIC_TASK_COUNT)
#define TREE_BASIC_STABLE_BASE        4000U
#define TREE_BASIC_INVALID_NODE_INDEX 0xFFFFU
#define TREE_BASIC_TITLE_TEXT_LEN     40
#define TREE_BASIC_BODY_TEXT_LEN      48
#define TREE_BASIC_META_TEXT_LEN      40
#define TREE_BASIC_BADGE_TEXT_LEN     16
#define TREE_BASIC_JUMP_STEP          5U

#define TREE_BASIC_MARGIN_X   8
#define TREE_BASIC_TOP_Y      8
#define TREE_BASIC_HEADER_W   (EGUI_CONFIG_SCEEN_WIDTH - TREE_BASIC_MARGIN_X * 2)
#define TREE_BASIC_HEADER_H   0
#define TREE_BASIC_TOOLBAR_Y  TREE_BASIC_TOP_Y
#define TREE_BASIC_TOOLBAR_H  34
#define TREE_BASIC_VIEW_Y     (TREE_BASIC_TOOLBAR_Y + TREE_BASIC_TOOLBAR_H + 6)
#define TREE_BASIC_VIEW_W     TREE_BASIC_HEADER_W
#define TREE_BASIC_VIEW_H     (EGUI_CONFIG_SCEEN_HEIGHT - TREE_BASIC_VIEW_Y - 8)
#define TREE_BASIC_ACTION_GAP 6
#define TREE_BASIC_ACTION_W   ((TREE_BASIC_HEADER_W - 20 - TREE_BASIC_ACTION_GAP * 2) / 3)

#define TREE_BASIC_ROOT_H                56
#define TREE_BASIC_GROUP_H               50
#define TREE_BASIC_TASK_H                56
#define TREE_BASIC_TASK_DETAIL_H         74
#define TREE_BASIC_JUMP_VERIFY_RETRY_MAX 3U

#define TREE_BASIC_FONT_TITLE ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define TREE_BASIC_FONT_BODY  ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define TREE_BASIC_FONT_HERO  ((const egui_font_t *)&egui_res_font_montserrat_12_4)

enum
{
    TREE_BASIC_NODE_ROOT = 1,
    TREE_BASIC_NODE_GROUP,
    TREE_BASIC_NODE_TASK,
};

enum
{
    TREE_BASIC_VIEW_BRANCH = 1,
    TREE_BASIC_VIEW_TASK = 2,
};

enum
{
    TREE_BASIC_ACTION_PATCH = 0,
    TREE_BASIC_ACTION_JUMP,
    TREE_BASIC_ACTION_RESET,
    TREE_BASIC_ACTION_COUNT,
};

typedef struct tree_basic_node tree_basic_node_t;
typedef struct tree_basic_branch_view tree_basic_branch_view_t;
typedef struct tree_basic_task_view tree_basic_task_view_t;
typedef struct tree_basic_context tree_basic_context_t;
#if EGUI_CONFIG_RECORDING_TEST
typedef struct tree_basic_visible_summary tree_basic_visible_summary_t;
#endif

struct tree_basic_node
{
    uint32_t stable_id;
    uint16_t parent_index;
    uint16_t child_indices[TREE_BASIC_MAX_CHILDREN];
    uint8_t child_count;
    uint8_t kind;
    uint8_t expanded;
    uint8_t detail;
    uint8_t progress;
    uint8_t revision;
    uint8_t root_ordinal;
    uint8_t group_ordinal;
    uint8_t task_ordinal;
};

struct tree_basic_branch_view
{
    egui_view_group_t root;
    egui_view_card_t guide;
    egui_view_card_t connector;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t meta;
    egui_view_label_t badge;
    char title_text[TREE_BASIC_TITLE_TEXT_LEN];
    char meta_text[TREE_BASIC_META_TEXT_LEN];
    char badge_text[TREE_BASIC_BADGE_TEXT_LEN];
};

struct tree_basic_task_view
{
    egui_view_group_t root;
    egui_view_card_t guide;
    egui_view_card_t connector;
    egui_view_card_t marker;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t body;
    egui_view_label_t meta;
    egui_view_label_t badge;
    egui_view_progress_bar_t progress;
    char title_text[TREE_BASIC_TITLE_TEXT_LEN];
    char body_text[TREE_BASIC_BODY_TEXT_LEN];
    char meta_text[TREE_BASIC_META_TEXT_LEN];
    char badge_text[TREE_BASIC_BADGE_TEXT_LEN];
};

struct tree_basic_context
{
    tree_basic_node_t nodes[TREE_BASIC_NODE_COUNT];
    uint16_t node_count;
    uint16_t root_indices[TREE_BASIC_ROOT_COUNT];
    uint32_t selected_id;
    uint32_t last_clicked_id;
    uint32_t last_clicked_visible_index;
    uint32_t click_count;
    uint32_t patch_count;
    uint16_t jump_cursor;
    uint32_t jump_target_id;
};

#if EGUI_CONFIG_RECORDING_TEST
struct tree_basic_visible_summary
{
    uint32_t first_visible_index;
    uint32_t first_visible_id;
    uint8_t visible_count;
    uint8_t has_first;
};
#endif

static const char *tree_basic_action_names[TREE_BASIC_ACTION_COUNT] = {"Patch", "Jump", "Reset"};
static const char *tree_basic_root_names[TREE_BASIC_ROOT_COUNT] = {"Fleet", "Inbox", "Audit", "Ops"};
static const char *tree_basic_group_names[TREE_BASIC_GROUP_COUNT] = {"Sync", "Queue", "Review"};
static const char *tree_basic_task_notes[TREE_BASIC_TASK_COUNT] = {"check guides", "confirm hit", "verify jump", "patch detail"};

static egui_view_t background_view;
static egui_view_card_t toolbar_card;
static egui_view_button_t action_buttons[TREE_BASIC_ACTION_COUNT];
static egui_view_virtual_tree_t tree_view;
static tree_basic_context_t tree_basic_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
static uint8_t recording_jump_verify_retry;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(tree_basic_toolbar_card_params, TREE_BASIC_MARGIN_X, TREE_BASIC_TOOLBAR_Y, TREE_BASIC_HEADER_W, TREE_BASIC_TOOLBAR_H, 12);

static const egui_view_virtual_tree_params_t tree_basic_view_params = {
        .region = {{TREE_BASIC_MARGIN_X, TREE_BASIC_VIEW_Y}, {TREE_BASIC_VIEW_W, TREE_BASIC_VIEW_H}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 2,
        .estimated_node_height = 54,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(tree_basic_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF5F8), EGUI_COLOR_HEX(0xDDE8EF),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(tree_basic_screen_bg_params, &tree_basic_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(tree_basic_screen_bg, &tree_basic_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(tree_basic_view_bg_param, EGUI_COLOR_HEX(0xF9FBFD), EGUI_ALPHA_100, 14, 1, EGUI_COLOR_HEX(0xC6D8E4),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(tree_basic_view_bg_params, &tree_basic_view_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_basic_view_bg, &tree_basic_view_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(tree_basic_action_patch_param, EGUI_COLOR_HEX(0xE6F6EF), EGUI_ALPHA_100, 10, 1,
                                                        EGUI_COLOR_HEX(0xA7D4BF), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(tree_basic_action_patch_params, &tree_basic_action_patch_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_basic_action_patch_bg, &tree_basic_action_patch_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(tree_basic_action_jump_param, EGUI_COLOR_HEX(0xE7F0FA), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xACC4DB),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(tree_basic_action_jump_params, &tree_basic_action_jump_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_basic_action_jump_bg, &tree_basic_action_jump_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(tree_basic_action_reset_param, EGUI_COLOR_HEX(0xF9E9E4), EGUI_ALPHA_100, 10, 1,
                                                        EGUI_COLOR_HEX(0xD4B2A9), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(tree_basic_action_reset_params, &tree_basic_action_reset_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_basic_action_reset_bg, &tree_basic_action_reset_params);

static uint32_t tree_basic_ds_get_root_count(void *context);
static uint32_t tree_basic_ds_get_root_stable_id(void *context, uint32_t root_index);
static uint32_t tree_basic_ds_get_child_count(void *context, uint32_t stable_id);
static uint32_t tree_basic_ds_get_child_stable_id(void *context, uint32_t stable_id, uint32_t child_index);
static uint8_t tree_basic_ds_is_node_expanded(void *context, uint32_t stable_id);
static uint16_t tree_basic_ds_get_node_view_type(void *context, const egui_view_virtual_tree_entry_t *entry);
static int32_t tree_basic_ds_measure_node_height(void *context, const egui_view_virtual_tree_entry_t *entry, int32_t width_hint);
static egui_view_t *tree_basic_ds_create_node_view(void *context, uint16_t view_type);
static void tree_basic_ds_destroy_node_view(void *context, egui_view_t *view, uint16_t view_type);
static void tree_basic_ds_bind_node_view(void *context, egui_view_t *view, const egui_view_virtual_tree_entry_t *entry);
static uint8_t tree_basic_ds_should_keep_alive(void *context, egui_view_t *view, uint32_t stable_id);

static const egui_view_virtual_tree_data_source_t tree_basic_data_source = {
        .get_root_count = tree_basic_ds_get_root_count,
        .get_root_stable_id = tree_basic_ds_get_root_stable_id,
        .get_child_count = tree_basic_ds_get_child_count,
        .get_child_stable_id = tree_basic_ds_get_child_stable_id,
        .is_node_expanded = tree_basic_ds_is_node_expanded,
        .get_node_view_type = tree_basic_ds_get_node_view_type,
        .measure_node_height = tree_basic_ds_measure_node_height,
        .create_node_view = tree_basic_ds_create_node_view,
        .destroy_node_view = tree_basic_ds_destroy_node_view,
        .bind_node_view = tree_basic_ds_bind_node_view,
        .unbind_node_view = NULL,
        .should_keep_alive = tree_basic_ds_should_keep_alive,
        .save_node_state = NULL,
        .restore_node_state = NULL,
        .default_view_type = TREE_BASIC_VIEW_BRANCH,
};

static tree_basic_node_t *tree_basic_get_node(uint16_t node_index)
{
    if (node_index == TREE_BASIC_INVALID_NODE_INDEX || node_index >= tree_basic_ctx.node_count)
    {
        return NULL;
    }

    return &tree_basic_ctx.nodes[node_index];
}

static int32_t tree_basic_find_node_index_by_stable_id(uint32_t stable_id)
{
    uint32_t node_index;

    if (stable_id < TREE_BASIC_STABLE_BASE)
    {
        return -1;
    }

    node_index = stable_id - TREE_BASIC_STABLE_BASE;
    if (node_index >= tree_basic_ctx.node_count)
    {
        return -1;
    }

    return (int32_t)node_index;
}

static tree_basic_node_t *tree_basic_get_node_by_stable_id(uint32_t stable_id)
{
    int32_t node_index = tree_basic_find_node_index_by_stable_id(stable_id);

    return node_index >= 0 ? &tree_basic_ctx.nodes[node_index] : NULL;
}

static uint8_t tree_basic_is_branch(const tree_basic_node_t *node)
{
    return (uint8_t)(node != NULL && node->child_count > 0U);
}

static uint8_t tree_basic_is_task(const tree_basic_node_t *node)
{
    return (uint8_t)(node != NULL && node->kind == TREE_BASIC_NODE_TASK);
}

static uint16_t tree_basic_append_node(uint8_t kind, uint16_t parent_index, uint8_t root_ordinal, uint8_t group_ordinal, uint8_t task_ordinal, uint8_t expanded)
{
    tree_basic_node_t *node;
    uint16_t node_index = tree_basic_ctx.node_count;

    if (node_index >= TREE_BASIC_NODE_COUNT)
    {
        return TREE_BASIC_INVALID_NODE_INDEX;
    }

    node = &tree_basic_ctx.nodes[node_index];
    memset(node, 0, sizeof(*node));
    node->stable_id = TREE_BASIC_STABLE_BASE + node_index;
    node->parent_index = parent_index;
    node->kind = kind;
    node->expanded = expanded;
    node->detail = (uint8_t)(kind == TREE_BASIC_NODE_TASK && ((root_ordinal + group_ordinal + task_ordinal) % 5U) == 0U);
    node->progress = (uint8_t)((root_ordinal * 23U + group_ordinal * 17U + task_ordinal * 11U + 15U) % 100U);
    node->revision = (uint8_t)((root_ordinal * 3U + group_ordinal * 5U + task_ordinal) % 6U);
    node->root_ordinal = root_ordinal;
    node->group_ordinal = group_ordinal;
    node->task_ordinal = task_ordinal;
    tree_basic_ctx.node_count++;

    if (parent_index != TREE_BASIC_INVALID_NODE_INDEX)
    {
        tree_basic_node_t *parent = &tree_basic_ctx.nodes[parent_index];

        if (parent->child_count < TREE_BASIC_MAX_CHILDREN)
        {
            parent->child_indices[parent->child_count++] = node_index;
        }
    }

    return node_index;
}

static void tree_basic_abort_motion(void)
{
    egui_scroller_about_animation(&tree_view.base.base.scroller);
    tree_view.base.base.is_begin_dragged = 0U;
}

static int32_t tree_basic_measure_node_height_by_node(const tree_basic_node_t *node)
{
    if (node == NULL)
    {
        return TREE_BASIC_GROUP_H;
    }

    if (node->kind == TREE_BASIC_NODE_ROOT)
    {
        return TREE_BASIC_ROOT_H;
    }
    if (node->kind == TREE_BASIC_NODE_GROUP)
    {
        return TREE_BASIC_GROUP_H;
    }
    return node->detail ? TREE_BASIC_TASK_DETAIL_H : TREE_BASIC_TASK_H;
}

static void tree_basic_expand_path_to_node(uint16_t node_index)
{
    tree_basic_node_t *node = tree_basic_get_node(node_index);

    while (node != NULL && node->parent_index != TREE_BASIC_INVALID_NODE_INDEX)
    {
        node = tree_basic_get_node(node->parent_index);
        if (node != NULL && tree_basic_is_branch(node))
        {
            node->expanded = 1U;
        }
    }
}

static void tree_basic_reset_model(void)
{
    uint8_t root_ordinal;

    memset(&tree_basic_ctx, 0, sizeof(tree_basic_ctx));

    for (root_ordinal = 0; root_ordinal < TREE_BASIC_ROOT_COUNT; root_ordinal++)
    {
        uint8_t group_ordinal;
        uint16_t root_index = tree_basic_append_node(TREE_BASIC_NODE_ROOT, TREE_BASIC_INVALID_NODE_INDEX, root_ordinal, 0, 0, 1);

        tree_basic_ctx.root_indices[root_ordinal] = root_index;

        for (group_ordinal = 0; group_ordinal < TREE_BASIC_GROUP_COUNT; group_ordinal++)
        {
            uint8_t task_ordinal;
            uint16_t group_index = tree_basic_append_node(TREE_BASIC_NODE_GROUP, root_index, root_ordinal, group_ordinal, 0,
                                                          (uint8_t)(root_ordinal == 0U && group_ordinal == 0U));

            for (task_ordinal = 0; task_ordinal < TREE_BASIC_TASK_COUNT; task_ordinal++)
            {
                (void)tree_basic_append_node(TREE_BASIC_NODE_TASK, group_index, root_ordinal, group_ordinal, task_ordinal, 0);
            }
        }
    }

    tree_basic_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    tree_basic_ctx.last_clicked_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    tree_basic_ctx.last_clicked_visible_index = EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX;
    tree_basic_ctx.jump_cursor = 0U;
    tree_basic_ctx.jump_target_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t tree_basic_visible_node_visitor(egui_view_t *self, const egui_view_virtual_tree_slot_t *slot, const egui_view_virtual_tree_entry_t *entry,
                                               egui_view_t *node_view, void *context)
{
    tree_basic_visible_summary_t *summary = (tree_basic_visible_summary_t *)context;

    EGUI_UNUSED(node_view);

    if (slot == NULL || entry == NULL || !egui_view_virtual_viewport_is_slot_center_visible(self, slot))
    {
        return 1;
    }

    if (!summary->has_first)
    {
        summary->first_visible_index = entry->visible_index;
        summary->first_visible_id = entry->stable_id;
        summary->has_first = 1U;
    }

    summary->visible_count++;
    return 1;
}
#endif

static void tree_basic_mark_selected(uint32_t stable_id)
{
    uint32_t previous_id = tree_basic_ctx.selected_id;

    if (previous_id == stable_id)
    {
        return;
    }

    tree_basic_ctx.selected_id = stable_id;
    if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_tree_notify_node_changed_by_stable_id(EGUI_VIEW_OF(&tree_view), previous_id);
    }
    if (stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_tree_notify_node_changed_by_stable_id(EGUI_VIEW_OF(&tree_view), stable_id);
    }
}

static uint8_t tree_basic_resolve_node_from_any_view(egui_view_t *view, egui_view_virtual_tree_entry_t *entry)
{
    egui_view_t *cursor = view;

    while (cursor != NULL)
    {
        if (egui_view_virtual_tree_resolve_node_by_view(EGUI_VIEW_OF(&tree_view), cursor, entry))
        {
            return 1;
        }
        cursor = EGUI_VIEW_PARENT(cursor);
    }

    return 0;
}

static void tree_basic_node_click_cb(egui_view_t *self)
{
    egui_view_virtual_tree_entry_t entry;
    tree_basic_node_t *node;

    if (!tree_basic_resolve_node_from_any_view(self, &entry))
    {
        return;
    }

    node = tree_basic_get_node_by_stable_id(entry.stable_id);
    if (node == NULL)
    {
        return;
    }

    tree_basic_ctx.click_count++;
    tree_basic_ctx.last_clicked_id = entry.stable_id;
    tree_basic_ctx.last_clicked_visible_index = entry.visible_index;
    tree_basic_mark_selected(entry.stable_id);

    if (tree_basic_is_branch(node))
    {
        node->expanded = (uint8_t)!node->expanded;
        egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&tree_view));
        (void)egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&tree_view), entry.stable_id, 6);
    }
}

static uint16_t tree_basic_find_first_task_index(void)
{
    uint16_t node_index;

    for (node_index = 0; node_index < tree_basic_ctx.node_count; node_index++)
    {
        if (tree_basic_ctx.nodes[node_index].kind == TREE_BASIC_NODE_TASK)
        {
            return node_index;
        }
    }

    return TREE_BASIC_INVALID_NODE_INDEX;
}

static void tree_basic_patch_selected(void)
{
    tree_basic_node_t *node = tree_basic_get_node_by_stable_id(tree_basic_ctx.selected_id);
    uint8_t was_detail;

    if (!tree_basic_is_task(node))
    {
        node = tree_basic_get_node(tree_basic_find_first_task_index());
        if (node == NULL)
        {
            return;
        }
        tree_basic_mark_selected(node->stable_id);
    }

    was_detail = node->detail;
    node->detail = (uint8_t)!node->detail;
    node->progress = (uint8_t)((node->progress + 17U) % 100U);
    node->revision++;
    tree_basic_ctx.patch_count++;
    tree_basic_ctx.jump_target_id = node->stable_id;

    if (was_detail != node->detail)
    {
        egui_view_virtual_tree_notify_node_resized_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id);
    }
    else
    {
        egui_view_virtual_tree_notify_node_changed_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id);
    }

    (void)egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id, 6);
}

static void tree_basic_jump_to_next(void)
{
    uint16_t node_index;
    uint16_t start_index = tree_basic_ctx.jump_cursor;

    for (node_index = 0; node_index < tree_basic_ctx.node_count; node_index++)
    {
        uint16_t candidate = (uint16_t)((start_index + TREE_BASIC_JUMP_STEP + node_index) % tree_basic_ctx.node_count);
        tree_basic_node_t *node = &tree_basic_ctx.nodes[candidate];

        if (node->kind != TREE_BASIC_NODE_TASK)
        {
            continue;
        }

        tree_basic_ctx.jump_cursor = candidate;
        tree_basic_ctx.jump_target_id = node->stable_id;
        tree_basic_expand_path_to_node(candidate);
        tree_basic_abort_motion();
        tree_basic_mark_selected(node->stable_id);
        egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&tree_view));
        egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&tree_view), node->stable_id, 0);
        egui_view_virtual_tree_scroll_to_node_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id, 0);
        (void)egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id, 6);
        return;
    }
}

static void tree_basic_reset_demo(void)
{
    tree_basic_abort_motion();
    tree_basic_reset_model();
    egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&tree_view));
    egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&tree_view), tree_basic_ctx.nodes[tree_basic_ctx.root_indices[0]].stable_id, 0);
    egui_view_virtual_tree_set_scroll_y(EGUI_VIEW_OF(&tree_view), 0);
}

static int tree_basic_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < TREE_BASIC_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void tree_basic_action_button_click_cb(egui_view_t *self)
{
    switch (tree_basic_find_action_button_index(self))
    {
    case TREE_BASIC_ACTION_PATCH:
        tree_basic_patch_selected();
        break;
    case TREE_BASIC_ACTION_JUMP:
        tree_basic_jump_to_next();
        break;
    case TREE_BASIC_ACTION_RESET:
        tree_basic_reset_demo();
        break;
    default:
        break;
    }
}

static void tree_basic_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, const egui_font_t *font, uint8_t align,
                                  egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), w, h);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), align);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static void tree_basic_style_action_button(egui_view_button_t *button, uint8_t action_index)
{
    egui_background_t *background;

    switch (action_index)
    {
    case TREE_BASIC_ACTION_PATCH:
        background = EGUI_BG_OF(&tree_basic_action_patch_bg);
        break;
    case TREE_BASIC_ACTION_JUMP:
        background = EGUI_BG_OF(&tree_basic_action_jump_bg);
        break;
    default:
        background = EGUI_BG_OF(&tree_basic_action_reset_bg);
        break;
    }

    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_size(EGUI_VIEW_OF(button), TREE_BASIC_ACTION_W, 22);
    egui_view_set_background(EGUI_VIEW_OF(button), background);
    egui_view_label_set_font(EGUI_VIEW_OF(button), TREE_BASIC_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x314454), EGUI_ALPHA_100);
    egui_view_label_set_text(EGUI_VIEW_OF(button), tree_basic_action_names[action_index]);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), tree_basic_action_button_click_cb);
}

static uint32_t tree_basic_ds_get_root_count(void *context)
{
    EGUI_UNUSED(context);
    return TREE_BASIC_ROOT_COUNT;
}

static uint32_t tree_basic_ds_get_root_stable_id(void *context, uint32_t root_index)
{
    tree_basic_context_t *ctx = (tree_basic_context_t *)context;

    if (root_index >= TREE_BASIC_ROOT_COUNT)
    {
        return EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    return ctx->nodes[ctx->root_indices[root_index]].stable_id;
}

static uint32_t tree_basic_ds_get_child_count(void *context, uint32_t stable_id)
{
    tree_basic_context_t *ctx = (tree_basic_context_t *)context;
    int32_t node_index = (int32_t)(stable_id - TREE_BASIC_STABLE_BASE);

    if (node_index < 0 || node_index >= ctx->node_count)
    {
        return 0;
    }

    return ctx->nodes[node_index].child_count;
}

static uint32_t tree_basic_ds_get_child_stable_id(void *context, uint32_t stable_id, uint32_t child_index)
{
    tree_basic_context_t *ctx = (tree_basic_context_t *)context;
    int32_t node_index = (int32_t)(stable_id - TREE_BASIC_STABLE_BASE);

    if (node_index < 0 || node_index >= ctx->node_count || child_index >= ctx->nodes[node_index].child_count)
    {
        return EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    return ctx->nodes[ctx->nodes[node_index].child_indices[child_index]].stable_id;
}

static uint8_t tree_basic_ds_is_node_expanded(void *context, uint32_t stable_id)
{
    tree_basic_context_t *ctx = (tree_basic_context_t *)context;
    int32_t node_index = (int32_t)(stable_id - TREE_BASIC_STABLE_BASE);

    if (node_index < 0 || node_index >= ctx->node_count)
    {
        return 0;
    }

    return ctx->nodes[node_index].expanded;
}

static uint16_t tree_basic_ds_get_node_view_type(void *context, const egui_view_virtual_tree_entry_t *entry)
{
    tree_basic_context_t *ctx = (tree_basic_context_t *)context;
    int32_t node_index = (int32_t)(entry->stable_id - TREE_BASIC_STABLE_BASE);

    if (node_index < 0 || node_index >= ctx->node_count)
    {
        return TREE_BASIC_VIEW_BRANCH;
    }

    return ctx->nodes[node_index].kind == TREE_BASIC_NODE_TASK ? TREE_BASIC_VIEW_TASK : TREE_BASIC_VIEW_BRANCH;
}

static int32_t tree_basic_ds_measure_node_height(void *context, const egui_view_virtual_tree_entry_t *entry, int32_t width_hint)
{
    tree_basic_context_t *ctx = (tree_basic_context_t *)context;
    int32_t node_index = (int32_t)(entry->stable_id - TREE_BASIC_STABLE_BASE);

    EGUI_UNUSED(width_hint);

    if (node_index < 0 || node_index >= ctx->node_count)
    {
        return TREE_BASIC_GROUP_H;
    }

    return tree_basic_measure_node_height_by_node(&ctx->nodes[node_index]);
}

static egui_color_t tree_basic_line_color(const tree_basic_node_t *node, uint8_t selected)
{
    if (selected)
    {
        return EGUI_COLOR_HEX(0x2F5E8A);
    }

    return node->kind == TREE_BASIC_NODE_GROUP ? EGUI_COLOR_HEX(0x98C6AB) : EGUI_COLOR_HEX(0xAFC6D8);
}

static egui_color_t tree_basic_fill_color(const tree_basic_node_t *node, uint8_t selected)
{
    if (selected)
    {
        return EGUI_COLOR_HEX(0xEAF2FA);
    }

    if (node->kind == TREE_BASIC_NODE_ROOT)
    {
        return EGUI_COLOR_HEX(0xEEF6FB);
    }
    if (node->kind == TREE_BASIC_NODE_GROUP)
    {
        return EGUI_COLOR_HEX(0xF4FBF7);
    }
    return node->detail ? EGUI_COLOR_HEX(0xFFF8EE) : EGUI_COLOR_WHITE;
}

static egui_color_t tree_basic_border_color(const tree_basic_node_t *node, uint8_t selected)
{
    if (selected)
    {
        return EGUI_COLOR_HEX(0x2F5E8A);
    }

    if (node->kind == TREE_BASIC_NODE_ROOT)
    {
        return EGUI_COLOR_HEX(0xAFC6D8);
    }
    if (node->kind == TREE_BASIC_NODE_GROUP)
    {
        return EGUI_COLOR_HEX(0x9ED0B3);
    }
    return EGUI_COLOR_HEX(0xD5C18A);
}

static void tree_basic_bind_branch_view(tree_basic_branch_view_t *view, const egui_view_virtual_tree_entry_t *entry, const tree_basic_node_t *node)
{
    uint8_t selected = (uint8_t)(node->stable_id == tree_basic_ctx.selected_id);
    egui_dim_t node_height = (egui_dim_t)tree_basic_measure_node_height_by_node(node);
    egui_dim_t card_x = node->kind == TREE_BASIC_NODE_ROOT ? 8 : 42;
    egui_dim_t card_w = (egui_dim_t)(TREE_BASIC_VIEW_W - card_x - 8);
    egui_color_t line = tree_basic_line_color(node, selected);

    if (node->kind == TREE_BASIC_NODE_ROOT)
    {
        snprintf(view->title_text, sizeof(view->title_text), "Root %u  %s", (unsigned)node->root_ordinal, tree_basic_root_names[node->root_ordinal]);
        snprintf(view->meta_text, sizeof(view->meta_text), "%u groups  %u tasks", (unsigned)node->child_count,
                 (unsigned)(node->child_count * TREE_BASIC_TASK_COUNT));
        snprintf(view->badge_text, sizeof(view->badge_text), node->expanded ? "Open" : "Fold");
    }
    else
    {
        snprintf(view->title_text, sizeof(view->title_text), "Group %u-%u  %s", (unsigned)node->root_ordinal, (unsigned)node->group_ordinal,
                 tree_basic_group_names[node->group_ordinal]);
        snprintf(view->meta_text, sizeof(view->meta_text), "%u tasks  tap branch", (unsigned)node->child_count);
        snprintf(view->badge_text, sizeof(view->badge_text), node->expanded ? "Open" : "Fold");
    }

    egui_view_set_position(EGUI_VIEW_OF(&view->root), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&view->root), TREE_BASIC_VIEW_W, node_height);
    egui_view_set_position(EGUI_VIEW_OF(&view->guide), node->kind == TREE_BASIC_NODE_ROOT ? 0 : 24, 0);
    egui_view_set_size(EGUI_VIEW_OF(&view->guide), node->kind == TREE_BASIC_NODE_ROOT ? 0 : 2, node_height);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->guide), line, EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&view->connector), node->kind == TREE_BASIC_NODE_ROOT ? 0 : 24, node_height / 2);
    egui_view_set_size(EGUI_VIEW_OF(&view->connector), node->kind == TREE_BASIC_NODE_ROOT ? 0 : 16, 2);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->connector), line, EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&view->card), card_x, 4);
    egui_view_set_size(EGUI_VIEW_OF(&view->card), card_w, node_height - 8);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->card), tree_basic_fill_color(node, selected), EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&view->card), 1, tree_basic_border_color(node, selected));

    egui_view_set_position(EGUI_VIEW_OF(&view->title), 12, 8);
    egui_view_set_size(EGUI_VIEW_OF(&view->title), card_w - 76, 14);
    egui_view_set_position(EGUI_VIEW_OF(&view->meta), 12, 26);
    egui_view_set_size(EGUI_VIEW_OF(&view->meta), card_w - 24, 12);
    egui_view_set_position(EGUI_VIEW_OF(&view->badge), card_w - 58, 8);
    egui_view_set_size(EGUI_VIEW_OF(&view->badge), 46, 12);

    egui_view_label_set_text(EGUI_VIEW_OF(&view->title), view->title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->meta), view->meta_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->badge), view->badge_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&view->title), node->kind == TREE_BASIC_NODE_ROOT ? EGUI_COLOR_HEX(0x26425A) : EGUI_COLOR_HEX(0x2F5640),
                                   EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&view->badge), selected ? EGUI_COLOR_HEX(0x2F5E8A) : EGUI_COLOR_HEX(0x5E7487), EGUI_ALPHA_100);

    EGUI_UNUSED(entry);
}

static void tree_basic_bind_task_view(tree_basic_task_view_t *view, const egui_view_virtual_tree_entry_t *entry, const tree_basic_node_t *node)
{
    uint8_t selected = (uint8_t)(node->stable_id == tree_basic_ctx.selected_id);
    egui_dim_t node_height = (egui_dim_t)tree_basic_measure_node_height_by_node(node);
    egui_dim_t card_x = 60;
    egui_dim_t card_w = (egui_dim_t)(TREE_BASIC_VIEW_W - card_x - 8);
    egui_dim_t card_h = (egui_dim_t)(node_height - 8);
    egui_color_t line = tree_basic_line_color(node, selected);

    snprintf(view->title_text, sizeof(view->title_text), "Task %u-%u-%u", (unsigned)node->root_ordinal, (unsigned)node->group_ordinal,
             (unsigned)node->task_ordinal);
    snprintf(view->body_text, sizeof(view->body_text), node->detail ? "%s, revision %u" : "", tree_basic_task_notes[node->task_ordinal],
             (unsigned)node->revision);
    snprintf(view->meta_text, sizeof(view->meta_text), "rev %u  hit confirms node", (unsigned)node->revision);
    snprintf(view->badge_text, sizeof(view->badge_text), "%u%%", (unsigned)node->progress);

    egui_view_set_position(EGUI_VIEW_OF(&view->root), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&view->root), TREE_BASIC_VIEW_W, node_height);
    egui_view_set_position(EGUI_VIEW_OF(&view->guide), 24, 0);
    egui_view_set_size(EGUI_VIEW_OF(&view->guide), 2, node_height);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->guide), line, EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&view->connector), 24, 22);
    egui_view_set_size(EGUI_VIEW_OF(&view->connector), 18, 2);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->connector), line, EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&view->marker), 40, 18);
    egui_view_set_size(EGUI_VIEW_OF(&view->marker), 8, 8);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->marker), selected ? EGUI_COLOR_HEX(0x2F5E8A) : EGUI_COLOR_HEX(0xD9AA52), EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&view->card), card_x, 4);
    egui_view_set_size(EGUI_VIEW_OF(&view->card), card_w, card_h);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->card), tree_basic_fill_color(node, selected), EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&view->card), 1, tree_basic_border_color(node, selected));

    egui_view_set_position(EGUI_VIEW_OF(&view->title), 12, 8);
    egui_view_set_size(EGUI_VIEW_OF(&view->title), card_w - 70, 14);
    egui_view_set_position(EGUI_VIEW_OF(&view->badge), card_w - 52, 8);
    egui_view_set_size(EGUI_VIEW_OF(&view->badge), 40, 12);
    egui_view_set_position(EGUI_VIEW_OF(&view->body), 12, 24);
    egui_view_set_size(EGUI_VIEW_OF(&view->body), card_w - 24, 12);
    egui_view_set_position(EGUI_VIEW_OF(&view->meta), 12, node->detail ? 40 : 24);
    egui_view_set_size(EGUI_VIEW_OF(&view->meta), card_w - 24, 12);

    egui_view_set_position(EGUI_VIEW_OF(&view->progress), 12, node->detail ? 56 : 40);
    egui_view_set_size(EGUI_VIEW_OF(&view->progress), card_w - 24, 7);

    egui_view_label_set_text(EGUI_VIEW_OF(&view->title), view->title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->body), view->body_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->meta), view->meta_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->badge), view->badge_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&view->title), EGUI_COLOR_HEX(0x4A3A20), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&view->badge), selected ? EGUI_COLOR_HEX(0x2F5E8A) : EGUI_COLOR_HEX(0x86632A), EGUI_ALPHA_100);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&view->progress), node->progress);
    view->progress.bk_color = EGUI_COLOR_HEX(0xE4E9ED);
    view->progress.progress_color = selected ? EGUI_COLOR_HEX(0x2F5E8A) : EGUI_COLOR_HEX(0xD9AA52);
    view->progress.control_color = view->progress.progress_color;
    view->progress.is_show_control = 0;

    EGUI_UNUSED(entry);
}

static egui_view_t *tree_basic_ds_create_node_view(void *context, uint16_t view_type)
{
    EGUI_UNUSED(context);

    if (view_type == TREE_BASIC_VIEW_TASK)
    {
        tree_basic_task_view_t *view = (tree_basic_task_view_t *)egui_malloc(sizeof(tree_basic_task_view_t));
        if (view == NULL)
        {
            return NULL;
        }

        memset(view, 0, sizeof(*view));
        egui_view_group_init(EGUI_VIEW_OF(&view->root));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->root), tree_basic_node_click_cb);

        egui_view_card_init(EGUI_VIEW_OF(&view->guide));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->guide), tree_basic_node_click_cb);
        egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->guide));

        egui_view_card_init(EGUI_VIEW_OF(&view->connector));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->connector), tree_basic_node_click_cb);
        egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->connector));

        egui_view_card_init(EGUI_VIEW_OF(&view->marker));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->marker), tree_basic_node_click_cb);
        egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->marker));

        egui_view_card_init(EGUI_VIEW_OF(&view->card));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->card), tree_basic_node_click_cb);
        egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->card));

        tree_basic_init_label(&view->title, 12, 8, 120, 14, TREE_BASIC_FONT_TITLE, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x4A3A20));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->title));
        tree_basic_init_label(&view->body, 12, 24, 120, 12, TREE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x6E5A35));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->body));
        tree_basic_init_label(&view->meta, 12, 40, 140, 12, TREE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x7A6242));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->meta));
        tree_basic_init_label(&view->badge, 150, 8, 40, 12, TREE_BASIC_FONT_BODY, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x86632A));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->badge));

        egui_view_progress_bar_init(EGUI_VIEW_OF(&view->progress));
        egui_view_set_position(EGUI_VIEW_OF(&view->progress), 12, 40);
        egui_view_set_size(EGUI_VIEW_OF(&view->progress), 120, 7);
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->progress));

        return EGUI_VIEW_OF(&view->root);
    }

    {
        tree_basic_branch_view_t *view = (tree_basic_branch_view_t *)egui_malloc(sizeof(tree_basic_branch_view_t));
        if (view == NULL)
        {
            return NULL;
        }

        memset(view, 0, sizeof(*view));
        egui_view_group_init(EGUI_VIEW_OF(&view->root));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->root), tree_basic_node_click_cb);

        egui_view_card_init(EGUI_VIEW_OF(&view->guide));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->guide), tree_basic_node_click_cb);
        egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->guide));

        egui_view_card_init(EGUI_VIEW_OF(&view->connector));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->connector), tree_basic_node_click_cb);
        egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->connector));

        egui_view_card_init(EGUI_VIEW_OF(&view->card));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->card), tree_basic_node_click_cb);
        egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->card));

        tree_basic_init_label(&view->title, 12, 8, 140, 14, TREE_BASIC_FONT_TITLE, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x26425A));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->title));
        tree_basic_init_label(&view->meta, 12, 26, 140, 12, TREE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x5F7687));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->meta));
        tree_basic_init_label(&view->badge, 156, 8, 40, 12, TREE_BASIC_FONT_BODY, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x5F7687));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->badge));

        return EGUI_VIEW_OF(&view->root);
    }
}

static void tree_basic_ds_destroy_node_view(void *context, egui_view_t *view, uint16_t view_type)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(view_type);
    egui_free(view);
}

static void tree_basic_ds_bind_node_view(void *context, egui_view_t *view, const egui_view_virtual_tree_entry_t *entry)
{
    tree_basic_context_t *ctx = (tree_basic_context_t *)context;
    int32_t node_index = (int32_t)(entry->stable_id - TREE_BASIC_STABLE_BASE);

    if (node_index < 0 || node_index >= ctx->node_count)
    {
        return;
    }

    if (ctx->nodes[node_index].kind == TREE_BASIC_NODE_TASK)
    {
        tree_basic_bind_task_view((tree_basic_task_view_t *)view, entry, &ctx->nodes[node_index]);
    }
    else
    {
        tree_basic_bind_branch_view((tree_basic_branch_view_t *)view, entry, &ctx->nodes[node_index]);
    }
}

static uint8_t tree_basic_ds_should_keep_alive(void *context, egui_view_t *view, uint32_t stable_id)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(view);

    return stable_id == tree_basic_ctx.selected_id;
}

#if EGUI_CONFIG_RECORDING_TEST
static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1U;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static uint8_t tree_basic_is_view_clickable(egui_view_t *view)
{
    int click_x;
    int click_y;

    return (uint8_t)egui_sim_get_view_clipped_center(view, &EGUI_VIEW_OF(&tree_view)->region_screen, &click_x, &click_y);
}

static uint8_t tree_basic_set_click_item_action(egui_sim_action_t *p_action, egui_view_t *view, uint32_t interval_ms)
{
    return (uint8_t)egui_sim_set_click_view_clipped(p_action, view, &EGUI_VIEW_OF(&tree_view)->region_screen, (int)interval_ms);
}

static tree_basic_node_t *tree_basic_find_node_by_triplet(uint8_t kind, uint8_t root_ordinal, uint8_t group_ordinal, uint8_t task_ordinal)
{
    uint16_t node_index;

    for (node_index = 0; node_index < tree_basic_ctx.node_count; node_index++)
    {
        tree_basic_node_t *node = &tree_basic_ctx.nodes[node_index];

        if (node->kind != kind || node->root_ordinal != root_ordinal)
        {
            continue;
        }
        if (kind != TREE_BASIC_NODE_ROOT && node->group_ordinal != group_ordinal)
        {
            continue;
        }
        if (kind == TREE_BASIC_NODE_TASK && node->task_ordinal != task_ordinal)
        {
            continue;
        }
        return node;
    }

    return NULL;
}

static egui_view_t *tree_basic_find_visible_view_by_stable_id(uint32_t stable_id)
{
    const egui_view_virtual_tree_slot_t *slot = egui_view_virtual_tree_find_slot_by_stable_id(EGUI_VIEW_OF(&tree_view), stable_id);
    egui_view_t *view;

    if (slot == NULL)
    {
        return NULL;
    }

    view = egui_view_virtual_tree_find_view_by_stable_id(EGUI_VIEW_OF(&tree_view), stable_id);
    return tree_basic_is_view_clickable(view) ? view : NULL;
}

static egui_view_t *tree_basic_find_visible_view(uint8_t kind, uint8_t root_ordinal, uint8_t group_ordinal, uint8_t task_ordinal)
{
    tree_basic_node_t *node = tree_basic_find_node_by_triplet(kind, root_ordinal, group_ordinal, task_ordinal);

    if (node == NULL)
    {
        return NULL;
    }

    return tree_basic_find_visible_view_by_stable_id(node->stable_id);
}

static void tree_basic_set_scroll_action(egui_sim_action_t *p_action, uint32_t interval_ms)
{
    egui_region_t *region = &EGUI_VIEW_OF(&tree_view)->region_screen;

    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = (egui_dim_t)(region->location.x + region->size.width / 2);
    p_action->y1 = (egui_dim_t)(region->location.y + region->size.height - 24);
    p_action->x2 = p_action->x1;
    p_action->y2 = (egui_dim_t)(region->location.y + 42);
    p_action->steps = 10;
    p_action->interval_ms = interval_ms;
}

static uint32_t tree_basic_get_first_center_visible_id(void)
{
    tree_basic_visible_summary_t summary;

    memset(&summary, 0, sizeof(summary));
    egui_view_virtual_tree_visit_visible_nodes(EGUI_VIEW_OF(&tree_view), tree_basic_visible_node_visitor, &summary);
    return summary.has_first ? summary.first_visible_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    static uint32_t visible_before_collapse = 0;
    int first_call = (action_index != last_action);
    egui_view_t *view;
    tree_basic_node_t *node;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            if (egui_view_virtual_tree_get_slot_count(EGUI_VIEW_OF(&tree_view)) == 0U)
            {
                report_runtime_failure("tree basic should materialize initial slots");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        view = tree_basic_find_visible_view(TREE_BASIC_NODE_TASK, 0, 0, 0);
        if (view == NULL)
        {
            report_runtime_failure("initial task node was not visible");
            EGUI_SIM_SET_WAIT(p_action, 200);
            return true;
        }
        if (!tree_basic_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("initial task node click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
        }
        return true;
    case 2:
        node = tree_basic_find_node_by_triplet(TREE_BASIC_NODE_TASK, 0, 0, 0);
        if (first_call && (node == NULL || tree_basic_ctx.selected_id != node->stable_id))
        {
            report_runtime_failure("task click did not update selected node");
        }
        visible_before_collapse = egui_view_virtual_tree_get_visible_node_count(EGUI_VIEW_OF(&tree_view));
        view = tree_basic_find_visible_view(TREE_BASIC_NODE_GROUP, 0, 0, 0);
        if (view == NULL)
        {
            report_runtime_failure("group branch was not visible");
            EGUI_SIM_SET_WAIT(p_action, 200);
            return true;
        }
        if (!tree_basic_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("group branch click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
        }
        return true;
    case 3:
        if (first_call && egui_view_virtual_tree_get_visible_node_count(EGUI_VIEW_OF(&tree_view)) >= visible_before_collapse)
        {
            report_runtime_failure("branch collapse did not reduce visible nodes");
        }
        recording_jump_verify_retry = 0U;
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[TREE_BASIC_ACTION_JUMP]), 220);
        return true;
    case 4:
        view = tree_basic_find_visible_view_by_stable_id(tree_basic_ctx.jump_target_id);
        if (tree_basic_ctx.jump_target_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID || tree_basic_ctx.selected_id != tree_basic_ctx.jump_target_id)
        {
            if (recording_jump_verify_retry < TREE_BASIC_JUMP_VERIFY_RETRY_MAX)
            {
                if (tree_basic_ctx.jump_target_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
                {
                    tree_basic_abort_motion();
                    egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&tree_view), tree_basic_ctx.jump_target_id, 0);
                    egui_view_virtual_tree_scroll_to_node_by_stable_id(EGUI_VIEW_OF(&tree_view), tree_basic_ctx.jump_target_id, 0);
                    (void)egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&tree_view), tree_basic_ctx.jump_target_id, 0);
                }
                recording_jump_verify_retry++;
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            if (tree_basic_ctx.jump_target_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
            {
                report_runtime_failure("jump action did not choose a target task");
            }
            else
            {
                report_runtime_failure("jump action did not select target task");
            }
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        recording_jump_verify_retry = 0U;
        if (view != NULL && !tree_basic_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("jump target task click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 5:
        if (first_call && tree_basic_ctx.selected_id != tree_basic_ctx.jump_target_id)
        {
            report_runtime_failure("target task was not selected correctly after jump");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[TREE_BASIC_ACTION_PATCH]), 220);
        return true;
    case 6:
        node = tree_basic_get_node_by_stable_id(tree_basic_ctx.jump_target_id);
        if (first_call && (node == NULL || !node->detail))
        {
            report_runtime_failure("patch action did not toggle task detail state");
        }
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 7:
        tree_basic_set_scroll_action(p_action, 320);
        return true;
    case 8:
        if (first_call && tree_basic_get_first_center_visible_id() == tree_basic_ctx.nodes[tree_basic_ctx.root_indices[0]].stable_id)
        {
            report_runtime_failure("scroll action did not move tree viewport");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[TREE_BASIC_ACTION_RESET]), 220);
        return true;
    case 9:
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 10:
        if (first_call)
        {
            if (tree_basic_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
            {
                report_runtime_failure("reset action did not clear selected node");
            }
            if (tree_basic_get_first_center_visible_id() != tree_basic_ctx.nodes[tree_basic_ctx.root_indices[0]].stable_id)
            {
                report_runtime_failure("reset action did not restore top position");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    default:
        return false;
    }
}
#endif

void test_init_ui(void)
{
    uint8_t i;
    egui_view_virtual_tree_setup_t setup = {
            .params = &tree_basic_view_params,
            .data_source = &tree_basic_data_source,
            .data_source_context = &tree_basic_ctx,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };

    tree_basic_reset_model();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0U;
    recording_jump_verify_retry = 0U;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&tree_basic_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &tree_basic_toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD1DEE7));

    for (i = 0; i < TREE_BASIC_ACTION_COUNT; i++)
    {
        tree_basic_style_action_button(&action_buttons[i], i);
        egui_view_set_position(EGUI_VIEW_OF(&action_buttons[i]), 10 + i * (TREE_BASIC_ACTION_W + TREE_BASIC_ACTION_GAP), 6);
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
    }

    egui_view_virtual_tree_init_with_setup(EGUI_VIEW_OF(&tree_view), &setup);
    egui_view_set_background(EGUI_VIEW_OF(&tree_view), EGUI_BG_OF(&tree_basic_view_bg));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&tree_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}
