#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "uicode.h"

#define LIST_VIEW_DEMO_MAX_ITEMS         32U
#define LIST_VIEW_DEMO_MODE_COUNT        3U
#define LIST_VIEW_DEMO_STATE_CACHE_COUNT 24U
#define LIST_VIEW_DEMO_TITLE_LEN         32
#define LIST_VIEW_DEMO_DETAIL_LEN        48
#define LIST_VIEW_DEMO_META_LEN          48
#define LIST_VIEW_DEMO_PREVIEW_LEN       20
#define LIST_VIEW_DEMO_HEADER_LEN        112

#define LIST_VIEW_DEMO_VIEWTYPE_HERO    1U
#define LIST_VIEW_DEMO_VIEWTYPE_COMPACT 2U

#define LIST_VIEW_DEMO_HERO_HEIGHT             106
#define LIST_VIEW_DEMO_HERO_EXPANDED_HEIGHT    126
#define LIST_VIEW_DEMO_COMPACT_HEIGHT          82
#define LIST_VIEW_DEMO_COMPACT_EXPANDED_HEIGHT 98

#define LIST_VIEW_DEMO_MARGIN_X  8
#define LIST_VIEW_DEMO_TOP_Y     8
#define LIST_VIEW_DEMO_CONTENT_W (EGUI_CONFIG_SCEEN_WIDTH - LIST_VIEW_DEMO_MARGIN_X * 2)
#define LIST_VIEW_DEMO_HEADER_H  66
#define LIST_VIEW_DEMO_TOOLBAR_Y (LIST_VIEW_DEMO_TOP_Y + LIST_VIEW_DEMO_HEADER_H + 6)
#define LIST_VIEW_DEMO_TOOLBAR_H 34
#define LIST_VIEW_DEMO_LIST_Y    (LIST_VIEW_DEMO_TOOLBAR_Y + LIST_VIEW_DEMO_TOOLBAR_H + 6)
#define LIST_VIEW_DEMO_LIST_H    (EGUI_CONFIG_SCEEN_HEIGHT - LIST_VIEW_DEMO_LIST_Y - 8)

#define LIST_VIEW_DEMO_ACTION_GAP 4
#define LIST_VIEW_DEMO_ACTION_W   ((LIST_VIEW_DEMO_CONTENT_W - 20 - LIST_VIEW_DEMO_ACTION_GAP * 4) / 5)
#define LIST_VIEW_DEMO_ACTION_H   20

#define LIST_VIEW_DEMO_FONT_HEADER ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define LIST_VIEW_DEMO_FONT_BODY   ((const egui_font_t *)&egui_res_font_montserrat_8_4)

typedef struct list_view_demo_item list_view_demo_item_t;
typedef struct list_view_demo_context list_view_demo_context_t;
typedef struct list_view_demo_hero_holder list_view_demo_hero_holder_t;
typedef struct list_view_demo_compact_holder list_view_demo_compact_holder_t;
typedef struct list_view_demo_holder_state list_view_demo_holder_state_t;

struct list_view_demo_item
{
    uint32_t stable_id;
    uint8_t view_type;
    uint8_t enabled;
    uint8_t mode_index;
    uint8_t expanded;
    uint8_t progress;
    uint8_t revision;
};

struct list_view_demo_context
{
    list_view_demo_item_t items[LIST_VIEW_DEMO_MAX_ITEMS];
    uint32_t item_count;
    uint32_t next_stable_id;
    uint32_t selected_id;
    uint32_t mutation_cursor;
    uint16_t switch_count;
    uint16_t toggle_count;
    uint16_t combo_count;
    uint16_t preview_count;
    uint16_t action_count;
    char header_title[LIST_VIEW_DEMO_HEADER_LEN];
    char header_detail[LIST_VIEW_DEMO_HEADER_LEN];
    char header_hint[LIST_VIEW_DEMO_HEADER_LEN];
};

struct list_view_demo_holder_state
{
    uint8_t preview_hits;
};

struct list_view_demo_hero_holder
{
    egui_view_list_view_holder_t base;
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t detail;
    egui_view_label_t meta;
    egui_view_label_t preview_label;
    egui_view_progress_bar_t progress_bar;
    egui_view_switch_t enabled_switch;
    egui_view_combobox_t mode_combo;
    egui_view_button_t preview_button;
    char title_text[LIST_VIEW_DEMO_TITLE_LEN];
    char detail_text[LIST_VIEW_DEMO_DETAIL_LEN];
    char meta_text[LIST_VIEW_DEMO_META_LEN];
    char preview_text[LIST_VIEW_DEMO_PREVIEW_LEN];
    uint8_t preview_hits;
};

struct list_view_demo_compact_holder
{
    egui_view_list_view_holder_t base;
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t detail;
    egui_view_label_t meta;
    egui_view_label_t preview_label;
    egui_view_progress_bar_t progress_bar;
    egui_view_toggle_button_t arm_toggle;
    egui_view_button_t preview_button;
    char title_text[LIST_VIEW_DEMO_TITLE_LEN];
    char detail_text[LIST_VIEW_DEMO_DETAIL_LEN];
    char meta_text[LIST_VIEW_DEMO_META_LEN];
    char preview_text[LIST_VIEW_DEMO_PREVIEW_LEN];
    uint8_t preview_hits;
};

static const char *list_view_demo_mode_items[LIST_VIEW_DEMO_MODE_COUNT] = {"Auto", "Eco", "Boost"};
static const char *list_view_demo_action_names[5] = {"Add", "Del", "Move", "Patch", "Jump"};

static egui_view_t background_view;
static egui_view_card_t header_card;
static egui_view_card_t toolbar_card;
static egui_view_label_t header_title;
static egui_view_label_t header_detail;
static egui_view_label_t header_hint;
static egui_view_button_t action_buttons[5];
static egui_view_list_view_t list_view;
static list_view_demo_context_t list_view_demo_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(list_view_demo_header_card_params, LIST_VIEW_DEMO_MARGIN_X, LIST_VIEW_DEMO_TOP_Y, LIST_VIEW_DEMO_CONTENT_W, LIST_VIEW_DEMO_HEADER_H,
                           14);
EGUI_VIEW_CARD_PARAMS_INIT(list_view_demo_toolbar_card_params, LIST_VIEW_DEMO_MARGIN_X, LIST_VIEW_DEMO_TOOLBAR_Y, LIST_VIEW_DEMO_CONTENT_W,
                           LIST_VIEW_DEMO_TOOLBAR_H, 12);
EGUI_VIEW_SWITCH_PARAMS_INIT(list_view_demo_switch_params, 0, 0, 52, 28, 0);
EGUI_VIEW_COMBOBOX_PARAMS_INIT(list_view_demo_combo_params, 0, 0, 92, 28, list_view_demo_mode_items, LIST_VIEW_DEMO_MODE_COUNT, 0);
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(list_view_demo_toggle_params, 0, 0, 72, 26, "Arm", 0);

static const egui_view_list_view_params_t list_view_demo_params = {
        .region = {{LIST_VIEW_DEMO_MARGIN_X, LIST_VIEW_DEMO_LIST_Y}, {LIST_VIEW_DEMO_CONTENT_W, LIST_VIEW_DEMO_LIST_H}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 3,
        .estimated_item_height = LIST_VIEW_DEMO_HERO_HEIGHT,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(list_view_demo_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF4F7), EGUI_COLOR_HEX(0xD8E4EF),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(list_view_demo_screen_bg_params, &list_view_demo_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(list_view_demo_screen_bg, &list_view_demo_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(list_view_demo_list_bg_param, EGUI_COLOR_HEX(0xF8FBFD), EGUI_ALPHA_100, 14, 1, EGUI_COLOR_HEX(0xCBDAE6),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(list_view_demo_list_bg_params, &list_view_demo_list_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(list_view_demo_list_bg, &list_view_demo_list_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(list_view_demo_action_bg_param, EGUI_COLOR_HEX(0xE6EEF5), EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(list_view_demo_action_bg_params, &list_view_demo_action_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(list_view_demo_action_bg, &list_view_demo_action_bg_params);

static void list_view_demo_refresh_header(void);
static void list_view_demo_row_click_cb(egui_view_t *self);
static void list_view_demo_switch_checked_cb(egui_view_t *self, int is_checked);
static void list_view_demo_mode_selected_cb(egui_view_t *self, uint8_t index);
static void list_view_demo_toggle_cb(egui_view_t *self, uint8_t is_toggled);
static void list_view_demo_preview_click_cb(egui_view_t *self);
static void list_view_demo_action_click_cb(egui_view_t *self);

static list_view_demo_item_t *list_view_demo_get_item(uint32_t index)
{
    if (index >= list_view_demo_ctx.item_count)
    {
        return NULL;
    }

    return &list_view_demo_ctx.items[index];
}

static int32_t list_view_demo_find_index_by_stable_id_internal(uint32_t stable_id)
{
    uint32_t index;

    for (index = 0; index < list_view_demo_ctx.item_count; index++)
    {
        if (list_view_demo_ctx.items[index].stable_id == stable_id)
        {
            return (int32_t)index;
        }
    }

    return -1;
}

static list_view_demo_item_t *list_view_demo_find_item_by_stable_id_internal(uint32_t stable_id)
{
    int32_t index = list_view_demo_find_index_by_stable_id_internal(stable_id);
    return index >= 0 ? &list_view_demo_ctx.items[index] : NULL;
}

static uint32_t list_view_demo_count_enabled_items(void)
{
    uint32_t index;
    uint32_t enabled_count = 0;

    for (index = 0; index < list_view_demo_ctx.item_count; index++)
    {
        if (list_view_demo_ctx.items[index].enabled)
        {
            enabled_count++;
        }
    }

    return enabled_count;
}

static void list_view_demo_fill_item(list_view_demo_item_t *item, uint8_t view_type, uint32_t seed)
{
    memset(item, 0, sizeof(*item));
    item->stable_id = list_view_demo_ctx.next_stable_id++;
    item->view_type = view_type;
    item->enabled = (uint8_t)((seed % 3U) == 0U);
    item->mode_index = (uint8_t)(seed % LIST_VIEW_DEMO_MODE_COUNT);
    item->expanded = (uint8_t)((seed % 5U) == 0U);
    item->progress = (uint8_t)(18U + ((seed * 13U) % 74U));
    item->revision = (uint8_t)(1U + (seed % 7U));
}

static void list_view_demo_reset_model(void)
{
    uint32_t index;

    memset(&list_view_demo_ctx, 0, sizeof(list_view_demo_ctx));
    list_view_demo_ctx.item_count = 24U;
    list_view_demo_ctx.next_stable_id = 30001U;
    list_view_demo_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    list_view_demo_ctx.mutation_cursor = 0U;

    for (index = 0; index < list_view_demo_ctx.item_count; index++)
    {
        list_view_demo_fill_item(&list_view_demo_ctx.items[index],
                                 (uint8_t)((index % 4U) == 0U ? LIST_VIEW_DEMO_VIEWTYPE_HERO : LIST_VIEW_DEMO_VIEWTYPE_COMPACT), index + 3U);
    }
}

static void list_view_demo_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const egui_font_t *font,
                                      uint8_t align, egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), width, height);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), align);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static void list_view_demo_init_button(egui_view_button_t *button, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const char *text,
                                       egui_view_on_click_listener_t listener)
{
    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_position(EGUI_VIEW_OF(button), x, y);
    egui_view_set_size(EGUI_VIEW_OF(button), width, height);
    egui_view_label_set_text(EGUI_VIEW_OF(button), text);
    egui_view_label_set_font(EGUI_VIEW_OF(button), LIST_VIEW_DEMO_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x274056), EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(button), EGUI_BG_OF(&list_view_demo_action_bg));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), listener);
}

static int32_t list_view_demo_measure_item_height_from_item(const list_view_demo_item_t *item)
{
    if (item == NULL)
    {
        return LIST_VIEW_DEMO_COMPACT_HEIGHT;
    }

    if (item->view_type == LIST_VIEW_DEMO_VIEWTYPE_HERO)
    {
        return item->expanded ? LIST_VIEW_DEMO_HERO_EXPANDED_HEIGHT : LIST_VIEW_DEMO_HERO_HEIGHT;
    }

    return item->expanded ? LIST_VIEW_DEMO_COMPACT_EXPANDED_HEIGHT : LIST_VIEW_DEMO_COMPACT_HEIGHT;
}

static void list_view_demo_set_selected(uint32_t stable_id, uint8_t ensure_visible)
{
    uint32_t previous_id = list_view_demo_ctx.selected_id;

    if (previous_id == stable_id)
    {
        if (ensure_visible)
        {
            egui_view_list_view_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&list_view), stable_id, 6);
        }
        return;
    }

    list_view_demo_ctx.selected_id = stable_id;
    if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_list_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&list_view), previous_id);
    }
    if (stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_list_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&list_view), stable_id);
        if (ensure_visible)
        {
            egui_view_list_view_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&list_view), stable_id, 6);
        }
    }

    list_view_demo_refresh_header();
}

static uint32_t list_view_demo_pick_target_index(void)
{
    int32_t selected_index = list_view_demo_find_index_by_stable_id_internal(list_view_demo_ctx.selected_id);

    if (selected_index >= 0)
    {
        return (uint32_t)selected_index;
    }

    if (list_view_demo_ctx.item_count == 0U)
    {
        return 0U;
    }

    if (list_view_demo_ctx.mutation_cursor >= list_view_demo_ctx.item_count)
    {
        list_view_demo_ctx.mutation_cursor = 0U;
    }

    return list_view_demo_ctx.mutation_cursor;
}

static void list_view_demo_set_preview_text(char *buffer, uint16_t capacity, uint8_t preview_hits)
{
    snprintf(buffer, capacity, "Peek %u", (unsigned)preview_hits);
}

static void list_view_demo_set_hero_preview_hits(list_view_demo_hero_holder_t *holder, uint8_t preview_hits)
{
    holder->preview_hits = preview_hits;
    list_view_demo_set_preview_text(holder->preview_text, sizeof(holder->preview_text), preview_hits);
    egui_view_label_set_text(EGUI_VIEW_OF(&holder->preview_label), holder->preview_text);
}

static void list_view_demo_set_compact_preview_hits(list_view_demo_compact_holder_t *holder, uint8_t preview_hits)
{
    holder->preview_hits = preview_hits;
    list_view_demo_set_preview_text(holder->preview_text, sizeof(holder->preview_text), preview_hits);
    egui_view_label_set_text(EGUI_VIEW_OF(&holder->preview_label), holder->preview_text);
}

static void list_view_demo_sync_switch(list_view_demo_hero_holder_t *holder, uint8_t is_checked)
{
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&holder->enabled_switch), NULL);
    egui_view_switch_set_checked(EGUI_VIEW_OF(&holder->enabled_switch), is_checked);
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&holder->enabled_switch), list_view_demo_switch_checked_cb);
}

static void list_view_demo_sync_toggle(list_view_demo_compact_holder_t *holder, uint8_t is_toggled)
{
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&holder->arm_toggle), NULL);
    egui_view_toggle_button_set_toggled(EGUI_VIEW_OF(&holder->arm_toggle), is_toggled);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&holder->arm_toggle), list_view_demo_toggle_cb);
}

static void list_view_demo_refresh_header(void)
{
    snprintf(list_view_demo_ctx.header_title, sizeof(list_view_demo_ctx.header_title), "list_view  rows %u  enabled %lu",
             (unsigned)list_view_demo_ctx.item_count, (unsigned long)list_view_demo_count_enabled_items());
    snprintf(list_view_demo_ctx.header_detail, sizeof(list_view_demo_ctx.header_detail), "switch %u  toggle %u  combo %u  preview %u",
             (unsigned)list_view_demo_ctx.switch_count, (unsigned)list_view_demo_ctx.toggle_count, (unsigned)list_view_demo_ctx.combo_count,
             (unsigned)list_view_demo_ctx.preview_count);
    snprintf(list_view_demo_ctx.header_hint, sizeof(list_view_demo_ctx.header_hint), "selected %lu  actions %u  state-cache keeps preview hits",
             (unsigned long)list_view_demo_ctx.selected_id, (unsigned)list_view_demo_ctx.action_count);

    egui_view_label_set_text(EGUI_VIEW_OF(&header_title), list_view_demo_ctx.header_title);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_detail), list_view_demo_ctx.header_detail);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_hint), list_view_demo_ctx.header_hint);
}

static uint32_t list_view_demo_get_count(void *data_model_context)
{
    return ((list_view_demo_context_t *)data_model_context)->item_count;
}

static uint32_t list_view_demo_get_stable_id(void *data_model_context, uint32_t index)
{
    list_view_demo_context_t *context = (list_view_demo_context_t *)data_model_context;
    return index < context->item_count ? context->items[index].stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t list_view_demo_find_index_by_stable_id(void *data_model_context, uint32_t stable_id)
{
    list_view_demo_context_t *context = (list_view_demo_context_t *)data_model_context;
    uint32_t index;

    for (index = 0; index < context->item_count; index++)
    {
        if (context->items[index].stable_id == stable_id)
        {
            return (int32_t)index;
        }
    }

    return -1;
}

static uint16_t list_view_demo_get_view_type(void *data_model_context, uint32_t index)
{
    list_view_demo_context_t *context = (list_view_demo_context_t *)data_model_context;
    return index < context->item_count ? context->items[index].view_type : LIST_VIEW_DEMO_VIEWTYPE_COMPACT;
}

static int32_t list_view_demo_measure_item_height(void *data_model_context, uint32_t index, int32_t width_hint)
{
    list_view_demo_context_t *context = (list_view_demo_context_t *)data_model_context;

    EGUI_UNUSED(width_hint);

    if (index >= context->item_count)
    {
        return LIST_VIEW_DEMO_COMPACT_HEIGHT;
    }

    return list_view_demo_measure_item_height_from_item(&context->items[index]);
}

static egui_view_list_view_holder_t *list_view_demo_create_holder(void *data_model_context, uint16_t view_type)
{
    EGUI_UNUSED(data_model_context);

    if (view_type == LIST_VIEW_DEMO_VIEWTYPE_HERO)
    {
        list_view_demo_hero_holder_t *holder = (list_view_demo_hero_holder_t *)egui_malloc(sizeof(list_view_demo_hero_holder_t));

        if (holder == NULL)
        {
            return NULL;
        }

        memset(holder, 0, sizeof(*holder));
        egui_view_group_init(EGUI_VIEW_OF(&holder->root));

        egui_view_card_init(EGUI_VIEW_OF(&holder->card));
        egui_view_set_position(EGUI_VIEW_OF(&holder->card), 4, 4);
        egui_view_group_add_child(EGUI_VIEW_OF(&holder->root), EGUI_VIEW_OF(&holder->card));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&holder->card), list_view_demo_row_click_cb);

        list_view_demo_init_label(&holder->title, 12, 10, 160, 14, LIST_VIEW_DEMO_FONT_HEADER, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x223649));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->title));

        list_view_demo_init_label(&holder->detail, 12, 28, 180, 12, LIST_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x52687A));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->detail));

        list_view_demo_init_label(&holder->meta, 12, 44, 180, 12, LIST_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x6A7B89));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->meta));

        egui_view_progress_bar_init(EGUI_VIEW_OF(&holder->progress_bar));
        holder->progress_bar.is_show_control = 0;
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->progress_bar));

        egui_view_switch_init_with_params(EGUI_VIEW_OF(&holder->enabled_switch), &list_view_demo_switch_params);
        egui_view_switch_set_state_icons(EGUI_VIEW_OF(&holder->enabled_switch), EGUI_ICON_MS_DONE, EGUI_ICON_MS_CLOSE);
        egui_view_switch_set_icon_font(EGUI_VIEW_OF(&holder->enabled_switch), EGUI_FONT_ICON_MS_16);
        egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&holder->enabled_switch), list_view_demo_switch_checked_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->enabled_switch));

        egui_view_combobox_init_with_params(EGUI_VIEW_OF(&holder->mode_combo), &list_view_demo_combo_params);
        egui_view_combobox_set_font(EGUI_VIEW_OF(&holder->mode_combo), LIST_VIEW_DEMO_FONT_BODY);
        egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&holder->mode_combo), 3);
        egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&holder->mode_combo), EGUI_ICON_MS_EXPAND_MORE, EGUI_ICON_MS_EXPAND_LESS);
        egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&holder->mode_combo), list_view_demo_mode_selected_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->mode_combo));

        list_view_demo_init_button(&holder->preview_button, 0, 0, 50, 22, "Peek", list_view_demo_preview_click_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->preview_button));

        list_view_demo_init_label(&holder->preview_label, 0, 0, 42, 12, LIST_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER,
                                  EGUI_COLOR_HEX(0x36556F));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->preview_label));
        list_view_demo_set_hero_preview_hits(holder, 0U);

        holder->base.item_view = EGUI_VIEW_OF(&holder->root);
        return &holder->base;
    }
    else
    {
        list_view_demo_compact_holder_t *holder = (list_view_demo_compact_holder_t *)egui_malloc(sizeof(list_view_demo_compact_holder_t));

        if (holder == NULL)
        {
            return NULL;
        }

        memset(holder, 0, sizeof(*holder));
        egui_view_group_init(EGUI_VIEW_OF(&holder->root));

        egui_view_card_init(EGUI_VIEW_OF(&holder->card));
        egui_view_set_position(EGUI_VIEW_OF(&holder->card), 4, 4);
        egui_view_group_add_child(EGUI_VIEW_OF(&holder->root), EGUI_VIEW_OF(&holder->card));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&holder->card), list_view_demo_row_click_cb);

        list_view_demo_init_label(&holder->title, 12, 10, 150, 14, LIST_VIEW_DEMO_FONT_HEADER, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x223649));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->title));

        list_view_demo_init_label(&holder->detail, 12, 28, 170, 12, LIST_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x52687A));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->detail));

        list_view_demo_init_label(&holder->meta, 12, 44, 170, 12, LIST_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x6A7B89));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->meta));

        egui_view_progress_bar_init(EGUI_VIEW_OF(&holder->progress_bar));
        holder->progress_bar.is_show_control = 0;
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->progress_bar));

        egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&holder->arm_toggle), &list_view_demo_toggle_params);
        egui_view_toggle_button_set_icon(EGUI_VIEW_OF(&holder->arm_toggle), EGUI_ICON_MS_VISIBILITY);
        egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&holder->arm_toggle), EGUI_FONT_ICON_MS_16);
        egui_view_toggle_button_set_icon_text_gap(EGUI_VIEW_OF(&holder->arm_toggle), 4);
        egui_view_toggle_button_set_text_color(EGUI_VIEW_OF(&holder->arm_toggle), EGUI_COLOR_WHITE);
        egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&holder->arm_toggle), list_view_demo_toggle_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->arm_toggle));

        list_view_demo_init_button(&holder->preview_button, 0, 0, 50, 22, "Peek", list_view_demo_preview_click_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->preview_button));

        list_view_demo_init_label(&holder->preview_label, 0, 0, 42, 12, LIST_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER,
                                  EGUI_COLOR_HEX(0x36556F));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->preview_label));
        list_view_demo_set_compact_preview_hits(holder, 0U);

        holder->base.item_view = EGUI_VIEW_OF(&holder->root);
        return &holder->base;
    }
}

static void list_view_demo_destroy_holder(void *data_model_context, egui_view_list_view_holder_t *holder, uint16_t view_type)
{
    EGUI_UNUSED(data_model_context);
    EGUI_UNUSED(view_type);
    egui_free(holder);
}

static void list_view_demo_bind_hero_holder(list_view_demo_hero_holder_t *holder, const list_view_demo_item_t *item, uint32_t index, uint8_t selected)
{
    egui_dim_t root_w = holder->base.host_view->region.size.width;
    egui_dim_t root_h = holder->base.host_view->region.size.height;
    egui_dim_t card_w = root_w - 8;
    egui_dim_t card_h = root_h - 8;

    egui_view_set_size(EGUI_VIEW_OF(&holder->root), root_w, root_h);
    egui_view_set_size(EGUI_VIEW_OF(&holder->card), card_w, card_h);

    snprintf(holder->title_text, sizeof(holder->title_text), "Hero %02lu  %s", (unsigned long)(index + 1U), item->enabled ? "Live" : "Idle");
    snprintf(holder->detail_text, sizeof(holder->detail_text), "Mode %s  progress %u%%", list_view_demo_mode_items[item->mode_index], (unsigned)item->progress);
    snprintf(holder->meta_text, sizeof(holder->meta_text), "id %05lu  rev %u  %s", (unsigned long)item->stable_id, (unsigned)item->revision,
             item->expanded ? "expanded" : "compact");

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&holder->card), selected ? EGUI_COLOR_HEX(0xE6F2FF) : EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&holder->card), 1, selected ? EGUI_COLOR_HEX(0x6DA1D4) : EGUI_COLOR_HEX(0xCBD8E3));

    egui_view_label_set_text(EGUI_VIEW_OF(&holder->title), holder->title_text);
    egui_view_set_size(EGUI_VIEW_OF(&holder->title), card_w - 24, 14);

    egui_view_label_set_text(EGUI_VIEW_OF(&holder->detail), holder->detail_text);
    egui_view_set_size(EGUI_VIEW_OF(&holder->detail), card_w - 24, 12);

    egui_view_label_set_text(EGUI_VIEW_OF(&holder->meta), holder->meta_text);
    egui_view_set_size(EGUI_VIEW_OF(&holder->meta), card_w - 24, 12);

    egui_view_set_position(EGUI_VIEW_OF(&holder->enabled_switch), card_w - 64, 12);
    egui_view_set_size(EGUI_VIEW_OF(&holder->enabled_switch), 52, 28);
    list_view_demo_sync_switch(holder, item->enabled);

    egui_view_set_position(EGUI_VIEW_OF(&holder->mode_combo), 12, 62);
    egui_view_set_size(EGUI_VIEW_OF(&holder->mode_combo), card_w - 92, 28);
    egui_view_combobox_set_current_index(EGUI_VIEW_OF(&holder->mode_combo), item->mode_index);

    egui_view_set_position(EGUI_VIEW_OF(&holder->progress_bar), 12, card_h - 28);
    egui_view_set_size(EGUI_VIEW_OF(&holder->progress_bar), card_w - 74, 6);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&holder->progress_bar), item->progress);
    holder->progress_bar.progress_color = item->enabled ? EGUI_COLOR_HEX(0x32739B) : EGUI_COLOR_HEX(0x7DA0B6);
    holder->progress_bar.bk_color = EGUI_COLOR_HEX(0xD7E4EC);

    egui_view_set_position(EGUI_VIEW_OF(&holder->preview_button), card_w - 60, card_h - 38);
    egui_view_set_size(EGUI_VIEW_OF(&holder->preview_button), 48, 22);
    egui_view_set_position(EGUI_VIEW_OF(&holder->preview_label), card_w - 64, card_h - 56);
    egui_view_set_size(EGUI_VIEW_OF(&holder->preview_label), 52, 12);
}

static void list_view_demo_bind_compact_holder(list_view_demo_compact_holder_t *holder, const list_view_demo_item_t *item, uint32_t index, uint8_t selected)
{
    egui_dim_t root_w = holder->base.host_view->region.size.width;
    egui_dim_t root_h = holder->base.host_view->region.size.height;
    egui_dim_t card_w = root_w - 8;
    egui_dim_t card_h = root_h - 8;

    egui_view_set_size(EGUI_VIEW_OF(&holder->root), root_w, root_h);
    egui_view_set_size(EGUI_VIEW_OF(&holder->card), card_w, card_h);

    snprintf(holder->title_text, sizeof(holder->title_text), "Compact %02lu", (unsigned long)(index + 1U));
    snprintf(holder->detail_text, sizeof(holder->detail_text), "%s  progress %u%%", item->enabled ? "armed" : "standby", (unsigned)item->progress);
    snprintf(holder->meta_text, sizeof(holder->meta_text), "id %05lu  rev %u  %s", (unsigned long)item->stable_id, (unsigned)item->revision,
             item->expanded ? "expanded" : "compact");

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&holder->card), selected ? EGUI_COLOR_HEX(0xEEF7EF) : EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&holder->card), 1, selected ? EGUI_COLOR_HEX(0x5B9A6B) : EGUI_COLOR_HEX(0xCBD8E3));

    egui_view_label_set_text(EGUI_VIEW_OF(&holder->title), holder->title_text);
    egui_view_set_size(EGUI_VIEW_OF(&holder->title), card_w - 24, 14);

    egui_view_label_set_text(EGUI_VIEW_OF(&holder->detail), holder->detail_text);
    egui_view_set_size(EGUI_VIEW_OF(&holder->detail), card_w - 24, 12);

    egui_view_label_set_text(EGUI_VIEW_OF(&holder->meta), holder->meta_text);
    egui_view_set_size(EGUI_VIEW_OF(&holder->meta), card_w - 24, 12);

    egui_view_set_position(EGUI_VIEW_OF(&holder->arm_toggle), card_w - 84, 10);
    egui_view_set_size(EGUI_VIEW_OF(&holder->arm_toggle), 72, 26);
    list_view_demo_sync_toggle(holder, item->enabled);

    egui_view_set_position(EGUI_VIEW_OF(&holder->progress_bar), 12, card_h - 28);
    egui_view_set_size(EGUI_VIEW_OF(&holder->progress_bar), card_w - 74, 6);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&holder->progress_bar), item->progress);
    holder->progress_bar.progress_color = item->enabled ? EGUI_COLOR_HEX(0x4C8A63) : EGUI_COLOR_HEX(0x8CA3B5);
    holder->progress_bar.bk_color = EGUI_COLOR_HEX(0xD7E4EC);

    egui_view_set_position(EGUI_VIEW_OF(&holder->preview_button), card_w - 60, card_h - 38);
    egui_view_set_size(EGUI_VIEW_OF(&holder->preview_button), 48, 22);
    egui_view_set_position(EGUI_VIEW_OF(&holder->preview_label), card_w - 64, card_h - 56);
    egui_view_set_size(EGUI_VIEW_OF(&holder->preview_label), 52, 12);
}

static void list_view_demo_bind_holder(void *data_model_context, egui_view_list_view_holder_t *holder, uint32_t index, uint32_t stable_id)
{
    list_view_demo_context_t *context = (list_view_demo_context_t *)data_model_context;
    const list_view_demo_item_t *item;
    uint8_t selected;

    EGUI_UNUSED(stable_id);

    if (index >= context->item_count)
    {
        return;
    }

    item = &context->items[index];
    selected = item->stable_id == context->selected_id ? 1U : 0U;

    if (item->view_type == LIST_VIEW_DEMO_VIEWTYPE_HERO)
    {
        list_view_demo_bind_hero_holder((list_view_demo_hero_holder_t *)holder, item, index, selected);
    }
    else
    {
        list_view_demo_bind_compact_holder((list_view_demo_compact_holder_t *)holder, item, index, selected);
    }
}

static uint8_t list_view_demo_should_keep_alive(void *data_model_context, egui_view_list_view_holder_t *holder, uint32_t stable_id)
{
    list_view_demo_context_t *context = (list_view_demo_context_t *)data_model_context;
    int32_t index;

    EGUI_UNUSED(holder);

    index = list_view_demo_find_index_by_stable_id(data_model_context, stable_id);
    if (index < 0)
    {
        return 0;
    }

    return (uint8_t)(context->items[index].view_type == LIST_VIEW_DEMO_VIEWTYPE_HERO && context->items[index].enabled &&
                     context->items[index].mode_index == 0U);
}

static void list_view_demo_save_holder_state(void *data_model_context, egui_view_list_view_holder_t *holder, uint32_t stable_id)
{
    list_view_demo_holder_state_t state;

    EGUI_UNUSED(data_model_context);

    memset(&state, 0, sizeof(state));
    if (holder->view_type == LIST_VIEW_DEMO_VIEWTYPE_HERO)
    {
        state.preview_hits = ((list_view_demo_hero_holder_t *)holder)->preview_hits;
    }
    else
    {
        state.preview_hits = ((list_view_demo_compact_holder_t *)holder)->preview_hits;
    }

    if (state.preview_hits == 0U)
    {
        egui_view_list_view_remove_item_state_by_stable_id(EGUI_VIEW_OF(&list_view), stable_id);
        return;
    }

    (void)egui_view_list_view_write_item_state_for_view(holder->item_view, stable_id, &state, sizeof(state));
}

static void list_view_demo_restore_holder_state(void *data_model_context, egui_view_list_view_holder_t *holder, uint32_t stable_id)
{
    list_view_demo_holder_state_t state;

    EGUI_UNUSED(data_model_context);

    memset(&state, 0, sizeof(state));
    if (egui_view_list_view_read_item_state_for_view(holder->item_view, stable_id, &state, sizeof(state)) != sizeof(state))
    {
        state.preview_hits = 0U;
    }

    if (holder->view_type == LIST_VIEW_DEMO_VIEWTYPE_HERO)
    {
        list_view_demo_set_hero_preview_hits((list_view_demo_hero_holder_t *)holder, state.preview_hits);
    }
    else
    {
        list_view_demo_set_compact_preview_hits((list_view_demo_compact_holder_t *)holder, state.preview_hits);
    }
}

static const egui_view_list_view_data_model_t list_view_demo_data_model = {
        .get_count = list_view_demo_get_count,
        .get_stable_id = list_view_demo_get_stable_id,
        .find_index_by_stable_id = list_view_demo_find_index_by_stable_id,
        .get_view_type = list_view_demo_get_view_type,
        .measure_item_height = list_view_demo_measure_item_height,
        .default_view_type = LIST_VIEW_DEMO_VIEWTYPE_COMPACT,
};

static const egui_view_list_view_holder_ops_t list_view_demo_holder_ops = {
        .create_holder = list_view_demo_create_holder,
        .destroy_holder = list_view_demo_destroy_holder,
        .bind_holder = list_view_demo_bind_holder,
        .unbind_holder = NULL,
        .should_keep_alive = list_view_demo_should_keep_alive,
        .save_holder_state = list_view_demo_save_holder_state,
        .restore_holder_state = list_view_demo_restore_holder_state,
};

static void list_view_demo_row_click_cb(egui_view_t *self)
{
    egui_view_list_view_entry_t entry;

    if (!egui_view_list_view_resolve_item_by_view(EGUI_VIEW_OF(&list_view), self, &entry))
    {
        return;
    }

    list_view_demo_set_selected(entry.stable_id, 0);
}

static void list_view_demo_switch_checked_cb(egui_view_t *self, int is_checked)
{
    egui_view_list_view_entry_t entry;
    list_view_demo_item_t *item;

    if (!egui_view_list_view_resolve_item_by_view(EGUI_VIEW_OF(&list_view), self, &entry))
    {
        return;
    }

    item = list_view_demo_get_item(entry.index);
    if (item == NULL)
    {
        return;
    }

    item->enabled = is_checked ? 1U : 0U;
    item->progress = (uint8_t)(item->enabled ? 72U : 36U);
    list_view_demo_ctx.switch_count++;
    list_view_demo_set_selected(entry.stable_id, 0);
    egui_view_list_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&list_view), entry.stable_id);
    list_view_demo_refresh_header();
}

static void list_view_demo_mode_selected_cb(egui_view_t *self, uint8_t index)
{
    egui_view_list_view_entry_t entry;
    list_view_demo_item_t *item;

    if (!egui_view_list_view_resolve_item_by_view(EGUI_VIEW_OF(&list_view), self, &entry))
    {
        return;
    }

    item = list_view_demo_get_item(entry.index);
    if (item == NULL)
    {
        return;
    }

    item->mode_index = index;
    item->progress = (uint8_t)(28U + index * 24U);
    item->revision++;
    list_view_demo_ctx.combo_count++;
    list_view_demo_set_selected(entry.stable_id, 0);
    egui_view_list_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&list_view), entry.stable_id);
    list_view_demo_refresh_header();
}

static void list_view_demo_toggle_cb(egui_view_t *self, uint8_t is_toggled)
{
    egui_view_list_view_entry_t entry;
    list_view_demo_item_t *item;

    if (!egui_view_list_view_resolve_item_by_view(EGUI_VIEW_OF(&list_view), self, &entry))
    {
        return;
    }

    item = list_view_demo_get_item(entry.index);
    if (item == NULL)
    {
        return;
    }

    item->enabled = is_toggled ? 1U : 0U;
    item->progress = (uint8_t)(item->enabled ? 66U : 24U);
    list_view_demo_ctx.toggle_count++;
    list_view_demo_set_selected(entry.stable_id, 0);
    egui_view_list_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&list_view), entry.stable_id);
    list_view_demo_refresh_header();
}

static void list_view_demo_preview_click_cb(egui_view_t *self)
{
    egui_view_list_view_holder_t *holder;
    egui_view_list_view_entry_t entry;

    if (!egui_view_list_view_resolve_holder_by_view(EGUI_VIEW_OF(&list_view), self, &holder, &entry))
    {
        return;
    }

    if (holder->view_type == LIST_VIEW_DEMO_VIEWTYPE_HERO)
    {
        list_view_demo_hero_holder_t *typed_holder = (list_view_demo_hero_holder_t *)holder;
        list_view_demo_set_hero_preview_hits(typed_holder, (uint8_t)(typed_holder->preview_hits + 1U));
    }
    else
    {
        list_view_demo_compact_holder_t *typed_holder = (list_view_demo_compact_holder_t *)holder;
        list_view_demo_set_compact_preview_hits(typed_holder, (uint8_t)(typed_holder->preview_hits + 1U));
    }

    list_view_demo_ctx.preview_count++;
    list_view_demo_set_selected(entry.stable_id, 0);
    list_view_demo_refresh_header();
}

static int list_view_demo_find_action_button_index(egui_view_t *self)
{
    uint8_t index;

    for (index = 0; index < 5U; index++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[index]))
        {
            return (int)index;
        }
    }

    return -1;
}

static void list_view_demo_action_add(void)
{
    uint32_t index;

    if (list_view_demo_ctx.item_count >= LIST_VIEW_DEMO_MAX_ITEMS)
    {
        list_view_demo_refresh_header();
        return;
    }

    index = list_view_demo_pick_target_index();
    if (list_view_demo_ctx.item_count > 0U)
    {
        index++;
    }
    if (index > list_view_demo_ctx.item_count)
    {
        index = list_view_demo_ctx.item_count;
    }

    if (index < list_view_demo_ctx.item_count)
    {
        memmove(&list_view_demo_ctx.items[index + 1U], &list_view_demo_ctx.items[index],
                (list_view_demo_ctx.item_count - index) * sizeof(list_view_demo_ctx.items[0]));
    }

    list_view_demo_fill_item(&list_view_demo_ctx.items[index], (uint8_t)((index & 1U) ? LIST_VIEW_DEMO_VIEWTYPE_HERO : LIST_VIEW_DEMO_VIEWTYPE_COMPACT),
                             list_view_demo_ctx.next_stable_id + index);
    list_view_demo_ctx.item_count++;
    list_view_demo_ctx.action_count++;
    list_view_demo_ctx.mutation_cursor = index;
    list_view_demo_set_selected(list_view_demo_ctx.items[index].stable_id, 1);
    egui_view_list_view_notify_item_inserted(EGUI_VIEW_OF(&list_view), index, 1);
    list_view_demo_refresh_header();
}

static void list_view_demo_action_remove(void)
{
    uint32_t index;
    uint32_t stable_id;

    if (list_view_demo_ctx.item_count == 0U)
    {
        return;
    }

    index = list_view_demo_pick_target_index();
    stable_id = list_view_demo_ctx.items[index].stable_id;
    egui_view_list_view_remove_item_state_by_stable_id(EGUI_VIEW_OF(&list_view), stable_id);

    if ((index + 1U) < list_view_demo_ctx.item_count)
    {
        memmove(&list_view_demo_ctx.items[index], &list_view_demo_ctx.items[index + 1U],
                (list_view_demo_ctx.item_count - index - 1U) * sizeof(list_view_demo_ctx.items[0]));
    }

    list_view_demo_ctx.item_count--;
    list_view_demo_ctx.action_count++;
    if (list_view_demo_ctx.item_count == 0U)
    {
        list_view_demo_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
        list_view_demo_ctx.mutation_cursor = 0U;
    }
    else
    {
        if (index >= list_view_demo_ctx.item_count)
        {
            index = list_view_demo_ctx.item_count - 1U;
        }
        list_view_demo_ctx.selected_id = list_view_demo_ctx.items[index].stable_id;
        list_view_demo_ctx.mutation_cursor = index;
    }

    egui_view_list_view_notify_item_removed(EGUI_VIEW_OF(&list_view), index, 1);
    list_view_demo_refresh_header();
}

static void list_view_demo_action_move(void)
{
    uint32_t index;
    list_view_demo_item_t moved_item;

    if (list_view_demo_ctx.item_count < 2U)
    {
        return;
    }

    index = list_view_demo_pick_target_index();
    if ((index + 1U) >= list_view_demo_ctx.item_count)
    {
        index = 0U;
    }

    moved_item = list_view_demo_ctx.items[index];
    list_view_demo_ctx.items[index] = list_view_demo_ctx.items[index + 1U];
    list_view_demo_ctx.items[index + 1U] = moved_item;
    list_view_demo_ctx.action_count++;
    list_view_demo_ctx.mutation_cursor = index + 1U;
    list_view_demo_set_selected(moved_item.stable_id, 1);
    egui_view_list_view_notify_item_moved(EGUI_VIEW_OF(&list_view), index, index + 1U);
    list_view_demo_refresh_header();
}

static void list_view_demo_action_patch(void)
{
    uint32_t index;
    list_view_demo_item_t *item;
    int32_t before_height;
    int32_t after_height;

    if (list_view_demo_ctx.item_count == 0U)
    {
        return;
    }

    index = list_view_demo_pick_target_index();
    item = &list_view_demo_ctx.items[index];
    before_height = list_view_demo_measure_item_height_from_item(item);

    item->expanded = item->expanded ? 0U : 1U;
    item->progress = (uint8_t)(22U + ((item->progress + 19U) % 70U));
    item->revision++;
    after_height = list_view_demo_measure_item_height_from_item(item);
    list_view_demo_ctx.action_count++;
    list_view_demo_ctx.mutation_cursor = index;
    list_view_demo_set_selected(item->stable_id, 0);

    if (before_height != after_height)
    {
        egui_view_list_view_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&list_view), item->stable_id);
    }
    else
    {
        egui_view_list_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&list_view), item->stable_id);
    }

    list_view_demo_refresh_header();
}

static void list_view_demo_action_jump(void)
{
    uint32_t index;

    if (list_view_demo_ctx.item_count == 0U)
    {
        return;
    }

    if (list_view_demo_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_list_view_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&list_view), list_view_demo_ctx.selected_id, 6);
        list_view_demo_refresh_header();
        return;
    }

    list_view_demo_ctx.mutation_cursor = (list_view_demo_ctx.mutation_cursor + 7U) % list_view_demo_ctx.item_count;
    index = list_view_demo_ctx.mutation_cursor;
    list_view_demo_set_selected(list_view_demo_ctx.items[index].stable_id, 1);
    list_view_demo_ctx.action_count++;
    list_view_demo_refresh_header();
}

static void list_view_demo_action_click_cb(egui_view_t *self)
{
    switch (list_view_demo_find_action_button_index(self))
    {
    case 0:
        list_view_demo_action_add();
        break;
    case 1:
        list_view_demo_action_remove();
        break;
    case 2:
        list_view_demo_action_move();
        break;
    case 3:
        list_view_demo_action_patch();
        break;
    case 4:
        list_view_demo_action_jump();
        break;
    default:
        break;
    }
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

static list_view_demo_hero_holder_t *list_view_demo_find_first_hero_holder(void)
{
    uint32_t index;

    for (index = 0; index < list_view_demo_ctx.item_count; index++)
    {
        egui_view_list_view_holder_t *holder;

        if (list_view_demo_ctx.items[index].view_type != LIST_VIEW_DEMO_VIEWTYPE_HERO)
        {
            continue;
        }

        holder = egui_view_list_view_find_holder_by_stable_id(EGUI_VIEW_OF(&list_view), list_view_demo_ctx.items[index].stable_id);
        if (holder != NULL)
        {
            return (list_view_demo_hero_holder_t *)holder;
        }
    }

    return NULL;
}

static void list_view_demo_set_scroll_action(egui_sim_action_t *p_action, uint32_t interval_ms)
{
    egui_region_t *region = &EGUI_VIEW_OF(&list_view)->region_screen;

    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = (egui_dim_t)(region->location.x + region->size.width / 2);
    p_action->y1 = (egui_dim_t)(region->location.y + region->size.height - 20);
    p_action->x2 = p_action->x1;
    p_action->y2 = (egui_dim_t)(region->location.y + 24);
    p_action->steps = 12;
    p_action->interval_ms = interval_ms;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    static uint32_t target_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    static uint8_t expected_enabled = 0U;
    static uint8_t expected_expanded = 0U;
    int first_call = (action_index != last_action);
    list_view_demo_hero_holder_t *holder;
    list_view_demo_item_t *item;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            target_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
            expected_enabled = 0U;
            expected_expanded = 0U;
            if (list_view_demo_find_first_hero_holder() == NULL)
            {
                report_runtime_failure("hero holder was not materialized");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        holder = list_view_demo_find_first_hero_holder();
        if (holder == NULL)
        {
            report_runtime_failure("hero row was not visible");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (first_call)
        {
            target_stable_id = holder->base.bound_stable_id;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&holder->preview_button), 220);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 140);
        return true;
    case 3:
        holder = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID
                         ? (list_view_demo_hero_holder_t *)egui_view_list_view_find_holder_by_stable_id(EGUI_VIEW_OF(&list_view), target_stable_id)
                         : list_view_demo_find_first_hero_holder();
        item = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID ? list_view_demo_find_item_by_stable_id_internal(target_stable_id) : NULL;
        if (first_call && (holder == NULL || holder->preview_hits != 1U))
        {
            report_runtime_failure("holder local preview state did not update");
        }
        if (holder == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (item == NULL)
        {
            report_runtime_failure("hero row data model entry was not found");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (first_call)
        {
            expected_enabled = item->enabled ? 0U : 1U;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&holder->enabled_switch), 220);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 140);
        return true;
    case 5:
        holder = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID
                         ? (list_view_demo_hero_holder_t *)egui_view_list_view_find_holder_by_stable_id(EGUI_VIEW_OF(&list_view), target_stable_id)
                         : list_view_demo_find_first_hero_holder();
        item = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID ? list_view_demo_find_item_by_stable_id_internal(target_stable_id) : NULL;
        if (first_call && (holder == NULL || item == NULL || item->enabled != expected_enabled))
        {
            report_runtime_failure("switch interaction did not update data model");
        }
        if (holder == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (first_call)
        {
            list_view_demo_mode_selected_cb(EGUI_VIEW_OF(&holder->mode_combo), 2U);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 140);
        return true;
    case 7:
        item = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID ? list_view_demo_find_item_by_stable_id_internal(target_stable_id) : NULL;
        if (first_call && (item == NULL || item->mode_index != 2U))
        {
            report_runtime_failure("combobox selection did not update data model");
        }
        if (item == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (first_call)
        {
            expected_expanded = item->expanded ? 0U : 1U;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[3]), 220);
        return true;
    case 8:
        EGUI_SIM_SET_WAIT(p_action, 140);
        return true;
    case 9:
        item = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID ? list_view_demo_find_item_by_stable_id_internal(target_stable_id) : NULL;
        if (first_call && (item == NULL || item->expanded != expected_expanded))
        {
            report_runtime_failure("patch action did not resize selected row");
        }
        list_view_demo_set_scroll_action(p_action, 360);
        return true;
    case 10:
        EGUI_SIM_SET_WAIT(p_action, 140);
        return true;
    case 11:
        if (first_call && egui_view_list_view_get_scroll_y(EGUI_VIEW_OF(&list_view)) <= 0)
        {
            report_runtime_failure("list view did not scroll");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[4]), 220);
        return true;
    case 12:
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 13:
        holder = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID
                         ? (list_view_demo_hero_holder_t *)egui_view_list_view_find_holder_by_stable_id(EGUI_VIEW_OF(&list_view), target_stable_id)
                         : list_view_demo_find_first_hero_holder();
        if (first_call)
        {
            if (holder == NULL || holder->preview_hits != 1U)
            {
                report_runtime_failure("state cache did not restore holder local preview state");
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
    uint8_t action_index;
    const egui_view_list_view_setup_t setup = {
            .params = &list_view_demo_params,
            .data_model = &list_view_demo_data_model,
            .holder_ops = &list_view_demo_holder_ops,
            .data_model_context = &list_view_demo_ctx,
            .state_cache_max_entries = LIST_VIEW_DEMO_STATE_CACHE_COUNT,
            .state_cache_max_bytes = LIST_VIEW_DEMO_STATE_CACHE_COUNT * (uint32_t)sizeof(list_view_demo_holder_state_t),
    };

    list_view_demo_reset_model();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0U;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&list_view_demo_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&header_card), &list_view_demo_header_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_card), 1, EGUI_COLOR_HEX(0xD1DEE7));

    list_view_demo_init_label(&header_title, 12, 10, LIST_VIEW_DEMO_CONTENT_W - 24, 14, LIST_VIEW_DEMO_FONT_HEADER, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                              EGUI_COLOR_HEX(0x23384A));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_title));

    list_view_demo_init_label(&header_detail, 12, 28, LIST_VIEW_DEMO_CONTENT_W - 24, 12, LIST_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                              EGUI_COLOR_HEX(0x556A79));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_detail));

    list_view_demo_init_label(&header_hint, 12, 44, LIST_VIEW_DEMO_CONTENT_W - 24, 12, LIST_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                              EGUI_COLOR_HEX(0x6B7C8A));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_hint));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &list_view_demo_toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD1DEE7));

    for (action_index = 0; action_index < 5U; action_index++)
    {
        list_view_demo_init_button(&action_buttons[action_index], (egui_dim_t)(10 + action_index * (LIST_VIEW_DEMO_ACTION_W + LIST_VIEW_DEMO_ACTION_GAP)), 7,
                                   LIST_VIEW_DEMO_ACTION_W, LIST_VIEW_DEMO_ACTION_H, list_view_demo_action_names[action_index], list_view_demo_action_click_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[action_index]));
    }

    egui_view_list_view_init_with_setup(EGUI_VIEW_OF(&list_view), &setup);
    egui_view_set_background(EGUI_VIEW_OF(&list_view), EGUI_BG_OF(&list_view_demo_list_bg));

    list_view_demo_refresh_header();

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&list_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&header_card));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}
