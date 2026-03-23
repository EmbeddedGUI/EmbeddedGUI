#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "uicode.h"

#define GRID_VIEW_DEMO_MAX_ITEMS         40U
#define GRID_VIEW_DEMO_STATE_CACHE_COUNT 24U
#define GRID_VIEW_DEMO_TITLE_LEN         28
#define GRID_VIEW_DEMO_DETAIL_LEN        36
#define GRID_VIEW_DEMO_META_LEN          36
#define GRID_VIEW_DEMO_PREVIEW_LEN       20
#define GRID_VIEW_DEMO_HEADER_LEN        112

#define GRID_VIEW_DEMO_VIEWTYPE_HERO   1U
#define GRID_VIEW_DEMO_VIEWTYPE_METRIC 2U

#define GRID_VIEW_DEMO_MARGIN_X  8
#define GRID_VIEW_DEMO_TOP_Y     8
#define GRID_VIEW_DEMO_CONTENT_W (EGUI_CONFIG_SCEEN_WIDTH - GRID_VIEW_DEMO_MARGIN_X * 2)
#define GRID_VIEW_DEMO_HEADER_H  66
#define GRID_VIEW_DEMO_TOOLBAR_Y (GRID_VIEW_DEMO_TOP_Y + GRID_VIEW_DEMO_HEADER_H + 6)
#define GRID_VIEW_DEMO_TOOLBAR_H 56
#define GRID_VIEW_DEMO_GRID_Y    (GRID_VIEW_DEMO_TOOLBAR_Y + GRID_VIEW_DEMO_TOOLBAR_H + 6)
#define GRID_VIEW_DEMO_GRID_H    (EGUI_CONFIG_SCEEN_HEIGHT - GRID_VIEW_DEMO_GRID_Y - 8)

#define GRID_VIEW_DEMO_ACTION_GAP 4
#define GRID_VIEW_DEMO_ACTION_W   ((GRID_VIEW_DEMO_CONTENT_W - 20 - GRID_VIEW_DEMO_ACTION_GAP * 3) / 4)
#define GRID_VIEW_DEMO_ACTION_H   18
#define GRID_VIEW_DEMO_MODE_W     ((GRID_VIEW_DEMO_CONTENT_W - 20 - GRID_VIEW_DEMO_ACTION_GAP * 2) / 3)
#define GRID_VIEW_DEMO_MODE_H     18

#define GRID_VIEW_DEMO_FONT_HEADER ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define GRID_VIEW_DEMO_FONT_BODY   ((const egui_font_t *)&egui_res_font_montserrat_8_4)

typedef struct grid_view_demo_item grid_view_demo_item_t;
typedef struct grid_view_demo_context grid_view_demo_context_t;
typedef struct grid_view_demo_hero_holder grid_view_demo_hero_holder_t;
typedef struct grid_view_demo_metric_holder grid_view_demo_metric_holder_t;
typedef struct grid_view_demo_holder_state grid_view_demo_holder_state_t;

struct grid_view_demo_item
{
    uint32_t stable_id;
    uint8_t view_type;
    uint8_t armed;
    uint8_t expanded;
    uint8_t progress;
    uint8_t revision;
};

struct grid_view_demo_context
{
    grid_view_demo_item_t items[GRID_VIEW_DEMO_MAX_ITEMS];
    uint32_t item_count;
    uint32_t next_stable_id;
    uint32_t selected_id;
    uint32_t mutation_cursor;
    uint16_t toggle_count;
    uint16_t switch_count;
    uint16_t preview_count;
    uint16_t action_count;
    char header_title[GRID_VIEW_DEMO_HEADER_LEN];
    char header_detail[GRID_VIEW_DEMO_HEADER_LEN];
    char header_hint[GRID_VIEW_DEMO_HEADER_LEN];
};

struct grid_view_demo_holder_state
{
    uint8_t preview_hits;
};

struct grid_view_demo_hero_holder
{
    egui_view_grid_view_holder_t base;
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t detail;
    egui_view_label_t preview_label;
    egui_view_progress_bar_t progress_bar;
    egui_view_toggle_button_t arm_toggle;
    egui_view_button_t preview_button;
    char title_text[GRID_VIEW_DEMO_TITLE_LEN];
    char detail_text[GRID_VIEW_DEMO_DETAIL_LEN];
    char preview_text[GRID_VIEW_DEMO_PREVIEW_LEN];
    uint8_t preview_hits;
};

struct grid_view_demo_metric_holder
{
    egui_view_grid_view_holder_t base;
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t detail;
    egui_view_label_t meta;
    egui_view_label_t preview_label;
    egui_view_progress_bar_t progress_bar;
    egui_view_switch_t armed_switch;
    egui_view_button_t preview_button;
    char title_text[GRID_VIEW_DEMO_TITLE_LEN];
    char detail_text[GRID_VIEW_DEMO_DETAIL_LEN];
    char meta_text[GRID_VIEW_DEMO_META_LEN];
    char preview_text[GRID_VIEW_DEMO_PREVIEW_LEN];
    uint8_t preview_hits;
};

static const char *grid_view_demo_action_names[4] = {"Add", "Del", "Patch", "Jump"};
static const char *grid_view_demo_mode_names[3] = {"2 Col", "3 Col", "4 Col"};

static egui_view_t background_view;
static egui_view_card_t header_card;
static egui_view_card_t toolbar_card;
static egui_view_label_t header_title;
static egui_view_label_t header_detail;
static egui_view_label_t header_hint;
static egui_view_button_t action_buttons[4];
static egui_view_button_t mode_buttons[3];
static egui_view_grid_view_t grid_view;
static grid_view_demo_context_t grid_view_demo_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(grid_view_demo_header_card_params, GRID_VIEW_DEMO_MARGIN_X, GRID_VIEW_DEMO_TOP_Y, GRID_VIEW_DEMO_CONTENT_W, GRID_VIEW_DEMO_HEADER_H,
                           14);
EGUI_VIEW_CARD_PARAMS_INIT(grid_view_demo_toolbar_card_params, GRID_VIEW_DEMO_MARGIN_X, GRID_VIEW_DEMO_TOOLBAR_Y, GRID_VIEW_DEMO_CONTENT_W,
                           GRID_VIEW_DEMO_TOOLBAR_H, 12);
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(grid_view_demo_toggle_params, 0, 0, 72, 26, "Arm", 0);
EGUI_VIEW_SWITCH_PARAMS_INIT(grid_view_demo_switch_params, 0, 0, 46, 24, 0);

static const egui_view_grid_view_params_t grid_view_demo_params = {
        .region = {{GRID_VIEW_DEMO_MARGIN_X, GRID_VIEW_DEMO_GRID_Y}, {GRID_VIEW_DEMO_CONTENT_W, GRID_VIEW_DEMO_GRID_H}},
        .column_count = 2,
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 3,
        .column_spacing = 6,
        .row_spacing = 6,
        .estimated_item_height = 88,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(grid_view_demo_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xF4F1EA), EGUI_COLOR_HEX(0xDCE8F2),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(grid_view_demo_screen_bg_params, &grid_view_demo_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(grid_view_demo_screen_bg, &grid_view_demo_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(grid_view_demo_grid_bg_param, EGUI_COLOR_HEX(0xF8FBFD), EGUI_ALPHA_100, 14, 1, EGUI_COLOR_HEX(0xCBD8E3),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(grid_view_demo_grid_bg_params, &grid_view_demo_grid_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_view_demo_grid_bg, &grid_view_demo_grid_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_view_demo_button_bg_param, EGUI_COLOR_HEX(0xE6EEF5), EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(grid_view_demo_button_bg_params, &grid_view_demo_button_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_view_demo_button_bg, &grid_view_demo_button_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(grid_view_demo_mode_active_bg_param, EGUI_COLOR_HEX(0xD7EBFF), EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(grid_view_demo_mode_active_bg_params, &grid_view_demo_mode_active_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_view_demo_mode_active_bg, &grid_view_demo_mode_active_bg_params);

static void grid_view_demo_refresh_header(void);
static void grid_view_demo_toggle_cb(egui_view_t *self, uint8_t is_toggled);
static void grid_view_demo_switch_checked_cb(egui_view_t *self, int is_checked);
static void grid_view_demo_preview_click_cb(egui_view_t *self);
static void grid_view_demo_action_click_cb(egui_view_t *self);

static grid_view_demo_item_t *grid_view_demo_get_item(uint32_t index)
{
    if (index >= grid_view_demo_ctx.item_count)
    {
        return NULL;
    }

    return &grid_view_demo_ctx.items[index];
}

static grid_view_demo_item_t *grid_view_demo_find_item_by_stable_id_internal(uint32_t stable_id)
{
    uint32_t index;

    for (index = 0; index < grid_view_demo_ctx.item_count; index++)
    {
        if (grid_view_demo_ctx.items[index].stable_id == stable_id)
        {
            return &grid_view_demo_ctx.items[index];
        }
    }

    return NULL;
}

static int32_t grid_view_demo_find_index_by_stable_id_internal(uint32_t stable_id)
{
    uint32_t index;

    for (index = 0; index < grid_view_demo_ctx.item_count; index++)
    {
        if (grid_view_demo_ctx.items[index].stable_id == stable_id)
        {
            return (int32_t)index;
        }
    }

    return -1;
}

static void grid_view_demo_fill_item(grid_view_demo_item_t *item, uint8_t view_type, uint32_t seed)
{
    memset(item, 0, sizeof(*item));
    item->stable_id = grid_view_demo_ctx.next_stable_id++;
    item->view_type = view_type;
    item->armed = (uint8_t)((seed % 3U) == 1U);
    item->expanded = (uint8_t)((seed % 5U) == 0U);
    item->progress = (uint8_t)(20U + ((seed * 11U) % 71U));
    item->revision = (uint8_t)(1U + (seed % 7U));
}

static void grid_view_demo_reset_model(void)
{
    uint32_t index;

    memset(&grid_view_demo_ctx, 0, sizeof(grid_view_demo_ctx));
    grid_view_demo_ctx.item_count = 24U;
    grid_view_demo_ctx.next_stable_id = 42001U;
    grid_view_demo_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    grid_view_demo_ctx.mutation_cursor = 0U;

    for (index = 0; index < grid_view_demo_ctx.item_count; index++)
    {
        grid_view_demo_fill_item(&grid_view_demo_ctx.items[index],
                                 (uint8_t)((index % 4U) == 0U ? GRID_VIEW_DEMO_VIEWTYPE_HERO : GRID_VIEW_DEMO_VIEWTYPE_METRIC), index + 5U);
    }
}

static void grid_view_demo_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const egui_font_t *font,
                                      uint8_t align, egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), width, height);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), align);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static void grid_view_demo_init_button(egui_view_button_t *button, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const char *text,
                                       egui_view_on_click_listener_t listener)
{
    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_position(EGUI_VIEW_OF(button), x, y);
    egui_view_set_size(EGUI_VIEW_OF(button), width, height);
    egui_view_label_set_text(EGUI_VIEW_OF(button), text);
    egui_view_label_set_font(EGUI_VIEW_OF(button), GRID_VIEW_DEMO_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x274056), EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(button), EGUI_BG_OF(&grid_view_demo_button_bg));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), listener);
}

static int32_t grid_view_demo_measure_item_height_from_item(const grid_view_demo_item_t *item, int32_t width_hint)
{
    uint8_t compact = width_hint < 70 ? 1U : 0U;
    int32_t base_height;

    if (item == NULL)
    {
        return compact ? 60 : 78;
    }

    if (item->view_type == GRID_VIEW_DEMO_VIEWTYPE_HERO)
    {
        base_height = compact ? 78 : 96;
    }
    else
    {
        base_height = compact ? 62 : 76;
    }

    if (item->expanded)
    {
        base_height += compact ? 10 : 14;
    }

    return base_height;
}

static void grid_view_demo_set_preview_text(char *buffer, uint16_t capacity, uint8_t preview_hits)
{
    snprintf(buffer, capacity, "Peek %u", (unsigned)preview_hits);
}

static void grid_view_demo_set_hero_preview_hits(grid_view_demo_hero_holder_t *holder, uint8_t preview_hits)
{
    holder->preview_hits = preview_hits;
    grid_view_demo_set_preview_text(holder->preview_text, sizeof(holder->preview_text), preview_hits);
    egui_view_label_set_text(EGUI_VIEW_OF(&holder->preview_label), holder->preview_text);
}

static void grid_view_demo_set_metric_preview_hits(grid_view_demo_metric_holder_t *holder, uint8_t preview_hits)
{
    holder->preview_hits = preview_hits;
    grid_view_demo_set_preview_text(holder->preview_text, sizeof(holder->preview_text), preview_hits);
    egui_view_label_set_text(EGUI_VIEW_OF(&holder->preview_label), holder->preview_text);
}

static void grid_view_demo_set_selected(uint32_t stable_id, uint8_t ensure_visible)
{
    uint32_t previous_id = grid_view_demo_ctx.selected_id;

    if (previous_id == stable_id)
    {
        if (ensure_visible)
        {
            egui_view_grid_view_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id, 4);
        }
        return;
    }

    grid_view_demo_ctx.selected_id = stable_id;
    if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_grid_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&grid_view), previous_id);
    }
    if (stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_grid_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id);
        if (ensure_visible)
        {
            egui_view_grid_view_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id, 4);
        }
    }

    grid_view_demo_refresh_header();
}

static uint32_t grid_view_demo_pick_target_index(void)
{
    int32_t selected_index = grid_view_demo_find_index_by_stable_id_internal(grid_view_demo_ctx.selected_id);

    if (selected_index >= 0)
    {
        return (uint32_t)selected_index;
    }

    if (grid_view_demo_ctx.item_count == 0U)
    {
        return 0U;
    }

    if (grid_view_demo_ctx.mutation_cursor >= grid_view_demo_ctx.item_count)
    {
        grid_view_demo_ctx.mutation_cursor = 0U;
    }

    return grid_view_demo_ctx.mutation_cursor;
}

static void grid_view_demo_sync_toggle(grid_view_demo_hero_holder_t *holder, uint8_t is_toggled)
{
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&holder->arm_toggle), NULL);
    egui_view_toggle_button_set_toggled(EGUI_VIEW_OF(&holder->arm_toggle), is_toggled);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&holder->arm_toggle), grid_view_demo_toggle_cb);
}

static void grid_view_demo_sync_switch(grid_view_demo_metric_holder_t *holder, uint8_t is_checked)
{
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&holder->armed_switch), NULL);
    egui_view_switch_set_checked(EGUI_VIEW_OF(&holder->armed_switch), is_checked);
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&holder->armed_switch), grid_view_demo_switch_checked_cb);
}

static void grid_view_demo_refresh_mode_buttons(void)
{
    uint8_t index;
    uint8_t active_columns = egui_view_grid_view_get_column_count(EGUI_VIEW_OF(&grid_view));

    for (index = 0; index < 3U; index++)
    {
        egui_view_set_background(EGUI_VIEW_OF(&mode_buttons[index]),
                                 (index + 2U) == active_columns ? EGUI_BG_OF(&grid_view_demo_mode_active_bg) : EGUI_BG_OF(&grid_view_demo_button_bg));
    }
}

static void grid_view_demo_refresh_header(void)
{
    snprintf(grid_view_demo_ctx.header_title, sizeof(grid_view_demo_ctx.header_title), "grid_view  tiles %u  rows %u", (unsigned)grid_view_demo_ctx.item_count,
             (unsigned)egui_view_grid_view_get_row_count(EGUI_VIEW_OF(&grid_view)));
    snprintf(grid_view_demo_ctx.header_detail, sizeof(grid_view_demo_ctx.header_detail), "toggle %u  switch %u  preview %u  cols %u",
             (unsigned)grid_view_demo_ctx.toggle_count, (unsigned)grid_view_demo_ctx.switch_count, (unsigned)grid_view_demo_ctx.preview_count,
             (unsigned)egui_view_grid_view_get_column_count(EGUI_VIEW_OF(&grid_view)));
    snprintf(grid_view_demo_ctx.header_hint, sizeof(grid_view_demo_ctx.header_hint), "selected %lu  actions %u  width_hint drives height",
             (unsigned long)grid_view_demo_ctx.selected_id, (unsigned)grid_view_demo_ctx.action_count);

    egui_view_label_set_text(EGUI_VIEW_OF(&header_title), grid_view_demo_ctx.header_title);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_detail), grid_view_demo_ctx.header_detail);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_hint), grid_view_demo_ctx.header_hint);
}

static uint32_t grid_view_demo_get_count(void *data_model_context)
{
    return ((grid_view_demo_context_t *)data_model_context)->item_count;
}

static uint32_t grid_view_demo_get_stable_id(void *data_model_context, uint32_t index)
{
    grid_view_demo_context_t *context = (grid_view_demo_context_t *)data_model_context;
    return index < context->item_count ? context->items[index].stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t grid_view_demo_find_index_by_stable_id(void *data_model_context, uint32_t stable_id)
{
    grid_view_demo_context_t *context = (grid_view_demo_context_t *)data_model_context;
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

static uint16_t grid_view_demo_get_view_type(void *data_model_context, uint32_t index)
{
    grid_view_demo_context_t *context = (grid_view_demo_context_t *)data_model_context;
    return index < context->item_count ? context->items[index].view_type : GRID_VIEW_DEMO_VIEWTYPE_METRIC;
}

static int32_t grid_view_demo_measure_item_height(void *data_model_context, uint32_t index, int32_t width_hint)
{
    grid_view_demo_context_t *context = (grid_view_demo_context_t *)data_model_context;

    if (index >= context->item_count)
    {
        return grid_view_demo_measure_item_height_from_item(NULL, width_hint);
    }

    return grid_view_demo_measure_item_height_from_item(&context->items[index], width_hint);
}

static egui_view_grid_view_holder_t *grid_view_demo_create_holder(void *data_model_context, uint16_t view_type)
{
    EGUI_UNUSED(data_model_context);

    if (view_type == GRID_VIEW_DEMO_VIEWTYPE_HERO)
    {
        grid_view_demo_hero_holder_t *holder = (grid_view_demo_hero_holder_t *)egui_malloc(sizeof(grid_view_demo_hero_holder_t));

        if (holder == NULL)
        {
            return NULL;
        }

        memset(holder, 0, sizeof(*holder));
        egui_view_group_init(EGUI_VIEW_OF(&holder->root));

        egui_view_card_init(EGUI_VIEW_OF(&holder->card));
        egui_view_set_position(EGUI_VIEW_OF(&holder->card), 3, 3);
        egui_view_group_add_child(EGUI_VIEW_OF(&holder->root), EGUI_VIEW_OF(&holder->card));

        grid_view_demo_init_label(&holder->title, 10, 10, 88, 14, GRID_VIEW_DEMO_FONT_HEADER, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x213547));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->title));

        grid_view_demo_init_label(&holder->detail, 10, 28, 90, 12, GRID_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x566A7B));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->detail));

        grid_view_demo_init_label(&holder->preview_label, 0, 0, 46, 12, GRID_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER,
                                  EGUI_COLOR_HEX(0x35556E));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->preview_label));
        grid_view_demo_set_hero_preview_hits(holder, 0U);

        egui_view_progress_bar_init(EGUI_VIEW_OF(&holder->progress_bar));
        holder->progress_bar.is_show_control = 0;
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->progress_bar));

        egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&holder->arm_toggle), &grid_view_demo_toggle_params);
        egui_view_toggle_button_set_icon(EGUI_VIEW_OF(&holder->arm_toggle), EGUI_ICON_MS_VISIBILITY);
        egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&holder->arm_toggle), EGUI_FONT_ICON_MS_16);
        egui_view_toggle_button_set_icon_text_gap(EGUI_VIEW_OF(&holder->arm_toggle), 4);
        egui_view_toggle_button_set_text_color(EGUI_VIEW_OF(&holder->arm_toggle), EGUI_COLOR_WHITE);
        egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&holder->arm_toggle), grid_view_demo_toggle_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->arm_toggle));

        grid_view_demo_init_button(&holder->preview_button, 0, 0, 46, 20, "Peek", grid_view_demo_preview_click_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->preview_button));

        holder->base.item_view = EGUI_VIEW_OF(&holder->root);
        return &holder->base;
    }
    else
    {
        grid_view_demo_metric_holder_t *holder = (grid_view_demo_metric_holder_t *)egui_malloc(sizeof(grid_view_demo_metric_holder_t));

        if (holder == NULL)
        {
            return NULL;
        }

        memset(holder, 0, sizeof(*holder));
        egui_view_group_init(EGUI_VIEW_OF(&holder->root));

        egui_view_card_init(EGUI_VIEW_OF(&holder->card));
        egui_view_set_position(EGUI_VIEW_OF(&holder->card), 3, 3);
        egui_view_group_add_child(EGUI_VIEW_OF(&holder->root), EGUI_VIEW_OF(&holder->card));

        grid_view_demo_init_label(&holder->title, 10, 10, 88, 14, GRID_VIEW_DEMO_FONT_HEADER, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x213547));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->title));

        grid_view_demo_init_label(&holder->detail, 10, 28, 88, 12, GRID_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x566A7B));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->detail));

        grid_view_demo_init_label(&holder->meta, 10, 44, 88, 12, GRID_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x6D7D8B));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->meta));

        grid_view_demo_init_label(&holder->preview_label, 0, 0, 46, 12, GRID_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER,
                                  EGUI_COLOR_HEX(0x35556E));
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->preview_label));
        grid_view_demo_set_metric_preview_hits(holder, 0U);

        egui_view_progress_bar_init(EGUI_VIEW_OF(&holder->progress_bar));
        holder->progress_bar.is_show_control = 0;
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->progress_bar));

        egui_view_switch_init_with_params(EGUI_VIEW_OF(&holder->armed_switch), &grid_view_demo_switch_params);
        egui_view_switch_set_state_icons(EGUI_VIEW_OF(&holder->armed_switch), EGUI_ICON_MS_DONE, EGUI_ICON_MS_CLOSE);
        egui_view_switch_set_icon_font(EGUI_VIEW_OF(&holder->armed_switch), EGUI_FONT_ICON_MS_16);
        egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&holder->armed_switch), grid_view_demo_switch_checked_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->armed_switch));

        grid_view_demo_init_button(&holder->preview_button, 0, 0, 46, 20, "Peek", grid_view_demo_preview_click_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->preview_button));

        holder->base.item_view = EGUI_VIEW_OF(&holder->root);
        return &holder->base;
    }
}

static void grid_view_demo_destroy_holder(void *data_model_context, egui_view_grid_view_holder_t *holder, uint16_t view_type)
{
    EGUI_UNUSED(data_model_context);
    EGUI_UNUSED(view_type);
    egui_free(holder);
}

static void grid_view_demo_bind_hero_holder(grid_view_demo_hero_holder_t *holder, const grid_view_demo_item_t *item, uint32_t index, uint8_t selected)
{
    egui_dim_t root_w = holder->base.host_view->region.size.width;
    egui_dim_t root_h = holder->base.host_view->region.size.height;
    egui_dim_t card_w = root_w - 6;
    egui_dim_t card_h = root_h - 6;

    egui_view_set_size(EGUI_VIEW_OF(&holder->root), root_w, root_h);
    egui_view_set_size(EGUI_VIEW_OF(&holder->card), card_w, card_h);

    snprintf(holder->title_text, sizeof(holder->title_text), "Hero %02lu", (unsigned long)(index + 1U));
    snprintf(holder->detail_text, sizeof(holder->detail_text), "%s  %u%%", item->armed ? "armed" : "standby", (unsigned)item->progress);

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&holder->card), selected ? EGUI_COLOR_HEX(0xE8F2FF) : EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&holder->card), 1, selected ? EGUI_COLOR_HEX(0x6A99CF) : EGUI_COLOR_HEX(0xCBD8E3));

    egui_view_label_set_text(EGUI_VIEW_OF(&holder->title), holder->title_text);
    egui_view_set_size(EGUI_VIEW_OF(&holder->title), card_w - 20, 14);
    egui_view_label_set_text(EGUI_VIEW_OF(&holder->detail), holder->detail_text);
    egui_view_set_size(EGUI_VIEW_OF(&holder->detail), card_w - 20, 12);

    egui_view_set_position(EGUI_VIEW_OF(&holder->arm_toggle), 10, card_h - 34);
    egui_view_set_size(EGUI_VIEW_OF(&holder->arm_toggle), card_w - 20, 24);
    grid_view_demo_sync_toggle(holder, item->armed);

    egui_view_set_position(EGUI_VIEW_OF(&holder->progress_bar), 10, card_h - 48);
    egui_view_set_size(EGUI_VIEW_OF(&holder->progress_bar), card_w - 20, 6);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&holder->progress_bar), item->progress);
    holder->progress_bar.progress_color = item->armed ? EGUI_COLOR_HEX(0x346E9A) : EGUI_COLOR_HEX(0x8AA1B3);
    holder->progress_bar.bk_color = EGUI_COLOR_HEX(0xD7E4EC);

    egui_view_set_position(EGUI_VIEW_OF(&holder->preview_button), card_w - 54, 8);
    egui_view_set_size(EGUI_VIEW_OF(&holder->preview_button), 44, 20);
    egui_view_set_position(EGUI_VIEW_OF(&holder->preview_label), card_w - 58, 30);
    egui_view_set_size(EGUI_VIEW_OF(&holder->preview_label), 48, 12);
}

static void grid_view_demo_bind_metric_holder(grid_view_demo_metric_holder_t *holder, const grid_view_demo_item_t *item, uint32_t index, uint8_t selected)
{
    egui_dim_t root_w = holder->base.host_view->region.size.width;
    egui_dim_t root_h = holder->base.host_view->region.size.height;
    egui_dim_t card_w = root_w - 6;
    egui_dim_t card_h = root_h - 6;

    egui_view_set_size(EGUI_VIEW_OF(&holder->root), root_w, root_h);
    egui_view_set_size(EGUI_VIEW_OF(&holder->card), card_w, card_h);

    snprintf(holder->title_text, sizeof(holder->title_text), "Metric %02lu", (unsigned long)(index + 1U));
    snprintf(holder->detail_text, sizeof(holder->detail_text), "%u%%  rev %u", (unsigned)item->progress, (unsigned)item->revision);
    snprintf(holder->meta_text, sizeof(holder->meta_text), "%s", item->expanded ? "expanded" : "compact");

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&holder->card), selected ? EGUI_COLOR_HEX(0xEEF7EF) : EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&holder->card), 1, selected ? EGUI_COLOR_HEX(0x5B9A6B) : EGUI_COLOR_HEX(0xCBD8E3));

    egui_view_label_set_text(EGUI_VIEW_OF(&holder->title), holder->title_text);
    egui_view_set_size(EGUI_VIEW_OF(&holder->title), card_w - 20, 14);
    egui_view_label_set_text(EGUI_VIEW_OF(&holder->detail), holder->detail_text);
    egui_view_set_size(EGUI_VIEW_OF(&holder->detail), card_w - 20, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&holder->meta), holder->meta_text);
    egui_view_set_size(EGUI_VIEW_OF(&holder->meta), card_w - 20, 12);

    egui_view_set_position(EGUI_VIEW_OF(&holder->armed_switch), card_w - 56, 8);
    egui_view_set_size(EGUI_VIEW_OF(&holder->armed_switch), 46, 24);
    grid_view_demo_sync_switch(holder, item->armed);

    egui_view_set_position(EGUI_VIEW_OF(&holder->progress_bar), 10, card_h - 46);
    egui_view_set_size(EGUI_VIEW_OF(&holder->progress_bar), card_w - 20, 6);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&holder->progress_bar), item->progress);
    holder->progress_bar.progress_color = item->armed ? EGUI_COLOR_HEX(0x4D8B64) : EGUI_COLOR_HEX(0x8AA1B3);
    holder->progress_bar.bk_color = EGUI_COLOR_HEX(0xD7E4EC);

    egui_view_set_position(EGUI_VIEW_OF(&holder->preview_button), card_w - 54, card_h - 28);
    egui_view_set_size(EGUI_VIEW_OF(&holder->preview_button), 44, 20);
    egui_view_set_position(EGUI_VIEW_OF(&holder->preview_label), card_w - 58, 36);
    egui_view_set_size(EGUI_VIEW_OF(&holder->preview_label), 48, 12);
}

static void grid_view_demo_bind_holder(void *data_model_context, egui_view_grid_view_holder_t *holder, uint32_t index, uint32_t stable_id)
{
    grid_view_demo_context_t *context = (grid_view_demo_context_t *)data_model_context;
    const grid_view_demo_item_t *item;
    uint8_t selected;

    EGUI_UNUSED(stable_id);

    if (index >= context->item_count)
    {
        return;
    }

    item = &context->items[index];
    selected = item->stable_id == context->selected_id ? 1U : 0U;

    if (item->view_type == GRID_VIEW_DEMO_VIEWTYPE_HERO)
    {
        grid_view_demo_bind_hero_holder((grid_view_demo_hero_holder_t *)holder, item, index, selected);
    }
    else
    {
        grid_view_demo_bind_metric_holder((grid_view_demo_metric_holder_t *)holder, item, index, selected);
    }
}

static uint8_t grid_view_demo_should_keep_alive(void *data_model_context, egui_view_grid_view_holder_t *holder, uint32_t stable_id)
{
    grid_view_demo_context_t *context = (grid_view_demo_context_t *)data_model_context;
    int32_t index;

    EGUI_UNUSED(holder);

    index = grid_view_demo_find_index_by_stable_id(data_model_context, stable_id);
    if (index < 0)
    {
        return 0;
    }

    return (uint8_t)(context->items[index].view_type == GRID_VIEW_DEMO_VIEWTYPE_HERO && context->items[index].progress >= 80U);
}

static void grid_view_demo_save_holder_state(void *data_model_context, egui_view_grid_view_holder_t *holder, uint32_t stable_id)
{
    grid_view_demo_holder_state_t state;

    EGUI_UNUSED(data_model_context);

    memset(&state, 0, sizeof(state));
    if (holder->view_type == GRID_VIEW_DEMO_VIEWTYPE_HERO)
    {
        state.preview_hits = ((grid_view_demo_hero_holder_t *)holder)->preview_hits;
    }
    else
    {
        state.preview_hits = ((grid_view_demo_metric_holder_t *)holder)->preview_hits;
    }

    if (state.preview_hits == 0U)
    {
        egui_view_grid_view_remove_item_state_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id);
        return;
    }

    (void)egui_view_grid_view_write_item_state_for_view(holder->item_view, stable_id, &state, sizeof(state));
}

static void grid_view_demo_restore_holder_state(void *data_model_context, egui_view_grid_view_holder_t *holder, uint32_t stable_id)
{
    grid_view_demo_holder_state_t state;

    EGUI_UNUSED(data_model_context);

    memset(&state, 0, sizeof(state));
    if (egui_view_grid_view_read_item_state_for_view(holder->item_view, stable_id, &state, sizeof(state)) != sizeof(state))
    {
        state.preview_hits = 0U;
    }

    if (holder->view_type == GRID_VIEW_DEMO_VIEWTYPE_HERO)
    {
        grid_view_demo_set_hero_preview_hits((grid_view_demo_hero_holder_t *)holder, state.preview_hits);
    }
    else
    {
        grid_view_demo_set_metric_preview_hits((grid_view_demo_metric_holder_t *)holder, state.preview_hits);
    }
}

static const egui_view_grid_view_data_model_t grid_view_demo_data_model = {
        .get_count = grid_view_demo_get_count,
        .get_stable_id = grid_view_demo_get_stable_id,
        .find_index_by_stable_id = grid_view_demo_find_index_by_stable_id,
        .get_view_type = grid_view_demo_get_view_type,
        .measure_item_height = grid_view_demo_measure_item_height,
        .default_view_type = GRID_VIEW_DEMO_VIEWTYPE_METRIC,
};

static const egui_view_grid_view_holder_ops_t grid_view_demo_holder_ops = {
        .create_holder = grid_view_demo_create_holder,
        .destroy_holder = grid_view_demo_destroy_holder,
        .bind_holder = grid_view_demo_bind_holder,
        .unbind_holder = NULL,
        .should_keep_alive = grid_view_demo_should_keep_alive,
        .save_holder_state = grid_view_demo_save_holder_state,
        .restore_holder_state = grid_view_demo_restore_holder_state,
};

static int grid_view_demo_find_action_button_index(egui_view_t *self)
{
    uint8_t index;

    for (index = 0; index < 4U; index++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[index]))
        {
            return (int)index;
        }
    }

    for (index = 0; index < 3U; index++)
    {
        if (self == EGUI_VIEW_OF(&mode_buttons[index]))
        {
            return (int)(10 + index);
        }
    }

    return -1;
}

static void grid_view_demo_apply_column_count(uint8_t column_count)
{
    egui_view_grid_view_set_column_count(EGUI_VIEW_OF(&grid_view), column_count);
    grid_view_demo_refresh_mode_buttons();
    if (grid_view_demo_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_grid_view_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&grid_view), grid_view_demo_ctx.selected_id, 4);
    }
    grid_view_demo_refresh_header();
}

static void grid_view_demo_toggle_cb(egui_view_t *self, uint8_t is_toggled)
{
    egui_view_grid_view_entry_t entry;
    grid_view_demo_item_t *item;

    if (!egui_view_grid_view_resolve_item_by_view(EGUI_VIEW_OF(&grid_view), self, &entry))
    {
        return;
    }

    item = grid_view_demo_get_item(entry.index);
    if (item == NULL)
    {
        return;
    }

    item->armed = is_toggled ? 1U : 0U;
    item->progress = (uint8_t)(item->armed ? 78U : 34U);
    grid_view_demo_ctx.toggle_count++;
    grid_view_demo_set_selected(entry.stable_id, 0);
    egui_view_grid_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&grid_view), entry.stable_id);
    grid_view_demo_refresh_header();
}

static void grid_view_demo_switch_checked_cb(egui_view_t *self, int is_checked)
{
    egui_view_grid_view_entry_t entry;
    grid_view_demo_item_t *item;

    if (!egui_view_grid_view_resolve_item_by_view(EGUI_VIEW_OF(&grid_view), self, &entry))
    {
        return;
    }

    item = grid_view_demo_get_item(entry.index);
    if (item == NULL)
    {
        return;
    }

    item->armed = is_checked ? 1U : 0U;
    item->progress = (uint8_t)(item->armed ? 64U : 28U);
    grid_view_demo_ctx.switch_count++;
    grid_view_demo_set_selected(entry.stable_id, 0);
    egui_view_grid_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&grid_view), entry.stable_id);
    grid_view_demo_refresh_header();
}

static void grid_view_demo_preview_click_cb(egui_view_t *self)
{
    egui_view_grid_view_holder_t *holder;
    egui_view_grid_view_entry_t entry;

    if (!egui_view_grid_view_resolve_holder_by_view(EGUI_VIEW_OF(&grid_view), self, &holder, &entry))
    {
        return;
    }

    if (holder->view_type == GRID_VIEW_DEMO_VIEWTYPE_HERO)
    {
        grid_view_demo_hero_holder_t *typed_holder = (grid_view_demo_hero_holder_t *)holder;
        grid_view_demo_set_hero_preview_hits(typed_holder, (uint8_t)(typed_holder->preview_hits + 1U));
    }
    else
    {
        grid_view_demo_metric_holder_t *typed_holder = (grid_view_demo_metric_holder_t *)holder;
        grid_view_demo_set_metric_preview_hits(typed_holder, (uint8_t)(typed_holder->preview_hits + 1U));
    }

    grid_view_demo_ctx.preview_count++;
    grid_view_demo_set_selected(entry.stable_id, 0);
    grid_view_demo_refresh_header();
}

static void grid_view_demo_action_add(void)
{
    uint32_t index;

    if (grid_view_demo_ctx.item_count >= GRID_VIEW_DEMO_MAX_ITEMS)
    {
        return;
    }

    index = grid_view_demo_pick_target_index();
    if (grid_view_demo_ctx.item_count > 0U)
    {
        index++;
    }
    if (index > grid_view_demo_ctx.item_count)
    {
        index = grid_view_demo_ctx.item_count;
    }

    if (index < grid_view_demo_ctx.item_count)
    {
        memmove(&grid_view_demo_ctx.items[index + 1U], &grid_view_demo_ctx.items[index],
                (grid_view_demo_ctx.item_count - index) * sizeof(grid_view_demo_ctx.items[0]));
    }

    grid_view_demo_fill_item(&grid_view_demo_ctx.items[index], (uint8_t)((index & 1U) ? GRID_VIEW_DEMO_VIEWTYPE_HERO : GRID_VIEW_DEMO_VIEWTYPE_METRIC),
                             grid_view_demo_ctx.next_stable_id + index);
    grid_view_demo_ctx.item_count++;
    grid_view_demo_ctx.action_count++;
    grid_view_demo_ctx.mutation_cursor = index;
    egui_view_grid_view_notify_item_inserted(EGUI_VIEW_OF(&grid_view), index, 1);
    grid_view_demo_set_selected(grid_view_demo_ctx.items[index].stable_id, 1);
    grid_view_demo_refresh_header();
}

static void grid_view_demo_action_remove(void)
{
    uint32_t index;
    uint32_t stable_id;

    if (grid_view_demo_ctx.item_count == 0U)
    {
        return;
    }

    index = grid_view_demo_pick_target_index();
    stable_id = grid_view_demo_ctx.items[index].stable_id;
    egui_view_grid_view_remove_item_state_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id);

    if ((index + 1U) < grid_view_demo_ctx.item_count)
    {
        memmove(&grid_view_demo_ctx.items[index], &grid_view_demo_ctx.items[index + 1U],
                (grid_view_demo_ctx.item_count - index - 1U) * sizeof(grid_view_demo_ctx.items[0]));
    }

    grid_view_demo_ctx.item_count--;
    grid_view_demo_ctx.action_count++;
    if (grid_view_demo_ctx.item_count == 0U)
    {
        grid_view_demo_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
        grid_view_demo_ctx.mutation_cursor = 0U;
    }
    else
    {
        if (index >= grid_view_demo_ctx.item_count)
        {
            index = grid_view_demo_ctx.item_count - 1U;
        }
        grid_view_demo_ctx.selected_id = grid_view_demo_ctx.items[index].stable_id;
        grid_view_demo_ctx.mutation_cursor = index;
    }

    egui_view_grid_view_notify_item_removed(EGUI_VIEW_OF(&grid_view), index, 1);
    grid_view_demo_refresh_header();
}

static void grid_view_demo_action_patch(void)
{
    uint32_t index;
    grid_view_demo_item_t *item;
    int32_t before_height;
    int32_t after_height;

    if (grid_view_demo_ctx.item_count == 0U)
    {
        return;
    }

    index = grid_view_demo_pick_target_index();
    item = &grid_view_demo_ctx.items[index];
    before_height = grid_view_demo_measure_item_height_from_item(item, egui_view_grid_view_get_item_width(EGUI_VIEW_OF(&grid_view), index));
    item->expanded = item->expanded ? 0U : 1U;
    item->progress = (uint8_t)(22U + ((item->progress + 17U) % 72U));
    item->revision++;
    after_height = grid_view_demo_measure_item_height_from_item(item, egui_view_grid_view_get_item_width(EGUI_VIEW_OF(&grid_view), index));
    grid_view_demo_ctx.action_count++;
    grid_view_demo_ctx.mutation_cursor = index;
    grid_view_demo_set_selected(item->stable_id, 0);

    if (before_height != after_height)
    {
        egui_view_grid_view_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&grid_view), item->stable_id);
    }
    else
    {
        egui_view_grid_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&grid_view), item->stable_id);
    }

    grid_view_demo_refresh_header();
}

static void grid_view_demo_action_jump(void)
{
    uint32_t index;

    if (grid_view_demo_ctx.item_count == 0U)
    {
        return;
    }

    if (grid_view_demo_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_grid_view_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&grid_view), grid_view_demo_ctx.selected_id, 4);
        grid_view_demo_refresh_header();
        return;
    }

    grid_view_demo_ctx.mutation_cursor = (grid_view_demo_ctx.mutation_cursor + 7U) % grid_view_demo_ctx.item_count;
    index = grid_view_demo_ctx.mutation_cursor;
    grid_view_demo_set_selected(grid_view_demo_ctx.items[index].stable_id, 1);
    grid_view_demo_ctx.action_count++;
    grid_view_demo_refresh_header();
}

static void grid_view_demo_action_click_cb(egui_view_t *self)
{
    switch (grid_view_demo_find_action_button_index(self))
    {
    case 0:
        grid_view_demo_action_add();
        break;
    case 1:
        grid_view_demo_action_remove();
        break;
    case 2:
        grid_view_demo_action_patch();
        break;
    case 3:
        grid_view_demo_action_jump();
        break;
    case 10:
        grid_view_demo_apply_column_count(2U);
        break;
    case 11:
        grid_view_demo_apply_column_count(3U);
        break;
    case 12:
        grid_view_demo_apply_column_count(4U);
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

static grid_view_demo_hero_holder_t *grid_view_demo_find_first_hero_holder(void)
{
    uint32_t index;

    for (index = 0; index < grid_view_demo_ctx.item_count; index++)
    {
        egui_view_grid_view_holder_t *holder;

        if (grid_view_demo_ctx.items[index].view_type != GRID_VIEW_DEMO_VIEWTYPE_HERO)
        {
            continue;
        }

        holder = egui_view_grid_view_find_holder_by_stable_id(EGUI_VIEW_OF(&grid_view), grid_view_demo_ctx.items[index].stable_id);
        if (holder != NULL)
        {
            return (grid_view_demo_hero_holder_t *)holder;
        }
    }

    return NULL;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    static uint32_t target_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    static uint8_t expected_armed = 0U;
    static uint8_t expected_expanded = 0U;
    int first_call = (action_index != last_action);
    grid_view_demo_hero_holder_t *hero_holder;
    grid_view_demo_item_t *item;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            target_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
            expected_armed = 0U;
            expected_expanded = 0U;
            if (grid_view_demo_find_first_hero_holder() == NULL)
            {
                report_runtime_failure("hero grid holder was not materialized");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        hero_holder = grid_view_demo_find_first_hero_holder();
        if (hero_holder == NULL)
        {
            report_runtime_failure("hero tile was not visible");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (first_call)
        {
            target_stable_id = hero_holder->base.bound_stable_id;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&hero_holder->preview_button), 220);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 140);
        return true;
    case 3:
        hero_holder = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID
                              ? (grid_view_demo_hero_holder_t *)egui_view_grid_view_find_holder_by_stable_id(EGUI_VIEW_OF(&grid_view), target_stable_id)
                              : grid_view_demo_find_first_hero_holder();
        item = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID ? grid_view_demo_find_item_by_stable_id_internal(target_stable_id) : NULL;
        if (first_call && (hero_holder == NULL || hero_holder->preview_hits != 1U))
        {
            report_runtime_failure("hero holder local preview state did not update");
        }
        if (hero_holder == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (item == NULL)
        {
            report_runtime_failure("hero tile data model entry was not found");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (first_call)
        {
            expected_armed = item->armed ? 0U : 1U;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&hero_holder->arm_toggle), 220);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 140);
        return true;
    case 5:
        item = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID ? grid_view_demo_find_item_by_stable_id_internal(target_stable_id) : NULL;
        if (first_call && (item == NULL || item->armed != expected_armed))
        {
            report_runtime_failure("hero toggle did not update data model");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&mode_buttons[1]), 220);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 140);
        return true;
    case 7:
        if (first_call && egui_view_grid_view_get_column_count(EGUI_VIEW_OF(&grid_view)) != 3U)
        {
            report_runtime_failure("column mode did not switch to 3 columns");
        }
        item = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID ? grid_view_demo_find_item_by_stable_id_internal(target_stable_id) : NULL;
        if (item == NULL)
        {
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (first_call)
        {
            expected_expanded = item->expanded ? 0U : 1U;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[2]), 220);
        return true;
    case 8:
        EGUI_SIM_SET_WAIT(p_action, 140);
        return true;
    case 9:
        item = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID ? grid_view_demo_find_item_by_stable_id_internal(target_stable_id) : NULL;
        if (first_call && (item == NULL || item->expanded != expected_expanded))
        {
            report_runtime_failure("patch action did not resize selected tile");
        }
        p_action->type = EGUI_SIM_ACTION_DRAG;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT - 24;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = GRID_VIEW_DEMO_GRID_Y + 24;
        p_action->steps = 10;
        p_action->interval_ms = 320;
        return true;
    case 10:
        EGUI_SIM_SET_WAIT(p_action, 140);
        return true;
    case 11:
        if (first_call && egui_view_grid_view_get_scroll_y(EGUI_VIEW_OF(&grid_view)) <= 0)
        {
            report_runtime_failure("grid view did not scroll");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[3]), 220);
        return true;
    case 12:
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 13:
        hero_holder = target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID
                              ? (grid_view_demo_hero_holder_t *)egui_view_grid_view_find_holder_by_stable_id(EGUI_VIEW_OF(&grid_view), target_stable_id)
                              : grid_view_demo_find_first_hero_holder();
        if (first_call)
        {
            if (hero_holder == NULL || hero_holder->preview_hits != 1U)
            {
                report_runtime_failure("state cache did not restore hero preview state");
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
    uint8_t index;
    const egui_view_grid_view_setup_t setup = {
            .params = &grid_view_demo_params,
            .data_model = &grid_view_demo_data_model,
            .holder_ops = &grid_view_demo_holder_ops,
            .data_model_context = &grid_view_demo_ctx,
            .state_cache_max_entries = GRID_VIEW_DEMO_STATE_CACHE_COUNT,
            .state_cache_max_bytes = GRID_VIEW_DEMO_STATE_CACHE_COUNT * (uint32_t)sizeof(grid_view_demo_holder_state_t),
    };

    grid_view_demo_reset_model();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0U;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&grid_view_demo_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&header_card), &grid_view_demo_header_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_card), 1, EGUI_COLOR_HEX(0xD1DEE7));

    grid_view_demo_init_label(&header_title, 12, 10, GRID_VIEW_DEMO_CONTENT_W - 24, 14, GRID_VIEW_DEMO_FONT_HEADER, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                              EGUI_COLOR_HEX(0x23384A));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_title));

    grid_view_demo_init_label(&header_detail, 12, 28, GRID_VIEW_DEMO_CONTENT_W - 24, 12, GRID_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                              EGUI_COLOR_HEX(0x556A79));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_detail));

    grid_view_demo_init_label(&header_hint, 12, 44, GRID_VIEW_DEMO_CONTENT_W - 24, 12, GRID_VIEW_DEMO_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                              EGUI_COLOR_HEX(0x6B7C8A));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_hint));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &grid_view_demo_toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD1DEE7));

    for (index = 0; index < 4U; index++)
    {
        grid_view_demo_init_button(&action_buttons[index], (egui_dim_t)(10 + index * (GRID_VIEW_DEMO_ACTION_W + GRID_VIEW_DEMO_ACTION_GAP)), 6,
                                   GRID_VIEW_DEMO_ACTION_W, GRID_VIEW_DEMO_ACTION_H, grid_view_demo_action_names[index], grid_view_demo_action_click_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[index]));
    }

    for (index = 0; index < 3U; index++)
    {
        grid_view_demo_init_button(&mode_buttons[index], (egui_dim_t)(10 + index * (GRID_VIEW_DEMO_MODE_W + GRID_VIEW_DEMO_ACTION_GAP)), 30,
                                   GRID_VIEW_DEMO_MODE_W, GRID_VIEW_DEMO_MODE_H, grid_view_demo_mode_names[index], grid_view_demo_action_click_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&mode_buttons[index]));
    }

    egui_view_grid_view_init_with_setup(EGUI_VIEW_OF(&grid_view), &setup);
    egui_view_set_background(EGUI_VIEW_OF(&grid_view), EGUI_BG_OF(&grid_view_demo_grid_bg));

    grid_view_demo_refresh_mode_buttons();
    grid_view_demo_refresh_header();

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&header_card));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}
