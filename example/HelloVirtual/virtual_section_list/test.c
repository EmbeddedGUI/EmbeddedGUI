#include "egui.h"

#include <stdio.h>
#include <string.h>

#include "uicode.h"

#define SECTION_DEMO_MAX_SECTIONS          28U
#define SECTION_DEMO_INITIAL_SECTIONS      24U
#define SECTION_DEMO_MAX_ITEMS_PER_SECTION 56U
#define SECTION_DEMO_INVALID_INDEX         0xFFFFFFFFUL
#define SECTION_DEMO_STATUS_TEXT_LEN       96
#define SECTION_DEMO_TITLE_TEXT_LEN        40
#define SECTION_DEMO_META_TEXT_LEN         48
#define SECTION_DEMO_HINT_TEXT_LEN         24
#define SECTION_DEMO_BODY_TEXT_LEN         56
#define SECTION_DEMO_BADGE_TEXT_LEN        16
#define SECTION_DEMO_STATE_CACHE_COUNT     96U

#define SECTION_DEMO_MARGIN_X              8
#define SECTION_DEMO_TOP_Y                 8
#define SECTION_DEMO_HEADER_W              (EGUI_CONFIG_SCEEN_WIDTH - SECTION_DEMO_MARGIN_X * 2)
#define SECTION_DEMO_HEADER_H              68
#define SECTION_DEMO_TOOLBAR_Y             (SECTION_DEMO_TOP_Y + SECTION_DEMO_HEADER_H + 6)
#define SECTION_DEMO_TOOLBAR_H             32
#define SECTION_DEMO_VIEW_Y                (SECTION_DEMO_TOOLBAR_Y + SECTION_DEMO_TOOLBAR_H + 6)
#define SECTION_DEMO_VIEW_W                SECTION_DEMO_HEADER_W
#define SECTION_DEMO_VIEW_H                (EGUI_CONFIG_SCEEN_HEIGHT - SECTION_DEMO_VIEW_Y - 8)
#define SECTION_DEMO_BUTTON_GAP            4
#define SECTION_DEMO_BUTTON_W              ((SECTION_DEMO_HEADER_W - 20 - SECTION_DEMO_BUTTON_GAP * 3) / 4)
#define SECTION_DEMO_BUTTON_H              20
#define SECTION_DEMO_HEADER_CARD_INSET_X   6
#define SECTION_DEMO_ITEM_CARD_INSET_LEFT  18
#define SECTION_DEMO_ITEM_CARD_INSET_RIGHT 6
#define SECTION_DEMO_ENTRY_GAP             4
#define SECTION_DEMO_BADGE_H               18
#define SECTION_DEMO_PROGRESS_H            5
#define SECTION_DEMO_HEADER_INSET_Y        1

#define SECTION_DEMO_FONT_HEADER       ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define SECTION_DEMO_FONT_HEADER_TITLE ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define SECTION_DEMO_FONT_ITEM_TITLE   ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define SECTION_DEMO_FONT_BODY         ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define SECTION_DEMO_FONT_CAP          ((const egui_font_t *)&egui_res_font_montserrat_8_4)

enum
{
    SECTION_DEMO_ACTION_ADD = 0,
    SECTION_DEMO_ACTION_DEL,
    SECTION_DEMO_ACTION_PATCH,
    SECTION_DEMO_ACTION_JUMP,
    SECTION_DEMO_ACTION_COUNT,
};

enum
{
    SECTION_DEMO_TONE_INBOX = 0,
    SECTION_DEMO_TONE_OPS,
    SECTION_DEMO_TONE_CHAT,
    SECTION_DEMO_TONE_AUDIT,
    SECTION_DEMO_TONE_COUNT,
};

enum
{
    SECTION_DEMO_ITEM_VARIANT_COMPACT = 0,
    SECTION_DEMO_ITEM_VARIANT_DETAIL,
    SECTION_DEMO_ITEM_VARIANT_ALERT,
    SECTION_DEMO_ITEM_VARIANT_COUNT,
};

enum
{
    SECTION_DEMO_ITEM_STATE_IDLE = 0,
    SECTION_DEMO_ITEM_STATE_LIVE,
    SECTION_DEMO_ITEM_STATE_WARN,
    SECTION_DEMO_ITEM_STATE_DONE,
    SECTION_DEMO_ITEM_STATE_COUNT,
};

typedef struct section_demo_item section_demo_item_t;
typedef struct section_demo_section section_demo_section_t;
typedef struct section_demo_header_view section_demo_header_view_t;
typedef struct section_demo_item_view section_demo_item_view_t;
typedef struct section_demo_item_state section_demo_item_state_t;
typedef struct section_demo_context section_demo_context_t;

struct section_demo_item
{
    uint32_t stable_id;
    uint16_t revision;
    uint8_t variant;
    uint8_t state;
    uint8_t progress;
    uint8_t reserved;
};

struct section_demo_section
{
    uint32_t stable_id;
    uint16_t item_count;
    uint8_t collapsed;
    uint8_t tone;
    uint8_t revision;
    uint8_t reserved;
    section_demo_item_t items[SECTION_DEMO_MAX_ITEMS_PER_SECTION];
};

struct section_demo_header_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_card_t accent;
    egui_view_label_t title;
    egui_view_label_t meta;
    egui_view_label_t hint;
    uint32_t section_index;
    uint32_t stable_id;
};

struct section_demo_item_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_card_t accent;
    egui_view_label_t title;
    egui_view_label_t body;
    egui_view_label_t meta;
    egui_view_label_t badge;
    egui_view_progress_bar_t progress;
    egui_view_t pulse;
    egui_animation_alpha_t pulse_anim;
    egui_interpolator_linear_t pulse_interp;
    uint32_t section_index;
    uint32_t item_index;
    uint32_t stable_id;
    uint8_t pulse_running;
};

struct section_demo_item_state
{
    uint16_t pulse_elapsed_ms;
    uint8_t pulse_running;
    uint8_t pulse_alpha;
    uint8_t pulse_cycle_flip;
    uint8_t pulse_repeated;
};

struct section_demo_context
{
    uint32_t section_count;
    uint32_t next_stable_id;
    uint32_t selected_item_id;
    uint32_t last_clicked_section;
    uint32_t last_clicked_item;
    uint32_t click_count;
    uint32_t action_count;
    uint32_t section_cursor;
    uint32_t item_cursor;
    uint8_t created_header_count;
    uint8_t created_item_count;
    section_demo_section_t sections[SECTION_DEMO_MAX_SECTIONS];
    char header_title_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][SECTION_DEMO_TITLE_TEXT_LEN];
    char header_meta_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][SECTION_DEMO_META_TEXT_LEN];
    char header_hint_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][SECTION_DEMO_HINT_TEXT_LEN];
    char item_title_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][SECTION_DEMO_TITLE_TEXT_LEN];
    char item_body_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][SECTION_DEMO_BODY_TEXT_LEN];
    char item_meta_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][SECTION_DEMO_META_TEXT_LEN];
    char item_badge_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][SECTION_DEMO_BADGE_TEXT_LEN];
    char top_title_text[SECTION_DEMO_STATUS_TEXT_LEN];
    char top_detail_text[SECTION_DEMO_STATUS_TEXT_LEN];
    char top_hint_text[SECTION_DEMO_STATUS_TEXT_LEN];
    char last_action_text[SECTION_DEMO_STATUS_TEXT_LEN];
    section_demo_header_view_t header_views[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
    section_demo_item_view_t item_views[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
};

static const char *section_demo_action_names[SECTION_DEMO_ACTION_COUNT] = {"Add", "Del", "Patch", "Jump"};
static const char *section_demo_state_names[SECTION_DEMO_ITEM_STATE_COUNT] = {"IDLE", "LIVE", "WARN", "DONE"};
static const char *section_demo_header_labels[SECTION_DEMO_TONE_COUNT] = {"Inbox Lane", "Ops Board", "Chat Room", "Audit Batch"};
static const char *section_demo_item_labels[SECTION_DEMO_TONE_COUNT] = {"Mail", "Task", "Thread", "Record"};
static const char *section_demo_body_texts[SECTION_DEMO_TONE_COUNT][SECTION_DEMO_ITEM_VARIANT_COUNT] = {
        {"Unread mail.", "Pinned thread.", "Needs reply."},
        {"Ready queue.", "Deploy step.", "Escalated task."},
        {"Room update.", "Context synced.", "Priority ping."},
        {"Batch line.", "Trace detail.", "Flagged record."},
};

static egui_view_t background_view;
static egui_view_card_t header_card;
static egui_view_card_t toolbar_card;
static egui_view_label_t header_title;
static egui_view_label_t header_detail;
static egui_view_label_t header_hint;
static egui_view_button_t action_buttons[SECTION_DEMO_ACTION_COUNT];
static egui_view_virtual_section_list_t section_list_view;
static section_demo_context_t section_demo_ctx;

EGUI_VIEW_CARD_PARAMS_INIT(section_demo_header_card_params, SECTION_DEMO_MARGIN_X, SECTION_DEMO_TOP_Y, SECTION_DEMO_HEADER_W, SECTION_DEMO_HEADER_H, 14);
EGUI_VIEW_CARD_PARAMS_INIT(section_demo_toolbar_card_params, SECTION_DEMO_MARGIN_X, SECTION_DEMO_TOOLBAR_Y, SECTION_DEMO_HEADER_W, SECTION_DEMO_TOOLBAR_H, 12);
static const egui_view_virtual_section_list_params_t section_demo_view_params = {
        .region = {{SECTION_DEMO_MARGIN_X, SECTION_DEMO_VIEW_Y}, {SECTION_DEMO_VIEW_W, SECTION_DEMO_VIEW_H}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_entry_height = 60,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(section_demo_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF4F8), EGUI_COLOR_HEX(0xD8E5EE),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(section_demo_screen_bg_params, &section_demo_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(section_demo_screen_bg, &section_demo_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(section_demo_list_bg_param, EGUI_COLOR_HEX(0xF9FBFD), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(section_demo_list_bg_params, &section_demo_list_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_list_bg, &section_demo_list_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(section_demo_action_add_param, EGUI_COLOR_HEX(0xDFF3E9), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(section_demo_action_add_params, &section_demo_action_add_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_action_add_bg, &section_demo_action_add_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(section_demo_action_del_param, EGUI_COLOR_HEX(0xFFF1E1), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(section_demo_action_del_params, &section_demo_action_del_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_action_del_bg, &section_demo_action_del_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(section_demo_action_patch_param, EGUI_COLOR_HEX(0xE3F6F7), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(section_demo_action_patch_params, &section_demo_action_patch_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_action_patch_bg, &section_demo_action_patch_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(section_demo_action_jump_param, EGUI_COLOR_HEX(0xE5EEFF), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(section_demo_action_jump_params, &section_demo_action_jump_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_action_jump_bg, &section_demo_action_jump_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(section_demo_badge_selected_param, EGUI_COLOR_HEX(0x2F6CA8), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(section_demo_badge_selected_params, &section_demo_badge_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_badge_selected_bg, &section_demo_badge_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(section_demo_badge_idle_param, EGUI_COLOR_HEX(0x8D99A6), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(section_demo_badge_idle_params, &section_demo_badge_idle_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_badge_idle_bg, &section_demo_badge_idle_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(section_demo_badge_live_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(section_demo_badge_live_params, &section_demo_badge_live_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_badge_live_bg, &section_demo_badge_live_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(section_demo_badge_warn_param, EGUI_COLOR_HEX(0xE7B14A), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(section_demo_badge_warn_params, &section_demo_badge_warn_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_badge_warn_bg, &section_demo_badge_warn_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(section_demo_badge_done_param, EGUI_COLOR_HEX(0x56789A), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(section_demo_badge_done_params, &section_demo_badge_done_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_badge_done_bg, &section_demo_badge_done_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(section_demo_pulse_selected_param, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(section_demo_pulse_selected_params, &section_demo_pulse_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_pulse_selected_bg, &section_demo_pulse_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(section_demo_pulse_live_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(section_demo_pulse_live_params, &section_demo_pulse_live_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_pulse_live_bg, &section_demo_pulse_live_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(section_demo_pulse_warn_param, EGUI_COLOR_HEX(0xD08A2E), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(section_demo_pulse_warn_params, &section_demo_pulse_warn_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_pulse_warn_bg, &section_demo_pulse_warn_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(section_demo_pulse_done_param, EGUI_COLOR_HEX(0x5C9EE6), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(section_demo_pulse_done_params, &section_demo_pulse_done_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_demo_pulse_done_bg, &section_demo_pulse_done_params);

EGUI_SHADOW_PARAM_INIT_ROUND(section_demo_card_shadow, 10, 0, 3, EGUI_COLOR_BLACK, EGUI_ALPHA_20, 12);
EGUI_ANIMATION_ALPHA_PARAMS_INIT(section_demo_pulse_anim_param, EGUI_ALPHA_100, EGUI_ALPHA_20);

static void section_demo_refresh_status(void);
static void section_demo_apply_action(uint8_t action);

static void section_demo_fill_item(section_demo_item_t *item, uint32_t stable_id, uint32_t seed)
{
    uint8_t progress = (uint8_t)(18U + ((seed * 11U) % 71U));

    memset(item, 0, sizeof(*item));
    item->stable_id = stable_id;
    item->revision = (uint16_t)(1U + (seed % 9U));
    item->variant = (uint8_t)(seed % SECTION_DEMO_ITEM_VARIANT_COUNT);
    item->state = (uint8_t)((seed / 3U) % SECTION_DEMO_ITEM_STATE_COUNT);

    if (item->variant == SECTION_DEMO_ITEM_VARIANT_ALERT && item->state == SECTION_DEMO_ITEM_STATE_IDLE)
    {
        item->state = SECTION_DEMO_ITEM_STATE_WARN;
    }
    if (item->state == SECTION_DEMO_ITEM_STATE_DONE)
    {
        progress = 100U;
    }
    if (item->variant == SECTION_DEMO_ITEM_VARIANT_COMPACT && progress < 30U)
    {
        progress = (uint8_t)(progress + 20U);
    }

    item->progress = progress;
}

static void section_demo_fill_section(section_demo_section_t *section, uint32_t stable_id, uint32_t seed)
{
    uint32_t item_index;

    memset(section, 0, sizeof(*section));
    section->stable_id = stable_id;
    section->tone = (uint8_t)(seed % SECTION_DEMO_TONE_COUNT);
    section->collapsed = (uint8_t)((seed % 7U) == 0U ? 1U : 0U);
    section->revision = (uint8_t)(1U + (seed % 5U));
    section->item_count = (uint16_t)(34U + ((seed * 7U) % 22U));

    for (item_index = 0; item_index < section->item_count; item_index++)
    {
        section_demo_fill_item(&section->items[item_index], section_demo_ctx.next_stable_id++, seed * 17U + item_index + 1U);
    }
}

static void section_demo_reset_model(void)
{
    uint32_t section_index;

    memset(&section_demo_ctx, 0, sizeof(section_demo_ctx));
    section_demo_ctx.section_count = SECTION_DEMO_INITIAL_SECTIONS;
    section_demo_ctx.next_stable_id = 500001U;
    section_demo_ctx.selected_item_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    section_demo_ctx.last_clicked_section = SECTION_DEMO_INVALID_INDEX;
    section_demo_ctx.last_clicked_item = SECTION_DEMO_INVALID_INDEX;

    for (section_index = 0; section_index < section_demo_ctx.section_count; section_index++)
    {
        section_demo_fill_section(&section_demo_ctx.sections[section_index], section_demo_ctx.next_stable_id++, section_index);
    }

    snprintf(section_demo_ctx.last_action_text, sizeof(section_demo_ctx.last_action_text), "Tap header to fold. Tap row to select.");
}

static uint32_t section_demo_get_visible_item_count(void)
{
    uint32_t section_index;
    uint32_t total = 0;

    for (section_index = 0; section_index < section_demo_ctx.section_count; section_index++)
    {
        if (!section_demo_ctx.sections[section_index].collapsed)
        {
            total += section_demo_ctx.sections[section_index].item_count;
        }
    }

    return total;
}

static uint8_t section_demo_find_item_position_by_stable_id(uint32_t stable_id, uint32_t *section_index, uint32_t *item_index)
{
    uint32_t current_section_index;

    if (section_index != NULL)
    {
        *section_index = SECTION_DEMO_INVALID_INDEX;
    }
    if (item_index != NULL)
    {
        *item_index = SECTION_DEMO_INVALID_INDEX;
    }

    for (current_section_index = 0; current_section_index < section_demo_ctx.section_count; current_section_index++)
    {
        uint32_t current_item_index;
        const section_demo_section_t *section = &section_demo_ctx.sections[current_section_index];

        for (current_item_index = 0; current_item_index < section->item_count; current_item_index++)
        {
            if (section->items[current_item_index].stable_id == stable_id)
            {
                if (section_index != NULL)
                {
                    *section_index = current_section_index;
                }
                if (item_index != NULL)
                {
                    *item_index = current_item_index;
                }
                return 1;
            }
        }
    }

    return 0;
}

static uint8_t section_demo_is_selected_item(uint32_t stable_id)
{
    return (uint8_t)(stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && stable_id == section_demo_ctx.selected_item_id);
}

static int32_t section_demo_measure_header_height_with_state(const section_demo_section_t *section)
{
    int32_t height = section->collapsed ? 40 : 48;

    if (section->tone == SECTION_DEMO_TONE_AUDIT)
    {
        height += 2;
    }

    return height;
}

static int32_t section_demo_measure_item_height_with_state(const section_demo_item_t *item, uint8_t selected)
{
    int32_t height;

    if (item->variant == SECTION_DEMO_ITEM_VARIANT_ALERT)
    {
        height = 76;
    }
    else if (item->variant == SECTION_DEMO_ITEM_VARIANT_DETAIL)
    {
        height = 64;
    }
    else
    {
        height = 44;
    }

    if (item->state == SECTION_DEMO_ITEM_STATE_LIVE)
    {
        height += 2;
    }
    else if (item->state == SECTION_DEMO_ITEM_STATE_WARN)
    {
        height += 4;
    }

    if (selected)
    {
        height += 6;
    }

    return height;
}

static uint8_t section_demo_item_has_pulse(const section_demo_item_t *item, uint8_t selected)
{
    return (uint8_t)(selected || item->state == SECTION_DEMO_ITEM_STATE_LIVE || item->state == SECTION_DEMO_ITEM_STATE_WARN);
}

static egui_dim_t section_demo_get_view_width(void)
{
    egui_dim_t width = EGUI_VIEW_OF(&section_list_view)->region.size.width;
    return width > 0 ? width : SECTION_DEMO_VIEW_W;
}

static int section_demo_get_header_pool_index(section_demo_header_view_t *header_view)
{
    uint8_t i;

    for (i = 0; i < section_demo_ctx.created_header_count; i++)
    {
        if (header_view == &section_demo_ctx.header_views[i])
        {
            return (int)i;
        }
    }

    return -1;
}

static int section_demo_get_item_pool_index(section_demo_item_view_t *item_view)
{
    uint8_t i;

    for (i = 0; i < section_demo_ctx.created_item_count; i++)
    {
        if (item_view == &section_demo_ctx.item_views[i])
        {
            return (int)i;
        }
    }

    return -1;
}

static section_demo_header_view_t *section_demo_find_header_view_by_root(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < section_demo_ctx.created_header_count; i++)
    {
        if (view == EGUI_VIEW_OF(&section_demo_ctx.header_views[i].root))
        {
            return &section_demo_ctx.header_views[i];
        }
    }

    return NULL;
}

static section_demo_item_view_t *section_demo_find_item_view_by_root(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < section_demo_ctx.created_item_count; i++)
    {
        if (view == EGUI_VIEW_OF(&section_demo_ctx.item_views[i].root))
        {
            return &section_demo_ctx.item_views[i];
        }
    }

    return NULL;
}

static void section_demo_set_item_pulse(section_demo_item_view_t *item_view, const section_demo_item_t *item, uint8_t visible, uint8_t selected)
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
        pulse_bg = EGUI_BG_OF(&section_demo_pulse_selected_bg);
    }
    else if (item->state == SECTION_DEMO_ITEM_STATE_WARN)
    {
        pulse_bg = EGUI_BG_OF(&section_demo_pulse_warn_bg);
    }
    else if (item->state == SECTION_DEMO_ITEM_STATE_DONE)
    {
        pulse_bg = EGUI_BG_OF(&section_demo_pulse_done_bg);
    }
    else
    {
        pulse_bg = EGUI_BG_OF(&section_demo_pulse_live_bg);
    }

    egui_view_set_gone(EGUI_VIEW_OF(&item_view->pulse), 0);
    egui_view_set_background(EGUI_VIEW_OF(&item_view->pulse), pulse_bg);
    if (!item_view->pulse_running)
    {
        egui_animation_start(EGUI_ANIM_OF(&item_view->pulse_anim));
        item_view->pulse_running = 1;
    }
}

static void section_demo_capture_item_state(section_demo_item_view_t *item_view, section_demo_item_state_t *state)
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

static void section_demo_restore_item_state(section_demo_item_view_t *item_view, const section_demo_item_state_t *state)
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

static uint32_t section_demo_count_state(const section_demo_section_t *section, uint8_t state)
{
    uint32_t item_index;
    uint32_t count = 0;

    for (item_index = 0; item_index < section->item_count; item_index++)
    {
        if (section->items[item_index].state == state)
        {
            count++;
        }
    }

    return count;
}

static void section_demo_refresh_status(void)
{
    uint32_t selected_section_index;
    uint32_t selected_item_index;

    snprintf(section_demo_ctx.top_title_text, sizeof(section_demo_ctx.top_title_text), "Virtual Section List");

    if (section_demo_find_item_position_by_stable_id(section_demo_ctx.selected_item_id, &selected_section_index, &selected_item_index))
    {
        snprintf(section_demo_ctx.top_detail_text, sizeof(section_demo_ctx.top_detail_text), "Sections %lu | rows %lu | sel %02lu-%02lu",
                 (unsigned long)section_demo_ctx.section_count, (unsigned long)section_demo_get_visible_item_count(), (unsigned long)selected_section_index,
                 (unsigned long)selected_item_index);
    }
    else
    {
        snprintf(section_demo_ctx.top_detail_text, sizeof(section_demo_ctx.top_detail_text), "Sections %lu | rows %lu | sel none",
                 (unsigned long)section_demo_ctx.section_count, (unsigned long)section_demo_get_visible_item_count());
    }

    snprintf(section_demo_ctx.top_hint_text, sizeof(section_demo_ctx.top_hint_text), "%s", section_demo_ctx.last_action_text);

    if (EGUI_VIEW_OF(&header_title)->api == NULL)
    {
        return;
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&header_title), section_demo_ctx.top_title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_detail), section_demo_ctx.top_detail_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_hint), section_demo_ctx.top_hint_text);
}

static void section_demo_header_click_cb(egui_view_t *self)
{
    egui_view_virtual_section_list_entry_t entry;
    section_demo_section_t *section;
    uint32_t selected_section_index;
    uint32_t selected_item_index;

    if (!egui_view_virtual_section_list_resolve_entry_by_view(EGUI_VIEW_OF(&section_list_view), self, &entry) || !entry.is_section_header ||
        entry.section_index >= section_demo_ctx.section_count)
    {
        return;
    }

    section = &section_demo_ctx.sections[entry.section_index];
    section->collapsed = section->collapsed ? 0U : 1U;
    section_demo_ctx.last_clicked_section = entry.section_index;
    section_demo_ctx.click_count++;

    if (section->collapsed && section_demo_find_item_position_by_stable_id(section_demo_ctx.selected_item_id, &selected_section_index, &selected_item_index) &&
        selected_section_index == entry.section_index)
    {
        section_demo_ctx.selected_item_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    snprintf(section_demo_ctx.last_action_text, sizeof(section_demo_ctx.last_action_text), "%s group %02lu", section->collapsed ? "Fold" : "Open",
             (unsigned long)entry.section_index);
    egui_view_virtual_section_list_notify_data_changed(EGUI_VIEW_OF(&section_list_view));
    egui_view_virtual_section_list_scroll_to_section(EGUI_VIEW_OF(&section_list_view), entry.section_index, 0);
    section_demo_refresh_status();
}

static void section_demo_item_click_cb(egui_view_t *self)
{
    egui_view_virtual_section_list_entry_t entry;
    uint32_t previous_selected = section_demo_ctx.selected_item_id;
    uint32_t previous_section_index;
    uint32_t previous_item_index;

    if (!egui_view_virtual_section_list_resolve_entry_by_view(EGUI_VIEW_OF(&section_list_view), self, &entry) || entry.is_section_header ||
        entry.stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return;
    }

    section_demo_ctx.selected_item_id = entry.stable_id;
    section_demo_ctx.last_clicked_section = entry.section_index;
    section_demo_ctx.last_clicked_item = entry.item_index;
    section_demo_ctx.click_count++;

    snprintf(section_demo_ctx.last_action_text, sizeof(section_demo_ctx.last_action_text), "Click row %02lu-%02lu", (unsigned long)entry.section_index,
             (unsigned long)entry.item_index);

    if (previous_selected != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && previous_selected != entry.stable_id &&
        section_demo_find_item_position_by_stable_id(previous_selected, &previous_section_index, &previous_item_index))
    {
        egui_view_virtual_section_list_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&section_list_view), previous_selected);
    }

    egui_view_virtual_section_list_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&section_list_view), entry.stable_id);
    egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(EGUI_VIEW_OF(&section_list_view), entry.stable_id, SECTION_DEMO_ENTRY_GAP / 2);
    section_demo_refresh_status();

    EGUI_LOG_INF("Virtual section row clicked: section=%d item=%d stable_id=%lu\n", (int)entry.section_index, (int)entry.item_index,
                 (unsigned long)entry.stable_id);
}

static uint32_t section_demo_get_section_count_adapter(void *data_source_context)
{
    return ((section_demo_context_t *)data_source_context)->section_count;
}

static uint32_t section_demo_get_section_stable_id_adapter(void *data_source_context, uint32_t section_index)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    return ctx->sections[section_index].stable_id;
}

static int32_t section_demo_find_section_index_adapter(void *data_source_context, uint32_t stable_id)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    uint32_t section_index;

    for (section_index = 0; section_index < ctx->section_count; section_index++)
    {
        if (ctx->sections[section_index].stable_id == stable_id)
        {
            return (int32_t)section_index;
        }
    }

    return -1;
}

static uint32_t section_demo_get_item_count_adapter(void *data_source_context, uint32_t section_index)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    return ctx->sections[section_index].collapsed ? 0U : ctx->sections[section_index].item_count;
}

static uint32_t section_demo_get_item_stable_id_adapter(void *data_source_context, uint32_t section_index, uint32_t item_index)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    return ctx->sections[section_index].items[item_index].stable_id;
}

static uint8_t section_demo_find_item_position_adapter(void *data_source_context, uint32_t stable_id, uint32_t *section_index, uint32_t *item_index)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    uint32_t current_section_index;

    if (section_index != NULL)
    {
        *section_index = SECTION_DEMO_INVALID_INDEX;
    }
    if (item_index != NULL)
    {
        *item_index = SECTION_DEMO_INVALID_INDEX;
    }

    for (current_section_index = 0; current_section_index < ctx->section_count; current_section_index++)
    {
        uint32_t current_item_index;
        const section_demo_section_t *section = &ctx->sections[current_section_index];

        for (current_item_index = 0; current_item_index < section->item_count; current_item_index++)
        {
            if (section->items[current_item_index].stable_id == stable_id)
            {
                if (section_index != NULL)
                {
                    *section_index = current_section_index;
                }
                if (item_index != NULL)
                {
                    *item_index = current_item_index;
                }
                return 1;
            }
        }
    }

    return 0;
}

static int32_t section_demo_measure_header_adapter(void *data_source_context, uint32_t section_index, int32_t width_hint)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    EGUI_UNUSED(width_hint);
    return section_demo_measure_header_height_with_state(&ctx->sections[section_index]);
}

static int32_t section_demo_measure_item_adapter(void *data_source_context, uint32_t section_index, uint32_t item_index, int32_t width_hint)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    const section_demo_item_t *item = &ctx->sections[section_index].items[item_index];
    EGUI_UNUSED(width_hint);
    return section_demo_measure_item_height_with_state(item, section_demo_is_selected_item(item->stable_id));
}

static egui_view_t *section_demo_create_header_view(void *data_source_context, uint16_t view_type)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    section_demo_header_view_t *header_view;

    EGUI_UNUSED(view_type);

    if (ctx->created_header_count >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
    {
        return NULL;
    }

    header_view = &ctx->header_views[ctx->created_header_count];
    memset(header_view, 0, sizeof(*header_view));
    header_view->section_index = SECTION_DEMO_INVALID_INDEX;
    header_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;

    egui_view_group_init(EGUI_VIEW_OF(&header_view->root));
    egui_view_card_init(EGUI_VIEW_OF(&header_view->card));
    egui_view_card_init(EGUI_VIEW_OF(&header_view->accent));
    egui_view_label_init(EGUI_VIEW_OF(&header_view->title));
    egui_view_label_init(EGUI_VIEW_OF(&header_view->meta));
    egui_view_label_init(EGUI_VIEW_OF(&header_view->hint));

    egui_view_label_set_font(EGUI_VIEW_OF(&header_view->title), SECTION_DEMO_FONT_HEADER_TITLE);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_view->meta), SECTION_DEMO_FONT_CAP);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_view->hint), SECTION_DEMO_FONT_CAP);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_view->title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_view->meta), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_view->hint), EGUI_ALIGN_CENTER);

    egui_view_group_add_child(EGUI_VIEW_OF(&header_view->root), EGUI_VIEW_OF(&header_view->card));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_view->card), EGUI_VIEW_OF(&header_view->accent));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_view->card), EGUI_VIEW_OF(&header_view->title));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_view->card), EGUI_VIEW_OF(&header_view->meta));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_view->card), EGUI_VIEW_OF(&header_view->hint));
    egui_view_set_shadow(EGUI_VIEW_OF(&header_view->card), &section_demo_card_shadow);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&header_view->root), section_demo_header_click_cb);

    ctx->created_header_count++;
    return EGUI_VIEW_OF(&header_view->root);
}

static egui_view_t *section_demo_create_item_view(void *data_source_context, uint16_t view_type)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    section_demo_item_view_t *item_view;

    EGUI_UNUSED(view_type);

    if (ctx->created_item_count >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
    {
        return NULL;
    }

    item_view = &ctx->item_views[ctx->created_item_count];
    memset(item_view, 0, sizeof(*item_view));
    item_view->section_index = SECTION_DEMO_INVALID_INDEX;
    item_view->item_index = SECTION_DEMO_INVALID_INDEX;
    item_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;

    egui_view_group_init(EGUI_VIEW_OF(&item_view->root));
    egui_view_card_init(EGUI_VIEW_OF(&item_view->card));
    egui_view_card_init(EGUI_VIEW_OF(&item_view->accent));
    egui_view_label_init(EGUI_VIEW_OF(&item_view->title));
    egui_view_label_init(EGUI_VIEW_OF(&item_view->body));
    egui_view_label_init(EGUI_VIEW_OF(&item_view->meta));
    egui_view_label_init(EGUI_VIEW_OF(&item_view->badge));
    egui_view_progress_bar_init(EGUI_VIEW_OF(&item_view->progress));
    egui_view_init(EGUI_VIEW_OF(&item_view->pulse));

    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->title), SECTION_DEMO_FONT_ITEM_TITLE);
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->body), SECTION_DEMO_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->meta), SECTION_DEMO_FONT_CAP);
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->badge), SECTION_DEMO_FONT_CAP);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->body), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->meta), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->badge), EGUI_ALIGN_CENTER);

    egui_view_group_add_child(EGUI_VIEW_OF(&item_view->root), EGUI_VIEW_OF(&item_view->card));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->accent));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->title));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->body));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->meta));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->badge));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->progress));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->pulse));
    egui_view_set_shadow(EGUI_VIEW_OF(&item_view->card), &section_demo_card_shadow);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&item_view->root), section_demo_item_click_cb);

    egui_animation_alpha_init(EGUI_ANIM_OF(&item_view->pulse_anim));
    egui_animation_alpha_params_set(&item_view->pulse_anim, &section_demo_pulse_anim_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&item_view->pulse_anim), 900);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&item_view->pulse_anim), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&item_view->pulse_anim), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_interpolator_linear_init((egui_interpolator_t *)&item_view->pulse_interp);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&item_view->pulse_anim), (egui_interpolator_t *)&item_view->pulse_interp);
    egui_animation_target_view_set(EGUI_ANIM_OF(&item_view->pulse_anim), EGUI_VIEW_OF(&item_view->pulse));
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->pulse), 1);

    ctx->created_item_count++;
    return EGUI_VIEW_OF(&item_view->root);
}

static void section_demo_bind_header_view(void *data_source_context, egui_view_t *view, uint32_t section_index, uint32_t stable_id)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    section_demo_header_view_t *header_view = section_demo_find_header_view_by_root(view);
    section_demo_section_t *section;
    int pool_index;
    egui_dim_t view_width;
    egui_dim_t card_x;
    egui_dim_t card_w;
    egui_dim_t card_h;
    egui_color_t fill_color;
    egui_color_t border_color;
    egui_color_t accent_color;
    egui_color_t title_color;
    egui_color_t meta_color;
    egui_dim_t accent_h;

    if (header_view == NULL || section_index >= ctx->section_count)
    {
        return;
    }

    pool_index = section_demo_get_header_pool_index(header_view);
    if (pool_index < 0)
    {
        return;
    }

    section = &ctx->sections[section_index];
    header_view->section_index = section_index;
    header_view->stable_id = stable_id;

    snprintf(ctx->header_title_texts[pool_index], sizeof(ctx->header_title_texts[pool_index]), "%s %02lu", section_demo_header_labels[section->tone],
             (unsigned long)section_index);
    snprintf(ctx->header_meta_texts[pool_index], sizeof(ctx->header_meta_texts[pool_index]), "%lu rows | %lu live | r%u", (unsigned long)section->item_count,
             (unsigned long)section_demo_count_state(section, SECTION_DEMO_ITEM_STATE_LIVE), (unsigned)section->revision);
    snprintf(ctx->header_hint_texts[pool_index], sizeof(ctx->header_hint_texts[pool_index]), "%s", section->collapsed ? "Open" : "Fold");

    egui_view_label_set_text(EGUI_VIEW_OF(&header_view->title), ctx->header_title_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_view->meta), ctx->header_meta_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_view->hint), ctx->header_hint_texts[pool_index]);

    if (section->tone == SECTION_DEMO_TONE_OPS)
    {
        fill_color = section->collapsed ? EGUI_COLOR_HEX(0xF1F8F3) : EGUI_COLOR_HEX(0xE5F3EA);
        border_color = EGUI_COLOR_HEX(0xB5D2BE);
        accent_color = EGUI_COLOR_HEX(0x3E8D64);
        title_color = EGUI_COLOR_HEX(0x234332);
        meta_color = EGUI_COLOR_HEX(0x507061);
    }
    else if (section->tone == SECTION_DEMO_TONE_CHAT)
    {
        fill_color = section->collapsed ? EGUI_COLOR_HEX(0xF1F6FC) : EGUI_COLOR_HEX(0xE7F0FA);
        border_color = EGUI_COLOR_HEX(0xC0D4E7);
        accent_color = EGUI_COLOR_HEX(0x4F84BE);
        title_color = EGUI_COLOR_HEX(0x23405B);
        meta_color = EGUI_COLOR_HEX(0x5E7891);
    }
    else if (section->tone == SECTION_DEMO_TONE_AUDIT)
    {
        fill_color = section->collapsed ? EGUI_COLOR_HEX(0xFFF8EE) : EGUI_COLOR_HEX(0xFFF2E1);
        border_color = EGUI_COLOR_HEX(0xE2C893);
        accent_color = EGUI_COLOR_HEX(0xC98D3C);
        title_color = EGUI_COLOR_HEX(0x5B3B12);
        meta_color = EGUI_COLOR_HEX(0x8E6736);
    }
    else
    {
        fill_color = section->collapsed ? EGUI_COLOR_HEX(0xF5F8FB) : EGUI_COLOR_HEX(0xECF2F8);
        border_color = EGUI_COLOR_HEX(0xC6D5E2);
        accent_color = EGUI_COLOR_HEX(0x6F8AA4);
        title_color = EGUI_COLOR_HEX(0x283848);
        meta_color = EGUI_COLOR_HEX(0x65788A);
    }

    view_width = section_demo_get_view_width();
    card_x = 4;
    card_w = view_width - card_x - 4;
    card_h = (egui_dim_t)(section_demo_measure_header_height_with_state(section) - SECTION_DEMO_HEADER_INSET_Y * 2);
    accent_h = card_h > 12 ? (egui_dim_t)(card_h - 12) : card_h;

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_view->card), fill_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_view->card), 1, border_color);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&header_view->card), 14);
    egui_view_set_shadow(EGUI_VIEW_OF(&header_view->card), NULL);
    egui_view_set_position(EGUI_VIEW_OF(&header_view->card), card_x, SECTION_DEMO_HEADER_INSET_Y);
    egui_view_set_size(EGUI_VIEW_OF(&header_view->card), card_w, card_h);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_view->accent), accent_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_view->accent), 0, accent_color);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&header_view->accent), 3);
    egui_view_set_position(EGUI_VIEW_OF(&header_view->accent), 8, 6);
    egui_view_set_size(EGUI_VIEW_OF(&header_view->accent), 6, accent_h);
    egui_view_set_position(EGUI_VIEW_OF(&header_view->title), 22, 7);
    egui_view_set_size(EGUI_VIEW_OF(&header_view->title), card_w - 78, 13);
    egui_view_set_position(EGUI_VIEW_OF(&header_view->meta), 22, section->collapsed ? 22 : 25);
    egui_view_set_size(EGUI_VIEW_OF(&header_view->meta), card_w - 88, 12);
    egui_view_set_position(EGUI_VIEW_OF(&header_view->hint), card_w - 54, (egui_dim_t)((card_h - SECTION_DEMO_BADGE_H) / 2));
    egui_view_set_size(EGUI_VIEW_OF(&header_view->hint), 44, SECTION_DEMO_BADGE_H);

    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_view->title), title_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_view->meta), meta_color, EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&header_view->hint),
                             section->collapsed ? EGUI_BG_OF(&section_demo_badge_idle_bg) : EGUI_BG_OF(&section_demo_badge_live_bg));
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_view->hint), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void section_demo_bind_item_view(void *data_source_context, egui_view_t *view, uint32_t section_index, uint32_t item_index, uint32_t stable_id)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    section_demo_item_view_t *item_view = section_demo_find_item_view_by_root(view);
    const section_demo_section_t *section;
    const section_demo_item_t *item;
    int pool_index;
    uint8_t selected;
    egui_dim_t view_width;
    egui_dim_t card_x;
    egui_dim_t card_w;
    egui_dim_t card_h;
    egui_dim_t accent_w;
    egui_dim_t accent_h;
    egui_dim_t content_x;
    egui_dim_t body_w;
    egui_dim_t progress_w;
    egui_dim_t badge_w;
    egui_dim_t badge_y;
    egui_dim_t title_w;
    egui_dim_t title_y;
    egui_dim_t title_h;
    egui_dim_t body_y;
    egui_dim_t body_h;
    egui_dim_t meta_y;
    egui_dim_t meta_h;
    egui_dim_t progress_y;
    egui_dim_t corner_radius;
    uint8_t show_body;
    uint8_t show_progress;
    egui_color_t fill_color;
    egui_color_t border_color;
    egui_color_t accent_color;
    egui_color_t title_color;
    egui_color_t body_color;
    egui_color_t meta_color;
    egui_background_t *badge_bg;
    egui_color_t badge_text_color;

    if (item_view == NULL || section_index >= ctx->section_count || item_index >= ctx->sections[section_index].item_count)
    {
        return;
    }

    pool_index = section_demo_get_item_pool_index(item_view);
    if (pool_index < 0)
    {
        return;
    }

    section = &ctx->sections[section_index];
    item = &section->items[item_index];
    selected = section_demo_is_selected_item(stable_id);
    item_view->section_index = section_index;
    item_view->item_index = item_index;
    item_view->stable_id = stable_id;

    snprintf(ctx->item_title_texts[pool_index], sizeof(ctx->item_title_texts[pool_index]), "%s %02lu-%02lu", section_demo_item_labels[section->tone],
             (unsigned long)section_index, (unsigned long)item_index);
    snprintf(ctx->item_body_texts[pool_index], sizeof(ctx->item_body_texts[pool_index]), "%s",
             section_demo_body_texts[section->tone % SECTION_DEMO_TONE_COUNT][item->variant % SECTION_DEMO_ITEM_VARIANT_COUNT]);
    snprintf(ctx->item_meta_texts[pool_index], sizeof(ctx->item_meta_texts[pool_index]), "r%u  %u%%  #%03lu", (unsigned)item->revision,
             (unsigned)item->progress, (unsigned long)(item->stable_id % 1000U));
    snprintf(ctx->item_badge_texts[pool_index], sizeof(ctx->item_badge_texts[pool_index]), "%s", selected ? "SEL" : section_demo_state_names[item->state]);

    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->title), ctx->item_title_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->body), ctx->item_body_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->meta), ctx->item_meta_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->badge), ctx->item_badge_texts[pool_index]);

    if (selected)
    {
        fill_color = EGUI_COLOR_HEX(0xDCEBFA);
        border_color = EGUI_COLOR_HEX(0x2F6CA8);
        accent_color = EGUI_COLOR_HEX(0x2F6CA8);
        title_color = EGUI_COLOR_HEX(0x1E4468);
        body_color = EGUI_COLOR_HEX(0x4A6783);
        meta_color = EGUI_COLOR_HEX(0x59758F);
        badge_bg = EGUI_BG_OF(&section_demo_badge_selected_bg);
        badge_text_color = EGUI_COLOR_WHITE;
        item_view->progress.progress_color = EGUI_COLOR_HEX(0x2F6CA8);
        item_view->progress.bk_color = EGUI_COLOR_HEX(0xA8C4DE);
    }
    else if (item->state == SECTION_DEMO_ITEM_STATE_WARN)
    {
        fill_color = EGUI_COLOR_HEX(0xFFF4E6);
        border_color = EGUI_COLOR_HEX(0xD89C57);
        accent_color = EGUI_COLOR_HEX(0xD18931);
        title_color = EGUI_COLOR_HEX(0x5E3C14);
        body_color = EGUI_COLOR_HEX(0x7B562B);
        meta_color = EGUI_COLOR_HEX(0x8A5A23);
        badge_bg = EGUI_BG_OF(&section_demo_badge_warn_bg);
        badge_text_color = EGUI_COLOR_BLACK;
        item_view->progress.progress_color = EGUI_COLOR_HEX(0xD28732);
        item_view->progress.bk_color = EGUI_COLOR_HEX(0xF0DFC8);
    }
    else if (item->state == SECTION_DEMO_ITEM_STATE_LIVE)
    {
        fill_color = EGUI_COLOR_HEX(0xEAF6EE);
        border_color = EGUI_COLOR_HEX(0x93BE9F);
        accent_color = EGUI_COLOR_HEX(0x2F976A);
        title_color = EGUI_COLOR_HEX(0x214A34);
        body_color = EGUI_COLOR_HEX(0x4F6F5C);
        meta_color = EGUI_COLOR_HEX(0x406E4E);
        badge_bg = EGUI_BG_OF(&section_demo_badge_live_bg);
        badge_text_color = EGUI_COLOR_WHITE;
        item_view->progress.progress_color = EGUI_COLOR_HEX(0x2E9A6F);
        item_view->progress.bk_color = EGUI_COLOR_HEX(0xD9E9DE);
    }
    else if (item->state == SECTION_DEMO_ITEM_STATE_DONE)
    {
        fill_color = EGUI_COLOR_HEX(0xEEF3F8);
        border_color = EGUI_COLOR_HEX(0xB7C4D0);
        accent_color = EGUI_COLOR_HEX(0x6D89A6);
        title_color = EGUI_COLOR_HEX(0x2C3E50);
        body_color = EGUI_COLOR_HEX(0x617486);
        meta_color = EGUI_COLOR_HEX(0x5D7287);
        badge_bg = EGUI_BG_OF(&section_demo_badge_done_bg);
        badge_text_color = EGUI_COLOR_WHITE;
        item_view->progress.progress_color = EGUI_COLOR_HEX(0x6B88A6);
        item_view->progress.bk_color = EGUI_COLOR_HEX(0xDCE6F0);
    }
    else
    {
        if (section->tone == SECTION_DEMO_TONE_OPS)
        {
            fill_color = EGUI_COLOR_HEX(0xF5FBF7);
            border_color = EGUI_COLOR_HEX(0xC8DED0);
            accent_color = EGUI_COLOR_HEX(0x599671);
            title_color = EGUI_COLOR_HEX(0x244233);
            body_color = EGUI_COLOR_HEX(0x5C7566);
        }
        else if (section->tone == SECTION_DEMO_TONE_CHAT)
        {
            fill_color = EGUI_COLOR_HEX(0xF4F8FD);
            border_color = EGUI_COLOR_HEX(0xC8D8E8);
            accent_color = EGUI_COLOR_HEX(0x668FC0);
            title_color = EGUI_COLOR_HEX(0x28405A);
            body_color = EGUI_COLOR_HEX(0x607588);
        }
        else if (section->tone == SECTION_DEMO_TONE_AUDIT)
        {
            fill_color = EGUI_COLOR_HEX(0xFFFAF2);
            border_color = EGUI_COLOR_HEX(0xE4D4B2);
            accent_color = EGUI_COLOR_HEX(0xC89242);
            title_color = EGUI_COLOR_HEX(0x5B3D18);
            body_color = EGUI_COLOR_HEX(0x836238);
        }
        else
        {
            fill_color = EGUI_COLOR_WHITE;
            border_color = EGUI_COLOR_HEX(0xD5DEE6);
            accent_color = EGUI_COLOR_HEX(0x7B95AD);
            title_color = EGUI_COLOR_HEX(0x2A3A4A);
            body_color = EGUI_COLOR_HEX(0x607181);
        }
        meta_color = EGUI_COLOR_HEX(0x667A8B);
        badge_bg = EGUI_BG_OF(&section_demo_badge_idle_bg);
        badge_text_color = EGUI_COLOR_WHITE;
        item_view->progress.progress_color = EGUI_COLOR_HEX(0x7A8EA4);
        item_view->progress.bk_color = EGUI_COLOR_HEX(0xE2E8EE);
    }
    item_view->progress.control_color = item_view->progress.progress_color;

    view_width = section_demo_get_view_width();
    title_h = 12;
    body_h = 11;
    meta_h = 10;
    if (item->variant == SECTION_DEMO_ITEM_VARIANT_ALERT)
    {
        card_x = 10;
        badge_w = 48;
        accent_w = 7;
        content_x = 24;
        title_y = 8;
        body_y = 28;
        meta_y = 47;
        corner_radius = 14;
        show_body = 1;
        show_progress = 1;
    }
    else if (item->variant == SECTION_DEMO_ITEM_VARIANT_DETAIL)
    {
        card_x = 22;
        badge_w = 44;
        accent_w = 5;
        content_x = 20;
        title_y = 8;
        body_y = 25;
        meta_y = 43;
        corner_radius = 12;
        show_body = 1;
        show_progress = 1;
    }
    else
    {
        card_x = 34;
        badge_w = 40;
        accent_w = 4;
        content_x = 18;
        title_y = 8;
        body_y = 0;
        corner_radius = 10;
        show_body = 0;
        show_progress = (uint8_t)(selected || item->state != SECTION_DEMO_ITEM_STATE_IDLE);
        meta_y = show_progress ? 20 : 22;
    }

    if (selected && card_x > 14)
    {
        card_x -= 6;
    }

    card_w = view_width - card_x - 8;
    card_h = (egui_dim_t)(section_demo_measure_item_height_with_state(item, selected) - SECTION_DEMO_ENTRY_GAP);
    accent_h = card_h > 14 ? (egui_dim_t)(card_h - 14) : card_h;
    badge_y = item->variant == SECTION_DEMO_ITEM_VARIANT_ALERT ? 8 : 7;
    title_w = card_w - content_x - badge_w - 14;
    body_w = card_w - content_x - 12;
    progress_w = card_w - content_x - 12;
    progress_y = show_progress ? (egui_dim_t)(card_h - 10) : 0;
    if (item->variant == SECTION_DEMO_ITEM_VARIANT_DETAIL && show_progress && meta_y > 1)
    {
        meta_y = (egui_dim_t)(progress_y - 12);
    }
    else if (item->variant == SECTION_DEMO_ITEM_VARIANT_ALERT && show_progress && meta_y > 1)
    {
        meta_y = (egui_dim_t)(progress_y - 15);
    }

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&item_view->card), fill_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&item_view->card), selected ? 2 : 1, border_color);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&item_view->card), corner_radius);
    egui_view_set_shadow(EGUI_VIEW_OF(&item_view->card), NULL);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&item_view->accent), accent_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&item_view->accent), 0, accent_color);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&item_view->accent), accent_w);
    egui_view_set_background(EGUI_VIEW_OF(&item_view->badge), badge_bg);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->title), title_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->body), body_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->meta), meta_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->badge), badge_text_color, EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->card), card_x, SECTION_DEMO_ENTRY_GAP / 2);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->card), card_w, card_h);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->accent), 8, 7);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->accent), accent_w, accent_h);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->badge), card_w - badge_w - 8, badge_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->badge), badge_w, SECTION_DEMO_BADGE_H);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->pulse), content_x, 11);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->pulse), 8, 8);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->title), content_x + 12, title_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->title), title_w, title_h);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->body), content_x + 12, body_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->body), body_w, body_h);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->meta), content_x + 12, meta_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->meta), body_w, meta_h);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->progress), content_x + 12, progress_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->progress), progress_w, SECTION_DEMO_PROGRESS_H);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&item_view->progress), item->progress);
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->body), show_body ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->progress), show_progress ? 0 : 1);
    section_demo_set_item_pulse(item_view, item, section_demo_item_has_pulse(item, selected), selected);
}

static void section_demo_unbind_header_view(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    section_demo_header_view_t *header_view = section_demo_find_header_view_by_root(view);
    EGUI_UNUSED(data_source_context);
    EGUI_UNUSED(stable_id);

    if (header_view != NULL)
    {
        header_view->section_index = SECTION_DEMO_INVALID_INDEX;
        header_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }
}

static void section_demo_unbind_item_view(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    section_demo_item_view_t *item_view = section_demo_find_item_view_by_root(view);
    EGUI_UNUSED(data_source_context);
    EGUI_UNUSED(stable_id);

    if (item_view == NULL)
    {
        return;
    }

    section_demo_set_item_pulse(item_view, NULL, 0, 0);
    item_view->section_index = SECTION_DEMO_INVALID_INDEX;
    item_view->item_index = SECTION_DEMO_INVALID_INDEX;
    item_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static uint8_t section_demo_should_keep_item_alive(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    section_demo_context_t *ctx = (section_demo_context_t *)data_source_context;
    uint32_t section_index;
    uint32_t item_index;
    EGUI_UNUSED(view);

    if (stable_id == ctx->selected_item_id)
    {
        return 1;
    }

    if (!section_demo_find_item_position_by_stable_id(stable_id, &section_index, &item_index))
    {
        return 0;
    }

    return section_demo_item_has_pulse(&ctx->sections[section_index].items[item_index], 0);
}

static void section_demo_save_item_state(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    section_demo_item_view_t *item_view = section_demo_find_item_view_by_root(view);
    section_demo_item_state_t state;
    EGUI_UNUSED(data_source_context);

    if (item_view == NULL)
    {
        return;
    }

    section_demo_capture_item_state(item_view, &state);
    if (!state.pulse_running)
    {
        egui_view_virtual_section_list_remove_entry_state_by_stable_id(EGUI_VIEW_OF(&section_list_view), stable_id);
        return;
    }

    (void)egui_view_virtual_section_list_write_entry_state_for_view(view, stable_id, &state, sizeof(state));
}

static void section_demo_restore_item_state_adapter(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    section_demo_item_view_t *item_view = section_demo_find_item_view_by_root(view);
    section_demo_item_state_t state;
    EGUI_UNUSED(data_source_context);

    if (item_view == NULL)
    {
        return;
    }

    if (egui_view_virtual_section_list_read_entry_state_for_view(view, stable_id, &state, sizeof(state)) != sizeof(state))
    {
        return;
    }

    section_demo_restore_item_state(item_view, &state);
}

static const egui_view_virtual_section_list_data_source_t section_demo_data_source = {
        .get_section_count = section_demo_get_section_count_adapter,
        .get_section_stable_id = section_demo_get_section_stable_id_adapter,
        .find_section_index_by_stable_id = section_demo_find_section_index_adapter,
        .get_item_count = section_demo_get_item_count_adapter,
        .get_item_stable_id = section_demo_get_item_stable_id_adapter,
        .find_item_position_by_stable_id = section_demo_find_item_position_adapter,
        .get_section_header_view_type = NULL,
        .get_item_view_type = NULL,
        .measure_section_header_height = section_demo_measure_header_adapter,
        .measure_item_height = section_demo_measure_item_adapter,
        .create_section_header_view = section_demo_create_header_view,
        .create_item_view = section_demo_create_item_view,
        .destroy_section_header_view = NULL,
        .destroy_item_view = NULL,
        .bind_section_header_view = section_demo_bind_header_view,
        .bind_item_view = section_demo_bind_item_view,
        .unbind_section_header_view = section_demo_unbind_header_view,
        .unbind_item_view = section_demo_unbind_item_view,
        .should_keep_section_header_alive = NULL,
        .should_keep_item_alive = section_demo_should_keep_item_alive,
        .save_section_header_state = NULL,
        .save_item_state = section_demo_save_item_state,
        .restore_section_header_state = NULL,
        .restore_item_state = section_demo_restore_item_state_adapter,
        .default_section_header_view_type = 0,
        .default_item_view_type = 0,
};

static uint8_t section_demo_pick_target(uint32_t *section_index, uint32_t *item_index)
{
    uint32_t offset;

    if (section_demo_ctx.section_count == 0)
    {
        return 0;
    }

    for (offset = 0; offset < section_demo_ctx.section_count; offset++)
    {
        uint32_t current_section_index = (section_demo_ctx.section_cursor + offset) % section_demo_ctx.section_count;
        section_demo_section_t *section = &section_demo_ctx.sections[current_section_index];

        if (section->item_count == 0)
        {
            continue;
        }

        section_demo_ctx.section_cursor = (current_section_index + 1U) % section_demo_ctx.section_count;
        section_demo_ctx.item_cursor = (section_demo_ctx.item_cursor + 5U) % section->item_count;

        if (section_index != NULL)
        {
            *section_index = current_section_index;
        }
        if (item_index != NULL)
        {
            *item_index = section_demo_ctx.item_cursor;
        }
        return 1;
    }

    return 0;
}

static void section_demo_action_add(void)
{
    uint32_t section_index;
    section_demo_section_t *section;
    uint32_t insert_index;
    uint32_t stable_id;

    if (section_demo_ctx.section_count == 0)
    {
        return;
    }

    section_index = section_demo_ctx.section_cursor % section_demo_ctx.section_count;
    section = &section_demo_ctx.sections[section_index];
    if (section->item_count >= SECTION_DEMO_MAX_ITEMS_PER_SECTION)
    {
        snprintf(section_demo_ctx.last_action_text, sizeof(section_demo_ctx.last_action_text), "Add ignored. Group %02lu is full.",
                 (unsigned long)section_index);
        section_demo_refresh_status();
        return;
    }

    insert_index = section->item_count == 0 ? 0U : (section_demo_ctx.item_cursor % (section->item_count + 1U));
    if (insert_index < section->item_count)
    {
        memmove(&section->items[insert_index + 1], &section->items[insert_index], (section->item_count - insert_index) * sizeof(section->items[0]));
    }

    stable_id = section_demo_ctx.next_stable_id++;
    section_demo_fill_item(&section->items[insert_index], stable_id, stable_id);
    section->items[insert_index].state = SECTION_DEMO_ITEM_STATE_LIVE;
    section->items[insert_index].progress = 22U;
    section->item_count++;
    section->collapsed = 0;
    section_demo_ctx.action_count++;

    egui_view_virtual_section_list_notify_item_inserted(EGUI_VIEW_OF(&section_list_view), section_index, insert_index, 1);
    egui_view_virtual_section_list_notify_section_header_changed(EGUI_VIEW_OF(&section_list_view), section_index);
    egui_view_virtual_section_list_scroll_to_item_by_stable_id(EGUI_VIEW_OF(&section_list_view), stable_id, SECTION_DEMO_ENTRY_GAP / 2);
    snprintf(section_demo_ctx.last_action_text, sizeof(section_demo_ctx.last_action_text), "Add row #%05lu into %02lu.", (unsigned long)stable_id,
             (unsigned long)section_index);
    section_demo_refresh_status();
}

static void section_demo_action_del(void)
{
    uint32_t section_index;
    uint32_t item_index;
    section_demo_section_t *section;
    uint32_t stable_id;

    if (!section_demo_pick_target(&section_index, &item_index))
    {
        snprintf(section_demo_ctx.last_action_text, sizeof(section_demo_ctx.last_action_text), "Del ignored. No rows.");
        section_demo_refresh_status();
        return;
    }

    section = &section_demo_ctx.sections[section_index];
    stable_id = section->items[item_index].stable_id;
    egui_view_virtual_section_list_remove_entry_state_by_stable_id(EGUI_VIEW_OF(&section_list_view), stable_id);

    if ((item_index + 1U) < section->item_count)
    {
        memmove(&section->items[item_index], &section->items[item_index + 1], (section->item_count - item_index - 1U) * sizeof(section->items[0]));
    }

    section->item_count--;
    section_demo_ctx.action_count++;
    if (stable_id == section_demo_ctx.selected_item_id)
    {
        section_demo_ctx.selected_item_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    egui_view_virtual_section_list_notify_item_removed(EGUI_VIEW_OF(&section_list_view), section_index, item_index, 1);
    egui_view_virtual_section_list_notify_section_header_changed(EGUI_VIEW_OF(&section_list_view), section_index);
    snprintf(section_demo_ctx.last_action_text, sizeof(section_demo_ctx.last_action_text), "Del row #%05lu from %02lu.", (unsigned long)stable_id,
             (unsigned long)section_index);
    section_demo_refresh_status();
}

static void section_demo_action_patch(void)
{
    uint32_t section_index;
    uint32_t item_index;
    section_demo_section_t *section;
    section_demo_item_t *item;
    int32_t old_height;
    int32_t new_height;
    uint8_t selected;

    if (!section_demo_pick_target(&section_index, &item_index))
    {
        snprintf(section_demo_ctx.last_action_text, sizeof(section_demo_ctx.last_action_text), "Patch ignored. No rows.");
        section_demo_refresh_status();
        return;
    }

    section = &section_demo_ctx.sections[section_index];
    item = &section->items[item_index];
    selected = section_demo_is_selected_item(item->stable_id);
    old_height = section_demo_measure_item_height_with_state(item, selected);

    item->revision++;
    item->variant = (uint8_t)((item->variant + 1U) % SECTION_DEMO_ITEM_VARIANT_COUNT);
    item->state = (uint8_t)((item->state + 1U) % SECTION_DEMO_ITEM_STATE_COUNT);
    if (item->state == SECTION_DEMO_ITEM_STATE_DONE)
    {
        item->progress = 100U;
    }
    else
    {
        item->progress = (uint8_t)(20U + ((item->progress + 19U) % 71U));
    }
    if (item->variant == SECTION_DEMO_ITEM_VARIANT_ALERT && item->state == SECTION_DEMO_ITEM_STATE_IDLE)
    {
        item->state = SECTION_DEMO_ITEM_STATE_WARN;
    }
    section->revision = (uint8_t)((section->revision % 9U) + 1U);
    new_height = section_demo_measure_item_height_with_state(item, selected);
    section_demo_ctx.action_count++;

    egui_view_virtual_section_list_notify_section_header_changed(EGUI_VIEW_OF(&section_list_view), section_index);
    if (new_height != old_height)
    {
        egui_view_virtual_section_list_notify_item_resized(EGUI_VIEW_OF(&section_list_view), section_index, item_index);
    }
    else
    {
        egui_view_virtual_section_list_notify_item_changed(EGUI_VIEW_OF(&section_list_view), section_index, item_index);
    }

    if (selected)
    {
        egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(EGUI_VIEW_OF(&section_list_view), item->stable_id, SECTION_DEMO_ENTRY_GAP / 2);
    }

    snprintf(section_demo_ctx.last_action_text, sizeof(section_demo_ctx.last_action_text), "Patch row #%05lu.", (unsigned long)item->stable_id);
    section_demo_refresh_status();
}

static void section_demo_action_jump(void)
{
    uint32_t section_index;
    uint32_t item_index;
    section_demo_section_t *section;
    uint32_t stable_id;
    uint32_t previous_selected = section_demo_ctx.selected_item_id;

    if (!section_demo_pick_target(&section_index, &item_index))
    {
        snprintf(section_demo_ctx.last_action_text, sizeof(section_demo_ctx.last_action_text), "Jump ignored. No rows.");
        section_demo_refresh_status();
        return;
    }

    section = &section_demo_ctx.sections[section_index];
    if (section->collapsed)
    {
        section->collapsed = 0;
        egui_view_virtual_section_list_notify_data_changed(EGUI_VIEW_OF(&section_list_view));
    }

    stable_id = section->items[item_index].stable_id;
    section_demo_ctx.selected_item_id = stable_id;
    section_demo_ctx.action_count++;

    if (previous_selected != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && previous_selected != stable_id)
    {
        egui_view_virtual_section_list_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&section_list_view), previous_selected);
    }

    egui_view_virtual_section_list_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&section_list_view), stable_id);
    egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(EGUI_VIEW_OF(&section_list_view), stable_id, SECTION_DEMO_ENTRY_GAP / 2);
    snprintf(section_demo_ctx.last_action_text, sizeof(section_demo_ctx.last_action_text), "Jump to %02lu-%02lu.", (unsigned long)section_index,
             (unsigned long)item_index);
    section_demo_refresh_status();
}

static void section_demo_apply_action(uint8_t action)
{
    switch (action)
    {
    case SECTION_DEMO_ACTION_ADD:
        section_demo_action_add();
        break;
    case SECTION_DEMO_ACTION_DEL:
        section_demo_action_del();
        break;
    case SECTION_DEMO_ACTION_PATCH:
        section_demo_action_patch();
        break;
    case SECTION_DEMO_ACTION_JUMP:
        section_demo_action_jump();
        break;
    default:
        break;
    }
}

static int section_demo_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < SECTION_DEMO_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void section_demo_action_click_cb(egui_view_t *self)
{
    int action = section_demo_find_action_button_index(self);

    if (action >= 0)
    {
        section_demo_apply_action((uint8_t)action);
    }
}

static void section_demo_init_action_button(egui_view_button_t *button, egui_dim_t x, const char *text)
{
    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_position(EGUI_VIEW_OF(button), x, 6);
    egui_view_set_size(EGUI_VIEW_OF(button), SECTION_DEMO_BUTTON_W, SECTION_DEMO_BUTTON_H);
    egui_view_label_set_text(EGUI_VIEW_OF(button), text);
    egui_view_label_set_font(EGUI_VIEW_OF(button), SECTION_DEMO_FONT_CAP);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x2B3F52), EGUI_ALPHA_100);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), section_demo_action_click_cb);
}

void test_init_ui(void)
{
    uint8_t i;
    egui_dim_t button_x = 10;

    section_demo_reset_model();

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&section_demo_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&header_card), &section_demo_header_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_card), 1, EGUI_COLOR_HEX(0xD7E1EA));
    egui_view_set_shadow(EGUI_VIEW_OF(&header_card), &section_demo_card_shadow);

    egui_view_label_init(EGUI_VIEW_OF(&header_title));
    egui_view_label_init(EGUI_VIEW_OF(&header_detail));
    egui_view_label_init(EGUI_VIEW_OF(&header_hint));
    egui_view_set_position(EGUI_VIEW_OF(&header_title), 12, 10);
    egui_view_set_size(EGUI_VIEW_OF(&header_title), SECTION_DEMO_HEADER_W - 24, 16);
    egui_view_set_position(EGUI_VIEW_OF(&header_detail), 12, 28);
    egui_view_set_size(EGUI_VIEW_OF(&header_detail), SECTION_DEMO_HEADER_W - 24, 14);
    egui_view_set_position(EGUI_VIEW_OF(&header_hint), 12, 44);
    egui_view_set_size(EGUI_VIEW_OF(&header_hint), SECTION_DEMO_HEADER_W - 24, 14);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_title), SECTION_DEMO_FONT_HEADER);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_detail), SECTION_DEMO_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_hint), SECTION_DEMO_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_detail), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_hint), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_title), EGUI_COLOR_HEX(0x203243), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_detail), EGUI_COLOR_HEX(0x5A6D7E), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_hint), EGUI_COLOR_HEX(0x6B7C8A), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_title));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_detail));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_hint));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &section_demo_toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD7E1EA));

    for (i = 0; i < SECTION_DEMO_ACTION_COUNT; i++)
    {
        section_demo_init_action_button(&action_buttons[i], button_x, section_demo_action_names[i]);
        egui_view_set_background(EGUI_VIEW_OF(&action_buttons[i]), i == SECTION_DEMO_ACTION_ADD     ? EGUI_BG_OF(&section_demo_action_add_bg)
                                                                   : i == SECTION_DEMO_ACTION_DEL   ? EGUI_BG_OF(&section_demo_action_del_bg)
                                                                   : i == SECTION_DEMO_ACTION_PATCH ? EGUI_BG_OF(&section_demo_action_patch_bg)
                                                                                                    : EGUI_BG_OF(&section_demo_action_jump_bg));
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
        button_x += SECTION_DEMO_BUTTON_W + SECTION_DEMO_BUTTON_GAP;
    }

    {
        const egui_view_virtual_section_list_setup_t section_view_setup = {
                .params = &section_demo_view_params,
                .data_source = &section_demo_data_source,
                .data_source_context = &section_demo_ctx,
                .state_cache_max_entries = SECTION_DEMO_STATE_CACHE_COUNT,
                .state_cache_max_bytes = SECTION_DEMO_STATE_CACHE_COUNT * (uint32_t)sizeof(section_demo_item_state_t),
        };

        egui_view_virtual_section_list_init_with_setup(EGUI_VIEW_OF(&section_list_view), &section_view_setup);
    }
    egui_view_set_background(EGUI_VIEW_OF(&section_list_view), EGUI_BG_OF(&section_demo_list_bg));
    egui_view_set_shadow(EGUI_VIEW_OF(&section_list_view), &section_demo_card_shadow);

    section_demo_refresh_status();

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&section_list_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&header_card));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}

#if EGUI_CONFIG_RECORDING_TEST
typedef struct section_demo_visible_search_context
{
    uint32_t min_section_index;
    uint8_t want_header;
} section_demo_visible_search_context_t;

static uint8_t section_demo_match_visible_entry(egui_view_t *self, const egui_view_virtual_section_list_slot_t *slot,
                                                const egui_view_virtual_section_list_entry_t *entry, egui_view_t *entry_view, void *context)
{
    section_demo_visible_search_context_t *ctx = (section_demo_visible_search_context_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(entry_view);

    if (!egui_view_virtual_viewport_is_slot_center_visible(EGUI_VIEW_OF(&section_list_view), slot) || entry == NULL)
    {
        return 0;
    }
    if (ctx->want_header)
    {
        if (!entry->is_section_header || entry->section_index < ctx->min_section_index)
        {
            return 0;
        }
    }
    else if (entry->is_section_header)
    {
        return 0;
    }

    return 1;
}

static egui_view_t *section_demo_find_first_visible_header_after(uint32_t min_section_index)
{
    section_demo_visible_search_context_t ctx = {
            .min_section_index = min_section_index,
            .want_header = 1,
    };

    return egui_view_virtual_section_list_find_first_visible_entry_view(EGUI_VIEW_OF(&section_list_view), section_demo_match_visible_entry, &ctx, NULL);
}

static egui_view_t *section_demo_find_first_visible_item_view(void)
{
    section_demo_visible_search_context_t ctx = {
            .min_section_index = 0,
            .want_header = 0,
    };

    return egui_view_virtual_section_list_find_first_visible_entry_view(EGUI_VIEW_OF(&section_list_view), section_demo_match_visible_entry, &ctx, NULL);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    egui_view_t *view;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        view = section_demo_find_first_visible_item_view();
        if (view == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 260);
        return true;
    case 1:
        view = section_demo_find_first_visible_header_after(1);
        if (view == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 260);
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[SECTION_DEMO_ACTION_PATCH]), 220);
        return true;
    case 3:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 4 / 5;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 3;
        p_action->steps = 6;
        p_action->interval_ms = 220;
        return true;
    case 4:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[SECTION_DEMO_ACTION_ADD]), 220);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[SECTION_DEMO_ACTION_JUMP]), 220);
        return true;
    case 6:
        view = section_demo_find_first_visible_header_after(2);
        if (view == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 240);
        return true;
    case 7:
        view = section_demo_find_first_visible_item_view();
        if (view == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 220);
        return true;
    case 8:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[SECTION_DEMO_ACTION_DEL]), 220);
        return true;
    case 9:
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
