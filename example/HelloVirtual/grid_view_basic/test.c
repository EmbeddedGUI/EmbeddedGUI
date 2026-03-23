#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "uicode.h"

#define GRID_VIEW_BASIC_ITEM_COUNT    30U
#define GRID_VIEW_BASIC_TITLE_LEN     24
#define GRID_VIEW_BASIC_DETAIL_LEN    32
#define GRID_VIEW_BASIC_TOGGLE_LEN    16
#define GRID_VIEW_BASIC_TILE_HEIGHT   88
#define GRID_VIEW_BASIC_ACTIVE_HEIGHT 104

#define GRID_VIEW_BASIC_MARGIN_X 8
#define GRID_VIEW_BASIC_TOP_Y    8
#define GRID_VIEW_BASIC_GRID_Y   GRID_VIEW_BASIC_TOP_Y
#define GRID_VIEW_BASIC_GRID_W   (EGUI_CONFIG_SCEEN_WIDTH - GRID_VIEW_BASIC_MARGIN_X * 2)
#define GRID_VIEW_BASIC_GRID_H   (EGUI_CONFIG_SCEEN_HEIGHT - GRID_VIEW_BASIC_GRID_Y - 8)

#define GRID_VIEW_BASIC_FONT_TITLE ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define GRID_VIEW_BASIC_FONT_BODY  ((const egui_font_t *)&egui_res_font_montserrat_8_4)

typedef struct grid_view_basic_item grid_view_basic_item_t;
typedef struct grid_view_basic_context grid_view_basic_context_t;
typedef struct grid_view_basic_holder grid_view_basic_holder_t;

struct grid_view_basic_item
{
    uint32_t stable_id;
    uint8_t active;
    uint8_t progress;
};

struct grid_view_basic_context
{
    grid_view_basic_item_t items[GRID_VIEW_BASIC_ITEM_COUNT];
};

struct grid_view_basic_holder
{
    egui_view_grid_view_holder_t base;
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t detail;
    egui_view_progress_bar_t progress_bar;
    egui_view_toggle_button_t toggle_button;
    char title_text[GRID_VIEW_BASIC_TITLE_LEN];
    char detail_text[GRID_VIEW_BASIC_DETAIL_LEN];
    char toggle_text[GRID_VIEW_BASIC_TOGGLE_LEN];
};

static egui_view_t background_view;
static egui_view_grid_view_t grid_view;
static grid_view_basic_context_t grid_view_basic_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(grid_view_basic_toggle_params, 0, 0, 76, 28, "Idle", 0);
EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT(grid_view_basic_progress_params, 0, 0, 80, 6, 20);

static const egui_view_grid_view_params_t grid_view_basic_params = {
        .region = {{GRID_VIEW_BASIC_MARGIN_X, GRID_VIEW_BASIC_GRID_Y}, {GRID_VIEW_BASIC_GRID_W, GRID_VIEW_BASIC_GRID_H}},
        .column_count = 2,
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 2,
        .column_spacing = 6,
        .row_spacing = 6,
        .estimated_item_height = GRID_VIEW_BASIC_TILE_HEIGHT,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(grid_view_basic_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF4F7), EGUI_COLOR_HEX(0xDDE8F0),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(grid_view_basic_screen_bg_params, &grid_view_basic_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(grid_view_basic_screen_bg, &grid_view_basic_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(grid_view_basic_grid_bg_param, EGUI_COLOR_HEX(0xF8FBFD), EGUI_ALPHA_100, 14, 1,
                                                        EGUI_COLOR_HEX(0xC9D8E4), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(grid_view_basic_grid_bg_params, &grid_view_basic_grid_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_view_basic_grid_bg, &grid_view_basic_grid_bg_params);

static void grid_view_basic_toggle_cb(egui_view_t *self, uint8_t is_toggled);

static int32_t grid_view_basic_get_item_height_from_state(const grid_view_basic_item_t *item)
{
    return item != NULL && item->active ? GRID_VIEW_BASIC_ACTIVE_HEIGHT : GRID_VIEW_BASIC_TILE_HEIGHT;
}

static void grid_view_basic_init_items(void)
{
    uint32_t index;

    memset(&grid_view_basic_ctx, 0, sizeof(grid_view_basic_ctx));
    for (index = 0; index < GRID_VIEW_BASIC_ITEM_COUNT; index++)
    {
        grid_view_basic_ctx.items[index].stable_id = 2000U + index;
        grid_view_basic_ctx.items[index].active = (uint8_t)((index % 4U) == 1U);
        grid_view_basic_ctx.items[index].progress = (uint8_t)(20U + ((index * 11U) % 70U));
    }
}

static void grid_view_basic_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const egui_font_t *font,
                                       uint8_t align, egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), width, height);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), align);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static uint32_t grid_view_basic_get_count(void *data_model_context)
{
    EGUI_UNUSED(data_model_context);
    return GRID_VIEW_BASIC_ITEM_COUNT;
}

static uint32_t grid_view_basic_get_stable_id(void *data_model_context, uint32_t index)
{
    grid_view_basic_context_t *context = (grid_view_basic_context_t *)data_model_context;
    return index < GRID_VIEW_BASIC_ITEM_COUNT ? context->items[index].stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t grid_view_basic_find_index_by_stable_id(void *data_model_context, uint32_t stable_id)
{
    grid_view_basic_context_t *context = (grid_view_basic_context_t *)data_model_context;
    uint32_t index;

    for (index = 0; index < GRID_VIEW_BASIC_ITEM_COUNT; index++)
    {
        if (context->items[index].stable_id == stable_id)
        {
            return (int32_t)index;
        }
    }

    return -1;
}

static int32_t grid_view_basic_measure_item_height(void *data_model_context, uint32_t index, int32_t width_hint)
{
    grid_view_basic_context_t *context = (grid_view_basic_context_t *)data_model_context;

    EGUI_UNUSED(width_hint);

    if (index >= GRID_VIEW_BASIC_ITEM_COUNT)
    {
        return GRID_VIEW_BASIC_TILE_HEIGHT;
    }

    return grid_view_basic_get_item_height_from_state(&context->items[index]);
}

static void grid_view_basic_sync_toggle(grid_view_basic_holder_t *holder, uint8_t is_toggled)
{
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&holder->toggle_button), NULL);
    egui_view_toggle_button_set_toggled(EGUI_VIEW_OF(&holder->toggle_button), is_toggled);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&holder->toggle_button), grid_view_basic_toggle_cb);
}

static egui_view_grid_view_holder_t *grid_view_basic_create_holder(void *data_model_context, uint16_t view_type)
{
    grid_view_basic_holder_t *holder;

    EGUI_UNUSED(data_model_context);
    EGUI_UNUSED(view_type);

    holder = (grid_view_basic_holder_t *)egui_malloc(sizeof(grid_view_basic_holder_t));
    if (holder == NULL)
    {
        return NULL;
    }

    memset(holder, 0, sizeof(*holder));
    egui_view_group_init(EGUI_VIEW_OF(&holder->root));

    egui_view_card_init(EGUI_VIEW_OF(&holder->card));
    egui_view_set_position(EGUI_VIEW_OF(&holder->card), 3, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&holder->root), EGUI_VIEW_OF(&holder->card));

    grid_view_basic_init_label(&holder->title, 10, 10, 80, 14, GRID_VIEW_BASIC_FONT_TITLE, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x23384A));
    egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->title));

    grid_view_basic_init_label(&holder->detail, 10, 30, 80, 12, GRID_VIEW_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x5A6F82));
    egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->detail));

    egui_view_progress_bar_init_with_params(EGUI_VIEW_OF(&holder->progress_bar), &grid_view_basic_progress_params);
    holder->progress_bar.is_show_control = 0;
    egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->progress_bar));

    egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&holder->toggle_button), &grid_view_basic_toggle_params);
    egui_view_toggle_button_set_icon(EGUI_VIEW_OF(&holder->toggle_button), EGUI_ICON_MS_VISIBILITY);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&holder->toggle_button), EGUI_FONT_ICON_MS_16);
    egui_view_toggle_button_set_icon_text_gap(EGUI_VIEW_OF(&holder->toggle_button), 4);
    egui_view_toggle_button_set_text_color(EGUI_VIEW_OF(&holder->toggle_button), EGUI_COLOR_WHITE);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&holder->toggle_button), grid_view_basic_toggle_cb);
    egui_view_card_add_child(EGUI_VIEW_OF(&holder->card), EGUI_VIEW_OF(&holder->toggle_button));

    holder->base.item_view = EGUI_VIEW_OF(&holder->root);
    return &holder->base;
}

static void grid_view_basic_destroy_holder(void *data_model_context, egui_view_grid_view_holder_t *holder, uint16_t view_type)
{
    EGUI_UNUSED(data_model_context);
    EGUI_UNUSED(view_type);
    egui_free(holder);
}

static void grid_view_basic_bind_holder(void *data_model_context, egui_view_grid_view_holder_t *holder, uint32_t index, uint32_t stable_id)
{
    grid_view_basic_context_t *context = (grid_view_basic_context_t *)data_model_context;
    grid_view_basic_holder_t *typed_holder = (grid_view_basic_holder_t *)holder;
    grid_view_basic_item_t *item;
    egui_dim_t root_w;
    egui_dim_t root_h;
    egui_dim_t card_w;
    egui_dim_t card_h;
    egui_color_t card_fill;
    egui_color_t card_border;

    EGUI_UNUSED(stable_id);

    if (index >= GRID_VIEW_BASIC_ITEM_COUNT)
    {
        return;
    }

    item = &context->items[index];
    root_w = holder->host_view->region.size.width;
    root_h = holder->host_view->region.size.height;
    card_w = root_w - 6;
    card_h = root_h - 6;

    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->root), root_w, root_h);
    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->card), card_w, card_h);

    snprintf(typed_holder->title_text, sizeof(typed_holder->title_text), "Tile %02lu", (unsigned long)(index + 1U));
    snprintf(typed_holder->detail_text, sizeof(typed_holder->detail_text), "%s  %u%%", item->active ? "Live" : "Idle", (unsigned)item->progress);
    snprintf(typed_holder->toggle_text, sizeof(typed_holder->toggle_text), "%s", item->active ? "Live" : "Idle");

    if (item->active)
    {
        card_fill = EGUI_COLOR_HEX(0xE7F0FA);
        card_border = EGUI_COLOR_HEX(0xB7CCE2);
        typed_holder->progress_bar.progress_color = EGUI_COLOR_HEX(0x326993);
        typed_holder->progress_bar.bk_color = EGUI_COLOR_HEX(0xCFE0EF);
    }
    else
    {
        card_fill = EGUI_COLOR_WHITE;
        card_border = EGUI_COLOR_HEX(0xC8D7E3);
        typed_holder->progress_bar.progress_color = EGUI_COLOR_HEX(0x4C8A63);
        typed_holder->progress_bar.bk_color = EGUI_COLOR_HEX(0xD7E5DD);
    }

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&typed_holder->card), card_fill, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&typed_holder->card), 1, card_border);

    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->title), card_w - 20, 14);
    egui_view_label_set_text(EGUI_VIEW_OF(&typed_holder->title), typed_holder->title_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&typed_holder->title), EGUI_COLOR_HEX(0x23384A), EGUI_ALPHA_100);

    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->detail), card_w - 20, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&typed_holder->detail), typed_holder->detail_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&typed_holder->detail), EGUI_COLOR_HEX(0x5A6F82), EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&typed_holder->progress_bar), 10, card_h - 52);
    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->progress_bar), card_w - 20, 6);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&typed_holder->progress_bar), item->progress);

    egui_view_set_position(EGUI_VIEW_OF(&typed_holder->toggle_button), 10, card_h - 36);
    egui_view_set_size(EGUI_VIEW_OF(&typed_holder->toggle_button), card_w - 20, 28);
    egui_view_toggle_button_set_text(EGUI_VIEW_OF(&typed_holder->toggle_button), typed_holder->toggle_text);
    grid_view_basic_sync_toggle(typed_holder, item->active);
}

static const egui_view_grid_view_data_model_t grid_view_basic_data_model = {
        .get_count = grid_view_basic_get_count,
        .get_stable_id = grid_view_basic_get_stable_id,
        .find_index_by_stable_id = grid_view_basic_find_index_by_stable_id,
        .get_view_type = NULL,
        .measure_item_height = grid_view_basic_measure_item_height,
        .default_view_type = 0,
};

static const egui_view_grid_view_holder_ops_t grid_view_basic_holder_ops = {
        .create_holder = grid_view_basic_create_holder,
        .destroy_holder = grid_view_basic_destroy_holder,
        .bind_holder = grid_view_basic_bind_holder,
        .unbind_holder = NULL,
        .should_keep_alive = NULL,
        .save_holder_state = NULL,
        .restore_holder_state = NULL,
};

static void grid_view_basic_toggle_cb(egui_view_t *self, uint8_t is_toggled)
{
    egui_view_grid_view_entry_t entry;
    int32_t old_height;
    int32_t new_height;

    if (!egui_view_grid_view_resolve_item_by_view(EGUI_VIEW_OF(&grid_view), self, &entry))
    {
        return;
    }

    old_height = grid_view_basic_get_item_height_from_state(&grid_view_basic_ctx.items[entry.index]);
    grid_view_basic_ctx.items[entry.index].active = is_toggled ? 1U : 0U;
    grid_view_basic_ctx.items[entry.index].progress = (uint8_t)(is_toggled ? 80U : 34U);
    new_height = grid_view_basic_get_item_height_from_state(&grid_view_basic_ctx.items[entry.index]);

    if (old_height != new_height)
    {
        egui_view_grid_view_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&grid_view), entry.stable_id);
    }
    else
    {
        egui_view_grid_view_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&grid_view), entry.stable_id);
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

static grid_view_basic_holder_t *grid_view_basic_find_holder_by_index(uint32_t index)
{
    egui_view_grid_view_holder_t *holder;

    if (index >= GRID_VIEW_BASIC_ITEM_COUNT)
    {
        return NULL;
    }

    holder = egui_view_grid_view_find_holder_by_stable_id(EGUI_VIEW_OF(&grid_view), grid_view_basic_ctx.items[index].stable_id);
    return (grid_view_basic_holder_t *)holder;
}

static void grid_view_basic_set_scroll_action(egui_sim_action_t *p_action, uint32_t interval_ms)
{
    egui_region_t *region = &EGUI_VIEW_OF(&grid_view)->region_screen;

    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = (egui_dim_t)(region->location.x + region->size.width / 2);
    p_action->y1 = (egui_dim_t)(region->location.y + region->size.height - 24);
    p_action->x2 = p_action->x1;
    p_action->y2 = (egui_dim_t)(region->location.y + 36);
    p_action->steps = 10;
    p_action->interval_ms = interval_ms;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    grid_view_basic_holder_t *holder;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            if (grid_view_basic_find_holder_by_index(0) == NULL)
            {
                report_runtime_failure("initial grid holders were not materialized");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        holder = grid_view_basic_find_holder_by_index(0);
        if (holder == NULL)
        {
            report_runtime_failure("first grid tile was not visible");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&holder->toggle_button), 220);
        return true;
    case 2:
        if (first_call && grid_view_basic_ctx.items[0].active != 1U)
        {
            report_runtime_failure("first tile toggle did not update data model");
        }
        grid_view_basic_set_scroll_action(p_action, 320);
        return true;
    case 3:
        if (first_call)
        {
            if (egui_view_grid_view_get_scroll_y(EGUI_VIEW_OF(&grid_view)) <= 0)
            {
                report_runtime_failure("grid view did not scroll");
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
    const egui_view_grid_view_setup_t setup = {
            .params = &grid_view_basic_params,
            .data_model = &grid_view_basic_data_model,
            .holder_ops = &grid_view_basic_holder_ops,
            .data_model_context = &grid_view_basic_ctx,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };

    grid_view_basic_init_items();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0U;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&grid_view_basic_screen_bg));

    egui_view_grid_view_init_with_setup(EGUI_VIEW_OF(&grid_view), &setup);
    egui_view_set_background(EGUI_VIEW_OF(&grid_view), EGUI_BG_OF(&grid_view_basic_grid_bg));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid_view));
}
