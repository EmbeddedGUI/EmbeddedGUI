#include "egui.h"
#include <stdio.h>
#include <string.h>
#include "uicode.h"

#define DEMO_MAX_ITEMS         1200U
#define DEMO_FEED_ITEMS        1080U
#define DEMO_CHAT_ITEMS        720U
#define DEMO_TASK_ITEMS        260U
#define DEMO_INVALID_INDEX     0xFFFFFFFFUL
#define DEMO_STATUS_TEXT_LEN   96
#define DEMO_TITLE_TEXT_LEN    48
#define DEMO_SUBTEXT_LEN       72
#define DEMO_META_TEXT_LEN     72
#define DEMO_BADGE_TEXT_LEN    20
#define DEMO_STATE_CACHE_COUNT 96U

#define DEMO_MARGIN_X          8
#define DEMO_TOP_Y             8
#define DEMO_HEADER_W          (EGUI_CONFIG_SCEEN_WIDTH - DEMO_MARGIN_X * 2)
#define DEMO_HEADER_H          82
#define DEMO_TOOLBAR_Y         (DEMO_TOP_Y + DEMO_HEADER_H + 6)
#define DEMO_TOOLBAR_H         32
#define DEMO_LIST_Y            (DEMO_TOOLBAR_Y + DEMO_TOOLBAR_H + 6)
#define DEMO_LIST_W            DEMO_HEADER_W
#define DEMO_LIST_H            (EGUI_CONFIG_SCEEN_HEIGHT - DEMO_LIST_Y - 8)
#define DEMO_SCENE_BUTTON_GAP  6
#define DEMO_ACTION_BUTTON_GAP 4
#define DEMO_SCENE_BUTTON_W    ((DEMO_HEADER_W - 24 - DEMO_SCENE_BUTTON_GAP * 2) / 3)
#define DEMO_ACTION_BUTTON_W   ((DEMO_HEADER_W - 20 - DEMO_ACTION_BUTTON_GAP * 4) / 5)
#define DEMO_CHIP_H            20
#define DEMO_ROW_GAP           6
#define DEMO_ROW_SIDE_INSET    8
#define DEMO_BADGE_H           18
#define DEMO_TITLE_H           14
#define DEMO_TEXT_H            12
#define DEMO_PROGRESS_H        4

#define DEMO_FONT_HEADER ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define DEMO_FONT_TITLE  ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define DEMO_FONT_BODY   ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define DEMO_FONT_CAP    ((const egui_font_t *)&egui_res_font_montserrat_8_4)

enum
{
    DEMO_SCENE_FEED = 0,
    DEMO_SCENE_CHAT,
    DEMO_SCENE_TASK,
    DEMO_SCENE_COUNT,
};

enum
{
    DEMO_ACTION_ADD = 0,
    DEMO_ACTION_DEL,
    DEMO_ACTION_MOVE,
    DEMO_ACTION_PATCH,
    DEMO_ACTION_JUMP,
    DEMO_ACTION_COUNT,
};

typedef struct demo_virtual_item demo_virtual_item_t;
typedef struct demo_virtual_row demo_virtual_row_t;
typedef struct demo_virtual_row_state demo_virtual_row_state_t;
typedef struct demo_virtual_viewport_context demo_virtual_viewport_context_t;

struct demo_virtual_item
{
    uint32_t stable_id;
    uint16_t revision;
    uint8_t variant;
    uint8_t progress;
    uint8_t state;
    uint8_t reserved;
};

struct demo_virtual_row
{
    egui_view_group_t root;
    egui_view_card_t surface;
    egui_view_label_t title;
    egui_view_label_t subtitle;
    egui_view_label_t meta;
    egui_view_label_t badge;
    egui_view_progress_bar_t progress;
    egui_view_t pulse;
    egui_animation_alpha_t pulse_anim;
    egui_interpolator_linear_t pulse_interp;
    uint32_t bound_index;
    uint32_t stable_id;
    uint8_t pulse_running;
};

struct demo_virtual_row_state
{
    uint16_t pulse_elapsed_ms;
    uint8_t pulse_running;
    uint8_t pulse_alpha;
    uint8_t pulse_cycle_flip;
    uint8_t pulse_repeated;
};

struct demo_virtual_viewport_context
{
    uint8_t scene;
    uint32_t item_count;
    uint32_t next_stable_id;
    uint32_t selected_id;
    uint32_t last_clicked_index;
    uint32_t click_count;
    uint32_t action_count;
    uint32_t mutation_count;
    uint32_t scene_switch_count;
    uint32_t jump_cursor;
    uint8_t created_count;
    demo_virtual_item_t items[DEMO_MAX_ITEMS];
    char title_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][DEMO_TITLE_TEXT_LEN];
    char subtitle_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][DEMO_SUBTEXT_LEN];
    char meta_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][DEMO_META_TEXT_LEN];
    char badge_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][DEMO_BADGE_TEXT_LEN];
    char header_title_text[DEMO_TITLE_TEXT_LEN];
    char status_detail[DEMO_STATUS_TEXT_LEN];
    char status_hint[DEMO_STATUS_TEXT_LEN];
    char last_action_text[DEMO_STATUS_TEXT_LEN];
    demo_virtual_row_t rows[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
};

static const char *demo_scene_names[DEMO_SCENE_COUNT] = {"Feed", "Chat", "Ops"};
static const char *demo_action_names[DEMO_ACTION_COUNT] = {"Add", "Del", "Move", "Patch", "Jump"};

static egui_view_t background_view;
static egui_view_card_t header_card;
static egui_view_card_t toolbar_card;
static egui_view_label_t header_title;
static egui_view_label_t header_detail;
static egui_view_label_t header_hint;
static egui_view_button_t scene_buttons[DEMO_SCENE_COUNT];
static egui_view_button_t action_buttons[DEMO_ACTION_COUNT];
static egui_view_virtual_list_t viewport_1;
static demo_virtual_viewport_context_t viewport_context;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(header_card_params, DEMO_MARGIN_X, DEMO_TOP_Y, DEMO_HEADER_W, DEMO_HEADER_H, 14);
EGUI_VIEW_CARD_PARAMS_INIT(toolbar_card_params, DEMO_MARGIN_X, DEMO_TOOLBAR_Y, DEMO_HEADER_W, DEMO_TOOLBAR_H, 12);
EGUI_VIEW_VIRTUAL_LIST_PARAMS_INIT(viewport_1_params, DEMO_MARGIN_X, DEMO_LIST_Y, DEMO_LIST_W, DEMO_LIST_H);

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF3F7), EGUI_COLOR_HEX(0xD8E4EF),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(screen_bg_params, &screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(screen_bg, &screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(scene_chip_idle_param, EGUI_COLOR_HEX(0xE8EEF5), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(scene_chip_idle_params, &scene_chip_idle_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(scene_chip_idle_bg, &scene_chip_idle_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(scene_chip_active_param, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(scene_chip_active_params, &scene_chip_active_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(scene_chip_active_bg, &scene_chip_active_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(list_bg_param, EGUI_COLOR_HEX(0xF9FBFD), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(list_bg_params, &list_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(list_bg, &list_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(action_add_param, EGUI_COLOR_HEX(0xDFF3E9), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(action_add_params, &action_add_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(action_add_bg, &action_add_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(action_del_param, EGUI_COLOR_HEX(0xFFF1E1), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(action_del_params, &action_del_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(action_del_bg, &action_del_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(action_move_param, EGUI_COLOR_HEX(0xE7EDF7), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(action_move_params, &action_move_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(action_move_bg, &action_move_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(action_patch_param, EGUI_COLOR_HEX(0xE3F6F7), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(action_patch_params, &action_patch_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(action_patch_bg, &action_patch_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(action_jump_param, EGUI_COLOR_HEX(0xE5EEFF), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(action_jump_params, &action_jump_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(action_jump_bg, &action_jump_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_selected_param, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_selected_params, &badge_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_selected_bg, &badge_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_info_param, EGUI_COLOR_HEX(0x56789A), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_info_params, &badge_info_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_info_bg, &badge_info_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_success_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_success_params, &badge_success_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_success_bg, &badge_success_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_warning_param, EGUI_COLOR_HEX(0xE7B14A), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_warning_params, &badge_warning_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_warning_bg, &badge_warning_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_muted_param, EGUI_COLOR_HEX(0x8D99A6), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_muted_params, &badge_muted_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_muted_bg, &badge_muted_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(pulse_selected_param, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(pulse_selected_params, &pulse_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(pulse_selected_bg, &pulse_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(pulse_live_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(pulse_live_params, &pulse_live_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(pulse_live_bg, &pulse_live_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(pulse_chat_param, EGUI_COLOR_HEX(0x5C9EE6), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(pulse_chat_params, &pulse_chat_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(pulse_chat_bg, &pulse_chat_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(pulse_warning_param, EGUI_COLOR_HEX(0xD08A2E), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(pulse_warning_params, &pulse_warning_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(pulse_warning_bg, &pulse_warning_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(pulse_task_param, EGUI_COLOR_HEX(0xC95B4A), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(pulse_task_params, &pulse_task_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(pulse_task_bg, &pulse_task_params);

EGUI_SHADOW_PARAM_INIT_ROUND(demo_card_shadow, 10, 0, 3, EGUI_COLOR_BLACK, EGUI_ALPHA_20, 12);
EGUI_ANIMATION_ALPHA_PARAMS_INIT(pulse_anim_param, EGUI_ALPHA_100, EGUI_ALPHA_20);

static uint32_t demo_scene_default_count(uint8_t scene)
{
    switch (scene)
    {
    case DEMO_SCENE_CHAT:
        return DEMO_CHAT_ITEMS;
    case DEMO_SCENE_TASK:
        return DEMO_TASK_ITEMS;
    default:
        return DEMO_FEED_ITEMS;
    }
}

static uint32_t demo_scene_base_id(uint8_t scene)
{
    return (uint32_t)(scene + 1U) * 100000U;
}

static int32_t demo_find_index_by_stable_id(uint32_t stable_id)
{
    uint32_t i;

    for (i = 0; i < viewport_context.item_count; i++)
    {
        if (viewport_context.items[i].stable_id == stable_id)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static demo_virtual_item_t *demo_get_item(uint32_t index)
{
    if (index >= viewport_context.item_count)
    {
        return NULL;
    }

    return &viewport_context.items[index];
}

static const demo_virtual_item_t *demo_get_item_const(uint32_t index)
{
    if (index >= viewport_context.item_count)
    {
        return NULL;
    }

    return &viewport_context.items[index];
}

static uint8_t demo_is_selected_id(uint32_t stable_id)
{
    return stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && stable_id == viewport_context.selected_id;
}

static demo_virtual_item_t demo_make_item(uint8_t scene, uint32_t ordinal)
{
    demo_virtual_item_t item;

    memset(&item, 0, sizeof(item));
    item.stable_id = viewport_context.next_stable_id++;
    item.revision = (uint16_t)((ordinal * 7U + scene * 11U) % 29U);
    item.progress = (uint8_t)((ordinal * 13U + scene * 17U + 11U) % 100U);

    switch (scene)
    {
    case DEMO_SCENE_CHAT:
        item.variant = (ordinal % 9U == 0U) ? 2U : (uint8_t)(ordinal & 1U);
        item.state = (ordinal % 8U == 0U) ? 2U : ((ordinal % 5U == 0U) ? 1U : 0U);
        break;
    case DEMO_SCENE_TASK:
        item.state = (uint8_t)(ordinal % 4U);
        item.variant = (item.state == 2U) ? 2U : ((item.state == 1U) ? 1U : 0U);
        if (item.state == 3U)
        {
            item.progress = 100U;
        }
        break;
    default:
        item.variant = (uint8_t)(ordinal % 3U);
        item.state = (ordinal % 7U == 0U) ? 1U : ((ordinal % 5U == 0U) ? 2U : 0U);
        break;
    }

    return item;
}

static demo_virtual_item_t demo_make_inserted_item(uint8_t scene)
{
    demo_virtual_item_t item = demo_make_item(scene, viewport_context.mutation_count + viewport_context.item_count + 5U);

    item.revision = (uint16_t)(item.revision + 7U);
    switch (scene)
    {
    case DEMO_SCENE_CHAT:
        item.variant = 1U;
        item.state = 2U;
        item.progress = 24U;
        break;
    case DEMO_SCENE_TASK:
        item.variant = 1U;
        item.state = 1U;
        item.progress = 12U;
        break;
    default:
        item.variant = 0U;
        item.state = 1U;
        item.progress = 9U;
        break;
    }

    return item;
}

static int32_t demo_get_item_height_by_stable_id(uint32_t stable_id)
{
    int32_t index = demo_find_index_by_stable_id(stable_id);
    const demo_virtual_item_t *item;
    int32_t base_height;
    int32_t selected_extra = demo_is_selected_id(stable_id) ? 18 : 0;

    if (index < 0)
    {
        return 64;
    }

    item = &viewport_context.items[index];
    switch (viewport_context.scene)
    {
    case DEMO_SCENE_CHAT:
        if (item->variant == 2U)
        {
            base_height = 60;
        }
        else if (item->variant == 1U)
        {
            base_height = 74;
        }
        else
        {
            base_height = 62;
        }
        if (item->state == 2U)
        {
            base_height += 8;
        }
        break;
    case DEMO_SCENE_TASK:
        if (item->variant == 2U)
        {
            base_height = 94;
        }
        else if (item->variant == 1U)
        {
            base_height = 82;
        }
        else
        {
            base_height = 66;
        }
        if (item->state == 2U)
        {
            base_height += 6;
        }
        break;
    default:
        if (item->variant == 0U)
        {
            base_height = 92;
        }
        else if (item->variant == 1U)
        {
            base_height = 74;
        }
        else
        {
            base_height = 58;
        }
        if (item->state == 2U)
        {
            base_height += 4;
        }
        break;
    }

    return base_height + selected_extra;
}

static uint8_t demo_item_should_keepalive(const demo_virtual_item_t *item)
{
    if (item == NULL)
    {
        return 0;
    }

    switch (viewport_context.scene)
    {
    case DEMO_SCENE_CHAT:
        return item->state == 2U;
    case DEMO_SCENE_TASK:
        return item->state == 1U || item->state == 2U;
    default:
        return item->state == 1U || item->state == 2U;
    }
}

static egui_dim_t demo_get_list_width(void)
{
    egui_dim_t width = EGUI_VIEW_OF(&viewport_1)->region.size.width;

    return width > 0 ? width : DEMO_LIST_W;
}

static void demo_keep_item_fully_visible(uint32_t index, uint32_t stable_id)
{
    int32_t item_y;
    int32_t item_h;
    int32_t viewport_h;
    int32_t inset = DEMO_ROW_GAP / 2;

    if (index == DEMO_INVALID_INDEX || stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return;
    }

    item_y = egui_view_virtual_list_get_item_y(EGUI_VIEW_OF(&viewport_1), index);
    item_h = demo_get_item_height_by_stable_id(stable_id);
    viewport_h = EGUI_VIEW_OF(&viewport_1)->region.size.height;

    if (viewport_h <= 0)
    {
        return;
    }

    if (item_y < inset || (item_y + item_h) > (viewport_h - inset))
    {
        egui_view_virtual_list_scroll_to_stable_id(EGUI_VIEW_OF(&viewport_1), stable_id, inset);
    }
}

static int demo_get_row_pool_index(demo_virtual_row_t *row)
{
    uint8_t i;

    for (i = 0; i < viewport_context.created_count; i++)
    {
        if (row == &viewport_context.rows[i])
        {
            return (int)i;
        }
    }

    return -1;
}

static demo_virtual_row_t *demo_find_row_by_root_view(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < viewport_context.created_count; i++)
    {
        if (view == EGUI_VIEW_OF(&viewport_context.rows[i].root))
        {
            return &viewport_context.rows[i];
        }
    }

    return NULL;
}

static void demo_set_row_pulse(demo_virtual_row_t *row, const demo_virtual_item_t *item, uint8_t visible, uint8_t selected)
{
    egui_background_t *pulse_bg;

    if (!visible || item == NULL)
    {
        if (row->pulse_running)
        {
            egui_animation_stop(EGUI_ANIM_OF(&row->pulse_anim));
            row->pulse_running = 0;
        }
        egui_view_set_gone(EGUI_VIEW_OF(&row->pulse), 1);
        egui_view_set_alpha(EGUI_VIEW_OF(&row->pulse), EGUI_ALPHA_100);
        return;
    }

    if (selected)
    {
        pulse_bg = EGUI_BG_OF(&pulse_selected_bg);
    }
    else if (viewport_context.scene == DEMO_SCENE_CHAT)
    {
        pulse_bg = EGUI_BG_OF(item->state == 2U ? &pulse_chat_bg : &pulse_warning_bg);
    }
    else if (viewport_context.scene == DEMO_SCENE_TASK)
    {
        pulse_bg = EGUI_BG_OF(item->state == 2U ? &pulse_task_bg : &pulse_live_bg);
    }
    else
    {
        pulse_bg = EGUI_BG_OF(item->state == 2U ? &pulse_warning_bg : &pulse_live_bg);
    }

    egui_view_set_gone(EGUI_VIEW_OF(&row->pulse), 0);
    egui_view_set_background(EGUI_VIEW_OF(&row->pulse), pulse_bg);
    if (!row->pulse_running)
    {
        egui_animation_start(EGUI_ANIM_OF(&row->pulse_anim));
        row->pulse_running = 1;
    }
}

static void demo_capture_row_state(demo_virtual_row_t *row, demo_virtual_row_state_t *state)
{
    egui_animation_t *anim = EGUI_ANIM_OF(&row->pulse_anim);

    memset(state, 0, sizeof(*state));
    state->pulse_running = row->pulse_running ? 1U : 0U;
    state->pulse_alpha = EGUI_VIEW_OF(&row->pulse)->alpha;

    if (!row->pulse_running)
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

static void demo_restore_row_state(demo_virtual_row_t *row, const demo_virtual_row_state_t *state)
{
    egui_animation_t *anim;

    if (state == NULL || !state->pulse_running)
    {
        return;
    }

    anim = EGUI_ANIM_OF(&row->pulse_anim);
    if (!anim->is_running)
    {
        egui_animation_start(anim);
    }

    row->pulse_running = 1;
    egui_view_set_gone(EGUI_VIEW_OF(&row->pulse), 0);
    egui_view_set_alpha(EGUI_VIEW_OF(&row->pulse), state->pulse_alpha);
    anim->is_started = 1;
    anim->is_ended = 0;
    anim->is_cycle_flip = state->pulse_cycle_flip ? 1U : 0U;
    anim->repeated = (int8_t)state->pulse_repeated;
    anim->start_time = egui_api_timer_get_current() - state->pulse_elapsed_ms;
}

static void demo_style_scene_buttons(void)
{
    uint8_t i;

    for (i = 0; i < DEMO_SCENE_COUNT; i++)
    {
        uint8_t active = viewport_context.scene == i;

        egui_view_set_background(EGUI_VIEW_OF(&scene_buttons[i]), active ? EGUI_BG_OF(&scene_chip_active_bg) : EGUI_BG_OF(&scene_chip_idle_bg));
        egui_view_label_set_font_color(EGUI_VIEW_OF(&scene_buttons[i]), active ? EGUI_COLOR_WHITE : EGUI_COLOR_HEX(0x33485C), EGUI_ALPHA_100);
    }
}

static void demo_style_action_buttons(void)
{
    egui_view_set_background(EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_ADD]), EGUI_BG_OF(&action_add_bg));
    egui_view_set_background(EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_DEL]), EGUI_BG_OF(&action_del_bg));
    egui_view_set_background(EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_MOVE]), EGUI_BG_OF(&action_move_bg));
    egui_view_set_background(EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_PATCH]), EGUI_BG_OF(&action_patch_bg));
    egui_view_set_background(EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_JUMP]), EGUI_BG_OF(&action_jump_bg));

    egui_view_label_set_font_color(EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_ADD]), EGUI_COLOR_HEX(0x1D6A46), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_DEL]), EGUI_COLOR_HEX(0x985B1F), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_MOVE]), EGUI_COLOR_HEX(0x354B64), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_PATCH]), EGUI_COLOR_HEX(0x156C76), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_JUMP]), EGUI_COLOR_HEX(0x285EA8), EGUI_ALPHA_100);
}

static void demo_update_status_labels(void)
{
    int32_t selected_index = demo_find_index_by_stable_id(viewport_context.selected_id);

    snprintf(viewport_context.status_detail, sizeof(viewport_context.status_detail), "%s | rows=%lu | slots=%u/%u | mut=%lu",
             demo_scene_names[viewport_context.scene], (unsigned long)viewport_context.item_count, (unsigned)viewport_context.created_count,
             (unsigned)EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS, (unsigned long)viewport_context.mutation_count);

    if (viewport_context.last_action_text[0] == '\0')
    {
        snprintf(viewport_context.status_hint, sizeof(viewport_context.status_hint), "Tap a row or toolbar action.");
    }
    else if (selected_index >= 0)
    {
        snprintf(viewport_context.status_hint, sizeof(viewport_context.status_hint), "sel=%04ld | %s", (long)selected_index, viewport_context.last_action_text);
    }
    else
    {
        snprintf(viewport_context.status_hint, sizeof(viewport_context.status_hint), "%s", viewport_context.last_action_text);
    }

    snprintf(viewport_context.header_title_text, sizeof(viewport_context.header_title_text), "Virtual Viewport | %s", demo_scene_names[viewport_context.scene]);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_title), viewport_context.header_title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_detail), viewport_context.status_detail);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_hint), viewport_context.status_hint);

    demo_style_scene_buttons();
}

static void demo_reset_scene(uint8_t scene)
{
    uint32_t i;
    uint32_t count = demo_scene_default_count(scene);
    uint32_t default_index;

    viewport_context.scene = scene;
    viewport_context.item_count = count;
    viewport_context.next_stable_id = demo_scene_base_id(scene);
    viewport_context.jump_cursor = 0;
    viewport_context.last_clicked_index = DEMO_INVALID_INDEX;

    for (i = 0; i < count; i++)
    {
        viewport_context.items[i] = demo_make_item(scene, i);
    }

    default_index = (scene == DEMO_SCENE_CHAT) ? 3U : 2U;
    if (default_index >= count)
    {
        default_index = 0U;
    }
    viewport_context.selected_id = count > 0 ? viewport_context.items[default_index].stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static void demo_switch_scene(uint8_t scene)
{
    if (scene >= DEMO_SCENE_COUNT)
    {
        return;
    }

    demo_reset_scene(scene);
    viewport_context.scene_switch_count++;
    viewport_context.action_count++;
    snprintf(viewport_context.last_action_text, sizeof(viewport_context.last_action_text), "scene -> %s", demo_scene_names[scene]);
    egui_view_virtual_list_set_scroll_y(EGUI_VIEW_OF(&viewport_1), 0);
    egui_view_virtual_list_notify_data_changed(EGUI_VIEW_OF(&viewport_1));
    demo_update_status_labels();
    egui_view_invalidate(EGUI_VIEW_OF(&background_view));
    egui_view_invalidate(EGUI_VIEW_OF(&header_card));
    egui_view_invalidate(EGUI_VIEW_OF(&toolbar_card));
    egui_view_invalidate(EGUI_VIEW_OF(&viewport_1));
}

static void demo_insert_item_at(uint32_t index, const demo_virtual_item_t *item)
{
    if (viewport_context.item_count >= DEMO_MAX_ITEMS || item == NULL)
    {
        return;
    }
    if (index > viewport_context.item_count)
    {
        index = viewport_context.item_count;
    }

    if (index < viewport_context.item_count)
    {
        memmove(&viewport_context.items[index + 1], &viewport_context.items[index],
                (viewport_context.item_count - index) * sizeof(viewport_context.items[0]));
    }
    viewport_context.items[index] = *item;
    viewport_context.item_count++;
}

static void demo_remove_item_at(uint32_t index)
{
    if (viewport_context.item_count == 0 || index >= viewport_context.item_count)
    {
        return;
    }

    if (index + 1 < viewport_context.item_count)
    {
        memmove(&viewport_context.items[index], &viewport_context.items[index + 1],
                (viewport_context.item_count - index - 1) * sizeof(viewport_context.items[0]));
    }
    viewport_context.item_count--;
}

static void demo_move_item(uint32_t from_index, uint32_t to_index)
{
    demo_virtual_item_t moved;

    if (from_index >= viewport_context.item_count || to_index >= viewport_context.item_count || from_index == to_index)
    {
        return;
    }

    moved = viewport_context.items[from_index];
    if (from_index < to_index)
    {
        memmove(&viewport_context.items[from_index], &viewport_context.items[from_index + 1],
                (to_index - from_index) * sizeof(viewport_context.items[0]));
    }
    else
    {
        memmove(&viewport_context.items[to_index + 1], &viewport_context.items[to_index],
                (from_index - to_index) * sizeof(viewport_context.items[0]));
    }
    viewport_context.items[to_index] = moved;
}

static void demo_patch_item(demo_virtual_item_t *item)
{
    if (item == NULL)
    {
        return;
    }

    item->revision++;
    item->progress = (uint8_t)((item->progress + 19U) % 100U);

    switch (viewport_context.scene)
    {
    case DEMO_SCENE_CHAT:
        item->state = (uint8_t)((item->state + 1U) % 3U);
        if (item->variant == 2U)
        {
            item->variant = 0U;
        }
        else
        {
            item->variant ^= 1U;
        }
        break;
    case DEMO_SCENE_TASK:
        item->state = (uint8_t)((item->state + 1U) % 4U);
        item->variant = (item->state == 2U) ? 2U : ((item->state == 1U) ? 1U : 0U);
        if (item->state == 3U)
        {
            item->progress = 100U;
        }
        break;
    default:
        item->state = (uint8_t)((item->state + 1U) % 3U);
        item->variant = (uint8_t)((item->variant + 1U) % 3U);
        break;
    }
}

static void demo_action_add(void)
{
    demo_virtual_item_t item;
    int32_t selected_index = demo_find_index_by_stable_id(viewport_context.selected_id);
    uint32_t insert_index;

    if (viewport_context.item_count >= DEMO_MAX_ITEMS)
    {
        snprintf(viewport_context.last_action_text, sizeof(viewport_context.last_action_text), "insert skipped at cap");
        demo_update_status_labels();
        return;
    }

    if (viewport_context.scene == DEMO_SCENE_CHAT && selected_index >= 0)
    {
        insert_index = (uint32_t)selected_index + 1U;
    }
    else if (viewport_context.scene == DEMO_SCENE_TASK && selected_index > 0)
    {
        insert_index = (uint32_t)selected_index;
    }
    else
    {
        insert_index = 0U;
    }

    item = demo_make_inserted_item(viewport_context.scene);
    demo_insert_item_at(insert_index, &item);
    viewport_context.selected_id = item.stable_id;
    viewport_context.action_count++;
    viewport_context.mutation_count++;
    snprintf(viewport_context.last_action_text, sizeof(viewport_context.last_action_text), "insert @%04lu", (unsigned long)insert_index);
    egui_view_virtual_list_notify_item_inserted(EGUI_VIEW_OF(&viewport_1), insert_index, 1);
    egui_view_virtual_list_scroll_to_stable_id(EGUI_VIEW_OF(&viewport_1), item.stable_id, 0);
    demo_update_status_labels();
}

static void demo_action_del(void)
{
    int32_t selected_index = demo_find_index_by_stable_id(viewport_context.selected_id);
    uint32_t remove_index;

    if (viewport_context.item_count <= 1U)
    {
        snprintf(viewport_context.last_action_text, sizeof(viewport_context.last_action_text), "delete skipped");
        demo_update_status_labels();
        return;
    }

    if (selected_index >= 0)
    {
        remove_index = (uint32_t)selected_index;
    }
    else
    {
        remove_index = viewport_context.item_count / 2U;
    }

    demo_remove_item_at(remove_index);
    if (viewport_context.item_count == 0U)
    {
        viewport_context.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }
    else if (remove_index >= viewport_context.item_count)
    {
        viewport_context.selected_id = viewport_context.items[viewport_context.item_count - 1U].stable_id;
    }
    else
    {
        viewport_context.selected_id = viewport_context.items[remove_index].stable_id;
    }

    viewport_context.action_count++;
    viewport_context.mutation_count++;
    snprintf(viewport_context.last_action_text, sizeof(viewport_context.last_action_text), "delete @%04lu", (unsigned long)remove_index);
    egui_view_virtual_list_notify_item_removed(EGUI_VIEW_OF(&viewport_1), remove_index, 1);
    demo_update_status_labels();
}

static void demo_action_move(void)
{
    int32_t selected_index = demo_find_index_by_stable_id(viewport_context.selected_id);
    uint32_t from_index;
    uint32_t to_index;
    uint32_t moved_stable_id;

    if (viewport_context.item_count < 3U)
    {
        return;
    }

    from_index = selected_index >= 0 ? (uint32_t)selected_index : (viewport_context.item_count / 2U);
    if (viewport_context.scene == DEMO_SCENE_CHAT)
    {
        to_index = viewport_context.item_count / 4U;
    }
    else if (viewport_context.scene == DEMO_SCENE_TASK)
    {
        to_index = 0U;
    }
    else
    {
        to_index = 1U;
    }
    if (from_index == to_index)
    {
        to_index = (to_index + 3U < viewport_context.item_count) ? (to_index + 3U) : 0U;
    }

    moved_stable_id = viewport_context.items[from_index].stable_id;
    demo_move_item(from_index, to_index);
    viewport_context.action_count++;
    viewport_context.mutation_count++;
    snprintf(viewport_context.last_action_text, sizeof(viewport_context.last_action_text), "move %04lu -> %04lu", (unsigned long)from_index,
             (unsigned long)to_index);
    egui_view_virtual_list_notify_item_moved(EGUI_VIEW_OF(&viewport_1), from_index, to_index);
    egui_view_virtual_list_scroll_to_stable_id(EGUI_VIEW_OF(&viewport_1), moved_stable_id, 0);
    demo_update_status_labels();
}

static void demo_action_patch(void)
{
    int32_t selected_index = demo_find_index_by_stable_id(viewport_context.selected_id);
    uint32_t target_index = selected_index >= 0 ? (uint32_t)selected_index : viewport_context.item_count / 3U;
    demo_virtual_item_t *item = demo_get_item(target_index);

    if (item == NULL)
    {
        return;
    }

    demo_patch_item(item);
    viewport_context.action_count++;
    viewport_context.mutation_count++;
    snprintf(viewport_context.last_action_text, sizeof(viewport_context.last_action_text), "patch @%04lu", (unsigned long)target_index);
    egui_view_virtual_list_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&viewport_1), item->stable_id);
    egui_view_virtual_list_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&viewport_1), item->stable_id);
    if (target_index < viewport_context.item_count)
    {
        demo_keep_item_fully_visible(target_index, viewport_context.items[target_index].stable_id);
    }
    demo_update_status_labels();
}

static void demo_action_jump(void)
{
    uint32_t target_index;
    uint32_t step;

    if (viewport_context.item_count == 0U)
    {
        return;
    }

    step = (viewport_context.scene == DEMO_SCENE_CHAT) ? 73U : ((viewport_context.scene == DEMO_SCENE_TASK) ? 29U : 157U);
    target_index = (viewport_context.jump_cursor * step + 3U) % viewport_context.item_count;
    viewport_context.jump_cursor++;
    viewport_context.action_count++;
    snprintf(viewport_context.last_action_text, sizeof(viewport_context.last_action_text), "jump -> %04lu", (unsigned long)target_index);
    egui_view_virtual_list_scroll_to_stable_id(EGUI_VIEW_OF(&viewport_1), viewport_context.items[target_index].stable_id, 0);
    demo_update_status_labels();
}

static void demo_apply_action(uint8_t action)
{
    switch (action)
    {
    case DEMO_ACTION_ADD:
        demo_action_add();
        break;
    case DEMO_ACTION_DEL:
        demo_action_del();
        break;
    case DEMO_ACTION_MOVE:
        demo_action_move();
        break;
    case DEMO_ACTION_PATCH:
        demo_action_patch();
        break;
    case DEMO_ACTION_JUMP:
        demo_action_jump();
        break;
    default:
        break;
    }
}

static void demo_bind_feed_row(demo_virtual_row_t *row, int pool_index, uint32_t index, const demo_virtual_item_t *item, uint8_t selected)
{
    uint8_t live = item->state == 1U;
    uint8_t warning = item->state == 2U;
    egui_dim_t row_width = demo_get_list_width();
    egui_dim_t surface_x = DEMO_ROW_SIDE_INSET;
    egui_dim_t surface_y = DEMO_ROW_GAP / 2;
    egui_dim_t surface_w = row_width - DEMO_ROW_SIDE_INSET * 2;
    egui_dim_t surface_h = (egui_dim_t)(demo_get_item_height_by_stable_id(item->stable_id) - DEMO_ROW_GAP);
    egui_dim_t badge_w = selected ? 56 : 46;
    egui_dim_t badge_x;
    egui_dim_t title_w;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t title_color;
    egui_color_t subtitle_color;
    egui_color_t meta_color;
    egui_background_t *badge_bg;
    egui_color_t badge_text_color;
    uint8_t show_subtitle;
    uint8_t show_progress;

    if (surface_h < 42)
    {
        surface_h = 42;
    }

    snprintf(viewport_context.title_texts[pool_index], sizeof(viewport_context.title_texts[pool_index]), "Node #%04lu", (unsigned long)index);
    show_subtitle = item->variant != 2U || selected;
    show_progress = item->variant == 0U || selected || warning;

    snprintf(viewport_context.subtitle_texts[pool_index], sizeof(viewport_context.subtitle_texts[pool_index]), "%s",
             selected ? "Selected row keeps pulse and metrics."
                      : (item->variant == 0U ? "Large feed row with wide spacing."
                                             : (item->variant == 1U ? "Detail row; overscan reuses slots."
                                                                    : "Compact row; tap to expand.")));
    snprintf(viewport_context.badge_texts[pool_index], sizeof(viewport_context.badge_texts[pool_index]), "%s",
             selected ? "SELECT" : (live ? "LIVE" : (warning ? "WARN" : (item->variant == 0U ? "HERO" : (item->variant == 1U ? "DETAIL" : "COMPACT")))));
    snprintf(viewport_context.meta_texts[pool_index], sizeof(viewport_context.meta_texts[pool_index]), "#%05lu  %u%%  h=%ld  r%u",
             (unsigned long)item->stable_id, (unsigned)item->progress, (long)demo_get_item_height_by_stable_id(item->stable_id), (unsigned)item->revision);

    egui_view_label_set_text(EGUI_VIEW_OF(&row->title), viewport_context.title_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->subtitle), viewport_context.subtitle_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->meta), viewport_context.meta_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->badge), viewport_context.badge_texts[pool_index]);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->subtitle), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->meta), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->badge), EGUI_ALIGN_CENTER);

    if (selected)
    {
        surface_color = EGUI_COLOR_HEX(0x2F5E91);
        border_color = EGUI_COLOR_WHITE;
        title_color = EGUI_COLOR_WHITE;
        subtitle_color = EGUI_COLOR_WHITE;
        meta_color = EGUI_COLOR_WHITE;
        badge_bg = EGUI_BG_OF(&badge_selected_bg);
        badge_text_color = EGUI_COLOR_HEX(0x2F5E91);
        row->progress.progress_color = EGUI_COLOR_WHITE;
        row->progress.bk_color = EGUI_COLOR_HEX(0x7698BF);
    }
    else
    {
        surface_color = (item->variant == 0U) ? EGUI_COLOR_HEX(0xF1F6FB) : ((item->variant == 1U) ? EGUI_COLOR_WHITE : EGUI_COLOR_HEX(0xEDF1F5));
        border_color = live ? EGUI_COLOR_HEX(0x2E9A6F) : (warning ? EGUI_COLOR_HEX(0xC9933B) : EGUI_COLOR_HEX(0xD5DEE8));
        title_color = EGUI_COLOR_HEX(0x233443);
        subtitle_color = EGUI_COLOR_HEX(0x586B7B);
        meta_color = live ? EGUI_COLOR_HEX(0x267C5A) : (warning ? EGUI_COLOR_HEX(0x8E6726) : EGUI_COLOR_HEX(0x6A7B8B));
        badge_bg = live ? EGUI_BG_OF(&badge_success_bg) : (warning ? EGUI_BG_OF(&badge_warning_bg) : EGUI_BG_OF(&badge_info_bg));
        badge_text_color = live ? EGUI_COLOR_WHITE : (warning ? EGUI_COLOR_BLACK : EGUI_COLOR_WHITE);
        row->progress.progress_color = live ? EGUI_COLOR_HEX(0x2E9A6F) : EGUI_COLOR_HEX(0x4B79B2);
        row->progress.bk_color = EGUI_COLOR_HEX(0xDCE6F0);
    }
    row->progress.control_color = row->progress.progress_color;

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&row->surface), surface_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&row->surface), selected ? 2 : 1, border_color);
    egui_view_set_background(EGUI_VIEW_OF(&row->badge), badge_bg);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->title), title_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->subtitle), subtitle_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->meta), meta_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->badge), badge_text_color, EGUI_ALPHA_100);

    badge_x = surface_w - badge_w - 12;
    title_w = badge_x - 16;
    if (title_w < 70)
    {
        title_w = 70;
    }

    egui_view_set_position(EGUI_VIEW_OF(&row->surface), surface_x, surface_y);
    egui_view_set_size(EGUI_VIEW_OF(&row->surface), surface_w, surface_h);
    egui_view_set_position(EGUI_VIEW_OF(&row->badge), badge_x, 8);
    egui_view_set_size(EGUI_VIEW_OF(&row->badge), badge_w, DEMO_BADGE_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->pulse), badge_x - 16, 13);
    egui_view_set_size(EGUI_VIEW_OF(&row->pulse), 9, 9);
    egui_view_set_position(EGUI_VIEW_OF(&row->title), 12, 8);
    egui_view_set_size(EGUI_VIEW_OF(&row->title), title_w, DEMO_TITLE_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->subtitle), 12, 25);
    egui_view_set_size(EGUI_VIEW_OF(&row->subtitle), surface_w - 24, DEMO_TEXT_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->meta), 12, show_progress ? (surface_h - 27) : (surface_h - 18));
    egui_view_set_size(EGUI_VIEW_OF(&row->meta), surface_w - 24, DEMO_TEXT_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->progress), 12, surface_h - 10);
    egui_view_set_size(EGUI_VIEW_OF(&row->progress), surface_w - 24, DEMO_PROGRESS_H);

    egui_view_set_gone(EGUI_VIEW_OF(&row->subtitle), show_subtitle ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&row->progress), show_progress ? 0 : 1);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&row->progress), item->progress);
    demo_set_row_pulse(row, item, live || warning || selected, selected);
}

static void demo_bind_chat_row(demo_virtual_row_t *row, int pool_index, uint32_t index, const demo_virtual_item_t *item, uint8_t selected)
{
    uint8_t outgoing = item->variant == 1U;
    uint8_t system_msg = item->variant == 2U;
    uint8_t unread = item->state == 1U;
    uint8_t typing = item->state == 2U;
    egui_dim_t row_width = demo_get_list_width();
    egui_dim_t surface_y = DEMO_ROW_GAP / 2;
    egui_dim_t surface_h = (egui_dim_t)(demo_get_item_height_by_stable_id(item->stable_id) - DEMO_ROW_GAP);
    egui_dim_t surface_w = system_msg ? (row_width - 26) : (selected ? (row_width - 30) : (row_width - 42));
    egui_dim_t surface_x;
    egui_dim_t badge_w = system_msg ? 34 : 38;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t meta_color;
    egui_background_t *badge_bg;
    egui_color_t badge_text_color;
    egui_dim_t badge_x;

    if (surface_h < 42)
    {
        surface_h = 42;
    }
    if (surface_w < 120)
    {
        surface_w = 120;
    }

    if (system_msg)
    {
        surface_x = 17;
    }
    else if (outgoing)
    {
        surface_x = row_width - surface_w - 8;
    }
    else
    {
        surface_x = 8;
    }

    snprintf(viewport_context.title_texts[pool_index], sizeof(viewport_context.title_texts[pool_index]), "%s",
             system_msg ? "System" : (outgoing ? "You" : "Ops Bot"));
    snprintf(viewport_context.subtitle_texts[pool_index], sizeof(viewport_context.subtitle_texts[pool_index]), "%s",
             typing ? "Typing row stays alive offscreen."
                    : (unread ? "Unread bubble supports quick jump."
                              : (outgoing ? "Outbound bubble keeps stable id." : "Inbound bubble reuses slot.")));
    snprintf(viewport_context.badge_texts[pool_index], sizeof(viewport_context.badge_texts[pool_index]), "%s",
             selected ? "OPEN" : (system_msg ? "SYS" : (outgoing ? "YOU" : "BOT")));
    snprintf(viewport_context.meta_texts[pool_index], sizeof(viewport_context.meta_texts[pool_index]), "#%05lu  i%04lu  r%u",
             (unsigned long)item->stable_id, (unsigned long)index, (unsigned)item->revision);

    egui_view_label_set_text(EGUI_VIEW_OF(&row->title), viewport_context.title_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->subtitle), viewport_context.subtitle_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->meta), viewport_context.meta_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->badge), viewport_context.badge_texts[pool_index]);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->title),
                                   outgoing ? (EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER) : (EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER));
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->subtitle),
                                   outgoing ? (EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER) : (EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER));
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->meta),
                                   outgoing ? (EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER) : (EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER));
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->badge), EGUI_ALIGN_CENTER);

    if (selected)
    {
        surface_color = EGUI_COLOR_HEX(0x2F5E91);
        border_color = EGUI_COLOR_WHITE;
    }
    else if (system_msg)
    {
        surface_color = EGUI_COLOR_HEX(0xFFF4E2);
        border_color = EGUI_COLOR_HEX(0xE5C07A);
    }
    else if (outgoing)
    {
        surface_color = EGUI_COLOR_HEX(0xDCF3E7);
        border_color = EGUI_COLOR_HEX(0x75B195);
    }
    else
    {
        surface_color = EGUI_COLOR_HEX(0xF7FAFD);
        border_color = EGUI_COLOR_HEX(0xD1DAE5);
    }
    text_color = selected ? EGUI_COLOR_WHITE : EGUI_COLOR_HEX(0x243648);
    meta_color = selected ? EGUI_COLOR_WHITE : (typing ? EGUI_COLOR_HEX(0x2C77A8) : (unread ? EGUI_COLOR_HEX(0x8A5D1E) : EGUI_COLOR_HEX(0x677989)));
    badge_bg = selected ? EGUI_BG_OF(&badge_selected_bg) : (system_msg ? EGUI_BG_OF(&badge_warning_bg) : (outgoing ? EGUI_BG_OF(&badge_success_bg)
                                                                                                                : EGUI_BG_OF(&badge_info_bg)));
    badge_text_color = selected ? EGUI_COLOR_HEX(0x2F5E91) : (system_msg ? EGUI_COLOR_BLACK : EGUI_COLOR_WHITE);
    row->progress.progress_color = selected ? EGUI_COLOR_WHITE : (outgoing ? EGUI_COLOR_HEX(0x2E9A6F) : EGUI_COLOR_HEX(0x4B79B2));
    row->progress.bk_color = selected ? EGUI_COLOR_HEX(0x7698BF) : EGUI_COLOR_HEX(0xDCE6F0);
    row->progress.control_color = row->progress.progress_color;

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&row->surface), surface_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&row->surface), selected ? 2 : 1, border_color);
    egui_view_set_background(EGUI_VIEW_OF(&row->badge), badge_bg);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->title), text_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->subtitle), text_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->meta), meta_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->badge), badge_text_color, EGUI_ALPHA_100);

    badge_x = outgoing ? 10 : (surface_w - badge_w - 10);
    egui_view_set_position(EGUI_VIEW_OF(&row->surface), surface_x, surface_y);
    egui_view_set_size(EGUI_VIEW_OF(&row->surface), surface_w, surface_h);
    egui_view_set_position(EGUI_VIEW_OF(&row->badge), badge_x, 8);
    egui_view_set_size(EGUI_VIEW_OF(&row->badge), badge_w, DEMO_BADGE_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->pulse), outgoing ? (surface_w - 18) : (badge_x - 14), 12);
    egui_view_set_size(EGUI_VIEW_OF(&row->pulse), 9, 9);
    egui_view_set_position(EGUI_VIEW_OF(&row->title), 12, 8);
    egui_view_set_size(EGUI_VIEW_OF(&row->title), surface_w - 24, DEMO_TEXT_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->subtitle), 12, 24);
    egui_view_set_size(EGUI_VIEW_OF(&row->subtitle), surface_w - 24, DEMO_TEXT_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->meta), 12, (typing || selected) ? (surface_h - 24) : (surface_h - 16));
    egui_view_set_size(EGUI_VIEW_OF(&row->meta), surface_w - 24, DEMO_TEXT_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->progress), 12, surface_h - 10);
    egui_view_set_size(EGUI_VIEW_OF(&row->progress), surface_w - 24, DEMO_PROGRESS_H);
    egui_view_set_gone(EGUI_VIEW_OF(&row->progress), (typing || selected) ? 0 : 1);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&row->progress), item->progress);
    demo_set_row_pulse(row, item, unread || typing || selected, selected);
}

static void demo_bind_task_row(demo_virtual_row_t *row, int pool_index, uint32_t index, const demo_virtual_item_t *item, uint8_t selected)
{
    static const char *state_names[] = {"QUEUE", "RUN", "BLOCK", "DONE"};
    egui_dim_t row_width = demo_get_list_width();
    egui_dim_t surface_y = DEMO_ROW_GAP / 2;
    egui_dim_t surface_h = (egui_dim_t)(demo_get_item_height_by_stable_id(item->stable_id) - DEMO_ROW_GAP);
    egui_dim_t surface_w = row_width - DEMO_ROW_SIDE_INSET * 2;
    egui_dim_t badge_x;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t meta_color;
    egui_background_t *badge_bg;
    egui_color_t badge_text_color;

    if (surface_h < 44)
    {
        surface_h = 44;
    }

    snprintf(viewport_context.title_texts[pool_index], sizeof(viewport_context.title_texts[pool_index]), "Task #%04lu", (unsigned long)index);
    snprintf(viewport_context.subtitle_texts[pool_index], sizeof(viewport_context.subtitle_texts[pool_index]), "%s",
             item->state == 2U ? "Blocked stage keeps alert alive."
                               : (item->state == 1U ? "Running task supports move + patch."
                                                    : (item->state == 3U ? "Done task still reuses slot view."
                                                                         : "Queued item covers insert/remove.")));
    snprintf(viewport_context.badge_texts[pool_index], sizeof(viewport_context.badge_texts[pool_index]), "%s",
             selected ? "FOCUS" : state_names[item->state]);
    snprintf(viewport_context.meta_texts[pool_index], sizeof(viewport_context.meta_texts[pool_index]), "ow%02lu  p%u%%  #%05lu",
             (unsigned long)(item->stable_id % 17U), (unsigned)item->progress, (unsigned long)item->stable_id);

    egui_view_label_set_text(EGUI_VIEW_OF(&row->title), viewport_context.title_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->subtitle), viewport_context.subtitle_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->meta), viewport_context.meta_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->badge), viewport_context.badge_texts[pool_index]);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->subtitle), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->meta), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->badge), EGUI_ALIGN_CENTER);

    if (selected)
    {
        surface_color = EGUI_COLOR_HEX(0x2F5E91);
        border_color = EGUI_COLOR_WHITE;
        meta_color = EGUI_COLOR_WHITE;
        badge_bg = EGUI_BG_OF(&badge_selected_bg);
        badge_text_color = EGUI_COLOR_HEX(0x2F5E91);
        row->progress.progress_color = EGUI_COLOR_WHITE;
        row->progress.bk_color = EGUI_COLOR_HEX(0x7698BF);
    }
    else if (item->state == 2U)
    {
        surface_color = EGUI_COLOR_HEX(0xFFF0E0);
        border_color = EGUI_COLOR_HEX(0xD29056);
        meta_color = EGUI_COLOR_HEX(0x8B5A27);
        badge_bg = EGUI_BG_OF(&badge_warning_bg);
        badge_text_color = EGUI_COLOR_BLACK;
        row->progress.progress_color = EGUI_COLOR_HEX(0xD07D33);
        row->progress.bk_color = EGUI_COLOR_HEX(0xF3DEC7);
    }
    else if (item->state == 1U)
    {
        surface_color = EGUI_COLOR_HEX(0xE8F4FF);
        border_color = EGUI_COLOR_HEX(0x74A6D5);
        meta_color = EGUI_COLOR_HEX(0x356689);
        badge_bg = EGUI_BG_OF(&badge_info_bg);
        badge_text_color = EGUI_COLOR_WHITE;
        row->progress.progress_color = EGUI_COLOR_HEX(0x4D82B7);
        row->progress.bk_color = EGUI_COLOR_HEX(0xD7E6F3);
    }
    else if (item->state == 3U)
    {
        surface_color = EGUI_COLOR_HEX(0xE8F3EA);
        border_color = EGUI_COLOR_HEX(0x7BA786);
        meta_color = EGUI_COLOR_HEX(0x447053);
        badge_bg = EGUI_BG_OF(&badge_success_bg);
        badge_text_color = EGUI_COLOR_WHITE;
        row->progress.progress_color = EGUI_COLOR_HEX(0x2E9A6F);
        row->progress.bk_color = EGUI_COLOR_HEX(0xD5E8DC);
    }
    else
    {
        surface_color = EGUI_COLOR_HEX(0xF6F8FA);
        border_color = EGUI_COLOR_HEX(0xD4DCE5);
        meta_color = EGUI_COLOR_HEX(0x697987);
        badge_bg = EGUI_BG_OF(&badge_muted_bg);
        badge_text_color = EGUI_COLOR_WHITE;
        row->progress.progress_color = EGUI_COLOR_HEX(0x7A8EA4);
        row->progress.bk_color = EGUI_COLOR_HEX(0xE2E8EE);
    }
    row->progress.control_color = row->progress.progress_color;

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&row->surface), surface_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&row->surface), selected ? 2 : 1, border_color);
    egui_view_set_background(EGUI_VIEW_OF(&row->badge), badge_bg);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->title), selected ? EGUI_COLOR_WHITE : EGUI_COLOR_HEX(0x233443), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->subtitle), selected ? EGUI_COLOR_WHITE : EGUI_COLOR_HEX(0x556979), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->meta), meta_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->badge), badge_text_color, EGUI_ALPHA_100);

    badge_x = surface_w - 58;
    egui_view_set_position(EGUI_VIEW_OF(&row->surface), DEMO_ROW_SIDE_INSET, surface_y);
    egui_view_set_size(EGUI_VIEW_OF(&row->surface), surface_w, surface_h);
    egui_view_set_position(EGUI_VIEW_OF(&row->badge), badge_x, 8);
    egui_view_set_size(EGUI_VIEW_OF(&row->badge), 48, DEMO_BADGE_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->pulse), 10, 13);
    egui_view_set_size(EGUI_VIEW_OF(&row->pulse), 9, 9);
    egui_view_set_position(EGUI_VIEW_OF(&row->title), 24, 8);
    egui_view_set_size(EGUI_VIEW_OF(&row->title), badge_x - 30, DEMO_TITLE_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->subtitle), 24, 25);
    egui_view_set_size(EGUI_VIEW_OF(&row->subtitle), surface_w - 34, DEMO_TEXT_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->meta), 24, surface_h - 27);
    egui_view_set_size(EGUI_VIEW_OF(&row->meta), surface_w - 34, DEMO_TEXT_H);
    egui_view_set_position(EGUI_VIEW_OF(&row->progress), 24, surface_h - 10);
    egui_view_set_size(EGUI_VIEW_OF(&row->progress), surface_w - 36, DEMO_PROGRESS_H);
    egui_view_set_gone(EGUI_VIEW_OF(&row->subtitle), 0);
    egui_view_set_gone(EGUI_VIEW_OF(&row->progress), 0);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&row->progress), item->progress);
    demo_set_row_pulse(row, item, item->state == 1U || item->state == 2U || selected, selected);
}

static void demo_bind_row_text(demo_virtual_row_t *row, uint32_t index, uint32_t stable_id)
{
    int pool_index = demo_get_row_pool_index(row);
    const demo_virtual_item_t *item = demo_get_item_const(index);
    uint8_t selected = demo_is_selected_id(stable_id);

    if (pool_index < 0 || item == NULL)
    {
        return;
    }

    if (viewport_context.scene == DEMO_SCENE_CHAT)
    {
        demo_bind_chat_row(row, pool_index, index, item, selected);
    }
    else if (viewport_context.scene == DEMO_SCENE_TASK)
    {
        demo_bind_task_row(row, pool_index, index, item, selected);
    }
    else
    {
        demo_bind_feed_row(row, pool_index, index, item, selected);
    }
}

static void virtual_item_click_cb(egui_view_t *self)
{
    demo_virtual_row_t *row = demo_find_row_by_root_view(self);
    uint32_t previous_selected = viewport_context.selected_id;
    int32_t previous_index = demo_find_index_by_stable_id(previous_selected);

    if (row == NULL || row->stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return;
    }

    viewport_context.selected_id = row->stable_id;
    viewport_context.last_clicked_index = row->bound_index;
    viewport_context.click_count++;
    snprintf(viewport_context.last_action_text, sizeof(viewport_context.last_action_text), "click row %04lu", (unsigned long)row->bound_index);

    if (previous_selected != viewport_context.selected_id && previous_index >= 0)
    {
        egui_view_virtual_list_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&viewport_1), previous_selected);
    }
    egui_view_virtual_list_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&viewport_1), row->stable_id);
    demo_keep_item_fully_visible(row->bound_index, row->stable_id);
    demo_update_status_labels();

    EGUI_LOG_INF("Virtual row clicked: scene=%s index=%d stable_id=%lu\n", demo_scene_names[viewport_context.scene], (int)row->bound_index,
                 (unsigned long)row->stable_id);
}

static uint32_t demo_get_count(void *adapter_context)
{
    return ((demo_virtual_viewport_context_t *)adapter_context)->item_count;
}

static uint32_t demo_get_stable_id(void *adapter_context, uint32_t index)
{
    demo_virtual_viewport_context_t *ctx = (demo_virtual_viewport_context_t *)adapter_context;

    if (index >= ctx->item_count)
    {
        return EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    return ctx->items[index].stable_id;
}

static int32_t demo_adapter_find_index_by_stable_id(void *adapter_context, uint32_t stable_id)
{
    demo_virtual_viewport_context_t *ctx = (demo_virtual_viewport_context_t *)adapter_context;
    uint32_t i;

    for (i = 0; i < ctx->item_count; i++)
    {
        if (ctx->items[i].stable_id == stable_id)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static int32_t demo_measure_item_height(void *adapter_context, uint32_t index, int32_t cross_size_hint)
{
    demo_virtual_viewport_context_t *ctx = (demo_virtual_viewport_context_t *)adapter_context;

    EGUI_UNUSED(cross_size_hint);
    if (index >= ctx->item_count)
    {
        return 64;
    }

    return demo_get_item_height_by_stable_id(ctx->items[index].stable_id);
}

static egui_view_t *demo_create_item_view(void *adapter_context, uint16_t view_type)
{
    demo_virtual_viewport_context_t *ctx = (demo_virtual_viewport_context_t *)adapter_context;
    demo_virtual_row_t *row;

    EGUI_UNUSED(view_type);

    if (ctx->created_count >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
    {
        return NULL;
    }

    row = &ctx->rows[ctx->created_count];
    memset(row, 0, sizeof(*row));
    row->bound_index = DEMO_INVALID_INDEX;
    row->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;

    egui_view_group_init(EGUI_VIEW_OF(&row->root));
    egui_view_card_init(EGUI_VIEW_OF(&row->surface));
    egui_view_label_init(EGUI_VIEW_OF(&row->title));
    egui_view_label_init(EGUI_VIEW_OF(&row->subtitle));
    egui_view_label_init(EGUI_VIEW_OF(&row->meta));
    egui_view_label_init(EGUI_VIEW_OF(&row->badge));
    egui_view_progress_bar_init(EGUI_VIEW_OF(&row->progress));
    egui_view_init(EGUI_VIEW_OF(&row->pulse));

    egui_view_label_set_font(EGUI_VIEW_OF(&row->title), DEMO_FONT_TITLE);
    egui_view_label_set_font(EGUI_VIEW_OF(&row->subtitle), DEMO_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&row->meta), DEMO_FONT_CAP);
    egui_view_label_set_font(EGUI_VIEW_OF(&row->badge), DEMO_FONT_CAP);

    egui_view_group_add_child(EGUI_VIEW_OF(&row->root), EGUI_VIEW_OF(&row->surface));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->title));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->subtitle));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->meta));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->badge));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->progress));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->pulse));
    egui_view_set_shadow(EGUI_VIEW_OF(&row->surface), &demo_card_shadow);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&row->root), virtual_item_click_cb);

    egui_animation_alpha_init(EGUI_ANIM_OF(&row->pulse_anim));
    egui_animation_alpha_params_set(&row->pulse_anim, &pulse_anim_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&row->pulse_anim), 900);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&row->pulse_anim), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&row->pulse_anim), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_interpolator_linear_init((egui_interpolator_t *)&row->pulse_interp);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&row->pulse_anim), (egui_interpolator_t *)&row->pulse_interp);
    egui_animation_target_view_set(EGUI_ANIM_OF(&row->pulse_anim), EGUI_VIEW_OF(&row->pulse));
    egui_view_set_gone(EGUI_VIEW_OF(&row->pulse), 1);

    ctx->created_count++;
    return EGUI_VIEW_OF(&row->root);
}

static void demo_bind_item_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    demo_virtual_row_t *row = demo_find_row_by_root_view(view);

    EGUI_UNUSED(adapter_context);

    if (row == NULL)
    {
        return;
    }

    row->bound_index = index;
    row->stable_id = stable_id;
    demo_bind_row_text(row, index, stable_id);
}

static void demo_unbind_item_view(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    demo_virtual_row_t *row = demo_find_row_by_root_view(view);

    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(stable_id);

    if (row == NULL)
    {
        return;
    }

    demo_set_row_pulse(row, NULL, 0, 0);
    row->bound_index = DEMO_INVALID_INDEX;
    row->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static void demo_save_item_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    demo_virtual_row_t *row = demo_find_row_by_root_view(view);
    demo_virtual_row_state_t state;

    EGUI_UNUSED(adapter_context);

    if (row == NULL)
    {
        return;
    }

    demo_capture_row_state(row, &state);
    if (!state.pulse_running)
    {
        egui_view_virtual_list_remove_item_state_by_stable_id(EGUI_VIEW_OF(&viewport_1), stable_id);
        return;
    }

    (void)egui_view_virtual_list_write_item_state_for_view(view, stable_id, &state, sizeof(state));
}

static void demo_restore_item_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    demo_virtual_row_t *row = demo_find_row_by_root_view(view);
    demo_virtual_row_state_t state;

    EGUI_UNUSED(adapter_context);

    if (row == NULL)
    {
        return;
    }

    if (egui_view_virtual_list_read_item_state_for_view(view, stable_id, &state, sizeof(state)) != sizeof(state))
    {
        return;
    }

    demo_restore_row_state(row, &state);
}

static uint8_t demo_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    demo_virtual_viewport_context_t *ctx = (demo_virtual_viewport_context_t *)adapter_context;
    int32_t index;

    EGUI_UNUSED(view);

    if (stable_id == ctx->selected_id)
    {
        return 1;
    }

    index = demo_adapter_find_index_by_stable_id(adapter_context, stable_id);
    if (index < 0)
    {
        return 0;
    }

    return demo_item_should_keepalive(&ctx->items[index]);
}

static const egui_view_virtual_list_data_source_t viewport_data_source = {
        .get_count = demo_get_count,
        .get_stable_id = demo_get_stable_id,
        .find_index_by_stable_id = demo_adapter_find_index_by_stable_id,
        .measure_item_height = demo_measure_item_height,
        .create_item_view = demo_create_item_view,
        .destroy_item_view = NULL,
        .bind_item_view = demo_bind_item_view,
        .unbind_item_view = demo_unbind_item_view,
        .should_keep_alive = demo_should_keep_alive,
        .save_item_state = demo_save_item_state,
        .restore_item_state = demo_restore_item_state,
        .default_view_type = 0,
};

static int demo_find_scene_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < DEMO_SCENE_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&scene_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static int demo_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < DEMO_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void scene_button_click_cb(egui_view_t *self)
{
    int scene = demo_find_scene_button_index(self);

    if (scene < 0)
    {
        return;
    }

    demo_switch_scene((uint8_t)scene);
}

static void action_button_click_cb(egui_view_t *self)
{
    int action = demo_find_action_button_index(self);

    if (action < 0)
    {
        return;
    }

    demo_apply_action((uint8_t)action);
}

static void demo_init_chip_button(egui_view_button_t *button, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const char *text,
                                  egui_view_on_click_listener_t listener)
{
    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_position(EGUI_VIEW_OF(button), x, y);
    egui_view_set_size(EGUI_VIEW_OF(button), width, height);
    egui_view_label_set_text(EGUI_VIEW_OF(button), text);
    egui_view_label_set_font(EGUI_VIEW_OF(button), DEMO_FONT_CAP);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), listener);
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

static uint8_t is_slot_rendered_in_viewport(const egui_view_virtual_viewport_slot_t *slot)
{
    int16_t main_center;
    int16_t viewport_extent;

    if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE)
    {
        return 0;
    }

    main_center = slot->render_region.location.y + slot->render_region.size.height / 2;
    viewport_extent = EGUI_VIEW_OF(&viewport_1)->region.size.height;

    return (main_center >= 0) && (main_center < viewport_extent);
}

static uint8_t is_slot_fully_visible_in_viewport(const egui_view_virtual_viewport_slot_t *slot)
{
    int16_t viewport_extent;
    int16_t top;
    int16_t bottom;

    if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE)
    {
        return 0;
    }

    top = slot->render_region.location.y;
    bottom = slot->render_region.location.y + slot->render_region.size.height;
    viewport_extent = EGUI_VIEW_OF(&viewport_1)->region.size.height;

    return top >= 0 && bottom <= viewport_extent;
}

static egui_view_t *find_visible_view_by_item_index(uint32_t index)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        const egui_view_virtual_list_slot_t *slot = egui_view_virtual_list_get_slot(EGUI_VIEW_OF(&viewport_1), i);

        if (slot != NULL && slot->index == index && is_slot_rendered_in_viewport(slot))
        {
            return slot->view;
        }
    }

    return NULL;
}

static egui_view_t *find_first_visible_view_after(uint32_t min_index)
{
    uint8_t i;
    const egui_view_virtual_viewport_slot_t *best = NULL;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        const egui_view_virtual_list_slot_t *slot = egui_view_virtual_list_get_slot(EGUI_VIEW_OF(&viewport_1), i);

        if (slot == NULL || slot->index < min_index || !is_slot_rendered_in_viewport(slot))
        {
            continue;
        }
        if (best == NULL || slot->index < best->index)
        {
            best = slot;
        }
    }

    return best != NULL ? best->view : NULL;
}

static egui_view_t *find_first_fully_visible_view_after(uint32_t min_index)
{
    uint8_t i;
    const egui_view_virtual_viewport_slot_t *best = NULL;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        const egui_view_virtual_list_slot_t *slot = egui_view_virtual_list_get_slot(EGUI_VIEW_OF(&viewport_1), i);

        if (slot == NULL || slot->index < min_index || !is_slot_fully_visible_in_viewport(slot))
        {
            continue;
        }
        if (best == NULL || slot->index < best->index)
        {
            best = slot;
        }
    }

    return best != NULL ? best->view : NULL;
}
#endif

void test_init_ui(void)
{
    uint8_t i;

    memset(&viewport_context, 0, sizeof(viewport_context));
    viewport_context.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    viewport_context.last_clicked_index = DEMO_INVALID_INDEX;
    demo_reset_scene(DEMO_SCENE_FEED);

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&header_card), &header_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_card), 1, EGUI_COLOR_HEX(0xD7E1EA));
    egui_view_set_shadow(EGUI_VIEW_OF(&header_card), &demo_card_shadow);

    egui_view_label_init(EGUI_VIEW_OF(&header_title));
    egui_view_label_init(EGUI_VIEW_OF(&header_detail));
    egui_view_label_init(EGUI_VIEW_OF(&header_hint));

    egui_view_label_set_font(EGUI_VIEW_OF(&header_title), DEMO_FONT_HEADER);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_detail), DEMO_FONT_CAP);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_hint), DEMO_FONT_CAP);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_detail), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_hint), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_title), EGUI_COLOR_HEX(0x25384A), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_detail), EGUI_COLOR_HEX(0x506477), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_hint), EGUI_COLOR_HEX(0x687B8D), EGUI_ALPHA_100);
    egui_view_set_position(EGUI_VIEW_OF(&header_title), 12, 9);
    egui_view_set_size(EGUI_VIEW_OF(&header_title), DEMO_HEADER_W - 24, DEMO_TITLE_H);
    egui_view_set_position(EGUI_VIEW_OF(&header_detail), 12, 25);
    egui_view_set_size(EGUI_VIEW_OF(&header_detail), DEMO_HEADER_W - 24, DEMO_TEXT_H);
    egui_view_set_position(EGUI_VIEW_OF(&header_hint), 12, 39);
    egui_view_set_size(EGUI_VIEW_OF(&header_hint), DEMO_HEADER_W - 24, DEMO_TEXT_H);

    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_title));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_detail));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_hint));

    for (i = 0; i < DEMO_SCENE_COUNT; i++)
    {
        demo_init_chip_button(&scene_buttons[i], 12 + i * (DEMO_SCENE_BUTTON_W + DEMO_SCENE_BUTTON_GAP), 55, DEMO_SCENE_BUTTON_W, DEMO_CHIP_H,
                              demo_scene_names[i],
                              scene_button_click_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&scene_buttons[i]));
    }

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD7E1EA));

    for (i = 0; i < DEMO_ACTION_COUNT; i++)
    {
        demo_init_chip_button(&action_buttons[i], 10 + i * (DEMO_ACTION_BUTTON_W + DEMO_ACTION_BUTTON_GAP), 6, DEMO_ACTION_BUTTON_W, DEMO_CHIP_H,
                              demo_action_names[i],
                              action_button_click_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
    }
    demo_style_action_buttons();

    egui_view_virtual_list_init_with_params(EGUI_VIEW_OF(&viewport_1), &viewport_1_params);
    egui_view_set_background(EGUI_VIEW_OF(&viewport_1), EGUI_BG_OF(&list_bg));
    egui_view_set_shadow(EGUI_VIEW_OF(&viewport_1), &demo_card_shadow);
    egui_view_virtual_list_set_data_source(EGUI_VIEW_OF(&viewport_1), &viewport_data_source, &viewport_context);
    egui_view_virtual_list_set_estimated_item_height(EGUI_VIEW_OF(&viewport_1), 72);
    egui_view_virtual_list_set_overscan(EGUI_VIEW_OF(&viewport_1), 1, 1);
    egui_view_virtual_list_set_keepalive_limit(EGUI_VIEW_OF(&viewport_1), 4);
    egui_view_virtual_list_set_state_cache_limits(EGUI_VIEW_OF(&viewport_1), DEMO_STATE_CACHE_COUNT,
                                                  DEMO_STATE_CACHE_COUNT * (uint32_t)sizeof(demo_virtual_row_state_t));

    viewport_context.last_action_text[0] = '\0';
    demo_update_status_labels();

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewport_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&header_card));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    egui_view_t *view;
    int first_call = (action_index != last_action);

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call && viewport_context.created_count > EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
        {
            report_runtime_failure("virtual list created more views than slot capacity");
        }
        view = find_visible_view_by_item_index(1);
        if (view == NULL)
        {
            report_runtime_failure("initial feed row was not visible");
            EGUI_SIM_SET_WAIT(p_action, 200);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 220);
        return true;
    case 1:
        if (first_call && viewport_context.last_clicked_index != 1U)
        {
            report_runtime_failure("feed row click did not update selected index");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_PATCH]), 180);
        return true;
    case 2:
        if (first_call && viewport_context.mutation_count < 1U)
        {
            report_runtime_failure("feed patch did not mutate data");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_MOVE]), 180);
        return true;
    case 3:
        if (first_call && viewport_context.mutation_count < 2U)
        {
            report_runtime_failure("feed move did not mutate data");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_ADD]), 180);
        return true;
    case 4:
        if (first_call && viewport_context.item_count != DEMO_FEED_ITEMS + 1U)
        {
            report_runtime_failure("feed insert did not change item count");
        }
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
        view = find_first_fully_visible_view_after(3);
        if (view == NULL)
        {
            report_runtime_failure("feed row after drag was not visible");
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 180);
        return true;
    case 7:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&scene_buttons[DEMO_SCENE_CHAT]), 220);
        return true;
    case 8:
        if (first_call && viewport_context.scene != DEMO_SCENE_CHAT)
        {
            report_runtime_failure("scene switch to chat failed");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_JUMP]), 180);
        return true;
    case 9:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 4 / 5;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 5;
        p_action->steps = 4;
        p_action->interval_ms = 520;
        return true;
    case 10:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 4;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 3;
        p_action->steps = 5;
        p_action->interval_ms = 620;
        return true;
    case 11:
        view = find_first_fully_visible_view_after(4);
        if (view == NULL)
        {
            report_runtime_failure("chat row after drag was not visible");
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 180);
        return true;
    case 12:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_PATCH]), 180);
        return true;
    case 13:
        if (first_call && viewport_context.mutation_count < 4U)
        {
            report_runtime_failure("chat patch did not mutate data");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&scene_buttons[DEMO_SCENE_TASK]), 220);
        return true;
    case 14:
        if (first_call && viewport_context.scene != DEMO_SCENE_TASK)
        {
            report_runtime_failure("scene switch to ops failed");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_DEL]), 180);
        return true;
    case 15:
        if (first_call && viewport_context.mutation_count < 5U)
        {
            report_runtime_failure("ops delete did not mutate data");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[DEMO_ACTION_PATCH]), 180);
        return true;
    case 16:
        if (first_call && viewport_context.mutation_count < 6U)
        {
            report_runtime_failure("ops patch did not mutate data");
        }
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 4 / 5;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 5;
        p_action->steps = 4;
        p_action->interval_ms = 520;
        return true;
    case 17:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 4;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 3;
        p_action->steps = 5;
        p_action->interval_ms = 620;
        return true;
    case 18:
        view = find_first_fully_visible_view_after(2);
        if (view == NULL)
        {
            report_runtime_failure("ops row after drag was not visible");
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 180);
        return true;
    case 19:
        if (first_call)
        {
            if (viewport_context.mutation_count < 6U)
            {
                report_runtime_failure("runtime flow missed some data mutations");
            }
            if (viewport_context.scene_switch_count < 2U)
            {
                report_runtime_failure("runtime flow did not switch scenes twice");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    default:
        return false;
    }
}
#endif
