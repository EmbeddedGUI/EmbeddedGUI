#include "egui.h"
#include <stdio.h>
#include <string.h>
#include "uicode.h"

#define PAGE_MAX_SECTIONS      360U
#define PAGE_INITIAL_SECTIONS  280U
#define PAGE_INVALID_INDEX     0xFFFFFFFFUL
#define PAGE_STATUS_TEXT_LEN   72
#define PAGE_TITLE_TEXT_LEN    48
#define PAGE_BODY_TEXT_LEN     64
#define PAGE_META_TEXT_LEN     48
#define PAGE_BADGE_TEXT_LEN    16
#define PAGE_EYEBROW_TEXT_LEN  24
#define PAGE_STATE_CACHE_COUNT 64U

#define PAGE_MARGIN_X          8
#define PAGE_TOP_Y             8
#define PAGE_HEADER_W          (EGUI_CONFIG_SCEEN_WIDTH - PAGE_MARGIN_X * 2)
#define PAGE_HEADER_H          68
#define PAGE_TOOLBAR_Y         (PAGE_TOP_Y + PAGE_HEADER_H + 6)
#define PAGE_TOOLBAR_H         32
#define PAGE_VIEW_Y            (PAGE_TOOLBAR_Y + PAGE_TOOLBAR_H + 6)
#define PAGE_VIEW_W            PAGE_HEADER_W
#define PAGE_VIEW_H            (EGUI_CONFIG_SCEEN_HEIGHT - PAGE_VIEW_Y - 8)
#define PAGE_BUTTON_GAP        4
#define PAGE_BUTTON_W          ((PAGE_HEADER_W - 20 - PAGE_BUTTON_GAP * 3) / 4)
#define PAGE_BUTTON_H          20
#define PAGE_CARD_INSET_X      8
#define PAGE_CARD_INSET_Y      4
#define PAGE_BADGE_H           18
#define PAGE_EYEBROW_H         10
#define PAGE_TITLE_H           14
#define PAGE_TEXT_H            12
#define PAGE_PROGRESS_H        4

#define PAGE_FONT_HEADER ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define PAGE_FONT_TITLE  ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define PAGE_FONT_BODY   ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define PAGE_FONT_CAP    ((const egui_font_t *)&egui_res_font_montserrat_8_4)

enum
{
    PAGE_ACTION_ADD = 0,
    PAGE_ACTION_DEL,
    PAGE_ACTION_PATCH,
    PAGE_ACTION_JUMP,
    PAGE_ACTION_COUNT,
};

enum
{
    PAGE_SECTION_VARIANT_HERO = 0,
    PAGE_SECTION_VARIANT_METRIC,
    PAGE_SECTION_VARIANT_ALERT,
    PAGE_SECTION_VARIANT_FORM,
    PAGE_SECTION_VARIANT_COUNT,
};

enum
{
    PAGE_SECTION_STATE_IDLE = 0,
    PAGE_SECTION_STATE_SYNC,
    PAGE_SECTION_STATE_WARNING,
    PAGE_SECTION_STATE_DONE,
    PAGE_SECTION_STATE_COUNT,
};

typedef struct page_demo_section page_demo_section_t;
typedef struct page_demo_section_view page_demo_section_view_t;
typedef struct page_demo_section_state page_demo_section_state_t;
typedef struct page_demo_context page_demo_context_t;

struct page_demo_section
{
    uint32_t stable_id;
    uint16_t revision;
    uint8_t variant;
    uint8_t state;
    uint8_t progress;
    uint8_t reserved;
};

struct page_demo_section_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t eyebrow;
    egui_view_label_t title;
    egui_view_label_t body;
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

struct page_demo_section_state
{
    uint16_t pulse_elapsed_ms;
    uint8_t pulse_running;
    uint8_t pulse_alpha;
    uint8_t pulse_cycle_flip;
    uint8_t pulse_repeated;
};

struct page_demo_context
{
    uint32_t item_count;
    uint32_t next_stable_id;
    uint32_t selected_id;
    uint32_t last_clicked_index;
    uint32_t click_count;
    uint32_t action_count;
    uint32_t mutation_cursor;
    uint8_t created_count;
    page_demo_section_t sections[PAGE_MAX_SECTIONS];
    char title_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][PAGE_TITLE_TEXT_LEN];
    char body_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][PAGE_BODY_TEXT_LEN];
    char meta_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][PAGE_META_TEXT_LEN];
    char badge_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][PAGE_BADGE_TEXT_LEN];
    char eyebrow_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][PAGE_EYEBROW_TEXT_LEN];
    char header_title_text[PAGE_STATUS_TEXT_LEN];
    char header_detail_text[PAGE_STATUS_TEXT_LEN];
    char header_hint_text[PAGE_STATUS_TEXT_LEN];
    char last_action_text[PAGE_STATUS_TEXT_LEN];
    page_demo_section_view_t section_views[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
};

static const char *page_demo_action_names[PAGE_ACTION_COUNT] = {"Add", "Del", "Patch", "Jump"};
static const char *page_demo_variant_names[PAGE_SECTION_VARIANT_COUNT] = {"Overview", "Metric", "Alert", "Checklist"};
static const char *page_demo_state_names[PAGE_SECTION_STATE_COUNT] = {"Idle", "Sync", "Warn", "Done"};
static const char *page_demo_variant_notes[PAGE_SECTION_VARIANT_COUNT] = {
        "Hero summary.",
        "Compact stats.",
        "Alert with pulse.",
        "Form-like block.",
};

static egui_view_t background_view;
static egui_view_card_t header_card;
static egui_view_card_t toolbar_card;
static egui_view_label_t header_title;
static egui_view_label_t header_detail;
static egui_view_label_t header_hint;
static egui_view_button_t action_buttons[PAGE_ACTION_COUNT];
static egui_view_virtual_page_t page_view;
static page_demo_context_t page_context;

EGUI_VIEW_CARD_PARAMS_INIT(header_card_params, PAGE_MARGIN_X, PAGE_TOP_Y, PAGE_HEADER_W, PAGE_HEADER_H, 14);
EGUI_VIEW_CARD_PARAMS_INIT(toolbar_card_params, PAGE_MARGIN_X, PAGE_TOOLBAR_Y, PAGE_HEADER_W, PAGE_TOOLBAR_H, 12);
static const egui_view_virtual_page_params_t page_view_params = {
        .region = {{PAGE_MARGIN_X, PAGE_VIEW_Y}, {PAGE_VIEW_W, PAGE_VIEW_H}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_section_height = 90,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF4F8), EGUI_COLOR_HEX(0xD9E5EE),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(screen_bg_params, &screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(screen_bg, &screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(page_bg_param, EGUI_COLOR_HEX(0xF9FBFD), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(page_bg_params, &page_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(page_bg, &page_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(action_add_param, EGUI_COLOR_HEX(0xDFF3E9), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(action_add_params, &action_add_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(action_add_bg, &action_add_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(action_del_param, EGUI_COLOR_HEX(0xFFF1E1), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(action_del_params, &action_del_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(action_del_bg, &action_del_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(action_patch_param, EGUI_COLOR_HEX(0xE3F6F7), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(action_patch_params, &action_patch_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(action_patch_bg, &action_patch_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(action_jump_param, EGUI_COLOR_HEX(0xE5EEFF), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(action_jump_params, &action_jump_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(action_jump_bg, &action_jump_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_selected_param, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_selected_params, &badge_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_selected_bg, &badge_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_idle_param, EGUI_COLOR_HEX(0x8D99A6), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_idle_params, &badge_idle_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_idle_bg, &badge_idle_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_sync_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_sync_params, &badge_sync_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_sync_bg, &badge_sync_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_warn_param, EGUI_COLOR_HEX(0xE7B14A), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_warn_params, &badge_warn_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_warn_bg, &badge_warn_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_done_param, EGUI_COLOR_HEX(0x56789A), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_done_params, &badge_done_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_done_bg, &badge_done_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(pulse_selected_param, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(pulse_selected_params, &pulse_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(pulse_selected_bg, &pulse_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(pulse_sync_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(pulse_sync_params, &pulse_sync_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(pulse_sync_bg, &pulse_sync_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(pulse_warn_param, EGUI_COLOR_HEX(0xD08A2E), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(pulse_warn_params, &pulse_warn_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(pulse_warn_bg, &pulse_warn_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(pulse_done_param, EGUI_COLOR_HEX(0x5C9EE6), EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(pulse_done_params, &pulse_done_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(pulse_done_bg, &pulse_done_params);

EGUI_SHADOW_PARAM_INIT_ROUND(page_card_shadow, 10, 0, 3, EGUI_COLOR_BLACK, EGUI_ALPHA_20, 12);
EGUI_ANIMATION_ALPHA_PARAMS_INIT(pulse_anim_param, EGUI_ALPHA_100, EGUI_ALPHA_20);

static void page_demo_refresh_header(void);
static void page_demo_apply_action(uint8_t action);

static void page_demo_fill_section(page_demo_section_t *section, uint32_t stable_id, uint32_t seed)
{
    uint8_t progress;

    memset(section, 0, sizeof(*section));
    section->stable_id = stable_id;
    section->revision = (uint16_t)(1U + (seed % 7U));
    section->variant = (uint8_t)(seed % PAGE_SECTION_VARIANT_COUNT);
    section->state = (uint8_t)((seed / 3U) % PAGE_SECTION_STATE_COUNT);
    progress = (uint8_t)(18U + ((seed * 13U) % 71U));

    if (section->variant == PAGE_SECTION_VARIANT_ALERT && section->state == PAGE_SECTION_STATE_IDLE)
    {
        section->state = PAGE_SECTION_STATE_WARNING;
    }
    if (section->state == PAGE_SECTION_STATE_DONE)
    {
        progress = 100U;
    }
    if (section->variant == PAGE_SECTION_VARIANT_HERO && progress < 50U)
    {
        progress = (uint8_t)(progress + 20U);
    }

    section->progress = progress;
}

static void page_demo_reset_model(void)
{
    uint32_t i;

    memset(&page_context, 0, sizeof(page_context));
    page_context.item_count = PAGE_INITIAL_SECTIONS;
    page_context.next_stable_id = 400001U;
    page_context.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    page_context.last_clicked_index = PAGE_INVALID_INDEX;

    for (i = 0; i < page_context.item_count; i++)
    {
        page_demo_fill_section(&page_context.sections[i], page_context.next_stable_id++, i + 1U);
    }

    snprintf(page_context.last_action_text, sizeof(page_context.last_action_text), "Tap to expand. Use buttons.");
}

static int32_t page_demo_find_index_by_stable_id(uint32_t stable_id)
{
    uint32_t i;

    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return -1;
    }

    for (i = 0; i < page_context.item_count; i++)
    {
        if (page_context.sections[i].stable_id == stable_id)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static int32_t page_demo_measure_section_height_with_state(const page_demo_section_t *section, uint8_t selected)
{
    int32_t base_height;

    if (section == NULL)
    {
        return 72;
    }

    switch (section->variant)
    {
    case PAGE_SECTION_VARIANT_METRIC:
        base_height = 72;
        break;
    case PAGE_SECTION_VARIANT_ALERT:
        base_height = 86;
        break;
    case PAGE_SECTION_VARIANT_FORM:
        base_height = 102;
        break;
    default:
        base_height = 94;
        break;
    }

    if (section->state == PAGE_SECTION_STATE_SYNC)
    {
        base_height += 6;
    }
    else if (section->state == PAGE_SECTION_STATE_WARNING)
    {
        base_height += 4;
    }

    if (selected)
    {
        base_height += 18;
    }

    return base_height;
}

static uint8_t page_demo_section_has_pulse(const page_demo_section_t *section, uint8_t selected)
{
    if (section == NULL)
    {
        return 0;
    }

    return (uint8_t)(selected || section->state == PAGE_SECTION_STATE_SYNC || section->state == PAGE_SECTION_STATE_WARNING);
}

static egui_dim_t page_demo_get_view_width(void)
{
    egui_dim_t width = EGUI_VIEW_OF(&page_view)->region.size.width;

    return width > 0 ? width : PAGE_VIEW_W;
}

static int page_demo_get_view_pool_index(page_demo_section_view_t *section_view)
{
    uint8_t i;

    for (i = 0; i < page_context.created_count; i++)
    {
        if (section_view == &page_context.section_views[i])
        {
            return (int)i;
        }
    }

    return -1;
}

static page_demo_section_view_t *page_demo_find_view_by_root(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < page_context.created_count; i++)
    {
        if (view == EGUI_VIEW_OF(&page_context.section_views[i].root))
        {
            return &page_context.section_views[i];
        }
    }

    return NULL;
}

static void page_demo_set_section_pulse(page_demo_section_view_t *section_view, const page_demo_section_t *section, uint8_t visible, uint8_t selected)
{
    egui_background_t *pulse_bg;

    if (!visible || section == NULL)
    {
        if (section_view->pulse_running)
        {
            egui_animation_stop(EGUI_ANIM_OF(&section_view->pulse_anim));
            section_view->pulse_running = 0;
        }
        egui_view_set_gone(EGUI_VIEW_OF(&section_view->pulse), 1);
        egui_view_set_alpha(EGUI_VIEW_OF(&section_view->pulse), EGUI_ALPHA_100);
        return;
    }

    if (selected)
    {
        pulse_bg = EGUI_BG_OF(&pulse_selected_bg);
    }
    else if (section->state == PAGE_SECTION_STATE_WARNING)
    {
        pulse_bg = EGUI_BG_OF(&pulse_warn_bg);
    }
    else if (section->state == PAGE_SECTION_STATE_DONE)
    {
        pulse_bg = EGUI_BG_OF(&pulse_done_bg);
    }
    else
    {
        pulse_bg = EGUI_BG_OF(&pulse_sync_bg);
    }

    egui_view_set_gone(EGUI_VIEW_OF(&section_view->pulse), 0);
    egui_view_set_background(EGUI_VIEW_OF(&section_view->pulse), pulse_bg);
    if (!section_view->pulse_running)
    {
        egui_animation_start(EGUI_ANIM_OF(&section_view->pulse_anim));
        section_view->pulse_running = 1;
    }
}

static void page_demo_capture_view_state(page_demo_section_view_t *section_view, page_demo_section_state_t *state)
{
    egui_animation_t *anim = EGUI_ANIM_OF(&section_view->pulse_anim);

    memset(state, 0, sizeof(*state));
    state->pulse_running = section_view->pulse_running ? 1U : 0U;
    state->pulse_alpha = EGUI_VIEW_OF(&section_view->pulse)->alpha;

    if (!section_view->pulse_running)
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

static void page_demo_restore_view_state(page_demo_section_view_t *section_view, const page_demo_section_state_t *state)
{
    egui_animation_t *anim;

    if (state == NULL || !state->pulse_running)
    {
        return;
    }

    anim = EGUI_ANIM_OF(&section_view->pulse_anim);
    if (!anim->is_running)
    {
        egui_animation_start(anim);
    }

    section_view->pulse_running = 1;
    egui_view_set_gone(EGUI_VIEW_OF(&section_view->pulse), 0);
    egui_view_set_alpha(EGUI_VIEW_OF(&section_view->pulse), state->pulse_alpha);
    anim->is_started = 1;
    anim->is_ended = 0;
    anim->is_cycle_flip = state->pulse_cycle_flip ? 1U : 0U;
    anim->repeated = (int8_t)state->pulse_repeated;
    anim->start_time = egui_api_timer_get_current() - state->pulse_elapsed_ms;
}

static void page_demo_refresh_header(void)
{
    int32_t selected_index = page_demo_find_index_by_stable_id(page_context.selected_id);

    snprintf(page_context.header_title_text, sizeof(page_context.header_title_text), "Virtual Page Demo");
    if (selected_index >= 0)
    {
        const page_demo_section_t *section = &page_context.sections[selected_index];

        snprintf(page_context.header_detail_text, sizeof(page_context.header_detail_text), "Sections %lu | sel %05lu | %u%%",
                 (unsigned long)page_context.item_count, (unsigned long)section->stable_id, (unsigned)section->progress);
    }
    else
    {
        snprintf(page_context.header_detail_text, sizeof(page_context.header_detail_text), "Sections %lu | sel none",
                 (unsigned long)page_context.item_count);
    }
    snprintf(page_context.header_hint_text, sizeof(page_context.header_hint_text), "%s", page_context.last_action_text);

    if (EGUI_VIEW_OF(&header_title)->api == NULL)
    {
        return;
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&header_title), page_context.header_title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_detail), page_context.header_detail_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_hint), page_context.header_hint_text);
}

static void page_demo_update_selection(uint32_t stable_id, uint8_t allow_toggle, uint8_t ensure_visible)
{
    uint32_t previous_id = page_context.selected_id;

    if (allow_toggle && previous_id == stable_id)
    {
        page_context.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }
    else
    {
        page_context.selected_id = stable_id;
    }

    if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && previous_id != page_context.selected_id)
    {
        egui_view_virtual_page_notify_section_resized_by_stable_id(EGUI_VIEW_OF(&page_view), previous_id);
    }
    if (page_context.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_page_notify_section_resized_by_stable_id(EGUI_VIEW_OF(&page_view), page_context.selected_id);
        if (ensure_visible)
        {
            egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id, PAGE_CARD_INSET_Y);
        }
    }
}

static uint32_t page_demo_pick_action_index(void)
{
    int32_t selected_index = page_demo_find_index_by_stable_id(page_context.selected_id);

    if (selected_index >= 0)
    {
        return (uint32_t)selected_index;
    }

    if (page_context.item_count == 0)
    {
        return 0;
    }

    return page_context.mutation_cursor % page_context.item_count;
}

static void page_demo_bind_section_card(page_demo_section_view_t *section_view, int pool_index, uint32_t index, const page_demo_section_t *section, uint8_t selected)
{
    egui_dim_t card_x = PAGE_CARD_INSET_X;
    egui_dim_t card_y = PAGE_CARD_INSET_Y;
    egui_dim_t card_w = page_demo_get_view_width() - PAGE_CARD_INSET_X * 2;
    egui_dim_t card_h = (egui_dim_t)(page_demo_measure_section_height_with_state(section, selected) - PAGE_CARD_INSET_Y * 2);
    egui_dim_t badge_w = selected ? 54 : 44;
    egui_dim_t badge_x;
    egui_dim_t title_w;
    egui_background_t *badge_bg;
    egui_color_t badge_text_color;
    egui_color_t card_color;
    egui_color_t border_color;
    egui_color_t title_color;
    egui_color_t body_color;
    egui_color_t meta_color;
    uint8_t show_body;
    uint8_t show_progress;

    if (card_h < 56)
    {
        card_h = 56;
    }
    if (card_w < 140)
    {
        card_w = 140;
    }

    show_body = (uint8_t)(selected || section->variant != PAGE_SECTION_VARIANT_METRIC);
    show_progress = (uint8_t)(selected || section->variant != PAGE_SECTION_VARIANT_ALERT || section->state != PAGE_SECTION_STATE_IDLE);

    snprintf(page_context.eyebrow_texts[pool_index], sizeof(page_context.eyebrow_texts[pool_index]), "%s / %s",
             page_demo_variant_names[section->variant], page_demo_state_names[section->state]);
    snprintf(page_context.title_texts[pool_index], sizeof(page_context.title_texts[pool_index]), "%s #%03lu", page_demo_variant_names[section->variant],
             (unsigned long)index);
    snprintf(page_context.body_texts[pool_index], sizeof(page_context.body_texts[pool_index]), "%s",
             selected ? "Selected keeps pulse." : page_demo_variant_notes[section->variant]);
    snprintf(page_context.meta_texts[pool_index], sizeof(page_context.meta_texts[pool_index]), "#%05lu  %u%%  r%u", (unsigned long)section->stable_id,
             (unsigned)section->progress, (unsigned)section->revision);
    snprintf(page_context.badge_texts[pool_index], sizeof(page_context.badge_texts[pool_index]), "%s",
             selected ? "OPEN" : (section->state == PAGE_SECTION_STATE_SYNC ? "SYNC"
                                                                           : (section->state == PAGE_SECTION_STATE_WARNING ? "WARN"
                                                                                                                          : (section->state == PAGE_SECTION_STATE_DONE ? "DONE"
                                                                                                                                                                      : "IDLE"))));

    if (selected)
    {
        card_color = EGUI_COLOR_HEX(0x2F5E91);
        border_color = EGUI_COLOR_WHITE;
        title_color = EGUI_COLOR_WHITE;
        body_color = EGUI_COLOR_WHITE;
        meta_color = EGUI_COLOR_WHITE;
        badge_bg = EGUI_BG_OF(&badge_selected_bg);
        badge_text_color = EGUI_COLOR_HEX(0x2F5E91);
        section_view->progress.progress_color = EGUI_COLOR_WHITE;
        section_view->progress.bk_color = EGUI_COLOR_HEX(0x7698BF);
    }
    else
    {
        switch (section->variant)
        {
        case PAGE_SECTION_VARIANT_METRIC:
            card_color = EGUI_COLOR_WHITE;
            border_color = EGUI_COLOR_HEX(0xD8E2EA);
            break;
        case PAGE_SECTION_VARIANT_ALERT:
            card_color = EGUI_COLOR_HEX(0xFFF6E8);
            border_color = EGUI_COLOR_HEX(0xE0B36B);
            break;
        case PAGE_SECTION_VARIANT_FORM:
            card_color = EGUI_COLOR_HEX(0xEEF5F0);
            border_color = EGUI_COLOR_HEX(0xC7D7C8);
            break;
        default:
            card_color = EGUI_COLOR_HEX(0xF4FAFF);
            border_color = EGUI_COLOR_HEX(0xCBDCE9);
            break;
        }
        title_color = EGUI_COLOR_HEX(0x233443);
        body_color = EGUI_COLOR_HEX(0x55697A);
        meta_color = section->state == PAGE_SECTION_STATE_WARNING ? EGUI_COLOR_HEX(0x8E6726)
                                                                  : (section->state == PAGE_SECTION_STATE_SYNC ? EGUI_COLOR_HEX(0x2A7C5A)
                                                                                                                 : EGUI_COLOR_HEX(0x677989));
        badge_bg = section->state == PAGE_SECTION_STATE_SYNC   ? EGUI_BG_OF(&badge_sync_bg)
                   : section->state == PAGE_SECTION_STATE_WARNING ? EGUI_BG_OF(&badge_warn_bg)
                   : section->state == PAGE_SECTION_STATE_DONE    ? EGUI_BG_OF(&badge_done_bg)
                                                                 : EGUI_BG_OF(&badge_idle_bg);
        badge_text_color = section->state == PAGE_SECTION_STATE_WARNING ? EGUI_COLOR_BLACK : EGUI_COLOR_WHITE;
        section_view->progress.progress_color = section->state == PAGE_SECTION_STATE_WARNING ? EGUI_COLOR_HEX(0xD08A2E)
                                              : section->state == PAGE_SECTION_STATE_SYNC    ? EGUI_COLOR_HEX(0x2E9A6F)
                                                                                            : EGUI_COLOR_HEX(0x4B79B2);
        section_view->progress.bk_color = EGUI_COLOR_HEX(0xDCE6F0);
    }

    section_view->progress.control_color = section_view->progress.progress_color;
    badge_x = card_w - badge_w - 10;
    title_w = badge_x - 16;
    if (title_w < 76)
    {
        title_w = 76;
    }

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&section_view->card), card_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&section_view->card), selected ? 2 : 1, border_color);
    egui_view_set_background(EGUI_VIEW_OF(&section_view->badge), badge_bg);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&section_view->eyebrow), meta_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&section_view->title), title_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&section_view->body), body_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&section_view->meta), meta_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&section_view->badge), badge_text_color, EGUI_ALPHA_100);

    egui_view_label_set_text(EGUI_VIEW_OF(&section_view->eyebrow), page_context.eyebrow_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&section_view->title), page_context.title_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&section_view->body), page_context.body_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&section_view->meta), page_context.meta_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&section_view->badge), page_context.badge_texts[pool_index]);

    egui_view_set_position(EGUI_VIEW_OF(&section_view->card), card_x, card_y);
    egui_view_set_size(EGUI_VIEW_OF(&section_view->card), card_w, card_h);
    egui_view_set_position(EGUI_VIEW_OF(&section_view->badge), badge_x, 10);
    egui_view_set_size(EGUI_VIEW_OF(&section_view->badge), badge_w, PAGE_BADGE_H);
    egui_view_set_position(EGUI_VIEW_OF(&section_view->pulse), badge_x - 15, 15);
    egui_view_set_size(EGUI_VIEW_OF(&section_view->pulse), 8, 8);
    egui_view_set_position(EGUI_VIEW_OF(&section_view->eyebrow), 12, 8);
    egui_view_set_size(EGUI_VIEW_OF(&section_view->eyebrow), title_w, PAGE_EYEBROW_H);
    egui_view_set_position(EGUI_VIEW_OF(&section_view->title), 12, 22);
    egui_view_set_size(EGUI_VIEW_OF(&section_view->title), title_w, PAGE_TITLE_H);
    egui_view_set_position(EGUI_VIEW_OF(&section_view->body), 12, 38);
    egui_view_set_size(EGUI_VIEW_OF(&section_view->body), card_w - 24, PAGE_TEXT_H);
    egui_view_set_position(EGUI_VIEW_OF(&section_view->meta), 12, show_progress ? (card_h - 24) : (card_h - 15));
    egui_view_set_size(EGUI_VIEW_OF(&section_view->meta), card_w - 24, PAGE_TEXT_H);
    egui_view_set_position(EGUI_VIEW_OF(&section_view->progress), 12, card_h - 10);
    egui_view_set_size(EGUI_VIEW_OF(&section_view->progress), card_w - 24, PAGE_PROGRESS_H);

    egui_view_set_gone(EGUI_VIEW_OF(&section_view->body), show_body ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&section_view->progress), show_progress ? 0 : 1);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&section_view->progress), section->progress);
    page_demo_set_section_pulse(section_view, section, page_demo_section_has_pulse(section, selected), selected);
}

static uint32_t page_demo_get_count(void *data_source_context)
{
    return ((page_demo_context_t *)data_source_context)->item_count;
}

static uint32_t page_demo_get_stable_id(void *data_source_context, uint32_t index)
{
    page_demo_context_t *ctx = (page_demo_context_t *)data_source_context;

    if (index >= ctx->item_count)
    {
        return EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    return ctx->sections[index].stable_id;
}

static int32_t page_demo_find_index_adapter(void *data_source_context, uint32_t stable_id)
{
    EGUI_UNUSED(data_source_context);
    return page_demo_find_index_by_stable_id(stable_id);
}

static int32_t page_demo_measure_section_height(void *data_source_context, uint32_t index, int32_t width_hint)
{
    page_demo_context_t *ctx = (page_demo_context_t *)data_source_context;

    EGUI_UNUSED(width_hint);
    if (index >= ctx->item_count)
    {
        return 72;
    }

    return page_demo_measure_section_height_with_state(&ctx->sections[index], ctx->sections[index].stable_id == ctx->selected_id);
}

static void page_demo_section_click_cb(egui_view_t *self)
{
    egui_view_virtual_page_entry_t entry;

    if (!egui_view_virtual_page_resolve_section_by_view(EGUI_VIEW_OF(&page_view), self, &entry) || entry.index == PAGE_INVALID_INDEX ||
        entry.stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return;
    }

    page_context.last_clicked_index = entry.index;
    page_context.click_count++;
    page_demo_update_selection(entry.stable_id, 1, 1);
    snprintf(page_context.last_action_text, sizeof(page_context.last_action_text), "Tap section #%05lu.", (unsigned long)entry.stable_id);
    page_demo_refresh_header();
}

static egui_view_t *page_demo_create_section_view(void *data_source_context, uint16_t section_type)
{
    page_demo_context_t *ctx = (page_demo_context_t *)data_source_context;
    page_demo_section_view_t *section_view;

    EGUI_UNUSED(section_type);

    if (ctx->created_count >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
    {
        return NULL;
    }

    section_view = &ctx->section_views[ctx->created_count];
    memset(section_view, 0, sizeof(*section_view));
    section_view->bound_index = PAGE_INVALID_INDEX;
    section_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;

    egui_view_group_init(EGUI_VIEW_OF(&section_view->root));
    egui_view_card_init(EGUI_VIEW_OF(&section_view->card));
    egui_view_label_init(EGUI_VIEW_OF(&section_view->eyebrow));
    egui_view_label_init(EGUI_VIEW_OF(&section_view->title));
    egui_view_label_init(EGUI_VIEW_OF(&section_view->body));
    egui_view_label_init(EGUI_VIEW_OF(&section_view->meta));
    egui_view_label_init(EGUI_VIEW_OF(&section_view->badge));
    egui_view_progress_bar_init(EGUI_VIEW_OF(&section_view->progress));
    egui_view_init(EGUI_VIEW_OF(&section_view->pulse));

    egui_view_label_set_font(EGUI_VIEW_OF(&section_view->eyebrow), PAGE_FONT_CAP);
    egui_view_label_set_font(EGUI_VIEW_OF(&section_view->title), PAGE_FONT_TITLE);
    egui_view_label_set_font(EGUI_VIEW_OF(&section_view->body), PAGE_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&section_view->meta), PAGE_FONT_CAP);
    egui_view_label_set_font(EGUI_VIEW_OF(&section_view->badge), PAGE_FONT_CAP);

    egui_view_label_set_align_type(EGUI_VIEW_OF(&section_view->eyebrow), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&section_view->title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&section_view->body), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&section_view->meta), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&section_view->badge), EGUI_ALIGN_CENTER);

    egui_view_group_add_child(EGUI_VIEW_OF(&section_view->root), EGUI_VIEW_OF(&section_view->card));
    egui_view_card_add_child(EGUI_VIEW_OF(&section_view->card), EGUI_VIEW_OF(&section_view->eyebrow));
    egui_view_card_add_child(EGUI_VIEW_OF(&section_view->card), EGUI_VIEW_OF(&section_view->title));
    egui_view_card_add_child(EGUI_VIEW_OF(&section_view->card), EGUI_VIEW_OF(&section_view->body));
    egui_view_card_add_child(EGUI_VIEW_OF(&section_view->card), EGUI_VIEW_OF(&section_view->meta));
    egui_view_card_add_child(EGUI_VIEW_OF(&section_view->card), EGUI_VIEW_OF(&section_view->badge));
    egui_view_card_add_child(EGUI_VIEW_OF(&section_view->card), EGUI_VIEW_OF(&section_view->progress));
    egui_view_card_add_child(EGUI_VIEW_OF(&section_view->card), EGUI_VIEW_OF(&section_view->pulse));
    egui_view_set_shadow(EGUI_VIEW_OF(&section_view->card), &page_card_shadow);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&section_view->root), page_demo_section_click_cb);

    egui_animation_alpha_init(EGUI_ANIM_OF(&section_view->pulse_anim));
    egui_animation_alpha_params_set(&section_view->pulse_anim, &pulse_anim_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&section_view->pulse_anim), 900);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&section_view->pulse_anim), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&section_view->pulse_anim), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_interpolator_linear_init((egui_interpolator_t *)&section_view->pulse_interp);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&section_view->pulse_anim), (egui_interpolator_t *)&section_view->pulse_interp);
    egui_animation_target_view_set(EGUI_ANIM_OF(&section_view->pulse_anim), EGUI_VIEW_OF(&section_view->pulse));
    egui_view_set_gone(EGUI_VIEW_OF(&section_view->pulse), 1);

    ctx->created_count++;
    return EGUI_VIEW_OF(&section_view->root);
}

static void page_demo_bind_section_view(void *data_source_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    page_demo_context_t *ctx = (page_demo_context_t *)data_source_context;
    page_demo_section_view_t *section_view = page_demo_find_view_by_root(view);
    int pool_index;

    if (section_view == NULL || index >= ctx->item_count)
    {
        return;
    }

    pool_index = page_demo_get_view_pool_index(section_view);
    if (pool_index < 0)
    {
        return;
    }

    section_view->bound_index = index;
    section_view->stable_id = stable_id;
    page_demo_bind_section_card(section_view, pool_index, index, &ctx->sections[index], stable_id == ctx->selected_id);
}

static void page_demo_unbind_section_view(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    page_demo_section_view_t *section_view = page_demo_find_view_by_root(view);

    EGUI_UNUSED(data_source_context);
    EGUI_UNUSED(stable_id);

    if (section_view == NULL)
    {
        return;
    }

    page_demo_set_section_pulse(section_view, NULL, 0, 0);
    section_view->bound_index = PAGE_INVALID_INDEX;
    section_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static void page_demo_save_section_state(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    page_demo_section_view_t *section_view = page_demo_find_view_by_root(view);
    page_demo_section_state_t state;

    EGUI_UNUSED(data_source_context);

    if (section_view == NULL)
    {
        return;
    }

    page_demo_capture_view_state(section_view, &state);
    if (!state.pulse_running)
    {
        egui_view_virtual_page_remove_section_state_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id);
        return;
    }

    (void)egui_view_virtual_page_write_section_state_for_view(view, stable_id, &state, sizeof(state));
}

static void page_demo_restore_section_state(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    page_demo_section_view_t *section_view = page_demo_find_view_by_root(view);
    page_demo_section_state_t state;

    EGUI_UNUSED(data_source_context);

    if (section_view == NULL)
    {
        return;
    }

    if (egui_view_virtual_page_read_section_state_for_view(view, stable_id, &state, sizeof(state)) != sizeof(state))
    {
        return;
    }

    page_demo_restore_view_state(section_view, &state);
}

static uint8_t page_demo_should_keep_alive(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    page_demo_context_t *ctx = (page_demo_context_t *)data_source_context;
    int32_t index = page_demo_find_index_by_stable_id(stable_id);

    EGUI_UNUSED(view);

    if (stable_id == ctx->selected_id)
    {
        return 1;
    }
    if (index < 0)
    {
        return 0;
    }

    return page_demo_section_has_pulse(&ctx->sections[index], 0);
}

static const egui_view_virtual_page_data_source_t page_demo_data_source = {
        .get_count = page_demo_get_count,
        .get_stable_id = page_demo_get_stable_id,
        .find_index_by_stable_id = page_demo_find_index_adapter,
        .get_section_type = NULL,
        .measure_section_height = page_demo_measure_section_height,
        .create_section_view = page_demo_create_section_view,
        .destroy_section_view = NULL,
        .bind_section_view = page_demo_bind_section_view,
        .unbind_section_view = page_demo_unbind_section_view,
        .should_keep_alive = page_demo_should_keep_alive,
        .save_section_state = page_demo_save_section_state,
        .restore_section_state = page_demo_restore_section_state,
        .default_section_type = 0,
};

static int page_demo_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < PAGE_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void page_demo_action_add(void)
{
    uint32_t index;
    uint32_t stable_id;

    if (page_context.item_count >= PAGE_MAX_SECTIONS)
    {
        snprintf(page_context.last_action_text, sizeof(page_context.last_action_text), "Add ignored. Page is full.");
        page_demo_refresh_header();
        return;
    }

    index = page_context.item_count == 0 ? 0U : page_demo_pick_action_index() + 1U;
    if (index > page_context.item_count)
    {
        index = page_context.item_count;
    }

    if (index < page_context.item_count)
    {
        memmove(&page_context.sections[index + 1], &page_context.sections[index], (page_context.item_count - index) * sizeof(page_context.sections[0]));
    }

    stable_id = page_context.next_stable_id++;
    page_demo_fill_section(&page_context.sections[index], stable_id, stable_id);
    page_context.sections[index].state = PAGE_SECTION_STATE_SYNC;
    page_context.sections[index].progress = 26U;
    page_context.item_count++;
    page_context.action_count++;
    page_context.mutation_cursor = index;

    egui_view_virtual_page_notify_section_inserted(EGUI_VIEW_OF(&page_view), index, 1);
    page_demo_update_selection(stable_id, 0, 1);
    snprintf(page_context.last_action_text, sizeof(page_context.last_action_text), "Inserted section #%05lu.", (unsigned long)stable_id);
    page_demo_refresh_header();
}

static void page_demo_action_del(void)
{
    uint32_t index;
    uint32_t stable_id;
    uint8_t removed_selected;

    if (page_context.item_count == 0)
    {
        snprintf(page_context.last_action_text, sizeof(page_context.last_action_text), "Del ignored. Page is empty.");
        page_demo_refresh_header();
        return;
    }

    index = page_demo_pick_action_index();
    stable_id = page_context.sections[index].stable_id;
    removed_selected = stable_id == page_context.selected_id ? 1U : 0U;

    egui_view_virtual_page_remove_section_state_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id);
    if ((index + 1U) < page_context.item_count)
    {
        memmove(&page_context.sections[index], &page_context.sections[index + 1], (page_context.item_count - index - 1U) * sizeof(page_context.sections[0]));
    }

    page_context.item_count--;
    page_context.action_count++;
    if (page_context.item_count == 0)
    {
        page_context.mutation_cursor = 0;
    }
    else if (page_context.mutation_cursor >= page_context.item_count)
    {
        page_context.mutation_cursor = 0;
    }
    if (removed_selected)
    {
        page_context.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    egui_view_virtual_page_notify_section_removed(EGUI_VIEW_OF(&page_view), index, 1);
    snprintf(page_context.last_action_text, sizeof(page_context.last_action_text), "Deleted section #%05lu.", (unsigned long)stable_id);
    page_demo_refresh_header();
}

static void page_demo_action_patch(void)
{
    uint32_t index;
    page_demo_section_t *section;
    int32_t old_height;
    int32_t new_height;
    uint8_t selected;

    if (page_context.item_count == 0)
    {
        snprintf(page_context.last_action_text, sizeof(page_context.last_action_text), "Patch ignored. Page is empty.");
        page_demo_refresh_header();
        return;
    }

    index = page_demo_pick_action_index();
    section = &page_context.sections[index];
    selected = section->stable_id == page_context.selected_id ? 1U : 0U;
    old_height = page_demo_measure_section_height_with_state(section, selected);

    section->revision++;
    section->variant = (uint8_t)((section->variant + 1U) % PAGE_SECTION_VARIANT_COUNT);
    section->state = (uint8_t)((section->state + 1U) % PAGE_SECTION_STATE_COUNT);
    if (section->state == PAGE_SECTION_STATE_DONE)
    {
        section->progress = 100U;
    }
    else
    {
        section->progress = (uint8_t)(20U + ((section->progress + 17U) % 71U));
    }
    if (section->variant == PAGE_SECTION_VARIANT_ALERT && section->state == PAGE_SECTION_STATE_IDLE)
    {
        section->state = PAGE_SECTION_STATE_WARNING;
    }

    new_height = page_demo_measure_section_height_with_state(section, selected);
    page_context.action_count++;
    page_context.mutation_cursor = (index + 17U) % page_context.item_count;

    if (new_height != old_height)
    {
        egui_view_virtual_page_notify_section_resized(EGUI_VIEW_OF(&page_view), index);
    }
    else
    {
        egui_view_virtual_page_notify_section_changed(EGUI_VIEW_OF(&page_view), index);
    }
    if (selected)
    {
        egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&page_view), section->stable_id, PAGE_CARD_INSET_Y);
    }

    snprintf(page_context.last_action_text, sizeof(page_context.last_action_text), "Patched section #%05lu.", (unsigned long)section->stable_id);
    page_demo_refresh_header();
}

static void page_demo_action_jump(void)
{
    uint32_t index;
    uint32_t stable_id;

    if (page_context.item_count == 0)
    {
        snprintf(page_context.last_action_text, sizeof(page_context.last_action_text), "Jump ignored. Page is empty.");
        page_demo_refresh_header();
        return;
    }

    page_context.mutation_cursor = (page_context.mutation_cursor + 37U) % page_context.item_count;
    index = page_context.mutation_cursor;
    stable_id = page_context.sections[index].stable_id;
    page_context.action_count++;

    page_demo_update_selection(stable_id, 0, 0);
    egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id, PAGE_CARD_INSET_Y);
    snprintf(page_context.last_action_text, sizeof(page_context.last_action_text), "Jump to section #%05lu.", (unsigned long)stable_id);
    page_demo_refresh_header();
}

static void page_demo_apply_action(uint8_t action)
{
    switch (action)
    {
    case PAGE_ACTION_ADD:
        page_demo_action_add();
        break;
    case PAGE_ACTION_DEL:
        page_demo_action_del();
        break;
    case PAGE_ACTION_PATCH:
        page_demo_action_patch();
        break;
    case PAGE_ACTION_JUMP:
        page_demo_action_jump();
        break;
    default:
        break;
    }
}

static void page_demo_action_click_cb(egui_view_t *self)
{
    int action = page_demo_find_action_button_index(self);

    if (action < 0)
    {
        return;
    }

    page_demo_apply_action((uint8_t)action);
}

static void page_demo_init_action_button(egui_view_button_t *button, egui_dim_t x, const char *text)
{
    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_position(EGUI_VIEW_OF(button), x, 6);
    egui_view_set_size(EGUI_VIEW_OF(button), PAGE_BUTTON_W, PAGE_BUTTON_H);
    egui_view_label_set_text(EGUI_VIEW_OF(button), text);
    egui_view_label_set_font(EGUI_VIEW_OF(button), PAGE_FONT_CAP);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x2B3F52), EGUI_ALPHA_100);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), page_demo_action_click_cb);
}

void test_init_ui(void)
{
    uint8_t i;
    egui_dim_t button_x = 10;

    page_demo_reset_model();

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&header_card), &header_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_card), 1, EGUI_COLOR_HEX(0xD7E1EA));
    egui_view_set_shadow(EGUI_VIEW_OF(&header_card), &page_card_shadow);

    egui_view_label_init(EGUI_VIEW_OF(&header_title));
    egui_view_set_position(EGUI_VIEW_OF(&header_title), 12, 10);
    egui_view_set_size(EGUI_VIEW_OF(&header_title), PAGE_HEADER_W - 24, PAGE_TITLE_H);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_title), PAGE_FONT_HEADER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_title), EGUI_COLOR_HEX(0x203243), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_title));

    egui_view_label_init(EGUI_VIEW_OF(&header_detail));
    egui_view_set_position(EGUI_VIEW_OF(&header_detail), 12, 28);
    egui_view_set_size(EGUI_VIEW_OF(&header_detail), PAGE_HEADER_W - 24, PAGE_TEXT_H);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_detail), PAGE_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_detail), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_detail), EGUI_COLOR_HEX(0x5A6D7E), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_detail));

    egui_view_label_init(EGUI_VIEW_OF(&header_hint));
    egui_view_set_position(EGUI_VIEW_OF(&header_hint), 12, 44);
    egui_view_set_size(EGUI_VIEW_OF(&header_hint), PAGE_HEADER_W - 24, PAGE_TEXT_H);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_hint), PAGE_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_hint), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_hint), EGUI_COLOR_HEX(0x6B7C8A), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_hint));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD7E1EA));

    for (i = 0; i < PAGE_ACTION_COUNT; i++)
    {
        page_demo_init_action_button(&action_buttons[i], button_x, page_demo_action_names[i]);
        egui_view_set_background(EGUI_VIEW_OF(&action_buttons[i]),
                                 i == PAGE_ACTION_ADD     ? EGUI_BG_OF(&action_add_bg)
                                 : i == PAGE_ACTION_DEL   ? EGUI_BG_OF(&action_del_bg)
                                 : i == PAGE_ACTION_PATCH ? EGUI_BG_OF(&action_patch_bg)
                                                          : EGUI_BG_OF(&action_jump_bg));
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
        button_x += PAGE_BUTTON_W + PAGE_BUTTON_GAP;
    }

    {
        const egui_view_virtual_page_setup_t page_view_setup = {
                .params = &page_view_params,
                .data_source = &page_demo_data_source,
                .data_source_context = &page_context,
                .state_cache_max_entries = PAGE_STATE_CACHE_COUNT,
                .state_cache_max_bytes = PAGE_STATE_CACHE_COUNT * (uint32_t)sizeof(page_demo_section_state_t),
        };

        egui_view_virtual_page_init_with_setup(EGUI_VIEW_OF(&page_view), &page_view_setup);
    }
    egui_view_set_background(EGUI_VIEW_OF(&page_view), EGUI_BG_OF(&page_bg));

    page_demo_refresh_header();

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&page_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&header_card));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t page_demo_match_visible_section(egui_view_t *self, const egui_view_virtual_page_slot_t *slot, const egui_view_virtual_page_entry_t *entry,
                                               egui_view_t *section_view, void *context)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(entry);
    EGUI_UNUSED(section_view);
    EGUI_UNUSED(context);
    return egui_view_virtual_viewport_is_slot_center_visible(EGUI_VIEW_OF(&page_view), slot);
}

static egui_view_t *page_demo_find_first_visible_section_view(void)
{
    return egui_view_virtual_page_find_first_visible_section_view(EGUI_VIEW_OF(&page_view), page_demo_match_visible_section, NULL, NULL);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    egui_view_t *view;

    switch (action_index)
    {
    case 0:
        view = page_demo_find_first_visible_section_view();
        if (view == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 160);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 700);
        return true;
    case 1:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT - 32;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = PAGE_VIEW_Y + 34;
        p_action->steps = 6;
        p_action->interval_ms = 180;
        return true;
    case 2:
        view = page_demo_find_first_visible_section_view();
        if (view == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 160);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 700);
        return true;
    case 3:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = PAGE_MARGIN_X + 10 + PAGE_BUTTON_W * 2 + PAGE_BUTTON_GAP * 2 + PAGE_BUTTON_W / 2;
        p_action->y1 = PAGE_TOOLBAR_Y + 6 + PAGE_BUTTON_H / 2;
        p_action->x2 = p_action->x1;
        p_action->y2 = p_action->y1;
        p_action->interval_ms = 650;
        return true;
    case 4:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = PAGE_MARGIN_X + 10 + (PAGE_BUTTON_W + PAGE_BUTTON_GAP) * 3 + PAGE_BUTTON_W / 2;
        p_action->y1 = PAGE_TOOLBAR_Y + 6 + PAGE_BUTTON_H / 2;
        p_action->x2 = p_action->x1;
        p_action->y2 = p_action->y1;
        p_action->interval_ms = 650;
        return true;
    case 5:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = PAGE_MARGIN_X + 10 + PAGE_BUTTON_W / 2;
        p_action->y1 = PAGE_TOOLBAR_Y + 6 + PAGE_BUTTON_H / 2;
        p_action->x2 = p_action->x1;
        p_action->y2 = p_action->y1;
        p_action->interval_ms = 650;
        return true;
    default:
        return false;
    }
}
#endif
