#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "uicode.h"

#define BASIC_VIEWPORT_ITEM_COUNT   160U
#define BASIC_VIEWPORT_STABLE_BASE  1000U
#define BASIC_INVALID_INDEX         0xFFFFFFFFUL
#define BASIC_BUTTON_TEXT_LEN       32
#define BASIC_TITLE_TEXT_LEN        28
#define BASIC_VALUE_TEXT_LEN        16
#define BASIC_JUMP_STEP             32U
#define BASIC_JUMP_VERIFY_RETRY_MAX 3U
#define BASIC_RESET_VERIFY_RETRY_MAX 3U

#define BASIC_MARGIN_X   8
#define BASIC_TOP_Y      8
#define BASIC_HEADER_W   (EGUI_CONFIG_SCEEN_WIDTH - BASIC_MARGIN_X * 2)
#define BASIC_HEADER_H   0
#define BASIC_TOOLBAR_Y  BASIC_TOP_Y
#define BASIC_TOOLBAR_H  34
#define BASIC_VIEWPORT_Y (BASIC_TOOLBAR_Y + BASIC_TOOLBAR_H + 6)
#define BASIC_VIEWPORT_W BASIC_HEADER_W
#define BASIC_VIEWPORT_H (EGUI_CONFIG_SCEEN_HEIGHT - BASIC_VIEWPORT_Y - 8)
#define BASIC_ACTION_GAP 6
#define BASIC_ACTION_W   ((BASIC_HEADER_W - 20 - BASIC_ACTION_GAP * 2) / 3)

#define BASIC_BUTTON_ROW_H   44
#define BASIC_SLIDER_ROW_H   58
#define BASIC_ROW_SIDE_INSET 8
#define BASIC_BUTTON_INNER_H 34
#define BASIC_SLIDER_CARD_H  50

#define BASIC_FONT_TITLE ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define BASIC_FONT_BODY  ((const egui_font_t *)&egui_res_font_montserrat_8_4)

enum
{
    BASIC_ITEM_KIND_BUTTON = 1,
    BASIC_ITEM_KIND_SLIDER = 2,
};

enum
{
    BASIC_VIEW_TYPE_BUTTON_ROW = 1,
    BASIC_VIEW_TYPE_SLIDER_ROW = 2,
};

enum
{
    BASIC_ACTION_PATCH = 0,
    BASIC_ACTION_JUMP,
    BASIC_ACTION_RESET,
    BASIC_ACTION_COUNT,
};

typedef struct basic_item basic_item_t;
typedef struct basic_button_row basic_button_row_t;
typedef struct basic_slider_row basic_slider_row_t;
typedef struct basic_viewport_context basic_viewport_context_t;
#if EGUI_CONFIG_RECORDING_TEST
typedef struct basic_visible_summary basic_visible_summary_t;
#endif

struct basic_item
{
    uint32_t stable_id;
    uint8_t kind;
    uint8_t enabled;
    uint8_t value;
    uint8_t revision;
};

struct basic_button_row
{
    egui_view_group_t root;
    egui_view_button_t button;
    char text[BASIC_BUTTON_TEXT_LEN];
    uint32_t stable_id;
};

struct basic_slider_row
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t value;
    egui_view_slider_t slider;
    char title_text[BASIC_TITLE_TEXT_LEN];
    char value_text[BASIC_VALUE_TEXT_LEN];
    uint32_t stable_id;
};

struct basic_viewport_context
{
    basic_item_t items[BASIC_VIEWPORT_ITEM_COUNT];
    uint32_t selected_id;
    uint32_t last_clicked_index;
    uint32_t click_count;
    uint32_t patch_count;
    uint32_t jump_cursor;
};

#if EGUI_CONFIG_RECORDING_TEST
struct basic_visible_summary
{
    uint32_t first_index;
    uint8_t visible_count;
    uint8_t has_first;
};
#endif

static const char *basic_action_names[BASIC_ACTION_COUNT] = {"Patch", "Jump", "Reset"};

static egui_view_t background_view;
static egui_view_card_t toolbar_card;
static egui_view_button_t action_buttons[BASIC_ACTION_COUNT];
static egui_view_virtual_viewport_t viewport_view;
static basic_viewport_context_t basic_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
static uint8_t recording_jump_verify_retry;
static uint8_t recording_reset_verify_retry;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(basic_toolbar_card_params, BASIC_MARGIN_X, BASIC_TOOLBAR_Y, BASIC_HEADER_W, BASIC_TOOLBAR_H, 12);

static const egui_view_virtual_viewport_params_t basic_viewport_params = {
        .region = {{BASIC_MARGIN_X, BASIC_VIEWPORT_Y}, {BASIC_VIEWPORT_W, BASIC_VIEWPORT_H}},
        .orientation = EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL,
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 2,
        .estimated_item_extent = 48,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(basic_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEFF4F7), EGUI_COLOR_HEX(0xDDE9F0),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_screen_bg_params, &basic_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(basic_screen_bg, &basic_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_viewport_bg_param, EGUI_COLOR_HEX(0xF8FBFD), EGUI_ALPHA_100, 14, 1, EGUI_COLOR_HEX(0xC9D9E4),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_viewport_bg_params, &basic_viewport_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_viewport_bg, &basic_viewport_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_button_idle_param, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 11, 1, EGUI_COLOR_HEX(0xB8C9D5),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_button_idle_params, &basic_button_idle_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_button_idle_bg, &basic_button_idle_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_button_on_param, EGUI_COLOR_HEX(0xDFF4EA), EGUI_ALPHA_100, 11, 1, EGUI_COLOR_HEX(0x7BBF9D),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_button_on_params, &basic_button_on_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_button_on_bg, &basic_button_on_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_button_selected_param, EGUI_COLOR_HEX(0x2F5E8A), EGUI_ALPHA_100, 11, 1, EGUI_COLOR_HEX(0x224665),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_button_selected_params, &basic_button_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_button_selected_bg, &basic_button_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_action_patch_param, EGUI_COLOR_HEX(0xE6F6EF), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xA7D4BF),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_action_patch_params, &basic_action_patch_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_action_patch_bg, &basic_action_patch_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_action_jump_param, EGUI_COLOR_HEX(0xE7F0FA), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xACC4DB),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_action_jump_params, &basic_action_jump_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_action_jump_bg, &basic_action_jump_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_action_reset_param, EGUI_COLOR_HEX(0xF9E9E4), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xD4B2A9),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_action_reset_params, &basic_action_reset_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_action_reset_bg, &basic_action_reset_params);

static uint32_t basic_adapter_get_count(void *adapter_context);
static uint32_t basic_adapter_get_stable_id(void *adapter_context, uint32_t index);
static int32_t basic_adapter_find_index_by_stable_id(void *adapter_context, uint32_t stable_id);
static uint16_t basic_adapter_get_view_type(void *adapter_context, uint32_t index);
static int32_t basic_adapter_measure_main_size(void *adapter_context, uint32_t index, int32_t cross_size_hint);
static egui_view_t *basic_adapter_create_view(void *adapter_context, uint16_t view_type);
static void basic_adapter_destroy_view(void *adapter_context, egui_view_t *view, uint16_t view_type);
static void basic_adapter_bind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id);
static uint8_t basic_adapter_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t stable_id);

static const egui_view_virtual_viewport_adapter_t basic_viewport_adapter = {
        .get_count = basic_adapter_get_count,
        .get_stable_id = basic_adapter_get_stable_id,
        .find_index_by_stable_id = basic_adapter_find_index_by_stable_id,
        .get_view_type = basic_adapter_get_view_type,
        .measure_main_size = basic_adapter_measure_main_size,
        .create_view = basic_adapter_create_view,
        .destroy_view = basic_adapter_destroy_view,
        .bind_view = basic_adapter_bind_view,
        .unbind_view = NULL,
        .should_keep_alive = basic_adapter_should_keep_alive,
        .save_state = NULL,
        .restore_state = NULL,
};

static basic_item_t *basic_get_item(uint32_t index)
{
    if (index >= BASIC_VIEWPORT_ITEM_COUNT)
    {
        return NULL;
    }

    return &basic_ctx.items[index];
}

static int32_t basic_find_index_by_stable_id(uint32_t stable_id)
{
    uint32_t i;

    for (i = 0; i < BASIC_VIEWPORT_ITEM_COUNT; i++)
    {
        if (basic_ctx.items[i].stable_id == stable_id)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static void basic_abort_viewport_motion(void)
{
    egui_scroller_about_animation(&viewport_view.scroller);
    viewport_view.is_begin_dragged = 0U;
}

static void basic_init_items(void)
{
    uint32_t i;

    for (i = 0; i < BASIC_VIEWPORT_ITEM_COUNT; i++)
    {
        basic_item_t *item = &basic_ctx.items[i];

        item->stable_id = BASIC_VIEWPORT_STABLE_BASE + i;
        item->kind = (i % 4U == 1U) ? BASIC_ITEM_KIND_SLIDER : BASIC_ITEM_KIND_BUTTON;
        item->enabled = (uint8_t)((i % 3U) == 0U);
        item->value = (uint8_t)((i * 17U + 21U) % 100U);
        item->revision = 0U;
    }

    basic_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    basic_ctx.last_clicked_index = BASIC_INVALID_INDEX;
    basic_ctx.click_count = 0U;
    basic_ctx.patch_count = 0U;
    basic_ctx.jump_cursor = 0U;
}

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t basic_visible_item_visitor(egui_view_t *self, const egui_view_virtual_viewport_slot_t *slot, const egui_view_virtual_viewport_entry_t *entry,
                                          egui_view_t *item_view, void *context)
{
    basic_visible_summary_t *summary = (basic_visible_summary_t *)context;

    EGUI_UNUSED(item_view);

    if (slot == NULL || entry == NULL || !egui_view_virtual_viewport_is_slot_center_visible(self, slot))
    {
        return 1;
    }

    if (!summary->has_first)
    {
        summary->first_index = entry->index;
        summary->has_first = 1U;
    }
    summary->visible_count++;
    return 1;
}
#endif

static void basic_mark_selected(uint32_t stable_id)
{
    uint32_t previous_id = basic_ctx.selected_id;

    if (previous_id == stable_id)
    {
        return;
    }

    basic_ctx.selected_id = stable_id;
    if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_viewport_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&viewport_view), previous_id);
    }
    if (stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_viewport_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&viewport_view), stable_id);
    }
}

static void basic_button_click_cb(egui_view_t *self)
{
    egui_view_virtual_viewport_entry_t entry;
    basic_item_t *item;

    if (!egui_view_virtual_viewport_resolve_item_by_view(EGUI_VIEW_OF(&viewport_view), self, &entry))
    {
        return;
    }

    item = basic_get_item(entry.index);
    if (item == NULL)
    {
        return;
    }

    item->enabled = (uint8_t)!item->enabled;
    item->revision++;
    basic_ctx.last_clicked_index = entry.index;
    basic_ctx.click_count++;
    basic_mark_selected(entry.stable_id);
    egui_view_virtual_viewport_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&viewport_view), entry.stable_id);
}

static void basic_slider_value_changed_cb(egui_view_t *self, uint8_t value)
{
    egui_view_virtual_viewport_entry_t entry;
    basic_item_t *item;

    if (!egui_view_virtual_viewport_resolve_item_by_view(EGUI_VIEW_OF(&viewport_view), self, &entry))
    {
        return;
    }

    item = basic_get_item(entry.index);
    if (item == NULL)
    {
        return;
    }

    item->value = value;
    basic_ctx.last_clicked_index = entry.index;
    basic_mark_selected(entry.stable_id);
    egui_view_virtual_viewport_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&viewport_view), entry.stable_id);
}

static void basic_patch_selected(void)
{
    int32_t index = basic_find_index_by_stable_id(basic_ctx.selected_id);
    basic_item_t *item;

    if (index < 0)
    {
        index = 0;
        basic_mark_selected(basic_ctx.items[0].stable_id);
    }

    item = basic_get_item((uint32_t)index);
    if (item == NULL)
    {
        return;
    }

    if (item->kind == BASIC_ITEM_KIND_SLIDER)
    {
        item->value = (uint8_t)((item->value + 23U) % 100U);
    }
    else
    {
        item->enabled = (uint8_t)!item->enabled;
    }
    item->revision++;
    basic_ctx.patch_count++;
    egui_view_virtual_viewport_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&viewport_view), item->stable_id);
}

static void basic_jump_to_next(void)
{
    uint32_t target_index;
    uint32_t stable_id;

    basic_ctx.jump_cursor = (basic_ctx.jump_cursor + BASIC_JUMP_STEP) % BASIC_VIEWPORT_ITEM_COUNT;
    target_index = basic_ctx.jump_cursor;
    stable_id = basic_ctx.items[target_index].stable_id;

    basic_abort_viewport_motion();
    basic_mark_selected(stable_id);
    egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&viewport_view), stable_id, 0);
    egui_view_virtual_viewport_scroll_to_stable_id(EGUI_VIEW_OF(&viewport_view), stable_id, 0);
    (void)egui_view_virtual_viewport_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&viewport_view), stable_id, 8);
}

static void basic_reset_demo(void)
{
    basic_abort_viewport_motion();
    basic_init_items();
    egui_view_virtual_viewport_notify_data_changed(EGUI_VIEW_OF(&viewport_view));
    egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&viewport_view), basic_ctx.items[0].stable_id, 0);
    egui_view_virtual_viewport_set_logical_offset(EGUI_VIEW_OF(&viewport_view), 0);
}

static int basic_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < BASIC_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void basic_action_button_click_cb(egui_view_t *self)
{
    switch (basic_find_action_button_index(self))
    {
    case BASIC_ACTION_PATCH:
        basic_patch_selected();
        break;
    case BASIC_ACTION_JUMP:
        basic_jump_to_next();
        break;
    case BASIC_ACTION_RESET:
        basic_reset_demo();
        break;
    default:
        break;
    }
}

static void basic_style_action_button(egui_view_button_t *button, uint8_t action_index)
{
    egui_background_t *background;

    switch (action_index)
    {
    case BASIC_ACTION_PATCH:
        background = EGUI_BG_OF(&basic_action_patch_bg);
        break;
    case BASIC_ACTION_JUMP:
        background = EGUI_BG_OF(&basic_action_jump_bg);
        break;
    default:
        background = EGUI_BG_OF(&basic_action_reset_bg);
        break;
    }

    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_size(EGUI_VIEW_OF(button), BASIC_ACTION_W, 22);
    egui_view_set_background(EGUI_VIEW_OF(button), background);
    egui_view_label_set_font(EGUI_VIEW_OF(button), BASIC_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x314454), EGUI_ALPHA_100);
    egui_view_label_set_text(EGUI_VIEW_OF(button), basic_action_names[action_index]);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), basic_action_button_click_cb);
}

static uint32_t basic_adapter_get_count(void *adapter_context)
{
    EGUI_UNUSED(adapter_context);
    return BASIC_VIEWPORT_ITEM_COUNT;
}

static uint32_t basic_adapter_get_stable_id(void *adapter_context, uint32_t index)
{
    basic_viewport_context_t *ctx = (basic_viewport_context_t *)adapter_context;

    return index < BASIC_VIEWPORT_ITEM_COUNT ? ctx->items[index].stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t basic_adapter_find_index_by_stable_id(void *adapter_context, uint32_t stable_id)
{
    EGUI_UNUSED(adapter_context);
    return basic_find_index_by_stable_id(stable_id);
}

static uint16_t basic_adapter_get_view_type(void *adapter_context, uint32_t index)
{
    basic_viewport_context_t *ctx = (basic_viewport_context_t *)adapter_context;

    if (index >= BASIC_VIEWPORT_ITEM_COUNT)
    {
        return BASIC_VIEW_TYPE_BUTTON_ROW;
    }

    return ctx->items[index].kind == BASIC_ITEM_KIND_SLIDER ? BASIC_VIEW_TYPE_SLIDER_ROW : BASIC_VIEW_TYPE_BUTTON_ROW;
}

static int32_t basic_adapter_measure_main_size(void *adapter_context, uint32_t index, int32_t cross_size_hint)
{
    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(cross_size_hint);

    return basic_adapter_get_view_type(adapter_context, index) == BASIC_VIEW_TYPE_SLIDER_ROW ? BASIC_SLIDER_ROW_H : BASIC_BUTTON_ROW_H;
}

static void basic_bind_button_row(basic_button_row_t *row, uint32_t index, const basic_item_t *item)
{
    egui_background_t *background = EGUI_BG_OF(&basic_button_idle_bg);
    egui_color_t text_color = EGUI_COLOR_HEX(0x2B4153);

    snprintf(row->text, sizeof(row->text), "Line %03lu %s", (unsigned long)index, item->enabled ? "on" : "off");

    if (item->stable_id == basic_ctx.selected_id)
    {
        background = EGUI_BG_OF(&basic_button_selected_bg);
        text_color = EGUI_COLOR_WHITE;
    }
    else if (item->enabled)
    {
        background = EGUI_BG_OF(&basic_button_on_bg);
        text_color = EGUI_COLOR_HEX(0x22553D);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&row->button), row->text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->button), text_color, EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&row->button), background);
}

static void basic_bind_slider_row(basic_slider_row_t *row, uint32_t index, const basic_item_t *item)
{
    egui_color_t fill_color = item->stable_id == basic_ctx.selected_id ? EGUI_COLOR_HEX(0xE8F1FA) : EGUI_COLOR_WHITE;
    egui_color_t border_color = item->stable_id == basic_ctx.selected_id ? EGUI_COLOR_HEX(0x6D95BD) : EGUI_COLOR_HEX(0xC7D7E3);

    snprintf(row->title_text, sizeof(row->title_text), "Speed %03lu", (unsigned long)index);
    snprintf(row->value_text, sizeof(row->value_text), "%u", (unsigned)item->value);

    egui_view_label_set_text(EGUI_VIEW_OF(&row->title), row->title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->value), row->value_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->value), item->stable_id == basic_ctx.selected_id ? EGUI_COLOR_HEX(0x2F5E8A) : EGUI_COLOR_HEX(0x526678),
                                   EGUI_ALPHA_100);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&row->card), fill_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&row->card), 1, border_color);
    egui_view_slider_set_value(EGUI_VIEW_OF(&row->slider), item->value);
}

static egui_view_t *basic_adapter_create_view(void *adapter_context, uint16_t view_type)
{
    EGUI_UNUSED(adapter_context);

    if (view_type == BASIC_VIEW_TYPE_SLIDER_ROW)
    {
        basic_slider_row_t *row = (basic_slider_row_t *)egui_malloc(sizeof(basic_slider_row_t));
        if (row == NULL)
        {
            return NULL;
        }

        memset(row, 0, sizeof(*row));
        egui_view_group_init(EGUI_VIEW_OF(&row->root));

        egui_view_card_init(EGUI_VIEW_OF(&row->card));
        egui_view_set_position(EGUI_VIEW_OF(&row->card), BASIC_ROW_SIDE_INSET, 4);
        egui_view_set_size(EGUI_VIEW_OF(&row->card), BASIC_VIEWPORT_W - BASIC_ROW_SIDE_INSET * 2, BASIC_SLIDER_CARD_H);
        egui_view_card_set_border(EGUI_VIEW_OF(&row->card), 1, EGUI_COLOR_HEX(0xC7D7E3));
        egui_view_group_add_child(EGUI_VIEW_OF(&row->root), EGUI_VIEW_OF(&row->card));

        egui_view_label_init(EGUI_VIEW_OF(&row->title));
        egui_view_set_position(EGUI_VIEW_OF(&row->title), 10, 7);
        egui_view_set_size(EGUI_VIEW_OF(&row->title), 160, 12);
        egui_view_label_set_font(EGUI_VIEW_OF(&row->title), BASIC_FONT_BODY);
        egui_view_label_set_align_type(EGUI_VIEW_OF(&row->title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&row->title), EGUI_COLOR_HEX(0x2B4153), EGUI_ALPHA_100);
        egui_view_card_add_child(EGUI_VIEW_OF(&row->card), EGUI_VIEW_OF(&row->title));

        egui_view_label_init(EGUI_VIEW_OF(&row->value));
        egui_view_set_position(EGUI_VIEW_OF(&row->value), BASIC_VIEWPORT_W - BASIC_ROW_SIDE_INSET * 2 - 52, 7);
        egui_view_set_size(EGUI_VIEW_OF(&row->value), 40, 12);
        egui_view_label_set_font(EGUI_VIEW_OF(&row->value), BASIC_FONT_BODY);
        egui_view_label_set_align_type(EGUI_VIEW_OF(&row->value), EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&row->value), EGUI_COLOR_HEX(0x526678), EGUI_ALPHA_100);
        egui_view_card_add_child(EGUI_VIEW_OF(&row->card), EGUI_VIEW_OF(&row->value));

        egui_view_slider_init(EGUI_VIEW_OF(&row->slider));
        egui_view_set_position(EGUI_VIEW_OF(&row->slider), 10, 24);
        egui_view_set_size(EGUI_VIEW_OF(&row->slider), BASIC_VIEWPORT_W - BASIC_ROW_SIDE_INSET * 2 - 20, 16);
        egui_view_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&row->slider), basic_slider_value_changed_cb);
        row->slider.track_color = EGUI_COLOR_HEX(0xD5E1EA);
        row->slider.active_color = EGUI_COLOR_HEX(0x3A6EA5);
        row->slider.thumb_color = EGUI_COLOR_HEX(0x2F5E8A);
        egui_view_card_add_child(EGUI_VIEW_OF(&row->card), EGUI_VIEW_OF(&row->slider));

        return EGUI_VIEW_OF(&row->root);
    }

    {
        basic_button_row_t *row = (basic_button_row_t *)egui_malloc(sizeof(basic_button_row_t));
        if (row == NULL)
        {
            return NULL;
        }

        memset(row, 0, sizeof(*row));
        egui_view_group_init(EGUI_VIEW_OF(&row->root));
        egui_view_button_init(EGUI_VIEW_OF(&row->button));
        egui_view_set_position(EGUI_VIEW_OF(&row->button), BASIC_ROW_SIDE_INSET, 4);
        egui_view_set_size(EGUI_VIEW_OF(&row->button), BASIC_VIEWPORT_W - BASIC_ROW_SIDE_INSET * 2, BASIC_BUTTON_INNER_H);
        egui_view_label_set_font(EGUI_VIEW_OF(&row->button), BASIC_FONT_BODY);
        egui_view_label_set_align_type(EGUI_VIEW_OF(&row->button), EGUI_ALIGN_CENTER);
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&row->button), basic_button_click_cb);
        egui_view_group_add_child(EGUI_VIEW_OF(&row->root), EGUI_VIEW_OF(&row->button));
        return EGUI_VIEW_OF(&row->root);
    }
}

static void basic_adapter_destroy_view(void *adapter_context, egui_view_t *view, uint16_t view_type)
{
    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(view_type);
    egui_free(view);
}

static void basic_adapter_bind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    basic_viewport_context_t *ctx = (basic_viewport_context_t *)adapter_context;
    const basic_item_t *item = index < BASIC_VIEWPORT_ITEM_COUNT ? &ctx->items[index] : NULL;

    if (item == NULL)
    {
        return;
    }

    if (item->kind == BASIC_ITEM_KIND_SLIDER)
    {
        basic_slider_row_t *row = (basic_slider_row_t *)view;
        row->stable_id = stable_id;
        basic_bind_slider_row(row, index, item);
    }
    else
    {
        basic_button_row_t *row = (basic_button_row_t *)view;
        row->stable_id = stable_id;
        basic_bind_button_row(row, index, item);
    }
}

static uint8_t basic_adapter_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(view);

    return stable_id == basic_ctx.selected_id;
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

static uint8_t basic_is_view_clickable(egui_view_t *view)
{
    int click_x;
    int click_y;

    return (uint8_t)egui_sim_get_view_clipped_center(view, &EGUI_VIEW_OF(&viewport_view)->region_screen, &click_x, &click_y);
}

static uint8_t basic_set_click_item_action(egui_sim_action_t *p_action, egui_view_t *view, uint32_t interval_ms)
{
    return (uint8_t)egui_sim_set_click_view_clipped(p_action, view, &EGUI_VIEW_OF(&viewport_view)->region_screen, (int)interval_ms);
}

static egui_view_t *basic_find_visible_view_by_index(uint32_t index)
{
    uint32_t stable_id;
    const egui_view_virtual_viewport_slot_t *slot;
    egui_view_t *view;

    if (index >= BASIC_VIEWPORT_ITEM_COUNT)
    {
        return NULL;
    }

    stable_id = basic_ctx.items[index].stable_id;
    slot = egui_view_virtual_viewport_find_slot_by_stable_id(EGUI_VIEW_OF(&viewport_view), stable_id);
    if (slot == NULL)
    {
        return NULL;
    }

    view = egui_view_virtual_viewport_find_view_by_stable_id(EGUI_VIEW_OF(&viewport_view), stable_id);
    return basic_is_view_clickable(view) ? view : NULL;
}

static void basic_set_slider_drag_action(egui_sim_action_t *p_action, uint32_t index, uint32_t interval_ms)
{
    basic_slider_row_t *row = (basic_slider_row_t *)basic_find_visible_view_by_index(index);

    if (row == NULL)
    {
        EGUI_SIM_SET_WAIT(p_action, interval_ms);
        return;
    }

    p_action->type = EGUI_SIM_ACTION_DRAG;
    egui_sim_get_view_pos(EGUI_VIEW_OF(&row->slider), 0.15f, 0.5f, &p_action->x1, &p_action->y1);
    egui_sim_get_view_pos(EGUI_VIEW_OF(&row->slider), 0.85f, 0.5f, &p_action->x2, &p_action->y2);
    p_action->steps = 8;
    p_action->interval_ms = interval_ms;
}

static void basic_set_scroll_action(egui_sim_action_t *p_action, uint32_t interval_ms)
{
    egui_region_t *region = &EGUI_VIEW_OF(&viewport_view)->region_screen;

    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = (egui_dim_t)(region->location.x + region->size.width / 2);
    p_action->y1 = (egui_dim_t)(region->location.y + region->size.height - 26);
    p_action->x2 = p_action->x1;
    p_action->y2 = (egui_dim_t)(region->location.y + 54);
    p_action->steps = 10;
    p_action->interval_ms = interval_ms;
}

static uint32_t basic_get_first_visible_index(void)
{
    basic_visible_summary_t summary;

    memset(&summary, 0, sizeof(summary));
    egui_view_virtual_viewport_visit_visible_items(EGUI_VIEW_OF(&viewport_view), basic_visible_item_visitor, &summary);
    return summary.has_first ? summary.first_index : BASIC_INVALID_INDEX;
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
        if (first_call)
        {
            if (egui_view_virtual_viewport_get_slot_count(EGUI_VIEW_OF(&viewport_view)) == 0U)
            {
                report_runtime_failure("viewport basic should materialize initial slots");
            }
            if (egui_view_virtual_viewport_get_slot_count(EGUI_VIEW_OF(&viewport_view)) > EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
            {
                report_runtime_failure("viewport basic exceeded slot capacity");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        view = basic_find_visible_view_by_index(0);
        if (view == NULL)
        {
            report_runtime_failure("first button row was not visible");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (!basic_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("first button row click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
        }
        return true;
    case 2:
        if (first_call && basic_ctx.selected_id != basic_ctx.items[0].stable_id)
        {
            report_runtime_failure("button row click did not update selected item");
        }
        basic_set_slider_drag_action(p_action, 1, 260);
        return true;
    case 3:
        if (first_call && basic_ctx.items[1].value <= 38U)
        {
            report_runtime_failure("slider drag did not update item value");
        }
        basic_set_scroll_action(p_action, 360);
        return true;
    case 4:
        if (first_call && basic_get_first_visible_index() == BASIC_INVALID_INDEX)
        {
            report_runtime_failure("scroll did not keep visible items");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[BASIC_ACTION_PATCH]), 220);
        return true;
    case 5:
        if (first_call && basic_ctx.patch_count == 0U)
        {
            report_runtime_failure("patch action did not mutate selected item");
        }
        recording_jump_verify_retry = 0U;
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[BASIC_ACTION_JUMP]), 220);
        return true;
    case 6:
        view = basic_find_visible_view_by_index(basic_ctx.jump_cursor);
        if (basic_ctx.selected_id != basic_ctx.items[basic_ctx.jump_cursor].stable_id)
        {
            if (recording_jump_verify_retry < BASIC_JUMP_VERIFY_RETRY_MAX)
            {
                recording_jump_verify_retry++;
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            report_runtime_failure("jump action did not select target item");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        recording_jump_verify_retry = 0U;
        if (view != NULL && !basic_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("jump target item click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 7:
        recording_reset_verify_retry = 0U;
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[BASIC_ACTION_RESET]), 220);
        return true;
    case 8:
        if (basic_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID || egui_view_virtual_viewport_get_logical_offset(EGUI_VIEW_OF(&viewport_view)) != 0)
        {
            if (recording_reset_verify_retry < BASIC_RESET_VERIFY_RETRY_MAX)
            {
                recording_reset_verify_retry++;
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            if (basic_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
            {
                report_runtime_failure("reset action did not clear selected item");
            }
            if (egui_view_virtual_viewport_get_logical_offset(EGUI_VIEW_OF(&viewport_view)) != 0)
            {
                report_runtime_failure("reset action did not restore top position");
            }
        }
        recording_reset_verify_retry = 0U;
        if (first_call)
        {
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
    egui_view_virtual_viewport_setup_t setup = {
            .params = &basic_viewport_params,
            .adapter = &basic_viewport_adapter,
            .adapter_context = &basic_ctx,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };

    memset(&basic_ctx, 0, sizeof(basic_ctx));
    basic_init_items();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
    recording_jump_verify_retry = 0U;
    recording_reset_verify_retry = 0U;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&basic_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &basic_toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD1DEE7));

    for (i = 0; i < BASIC_ACTION_COUNT; i++)
    {
        basic_style_action_button(&action_buttons[i], i);
        egui_view_set_position(EGUI_VIEW_OF(&action_buttons[i]), 10 + i * (BASIC_ACTION_W + BASIC_ACTION_GAP), 6);
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
    }

    egui_view_virtual_viewport_init_with_setup(EGUI_VIEW_OF(&viewport_view), &setup);
    egui_view_set_background(EGUI_VIEW_OF(&viewport_view), EGUI_BG_OF(&basic_viewport_bg));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewport_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}
