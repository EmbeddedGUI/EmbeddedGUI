#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "uicode.h"

#define LIST_VIEW_BASIC_ITEM_COUNT 24U
#define LIST_VIEW_BASIC_MODE_COUNT 3U
#define LIST_VIEW_BASIC_TITLE_LEN  24
#define LIST_VIEW_BASIC_DETAIL_LEN 48
#define LIST_VIEW_BASIC_ROW_HEIGHT 112

#define LIST_VIEW_BASIC_MARGIN_X 8
#define LIST_VIEW_BASIC_TOP_Y    8
#define LIST_VIEW_BASIC_LIST_Y   LIST_VIEW_BASIC_TOP_Y
#define LIST_VIEW_BASIC_LIST_W   (EGUI_CONFIG_SCEEN_WIDTH - LIST_VIEW_BASIC_MARGIN_X * 2)
#define LIST_VIEW_BASIC_LIST_H   (EGUI_CONFIG_SCEEN_HEIGHT - LIST_VIEW_BASIC_LIST_Y - 8)

#define LIST_VIEW_BASIC_FONT_TITLE ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define LIST_VIEW_BASIC_FONT_BODY  ((const egui_font_t *)&egui_res_font_montserrat_8_4)

typedef struct list_view_basic_item list_view_basic_item_t;
typedef struct list_view_basic_context list_view_basic_context_t;
typedef struct list_view_basic_holder list_view_basic_holder_t;

struct list_view_basic_item
{
    uint32_t stable_id;
    uint8_t enabled;
    uint8_t mode_index;
};

struct list_view_basic_context
{
    list_view_basic_item_t items[LIST_VIEW_BASIC_ITEM_COUNT];
};

struct list_view_basic_holder
{
    egui_view_list_view_holder_t base;
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t detail;
    egui_view_switch_t enabled_switch;
    egui_view_combobox_t mode_combo;
    char title_text[LIST_VIEW_BASIC_TITLE_LEN];
    char detail_text[LIST_VIEW_BASIC_DETAIL_LEN];
};

static const char *list_view_basic_mode_items[LIST_VIEW_BASIC_MODE_COUNT] = {"Auto", "Eco", "Boost"};

static egui_view_t background_view;
static egui_view_list_view_t list_view;
static list_view_basic_context_t list_view_basic_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

EGUI_VIEW_SWITCH_PARAMS_INIT(list_view_basic_switch_params, 0, 0, 52, 28, 0);
EGUI_VIEW_COMBOBOX_PARAMS_INIT(list_view_basic_combo_params, 0, 0, 92, 28, list_view_basic_mode_items, LIST_VIEW_BASIC_MODE_COUNT, 0);

static const egui_view_list_view_params_t list_view_basic_params = {
        .region = {{LIST_VIEW_BASIC_MARGIN_X, LIST_VIEW_BASIC_LIST_Y}, {LIST_VIEW_BASIC_LIST_W, LIST_VIEW_BASIC_LIST_H}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 2,
        .estimated_item_height = LIST_VIEW_BASIC_ROW_HEIGHT,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(list_view_basic_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF4F7), EGUI_COLOR_HEX(0xDDE8F0),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(list_view_basic_screen_bg_params, &list_view_basic_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(list_view_basic_screen_bg, &list_view_basic_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(list_view_basic_list_bg_param, EGUI_COLOR_HEX(0xF8FBFD), EGUI_ALPHA_100, 14, 1,
                                                        EGUI_COLOR_HEX(0xC9D8E4), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(list_view_basic_list_bg_params, &list_view_basic_list_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(list_view_basic_list_bg, &list_view_basic_list_bg_params);

static void list_view_basic_switch_checked_cb(egui_view_t *self, int is_checked);
static void list_view_basic_combo_selected_cb(egui_view_t *self, uint8_t index);

static void list_view_basic_init_items(void)
{
    uint32_t index;

    memset(&list_view_basic_ctx, 0, sizeof(list_view_basic_ctx));
    for (index = 0; index < LIST_VIEW_BASIC_ITEM_COUNT; index++)
    {
        list_view_basic_ctx.items[index].stable_id = 1000U + index;
        list_view_basic_ctx.items[index].enabled = (uint8_t)((index % 3U) == 1U);
        list_view_basic_ctx.items[index].mode_index = (uint8_t)(index % LIST_VIEW_BASIC_MODE_COUNT);
    }
}

static void list_view_basic_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const egui_font_t *font,
                                       uint8_t align, egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), width, height);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), align);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static uint32_t list_view_basic_get_count(void *data_model_context)
{
    EGUI_UNUSED(data_model_context);
    return LIST_VIEW_BASIC_ITEM_COUNT;
}

static uint32_t list_view_basic_get_stable_id(void *data_model_context, uint32_t index)
{
    list_view_basic_context_t *context = (list_view_basic_context_t *)data_model_context;
    return index < LIST_VIEW_BASIC_ITEM_COUNT ? context->items[index].stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t list_view_basic_find_index_by_stable_id(void *data_model_context, uint32_t stable_id)
{
    list_view_basic_context_t *context = (list_view_basic_context_t *)data_model_context;
    uint32_t index;

    for (index = 0; index < LIST_VIEW_BASIC_ITEM_COUNT; index++)
    {
        if (context->items[index].stable_id == stable_id)
        {
            return (int32_t)index;
        }
    }

    return -1;
}

static int32_t list_view_basic_measure_item_height(void *data_model_context, uint32_t index, int32_t width_hint)
{
    EGUI_UNUSED(data_model_context);
    EGUI_UNUSED(index);
    EGUI_UNUSED(width_hint);
    return LIST_VIEW_BASIC_ROW_HEIGHT;
}

static void list_view_basic_sync_switch(list_view_basic_holder_t *holder, uint8_t is_checked)
{
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&holder->enabled_switch), NULL);
    egui_view_switch_set_checked(EGUI_VIEW_OF(&holder->enabled_switch), is_checked);
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&holder->enabled_switch), list_view_basic_switch_checked_cb);
}

static egui_view_list_view_holder_t *list_view_basic_create_holder(void *data_model_context, uint16_t view_type)
{
    list_view_basic_holder_t *holder;

    EGUI_UNUSED(data_model_context);
    EGUI_UNUSED(view_type);

    holder = (list_view_basic_holder_t *)egui_malloc(sizeof(list_view_basic_holder_t));
    if (holder == NULL)
    {
        return NULL;
    }

    memset(holder, 0, sizeof(*holder));
    egui_view_group_init(EGUI_VIEW_OF(&holder->root));

    egui_view_card_init(EGUI_VIEW_OF(&holder->card));
    egui_view_set_position(EGUI_VIEW_OF(&holder->card), 4, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&holder->root), EGUI_VIEW_OF(&holder->card));

    list_view_basic_init_label(&holder->title, 12, 10, 140, 14, LIST_VIEW_BASIC_FONT_TITLE, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x23384A));
    egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->title));

    egui_view_combobox_init_with_params(EGUI_VIEW_OF(&holder->mode_combo), &list_view_basic_combo_params);
    egui_view_combobox_set_font(EGUI_VIEW_OF(&holder->mode_combo), LIST_VIEW_BASIC_FONT_BODY);
    egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&holder->mode_combo), EGUI_FONT_ICON_MS_20);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&holder->mode_combo), 2);
    egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&holder->mode_combo), EGUI_ICON_MS_EXPAND_MORE, EGUI_ICON_MS_EXPAND_LESS);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&holder->mode_combo), list_view_basic_combo_selected_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->mode_combo));

    egui_view_switch_init_with_params(EGUI_VIEW_OF(&holder->enabled_switch), &list_view_basic_switch_params);
    egui_view_switch_set_state_icons(EGUI_VIEW_OF(&holder->enabled_switch), EGUI_ICON_MS_DONE, EGUI_ICON_MS_CLOSE);
    egui_view_switch_set_icon_font(EGUI_VIEW_OF(&holder->enabled_switch), EGUI_FONT_ICON_MS_20);
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&holder->enabled_switch), list_view_basic_switch_checked_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->enabled_switch));

    list_view_basic_init_label(&holder->detail, 12, 82, 160, 12, LIST_VIEW_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x5A6F82));
    egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->detail));

    holder->base.item_view = EGUI_VIEW_OF(&holder->root);
    return &holder->base;
}

static void list_view_basic_destroy_holder(void *data_model_context, egui_view_list_view_holder_t *holder, uint16_t view_type)
{
    EGUI_UNUSED(data_model_context);
    EGUI_UNUSED(view_type);
    egui_free(holder);
}

static void list_view_basic_bind_holder(void *data_model_context, egui_view_list_view_holder_t *holder, uint32_t index, uint32_t stable_id)
{
    list_view_basic_context_t *context = (list_view_basic_context_t *)data_model_context;
    list_view_basic_holder_t *typed_holder = (list_view_basic_holder_t *)holder;
    list_view_basic_item_t *item;
    egui_dim_t root_w;
    egui_dim_t root_h;
    egui_dim_t card_w;
    egui_dim_t card_h;
    egui_color_t card_fill;
    egui_color_t card_border;

    EGUI_UNUSED(stable_id);

    if (index >= LIST_VIEW_BASIC_ITEM_COUNT)
    {
        return;
    }

    item = &context->items[index];
    root_w = holder->host_view->region.size.width;
    root_h = holder->host_view->region.size.height;
    card_w = root_w - 8;
    card_h = root_h - 8;

    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->root), root_w, root_h);
    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->card), card_w, card_h);

    snprintf(typed_holder->title_text, sizeof(typed_holder->title_text), "Device %02lu", (unsigned long)(index + 1U));
    snprintf(typed_holder->detail_text, sizeof(typed_holder->detail_text), "Mode %s  Switch %s", list_view_basic_mode_items[item->mode_index],
             item->enabled ? "On" : "Off");

    card_fill = item->enabled ? EGUI_COLOR_HEX(0xEAF6F0) : EGUI_COLOR_WHITE;
    card_border = item->enabled ? EGUI_COLOR_HEX(0xB6D9C7) : EGUI_COLOR_HEX(0xC8D7E3);

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&typed_holder->card), card_fill, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&typed_holder->card), 1, card_border);

    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->title), card_w - 24, 14);
    egui_view_label_set_text(EGUI_VIEW_OF(&typed_holder->title), typed_holder->title_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&typed_holder->title), EGUI_COLOR_HEX(0x23384A), EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&typed_holder->mode_combo), 12, 24);
    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->mode_combo), card_w - 88, 28);
    egui_view_combobox_set_current_index(EGUI_VIEW_OF(&typed_holder->mode_combo), item->mode_index);

    egui_view_set_position(EGUI_VIEW_OF(&typed_holder->enabled_switch), card_w - 64, 24);
    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->enabled_switch), 52, 28);
    list_view_basic_sync_switch(typed_holder, item->enabled);

    egui_view_set_position(EGUI_VIEW_OF(&typed_holder->detail), 12, card_h - 22);
    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->detail), card_w - 24, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&typed_holder->detail), typed_holder->detail_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&typed_holder->detail), EGUI_COLOR_HEX(0x5A6F82), EGUI_ALPHA_100);
}

static const egui_view_list_view_data_model_t list_view_basic_data_model = {
        .get_count = list_view_basic_get_count,
        .get_stable_id = list_view_basic_get_stable_id,
        .find_index_by_stable_id = list_view_basic_find_index_by_stable_id,
        .get_view_type = NULL,
        .measure_item_height = list_view_basic_measure_item_height,
        .default_view_type = 0,
};

static const egui_view_list_view_holder_ops_t list_view_basic_holder_ops = {
        .create_holder = list_view_basic_create_holder,
        .destroy_holder = list_view_basic_destroy_holder,
        .bind_holder = list_view_basic_bind_holder,
        .unbind_holder = NULL,
        .should_keep_alive = NULL,
        .save_holder_state = NULL,
        .restore_holder_state = NULL,
};

static void list_view_basic_switch_checked_cb(egui_view_t *self, int is_checked)
{
    egui_view_list_view_entry_t entry;

    if (!egui_view_list_view_resolve_item_by_view(EGUI_VIEW_OF(&list_view), self, &entry))
    {
        return;
    }

    list_view_basic_ctx.items[entry.index].enabled = is_checked ? 1U : 0U;
    egui_view_list_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&list_view), entry.stable_id);
}

static void list_view_basic_combo_selected_cb(egui_view_t *self, uint8_t index)
{
    egui_view_list_view_entry_t entry;

    if (!egui_view_list_view_resolve_item_by_view(EGUI_VIEW_OF(&list_view), self, &entry))
    {
        return;
    }

    list_view_basic_ctx.items[entry.index].mode_index = index;
    egui_view_list_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&list_view), entry.stable_id);
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

static list_view_basic_holder_t *list_view_basic_find_holder_by_index(uint32_t index)
{
    egui_view_list_view_holder_t *holder;

    if (index >= LIST_VIEW_BASIC_ITEM_COUNT)
    {
        return NULL;
    }

    holder = egui_view_list_view_find_holder_by_stable_id(EGUI_VIEW_OF(&list_view), list_view_basic_ctx.items[index].stable_id);
    return (list_view_basic_holder_t *)holder;
}

static void list_view_basic_set_scroll_action(egui_sim_action_t *p_action, uint32_t interval_ms)
{
    egui_region_t *region = &EGUI_VIEW_OF(&list_view)->region_screen;

    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = (egui_dim_t)(region->location.x + region->size.width / 2);
    p_action->y1 = (egui_dim_t)(region->location.y + region->size.height - 24);
    p_action->x2 = p_action->x1;
    p_action->y2 = (egui_dim_t)(region->location.y + 32);
    p_action->steps = 10;
    p_action->interval_ms = interval_ms;
}

static void list_view_basic_get_dropdown_item_center(egui_view_combobox_t *combo, uint8_t item_index, int *x, int *y)
{
    *x = combo->base.region_screen.location.x + combo->base.region_screen.size.width / 2;
    *y = combo->base.region_screen.location.y + combo->collapsed_height + item_index * combo->item_height + combo->item_height / 2;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    list_view_basic_holder_t *holder;
    int x;
    int y;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            if (list_view_basic_find_holder_by_index(0) == NULL || list_view_basic_find_holder_by_index(1) == NULL)
            {
                report_runtime_failure("initial list holders were not materialized");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        holder = list_view_basic_find_holder_by_index(0);
        if (holder == NULL)
        {
            report_runtime_failure("first list row was not visible");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&holder->enabled_switch), 220);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 140);
        return true;
    case 3:
        if (first_call && list_view_basic_ctx.items[0].enabled != 1U)
        {
            report_runtime_failure("switch change did not update data model");
        }
        holder = list_view_basic_find_holder_by_index(1);
        if (holder == NULL)
        {
            report_runtime_failure("second list row was not visible");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&holder->mode_combo), 220);
        return true;
    case 4:
        holder = list_view_basic_find_holder_by_index(1);
        if (first_call)
        {
            if (holder == NULL || !egui_view_combobox_is_expanded(EGUI_VIEW_OF(&holder->mode_combo)))
            {
                report_runtime_failure("combobox did not expand inside list row");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 5:
        holder = list_view_basic_find_holder_by_index(1);
        if (holder == NULL)
        {
            report_runtime_failure("combobox row disappeared before selection");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        list_view_basic_get_dropdown_item_center(&holder->mode_combo, 1, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 220;
        return true;
    case 6:
        if (first_call && list_view_basic_ctx.items[1].mode_index != 1U)
        {
            report_runtime_failure("combobox selection did not update data model");
        }
        if (first_call)
        {
            recording_request_snapshot();
        }
        list_view_basic_set_scroll_action(p_action, 320);
        return true;
    case 7:
        if (first_call)
        {
            if (egui_view_list_view_get_scroll_y(EGUI_VIEW_OF(&list_view)) <= 0)
            {
                report_runtime_failure("list view did not scroll");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    default:
        return false;
    }
}
#endif

void test_init_ui(void)
{
    const egui_view_list_view_setup_t setup = {
            .params = &list_view_basic_params,
            .data_model = &list_view_basic_data_model,
            .holder_ops = &list_view_basic_holder_ops,
            .data_model_context = &list_view_basic_ctx,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };

    list_view_basic_init_items();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0U;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&list_view_basic_screen_bg));

    egui_view_list_view_init_with_setup(EGUI_VIEW_OF(&list_view), &setup);
    egui_view_set_background(EGUI_VIEW_OF(&list_view), EGUI_BG_OF(&list_view_basic_list_bg));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&list_view));
}
