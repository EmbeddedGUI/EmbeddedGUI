#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "uicode.h"

#define SECTION_LIST_BASIC_SECTION_COUNT     10U
#define SECTION_LIST_BASIC_ITEMS_PER_SECTION 6U
#define SECTION_LIST_BASIC_INVALID_INDEX     0xFFFFFFFFUL
#define SECTION_LIST_BASIC_SECTION_ID_BASE   6000U
#define SECTION_LIST_BASIC_ITEM_ID_BASE      7000U
#define SECTION_LIST_BASIC_TITLE_TEXT_LEN    40
#define SECTION_LIST_BASIC_BODY_TEXT_LEN     48
#define SECTION_LIST_BASIC_META_TEXT_LEN     40
#define SECTION_LIST_BASIC_BADGE_TEXT_LEN    16

#define SECTION_LIST_BASIC_MARGIN_X   8
#define SECTION_LIST_BASIC_TOP_Y      8
#define SECTION_LIST_BASIC_HEADER_W   (EGUI_CONFIG_SCEEN_WIDTH - SECTION_LIST_BASIC_MARGIN_X * 2)
#define SECTION_LIST_BASIC_HEADER_H   0
#define SECTION_LIST_BASIC_TOOLBAR_Y  SECTION_LIST_BASIC_TOP_Y
#define SECTION_LIST_BASIC_TOOLBAR_H  34
#define SECTION_LIST_BASIC_VIEW_Y     (SECTION_LIST_BASIC_TOOLBAR_Y + SECTION_LIST_BASIC_TOOLBAR_H + 6)
#define SECTION_LIST_BASIC_VIEW_W     SECTION_LIST_BASIC_HEADER_W
#define SECTION_LIST_BASIC_VIEW_H     (EGUI_CONFIG_SCEEN_HEIGHT - SECTION_LIST_BASIC_VIEW_Y - 8)
#define SECTION_LIST_BASIC_ACTION_GAP 6
#define SECTION_LIST_BASIC_ACTION_W   ((SECTION_LIST_BASIC_HEADER_W - 20 - SECTION_LIST_BASIC_ACTION_GAP * 2) / 3)

#define SECTION_LIST_BASIC_HEADER_ENTRY_H 46
#define SECTION_LIST_BASIC_ITEM_H         54
#define SECTION_LIST_BASIC_ITEM_DETAIL_H  72
#define SECTION_LIST_BASIC_ENTRY_GAP      4
#define SECTION_LIST_BASIC_JUMP_VERIFY_RETRY_MAX 3U

#define SECTION_LIST_BASIC_FONT_TITLE ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define SECTION_LIST_BASIC_FONT_BODY  ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define SECTION_LIST_BASIC_FONT_HERO  ((const egui_font_t *)&egui_res_font_montserrat_12_4)

enum
{
    SECTION_LIST_BASIC_ACTION_PATCH = 0,
    SECTION_LIST_BASIC_ACTION_JUMP,
    SECTION_LIST_BASIC_ACTION_RESET,
    SECTION_LIST_BASIC_ACTION_COUNT,
};

enum
{
    SECTION_LIST_BASIC_TONE_INBOX = 0,
    SECTION_LIST_BASIC_TONE_OPS,
    SECTION_LIST_BASIC_TONE_CHAT,
    SECTION_LIST_BASIC_TONE_AUDIT,
    SECTION_LIST_BASIC_TONE_COUNT,
};

enum
{
    SECTION_LIST_BASIC_STATE_IDLE = 0,
    SECTION_LIST_BASIC_STATE_LIVE,
    SECTION_LIST_BASIC_STATE_WARN,
    SECTION_LIST_BASIC_STATE_COUNT,
};

typedef struct section_list_basic_item section_list_basic_item_t;
typedef struct section_list_basic_section section_list_basic_section_t;
typedef struct section_list_basic_header_view section_list_basic_header_view_t;
typedef struct section_list_basic_item_view section_list_basic_item_view_t;
typedef struct section_list_basic_context section_list_basic_context_t;
#if EGUI_CONFIG_RECORDING_TEST
typedef struct section_list_basic_visible_summary section_list_basic_visible_summary_t;
#endif

struct section_list_basic_item
{
    uint32_t stable_id;
    uint8_t detail;
    uint8_t progress;
    uint8_t state;
    uint8_t revision;
};

struct section_list_basic_section
{
    uint32_t stable_id;
    uint8_t tone;
    uint8_t collapsed;
    uint8_t revision;
    uint8_t item_count;
    section_list_basic_item_t items[SECTION_LIST_BASIC_ITEMS_PER_SECTION];
};

struct section_list_basic_header_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_card_t accent;
    egui_view_label_t title;
    egui_view_label_t meta;
    egui_view_label_t hint;
    char title_text[SECTION_LIST_BASIC_TITLE_TEXT_LEN];
    char meta_text[SECTION_LIST_BASIC_META_TEXT_LEN];
    char hint_text[SECTION_LIST_BASIC_BADGE_TEXT_LEN];
};

struct section_list_basic_item_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_card_t accent;
    egui_view_label_t title;
    egui_view_label_t body;
    egui_view_label_t meta;
    egui_view_label_t badge;
    egui_view_progress_bar_t progress;
    char title_text[SECTION_LIST_BASIC_TITLE_TEXT_LEN];
    char body_text[SECTION_LIST_BASIC_BODY_TEXT_LEN];
    char meta_text[SECTION_LIST_BASIC_META_TEXT_LEN];
    char badge_text[SECTION_LIST_BASIC_BADGE_TEXT_LEN];
};

struct section_list_basic_context
{
    section_list_basic_section_t sections[SECTION_LIST_BASIC_SECTION_COUNT];
    uint32_t selected_item_id;
    uint32_t last_clicked_section;
    uint32_t last_clicked_item;
    uint32_t click_count;
    uint32_t patch_count;
    uint32_t jump_cursor;
    uint32_t jump_target_id;
};

#if EGUI_CONFIG_RECORDING_TEST
struct section_list_basic_visible_summary
{
    uint32_t first_stable_id;
    uint8_t visible_count;
    uint8_t has_first;
};
#endif

static const char *section_list_basic_action_names[SECTION_LIST_BASIC_ACTION_COUNT] = {"Patch", "Jump", "Reset"};
static const char *section_list_basic_section_names[SECTION_LIST_BASIC_TONE_COUNT] = {"Inbox", "Ops", "Chat", "Audit"};
static const char *section_list_basic_item_names[SECTION_LIST_BASIC_TONE_COUNT] = {"Mail", "Task", "Thread", "Record"};
static const char *section_list_basic_state_names[SECTION_LIST_BASIC_STATE_COUNT] = {"IDLE", "LIVE", "WARN"};
static const char *section_list_basic_detail_notes[SECTION_LIST_BASIC_TONE_COUNT] = {"reply lane", "check deploy", "hit confirms row", "trace detail"};

static egui_view_t background_view;
static egui_view_card_t toolbar_card;
static egui_view_button_t action_buttons[SECTION_LIST_BASIC_ACTION_COUNT];
static egui_view_virtual_section_list_t section_list_view;
static section_list_basic_context_t section_list_basic_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
static uint8_t recording_jump_verify_retry;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(section_list_basic_toolbar_card_params, SECTION_LIST_BASIC_MARGIN_X, SECTION_LIST_BASIC_TOOLBAR_Y, SECTION_LIST_BASIC_HEADER_W,
                           SECTION_LIST_BASIC_TOOLBAR_H, 12);

static const egui_view_virtual_section_list_params_t section_list_basic_view_params = {
        .region = {{SECTION_LIST_BASIC_MARGIN_X, SECTION_LIST_BASIC_VIEW_Y}, {SECTION_LIST_BASIC_VIEW_W, SECTION_LIST_BASIC_VIEW_H}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 2,
        .estimated_entry_height = 58,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(section_list_basic_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF5F8),
                                    EGUI_COLOR_HEX(0xDDE8EF), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(section_list_basic_screen_bg_params, &section_list_basic_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(section_list_basic_screen_bg, &section_list_basic_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(section_list_basic_view_bg_param, EGUI_COLOR_HEX(0xF9FBFD), EGUI_ALPHA_100, 14, 1,
                                                        EGUI_COLOR_HEX(0xC6D8E4), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(section_list_basic_view_bg_params, &section_list_basic_view_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_list_basic_view_bg, &section_list_basic_view_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(section_list_basic_action_patch_param, EGUI_COLOR_HEX(0xE6F6EF), EGUI_ALPHA_100, 10, 1,
                                                        EGUI_COLOR_HEX(0xA7D4BF), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(section_list_basic_action_patch_params, &section_list_basic_action_patch_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_list_basic_action_patch_bg, &section_list_basic_action_patch_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(section_list_basic_action_jump_param, EGUI_COLOR_HEX(0xE7F0FA), EGUI_ALPHA_100, 10, 1,
                                                        EGUI_COLOR_HEX(0xACC4DB), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(section_list_basic_action_jump_params, &section_list_basic_action_jump_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_list_basic_action_jump_bg, &section_list_basic_action_jump_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(section_list_basic_action_reset_param, EGUI_COLOR_HEX(0xF9E9E4), EGUI_ALPHA_100, 10, 1,
                                                        EGUI_COLOR_HEX(0xD4B2A9), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(section_list_basic_action_reset_params, &section_list_basic_action_reset_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(section_list_basic_action_reset_bg, &section_list_basic_action_reset_params);

static uint32_t section_list_basic_ds_get_section_count(void *context);
static uint32_t section_list_basic_ds_get_section_stable_id(void *context, uint32_t section_index);
static int32_t section_list_basic_ds_find_section_index_by_stable_id(void *context, uint32_t stable_id);
static uint32_t section_list_basic_ds_get_item_count(void *context, uint32_t section_index);
static uint32_t section_list_basic_ds_get_item_stable_id(void *context, uint32_t section_index, uint32_t item_index);
static uint8_t section_list_basic_ds_find_item_position_by_stable_id(void *context, uint32_t stable_id, uint32_t *section_index, uint32_t *item_index);
static uint16_t section_list_basic_ds_get_section_header_view_type(void *context, uint32_t section_index);
static uint16_t section_list_basic_ds_get_item_view_type(void *context, uint32_t section_index, uint32_t item_index);
static int32_t section_list_basic_ds_measure_section_header_height(void *context, uint32_t section_index, int32_t width_hint);
static int32_t section_list_basic_ds_measure_item_height(void *context, uint32_t section_index, uint32_t item_index, int32_t width_hint);
static egui_view_t *section_list_basic_ds_create_section_header_view(void *context, uint16_t view_type);
static egui_view_t *section_list_basic_ds_create_item_view(void *context, uint16_t view_type);
static void section_list_basic_ds_destroy_section_header_view(void *context, egui_view_t *view, uint16_t view_type);
static void section_list_basic_ds_destroy_item_view(void *context, egui_view_t *view, uint16_t view_type);
static void section_list_basic_ds_bind_section_header_view(void *context, egui_view_t *view, uint32_t section_index, uint32_t stable_id);
static void section_list_basic_ds_bind_item_view(void *context, egui_view_t *view, uint32_t section_index, uint32_t item_index, uint32_t stable_id);
static uint8_t section_list_basic_ds_should_keep_item_alive(void *context, egui_view_t *view, uint32_t stable_id);

static const egui_view_virtual_section_list_data_source_t section_list_basic_data_source = {
        .get_section_count = section_list_basic_ds_get_section_count,
        .get_section_stable_id = section_list_basic_ds_get_section_stable_id,
        .find_section_index_by_stable_id = section_list_basic_ds_find_section_index_by_stable_id,
        .get_item_count = section_list_basic_ds_get_item_count,
        .get_item_stable_id = section_list_basic_ds_get_item_stable_id,
        .find_item_position_by_stable_id = section_list_basic_ds_find_item_position_by_stable_id,
        .get_section_header_view_type = section_list_basic_ds_get_section_header_view_type,
        .get_item_view_type = section_list_basic_ds_get_item_view_type,
        .measure_section_header_height = section_list_basic_ds_measure_section_header_height,
        .measure_item_height = section_list_basic_ds_measure_item_height,
        .create_section_header_view = section_list_basic_ds_create_section_header_view,
        .create_item_view = section_list_basic_ds_create_item_view,
        .destroy_section_header_view = section_list_basic_ds_destroy_section_header_view,
        .destroy_item_view = section_list_basic_ds_destroy_item_view,
        .bind_section_header_view = section_list_basic_ds_bind_section_header_view,
        .bind_item_view = section_list_basic_ds_bind_item_view,
        .unbind_section_header_view = NULL,
        .unbind_item_view = NULL,
        .should_keep_section_header_alive = NULL,
        .should_keep_item_alive = section_list_basic_ds_should_keep_item_alive,
        .save_section_header_state = NULL,
        .save_item_state = NULL,
        .restore_section_header_state = NULL,
        .restore_item_state = NULL,
        .default_section_header_view_type = 1,
        .default_item_view_type = 1,
};

static section_list_basic_section_t *section_list_basic_get_section(uint32_t section_index)
{
    if (section_index >= SECTION_LIST_BASIC_SECTION_COUNT)
    {
        return NULL;
    }

    return &section_list_basic_ctx.sections[section_index];
}

#if EGUI_CONFIG_RECORDING_TEST
static const section_list_basic_section_t *section_list_basic_get_section_const(uint32_t section_index)
{
    if (section_index >= SECTION_LIST_BASIC_SECTION_COUNT)
    {
        return NULL;
    }

    return &section_list_basic_ctx.sections[section_index];
}
#endif

static section_list_basic_item_t *section_list_basic_get_item(uint32_t section_index, uint32_t item_index)
{
    section_list_basic_section_t *section = section_list_basic_get_section(section_index);

    if (section == NULL || item_index >= section->item_count)
    {
        return NULL;
    }

    return &section->items[item_index];
}

#if EGUI_CONFIG_RECORDING_TEST
static const section_list_basic_item_t *section_list_basic_get_item_const(uint32_t section_index, uint32_t item_index)
{
    const section_list_basic_section_t *section = section_list_basic_get_section_const(section_index);

    if (section == NULL || item_index >= section->item_count)
    {
        return NULL;
    }

    return &section->items[item_index];
}
#endif

static int32_t section_list_basic_find_section_index_by_stable_id(uint32_t stable_id)
{
    uint32_t section_index;

    for (section_index = 0; section_index < SECTION_LIST_BASIC_SECTION_COUNT; section_index++)
    {
        if (section_list_basic_ctx.sections[section_index].stable_id == stable_id)
        {
            return (int32_t)section_index;
        }
    }

    return -1;
}

static uint8_t section_list_basic_find_item_position_by_stable_id(uint32_t stable_id, uint32_t *section_index, uint32_t *item_index)
{
    uint32_t current_section_index;

    if (section_index != NULL)
    {
        *section_index = SECTION_LIST_BASIC_INVALID_INDEX;
    }
    if (item_index != NULL)
    {
        *item_index = SECTION_LIST_BASIC_INVALID_INDEX;
    }

    for (current_section_index = 0; current_section_index < SECTION_LIST_BASIC_SECTION_COUNT; current_section_index++)
    {
        uint32_t current_item_index;
        const section_list_basic_section_t *section = &section_list_basic_ctx.sections[current_section_index];

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

static void section_list_basic_abort_motion(void)
{
    egui_scroller_about_animation(&section_list_view.base.base.scroller);
    section_list_view.base.base.is_begin_dragged = 0U;
}

static int32_t section_list_basic_measure_header_height_by_section(const section_list_basic_section_t *section)
{
    EGUI_UNUSED(section);
    return SECTION_LIST_BASIC_HEADER_ENTRY_H;
}

static int32_t section_list_basic_measure_item_height_by_item(const section_list_basic_item_t *item)
{
    return item != NULL && item->detail ? SECTION_LIST_BASIC_ITEM_DETAIL_H : SECTION_LIST_BASIC_ITEM_H;
}

#if EGUI_CONFIG_RECORDING_TEST
static uint32_t section_list_basic_get_total_visible_entries(void)
{
    uint32_t section_index;
    uint32_t total = 0;

    for (section_index = 0; section_index < SECTION_LIST_BASIC_SECTION_COUNT; section_index++)
    {
        total++;
        if (!section_list_basic_ctx.sections[section_index].collapsed)
        {
            total += section_list_basic_ctx.sections[section_index].item_count;
        }
    }

    return total;
}
#endif

static void section_list_basic_reset_model(void)
{
    uint32_t section_index;

    memset(&section_list_basic_ctx, 0, sizeof(section_list_basic_ctx));
    section_list_basic_ctx.selected_item_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    section_list_basic_ctx.last_clicked_section = SECTION_LIST_BASIC_INVALID_INDEX;
    section_list_basic_ctx.last_clicked_item = SECTION_LIST_BASIC_INVALID_INDEX;
    section_list_basic_ctx.jump_target_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;

    for (section_index = 0; section_index < SECTION_LIST_BASIC_SECTION_COUNT; section_index++)
    {
        uint32_t item_index;
        section_list_basic_section_t *section = &section_list_basic_ctx.sections[section_index];

        memset(section, 0, sizeof(*section));
        section->stable_id = SECTION_LIST_BASIC_SECTION_ID_BASE + section_index;
        section->tone = (uint8_t)(section_index % SECTION_LIST_BASIC_TONE_COUNT);
        section->collapsed = (uint8_t)((section_index % 4U) == 2U);
        section->revision = (uint8_t)(section_index % 5U);
        section->item_count = SECTION_LIST_BASIC_ITEMS_PER_SECTION;

        for (item_index = 0; item_index < section->item_count; item_index++)
        {
            section_list_basic_item_t *item = &section->items[item_index];

            item->stable_id = SECTION_LIST_BASIC_ITEM_ID_BASE + section_index * 16U + item_index;
            item->detail = (uint8_t)(((section_index * 3U + item_index) % 5U) == 4U);
            item->progress = (uint8_t)((section_index * 19U + item_index * 13U + 17U) % 100U);
            item->state = (uint8_t)((section_index + item_index) % SECTION_LIST_BASIC_STATE_COUNT);
            item->revision = (uint8_t)((section_index * 2U + item_index) % 7U);
        }
    }
}

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t section_list_basic_visible_entry_visitor(egui_view_t *self, const egui_view_virtual_section_list_slot_t *slot,
                                                        const egui_view_virtual_section_list_entry_t *entry, egui_view_t *entry_view, void *context)
{
    section_list_basic_visible_summary_t *summary = (section_list_basic_visible_summary_t *)context;

    EGUI_UNUSED(entry_view);

    if (slot == NULL || entry == NULL || !egui_view_virtual_viewport_is_slot_center_visible(self, slot))
    {
        return 1;
    }

    if (!summary->has_first)
    {
        summary->first_stable_id = entry->stable_id;
        summary->has_first = 1U;
    }

    summary->visible_count++;
    return 1;
}
#endif

static void section_list_basic_mark_selected(uint32_t stable_id)
{
    uint32_t previous_id = section_list_basic_ctx.selected_item_id;

    if (previous_id == stable_id)
    {
        return;
    }

    section_list_basic_ctx.selected_item_id = stable_id;
    if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_section_list_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&section_list_view), previous_id);
    }
    if (stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_section_list_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&section_list_view), stable_id);
    }
}

static uint8_t section_list_basic_resolve_entry_from_any_view(egui_view_t *view, egui_view_virtual_section_list_entry_t *entry)
{
    egui_view_t *cursor = view;

    while (cursor != NULL)
    {
        if (egui_view_virtual_section_list_resolve_entry_by_view(EGUI_VIEW_OF(&section_list_view), cursor, entry))
        {
            return 1;
        }
        cursor = EGUI_VIEW_PARENT(cursor);
    }

    return 0;
}

static void section_list_basic_header_click_cb(egui_view_t *self)
{
    egui_view_virtual_section_list_entry_t entry;
    section_list_basic_section_t *section;
    uint32_t selected_section_index;
    uint32_t selected_item_index;

    if (!section_list_basic_resolve_entry_from_any_view(self, &entry) || !entry.is_section_header || entry.section_index >= SECTION_LIST_BASIC_SECTION_COUNT)
    {
        return;
    }

    section = &section_list_basic_ctx.sections[entry.section_index];
    section->collapsed = (uint8_t)!section->collapsed;
    section_list_basic_ctx.last_clicked_section = entry.section_index;
    section_list_basic_ctx.last_clicked_item = SECTION_LIST_BASIC_INVALID_INDEX;
    section_list_basic_ctx.click_count++;

    if (section->collapsed &&
        section_list_basic_find_item_position_by_stable_id(section_list_basic_ctx.selected_item_id, &selected_section_index, &selected_item_index) &&
        selected_section_index == entry.section_index)
    {
        section_list_basic_mark_selected(EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID);
    }

    egui_view_virtual_section_list_notify_data_changed(EGUI_VIEW_OF(&section_list_view));
    egui_view_virtual_section_list_scroll_to_section(EGUI_VIEW_OF(&section_list_view), entry.section_index, 0);
}

static void section_list_basic_item_click_cb(egui_view_t *self)
{
    egui_view_virtual_section_list_entry_t entry;

    if (!section_list_basic_resolve_entry_from_any_view(self, &entry) || entry.is_section_header || entry.stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return;
    }

    section_list_basic_ctx.last_clicked_section = entry.section_index;
    section_list_basic_ctx.last_clicked_item = entry.item_index;
    section_list_basic_ctx.click_count++;
    section_list_basic_mark_selected(entry.stable_id);
    egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(EGUI_VIEW_OF(&section_list_view), entry.stable_id, SECTION_LIST_BASIC_ENTRY_GAP / 2);
}

static void section_list_basic_patch_selected(void)
{
    uint32_t section_index;
    uint32_t item_index;
    section_list_basic_section_t *section;
    section_list_basic_item_t *item;
    int32_t old_height;
    int32_t new_height;

    if (!section_list_basic_find_item_position_by_stable_id(section_list_basic_ctx.selected_item_id, &section_index, &item_index))
    {
        section_index = 0U;
        item_index = 0U;
        section_list_basic_mark_selected(section_list_basic_ctx.sections[0].items[0].stable_id);
    }

    section = section_list_basic_get_section(section_index);
    item = section_list_basic_get_item(section_index, item_index);
    if (section == NULL || item == NULL)
    {
        return;
    }

    old_height = section_list_basic_measure_item_height_by_item(item);
    item->detail = (uint8_t)!item->detail;
    item->progress = (uint8_t)((item->progress + 19U) % 100U);
    item->revision++;
    item->state = (uint8_t)((item->state + 1U) % SECTION_LIST_BASIC_STATE_COUNT);
    section->revision++;
    section_list_basic_ctx.patch_count++;
    new_height = section_list_basic_measure_item_height_by_item(item);

    egui_view_virtual_section_list_notify_section_header_changed(EGUI_VIEW_OF(&section_list_view), section_index);
    if (new_height != old_height)
    {
        egui_view_virtual_section_list_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&section_list_view), item->stable_id);
    }
    else
    {
        egui_view_virtual_section_list_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&section_list_view), item->stable_id);
    }

    (void)egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(EGUI_VIEW_OF(&section_list_view), item->stable_id, SECTION_LIST_BASIC_ENTRY_GAP / 2);
}

static void section_list_basic_jump_to_next(void)
{
    uint32_t section_index;
    uint32_t item_index;
    section_list_basic_section_t *section;
    uint32_t stable_id;

    section_list_basic_ctx.jump_cursor = (section_list_basic_ctx.jump_cursor + 1U) % SECTION_LIST_BASIC_SECTION_COUNT;
    section_index = section_list_basic_ctx.jump_cursor;
    section = section_list_basic_get_section(section_index);
    if (section == NULL || section->item_count == 0U)
    {
        return;
    }

    item_index = (section_index + 1U) % section->item_count;
    stable_id = section->items[item_index].stable_id;
    section_list_basic_ctx.jump_target_id = stable_id;

    section_list_basic_abort_motion();
    if (section->collapsed)
    {
        section->collapsed = 0U;
        egui_view_virtual_section_list_notify_data_changed(EGUI_VIEW_OF(&section_list_view));
    }

    section_list_basic_mark_selected(stable_id);
    egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&section_list_view), stable_id, 0);
    egui_view_virtual_section_list_scroll_to_item_by_stable_id(EGUI_VIEW_OF(&section_list_view), stable_id, 0);
    (void)egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(EGUI_VIEW_OF(&section_list_view), stable_id, SECTION_LIST_BASIC_ENTRY_GAP / 2);
}

static void section_list_basic_reset_demo(void)
{
    section_list_basic_abort_motion();
    section_list_basic_reset_model();
    egui_view_virtual_section_list_notify_data_changed(EGUI_VIEW_OF(&section_list_view));
    egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&section_list_view), section_list_basic_ctx.sections[0].stable_id, 0);
    egui_view_virtual_section_list_set_scroll_y(EGUI_VIEW_OF(&section_list_view), 0);
}

static int section_list_basic_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < SECTION_LIST_BASIC_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void section_list_basic_action_button_click_cb(egui_view_t *self)
{
    switch (section_list_basic_find_action_button_index(self))
    {
    case SECTION_LIST_BASIC_ACTION_PATCH:
        section_list_basic_patch_selected();
        break;
    case SECTION_LIST_BASIC_ACTION_JUMP:
        section_list_basic_jump_to_next();
        break;
    case SECTION_LIST_BASIC_ACTION_RESET:
        section_list_basic_reset_demo();
        break;
    default:
        break;
    }
}

static void section_list_basic_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, const egui_font_t *font,
                                          uint8_t align, egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), w, h);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), align);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static void section_list_basic_style_action_button(egui_view_button_t *button, uint8_t action_index)
{
    egui_background_t *background;

    switch (action_index)
    {
    case SECTION_LIST_BASIC_ACTION_PATCH:
        background = EGUI_BG_OF(&section_list_basic_action_patch_bg);
        break;
    case SECTION_LIST_BASIC_ACTION_JUMP:
        background = EGUI_BG_OF(&section_list_basic_action_jump_bg);
        break;
    default:
        background = EGUI_BG_OF(&section_list_basic_action_reset_bg);
        break;
    }

    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_size(EGUI_VIEW_OF(button), SECTION_LIST_BASIC_ACTION_W, 22);
    egui_view_set_background(EGUI_VIEW_OF(button), background);
    egui_view_label_set_font(EGUI_VIEW_OF(button), SECTION_LIST_BASIC_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x314454), EGUI_ALPHA_100);
    egui_view_label_set_text(EGUI_VIEW_OF(button), section_list_basic_action_names[action_index]);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), section_list_basic_action_button_click_cb);
}

static uint32_t section_list_basic_ds_get_section_count(void *context)
{
    EGUI_UNUSED(context);
    return SECTION_LIST_BASIC_SECTION_COUNT;
}

static uint32_t section_list_basic_ds_get_section_stable_id(void *context, uint32_t section_index)
{
    section_list_basic_context_t *ctx = (section_list_basic_context_t *)context;

    return section_index < SECTION_LIST_BASIC_SECTION_COUNT ? ctx->sections[section_index].stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t section_list_basic_ds_find_section_index_by_stable_id(void *context, uint32_t stable_id)
{
    EGUI_UNUSED(context);
    return section_list_basic_find_section_index_by_stable_id(stable_id);
}

static uint32_t section_list_basic_ds_get_item_count(void *context, uint32_t section_index)
{
    section_list_basic_context_t *ctx = (section_list_basic_context_t *)context;

    if (section_index >= SECTION_LIST_BASIC_SECTION_COUNT)
    {
        return 0U;
    }

    return ctx->sections[section_index].collapsed ? 0U : ctx->sections[section_index].item_count;
}

static uint32_t section_list_basic_ds_get_item_stable_id(void *context, uint32_t section_index, uint32_t item_index)
{
    section_list_basic_context_t *ctx = (section_list_basic_context_t *)context;

    if (section_index >= SECTION_LIST_BASIC_SECTION_COUNT || item_index >= ctx->sections[section_index].item_count)
    {
        return EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    return ctx->sections[section_index].items[item_index].stable_id;
}

static uint8_t section_list_basic_ds_find_item_position_by_stable_id(void *context, uint32_t stable_id, uint32_t *section_index, uint32_t *item_index)
{
    EGUI_UNUSED(context);
    return section_list_basic_find_item_position_by_stable_id(stable_id, section_index, item_index);
}

static uint16_t section_list_basic_ds_get_section_header_view_type(void *context, uint32_t section_index)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(section_index);
    return 1U;
}

static uint16_t section_list_basic_ds_get_item_view_type(void *context, uint32_t section_index, uint32_t item_index)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(section_index);
    EGUI_UNUSED(item_index);
    return 1U;
}

static int32_t section_list_basic_ds_measure_section_header_height(void *context, uint32_t section_index, int32_t width_hint)
{
    section_list_basic_context_t *ctx = (section_list_basic_context_t *)context;

    EGUI_UNUSED(width_hint);

    if (section_index >= SECTION_LIST_BASIC_SECTION_COUNT)
    {
        return SECTION_LIST_BASIC_HEADER_ENTRY_H;
    }

    return section_list_basic_measure_header_height_by_section(&ctx->sections[section_index]);
}

static int32_t section_list_basic_ds_measure_item_height(void *context, uint32_t section_index, uint32_t item_index, int32_t width_hint)
{
    section_list_basic_context_t *ctx = (section_list_basic_context_t *)context;

    EGUI_UNUSED(width_hint);

    if (section_index >= SECTION_LIST_BASIC_SECTION_COUNT || item_index >= ctx->sections[section_index].item_count)
    {
        return SECTION_LIST_BASIC_ITEM_H;
    }

    return section_list_basic_measure_item_height_by_item(&ctx->sections[section_index].items[item_index]);
}

static uint32_t section_list_basic_count_state(const section_list_basic_section_t *section, uint8_t state)
{
    uint32_t item_index;
    uint32_t count = 0U;

    if (section == NULL)
    {
        return 0U;
    }

    for (item_index = 0; item_index < section->item_count; item_index++)
    {
        if (section->items[item_index].state == state)
        {
            count++;
        }
    }

    return count;
}

static egui_dim_t section_list_basic_get_view_width(void)
{
    egui_dim_t width = EGUI_VIEW_OF(&section_list_view)->region.size.width;

    return width > 0 ? width : SECTION_LIST_BASIC_VIEW_W;
}

static egui_color_t section_list_basic_header_fill_color(const section_list_basic_section_t *section)
{
    if (section == NULL)
    {
        return EGUI_COLOR_WHITE;
    }

    switch (section->tone)
    {
    case SECTION_LIST_BASIC_TONE_OPS:
        return section->collapsed ? EGUI_COLOR_HEX(0xF1F8F3) : EGUI_COLOR_HEX(0xE5F3EA);
    case SECTION_LIST_BASIC_TONE_CHAT:
        return section->collapsed ? EGUI_COLOR_HEX(0xF1F6FC) : EGUI_COLOR_HEX(0xE7F0FA);
    case SECTION_LIST_BASIC_TONE_AUDIT:
        return section->collapsed ? EGUI_COLOR_HEX(0xFBF5EE) : EGUI_COLOR_HEX(0xF7EEDF);
    default:
        return section->collapsed ? EGUI_COLOR_HEX(0xF5F8FB) : EGUI_COLOR_HEX(0xEAF2FA);
    }
}

static egui_color_t section_list_basic_header_border_color(const section_list_basic_section_t *section)
{
    if (section == NULL)
    {
        return EGUI_COLOR_HEX(0xBFD1DE);
    }

    switch (section->tone)
    {
    case SECTION_LIST_BASIC_TONE_OPS:
        return EGUI_COLOR_HEX(0xB4D1BE);
    case SECTION_LIST_BASIC_TONE_CHAT:
        return EGUI_COLOR_HEX(0xBDD2E6);
    case SECTION_LIST_BASIC_TONE_AUDIT:
        return EGUI_COLOR_HEX(0xD9C29D);
    default:
        return EGUI_COLOR_HEX(0xBFD1DE);
    }
}

static egui_color_t section_list_basic_header_accent_color(const section_list_basic_section_t *section)
{
    if (section == NULL)
    {
        return EGUI_COLOR_HEX(0x5D8FC2);
    }

    switch (section->tone)
    {
    case SECTION_LIST_BASIC_TONE_OPS:
        return EGUI_COLOR_HEX(0x3F8E64);
    case SECTION_LIST_BASIC_TONE_CHAT:
        return EGUI_COLOR_HEX(0x4D82BC);
    case SECTION_LIST_BASIC_TONE_AUDIT:
        return EGUI_COLOR_HEX(0xC18B38);
    default:
        return EGUI_COLOR_HEX(0x5D8FC2);
    }
}

static egui_color_t section_list_basic_item_fill_color(const section_list_basic_section_t *section, uint8_t selected)
{
    if (selected)
    {
        return EGUI_COLOR_HEX(0xE7F0FA);
    }

    if (section == NULL)
    {
        return EGUI_COLOR_WHITE;
    }

    switch (section->tone)
    {
    case SECTION_LIST_BASIC_TONE_OPS:
        return EGUI_COLOR_HEX(0xF7FBF8);
    case SECTION_LIST_BASIC_TONE_CHAT:
        return EGUI_COLOR_HEX(0xF8FAFD);
    case SECTION_LIST_BASIC_TONE_AUDIT:
        return EGUI_COLOR_HEX(0xFFFBF6);
    default:
        return EGUI_COLOR_WHITE;
    }
}

static egui_color_t section_list_basic_item_border_color(const section_list_basic_section_t *section, uint8_t selected)
{
    if (selected)
    {
        return EGUI_COLOR_HEX(0x2F5E8A);
    }

    if (section == NULL)
    {
        return EGUI_COLOR_HEX(0xCDD8E1);
    }

    switch (section->tone)
    {
    case SECTION_LIST_BASIC_TONE_OPS:
        return EGUI_COLOR_HEX(0xC8DDD0);
    case SECTION_LIST_BASIC_TONE_CHAT:
        return EGUI_COLOR_HEX(0xCDD9E7);
    case SECTION_LIST_BASIC_TONE_AUDIT:
        return EGUI_COLOR_HEX(0xE2D1B9);
    default:
        return EGUI_COLOR_HEX(0xCDD8E1);
    }
}

static egui_color_t section_list_basic_state_color(const section_list_basic_item_t *item, uint8_t selected)
{
    if (selected)
    {
        return EGUI_COLOR_HEX(0x2F5E8A);
    }

    if (item == NULL)
    {
        return EGUI_COLOR_HEX(0x8A98A5);
    }

    switch (item->state)
    {
    case SECTION_LIST_BASIC_STATE_LIVE:
        return EGUI_COLOR_HEX(0x38986D);
    case SECTION_LIST_BASIC_STATE_WARN:
        return EGUI_COLOR_HEX(0xD08A2E);
    default:
        return EGUI_COLOR_HEX(0x8A98A5);
    }
}

static egui_view_t *section_list_basic_ds_create_section_header_view(void *context, uint16_t view_type)
{
    section_list_basic_header_view_t *view;
    egui_dim_t view_width = section_list_basic_get_view_width();

    EGUI_UNUSED(context);
    EGUI_UNUSED(view_type);

    view = (section_list_basic_header_view_t *)egui_malloc(sizeof(section_list_basic_header_view_t));
    if (view == NULL)
    {
        return NULL;
    }

    memset(view, 0, sizeof(*view));
    egui_view_group_init(EGUI_VIEW_OF(&view->root));
    egui_view_set_position(EGUI_VIEW_OF(&view->root), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&view->root), view_width, SECTION_LIST_BASIC_HEADER_ENTRY_H);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->root), section_list_basic_header_click_cb);

    egui_view_card_init(EGUI_VIEW_OF(&view->card));
    egui_view_set_position(EGUI_VIEW_OF(&view->card), 6, 4);
    egui_view_set_size(EGUI_VIEW_OF(&view->card), view_width - 12, SECTION_LIST_BASIC_HEADER_ENTRY_H - 8);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->card), section_list_basic_header_click_cb);
    egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->card));

    egui_view_card_init(EGUI_VIEW_OF(&view->accent));
    egui_view_set_position(EGUI_VIEW_OF(&view->accent), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&view->accent), 6, SECTION_LIST_BASIC_HEADER_ENTRY_H - 8);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->accent), section_list_basic_header_click_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->accent));

    section_list_basic_init_label(&view->title, 16, 8, view_width - 96, 14, SECTION_LIST_BASIC_FONT_TITLE, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                  EGUI_COLOR_HEX(0x24384A));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->title), section_list_basic_header_click_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->title));

    section_list_basic_init_label(&view->meta, 16, 26, view_width - 100, 12, SECTION_LIST_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                  EGUI_COLOR_HEX(0x5D7185));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->meta), section_list_basic_header_click_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->meta));

    section_list_basic_init_label(&view->hint, view_width - 74, 15, 44, 12, SECTION_LIST_BASIC_FONT_BODY, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x5D8FC2));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->hint), section_list_basic_header_click_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->hint));

    return EGUI_VIEW_OF(&view->root);
}

static egui_view_t *section_list_basic_ds_create_item_view(void *context, uint16_t view_type)
{
    section_list_basic_item_view_t *view;
    egui_dim_t view_width = section_list_basic_get_view_width();

    EGUI_UNUSED(context);
    EGUI_UNUSED(view_type);

    view = (section_list_basic_item_view_t *)egui_malloc(sizeof(section_list_basic_item_view_t));
    if (view == NULL)
    {
        return NULL;
    }

    memset(view, 0, sizeof(*view));
    egui_view_group_init(EGUI_VIEW_OF(&view->root));
    egui_view_set_position(EGUI_VIEW_OF(&view->root), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&view->root), view_width, SECTION_LIST_BASIC_ITEM_H);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->root), section_list_basic_item_click_cb);

    egui_view_card_init(EGUI_VIEW_OF(&view->card));
    egui_view_set_position(EGUI_VIEW_OF(&view->card), 28, 4);
    egui_view_set_size(EGUI_VIEW_OF(&view->card), view_width - 34, SECTION_LIST_BASIC_ITEM_H - 8);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->card), section_list_basic_item_click_cb);
    egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->card));

    egui_view_card_init(EGUI_VIEW_OF(&view->accent));
    egui_view_set_position(EGUI_VIEW_OF(&view->accent), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&view->accent), 4, SECTION_LIST_BASIC_ITEM_H - 8);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->accent), section_list_basic_item_click_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->accent));

    section_list_basic_init_label(&view->title, 14, 8, view_width - 116, 14, SECTION_LIST_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                  EGUI_COLOR_HEX(0x2B3F52));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->title), section_list_basic_item_click_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->title));

    section_list_basic_init_label(&view->body, 14, 24, view_width - 76, 12, SECTION_LIST_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                  EGUI_COLOR_HEX(0x607283));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->body), section_list_basic_item_click_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->body));

    section_list_basic_init_label(&view->meta, 14, 24, view_width - 76, 12, SECTION_LIST_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                  EGUI_COLOR_HEX(0x748493));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->meta), section_list_basic_item_click_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->meta));

    section_list_basic_init_label(&view->badge, view_width - 86, 8, 36, 12, SECTION_LIST_BASIC_FONT_BODY, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER,
                                  EGUI_COLOR_HEX(0x607283));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->badge), section_list_basic_item_click_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->badge));

    egui_view_progress_bar_init(EGUI_VIEW_OF(&view->progress));
    egui_view_set_position(EGUI_VIEW_OF(&view->progress), 14, 36);
    egui_view_set_size(EGUI_VIEW_OF(&view->progress), view_width - 76, 6);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->progress), section_list_basic_item_click_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->progress));

    return EGUI_VIEW_OF(&view->root);
}

static void section_list_basic_ds_destroy_section_header_view(void *context, egui_view_t *view, uint16_t view_type)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(view_type);
    egui_free(view);
}

static void section_list_basic_ds_destroy_item_view(void *context, egui_view_t *view, uint16_t view_type)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(view_type);
    egui_free(view);
}

static void section_list_basic_ds_bind_section_header_view(void *context, egui_view_t *view, uint32_t section_index, uint32_t stable_id)
{
    section_list_basic_context_t *ctx = (section_list_basic_context_t *)context;
    section_list_basic_header_view_t *header_view = (section_list_basic_header_view_t *)view;
    const section_list_basic_section_t *section;
    egui_dim_t view_width;
    egui_dim_t card_w;
    egui_dim_t card_h;
    egui_color_t accent_color;

    EGUI_UNUSED(stable_id);

    if (section_index >= SECTION_LIST_BASIC_SECTION_COUNT)
    {
        return;
    }

    section = &ctx->sections[section_index];
    view_width = section_list_basic_get_view_width();
    card_w = (egui_dim_t)(view_width - 12);
    card_h = (egui_dim_t)(section_list_basic_measure_header_height_by_section(section) - 8);
    accent_color = section_list_basic_header_accent_color(section);

    snprintf(header_view->title_text, sizeof(header_view->title_text), "%s %02u", section_list_basic_section_names[section->tone], (unsigned)section_index);
    snprintf(header_view->meta_text, sizeof(header_view->meta_text), "%u rows  %u live  r%u", (unsigned)section->item_count,
             (unsigned)section_list_basic_count_state(section, SECTION_LIST_BASIC_STATE_LIVE), (unsigned)section->revision);
    snprintf(header_view->hint_text, sizeof(header_view->hint_text), "%s", section->collapsed ? "Open" : "Fold");

    egui_view_set_position(EGUI_VIEW_OF(&header_view->root), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&header_view->root), view_width, (egui_dim_t)section_list_basic_measure_header_height_by_section(section));
    egui_view_set_position(EGUI_VIEW_OF(&header_view->card), 6, 4);
    egui_view_set_size(EGUI_VIEW_OF(&header_view->card), card_w, card_h);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_view->card), section_list_basic_header_fill_color(section), EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_view->card), 1, section_list_basic_header_border_color(section));

    egui_view_set_position(EGUI_VIEW_OF(&header_view->accent), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&header_view->accent), 6, card_h);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_view->accent), accent_color, EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&header_view->title), 16, 8);
    egui_view_set_size(EGUI_VIEW_OF(&header_view->title), card_w - 84, 14);
    egui_view_set_position(EGUI_VIEW_OF(&header_view->meta), 16, 26);
    egui_view_set_size(EGUI_VIEW_OF(&header_view->meta), card_w - 88, 12);
    egui_view_set_position(EGUI_VIEW_OF(&header_view->hint), card_w - 58, 15);
    egui_view_set_size(EGUI_VIEW_OF(&header_view->hint), 42, 12);

    egui_view_label_set_text(EGUI_VIEW_OF(&header_view->title), header_view->title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_view->meta), header_view->meta_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_view->hint), header_view->hint_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_view->title), EGUI_COLOR_HEX(0x22384B), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_view->meta), EGUI_COLOR_HEX(0x5D7185), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_view->hint), accent_color, EGUI_ALPHA_100);
}

static void section_list_basic_ds_bind_item_view(void *context, egui_view_t *view, uint32_t section_index, uint32_t item_index, uint32_t stable_id)
{
    section_list_basic_context_t *ctx = (section_list_basic_context_t *)context;
    section_list_basic_item_view_t *item_view = (section_list_basic_item_view_t *)view;
    const section_list_basic_section_t *section;
    const section_list_basic_item_t *item;
    uint8_t selected;
    egui_dim_t view_width;
    egui_dim_t item_h;
    egui_dim_t card_w;
    egui_dim_t card_h;
    egui_color_t state_color;

    EGUI_UNUSED(stable_id);

    if (section_index >= SECTION_LIST_BASIC_SECTION_COUNT || item_index >= ctx->sections[section_index].item_count)
    {
        return;
    }

    section = &ctx->sections[section_index];
    item = &ctx->sections[section_index].items[item_index];
    selected = (uint8_t)(item->stable_id == section_list_basic_ctx.selected_item_id);
    view_width = section_list_basic_get_view_width();
    item_h = (egui_dim_t)section_list_basic_measure_item_height_by_item(item);
    card_w = (egui_dim_t)(view_width - 34);
    card_h = (egui_dim_t)(item_h - 8);
    state_color = section_list_basic_state_color(item, selected);

    snprintf(item_view->title_text, sizeof(item_view->title_text), "%s %02u-%02u", section_list_basic_item_names[section->tone], (unsigned)section_index,
             (unsigned)item_index);
    snprintf(item_view->body_text, sizeof(item_view->body_text), item->detail ? "%s  rev %u" : "", section_list_basic_detail_notes[section->tone],
             (unsigned)item->revision);
    snprintf(item_view->meta_text, sizeof(item_view->meta_text), "%s  r%u", section_list_basic_state_names[item->state], (unsigned)item->revision);
    snprintf(item_view->badge_text, sizeof(item_view->badge_text), "%u%%", (unsigned)item->progress);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->root), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->root), view_width, item_h);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->card), 28, 4);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->card), card_w, card_h);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&item_view->card), section_list_basic_item_fill_color(section, selected), EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&item_view->card), 1, section_list_basic_item_border_color(section, selected));

    egui_view_set_position(EGUI_VIEW_OF(&item_view->accent), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->accent), selected ? 6 : 4, card_h);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&item_view->accent), state_color, EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->title), 14, 8);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->title), card_w - 64, 14);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->badge), card_w - 48, 8);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->badge), 36, 12);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->body), 14, 24);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->body), card_w - 28, 12);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->meta), 14, item->detail ? 40 : 24);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->meta), card_w - 28, 12);
    egui_view_set_position(EGUI_VIEW_OF(&item_view->progress), 14, item->detail ? 56 : 36);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->progress), card_w - 28, 6);

    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->title), item_view->title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->body), item_view->body_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->meta), item_view->meta_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->badge), item_view->badge_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->title), selected ? EGUI_COLOR_HEX(0x234866) : EGUI_COLOR_HEX(0x2B3F52), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->body), EGUI_COLOR_HEX(0x627485), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->meta), EGUI_COLOR_HEX(0x758594), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->badge), state_color, EGUI_ALPHA_100);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&item_view->progress), item->progress);
    item_view->progress.bk_color = EGUI_COLOR_HEX(0xDEE5EB);
    item_view->progress.progress_color = state_color;
    item_view->progress.control_color = state_color;
    item_view->progress.is_show_control = 0;
}

static uint8_t section_list_basic_ds_should_keep_item_alive(void *context, egui_view_t *view, uint32_t stable_id)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(view);

    return stable_id == section_list_basic_ctx.selected_item_id;
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

static uint8_t section_list_basic_is_view_clickable(egui_view_t *view)
{
    int click_x;
    int click_y;

    return (uint8_t)egui_sim_get_view_clipped_center(view, &EGUI_VIEW_OF(&section_list_view)->region_screen, &click_x, &click_y);
}

static uint8_t section_list_basic_set_click_item_action(egui_sim_action_t *p_action, egui_view_t *view, uint32_t interval_ms)
{
    return (uint8_t)egui_sim_set_click_view_clipped(p_action, view, &EGUI_VIEW_OF(&section_list_view)->region_screen, (int)interval_ms);
}

static egui_view_t *section_list_basic_find_visible_view_by_stable_id(uint32_t stable_id)
{
    const egui_view_virtual_section_list_slot_t *slot = egui_view_virtual_section_list_find_slot_by_stable_id(EGUI_VIEW_OF(&section_list_view), stable_id);
    egui_view_t *view;

    if (slot == NULL)
    {
        return NULL;
    }

    view = egui_view_virtual_section_list_find_view_by_stable_id(EGUI_VIEW_OF(&section_list_view), stable_id);
    return section_list_basic_is_view_clickable(view) ? view : NULL;
}

static void section_list_basic_set_scroll_action(egui_sim_action_t *p_action, uint32_t interval_ms)
{
    egui_region_t *region = &EGUI_VIEW_OF(&section_list_view)->region_screen;

    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = (egui_dim_t)(region->location.x + region->size.width / 2);
    p_action->y1 = (egui_dim_t)(region->location.y + region->size.height - 24);
    p_action->x2 = p_action->x1;
    p_action->y2 = (egui_dim_t)(region->location.y + 42);
    p_action->steps = 10;
    p_action->interval_ms = interval_ms;
}

static uint32_t section_list_basic_get_first_center_visible_id(void)
{
    section_list_basic_visible_summary_t summary;

    memset(&summary, 0, sizeof(summary));
    egui_view_virtual_section_list_visit_visible_entries(EGUI_VIEW_OF(&section_list_view), section_list_basic_visible_entry_visitor, &summary);
    return summary.has_first ? summary.first_stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    static uint32_t visible_before_collapse = 0U;
    static uint32_t expected_target_section = 0U;
    static uint32_t expected_target_item = 0U;
    int first_call = (action_index != last_action);
    egui_view_t *view;
    const section_list_basic_item_t *item;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            if (egui_view_virtual_section_list_get_slot_count(EGUI_VIEW_OF(&section_list_view)) == 0U)
            {
                report_runtime_failure("section list basic should materialize initial slots");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        view = section_list_basic_find_visible_view_by_stable_id(section_list_basic_ctx.sections[0].items[0].stable_id);
        if (view == NULL)
        {
            report_runtime_failure("first grouped row was not visible");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (!section_list_basic_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("first grouped row click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
        }
        return true;
    case 2:
        if (first_call && section_list_basic_ctx.selected_item_id != section_list_basic_ctx.sections[0].items[0].stable_id)
        {
            report_runtime_failure("row click did not update selected item");
        }
        if (first_call)
        {
            section_list_basic_abort_motion();
            egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&section_list_view), section_list_basic_ctx.sections[1].stable_id, 0);
            egui_view_virtual_section_list_scroll_to_section(EGUI_VIEW_OF(&section_list_view), 1, 0);
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 3:
        view = section_list_basic_find_visible_view_by_stable_id(section_list_basic_ctx.sections[1].stable_id);
        if (view == NULL)
        {
            report_runtime_failure("second section header was not visible");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        visible_before_collapse = section_list_basic_get_total_visible_entries();
        if (!section_list_basic_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("second section header click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
        }
        return true;
    case 4:
        if (first_call && section_list_basic_get_total_visible_entries() >= visible_before_collapse)
        {
            report_runtime_failure("header click did not collapse a section");
        }
        recording_jump_verify_retry = 0U;
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[SECTION_LIST_BASIC_ACTION_JUMP]), 220);
        return true;
    case 5:
        expected_target_section = section_list_basic_ctx.jump_cursor;
        expected_target_item = (expected_target_section + 1U) % SECTION_LIST_BASIC_ITEMS_PER_SECTION;
        view = section_list_basic_find_visible_view_by_stable_id(section_list_basic_ctx.jump_target_id);
        if (section_list_basic_ctx.jump_target_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID || section_list_basic_ctx.sections[expected_target_section].collapsed ||
            section_list_basic_ctx.selected_item_id != section_list_basic_ctx.jump_target_id)
        {
            if (recording_jump_verify_retry < SECTION_LIST_BASIC_JUMP_VERIFY_RETRY_MAX)
            {
                if (section_list_basic_ctx.jump_target_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
                {
                    section_list_basic_abort_motion();
                    egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&section_list_view), section_list_basic_ctx.jump_target_id, 0);
                    egui_view_virtual_section_list_scroll_to_item_by_stable_id(EGUI_VIEW_OF(&section_list_view), section_list_basic_ctx.jump_target_id, 0);
                    (void)egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(EGUI_VIEW_OF(&section_list_view), section_list_basic_ctx.jump_target_id,
                                                                                           0);
                }
                recording_jump_verify_retry++;
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            if (section_list_basic_ctx.jump_target_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
            {
                report_runtime_failure("jump action did not choose a target item");
            }
            else if (section_list_basic_ctx.sections[expected_target_section].collapsed)
            {
                report_runtime_failure("jump action did not reopen collapsed target section");
            }
            else
            {
                report_runtime_failure("jump action did not select target row");
            }
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        recording_jump_verify_retry = 0U;
        if (view != NULL && !section_list_basic_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("jump target row click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 6:
        if (first_call && section_list_basic_ctx.selected_item_id != section_list_basic_ctx.jump_target_id)
        {
            report_runtime_failure("target row was not selected correctly after jump");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[SECTION_LIST_BASIC_ACTION_PATCH]), 220);
        return true;
    case 7:
        item = section_list_basic_get_item_const(expected_target_section, expected_target_item);
        if (first_call && (item == NULL || !item->detail))
        {
            report_runtime_failure("patch action did not expand target row detail");
        }
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 8:
        section_list_basic_set_scroll_action(p_action, 320);
        return true;
    case 9:
        if (first_call && egui_view_virtual_section_list_get_scroll_y(EGUI_VIEW_OF(&section_list_view)) <= 0)
        {
            report_runtime_failure("scroll action did not move section list viewport");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[SECTION_LIST_BASIC_ACTION_RESET]), 220);
        return true;
    case 10:
        if (first_call)
        {
            if (section_list_basic_ctx.selected_item_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
            {
                report_runtime_failure("reset action did not clear selected row");
            }
            if (egui_view_virtual_section_list_get_scroll_y(EGUI_VIEW_OF(&section_list_view)) != 0)
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
    egui_view_virtual_section_list_setup_t setup = {
            .params = &section_list_basic_view_params,
            .data_source = &section_list_basic_data_source,
            .data_source_context = &section_list_basic_ctx,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };

    section_list_basic_reset_model();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0U;
    recording_jump_verify_retry = 0U;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&section_list_basic_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &section_list_basic_toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD1DEE7));

    for (i = 0; i < SECTION_LIST_BASIC_ACTION_COUNT; i++)
    {
        section_list_basic_style_action_button(&action_buttons[i], i);
        egui_view_set_position(EGUI_VIEW_OF(&action_buttons[i]), 10 + i * (SECTION_LIST_BASIC_ACTION_W + SECTION_LIST_BASIC_ACTION_GAP), 6);
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
    }

    egui_view_virtual_section_list_init_with_setup(EGUI_VIEW_OF(&section_list_view), &setup);
    egui_view_set_background(EGUI_VIEW_OF(&section_list_view), EGUI_BG_OF(&section_list_basic_view_bg));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&section_list_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}
