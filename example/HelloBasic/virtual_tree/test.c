#include "egui.h"

#include <stdio.h>
#include <string.h>

#include "uicode.h"

#define TREE_DEMO_ROOT_COUNT            6U
#define TREE_DEMO_MAX_GROUPS_PER_ROOT   13U
#define TREE_DEMO_MAX_TASKS_PER_GROUP   16U
#define TREE_DEMO_MAX_CHILDREN_PER_NODE 16U
#define TREE_DEMO_MAX_NODES                                                                                                                                    \
    (TREE_DEMO_ROOT_COUNT + TREE_DEMO_ROOT_COUNT * TREE_DEMO_MAX_GROUPS_PER_ROOT +                                                                             \
     TREE_DEMO_ROOT_COUNT * TREE_DEMO_MAX_GROUPS_PER_ROOT * TREE_DEMO_MAX_TASKS_PER_GROUP)
#define TREE_DEMO_NODE_ID_BASE       700000U
#define TREE_DEMO_INVALID_INDEX      0xFFFFFFFFUL
#define TREE_DEMO_INVALID_NODE_INDEX 0xFFFFU
#define TREE_DEMO_STATUS_TEXT_LEN    96
#define TREE_DEMO_TITLE_TEXT_LEN     40
#define TREE_DEMO_BODY_TEXT_LEN      48
#define TREE_DEMO_META_TEXT_LEN      40
#define TREE_DEMO_BADGE_TEXT_LEN     12
#define TREE_DEMO_STATE_CACHE_COUNT  96U

#define TREE_DEMO_MARGIN_X     8
#define TREE_DEMO_TOP_Y        8
#define TREE_DEMO_HEADER_W     (EGUI_CONFIG_SCEEN_WIDTH - TREE_DEMO_MARGIN_X * 2)
#define TREE_DEMO_HEADER_H     68
#define TREE_DEMO_TOOLBAR_Y    (TREE_DEMO_TOP_Y + TREE_DEMO_HEADER_H + 6)
#define TREE_DEMO_TOOLBAR_H    32
#define TREE_DEMO_VIEW_Y       (TREE_DEMO_TOOLBAR_Y + TREE_DEMO_TOOLBAR_H + 6)
#define TREE_DEMO_VIEW_W       TREE_DEMO_HEADER_W
#define TREE_DEMO_VIEW_H       (EGUI_CONFIG_SCEEN_HEIGHT - TREE_DEMO_VIEW_Y - 8)
#define TREE_DEMO_BUTTON_GAP   4
#define TREE_DEMO_BUTTON_W     ((TREE_DEMO_HEADER_W - 20 - TREE_DEMO_BUTTON_GAP * 3) / 4)
#define TREE_DEMO_BUTTON_H     20
#define TREE_DEMO_NODE_GAP_Y   4
#define TREE_DEMO_NODE_INSET_X 6
#define TREE_DEMO_NODE_PAD_X   8
#define TREE_DEMO_INDENT_STEP  20
#define TREE_DEMO_BADGE_W      38
#define TREE_DEMO_BADGE_H      18
#define TREE_DEMO_PROGRESS_H   5
#define TREE_DEMO_LABEL_H      14

#define TREE_DEMO_FONT_HEADER ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define TREE_DEMO_FONT_TITLE  ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define TREE_DEMO_FONT_BODY   ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define TREE_DEMO_FONT_CAP    ((const egui_font_t *)&egui_res_font_montserrat_8_4)

enum
{
    TREE_DEMO_ACTION_EXPAND = 0,
    TREE_DEMO_ACTION_FOLD,
    TREE_DEMO_ACTION_PATCH,
    TREE_DEMO_ACTION_JUMP,
    TREE_DEMO_ACTION_COUNT,
};

enum
{
    TREE_DEMO_NODE_KIND_ROOT = 0,
    TREE_DEMO_NODE_KIND_GROUP,
    TREE_DEMO_NODE_KIND_TASK,
};

enum
{
    TREE_DEMO_VIEW_TYPE_BRANCH = 0,
    TREE_DEMO_VIEW_TYPE_TASK,
};

enum
{
    TREE_DEMO_STATE_IDLE = 0,
    TREE_DEMO_STATE_LIVE,
    TREE_DEMO_STATE_WARN,
    TREE_DEMO_STATE_DONE,
    TREE_DEMO_STATE_COUNT,
};

enum
{
    TREE_DEMO_TASK_VARIANT_COMPACT = 0,
    TREE_DEMO_TASK_VARIANT_DETAIL,
    TREE_DEMO_TASK_VARIANT_ALERT,
    TREE_DEMO_TASK_VARIANT_COUNT,
};

typedef struct tree_demo_node tree_demo_node_t;
typedef struct tree_demo_node_view tree_demo_node_view_t;
typedef struct tree_demo_node_state tree_demo_node_state_t;
typedef struct tree_demo_context tree_demo_context_t;

struct tree_demo_node
{
    uint32_t stable_id;
    uint16_t parent_index;
    uint16_t revision;
    uint8_t child_count;
    uint8_t kind;
    uint8_t state;
    uint8_t variant;
    uint8_t progress;
    uint8_t expanded;
    uint8_t ordinal;
    uint8_t root_ordinal;
    uint8_t group_ordinal;
    uint16_t child_indices[TREE_DEMO_MAX_CHILDREN_PER_NODE];
};

struct tree_demo_node_view
{
    egui_view_card_t card;
    egui_view_card_t guide;
    egui_view_card_t connector;
    egui_view_card_t marker;
    egui_view_label_t title;
    egui_view_label_t body;
    egui_view_label_t meta;
    egui_view_label_t badge;
    egui_view_progress_bar_t progress;
    egui_view_t pulse;
    egui_animation_alpha_t pulse_anim;
    egui_interpolator_linear_t pulse_interp;
    uint32_t stable_id;
    uint8_t pulse_running;
};

struct tree_demo_node_state
{
    uint16_t pulse_elapsed_ms;
    uint8_t pulse_running;
    uint8_t pulse_alpha;
    uint8_t pulse_cycle_flip;
    uint8_t pulse_repeated;
};

struct tree_demo_context
{
    uint16_t node_count;
    uint16_t root_indices[TREE_DEMO_ROOT_COUNT];
    uint32_t selected_id;
    uint32_t last_clicked_id;
    uint32_t last_clicked_visible_index;
    uint32_t click_count;
    uint32_t action_count;
    uint32_t jump_cursor;
    uint8_t created_count;
    tree_demo_node_t nodes[TREE_DEMO_MAX_NODES];
    char title_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][TREE_DEMO_TITLE_TEXT_LEN];
    char body_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][TREE_DEMO_BODY_TEXT_LEN];
    char meta_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][TREE_DEMO_META_TEXT_LEN];
    char badge_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][TREE_DEMO_BADGE_TEXT_LEN];
    char header_title_text[TREE_DEMO_STATUS_TEXT_LEN];
    char header_detail_text[TREE_DEMO_STATUS_TEXT_LEN];
    char header_hint_text[TREE_DEMO_STATUS_TEXT_LEN];
    char last_action_text[TREE_DEMO_STATUS_TEXT_LEN];
    tree_demo_node_view_t node_views[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
};

static const char *tree_demo_action_names[TREE_DEMO_ACTION_COUNT] = {"Expand", "Fold", "Patch", "Jump"};
static const char *tree_demo_root_names[TREE_DEMO_ROOT_COUNT] = {"Fleet", "Deploy", "Audit", "Inbox", "Devices", "Ops"};
static const char *tree_demo_state_names[TREE_DEMO_STATE_COUNT] = {"IDLE", "LIVE", "WARN", "DONE"};
static const char *tree_demo_task_notes[TREE_DEMO_TASK_VARIANT_COUNT] = {"leaf sync", "detail leaf", "alert leaf"};

static egui_view_t background_view;
static egui_view_card_t header_card;
static egui_view_card_t toolbar_card;
static egui_view_label_t header_title;
static egui_view_label_t header_detail;
static egui_view_label_t header_hint;
static egui_view_button_t action_buttons[TREE_DEMO_ACTION_COUNT];
static egui_view_virtual_tree_t tree_view;
static tree_demo_context_t tree_demo_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(tree_demo_header_card_params, TREE_DEMO_MARGIN_X, TREE_DEMO_TOP_Y, TREE_DEMO_HEADER_W, TREE_DEMO_HEADER_H, 14);
EGUI_VIEW_CARD_PARAMS_INIT(tree_demo_toolbar_card_params, TREE_DEMO_MARGIN_X, TREE_DEMO_TOOLBAR_Y, TREE_DEMO_HEADER_W, TREE_DEMO_TOOLBAR_H, 12);
static const egui_view_virtual_tree_params_t tree_demo_view_params = {
        .region = {{TREE_DEMO_MARGIN_X, TREE_DEMO_VIEW_Y}, {TREE_DEMO_VIEW_W, TREE_DEMO_VIEW_H}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_node_height = 58,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(tree_demo_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF4F8), EGUI_COLOR_HEX(0xD9E5EE),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_screen_bg_params, &tree_demo_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(tree_demo_screen_bg, &tree_demo_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(tree_demo_view_bg_param, EGUI_COLOR_HEX(0xF9FBFD), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_view_bg_params, &tree_demo_view_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_view_bg, &tree_demo_view_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(tree_demo_action_expand_param, EGUI_COLOR_HEX(0xDFF3E9), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_action_expand_params, &tree_demo_action_expand_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_action_expand_bg, &tree_demo_action_expand_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(tree_demo_action_fold_param, EGUI_COLOR_HEX(0xFFF1E1), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_action_fold_params, &tree_demo_action_fold_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_action_fold_bg, &tree_demo_action_fold_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(tree_demo_action_patch_param, EGUI_COLOR_HEX(0xE3F6F7), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_action_patch_params, &tree_demo_action_patch_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_action_patch_bg, &tree_demo_action_patch_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(tree_demo_action_jump_param, EGUI_COLOR_HEX(0xE5EEFF), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_action_jump_params, &tree_demo_action_jump_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_action_jump_bg, &tree_demo_action_jump_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(tree_demo_badge_selected_param, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_badge_selected_params, &tree_demo_badge_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_badge_selected_bg, &tree_demo_badge_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(tree_demo_badge_idle_param, EGUI_COLOR_HEX(0x8D99A6), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_badge_idle_params, &tree_demo_badge_idle_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_badge_idle_bg, &tree_demo_badge_idle_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(tree_demo_badge_live_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_badge_live_params, &tree_demo_badge_live_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_badge_live_bg, &tree_demo_badge_live_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(tree_demo_badge_warn_param, EGUI_COLOR_HEX(0xE7B14A), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_badge_warn_params, &tree_demo_badge_warn_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_badge_warn_bg, &tree_demo_badge_warn_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(tree_demo_badge_done_param, EGUI_COLOR_HEX(0x56789A), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_badge_done_params, &tree_demo_badge_done_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_badge_done_bg, &tree_demo_badge_done_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(tree_demo_pulse_selected_param, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_pulse_selected_params, &tree_demo_pulse_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_pulse_selected_bg, &tree_demo_pulse_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(tree_demo_pulse_live_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_pulse_live_params, &tree_demo_pulse_live_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_pulse_live_bg, &tree_demo_pulse_live_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(tree_demo_pulse_warn_param, EGUI_COLOR_HEX(0xD08A2E), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_pulse_warn_params, &tree_demo_pulse_warn_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_pulse_warn_bg, &tree_demo_pulse_warn_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(tree_demo_pulse_done_param, EGUI_COLOR_HEX(0x5C9EE6), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(tree_demo_pulse_done_params, &tree_demo_pulse_done_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(tree_demo_pulse_done_bg, &tree_demo_pulse_done_params);

EGUI_SHADOW_PARAM_INIT_ROUND(tree_demo_card_shadow, 10, 0, 3, EGUI_COLOR_BLACK, EGUI_ALPHA_20, 12);
EGUI_ANIMATION_ALPHA_PARAMS_INIT(tree_demo_pulse_anim_param, EGUI_ALPHA_100, EGUI_ALPHA_20);

static void tree_demo_refresh_status(void);
static void tree_demo_apply_action(uint8_t action);

static tree_demo_node_t *tree_demo_get_node_by_index(uint16_t node_index)
{
    if (node_index == TREE_DEMO_INVALID_NODE_INDEX || node_index >= tree_demo_ctx.node_count)
    {
        return NULL;
    }

    return &tree_demo_ctx.nodes[node_index];
}

static const tree_demo_node_t *tree_demo_get_node_const_by_index(uint16_t node_index)
{
    if (node_index == TREE_DEMO_INVALID_NODE_INDEX || node_index >= tree_demo_ctx.node_count)
    {
        return NULL;
    }

    return &tree_demo_ctx.nodes[node_index];
}

static int32_t tree_demo_find_node_index_by_stable_id(uint32_t stable_id)
{
    uint32_t node_index;

    if (stable_id < TREE_DEMO_NODE_ID_BASE)
    {
        return -1;
    }

    node_index = stable_id - TREE_DEMO_NODE_ID_BASE;
    if (node_index >= tree_demo_ctx.node_count)
    {
        return -1;
    }

    return (int32_t)node_index;
}

static tree_demo_node_t *tree_demo_get_node_by_stable_id(uint32_t stable_id)
{
    int32_t node_index = tree_demo_find_node_index_by_stable_id(stable_id);

    return node_index >= 0 ? &tree_demo_ctx.nodes[node_index] : NULL;
}

static uint8_t tree_demo_is_branch(const tree_demo_node_t *node)
{
    return (uint8_t)(node != NULL && node->child_count > 0U);
}

static uint8_t tree_demo_is_hot_state(uint8_t state)
{
    return (uint8_t)(state == TREE_DEMO_STATE_LIVE || state == TREE_DEMO_STATE_WARN);
}

static uint8_t tree_demo_is_selected_id(uint32_t stable_id)
{
    return (uint8_t)(stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && stable_id == tree_demo_ctx.selected_id);
}

static uint16_t tree_demo_append_node(uint8_t kind, uint16_t parent_index, uint8_t root_ordinal, uint8_t group_ordinal, uint8_t ordinal, uint8_t variant,
                                      uint8_t state, uint8_t progress, uint8_t expanded)
{
    tree_demo_node_t *node;
    uint16_t node_index = tree_demo_ctx.node_count;

    if (node_index >= TREE_DEMO_MAX_NODES)
    {
        return TREE_DEMO_INVALID_NODE_INDEX;
    }

    node = &tree_demo_ctx.nodes[node_index];
    memset(node, 0, sizeof(*node));
    node->stable_id = TREE_DEMO_NODE_ID_BASE + node_index;
    node->parent_index = parent_index;
    node->revision = (uint16_t)(1U + ((root_ordinal * 17U + group_ordinal * 7U + ordinal * 3U) % 9U));
    node->kind = kind;
    node->state = state;
    node->variant = variant;
    node->progress = progress;
    node->expanded = expanded;
    node->ordinal = ordinal;
    node->root_ordinal = root_ordinal;
    node->group_ordinal = group_ordinal;
    tree_demo_ctx.node_count++;

    if (parent_index != TREE_DEMO_INVALID_NODE_INDEX)
    {
        tree_demo_node_t *parent = &tree_demo_ctx.nodes[parent_index];

        if (parent->child_count < TREE_DEMO_MAX_CHILDREN_PER_NODE)
        {
            parent->child_indices[parent->child_count++] = node_index;
        }
    }

    return node_index;
}

static void tree_demo_collect_branch_metrics(uint16_t node_index, uint16_t *branch_count, uint16_t *task_count, uint16_t *hot_count)
{
    const tree_demo_node_t *node = tree_demo_get_node_const_by_index(node_index);
    uint8_t child_index;

    if (node == NULL)
    {
        return;
    }

    if (node->kind == TREE_DEMO_NODE_KIND_TASK)
    {
        if (task_count != NULL)
        {
            (*task_count)++;
        }
        if (hot_count != NULL && tree_demo_is_hot_state(node->state))
        {
            (*hot_count)++;
        }
        return;
    }

    if (branch_count != NULL && node->kind != TREE_DEMO_NODE_KIND_ROOT)
    {
        (*branch_count)++;
    }

    for (child_index = 0; child_index < node->child_count; child_index++)
    {
        tree_demo_collect_branch_metrics(node->child_indices[child_index], branch_count, task_count, hot_count);
    }
}

static int32_t tree_demo_measure_node_height_with_state(const tree_demo_node_t *node, uint8_t selected)
{
    int32_t height;

    if (node == NULL)
    {
        return 52;
    }

    if (node->kind == TREE_DEMO_NODE_KIND_ROOT)
    {
        height = 76;
        if (node->expanded)
        {
            height += 4;
        }
    }
    else if (node->kind == TREE_DEMO_NODE_KIND_GROUP)
    {
        height = 66;
        if (node->expanded)
        {
            height += 2;
        }
    }
    else if (node->variant == TREE_DEMO_TASK_VARIANT_ALERT)
    {
        height = 70;
    }
    else if (node->variant == TREE_DEMO_TASK_VARIANT_DETAIL)
    {
        height = 58;
    }
    else
    {
        height = 40;
    }

    if (tree_demo_is_hot_state(node->state))
    {
        height += node->kind == TREE_DEMO_NODE_KIND_TASK ? 2 : 4;
    }
    if (selected)
    {
        height += 8;
    }

    return height;
}

static uint32_t tree_demo_count_visible_from_index(uint16_t node_index)
{
    const tree_demo_node_t *node = tree_demo_get_node_const_by_index(node_index);
    uint32_t total = 0;
    uint8_t child_index;

    if (node == NULL)
    {
        return 0;
    }

    total++;
    if (!tree_demo_is_branch(node) || !node->expanded)
    {
        return total;
    }

    for (child_index = 0; child_index < node->child_count; child_index++)
    {
        total += tree_demo_count_visible_from_index(node->child_indices[child_index]);
    }

    return total;
}

static uint32_t tree_demo_get_visible_count(void)
{
    uint32_t total = 0;
    uint8_t root_index;

    for (root_index = 0; root_index < TREE_DEMO_ROOT_COUNT; root_index++)
    {
        total += tree_demo_count_visible_from_index(tree_demo_ctx.root_indices[root_index]);
    }

    return total;
}

static void tree_demo_expand_path_to_index(uint16_t node_index)
{
    tree_demo_node_t *node = tree_demo_get_node_by_index(node_index);

    while (node != NULL && node->parent_index != TREE_DEMO_INVALID_NODE_INDEX)
    {
        tree_demo_node_t *parent = tree_demo_get_node_by_index(node->parent_index);

        if (parent == NULL)
        {
            break;
        }

        parent->expanded = 1;
        node = parent;
    }
}

static uint16_t tree_demo_find_next_branch(uint8_t want_expanded, uint16_t start_after)
{
    uint16_t offset;

    if (tree_demo_ctx.node_count == 0)
    {
        return TREE_DEMO_INVALID_NODE_INDEX;
    }

    for (offset = 0; offset < tree_demo_ctx.node_count; offset++)
    {
        uint16_t node_index = (uint16_t)((start_after + offset + 1U) % tree_demo_ctx.node_count);
        const tree_demo_node_t *node = tree_demo_get_node_const_by_index(node_index);

        if (!tree_demo_is_branch(node))
        {
            continue;
        }
        if (node->expanded == want_expanded)
        {
            return node_index;
        }
    }

    return TREE_DEMO_INVALID_NODE_INDEX;
}

static uint16_t tree_demo_find_hot_task_after(uint16_t start_after)
{
    uint16_t offset;

    if (tree_demo_ctx.node_count == 0)
    {
        return TREE_DEMO_INVALID_NODE_INDEX;
    }

    for (offset = 0; offset < tree_demo_ctx.node_count; offset++)
    {
        uint16_t node_index = (uint16_t)((start_after + offset + 1U) % tree_demo_ctx.node_count);
        const tree_demo_node_t *node = tree_demo_get_node_const_by_index(node_index);

        if (node == NULL || node->kind != TREE_DEMO_NODE_KIND_TASK)
        {
            continue;
        }
        if (tree_demo_is_hot_state(node->state) || node->variant == TREE_DEMO_TASK_VARIANT_ALERT)
        {
            return node_index;
        }
    }

    return TREE_DEMO_INVALID_NODE_INDEX;
}

static uint16_t tree_demo_get_selected_branch_target(uint8_t want_expanded)
{
    tree_demo_node_t *node = tree_demo_get_node_by_stable_id(tree_demo_ctx.selected_id);

    while (node != NULL)
    {
        if (tree_demo_is_branch(node) && node->expanded == want_expanded)
        {
            return (uint16_t)(node - tree_demo_ctx.nodes);
        }
        if (node->parent_index == TREE_DEMO_INVALID_NODE_INDEX)
        {
            break;
        }
        node = tree_demo_get_node_by_index(node->parent_index);
    }

    return TREE_DEMO_INVALID_NODE_INDEX;
}

static void tree_demo_reset_model(void)
{
    uint8_t root_ordinal;

    memset(&tree_demo_ctx, 0, sizeof(tree_demo_ctx));
    tree_demo_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    tree_demo_ctx.last_clicked_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    tree_demo_ctx.last_clicked_visible_index = TREE_DEMO_INVALID_INDEX;

    for (root_ordinal = 0; root_ordinal < TREE_DEMO_ROOT_COUNT; root_ordinal++)
    {
        uint8_t group_count = (uint8_t)(11U + (root_ordinal % 3U));
        uint16_t root_index = tree_demo_append_node(TREE_DEMO_NODE_KIND_ROOT, TREE_DEMO_INVALID_NODE_INDEX, root_ordinal, 0, root_ordinal, root_ordinal,
                                                    (uint8_t)(root_ordinal % TREE_DEMO_STATE_COUNT), 0, root_ordinal < 3U ? 1U : 0U);
        uint8_t group_ordinal;

        tree_demo_ctx.root_indices[root_ordinal] = root_index;
        for (group_ordinal = 0; group_ordinal < group_count; group_ordinal++)
        {
            uint8_t task_count = (uint8_t)(14U + ((root_ordinal * 7U + group_ordinal * 5U) % 3U));
            uint8_t task_ordinal;
            uint8_t group_state = (uint8_t)((root_ordinal * 3U + group_ordinal) % TREE_DEMO_STATE_COUNT);
            uint8_t group_expanded = (uint8_t)((root_ordinal < 2U && group_ordinal < 4U) || (root_ordinal == 2U && group_ordinal < 2U));
            uint16_t group_index = tree_demo_append_node(TREE_DEMO_NODE_KIND_GROUP, root_index, root_ordinal, group_ordinal, group_ordinal,
                                                         (uint8_t)(group_ordinal % 3U), group_state, 0, group_expanded);

            for (task_ordinal = 0; task_ordinal < task_count; task_ordinal++)
            {
                uint8_t state = (uint8_t)((root_ordinal * 5U + group_ordinal * 3U + task_ordinal) % TREE_DEMO_STATE_COUNT);
                uint8_t variant = (uint8_t)((task_ordinal + group_ordinal) % TREE_DEMO_TASK_VARIANT_COUNT);
                uint8_t progress = (uint8_t)(20U + ((root_ordinal * 13U + group_ordinal * 9U + task_ordinal * 11U) % 71U));

                if (variant == TREE_DEMO_TASK_VARIANT_ALERT && state == TREE_DEMO_STATE_IDLE)
                {
                    state = TREE_DEMO_STATE_WARN;
                }
                if (state == TREE_DEMO_STATE_DONE)
                {
                    progress = 100U;
                }

                (void)tree_demo_append_node(TREE_DEMO_NODE_KIND_TASK, group_index, root_ordinal, group_ordinal, task_ordinal, variant, state, progress, 0);
            }
        }
    }

    tree_demo_ctx.jump_cursor = 0;
    snprintf(tree_demo_ctx.last_action_text, sizeof(tree_demo_ctx.last_action_text), "tap branch or task");
}

static egui_dim_t tree_demo_get_view_width(void)
{
    egui_dim_t width = EGUI_VIEW_OF(&tree_view)->region.size.width;

    return width > 0 ? width : TREE_DEMO_VIEW_W;
}

static int tree_demo_get_view_pool_index(tree_demo_node_view_t *node_view)
{
    uint8_t i;

    for (i = 0; i < tree_demo_ctx.created_count; i++)
    {
        if (node_view == &tree_demo_ctx.node_views[i])
        {
            return (int)i;
        }
    }

    return -1;
}

static tree_demo_node_view_t *tree_demo_find_view_by_card(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < tree_demo_ctx.created_count; i++)
    {
        if (view == EGUI_VIEW_OF(&tree_demo_ctx.node_views[i].card))
        {
            return &tree_demo_ctx.node_views[i];
        }
    }

    return NULL;
}

static egui_background_t *tree_demo_get_badge_background(uint8_t selected, uint8_t state)
{
    if (selected)
    {
        return EGUI_BG_OF(&tree_demo_badge_selected_bg);
    }

    switch (state)
    {
    case TREE_DEMO_STATE_LIVE:
        return EGUI_BG_OF(&tree_demo_badge_live_bg);
    case TREE_DEMO_STATE_WARN:
        return EGUI_BG_OF(&tree_demo_badge_warn_bg);
    case TREE_DEMO_STATE_DONE:
        return EGUI_BG_OF(&tree_demo_badge_done_bg);
    default:
        return EGUI_BG_OF(&tree_demo_badge_idle_bg);
    }
}

static void tree_demo_set_node_pulse(tree_demo_node_view_t *node_view, const tree_demo_node_t *node, uint8_t visible, uint8_t selected)
{
    egui_background_t *pulse_bg;

    if (!visible || node == NULL || (!selected && !tree_demo_is_hot_state(node->state)))
    {
        if (node_view->pulse_running)
        {
            egui_animation_stop(EGUI_ANIM_OF(&node_view->pulse_anim));
            node_view->pulse_running = 0;
        }
        egui_view_set_gone(EGUI_VIEW_OF(&node_view->pulse), 1);
        egui_view_set_alpha(EGUI_VIEW_OF(&node_view->pulse), EGUI_ALPHA_100);
        return;
    }

    if (selected)
    {
        pulse_bg = EGUI_BG_OF(&tree_demo_pulse_selected_bg);
    }
    else if (node->state == TREE_DEMO_STATE_WARN)
    {
        pulse_bg = EGUI_BG_OF(&tree_demo_pulse_warn_bg);
    }
    else if (node->state == TREE_DEMO_STATE_DONE)
    {
        pulse_bg = EGUI_BG_OF(&tree_demo_pulse_done_bg);
    }
    else
    {
        pulse_bg = EGUI_BG_OF(&tree_demo_pulse_live_bg);
    }

    egui_view_set_gone(EGUI_VIEW_OF(&node_view->pulse), 0);
    egui_view_set_background(EGUI_VIEW_OF(&node_view->pulse), pulse_bg);
    if (!node_view->pulse_running)
    {
        egui_animation_start(EGUI_ANIM_OF(&node_view->pulse_anim));
        node_view->pulse_running = 1;
    }
}

static void tree_demo_capture_view_state(tree_demo_node_view_t *node_view, tree_demo_node_state_t *state)
{
    egui_animation_t *anim = EGUI_ANIM_OF(&node_view->pulse_anim);

    memset(state, 0, sizeof(*state));
    state->pulse_running = node_view->pulse_running ? 1U : 0U;
    state->pulse_alpha = EGUI_VIEW_OF(&node_view->pulse)->alpha;

    if (!node_view->pulse_running)
    {
        return;
    }

    state->pulse_cycle_flip = anim->is_cycle_flip ? 1U : 0U;
    state->pulse_repeated = (uint8_t)anim->repeated;

    if (anim->start_time != (uint32_t)-1 && anim->duration > 0)
    {
        uint32_t elapsed_ms = egui_api_timer_get_current() - anim->start_time;

        if (elapsed_ms >= anim->duration)
        {
            elapsed_ms = anim->duration > 1U ? (uint32_t)anim->duration - 1U : 0U;
        }

        state->pulse_elapsed_ms = (uint16_t)elapsed_ms;
    }
}

static void tree_demo_restore_view_state(tree_demo_node_view_t *node_view, const tree_demo_node_state_t *state)
{
    egui_animation_t *anim;

    if (state == NULL || !state->pulse_running)
    {
        return;
    }

    anim = EGUI_ANIM_OF(&node_view->pulse_anim);
    if (!anim->is_running)
    {
        egui_animation_start(anim);
    }

    node_view->pulse_running = 1;
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->pulse), 0);
    egui_view_set_alpha(EGUI_VIEW_OF(&node_view->pulse), state->pulse_alpha);
    anim->is_started = 1;
    anim->is_ended = 0;
    anim->is_cycle_flip = state->pulse_cycle_flip ? 1U : 0U;
    anim->repeated = (int8_t)state->pulse_repeated;
    anim->start_time = egui_api_timer_get_current() - state->pulse_elapsed_ms;
}

static void tree_demo_format_branch_texts(int pool_index, const egui_view_virtual_tree_entry_t *entry, const tree_demo_node_t *node)
{
    uint16_t branch_count = 0;
    uint16_t task_count = 0;
    uint16_t hot_count = 0;

    tree_demo_collect_branch_metrics((uint16_t)(node - tree_demo_ctx.nodes), &branch_count, &task_count, &hot_count);

    if (node->kind == TREE_DEMO_NODE_KIND_ROOT)
    {
        snprintf(tree_demo_ctx.title_texts[pool_index], sizeof(tree_demo_ctx.title_texts[pool_index]), "%s Hub %u",
                 tree_demo_root_names[node->root_ordinal % TREE_DEMO_ROOT_COUNT], (unsigned)(node->root_ordinal + 1U));
        snprintf(tree_demo_ctx.body_texts[pool_index], sizeof(tree_demo_ctx.body_texts[pool_index]), "%u lanes | %u hot", (unsigned)node->child_count,
                 (unsigned)hot_count);
        snprintf(tree_demo_ctx.meta_texts[pool_index], sizeof(tree_demo_ctx.meta_texts[pool_index]), "%s  #%05lu  r%u", node->expanded ? "open" : "fold",
                 (unsigned long)(node->stable_id % 100000UL), (unsigned)node->revision);
    }
    else
    {
        snprintf(tree_demo_ctx.title_texts[pool_index], sizeof(tree_demo_ctx.title_texts[pool_index]), "Lane %u.%u",
                 (unsigned)(node->root_ordinal + 1U), (unsigned)(node->group_ordinal + 1U));
        snprintf(tree_demo_ctx.body_texts[pool_index], sizeof(tree_demo_ctx.body_texts[pool_index]), "%u tasks | %u hot", (unsigned)task_count,
                 (unsigned)hot_count);
        snprintf(tree_demo_ctx.meta_texts[pool_index], sizeof(tree_demo_ctx.meta_texts[pool_index]), "%s  d%u  r%u", node->expanded ? "open" : "fold",
                 (unsigned)entry->depth, (unsigned)node->revision);
    }

    snprintf(tree_demo_ctx.badge_texts[pool_index], sizeof(tree_demo_ctx.badge_texts[pool_index]), "%s", tree_demo_state_names[node->state]);
}

static void tree_demo_format_task_texts(int pool_index, const egui_view_virtual_tree_entry_t *entry, const tree_demo_node_t *node)
{
    snprintf(tree_demo_ctx.title_texts[pool_index], sizeof(tree_demo_ctx.title_texts[pool_index]), "Leaf %u.%u.%u", (unsigned)(node->root_ordinal + 1U),
             (unsigned)(node->group_ordinal + 1U), (unsigned)(node->ordinal + 1U));
    snprintf(tree_demo_ctx.body_texts[pool_index], sizeof(tree_demo_ctx.body_texts[pool_index]), "%s", tree_demo_task_notes[node->variant]);
    snprintf(tree_demo_ctx.meta_texts[pool_index], sizeof(tree_demo_ctx.meta_texts[pool_index]), "%u%%  d%u  r%u", (unsigned)node->progress,
             (unsigned)entry->depth, (unsigned)node->revision);
    snprintf(tree_demo_ctx.badge_texts[pool_index], sizeof(tree_demo_ctx.badge_texts[pool_index]), "%s", tree_demo_state_names[node->state]);
}

static void tree_demo_bind_node_card(tree_demo_node_view_t *node_view, int pool_index, const egui_view_virtual_tree_entry_t *entry,
                                     const tree_demo_node_t *node, uint8_t selected)
{
    int32_t node_height = tree_demo_measure_node_height_with_state(node, selected);
    egui_dim_t view_width = tree_demo_get_view_width();
    egui_dim_t indent = TREE_DEMO_NODE_INSET_X + entry->depth * TREE_DEMO_INDENT_STEP;
    egui_dim_t card_x = indent;
    egui_dim_t card_width = view_width - card_x - TREE_DEMO_NODE_INSET_X;
    egui_dim_t card_height = node_height - TREE_DEMO_NODE_GAP_Y;
    egui_dim_t corner_radius = 10;
    egui_dim_t border_width = 1;
    egui_color_t badge_text_color;
    egui_color_t card_color;
    egui_color_t border_color;
    egui_color_t title_color;
    egui_color_t body_color;
    egui_color_t meta_color;
    egui_color_t guide_color = EGUI_COLOR_HEX(0xD6E0E8);
    egui_color_t marker_color = EGUI_COLOR_HEX(0x8EA1B2);
    egui_color_t connector_color = EGUI_COLOR_HEX(0xD6E0E8);
    const egui_shadow_t *card_shadow = NULL;
    uint8_t show_body = 1;
    uint8_t show_meta = 1;
    uint8_t show_guide = 0;
    uint8_t show_connector = 0;
    uint8_t show_marker = 0;
    uint8_t show_badge = 1;
    uint8_t show_progress = 0;
    egui_dim_t badge_x;
    egui_dim_t text_limit_x;
    egui_dim_t title_x = 20;
    egui_dim_t title_y = 8;
    egui_dim_t title_w;
    egui_dim_t body_x = 20;
    egui_dim_t body_y = 24;
    egui_dim_t body_w;
    egui_dim_t meta_x = 20;
    egui_dim_t meta_y = 38;
    egui_dim_t meta_w;
    egui_dim_t progress_x = 20;
    egui_dim_t progress_y = 0;
    egui_dim_t progress_w = 0;
    egui_dim_t guide_x = 0;
    egui_dim_t guide_y = 0;
    egui_dim_t guide_w = 2;
    egui_dim_t guide_h = 10;
    egui_dim_t connector_x = 0;
    egui_dim_t connector_y = 0;
    egui_dim_t connector_w = 10;
    egui_dim_t connector_h = 2;
    egui_dim_t marker_x = 0;
    egui_dim_t marker_y = 0;
    egui_dim_t marker_size = 8;
    egui_dim_t pulse_x = 0;
    egui_dim_t pulse_y = 0;

    if (card_width < 80)
    {
        card_width = 80;
    }

    if (node->kind != TREE_DEMO_NODE_KIND_TASK)
    {
        egui_view_label_set_font(EGUI_VIEW_OF(&node_view->title), node->kind == TREE_DEMO_NODE_KIND_ROOT ? TREE_DEMO_FONT_HEADER : TREE_DEMO_FONT_TITLE);
        egui_view_label_set_font(EGUI_VIEW_OF(&node_view->body), TREE_DEMO_FONT_CAP);
        egui_view_label_set_font(EGUI_VIEW_OF(&node_view->meta), TREE_DEMO_FONT_CAP);
        tree_demo_format_branch_texts(pool_index, entry, node);
        if (node->kind == TREE_DEMO_NODE_KIND_ROOT)
        {
            card_x = 8;
            card_width = view_width - 14;
            corner_radius = 15;
            card_shadow = &tree_demo_card_shadow;
            title_color = EGUI_COLOR_WHITE;
            body_color = EGUI_COLOR_HEX(0xE3EEF8);
            meta_color = EGUI_COLOR_HEX(0xC6D9EA);
            card_color = EGUI_COLOR_HEX(0x355B82);
            border_color = EGUI_COLOR_HEX(0x4C769E);
            marker_color = EGUI_COLOR_WHITE;
            connector_color = marker_color;
            show_marker = 1;
            marker_x = 12;
            marker_y = 12;
            marker_size = 10;
            title_x = 28;
            title_y = 12;
            body_x = 28;
            body_y = 34;
            meta_x = 28;
            meta_y = card_height - 22;
        }
        else
        {
            card_x = indent + 4;
            card_width = view_width - card_x - 16;
            corner_radius = 12;
            title_color = EGUI_COLOR_HEX(0x244155);
            body_color = EGUI_COLOR_HEX(0x536C81);
            meta_color = EGUI_COLOR_HEX(0x70859A);
            card_color = EGUI_COLOR_HEX(0xF7FBFE);
            border_color = EGUI_COLOR_HEX(0xC9D8E4);
            guide_color = EGUI_COLOR_HEX(0x87AACC);
            connector_color = guide_color;
            marker_color = node->state == TREE_DEMO_STATE_WARN ? EGUI_COLOR_HEX(0xD08A2E) : EGUI_COLOR_HEX(0x6089B3);
            show_guide = 1;
            show_connector = 1;
            show_marker = 1;
            guide_x = 10;
            guide_y = 6;
            guide_w = 3;
            guide_h = card_height - 12;
            connector_x = 10;
            connector_y = 17;
            connector_w = 16;
            connector_h = 3;
            marker_x = 24;
            marker_y = 13;
            marker_size = 8;
            title_x = 36;
            title_y = 8;
            body_x = 36;
            body_y = 26;
            meta_x = 36;
            meta_y = card_height - 18;
        }
        show_progress = 0;
    }
    else
    {
        egui_view_label_set_font(EGUI_VIEW_OF(&node_view->title), TREE_DEMO_FONT_BODY);
        egui_view_label_set_font(EGUI_VIEW_OF(&node_view->body), TREE_DEMO_FONT_CAP);
        egui_view_label_set_font(EGUI_VIEW_OF(&node_view->meta), TREE_DEMO_FONT_CAP);
        tree_demo_format_task_texts(pool_index, entry, node);
        card_x = (egui_dim_t)(indent + (node->variant == TREE_DEMO_TASK_VARIANT_COMPACT ? 12 : 10));
        card_width = view_width - card_x - (node->variant == TREE_DEMO_TASK_VARIANT_COMPACT ? 34 : (node->variant == TREE_DEMO_TASK_VARIANT_DETAIL ? 20 : 14));
        title_color = EGUI_COLOR_HEX(0x233445);
        body_color = EGUI_COLOR_HEX(0x4E6275);
        meta_color = EGUI_COLOR_HEX(0x687B8D);
        guide_color = node->state == TREE_DEMO_STATE_LIVE   ? EGUI_COLOR_HEX(0x62A886)
                      : node->state == TREE_DEMO_STATE_DONE ? EGUI_COLOR_HEX(0x7A95B1)
                                                            : node->state == TREE_DEMO_STATE_WARN ? EGUI_COLOR_HEX(0xD8A24A)
                                                                                                : EGUI_COLOR_HEX(0xCAD6E0);
        connector_color = guide_color;
        marker_color = node->state == TREE_DEMO_STATE_LIVE   ? EGUI_COLOR_HEX(0x2E9A6F)
                       : node->state == TREE_DEMO_STATE_WARN ? EGUI_COLOR_HEX(0xD08A2E)
                       : node->state == TREE_DEMO_STATE_DONE ? EGUI_COLOR_HEX(0x6D88A7)
                                                             : EGUI_COLOR_HEX(0x98A7B4);
        show_guide = 1;
        show_connector = 1;
        show_marker = 1;
        guide_x = 6;
        guide_y = 0;
        guide_w = selected ? 3 : 2;
        guide_h = card_height;
        connector_x = 6;
        connector_y = 13;
        connector_w = 14;
        connector_h = 2;
        marker_x = 18;
        marker_y = 10;
        marker_size = node->variant == TREE_DEMO_TASK_VARIANT_COMPACT ? 7 : 8;
        title_x = 28;
        body_x = 28;
        meta_x = 28;
        progress_x = 28;

        if (node->variant == TREE_DEMO_TASK_VARIANT_COMPACT)
        {
            corner_radius = 8;
            card_color = EGUI_COLOR_HEX(0xFBFDFF);
            border_color = EGUI_COLOR_HEX(0xD8E3EB);
            show_body = selected ? 1U : 0U;
            show_meta = 0;
            show_badge = (uint8_t)(selected || node->state != TREE_DEMO_STATE_IDLE);
            show_progress = (uint8_t)(selected || tree_demo_is_hot_state(node->state));
            title_y = show_body ? 7 : 10;
            body_y = 22;
            meta_y = 0;
            progress_y = card_height - 8;
            progress_w = card_width - progress_x - 10;
            marker_y = show_body ? 13 : 10;
        }
        else if (node->variant == TREE_DEMO_TASK_VARIANT_DETAIL)
        {
            corner_radius = 10;
            card_color = EGUI_COLOR_WHITE;
            border_color = EGUI_COLOR_HEX(0xDCE6EE);
            show_body = 1;
            show_meta = 1;
            show_badge = 1;
            show_progress = 1;
            title_y = 8;
            body_y = 24;
            meta_y = 32;
            progress_y = card_height - 7;
            progress_w = card_width - progress_x - 10;
        }
        else
        {
            corner_radius = 12;
            card_color = EGUI_COLOR_HEX(0xFFF8EF);
            border_color = EGUI_COLOR_HEX(0xE1BE7A);
            title_color = EGUI_COLOR_HEX(0x4D4026);
            body_color = EGUI_COLOR_HEX(0x8A6A31);
            meta_color = EGUI_COLOR_HEX(0x8C7347);
            guide_color = EGUI_COLOR_HEX(0xD8A24A);
            connector_color = guide_color;
            marker_color = EGUI_COLOR_HEX(0xD08A2E);
            show_body = 1;
            show_meta = 1;
            show_badge = 1;
            show_progress = 1;
            guide_w = 3;
            connector_h = 3;
            title_y = 10;
            body_y = 28;
            meta_y = 42;
            progress_y = card_height - 7;
            progress_w = card_width - progress_x - 10;
        }
    }

    if (card_width < 112)
    {
        card_width = 112;
    }

    if (selected)
    {
        border_width = 2;
        if (node->kind == TREE_DEMO_NODE_KIND_ROOT)
        {
            card_color = EGUI_COLOR_HEX(0x2F5E8E);
            border_color = EGUI_COLOR_WHITE;
            title_color = EGUI_COLOR_WHITE;
            body_color = EGUI_COLOR_HEX(0xEAF4FC);
            meta_color = EGUI_COLOR_HEX(0xD7E6F4);
            guide_color = EGUI_COLOR_HEX(0xAFC7DE);
            connector_color = guide_color;
            marker_color = EGUI_COLOR_WHITE;
        }
        else if (node->kind == TREE_DEMO_NODE_KIND_GROUP)
        {
            card_color = EGUI_COLOR_HEX(0xF7FBFF);
            border_color = EGUI_COLOR_HEX(0x3A6EA5);
            title_color = EGUI_COLOR_HEX(0x23435B);
            body_color = EGUI_COLOR_HEX(0x456176);
            meta_color = EGUI_COLOR_HEX(0x5E7890);
            guide_color = EGUI_COLOR_HEX(0x5F89B3);
            connector_color = guide_color;
            marker_color = EGUI_COLOR_HEX(0x3A6EA5);
        }
        else
        {
            card_color = node->variant == TREE_DEMO_TASK_VARIANT_ALERT ? EGUI_COLOR_HEX(0xFFF7EA) : EGUI_COLOR_HEX(0xF7FBFF);
            border_color = EGUI_COLOR_HEX(0x3A6EA5);
            title_color = EGUI_COLOR_HEX(0x23435B);
            body_color = EGUI_COLOR_HEX(0x456176);
            meta_color = EGUI_COLOR_HEX(0x5E7890);
            guide_color = EGUI_COLOR_HEX(0x5F89B3);
            connector_color = guide_color;
            marker_color = node->variant == TREE_DEMO_TASK_VARIANT_ALERT ? EGUI_COLOR_HEX(0xD08A2E) : EGUI_COLOR_HEX(0x3A6EA5);
        }
    }

    badge_x = card_width - TREE_DEMO_BADGE_W - TREE_DEMO_NODE_PAD_X;
    text_limit_x = show_badge ? (egui_dim_t)(badge_x - 8) : (egui_dim_t)(card_width - TREE_DEMO_NODE_PAD_X);
    title_w = text_limit_x - title_x;
    body_w = text_limit_x - body_x;
    meta_w = text_limit_x - meta_x;
    if (title_w < 56)
    {
        title_w = 56;
    }
    if (body_w < 52)
    {
        body_w = 52;
    }
    if (meta_w < 52)
    {
        meta_w = 52;
    }

    if (show_progress)
    {
        if (progress_w == 0)
        {
            progress_w = card_width - progress_x - 10;
        }
        if (progress_w < 48)
        {
            progress_w = 48;
        }
    }
    else
    {
        progress_w = 48;
        progress_y = card_height - TREE_DEMO_PROGRESS_H;
    }

    egui_view_set_position(EGUI_VIEW_OF(&node_view->card), card_x, TREE_DEMO_NODE_GAP_Y / 2);
    egui_view_set_size(EGUI_VIEW_OF(&node_view->card), card_width, card_height);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&node_view->card), corner_radius);
    egui_view_set_shadow(EGUI_VIEW_OF(&node_view->card), card_shadow);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&node_view->card), card_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&node_view->card), border_width, border_color);

    egui_view_set_size(EGUI_VIEW_OF(&node_view->title), title_w, TREE_DEMO_LABEL_H);
    egui_view_set_size(EGUI_VIEW_OF(&node_view->body), body_w, TREE_DEMO_LABEL_H);
    egui_view_set_size(EGUI_VIEW_OF(&node_view->meta), meta_w, TREE_DEMO_LABEL_H);
    egui_view_set_position(EGUI_VIEW_OF(&node_view->title), title_x, title_y);
    egui_view_set_position(EGUI_VIEW_OF(&node_view->body), body_x, body_y);
    egui_view_set_position(EGUI_VIEW_OF(&node_view->meta), meta_x, meta_y);

    badge_text_color = selected ? EGUI_COLOR_HEX(0x2B3F52) : (node->state == TREE_DEMO_STATE_WARN ? EGUI_COLOR_BLACK : EGUI_COLOR_WHITE);
    egui_view_label_set_text(EGUI_VIEW_OF(&node_view->title), tree_demo_ctx.title_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&node_view->body), tree_demo_ctx.body_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&node_view->meta), tree_demo_ctx.meta_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&node_view->badge), tree_demo_ctx.badge_texts[pool_index]);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&node_view->title), title_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&node_view->body), body_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&node_view->meta), meta_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&node_view->badge), badge_text_color, EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&node_view->badge), tree_demo_get_badge_background(selected, node->state));
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&node_view->guide), guide_color, EGUI_ALPHA_100);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&node_view->connector), connector_color, EGUI_ALPHA_100);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&node_view->marker), marker_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&node_view->guide), 0, guide_color);
    egui_view_card_set_border(EGUI_VIEW_OF(&node_view->connector), 0, connector_color);
    egui_view_card_set_border(EGUI_VIEW_OF(&node_view->marker), 0, marker_color);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&node_view->guide), guide_w > 2 ? 2 : 1);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&node_view->connector), connector_h > 2 ? 2 : 1);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&node_view->marker), marker_size / 2);
    egui_view_set_shadow(EGUI_VIEW_OF(&node_view->guide), NULL);
    egui_view_set_shadow(EGUI_VIEW_OF(&node_view->connector), NULL);
    egui_view_set_shadow(EGUI_VIEW_OF(&node_view->marker), NULL);
    egui_view_set_position(EGUI_VIEW_OF(&node_view->guide), guide_x, guide_y);
    egui_view_set_size(EGUI_VIEW_OF(&node_view->guide), guide_w, guide_h);
    egui_view_set_position(EGUI_VIEW_OF(&node_view->connector), connector_x, connector_y);
    egui_view_set_size(EGUI_VIEW_OF(&node_view->connector), connector_w, connector_h);
    egui_view_set_position(EGUI_VIEW_OF(&node_view->marker), marker_x, marker_y);
    egui_view_set_size(EGUI_VIEW_OF(&node_view->marker), marker_size, marker_size);
    egui_view_set_position(EGUI_VIEW_OF(&node_view->badge), badge_x, 8);
    egui_view_set_size(EGUI_VIEW_OF(&node_view->badge), TREE_DEMO_BADGE_W, TREE_DEMO_BADGE_H);
    if (node->kind == TREE_DEMO_NODE_KIND_ROOT)
    {
        pulse_x = badge_x - 12;
        pulse_y = card_height - 14;
    }
    else
    {
        pulse_x = card_width - 14;
        pulse_y = card_height - 14;
    }
    egui_view_set_position(EGUI_VIEW_OF(&node_view->pulse), pulse_x, pulse_y);
    egui_view_set_size(EGUI_VIEW_OF(&node_view->pulse), 8, 8);
    egui_view_set_position(EGUI_VIEW_OF(&node_view->progress), progress_x, progress_y);
    egui_view_set_size(EGUI_VIEW_OF(&node_view->progress), progress_w, TREE_DEMO_PROGRESS_H);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&node_view->progress), node->progress);
    node_view->progress.progress_color = selected ? EGUI_COLOR_HEX(0x3A6EA5)
                                       : node->state == TREE_DEMO_STATE_WARN ? EGUI_COLOR_HEX(0xD08A2E)
                                       : node->state == TREE_DEMO_STATE_LIVE ? EGUI_COLOR_HEX(0x2E9A6F)
                                       : node->state == TREE_DEMO_STATE_DONE ? EGUI_COLOR_HEX(0x6D88A7)
                                                                             : EGUI_COLOR_HEX(0x5B7FA0);
    node_view->progress.bk_color = selected ? EGUI_COLOR_HEX(0xC8D8E6) : EGUI_COLOR_HEX(0xDCE6EE);
    node_view->progress.control_color = node_view->progress.progress_color;
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->badge), show_badge ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->guide), show_guide ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->connector), show_connector ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->marker), show_marker ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->body), show_body ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->meta), show_meta ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->progress), show_progress ? 0 : 1);

    tree_demo_set_node_pulse(node_view, node, 1, selected);
}

static void tree_demo_refresh_status(void)
{
    snprintf(tree_demo_ctx.header_title_text, sizeof(tree_demo_ctx.header_title_text), "Virtual Tree Demo");
    snprintf(tree_demo_ctx.header_detail_text, sizeof(tree_demo_ctx.header_detail_text), "%u roots | %u all | %lu vis", (unsigned)TREE_DEMO_ROOT_COUNT,
             (unsigned)tree_demo_ctx.node_count, (unsigned long)tree_demo_get_visible_count());

    if (tree_demo_ctx.selected_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        snprintf(tree_demo_ctx.header_hint_text, sizeof(tree_demo_ctx.header_hint_text), "%s", tree_demo_ctx.last_action_text);
    }
    else
    {
        snprintf(tree_demo_ctx.header_hint_text, sizeof(tree_demo_ctx.header_hint_text), "sel %05lu | %s",
                 (unsigned long)(tree_demo_ctx.selected_id % 100000UL), tree_demo_ctx.last_action_text);
    }

    if (EGUI_VIEW_OF(&header_title)->api == NULL)
    {
        return;
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&header_title), tree_demo_ctx.header_title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_detail), tree_demo_ctx.header_detail_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_hint), tree_demo_ctx.header_hint_text);
}

static void tree_demo_update_selection(uint32_t stable_id, uint8_t notify_resize, uint8_t ensure_visible)
{
    uint32_t previous_id = tree_demo_ctx.selected_id;

    tree_demo_ctx.selected_id = stable_id;
    if (notify_resize)
    {
        if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && previous_id != stable_id)
        {
            egui_view_virtual_tree_notify_node_resized_by_stable_id(EGUI_VIEW_OF(&tree_view), previous_id);
        }
        if (stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
        {
            egui_view_virtual_tree_notify_node_resized_by_stable_id(EGUI_VIEW_OF(&tree_view), stable_id);
        }
    }
    if (ensure_visible && stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&tree_view), stable_id, TREE_DEMO_NODE_GAP_Y / 2);
    }
}

static void tree_demo_node_click_cb(egui_view_t *self)
{
    egui_view_virtual_tree_entry_t entry;
    tree_demo_node_t *node;

    if (!egui_view_virtual_tree_resolve_node_by_view(EGUI_VIEW_OF(&tree_view), self, &entry))
    {
        return;
    }

    node = tree_demo_get_node_by_stable_id(entry.stable_id);
    if (node == NULL)
    {
        return;
    }

    tree_demo_ctx.click_count++;
    tree_demo_ctx.last_clicked_id = entry.stable_id;
    tree_demo_ctx.last_clicked_visible_index = entry.visible_index;

    if (tree_demo_is_branch(node))
    {
        node->expanded = node->expanded ? 0U : 1U;
        tree_demo_ctx.selected_id = entry.stable_id;
        egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&tree_view));
    }
    else
    {
        tree_demo_update_selection(entry.stable_id, 1, 1);
    }

    snprintf(tree_demo_ctx.last_action_text, sizeof(tree_demo_ctx.last_action_text), "click %05lu @%lu", (unsigned long)(entry.stable_id % 100000UL),
             (unsigned long)entry.visible_index);
    tree_demo_refresh_status();
}

static uint32_t tree_demo_get_root_count_cb(void *data_source_context)
{
    EGUI_UNUSED(data_source_context);
    return TREE_DEMO_ROOT_COUNT;
}

static uint32_t tree_demo_get_root_stable_id_cb(void *data_source_context, uint32_t root_index)
{
    tree_demo_context_t *ctx = (tree_demo_context_t *)data_source_context;

    if (root_index >= TREE_DEMO_ROOT_COUNT)
    {
        return EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    return ctx->nodes[ctx->root_indices[root_index]].stable_id;
}

static uint32_t tree_demo_get_child_count_cb(void *data_source_context, uint32_t stable_id)
{
    tree_demo_context_t *ctx = (tree_demo_context_t *)data_source_context;
    int32_t node_index = (int32_t)(stable_id - TREE_DEMO_NODE_ID_BASE);

    if (node_index < 0 || node_index >= ctx->node_count)
    {
        return 0;
    }

    return ctx->nodes[node_index].child_count;
}

static uint32_t tree_demo_get_child_stable_id_cb(void *data_source_context, uint32_t stable_id, uint32_t child_index)
{
    tree_demo_context_t *ctx = (tree_demo_context_t *)data_source_context;
    int32_t node_index = (int32_t)(stable_id - TREE_DEMO_NODE_ID_BASE);

    if (node_index < 0 || node_index >= ctx->node_count || child_index >= ctx->nodes[node_index].child_count)
    {
        return EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    return ctx->nodes[ctx->nodes[node_index].child_indices[child_index]].stable_id;
}

static uint8_t tree_demo_is_node_expanded_cb(void *data_source_context, uint32_t stable_id)
{
    tree_demo_context_t *ctx = (tree_demo_context_t *)data_source_context;
    int32_t node_index = (int32_t)(stable_id - TREE_DEMO_NODE_ID_BASE);

    if (node_index < 0 || node_index >= ctx->node_count)
    {
        return 0;
    }

    return ctx->nodes[node_index].expanded;
}

static uint16_t tree_demo_get_node_view_type_cb(void *data_source_context, const egui_view_virtual_tree_entry_t *entry)
{
    tree_demo_context_t *ctx = (tree_demo_context_t *)data_source_context;
    int32_t node_index = (int32_t)(entry->stable_id - TREE_DEMO_NODE_ID_BASE);

    if (node_index < 0 || node_index >= ctx->node_count)
    {
        return TREE_DEMO_VIEW_TYPE_BRANCH;
    }

    return ctx->nodes[node_index].kind == TREE_DEMO_NODE_KIND_TASK ? TREE_DEMO_VIEW_TYPE_TASK : TREE_DEMO_VIEW_TYPE_BRANCH;
}

static int32_t tree_demo_measure_node_height_cb(void *data_source_context, const egui_view_virtual_tree_entry_t *entry, int32_t width_hint)
{
    tree_demo_context_t *ctx = (tree_demo_context_t *)data_source_context;
    int32_t node_index = (int32_t)(entry->stable_id - TREE_DEMO_NODE_ID_BASE);

    EGUI_UNUSED(width_hint);

    if (node_index < 0 || node_index >= ctx->node_count)
    {
        return 56;
    }

    return tree_demo_measure_node_height_with_state(&ctx->nodes[node_index], tree_demo_is_selected_id(entry->stable_id));
}

static egui_view_t *tree_demo_create_node_view_cb(void *data_source_context, uint16_t view_type)
{
    tree_demo_context_t *ctx = (tree_demo_context_t *)data_source_context;
    tree_demo_node_view_t *node_view;

    EGUI_UNUSED(view_type);

    if (ctx->created_count >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
    {
        return NULL;
    }

    node_view = &ctx->node_views[ctx->created_count];
    memset(node_view, 0, sizeof(*node_view));

    egui_view_card_init(EGUI_VIEW_OF(&node_view->card));
    egui_view_card_init(EGUI_VIEW_OF(&node_view->guide));
    egui_view_card_init(EGUI_VIEW_OF(&node_view->connector));
    egui_view_card_init(EGUI_VIEW_OF(&node_view->marker));
    egui_view_label_init(EGUI_VIEW_OF(&node_view->title));
    egui_view_label_init(EGUI_VIEW_OF(&node_view->body));
    egui_view_label_init(EGUI_VIEW_OF(&node_view->meta));
    egui_view_label_init(EGUI_VIEW_OF(&node_view->badge));
    egui_view_progress_bar_init(EGUI_VIEW_OF(&node_view->progress));
    egui_view_init(EGUI_VIEW_OF(&node_view->pulse));

    egui_view_label_set_font(EGUI_VIEW_OF(&node_view->title), TREE_DEMO_FONT_TITLE);
    egui_view_label_set_font(EGUI_VIEW_OF(&node_view->body), TREE_DEMO_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&node_view->meta), TREE_DEMO_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&node_view->badge), TREE_DEMO_FONT_CAP);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&node_view->title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&node_view->body), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&node_view->meta), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&node_view->badge), EGUI_ALIGN_CENTER);

    egui_view_card_add_child(EGUI_VIEW_OF(&node_view->card), EGUI_VIEW_OF(&node_view->guide));
    egui_view_card_add_child(EGUI_VIEW_OF(&node_view->card), EGUI_VIEW_OF(&node_view->connector));
    egui_view_card_add_child(EGUI_VIEW_OF(&node_view->card), EGUI_VIEW_OF(&node_view->marker));
    egui_view_card_add_child(EGUI_VIEW_OF(&node_view->card), EGUI_VIEW_OF(&node_view->title));
    egui_view_card_add_child(EGUI_VIEW_OF(&node_view->card), EGUI_VIEW_OF(&node_view->body));
    egui_view_card_add_child(EGUI_VIEW_OF(&node_view->card), EGUI_VIEW_OF(&node_view->meta));
    egui_view_card_add_child(EGUI_VIEW_OF(&node_view->card), EGUI_VIEW_OF(&node_view->badge));
    egui_view_card_add_child(EGUI_VIEW_OF(&node_view->card), EGUI_VIEW_OF(&node_view->progress));
    egui_view_card_add_child(EGUI_VIEW_OF(&node_view->card), EGUI_VIEW_OF(&node_view->pulse));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&node_view->card), tree_demo_node_click_cb);
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->pulse), 1);
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->guide), 1);
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->connector), 1);
    egui_view_set_gone(EGUI_VIEW_OF(&node_view->marker), 1);

    egui_animation_alpha_init(EGUI_ANIM_OF(&node_view->pulse_anim));
    egui_animation_alpha_params_set(&node_view->pulse_anim, &tree_demo_pulse_anim_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&node_view->pulse_anim), 900);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&node_view->pulse_anim), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&node_view->pulse_anim), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_interpolator_linear_init((egui_interpolator_t *)&node_view->pulse_interp);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&node_view->pulse_anim), (egui_interpolator_t *)&node_view->pulse_interp);
    egui_animation_target_view_set(EGUI_ANIM_OF(&node_view->pulse_anim), EGUI_VIEW_OF(&node_view->pulse));

    ctx->created_count++;
    return EGUI_VIEW_OF(&node_view->card);
}

static void tree_demo_bind_node_view_cb(void *data_source_context, egui_view_t *view, const egui_view_virtual_tree_entry_t *entry)
{
    tree_demo_node_view_t *node_view = tree_demo_find_view_by_card(view);
    int pool_index;
    const tree_demo_node_t *node;

    EGUI_UNUSED(data_source_context);

    if (node_view == NULL)
    {
        return;
    }

    pool_index = tree_demo_get_view_pool_index(node_view);
    if (pool_index < 0)
    {
        return;
    }

    node = tree_demo_get_node_const_by_index((uint16_t)(entry->stable_id - TREE_DEMO_NODE_ID_BASE));
    if (node == NULL)
    {
        return;
    }

    node_view->stable_id = entry->stable_id;
    tree_demo_bind_node_card(node_view, pool_index, entry, node, tree_demo_is_selected_id(entry->stable_id));
}

static void tree_demo_unbind_node_view_cb(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    tree_demo_node_view_t *node_view = tree_demo_find_view_by_card(view);

    EGUI_UNUSED(data_source_context);
    EGUI_UNUSED(stable_id);

    if (node_view == NULL)
    {
        return;
    }

    tree_demo_set_node_pulse(node_view, NULL, 0, 0);
    node_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static uint8_t tree_demo_should_keep_alive_cb(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    tree_demo_context_t *ctx = (tree_demo_context_t *)data_source_context;
    int32_t node_index = (int32_t)(stable_id - TREE_DEMO_NODE_ID_BASE);

    EGUI_UNUSED(view);

    if (stable_id == ctx->selected_id)
    {
        return 1;
    }
    if (node_index < 0 || node_index >= ctx->node_count)
    {
        return 0;
    }

    return tree_demo_is_hot_state(ctx->nodes[node_index].state);
}

static void tree_demo_save_node_state_cb(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    tree_demo_node_view_t *node_view = tree_demo_find_view_by_card(view);
    tree_demo_node_state_t state;

    EGUI_UNUSED(data_source_context);

    if (node_view == NULL)
    {
        return;
    }

    tree_demo_capture_view_state(node_view, &state);
    if (!state.pulse_running)
    {
        egui_view_virtual_tree_remove_node_state_by_stable_id(EGUI_VIEW_OF(&tree_view), stable_id);
        return;
    }

    (void)egui_view_virtual_tree_write_node_state_for_view(view, stable_id, &state, sizeof(state));
}

static void tree_demo_restore_node_state_cb(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    tree_demo_node_view_t *node_view = tree_demo_find_view_by_card(view);
    tree_demo_node_state_t state;

    EGUI_UNUSED(data_source_context);

    if (node_view == NULL)
    {
        return;
    }

    if (egui_view_virtual_tree_read_node_state_for_view(view, stable_id, &state, sizeof(state)) != sizeof(state))
    {
        return;
    }

    tree_demo_restore_view_state(node_view, &state);
}

static const egui_view_virtual_tree_data_source_t tree_demo_data_source = {
        .get_root_count = tree_demo_get_root_count_cb,
        .get_root_stable_id = tree_demo_get_root_stable_id_cb,
        .get_child_count = tree_demo_get_child_count_cb,
        .get_child_stable_id = tree_demo_get_child_stable_id_cb,
        .is_node_expanded = tree_demo_is_node_expanded_cb,
        .get_node_view_type = tree_demo_get_node_view_type_cb,
        .measure_node_height = tree_demo_measure_node_height_cb,
        .create_node_view = tree_demo_create_node_view_cb,
        .destroy_node_view = NULL,
        .bind_node_view = tree_demo_bind_node_view_cb,
        .unbind_node_view = tree_demo_unbind_node_view_cb,
        .should_keep_alive = tree_demo_should_keep_alive_cb,
        .save_node_state = tree_demo_save_node_state_cb,
        .restore_node_state = tree_demo_restore_node_state_cb,
        .default_view_type = TREE_DEMO_VIEW_TYPE_BRANCH,
};

static void tree_demo_expand_action(void)
{
    uint16_t target_index = tree_demo_get_selected_branch_target(0);
    tree_demo_node_t *node;

    if (target_index == TREE_DEMO_INVALID_NODE_INDEX)
    {
        target_index = tree_demo_find_next_branch(0, (uint16_t)(tree_demo_ctx.jump_cursor % tree_demo_ctx.node_count));
    }
    if (target_index == TREE_DEMO_INVALID_NODE_INDEX)
    {
        snprintf(tree_demo_ctx.last_action_text, sizeof(tree_demo_ctx.last_action_text), "expand ignored");
        tree_demo_refresh_status();
        return;
    }

    node = tree_demo_get_node_by_index(target_index);
    node->expanded = 1;
    tree_demo_ctx.selected_id = node->stable_id;
    tree_demo_ctx.action_count++;
    tree_demo_ctx.jump_cursor = target_index;
    egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&tree_view));
    egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id, TREE_DEMO_NODE_GAP_Y / 2);
    snprintf(tree_demo_ctx.last_action_text, sizeof(tree_demo_ctx.last_action_text), "expand %05lu", (unsigned long)(node->stable_id % 100000UL));
    tree_demo_refresh_status();
}

static void tree_demo_fold_action(void)
{
    uint16_t target_index = tree_demo_get_selected_branch_target(1);
    tree_demo_node_t *node;

    if (target_index == TREE_DEMO_INVALID_NODE_INDEX)
    {
        target_index = tree_demo_find_next_branch(1, (uint16_t)(tree_demo_ctx.jump_cursor % tree_demo_ctx.node_count));
    }
    if (target_index == TREE_DEMO_INVALID_NODE_INDEX)
    {
        snprintf(tree_demo_ctx.last_action_text, sizeof(tree_demo_ctx.last_action_text), "fold ignored");
        tree_demo_refresh_status();
        return;
    }

    node = tree_demo_get_node_by_index(target_index);
    node->expanded = 0;
    tree_demo_ctx.selected_id = node->stable_id;
    tree_demo_ctx.action_count++;
    tree_demo_ctx.jump_cursor = target_index;
    egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&tree_view));
    egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id, TREE_DEMO_NODE_GAP_Y / 2);
    snprintf(tree_demo_ctx.last_action_text, sizeof(tree_demo_ctx.last_action_text), "fold %05lu", (unsigned long)(node->stable_id % 100000UL));
    tree_demo_refresh_status();
}

static void tree_demo_patch_action(void)
{
    tree_demo_node_t *node = tree_demo_get_node_by_stable_id(tree_demo_ctx.selected_id);
    int32_t old_height;
    int32_t new_height;

    if (node == NULL)
    {
        uint16_t target_index = tree_demo_find_hot_task_after((uint16_t)(tree_demo_ctx.jump_cursor % tree_demo_ctx.node_count));

        if (target_index == TREE_DEMO_INVALID_NODE_INDEX)
        {
            snprintf(tree_demo_ctx.last_action_text, sizeof(tree_demo_ctx.last_action_text), "patch ignored");
            tree_demo_refresh_status();
            return;
        }
        node = tree_demo_get_node_by_index(target_index);
        tree_demo_ctx.selected_id = node->stable_id;
    }

    old_height = tree_demo_measure_node_height_with_state(node, 1);
    node->revision++;
    node->state = (uint8_t)((node->state + 1U) % TREE_DEMO_STATE_COUNT);

    if (node->kind == TREE_DEMO_NODE_KIND_TASK)
    {
        node->variant = (uint8_t)((node->variant + 1U) % TREE_DEMO_TASK_VARIANT_COUNT);
        if (node->state == TREE_DEMO_STATE_DONE)
        {
            node->progress = 100U;
        }
        else
        {
            node->progress = (uint8_t)(20U + ((node->progress + 17U) % 71U));
        }
        if (node->variant == TREE_DEMO_TASK_VARIANT_ALERT && node->state == TREE_DEMO_STATE_IDLE)
        {
            node->state = TREE_DEMO_STATE_WARN;
        }
    }

    new_height = tree_demo_measure_node_height_with_state(node, 1);
    tree_demo_ctx.action_count++;
    tree_demo_ctx.jump_cursor = (uint32_t)(node - tree_demo_ctx.nodes);
    if (new_height != old_height)
    {
        egui_view_virtual_tree_notify_node_resized_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id);
    }
    else
    {
        egui_view_virtual_tree_notify_node_changed_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id);
    }
    egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id, TREE_DEMO_NODE_GAP_Y / 2);
    snprintf(tree_demo_ctx.last_action_text, sizeof(tree_demo_ctx.last_action_text), "patch %05lu", (unsigned long)(node->stable_id % 100000UL));
    tree_demo_refresh_status();
}

static void tree_demo_jump_action(void)
{
    uint16_t target_index = tree_demo_find_hot_task_after((uint16_t)(tree_demo_ctx.jump_cursor % tree_demo_ctx.node_count));
    tree_demo_node_t *node;
    uint32_t previous_selected = tree_demo_ctx.selected_id;

    if (target_index == TREE_DEMO_INVALID_NODE_INDEX)
    {
        snprintf(tree_demo_ctx.last_action_text, sizeof(tree_demo_ctx.last_action_text), "jump ignored");
        tree_demo_refresh_status();
        return;
    }

    node = tree_demo_get_node_by_index(target_index);
    tree_demo_expand_path_to_index(target_index);
    tree_demo_ctx.selected_id = node->stable_id;
    tree_demo_ctx.action_count++;
    tree_demo_ctx.jump_cursor = target_index;
    egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&tree_view));
    if (previous_selected != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && previous_selected != node->stable_id)
    {
        egui_view_virtual_tree_notify_node_resized_by_stable_id(EGUI_VIEW_OF(&tree_view), previous_selected);
    }
    egui_view_virtual_tree_notify_node_resized_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id);
    egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&tree_view), node->stable_id, TREE_DEMO_NODE_GAP_Y / 2);
    snprintf(tree_demo_ctx.last_action_text, sizeof(tree_demo_ctx.last_action_text), "jump %05lu", (unsigned long)(node->stable_id % 100000UL));
    tree_demo_refresh_status();
}

static void tree_demo_apply_action(uint8_t action)
{
    switch (action)
    {
    case TREE_DEMO_ACTION_EXPAND:
        tree_demo_expand_action();
        break;
    case TREE_DEMO_ACTION_FOLD:
        tree_demo_fold_action();
        break;
    case TREE_DEMO_ACTION_PATCH:
        tree_demo_patch_action();
        break;
    case TREE_DEMO_ACTION_JUMP:
        tree_demo_jump_action();
        break;
    default:
        break;
    }
}

static int tree_demo_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < TREE_DEMO_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void tree_demo_action_click_cb(egui_view_t *self)
{
    int action = tree_demo_find_action_button_index(self);

    if (action >= 0)
    {
        tree_demo_apply_action((uint8_t)action);
    }
}

static void tree_demo_init_action_button(egui_view_button_t *button, egui_dim_t x, const char *text)
{
    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_position(EGUI_VIEW_OF(button), x, 6);
    egui_view_set_size(EGUI_VIEW_OF(button), TREE_DEMO_BUTTON_W, TREE_DEMO_BUTTON_H);
    egui_view_label_set_text(EGUI_VIEW_OF(button), text);
    egui_view_label_set_font(EGUI_VIEW_OF(button), TREE_DEMO_FONT_CAP);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x2B3F52), EGUI_ALPHA_100);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), tree_demo_action_click_cb);
}

void test_init_ui(void)
{
    uint8_t i;
    egui_dim_t button_x = 10;

    tree_demo_reset_model();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&tree_demo_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&header_card), &tree_demo_header_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_card), 1, EGUI_COLOR_HEX(0xD7E1EA));
    egui_view_set_shadow(EGUI_VIEW_OF(&header_card), &tree_demo_card_shadow);

    egui_view_label_init(EGUI_VIEW_OF(&header_title));
    egui_view_label_init(EGUI_VIEW_OF(&header_detail));
    egui_view_label_init(EGUI_VIEW_OF(&header_hint));
    egui_view_set_position(EGUI_VIEW_OF(&header_title), 12, 10);
    egui_view_set_size(EGUI_VIEW_OF(&header_title), TREE_DEMO_HEADER_W - 24, 16);
    egui_view_set_position(EGUI_VIEW_OF(&header_detail), 12, 28);
    egui_view_set_size(EGUI_VIEW_OF(&header_detail), TREE_DEMO_HEADER_W - 24, 14);
    egui_view_set_position(EGUI_VIEW_OF(&header_hint), 12, 44);
    egui_view_set_size(EGUI_VIEW_OF(&header_hint), TREE_DEMO_HEADER_W - 24, 14);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_title), TREE_DEMO_FONT_HEADER);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_detail), TREE_DEMO_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_hint), TREE_DEMO_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_detail), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_hint), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_title), EGUI_COLOR_HEX(0x203243), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_detail), EGUI_COLOR_HEX(0x5A6D7E), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_hint), EGUI_COLOR_HEX(0x6B7C8A), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_title));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_detail));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_hint));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &tree_demo_toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD7E1EA));

    for (i = 0; i < TREE_DEMO_ACTION_COUNT; i++)
    {
        tree_demo_init_action_button(&action_buttons[i], button_x, tree_demo_action_names[i]);
        egui_view_set_background(EGUI_VIEW_OF(&action_buttons[i]), i == TREE_DEMO_ACTION_EXPAND  ? EGUI_BG_OF(&tree_demo_action_expand_bg)
                                                                   : i == TREE_DEMO_ACTION_FOLD  ? EGUI_BG_OF(&tree_demo_action_fold_bg)
                                                                   : i == TREE_DEMO_ACTION_PATCH ? EGUI_BG_OF(&tree_demo_action_patch_bg)
                                                                                                 : EGUI_BG_OF(&tree_demo_action_jump_bg));
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
        button_x += TREE_DEMO_BUTTON_W + TREE_DEMO_BUTTON_GAP;
    }

    {
        const egui_view_virtual_tree_setup_t tree_view_setup = {
                .params = &tree_demo_view_params,
                .data_source = &tree_demo_data_source,
                .data_source_context = &tree_demo_ctx,
                .state_cache_max_entries = TREE_DEMO_STATE_CACHE_COUNT,
                .state_cache_max_bytes = TREE_DEMO_STATE_CACHE_COUNT * (uint32_t)sizeof(tree_demo_node_state_t),
        };

        egui_view_virtual_tree_init_with_setup(EGUI_VIEW_OF(&tree_view), &tree_view_setup);
    }
    egui_view_set_background(EGUI_VIEW_OF(&tree_view), EGUI_BG_OF(&tree_demo_view_bg));
    egui_view_set_shadow(EGUI_VIEW_OF(&tree_view), &tree_demo_card_shadow);

    tree_demo_refresh_status();

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&tree_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&header_card));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}

#if EGUI_CONFIG_RECORDING_TEST
static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

typedef struct tree_demo_visible_search_context
{
    uint32_t min_visible_index;
    uint8_t want_branch;
    uint8_t min_depth;
} tree_demo_visible_search_context_t;

static uint8_t tree_demo_match_visible_node(egui_view_t *self, const egui_view_virtual_tree_slot_t *slot, const egui_view_virtual_tree_entry_t *entry,
                                            egui_view_t *node_view, void *context)
{
    tree_demo_visible_search_context_t *ctx = (tree_demo_visible_search_context_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(node_view);

    if (!egui_view_virtual_viewport_is_slot_center_visible(EGUI_VIEW_OF(&tree_view), slot) || entry == NULL)
    {
        return 0;
    }
    if (entry->visible_index < ctx->min_visible_index || entry->depth < ctx->min_depth)
    {
        return 0;
    }
    if ((ctx->want_branch && !entry->has_children) || (!ctx->want_branch && entry->has_children))
    {
        return 0;
    }

    return 1;
}

static egui_view_t *tree_demo_find_visible_view(uint8_t want_branch, uint8_t min_depth, uint32_t min_visible_index)
{
    tree_demo_visible_search_context_t ctx = {
            .min_visible_index = min_visible_index,
            .want_branch = want_branch,
            .min_depth = min_depth,
    };

    return egui_view_virtual_tree_find_first_visible_node_view(EGUI_VIEW_OF(&tree_view), tree_demo_match_visible_node, &ctx, NULL);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    static uint32_t visible_before_collapse = 0;
    egui_view_t *view;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call && tree_demo_ctx.created_count > EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
        {
            report_runtime_failure("virtual tree created more views than slot capacity");
        }
        view = tree_demo_find_visible_view(0, 2, 2);
        if (view == NULL)
        {
            report_runtime_failure("initial task node was not visible");
            EGUI_SIM_SET_WAIT(p_action, 200);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 220);
        return true;
    case 1:
        if (first_call && tree_demo_ctx.last_clicked_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
        {
            report_runtime_failure("task click did not update selected node");
        }
        visible_before_collapse = tree_demo_get_visible_count();
        view = tree_demo_find_visible_view(1, 1, 1);
        if (view == NULL)
        {
            report_runtime_failure("branch node was not visible");
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 220);
        return true;
    case 2:
        if (first_call && tree_demo_get_visible_count() >= visible_before_collapse)
        {
            report_runtime_failure("branch collapse did not reduce visible nodes");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[TREE_DEMO_ACTION_PATCH]), 220);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[TREE_DEMO_ACTION_JUMP]), 220);
        return true;
    case 4:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 4 / 5;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 5;
        p_action->steps = 4;
        p_action->interval_ms = 520;
        return true;
    case 5:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 4;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 3;
        p_action->steps = 5;
        p_action->interval_ms = 620;
        return true;
    case 6:
        view = tree_demo_find_visible_view(0, 2, 8);
        if (view == NULL)
        {
            report_runtime_failure("task node after drag was not visible");
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 220);
        return true;
    case 7:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[TREE_DEMO_ACTION_FOLD]), 220);
        return true;
    case 8:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[TREE_DEMO_ACTION_EXPAND]), 220);
        return true;
    case 9:
        if (first_call && tree_demo_ctx.selected_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
        {
            report_runtime_failure("runtime flow ended without a selected node");
        }
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 320);
        return true;
    default:
        return false;
    }
}
#endif
