#include "egui.h"

#include <stdio.h>
#include <string.h>

#include "uicode.h"

#define GRID_MAX_ITEMS          420U
#define GRID_INITIAL_ITEMS      320U
#define GRID_INVALID_INDEX      0xFFFFFFFFUL
#define GRID_VIEW_POOL_CAPACITY (EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS * EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS)
#define GRID_STATE_CACHE_COUNT  96U

#define GRID_STATUS_TEXT_LEN 96
#define GRID_TITLE_TEXT_LEN  24
#define GRID_META_TEXT_LEN   24
#define GRID_BADGE_TEXT_LEN  12
#define GRID_TAG_TEXT_LEN    16
#define GRID_FOOTER_TEXT_LEN 28

#define GRID_MARGIN_X  8
#define GRID_TOP_Y     8
#define GRID_CONTENT_W (EGUI_CONFIG_SCEEN_WIDTH - GRID_MARGIN_X * 2)
#define GRID_HEADER_H  72
#define GRID_TOOLBAR_Y (GRID_TOP_Y + GRID_HEADER_H + 6)
#define GRID_TOOLBAR_H 56
#define GRID_VIEW_Y    (GRID_TOOLBAR_Y + GRID_TOOLBAR_H + 6)
#define GRID_VIEW_H    (EGUI_CONFIG_SCEEN_HEIGHT - GRID_VIEW_Y - 8)

#define GRID_ACTION_BUTTON_GAP 4
#define GRID_ACTION_BUTTON_W   ((GRID_CONTENT_W - 20 - GRID_ACTION_BUTTON_GAP * 3) / 4)
#define GRID_ACTION_BUTTON_H   18
#define GRID_MODE_BUTTON_GAP   4
#define GRID_MODE_BUTTON_W     ((GRID_CONTENT_W - 20 - GRID_MODE_BUTTON_GAP * 2) / 3)
#define GRID_MODE_BUTTON_H     18

#define GRID_CARD_INSET_X 4
#define GRID_CARD_INSET_Y 4

#define GRID_FONT_HEADER  ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define GRID_FONT_TITLE   ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define GRID_FONT_TITLE_L ((const egui_font_t *)&egui_res_font_montserrat_12_4)
#define GRID_FONT_META    ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define GRID_FONT_CAPTION ((const egui_font_t *)&egui_res_font_montserrat_8_4)

enum
{
    GRID_ACTION_ADD = 0,
    GRID_ACTION_DEL,
    GRID_ACTION_PATCH,
    GRID_ACTION_JUMP,
    GRID_ACTION_COUNT,
};

enum
{
    GRID_VARIANT_HERO = 0,
    GRID_VARIANT_METRIC,
    GRID_VARIANT_ALERT,
    GRID_VARIANT_TASK,
    GRID_VARIANT_COUNT,
};

enum
{
    GRID_STATE_IDLE = 0,
    GRID_STATE_SYNC,
    GRID_STATE_WARNING,
    GRID_STATE_DONE,
    GRID_STATE_COUNT,
};

typedef struct grid_demo_item grid_demo_item_t;
typedef struct grid_demo_item_view grid_demo_item_view_t;
typedef struct grid_demo_item_state grid_demo_item_state_t;
typedef struct grid_demo_context grid_demo_context_t;

struct grid_demo_item
{
    uint32_t stable_id;
    uint16_t revision;
    uint8_t variant;
    uint8_t state;
    uint8_t progress;
    uint8_t reserved;
};

struct grid_demo_item_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_card_t accent;
    egui_view_label_t tag;
    egui_view_label_t title;
    egui_view_label_t meta;
    egui_view_label_t badge;
    egui_view_label_t footer;
    egui_view_progress_bar_t progress;
    egui_view_t pulse;
    egui_animation_alpha_t pulse_anim;
    egui_interpolator_linear_t pulse_interp;
    uint32_t bound_index;
    uint32_t stable_id;
    uint8_t pulse_running;
};

struct grid_demo_item_state
{
    uint16_t pulse_elapsed_ms;
    uint8_t pulse_running;
    uint8_t pulse_alpha;
    uint8_t pulse_cycle_flip;
    uint8_t pulse_repeated;
};

struct grid_demo_context
{
    uint32_t item_count;
    uint32_t next_stable_id;
    uint32_t selected_id;
    uint32_t last_clicked_index;
    uint32_t click_count;
    uint32_t action_count;
    uint32_t mutation_cursor;
    uint8_t created_count;
    grid_demo_item_t items[GRID_MAX_ITEMS];
    char title_texts[GRID_VIEW_POOL_CAPACITY][GRID_TITLE_TEXT_LEN];
    char meta_texts[GRID_VIEW_POOL_CAPACITY][GRID_META_TEXT_LEN];
    char badge_texts[GRID_VIEW_POOL_CAPACITY][GRID_BADGE_TEXT_LEN];
    char tag_texts[GRID_VIEW_POOL_CAPACITY][GRID_TAG_TEXT_LEN];
    char footer_texts[GRID_VIEW_POOL_CAPACITY][GRID_FOOTER_TEXT_LEN];
    char header_title_text[GRID_STATUS_TEXT_LEN];
    char header_detail_text[GRID_STATUS_TEXT_LEN];
    char header_hint_text[GRID_STATUS_TEXT_LEN];
    char last_action_text[GRID_STATUS_TEXT_LEN];
    grid_demo_item_view_t item_views[GRID_VIEW_POOL_CAPACITY];
};

static const char *grid_demo_action_names[GRID_ACTION_COUNT] = {"Add", "Del", "Patch", "Jump"};
static const char *grid_demo_variant_names[GRID_VARIANT_COUNT] = {"Hero", "KPI", "Alert", "Task"};
static const char *grid_demo_variant_short_names[GRID_VARIANT_COUNT] = {"H", "K", "!", "T"};
static const char *grid_demo_state_names[GRID_STATE_COUNT] = {"Idle", "Sync", "Warn", "Done"};

static egui_view_t background_view;
static egui_view_card_t header_card;
static egui_view_card_t toolbar_card;
static egui_view_label_t header_title;
static egui_view_label_t header_detail;
static egui_view_label_t header_hint;
static egui_view_button_t action_buttons[GRID_ACTION_COUNT];
static egui_view_button_t mode_buttons[3];
static egui_view_virtual_grid_t grid_view;
static grid_demo_context_t grid_demo_ctx;

EGUI_VIEW_CARD_PARAMS_INIT(header_card_params, GRID_MARGIN_X, GRID_TOP_Y, GRID_CONTENT_W, GRID_HEADER_H, 14);
EGUI_VIEW_CARD_PARAMS_INIT(toolbar_card_params, GRID_MARGIN_X, GRID_TOOLBAR_Y, GRID_CONTENT_W, GRID_TOOLBAR_H, 12);
static const egui_view_virtual_grid_params_t grid_view_params = {
        .region = {{GRID_MARGIN_X, GRID_VIEW_Y}, {GRID_CONTENT_W, GRID_VIEW_H}},
        .column_count = 2,
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 6,
        .column_spacing = 6,
        .row_spacing = 6,
        .estimated_item_height = 76,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(grid_demo_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xF4F1EA), EGUI_COLOR_HEX(0xDCE8F2),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_screen_bg_params, &grid_demo_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(grid_demo_screen_bg, &grid_demo_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_demo_view_bg_param, EGUI_COLOR_HEX(0xF8FBFD), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_view_bg_params, &grid_demo_view_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_view_bg, &grid_demo_view_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_demo_btn_add_param, EGUI_COLOR_HEX(0xE4F4E7), EGUI_ALPHA_100, 9);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_btn_add_params, &grid_demo_btn_add_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_btn_add_bg, &grid_demo_btn_add_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_demo_btn_del_param, EGUI_COLOR_HEX(0xFFF0E4), EGUI_ALPHA_100, 9);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_btn_del_params, &grid_demo_btn_del_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_btn_del_bg, &grid_demo_btn_del_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_demo_btn_patch_param, EGUI_COLOR_HEX(0xE4F1F8), EGUI_ALPHA_100, 9);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_btn_patch_params, &grid_demo_btn_patch_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_btn_patch_bg, &grid_demo_btn_patch_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_demo_btn_jump_param, EGUI_COLOR_HEX(0xECE7FB), EGUI_ALPHA_100, 9);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_btn_jump_params, &grid_demo_btn_jump_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_btn_jump_bg, &grid_demo_btn_jump_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_demo_mode_idle_param, EGUI_COLOR_HEX(0xE8EEF3), EGUI_ALPHA_100, 9);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_mode_idle_params, &grid_demo_mode_idle_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_mode_idle_bg, &grid_demo_mode_idle_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_demo_mode_active_param, EGUI_COLOR_HEX(0xD5E8FF), EGUI_ALPHA_100, 9);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_mode_active_params, &grid_demo_mode_active_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_mode_active_bg, &grid_demo_mode_active_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_demo_badge_idle_param, EGUI_COLOR_HEX(0x8796A4), EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_badge_idle_params, &grid_demo_badge_idle_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_badge_idle_bg, &grid_demo_badge_idle_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_demo_badge_sync_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_badge_sync_params, &grid_demo_badge_sync_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_badge_sync_bg, &grid_demo_badge_sync_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_demo_badge_warn_param, EGUI_COLOR_HEX(0xD08A2E), EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_badge_warn_params, &grid_demo_badge_warn_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_badge_warn_bg, &grid_demo_badge_warn_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_demo_badge_done_param, EGUI_COLOR_HEX(0x52769A), EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_badge_done_params, &grid_demo_badge_done_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_badge_done_bg, &grid_demo_badge_done_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(grid_demo_pulse_selected_param, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_pulse_selected_params, &grid_demo_pulse_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_pulse_selected_bg, &grid_demo_pulse_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(grid_demo_pulse_sync_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_pulse_sync_params, &grid_demo_pulse_sync_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_pulse_sync_bg, &grid_demo_pulse_sync_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(grid_demo_pulse_warn_param, EGUI_COLOR_HEX(0xD08A2E), EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_pulse_warn_params, &grid_demo_pulse_warn_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_pulse_warn_bg, &grid_demo_pulse_warn_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(grid_demo_pulse_done_param, EGUI_COLOR_HEX(0x6C88B8), EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_PARAM_INIT(grid_demo_pulse_done_params, &grid_demo_pulse_done_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_demo_pulse_done_bg, &grid_demo_pulse_done_params);

EGUI_SHADOW_PARAM_INIT_ROUND(grid_demo_card_shadow, 10, 0, 3, EGUI_COLOR_BLACK, EGUI_ALPHA_20, 10);
EGUI_ANIMATION_ALPHA_PARAMS_INIT(grid_demo_pulse_anim_param, EGUI_ALPHA_100, EGUI_ALPHA_20);

static void grid_demo_refresh_header(void);
static void grid_demo_refresh_mode_buttons(void);
static void grid_demo_apply_action(uint8_t action);
static void grid_demo_apply_column_mode(uint8_t columns);

static void grid_demo_fill_item(grid_demo_item_t *item, uint32_t stable_id, uint32_t seed)
{
    uint8_t progress;

    memset(item, 0, sizeof(*item));
    item->stable_id = stable_id;
    item->revision = (uint16_t)(1U + (seed % 7U));
    item->variant = (uint8_t)(seed % GRID_VARIANT_COUNT);
    item->state = (uint8_t)((seed / 3U) % GRID_STATE_COUNT);
    progress = (uint8_t)(20U + ((seed * 11U) % 73U));

    if (item->variant == GRID_VARIANT_ALERT && item->state == GRID_STATE_IDLE)
    {
        item->state = GRID_STATE_WARNING;
    }
    if (item->state == GRID_STATE_DONE)
    {
        progress = 100U;
    }
    if (item->variant == GRID_VARIANT_HERO && progress < 48U)
    {
        progress = (uint8_t)(progress + 18U);
    }

    item->progress = progress;
}

static void grid_demo_reset_model(void)
{
    uint32_t i;

    memset(&grid_demo_ctx, 0, sizeof(grid_demo_ctx));
    grid_demo_ctx.item_count = GRID_INITIAL_ITEMS;
    grid_demo_ctx.next_stable_id = 500001U;
    grid_demo_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    grid_demo_ctx.last_clicked_index = GRID_INVALID_INDEX;

    for (i = 0; i < grid_demo_ctx.item_count; i++)
    {
        grid_demo_fill_item(&grid_demo_ctx.items[i], grid_demo_ctx.next_stable_id++, i + 3U);
    }

    snprintf(grid_demo_ctx.last_action_text, sizeof(grid_demo_ctx.last_action_text), "Tap a card. Switch columns to inspect width-adaptive layout.");
}

static int32_t grid_demo_find_index_by_stable_id(uint32_t stable_id)
{
    uint32_t i;

    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return -1;
    }

    for (i = 0; i < grid_demo_ctx.item_count; i++)
    {
        if (grid_demo_ctx.items[i].stable_id == stable_id)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static uint8_t grid_demo_get_column_count(void)
{
    return egui_view_virtual_grid_get_column_count(EGUI_VIEW_OF(&grid_view));
}

static int32_t grid_demo_get_item_width_hint(uint32_t index)
{
    int32_t width = egui_view_virtual_grid_get_item_width(EGUI_VIEW_OF(&grid_view), index);

    if (width > 0)
    {
        return width;
    }

    switch (grid_demo_get_column_count())
    {
    case 4:
        return 52;
    case 3:
        return 70;
    default:
        return 108;
    }
}

static int32_t grid_demo_measure_item_height_with_state(const grid_demo_item_t *item, int32_t width_hint, uint8_t selected)
{
    uint8_t compact = width_hint < 60 ? 1U : 0U;
    int32_t height;

    if (item == NULL)
    {
        return compact ? 56 : 72;
    }

    switch (item->variant)
    {
    case GRID_VARIANT_METRIC:
        height = compact ? 52 : 64;
        break;
    case GRID_VARIANT_ALERT:
        height = compact ? 62 : 78;
        break;
    case GRID_VARIANT_TASK:
        height = compact ? 68 : 84;
        break;
    default:
        height = compact ? 58 : 72;
        break;
    }

    if (item->state == GRID_STATE_SYNC)
    {
        height += compact ? 4 : 6;
    }
    else if (item->state == GRID_STATE_WARNING)
    {
        height += compact ? 6 : 8;
    }

    if (selected)
    {
        height += compact ? 10 : 12;
    }

    return height;
}

static uint8_t grid_demo_item_has_pulse(const grid_demo_item_t *item, uint8_t selected)
{
    if (item == NULL)
    {
        return 0;
    }

    return (uint8_t)(selected || item->state == GRID_STATE_SYNC || item->state == GRID_STATE_WARNING);
}

static int grid_demo_get_view_pool_index(grid_demo_item_view_t *item_view)
{
    uint8_t i;

    for (i = 0; i < grid_demo_ctx.created_count; i++)
    {
        if (item_view == &grid_demo_ctx.item_views[i])
        {
            return (int)i;
        }
    }

    return -1;
}

static grid_demo_item_view_t *grid_demo_find_view_by_root(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < grid_demo_ctx.created_count; i++)
    {
        if (view == EGUI_VIEW_OF(&grid_demo_ctx.item_views[i].root))
        {
            return &grid_demo_ctx.item_views[i];
        }
    }

    return NULL;
}

static egui_background_t *grid_demo_get_badge_background(uint8_t state)
{
    switch (state)
    {
    case GRID_STATE_SYNC:
        return EGUI_BG_OF(&grid_demo_badge_sync_bg);
    case GRID_STATE_WARNING:
        return EGUI_BG_OF(&grid_demo_badge_warn_bg);
    case GRID_STATE_DONE:
        return EGUI_BG_OF(&grid_demo_badge_done_bg);
    default:
        return EGUI_BG_OF(&grid_demo_badge_idle_bg);
    }
}

static void grid_demo_set_item_pulse(grid_demo_item_view_t *item_view, const grid_demo_item_t *item, uint8_t visible, uint8_t selected)
{
    egui_background_t *pulse_bg;

    if (!visible || item == NULL)
    {
        if (item_view->pulse_running)
        {
            egui_animation_stop(EGUI_ANIM_OF(&item_view->pulse_anim));
            item_view->pulse_running = 0;
        }
        egui_view_set_gone(EGUI_VIEW_OF(&item_view->pulse), 1);
        egui_view_set_alpha(EGUI_VIEW_OF(&item_view->pulse), EGUI_ALPHA_100);
        return;
    }

    if (selected)
    {
        pulse_bg = EGUI_BG_OF(&grid_demo_pulse_selected_bg);
    }
    else if (item->state == GRID_STATE_WARNING)
    {
        pulse_bg = EGUI_BG_OF(&grid_demo_pulse_warn_bg);
    }
    else if (item->state == GRID_STATE_DONE)
    {
        pulse_bg = EGUI_BG_OF(&grid_demo_pulse_done_bg);
    }
    else
    {
        pulse_bg = EGUI_BG_OF(&grid_demo_pulse_sync_bg);
    }

    egui_view_set_gone(EGUI_VIEW_OF(&item_view->pulse), 0);
    egui_view_set_background(EGUI_VIEW_OF(&item_view->pulse), pulse_bg);
    if (!item_view->pulse_running)
    {
        egui_animation_start(EGUI_ANIM_OF(&item_view->pulse_anim));
        item_view->pulse_running = 1;
    }
}

static void grid_demo_capture_view_state(grid_demo_item_view_t *item_view, grid_demo_item_state_t *state)
{
    egui_animation_t *anim = EGUI_ANIM_OF(&item_view->pulse_anim);

    memset(state, 0, sizeof(*state));
    state->pulse_running = item_view->pulse_running ? 1U : 0U;
    state->pulse_alpha = EGUI_VIEW_OF(&item_view->pulse)->alpha;

    if (!item_view->pulse_running)
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

static void grid_demo_restore_view_state(grid_demo_item_view_t *item_view, const grid_demo_item_state_t *state)
{
    egui_animation_t *anim;

    if (state == NULL || !state->pulse_running)
    {
        return;
    }

    anim = EGUI_ANIM_OF(&item_view->pulse_anim);
    if (!anim->is_running)
    {
        egui_animation_start(anim);
    }

    item_view->pulse_running = 1;
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->pulse), 0);
    egui_view_set_alpha(EGUI_VIEW_OF(&item_view->pulse), state->pulse_alpha);
    anim->is_started = 1;
    anim->is_ended = 0;
    anim->is_cycle_flip = state->pulse_cycle_flip ? 1U : 0U;
    anim->repeated = (int8_t)state->pulse_repeated;
    anim->start_time = egui_api_timer_get_current() - state->pulse_elapsed_ms;
}

static void grid_demo_layout_card_children(grid_demo_item_view_t *item_view, const grid_demo_item_t *item, int pool_index, uint32_t index, uint8_t selected)
{
    egui_dim_t root_w = EGUI_VIEW_OF(&item_view->root)->region.size.width > 0 ? EGUI_VIEW_OF(&item_view->root)->region.size.width : 72;
    egui_dim_t root_h = EGUI_VIEW_OF(&item_view->root)->region.size.height > 0 ? EGUI_VIEW_OF(&item_view->root)->region.size.height : 72;
    egui_dim_t card_w = root_w - GRID_CARD_INSET_X * 2;
    egui_dim_t card_h = root_h - GRID_CARD_INSET_Y * 2;
    uint8_t narrow = card_w < 56 ? 1U : 0U;
    uint8_t compact = card_w < 76 ? 1U : 0U;
    egui_dim_t inset = narrow ? 4 : 6;
    egui_dim_t badge_w = narrow ? 26 : compact ? 32 : 40;
    egui_dim_t badge_h = 14;
    egui_dim_t footer_h = 10;
    egui_dim_t progress_h = 4;
    egui_dim_t pulse_size = narrow ? 6 : 8;
    egui_dim_t text_x = inset;
    egui_dim_t text_w;
    egui_dim_t tag_y = inset;
    egui_dim_t tag_h = 10;
    egui_dim_t title_y = inset + 12;
    egui_dim_t title_h = narrow ? 10 : 12;
    egui_dim_t meta_y;
    egui_dim_t meta_h = 10;
    egui_dim_t footer_y;
    egui_dim_t progress_y;
    egui_dim_t accent_x = inset;
    egui_dim_t accent_y = inset + 8;
    egui_dim_t accent_w = card_w - inset * 2;
    egui_dim_t accent_h = narrow ? 8 : 12;
    uint8_t show_tag = (uint8_t)(!narrow || selected || item->variant == GRID_VARIANT_ALERT);
    uint8_t show_meta = (uint8_t)(!narrow || selected || item->variant == GRID_VARIANT_METRIC);
    uint8_t show_badge = (uint8_t)(!narrow || selected || item->state == GRID_STATE_WARNING);
    uint8_t show_footer = (uint8_t)(selected || (!compact && item->variant == GRID_VARIANT_TASK));
    egui_color_t title_color;
    egui_color_t meta_color;
    egui_color_t border_color;
    egui_color_t accent_color;
    egui_color_t tag_color = EGUI_COLOR_HEX(0x6E7F8D);
    egui_color_t badge_text_color = EGUI_COLOR_WHITE;
    egui_color_t card_color;

    switch (item->variant)
    {
    case GRID_VARIANT_METRIC:
        card_color = selected ? EGUI_COLOR_HEX(0xEAF5FF) : EGUI_COLOR_HEX(0xF5FAFD);
        accent_color = EGUI_COLOR_HEX(0x6A9FD1);
        break;
    case GRID_VARIANT_ALERT:
        card_color = selected ? EGUI_COLOR_HEX(0xFFF3E4) : EGUI_COLOR_HEX(0xFFF8F1);
        accent_color = EGUI_COLOR_HEX(0xD08A2E);
        break;
    case GRID_VARIANT_TASK:
        card_color = selected ? EGUI_COLOR_HEX(0xECF8EE) : EGUI_COLOR_HEX(0xF6FBF6);
        accent_color = EGUI_COLOR_HEX(0x5BA57B);
        break;
    default:
        card_color = selected ? EGUI_COLOR_HEX(0xEEF5FF) : EGUI_COLOR_WHITE;
        accent_color = EGUI_COLOR_HEX(0x94A7B7);
        break;
    }

    border_color = selected                            ? EGUI_COLOR_HEX(0x3A6EA5)
                   : item->state == GRID_STATE_WARNING ? EGUI_COLOR_HEX(0xD6A45C)
                   : item->state == GRID_STATE_SYNC    ? EGUI_COLOR_HEX(0x84BCA0)
                                                       : EGUI_COLOR_HEX(0xD8E1EA);
    title_color = selected ? EGUI_COLOR_HEX(0x17324A) : EGUI_COLOR_HEX(0x213344);
    meta_color = selected ? EGUI_COLOR_HEX(0x466174) : EGUI_COLOR_HEX(0x637788);

    snprintf(grid_demo_ctx.tag_texts[pool_index], sizeof(grid_demo_ctx.tag_texts[pool_index]), "%s",
             narrow ? grid_demo_variant_short_names[item->variant] : grid_demo_variant_names[item->variant]);
    if (item->variant == GRID_VARIANT_METRIC)
    {
        snprintf(grid_demo_ctx.title_texts[pool_index], sizeof(grid_demo_ctx.title_texts[pool_index]), narrow ? "%u" : "%u%%", (unsigned)item->progress);
        snprintf(grid_demo_ctx.meta_texts[pool_index], sizeof(grid_demo_ctx.meta_texts[pool_index]), compact ? "R%u" : "Rev %u",
                 (unsigned)item->revision);
    }
    else
    {
        snprintf(grid_demo_ctx.title_texts[pool_index], sizeof(grid_demo_ctx.title_texts[pool_index]), narrow ? "#%02lu" : "#%03lu",
                 (unsigned long)(item->stable_id % 1000U));
        snprintf(grid_demo_ctx.meta_texts[pool_index], sizeof(grid_demo_ctx.meta_texts[pool_index]), compact ? "R%u" : "R%u %u%%", (unsigned)item->revision,
                 (unsigned)item->progress);
    }
    snprintf(grid_demo_ctx.badge_texts[pool_index], sizeof(grid_demo_ctx.badge_texts[pool_index]), "%s", grid_demo_state_names[item->state]);
    snprintf(grid_demo_ctx.footer_texts[pool_index], sizeof(grid_demo_ctx.footer_texts[pool_index]),
             selected                              ? "Selected %03lu"
             : item->variant == GRID_VARIANT_ALERT ? (compact ? "Watch" : "Watch lane")
             : item->variant == GRID_VARIANT_TASK  ? "Checklist"
             : item->variant == GRID_VARIANT_METRIC ? "Sync panel"
                                                    : "Poster rail",
             (unsigned long)(index % 1000U));

    egui_view_set_size(EGUI_VIEW_OF(&item_view->card), card_w, card_h);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->card), GRID_CARD_INSET_X, GRID_CARD_INSET_Y);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&item_view->card), card_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&item_view->card), selected ? 2 : 1, border_color);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&item_view->card), narrow ? 8 : compact ? 10 : 12);
    egui_view_set_shadow(EGUI_VIEW_OF(&item_view->card), &grid_demo_card_shadow);

    if (item->variant == GRID_VARIANT_HERO)
    {
        accent_y = show_tag ? (egui_dim_t)(inset + 10) : (egui_dim_t)(inset + 6);
        accent_h = narrow ? 10 : compact ? 14 : 24;
        title_y = (egui_dim_t)(accent_y + accent_h + 6);
        tag_color = EGUI_COLOR_HEX(0x708291);
    }
    else if (item->variant == GRID_VARIANT_METRIC)
    {
        accent_y = show_tag ? (egui_dim_t)(inset + 10) : (egui_dim_t)(inset + 6);
        accent_h = narrow ? 4 : 6;
        title_y = (egui_dim_t)(accent_y + accent_h + (narrow ? 8 : 10));
        title_h = narrow ? 11 : 14;
        tag_color = EGUI_COLOR_HEX(0x5E86AB);
    }
    else if (item->variant == GRID_VARIANT_ALERT)
    {
        accent_w = narrow ? 4 : 6;
        accent_y = show_tag ? (egui_dim_t)(inset + 10) : (egui_dim_t)(inset + 6);
        accent_h = (egui_dim_t)(card_h - accent_y - inset - progress_h - 8);
        text_x = (egui_dim_t)(inset + accent_w + 6);
        title_y = show_tag ? (egui_dim_t)(inset + 12) : (egui_dim_t)(inset + 8);
        tag_color = EGUI_COLOR_HEX(0xA36B28);
    }
    else
    {
        accent_y = show_tag ? (egui_dim_t)(inset + 10) : (egui_dim_t)(inset + 6);
        accent_w = compact ? 16 : 28;
        accent_h = 4;
        title_y = (egui_dim_t)(accent_y + accent_h + 6);
        tag_color = EGUI_COLOR_HEX(0x5F7F6B);
    }

    text_w = (egui_dim_t)(card_w - text_x - inset - (show_badge ? badge_w + 4 : 0));
    if (text_w < 18)
    {
        text_w = 18;
    }

    meta_y = (egui_dim_t)(title_y + title_h + 2);
    progress_y = (egui_dim_t)(card_h - inset - progress_h);
    if (show_footer)
    {
        footer_y = (egui_dim_t)(progress_y - footer_h - 2);
    }
    else
    {
        footer_y = progress_y;
    }
    if (show_meta && progress_y < meta_y + meta_h + 4)
    {
        progress_y = (egui_dim_t)(meta_y + meta_h + 4);
    }
    else if (!show_meta && progress_y < title_y + title_h + 4)
    {
        progress_y = (egui_dim_t)(title_y + title_h + 4);
    }
    if (show_footer && footer_y < meta_y + meta_h + 2)
    {
        footer_y = (egui_dim_t)(meta_y + meta_h + 2);
    }

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&item_view->accent), accent_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&item_view->accent), 0, accent_color);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&item_view->accent), accent_h > accent_w ? accent_w / 2 : accent_h / 2);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->accent), accent_x, accent_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->accent), accent_w, accent_h);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->tag), text_x, tag_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->tag), text_w, tag_h);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->tag), grid_demo_ctx.tag_texts[pool_index]);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->tag), tag_color, EGUI_ALPHA_100);
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->tag), show_tag ? 0 : 1);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->badge), card_w - inset - badge_w, inset - 1);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->badge), badge_w, badge_h);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->badge), grid_demo_ctx.badge_texts[pool_index]);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->badge), badge_text_color, EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&item_view->badge), grid_demo_get_badge_background(item->state));
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->badge), show_badge ? 0 : 1);

    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->title),
                             item->variant == GRID_VARIANT_METRIC && !compact ? GRID_FONT_TITLE_L : (narrow ? GRID_FONT_META : GRID_FONT_TITLE));
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->title),
                                   item->variant == GRID_VARIANT_METRIC ? EGUI_ALIGN_CENTER : (EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER));
    egui_view_set_position(EGUI_VIEW_OF(&item_view->title), text_x, title_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->title), text_w, title_h);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->title), grid_demo_ctx.title_texts[pool_index]);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->title), title_color, EGUI_ALPHA_100);

    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->meta),
                                   item->variant == GRID_VARIANT_METRIC ? EGUI_ALIGN_CENTER : (EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER));
    egui_view_set_position(EGUI_VIEW_OF(&item_view->meta), text_x, meta_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->meta), text_w, meta_h);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->meta), grid_demo_ctx.meta_texts[pool_index]);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->meta), meta_color, EGUI_ALPHA_100);
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->meta), show_meta ? 0 : 1);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->footer), text_x, footer_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->footer), text_w, footer_h);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->footer), grid_demo_ctx.footer_texts[pool_index]);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->footer), EGUI_COLOR_HEX(0x607182), EGUI_ALPHA_100);
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->footer), show_footer ? 0 : 1);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->progress), inset, progress_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->progress), card_w - inset * 2, progress_h);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&item_view->progress), item->progress);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->pulse), card_w - inset - pulse_size, progress_y - pulse_size - 6);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->pulse), pulse_size, pulse_size);
    grid_demo_set_item_pulse(item_view, item, grid_demo_item_has_pulse(item, selected), selected);
}

static void grid_demo_refresh_header(void)
{
    int32_t selected_index = grid_demo_find_index_by_stable_id(grid_demo_ctx.selected_id);

    snprintf(grid_demo_ctx.header_title_text, sizeof(grid_demo_ctx.header_title_text), "Virtual Grid Demo");
    if (selected_index >= 0)
    {
        const grid_demo_item_t *item = &grid_demo_ctx.items[selected_index];

        snprintf(grid_demo_ctx.header_detail_text, sizeof(grid_demo_ctx.header_detail_text), "Cards %lu | cols %u | sel #%05lu | %u%%",
                 (unsigned long)grid_demo_ctx.item_count, (unsigned)grid_demo_get_column_count(), (unsigned long)item->stable_id, (unsigned)item->progress);
    }
    else
    {
        snprintf(grid_demo_ctx.header_detail_text, sizeof(grid_demo_ctx.header_detail_text), "Cards %lu | cols %u | sel none",
                 (unsigned long)grid_demo_ctx.item_count, (unsigned)grid_demo_get_column_count());
    }
    snprintf(grid_demo_ctx.header_hint_text, sizeof(grid_demo_ctx.header_hint_text), "%s", grid_demo_ctx.last_action_text);

    if (EGUI_VIEW_OF(&header_title)->api == NULL)
    {
        return;
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&header_title), grid_demo_ctx.header_title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_detail), grid_demo_ctx.header_detail_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_hint), grid_demo_ctx.header_hint_text);
}

static void grid_demo_refresh_mode_buttons(void)
{
    uint8_t i;
    uint8_t columns = grid_demo_get_column_count();

    for (i = 0; i < 3; i++)
    {
        egui_view_set_background(EGUI_VIEW_OF(&mode_buttons[i]),
                                 columns == (uint8_t)(i + 2U) ? EGUI_BG_OF(&grid_demo_mode_active_bg) : EGUI_BG_OF(&grid_demo_mode_idle_bg));
    }
}

static void grid_demo_update_selection(uint32_t stable_id, uint8_t allow_toggle, uint8_t ensure_visible)
{
    uint32_t previous_id = grid_demo_ctx.selected_id;

    if (allow_toggle && previous_id == stable_id)
    {
        grid_demo_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }
    else
    {
        grid_demo_ctx.selected_id = stable_id;
    }

    if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && previous_id != grid_demo_ctx.selected_id)
    {
        egui_view_virtual_grid_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&grid_view), previous_id);
    }
    if (grid_demo_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_grid_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&grid_view), grid_demo_ctx.selected_id);
        if (ensure_visible)
        {
            egui_view_virtual_grid_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&grid_view), grid_demo_ctx.selected_id, GRID_CARD_INSET_Y);
        }
    }
}

static uint32_t grid_demo_pick_action_index(void)
{
    int32_t selected_index = grid_demo_find_index_by_stable_id(grid_demo_ctx.selected_id);

    if (selected_index >= 0)
    {
        return (uint32_t)selected_index;
    }
    if (grid_demo_ctx.item_count == 0)
    {
        return 0;
    }

    return grid_demo_ctx.mutation_cursor % grid_demo_ctx.item_count;
}

static uint32_t grid_demo_get_count(void *data_source_context)
{
    return ((grid_demo_context_t *)data_source_context)->item_count;
}

static uint32_t grid_demo_get_stable_id(void *data_source_context, uint32_t index)
{
    grid_demo_context_t *ctx = (grid_demo_context_t *)data_source_context;
    return ctx->items[index].stable_id;
}

static int32_t grid_demo_find_index_adapter(void *data_source_context, uint32_t stable_id)
{
    grid_demo_context_t *ctx = (grid_demo_context_t *)data_source_context;
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

static int32_t grid_demo_measure_item_height(void *data_source_context, uint32_t index, int32_t width_hint)
{
    grid_demo_context_t *ctx = (grid_demo_context_t *)data_source_context;
    uint8_t selected = ctx->items[index].stable_id == ctx->selected_id ? 1U : 0U;

    return grid_demo_measure_item_height_with_state(&ctx->items[index], width_hint, selected);
}

static void grid_demo_card_click_cb(egui_view_t *self)
{
    grid_demo_item_view_t *item_view = grid_demo_find_view_by_root(self);

    if (item_view == NULL || item_view->stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return;
    }

    grid_demo_ctx.last_clicked_index = item_view->bound_index;
    grid_demo_ctx.click_count++;
    grid_demo_update_selection(item_view->stable_id, 1, 1);
    snprintf(grid_demo_ctx.last_action_text, sizeof(grid_demo_ctx.last_action_text), "Click item #%05lu @ index %lu.", (unsigned long)item_view->stable_id,
             (unsigned long)item_view->bound_index);
    grid_demo_refresh_header();
}

static egui_view_t *grid_demo_create_item_view(void *data_source_context, uint16_t view_type)
{
    grid_demo_context_t *ctx = (grid_demo_context_t *)data_source_context;
    grid_demo_item_view_t *item_view;

    EGUI_UNUSED(view_type);

    if (ctx->created_count >= GRID_VIEW_POOL_CAPACITY)
    {
        return NULL;
    }

    item_view = &ctx->item_views[ctx->created_count];
    memset(item_view, 0, sizeof(*item_view));

    egui_view_group_init(EGUI_VIEW_OF(&item_view->root));

    egui_view_card_init(EGUI_VIEW_OF(&item_view->card));
    egui_view_card_set_border(EGUI_VIEW_OF(&item_view->card), 1, EGUI_COLOR_HEX(0xD8E1EA));
    egui_view_set_clickable(EGUI_VIEW_OF(&item_view->root), 1);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&item_view->root), grid_demo_card_click_cb);
    egui_view_group_add_child(EGUI_VIEW_OF(&item_view->root), EGUI_VIEW_OF(&item_view->card));

    egui_view_card_init(EGUI_VIEW_OF(&item_view->accent));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->accent));

    egui_view_label_init(EGUI_VIEW_OF(&item_view->tag));
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->tag), GRID_FONT_CAPTION);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->tag), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->tag));

    egui_view_label_init(EGUI_VIEW_OF(&item_view->title));
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->title), GRID_FONT_TITLE);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->title));

    egui_view_label_init(EGUI_VIEW_OF(&item_view->meta));
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->meta), GRID_FONT_META);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->meta), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->meta));

    egui_view_label_init(EGUI_VIEW_OF(&item_view->badge));
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->badge), GRID_FONT_CAPTION);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->badge), EGUI_ALIGN_CENTER);
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->badge));

    egui_view_label_init(EGUI_VIEW_OF(&item_view->footer));
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->footer), GRID_FONT_META);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->footer), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->footer));

    egui_view_progress_bar_init(EGUI_VIEW_OF(&item_view->progress));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->progress));

    egui_view_init(EGUI_VIEW_OF(&item_view->pulse));
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->pulse), 1);
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->pulse));

    egui_animation_alpha_init(EGUI_ANIM_OF(&item_view->pulse_anim));
    egui_animation_alpha_params_set(&item_view->pulse_anim, &grid_demo_pulse_anim_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&item_view->pulse_anim), 850);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&item_view->pulse_anim), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&item_view->pulse_anim), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_interpolator_linear_init((egui_interpolator_t *)&item_view->pulse_interp);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&item_view->pulse_anim), (egui_interpolator_t *)&item_view->pulse_interp);
    egui_animation_target_view_set(EGUI_ANIM_OF(&item_view->pulse_anim), EGUI_VIEW_OF(&item_view->pulse));

    item_view->bound_index = GRID_INVALID_INDEX;
    item_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    ctx->created_count++;
    return EGUI_VIEW_OF(&item_view->root);
}

static void grid_demo_bind_item_view(void *data_source_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    grid_demo_context_t *ctx = (grid_demo_context_t *)data_source_context;
    grid_demo_item_view_t *item_view = grid_demo_find_view_by_root(view);
    int pool_index;

    if (item_view == NULL || index >= ctx->item_count)
    {
        return;
    }

    pool_index = grid_demo_get_view_pool_index(item_view);
    if (pool_index < 0)
    {
        return;
    }

    item_view->bound_index = index;
    item_view->stable_id = stable_id;
    grid_demo_layout_card_children(item_view, &ctx->items[index], pool_index, index, stable_id == ctx->selected_id);
}

static void grid_demo_unbind_item_view(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    grid_demo_item_view_t *item_view = grid_demo_find_view_by_root(view);

    EGUI_UNUSED(data_source_context);
    EGUI_UNUSED(stable_id);

    if (item_view == NULL)
    {
        return;
    }

    grid_demo_set_item_pulse(item_view, NULL, 0, 0);
    item_view->bound_index = GRID_INVALID_INDEX;
    item_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static void grid_demo_save_item_state(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    grid_demo_item_view_t *item_view = grid_demo_find_view_by_root(view);
    grid_demo_item_state_t state;

    EGUI_UNUSED(data_source_context);

    if (item_view == NULL)
    {
        return;
    }

    grid_demo_capture_view_state(item_view, &state);
    if (!state.pulse_running)
    {
        egui_view_virtual_grid_remove_item_state_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id);
        return;
    }

    (void)egui_view_virtual_grid_write_item_state_for_view(view, stable_id, &state, sizeof(state));
}

static void grid_demo_restore_item_state(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    grid_demo_item_view_t *item_view = grid_demo_find_view_by_root(view);
    grid_demo_item_state_t state;

    EGUI_UNUSED(data_source_context);

    if (item_view == NULL)
    {
        return;
    }

    if (egui_view_virtual_grid_read_item_state_for_view(view, stable_id, &state, sizeof(state)) != sizeof(state))
    {
        return;
    }

    grid_demo_restore_view_state(item_view, &state);
}

static uint8_t grid_demo_should_keep_alive(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    grid_demo_context_t *ctx = (grid_demo_context_t *)data_source_context;
    int32_t index = grid_demo_find_index_by_stable_id(stable_id);

    EGUI_UNUSED(view);

    if (stable_id == ctx->selected_id)
    {
        return 1;
    }
    if (index < 0)
    {
        return 0;
    }

    return grid_demo_item_has_pulse(&ctx->items[index], 0);
}

static const egui_view_virtual_grid_data_source_t grid_demo_data_source = {
        .get_count = grid_demo_get_count,
        .get_stable_id = grid_demo_get_stable_id,
        .find_index_by_stable_id = grid_demo_find_index_adapter,
        .get_item_view_type = NULL,
        .measure_item_height = grid_demo_measure_item_height,
        .create_item_view = grid_demo_create_item_view,
        .destroy_item_view = NULL,
        .bind_item_view = grid_demo_bind_item_view,
        .unbind_item_view = grid_demo_unbind_item_view,
        .should_keep_alive = grid_demo_should_keep_alive,
        .save_item_state = grid_demo_save_item_state,
        .restore_item_state = grid_demo_restore_item_state,
        .default_item_view_type = 0,
};

static void grid_demo_action_add(void)
{
    uint32_t index;
    uint32_t stable_id;

    if (grid_demo_ctx.item_count >= GRID_MAX_ITEMS)
    {
        snprintf(grid_demo_ctx.last_action_text, sizeof(grid_demo_ctx.last_action_text), "Add ignored. Grid is full.");
        grid_demo_refresh_header();
        return;
    }

    index = grid_demo_ctx.item_count == 0 ? 0U : grid_demo_pick_action_index() + 1U;
    if (index > grid_demo_ctx.item_count)
    {
        index = grid_demo_ctx.item_count;
    }

    if (index < grid_demo_ctx.item_count)
    {
        memmove(&grid_demo_ctx.items[index + 1], &grid_demo_ctx.items[index], (grid_demo_ctx.item_count - index) * sizeof(grid_demo_ctx.items[0]));
    }

    stable_id = grid_demo_ctx.next_stable_id++;
    grid_demo_fill_item(&grid_demo_ctx.items[index], stable_id, stable_id);
    grid_demo_ctx.items[index].state = GRID_STATE_SYNC;
    grid_demo_ctx.items[index].progress = 26U;
    grid_demo_ctx.item_count++;
    grid_demo_ctx.action_count++;
    grid_demo_ctx.mutation_cursor = index;

    egui_view_virtual_grid_notify_item_inserted(EGUI_VIEW_OF(&grid_view), index, 1);
    grid_demo_update_selection(stable_id, 0, 1);
    snprintf(grid_demo_ctx.last_action_text, sizeof(grid_demo_ctx.last_action_text), "Inserted card #%05lu.", (unsigned long)stable_id);
    grid_demo_refresh_header();
}

static void grid_demo_action_del(void)
{
    uint32_t index;
    uint32_t stable_id;
    uint8_t removed_selected;

    if (grid_demo_ctx.item_count == 0)
    {
        snprintf(grid_demo_ctx.last_action_text, sizeof(grid_demo_ctx.last_action_text), "Del ignored. Grid is empty.");
        grid_demo_refresh_header();
        return;
    }

    index = grid_demo_pick_action_index();
    stable_id = grid_demo_ctx.items[index].stable_id;
    removed_selected = stable_id == grid_demo_ctx.selected_id ? 1U : 0U;

    egui_view_virtual_grid_remove_item_state_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id);
    if ((index + 1U) < grid_demo_ctx.item_count)
    {
        memmove(&grid_demo_ctx.items[index], &grid_demo_ctx.items[index + 1], (grid_demo_ctx.item_count - index - 1U) * sizeof(grid_demo_ctx.items[0]));
    }

    grid_demo_ctx.item_count--;
    grid_demo_ctx.action_count++;
    if (grid_demo_ctx.item_count == 0)
    {
        grid_demo_ctx.mutation_cursor = 0;
    }
    else if (grid_demo_ctx.mutation_cursor >= grid_demo_ctx.item_count)
    {
        grid_demo_ctx.mutation_cursor = 0;
    }
    if (removed_selected)
    {
        grid_demo_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    egui_view_virtual_grid_notify_item_removed(EGUI_VIEW_OF(&grid_view), index, 1);
    snprintf(grid_demo_ctx.last_action_text, sizeof(grid_demo_ctx.last_action_text), "Deleted card #%05lu.", (unsigned long)stable_id);
    grid_demo_refresh_header();
}

static void grid_demo_action_patch(void)
{
    uint32_t index;
    grid_demo_item_t *item;
    int32_t width_hint;
    int32_t old_height;
    int32_t new_height;
    uint8_t selected;

    if (grid_demo_ctx.item_count == 0)
    {
        snprintf(grid_demo_ctx.last_action_text, sizeof(grid_demo_ctx.last_action_text), "Patch ignored. Grid is empty.");
        grid_demo_refresh_header();
        return;
    }

    index = grid_demo_pick_action_index();
    item = &grid_demo_ctx.items[index];
    selected = item->stable_id == grid_demo_ctx.selected_id ? 1U : 0U;
    width_hint = grid_demo_get_item_width_hint(index);
    old_height = grid_demo_measure_item_height_with_state(item, width_hint, selected);

    item->revision++;
    item->variant = (uint8_t)((item->variant + 1U) % GRID_VARIANT_COUNT);
    item->state = (uint8_t)((item->state + 1U) % GRID_STATE_COUNT);
    if (item->state == GRID_STATE_DONE)
    {
        item->progress = 100U;
    }
    else
    {
        item->progress = (uint8_t)(24U + ((item->progress + 19U) % 69U));
    }
    if (item->variant == GRID_VARIANT_ALERT && item->state == GRID_STATE_IDLE)
    {
        item->state = GRID_STATE_WARNING;
    }

    new_height = grid_demo_measure_item_height_with_state(item, width_hint, selected);
    grid_demo_ctx.action_count++;
    grid_demo_ctx.mutation_cursor = (index + 23U) % grid_demo_ctx.item_count;

    if (new_height != old_height)
    {
        egui_view_virtual_grid_notify_item_resized(EGUI_VIEW_OF(&grid_view), index);
    }
    else
    {
        egui_view_virtual_grid_notify_item_changed(EGUI_VIEW_OF(&grid_view), index);
    }
    if (selected)
    {
        egui_view_virtual_grid_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&grid_view), item->stable_id, GRID_CARD_INSET_Y);
    }

    snprintf(grid_demo_ctx.last_action_text, sizeof(grid_demo_ctx.last_action_text), "Patched card #%05lu.", (unsigned long)item->stable_id);
    grid_demo_refresh_header();
}

static void grid_demo_action_jump(void)
{
    uint32_t index;
    uint32_t stable_id;

    if (grid_demo_ctx.item_count == 0)
    {
        snprintf(grid_demo_ctx.last_action_text, sizeof(grid_demo_ctx.last_action_text), "Jump ignored. Grid is empty.");
        grid_demo_refresh_header();
        return;
    }

    grid_demo_ctx.mutation_cursor = (grid_demo_ctx.mutation_cursor + 37U) % grid_demo_ctx.item_count;
    index = grid_demo_ctx.mutation_cursor;
    stable_id = grid_demo_ctx.items[index].stable_id;
    grid_demo_ctx.action_count++;

    grid_demo_update_selection(stable_id, 0, 0);
    egui_view_virtual_grid_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id, GRID_CARD_INSET_Y);
    snprintf(grid_demo_ctx.last_action_text, sizeof(grid_demo_ctx.last_action_text), "Jump to card #%05lu.", (unsigned long)stable_id);
    grid_demo_refresh_header();
}

static void grid_demo_apply_action(uint8_t action)
{
    switch (action)
    {
    case GRID_ACTION_ADD:
        grid_demo_action_add();
        break;
    case GRID_ACTION_DEL:
        grid_demo_action_del();
        break;
    case GRID_ACTION_PATCH:
        grid_demo_action_patch();
        break;
    case GRID_ACTION_JUMP:
        grid_demo_action_jump();
        break;
    default:
        break;
    }
}

static void grid_demo_apply_column_mode(uint8_t columns)
{
    egui_view_virtual_grid_set_column_count(EGUI_VIEW_OF(&grid_view), columns);
    grid_demo_refresh_mode_buttons();
    if (grid_demo_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_grid_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&grid_view), grid_demo_ctx.selected_id, GRID_CARD_INSET_Y);
    }
    snprintf(grid_demo_ctx.last_action_text, sizeof(grid_demo_ctx.last_action_text), "Switch to %u columns.", (unsigned)columns);
    grid_demo_refresh_header();
}

static int grid_demo_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < GRID_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static int grid_demo_find_mode_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < 3; i++)
    {
        if (self == EGUI_VIEW_OF(&mode_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void grid_demo_action_click_cb(egui_view_t *self)
{
    int action = grid_demo_find_action_button_index(self);

    if (action >= 0)
    {
        grid_demo_apply_action((uint8_t)action);
        return;
    }

    action = grid_demo_find_mode_button_index(self);
    if (action >= 0)
    {
        grid_demo_apply_column_mode((uint8_t)(action + 2));
    }
}

static void grid_demo_init_button(egui_view_button_t *button, egui_dim_t x, egui_dim_t y, egui_dim_t width, const char *text)
{
    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_position(EGUI_VIEW_OF(button), x, y);
    egui_view_set_size(EGUI_VIEW_OF(button), width, y == 6 ? GRID_ACTION_BUTTON_H : GRID_MODE_BUTTON_H);
    egui_view_label_set_text(EGUI_VIEW_OF(button), text);
    egui_view_label_set_font(EGUI_VIEW_OF(button), GRID_FONT_CAPTION);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x2B3F52), EGUI_ALPHA_100);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), grid_demo_action_click_cb);
}

void test_init_ui(void)
{
    uint8_t i;
    egui_dim_t button_x = 10;
    static const char *mode_labels[3] = {"2 Col", "3 Col", "4 Col"};

    grid_demo_reset_model();

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&grid_demo_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&header_card), &header_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_card), 1, EGUI_COLOR_HEX(0xD8E1EA));
    egui_view_set_shadow(EGUI_VIEW_OF(&header_card), &grid_demo_card_shadow);

    egui_view_label_init(EGUI_VIEW_OF(&header_title));
    egui_view_set_position(EGUI_VIEW_OF(&header_title), 12, 10);
    egui_view_set_size(EGUI_VIEW_OF(&header_title), GRID_CONTENT_W - 24, 12);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_title), GRID_FONT_HEADER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_title), EGUI_COLOR_HEX(0x203243), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_title));

    egui_view_label_init(EGUI_VIEW_OF(&header_detail));
    egui_view_set_position(EGUI_VIEW_OF(&header_detail), 12, 30);
    egui_view_set_size(EGUI_VIEW_OF(&header_detail), GRID_CONTENT_W - 24, 10);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_detail), GRID_FONT_META);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_detail), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_detail), EGUI_COLOR_HEX(0x597082), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_detail));

    egui_view_label_init(EGUI_VIEW_OF(&header_hint));
    egui_view_set_position(EGUI_VIEW_OF(&header_hint), 12, 46);
    egui_view_set_size(EGUI_VIEW_OF(&header_hint), GRID_CONTENT_W - 24, 12);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_hint), GRID_FONT_META);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_hint), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_hint), EGUI_COLOR_HEX(0x6B7C8A), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_hint));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD8E1EA));

    for (i = 0; i < GRID_ACTION_COUNT; i++)
    {
        grid_demo_init_button(&action_buttons[i], button_x, 6, GRID_ACTION_BUTTON_W, grid_demo_action_names[i]);
        egui_view_set_background(EGUI_VIEW_OF(&action_buttons[i]), i == GRID_ACTION_ADD     ? EGUI_BG_OF(&grid_demo_btn_add_bg)
                                                                   : i == GRID_ACTION_DEL   ? EGUI_BG_OF(&grid_demo_btn_del_bg)
                                                                   : i == GRID_ACTION_PATCH ? EGUI_BG_OF(&grid_demo_btn_patch_bg)
                                                                                            : EGUI_BG_OF(&grid_demo_btn_jump_bg));
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
        button_x += GRID_ACTION_BUTTON_W + GRID_ACTION_BUTTON_GAP;
    }

    button_x = 10;
    for (i = 0; i < 3; i++)
    {
        grid_demo_init_button(&mode_buttons[i], button_x, 30, GRID_MODE_BUTTON_W, mode_labels[i]);
        egui_view_set_background(EGUI_VIEW_OF(&mode_buttons[i]), EGUI_BG_OF(&grid_demo_mode_idle_bg));
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&mode_buttons[i]));
        button_x += GRID_MODE_BUTTON_W + GRID_MODE_BUTTON_GAP;
    }

    {
        const egui_view_virtual_grid_setup_t grid_view_setup = {
                .params = &grid_view_params,
                .data_source = &grid_demo_data_source,
                .data_source_context = &grid_demo_ctx,
                .state_cache_max_entries = GRID_STATE_CACHE_COUNT,
                .state_cache_max_bytes = GRID_STATE_CACHE_COUNT * (uint32_t)sizeof(grid_demo_item_state_t),
        };

        egui_view_virtual_grid_init_with_setup(EGUI_VIEW_OF(&grid_view), &grid_view_setup);
    }
    egui_view_set_background(EGUI_VIEW_OF(&grid_view), EGUI_BG_OF(&grid_demo_view_bg));

    grid_demo_refresh_mode_buttons();
    grid_demo_refresh_header();

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&header_card));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t grid_demo_match_visible_card(egui_view_t *self, const egui_view_virtual_grid_slot_t *slot, const egui_view_virtual_grid_entry_t *entry,
                                            egui_view_t *item_view, void *context)
{
    int32_t y;

    EGUI_UNUSED(self);
    EGUI_UNUSED(item_view);
    EGUI_UNUSED(context);

    if (entry == NULL)
    {
        return 0;
    }

    y = egui_view_virtual_grid_get_item_y(EGUI_VIEW_OF(&grid_view), entry->index);
    return (uint8_t)(egui_view_virtual_viewport_is_slot_center_visible(EGUI_VIEW_OF(&grid_view), slot) &&
                     egui_view_virtual_viewport_is_main_span_center_visible(EGUI_VIEW_OF(&grid_view), y,
                                                                            egui_view_virtual_grid_get_item_height(EGUI_VIEW_OF(&grid_view), entry->index)));
}

static egui_view_t *grid_demo_find_first_visible_card_view(void)
{
    return egui_view_virtual_grid_find_first_visible_item_view(EGUI_VIEW_OF(&grid_view), grid_demo_match_visible_card, NULL, NULL);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    egui_view_t *view;

    switch (action_index)
    {
    case 0:
        view = grid_demo_find_first_visible_card_view();
        if (view == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 700);
        return true;
    case 1:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT - 34;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = GRID_VIEW_Y + 26;
        p_action->steps = 6;
        p_action->interval_ms = 180;
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&mode_buttons[1]), 650);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[GRID_ACTION_PATCH]), 650);
        return true;
    case 4:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[GRID_ACTION_JUMP]), 650);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[GRID_ACTION_ADD]), 650);
        return true;
    case 6:
        view = grid_demo_find_first_visible_card_view();
        if (view == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 700);
        return true;
    case 7:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[GRID_ACTION_DEL]), 650);
        return true;
    case 8:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&mode_buttons[2]), 650);
        return true;
    default:
        return false;
    }
}
#endif
