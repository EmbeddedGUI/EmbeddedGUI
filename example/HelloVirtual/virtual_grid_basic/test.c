#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "uicode.h"

#define GRID_BASIC_ITEM_COUNT             120U
#define GRID_BASIC_STABLE_BASE            5000U
#define GRID_BASIC_INVALID_INDEX          0xFFFFFFFFUL
#define GRID_BASIC_CLICK_VERIFY_RETRY_MAX 5U
#define GRID_BASIC_PATCH_VERIFY_RETRY_MAX 3U
#define GRID_BASIC_TITLE_LEN              28
#define GRID_BASIC_BADGE_LEN              16
#define GRID_BASIC_META_LEN               24

#define GRID_BASIC_MARGIN_X     8
#define GRID_BASIC_TOP_Y        8
#define GRID_BASIC_HEADER_W     (EGUI_CONFIG_SCEEN_WIDTH - GRID_BASIC_MARGIN_X * 2)
#define GRID_BASIC_HEADER_H     0
#define GRID_BASIC_TOOLBAR_Y    GRID_BASIC_TOP_Y
#define GRID_BASIC_TOOLBAR_H    34
#define GRID_BASIC_VIEW_Y       (GRID_BASIC_TOOLBAR_Y + GRID_BASIC_TOOLBAR_H + 6)
#define GRID_BASIC_VIEW_W       GRID_BASIC_HEADER_W
#define GRID_BASIC_VIEW_H       (EGUI_CONFIG_SCEEN_HEIGHT - GRID_BASIC_VIEW_Y - 8)
#define GRID_BASIC_ACTION_GAP   6
#define GRID_BASIC_ACTION_W     ((GRID_BASIC_HEADER_W - 20 - GRID_BASIC_ACTION_GAP * 2) / 3)
#define GRID_BASIC_CARD_INSET_X 3
#define GRID_BASIC_CARD_INSET_Y 4
#define GRID_BASIC_KEEP_INSET   6

#define GRID_BASIC_FONT_TITLE ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define GRID_BASIC_FONT_BODY  ((const egui_font_t *)&egui_res_font_montserrat_8_4)

enum
{
    GRID_BASIC_VARIANT_COMPACT = 0,
    GRID_BASIC_VARIANT_STANDARD,
    GRID_BASIC_VARIANT_TALL,
    GRID_BASIC_VARIANT_COUNT,
};

enum
{
    GRID_BASIC_ACTION_PATCH = 0,
    GRID_BASIC_ACTION_COLS,
    GRID_BASIC_ACTION_RESET,
    GRID_BASIC_ACTION_COUNT,
};

typedef struct grid_basic_item grid_basic_item_t;
typedef struct grid_basic_view grid_basic_view_t;
typedef struct grid_basic_context grid_basic_context_t;
#if EGUI_CONFIG_RECORDING_TEST
typedef struct grid_basic_visible_summary grid_basic_visible_summary_t;
#endif

struct grid_basic_item
{
    uint32_t stable_id;
    uint8_t variant;
    uint8_t progress;
    uint8_t active;
    uint8_t revision;
};

struct grid_basic_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t badge;
    egui_view_label_t meta;
    egui_view_progress_bar_t progress;
    char title_text[GRID_BASIC_TITLE_LEN];
    char badge_text[GRID_BASIC_BADGE_LEN];
    char meta_text[GRID_BASIC_META_LEN];
};

struct grid_basic_context
{
    grid_basic_item_t items[GRID_BASIC_ITEM_COUNT];
    uint32_t selected_id;
    uint32_t last_clicked_index;
    uint32_t click_count;
    uint32_t patch_count;
};

#if EGUI_CONFIG_RECORDING_TEST
struct grid_basic_visible_summary
{
    uint32_t first_index;
    uint8_t visible_count;
    uint8_t has_first;
};
#endif

static const char *grid_basic_action_names[GRID_BASIC_ACTION_COUNT] = {"Patch", "Cols", "Reset"};
static const char *grid_basic_variant_names[GRID_BASIC_VARIANT_COUNT] = {"Compact", "Standard", "Tall"};

static egui_view_t background_view;
static egui_view_card_t toolbar_card;
static egui_view_button_t action_buttons[GRID_BASIC_ACTION_COUNT];
static egui_view_virtual_grid_t grid_view;
static grid_basic_context_t grid_basic_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
static uint8_t recording_click_verify_retry;
static uint8_t recording_patch_verify_retry;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(grid_basic_toolbar_card_params, GRID_BASIC_MARGIN_X, GRID_BASIC_TOOLBAR_Y, GRID_BASIC_HEADER_W, GRID_BASIC_TOOLBAR_H, 12);

static const egui_view_virtual_grid_params_t grid_basic_view_params = {
        .region = {{GRID_BASIC_MARGIN_X, GRID_BASIC_VIEW_Y}, {GRID_BASIC_VIEW_W, GRID_BASIC_VIEW_H}},
        .column_count = 2,
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 2,
        .column_spacing = 6,
        .row_spacing = 6,
        .estimated_item_height = 76,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(grid_basic_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF4F7), EGUI_COLOR_HEX(0xDCE7EF),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(grid_basic_screen_bg_params, &grid_basic_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(grid_basic_screen_bg, &grid_basic_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(grid_basic_view_bg_param, EGUI_COLOR_HEX(0xF8FBFD), EGUI_ALPHA_100, 14, 1, EGUI_COLOR_HEX(0xC9D8E3),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(grid_basic_view_bg_params, &grid_basic_view_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_basic_view_bg, &grid_basic_view_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(grid_basic_patch_bg_param, EGUI_COLOR_HEX(0xE6F6EF), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xA7D4BF),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(grid_basic_patch_bg_params, &grid_basic_patch_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_basic_patch_bg, &grid_basic_patch_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(grid_basic_cols_bg_param, EGUI_COLOR_HEX(0xE7F0FA), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xACC4DB),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(grid_basic_cols_bg_params, &grid_basic_cols_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_basic_cols_bg, &grid_basic_cols_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(grid_basic_reset_bg_param, EGUI_COLOR_HEX(0xF9E9E4), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xD4B2A9),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(grid_basic_reset_bg_params, &grid_basic_reset_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(grid_basic_reset_bg, &grid_basic_reset_bg_params);

static uint32_t grid_basic_ds_get_count(void *context);
static uint32_t grid_basic_ds_get_stable_id(void *context, uint32_t index);
static int32_t grid_basic_ds_find_index_by_stable_id(void *context, uint32_t stable_id);
static int32_t grid_basic_ds_measure_item_height(void *context, uint32_t index, int32_t width_hint);
static egui_view_t *grid_basic_ds_create_item_view(void *context, uint16_t view_type);
static void grid_basic_ds_destroy_item_view(void *context, egui_view_t *view, uint16_t view_type);
static void grid_basic_ds_bind_item_view(void *context, egui_view_t *view, uint32_t index, uint32_t stable_id);
static uint8_t grid_basic_ds_should_keep_alive(void *context, egui_view_t *view, uint32_t stable_id);

static const egui_view_virtual_grid_data_source_t grid_basic_data_source = {
        .get_count = grid_basic_ds_get_count,
        .get_stable_id = grid_basic_ds_get_stable_id,
        .find_index_by_stable_id = grid_basic_ds_find_index_by_stable_id,
        .get_item_view_type = NULL,
        .measure_item_height = grid_basic_ds_measure_item_height,
        .create_item_view = grid_basic_ds_create_item_view,
        .destroy_item_view = grid_basic_ds_destroy_item_view,
        .bind_item_view = grid_basic_ds_bind_item_view,
        .unbind_item_view = NULL,
        .should_keep_alive = grid_basic_ds_should_keep_alive,
        .save_item_state = NULL,
        .restore_item_state = NULL,
        .default_item_view_type = 0,
};

static grid_basic_item_t *grid_basic_get_item(uint32_t index)
{
    if (index >= GRID_BASIC_ITEM_COUNT)
    {
        return NULL;
    }

    return &grid_basic_ctx.items[index];
}

static int32_t grid_basic_find_index_by_stable_id(uint32_t stable_id)
{
    uint32_t i;

    for (i = 0; i < GRID_BASIC_ITEM_COUNT; i++)
    {
        if (grid_basic_ctx.items[i].stable_id == stable_id)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static void grid_basic_abort_motion(void)
{
    egui_scroller_about_animation(&grid_view.base.base.scroller);
    grid_view.base.base.is_begin_dragged = 0U;
}

static uint8_t grid_basic_get_column_count(void)
{
    return egui_view_virtual_grid_get_column_count(EGUI_VIEW_OF(&grid_view));
}

static int32_t grid_basic_get_item_width_hint(uint32_t index)
{
    int32_t width = egui_view_virtual_grid_get_item_width(EGUI_VIEW_OF(&grid_view), index);

    if (width > 0)
    {
        return width;
    }

    switch (grid_basic_get_column_count())
    {
    case 4:
        return 52;
    case 3:
        return 72;
    default:
        return 108;
    }
}

static int32_t grid_basic_measure_item_height_with_state(const grid_basic_item_t *item, int32_t width_hint, uint8_t selected)
{
    int32_t height;

    if (item == NULL)
    {
        return 72;
    }

    switch (item->variant)
    {
    case GRID_BASIC_VARIANT_TALL:
        height = width_hint < 60 ? 68 : 96;
        break;
    case GRID_BASIC_VARIANT_STANDARD:
        height = width_hint < 60 ? 56 : 80;
        break;
    default:
        height = width_hint < 60 ? 48 : 66;
        break;
    }

    if (item->active)
    {
        height += width_hint < 60 ? 4 : 6;
    }
    if (selected)
    {
        height += width_hint < 60 ? 8 : 10;
    }

    return height;
}

static void grid_basic_init_items(void)
{
    uint32_t i;

    for (i = 0; i < GRID_BASIC_ITEM_COUNT; i++)
    {
        grid_basic_item_t *item = &grid_basic_ctx.items[i];

        item->stable_id = GRID_BASIC_STABLE_BASE + i;
        item->variant = (uint8_t)(i % GRID_BASIC_VARIANT_COUNT);
        item->progress = (uint8_t)(18U + ((i * 13U) % 78U));
        item->active = (uint8_t)((i % 4U) == 1U);
        item->revision = (uint8_t)(i % 6U);
    }

    grid_basic_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    grid_basic_ctx.last_clicked_index = GRID_BASIC_INVALID_INDEX;
    grid_basic_ctx.click_count = 0U;
    grid_basic_ctx.patch_count = 0U;
}

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t grid_basic_visible_item_visitor(egui_view_t *self, const egui_view_virtual_grid_slot_t *slot, const egui_view_virtual_grid_entry_t *entry,
                                               egui_view_t *item_view, void *context)
{
    grid_basic_visible_summary_t *summary = (grid_basic_visible_summary_t *)context;

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

static void grid_basic_mark_selected(uint32_t stable_id)
{
    uint32_t previous_id = grid_basic_ctx.selected_id;

    if (previous_id == stable_id)
    {
        return;
    }

    grid_basic_ctx.selected_id = stable_id;
    if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_grid_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&grid_view), previous_id);
    }
    if (stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_grid_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id);
    }
}

static uint8_t grid_basic_resolve_item_from_any_view(egui_view_t *view, egui_view_virtual_grid_entry_t *entry)
{
    egui_view_t *cursor = view;

    while (cursor != NULL)
    {
        if (egui_view_virtual_grid_resolve_item_by_view(EGUI_VIEW_OF(&grid_view), cursor, entry))
        {
            return 1;
        }
        cursor = EGUI_VIEW_PARENT(cursor);
    }

    return 0;
}

static void grid_basic_item_click_cb(egui_view_t *self)
{
    egui_view_virtual_grid_entry_t entry;

    if (!grid_basic_resolve_item_from_any_view(self, &entry))
    {
        return;
    }

    grid_basic_ctx.last_clicked_index = entry.index;
    grid_basic_ctx.click_count++;
    grid_basic_mark_selected(entry.stable_id);
}

static void grid_basic_patch_selected(void)
{
    int32_t index = grid_basic_find_index_by_stable_id(grid_basic_ctx.selected_id);
    grid_basic_item_t *item;
    int32_t width_hint;
    int32_t old_height;
    int32_t new_height;

    if (index < 0)
    {
        index = 0;
        grid_basic_mark_selected(grid_basic_ctx.items[0].stable_id);
    }

    item = grid_basic_get_item((uint32_t)index);
    if (item == NULL)
    {
        return;
    }

    width_hint = grid_basic_get_item_width_hint((uint32_t)index);
    old_height = grid_basic_measure_item_height_with_state(item, width_hint, 1);
    item->variant = (uint8_t)((item->variant + 1U) % GRID_BASIC_VARIANT_COUNT);
    item->active = (uint8_t)!item->active;
    item->progress = (uint8_t)(20U + ((item->progress + 21U) % 76U));
    item->revision++;
    new_height = grid_basic_measure_item_height_with_state(item, width_hint, 1);
    grid_basic_ctx.patch_count++;

    if (old_height != new_height)
    {
        egui_view_virtual_grid_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&grid_view), item->stable_id);
    }
    else
    {
        egui_view_virtual_grid_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&grid_view), item->stable_id);
    }

    (void)egui_view_virtual_grid_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&grid_view), item->stable_id, GRID_BASIC_KEEP_INSET);
}

static void grid_basic_cycle_columns(void)
{
    uint8_t columns = grid_basic_get_column_count();

    columns = columns == 2U ? 3U : columns == 3U ? 4U : 2U;
    egui_view_virtual_grid_set_column_count(EGUI_VIEW_OF(&grid_view), columns);
    if (grid_basic_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        (void)egui_view_virtual_grid_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&grid_view), grid_basic_ctx.selected_id, GRID_BASIC_KEEP_INSET);
    }
}

static void grid_basic_reset_demo(void)
{
    grid_basic_abort_motion();
    grid_basic_init_items();
    egui_view_virtual_grid_set_column_count(EGUI_VIEW_OF(&grid_view), 2);
    egui_view_virtual_grid_notify_data_changed(EGUI_VIEW_OF(&grid_view));
    egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&grid_view), grid_basic_ctx.items[0].stable_id, 0);
    egui_view_virtual_grid_scroll_to_item_by_stable_id(EGUI_VIEW_OF(&grid_view), grid_basic_ctx.items[0].stable_id, 0);
    egui_view_virtual_grid_set_scroll_y(EGUI_VIEW_OF(&grid_view), 0);
}

static int grid_basic_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < GRID_BASIC_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void grid_basic_action_button_click_cb(egui_view_t *self)
{
    switch (grid_basic_find_action_button_index(self))
    {
    case GRID_BASIC_ACTION_PATCH:
        grid_basic_patch_selected();
        break;
    case GRID_BASIC_ACTION_COLS:
        grid_basic_cycle_columns();
        break;
    case GRID_BASIC_ACTION_RESET:
        grid_basic_reset_demo();
        break;
    default:
        break;
    }
}

static void grid_basic_style_action_button(egui_view_button_t *button, uint8_t action_index)
{
    egui_background_t *background;

    switch (action_index)
    {
    case GRID_BASIC_ACTION_PATCH:
        background = EGUI_BG_OF(&grid_basic_patch_bg);
        break;
    case GRID_BASIC_ACTION_COLS:
        background = EGUI_BG_OF(&grid_basic_cols_bg);
        break;
    default:
        background = EGUI_BG_OF(&grid_basic_reset_bg);
        break;
    }

    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_size(EGUI_VIEW_OF(button), GRID_BASIC_ACTION_W, 22);
    egui_view_set_background(EGUI_VIEW_OF(button), background);
    egui_view_label_set_font(EGUI_VIEW_OF(button), GRID_BASIC_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x314454), EGUI_ALPHA_100);
    egui_view_label_set_text(EGUI_VIEW_OF(button), grid_basic_action_names[action_index]);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), grid_basic_action_button_click_cb);
}

static void grid_basic_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const egui_font_t *font,
                                  uint8_t align, egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), width, height);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), align);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static uint32_t grid_basic_ds_get_count(void *context)
{
    EGUI_UNUSED(context);
    return GRID_BASIC_ITEM_COUNT;
}

static uint32_t grid_basic_ds_get_stable_id(void *context, uint32_t index)
{
    grid_basic_context_t *ctx = (grid_basic_context_t *)context;

    return index < GRID_BASIC_ITEM_COUNT ? ctx->items[index].stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t grid_basic_ds_find_index_by_stable_id(void *context, uint32_t stable_id)
{
    EGUI_UNUSED(context);
    return grid_basic_find_index_by_stable_id(stable_id);
}

static int32_t grid_basic_ds_measure_item_height(void *context, uint32_t index, int32_t width_hint)
{
    grid_basic_context_t *ctx = (grid_basic_context_t *)context;
    const grid_basic_item_t *item = index < GRID_BASIC_ITEM_COUNT ? &ctx->items[index] : NULL;
    uint8_t selected = item != NULL && item->stable_id == ctx->selected_id;

    return grid_basic_measure_item_height_with_state(item, width_hint, selected);
}

static egui_view_t *grid_basic_ds_create_item_view(void *context, uint16_t view_type)
{
    grid_basic_view_t *view = (grid_basic_view_t *)egui_malloc(sizeof(grid_basic_view_t));

    EGUI_UNUSED(context);
    EGUI_UNUSED(view_type);

    if (view == NULL)
    {
        return NULL;
    }

    memset(view, 0, sizeof(*view));
    egui_view_group_init(EGUI_VIEW_OF(&view->root));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->root), grid_basic_item_click_cb);

    egui_view_card_init(EGUI_VIEW_OF(&view->card));
    egui_view_set_position(EGUI_VIEW_OF(&view->card), GRID_BASIC_CARD_INSET_X, GRID_BASIC_CARD_INSET_Y);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->card), grid_basic_item_click_cb);
    egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->card));

    grid_basic_init_label(&view->title, 8, 10, 80, 14, GRID_BASIC_FONT_TITLE, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x22384A));
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->title));

    grid_basic_init_label(&view->badge, 38, 10, 36, 12, GRID_BASIC_FONT_BODY, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x5A6F82));
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->badge));

    grid_basic_init_label(&view->meta, 8, 28, 80, 12, GRID_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x5A6F82));
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->meta));

    egui_view_progress_bar_init(EGUI_VIEW_OF(&view->progress));
    egui_view_set_position(EGUI_VIEW_OF(&view->progress), 8, 48);
    egui_view_set_size(EGUI_VIEW_OF(&view->progress), 80, 6);
    view->progress.is_show_control = 0;
    view->progress.bk_color = EGUI_COLOR_HEX(0xD6E2EA);
    view->progress.progress_color = EGUI_COLOR_HEX(0x3E739A);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->progress));

    return EGUI_VIEW_OF(&view->root);
}

static void grid_basic_ds_destroy_item_view(void *context, egui_view_t *view, uint16_t view_type)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(view_type);
    egui_free(view);
}

static void grid_basic_ds_bind_item_view(void *context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    grid_basic_context_t *ctx = (grid_basic_context_t *)context;
    grid_basic_view_t *item_view = (grid_basic_view_t *)view;
    const grid_basic_item_t *item = index < GRID_BASIC_ITEM_COUNT ? &ctx->items[index] : NULL;
    egui_dim_t root_w;
    egui_dim_t root_h;
    egui_dim_t card_w;
    egui_dim_t card_h;
    uint8_t selected;
    egui_color_t fill;
    egui_color_t border;
    egui_color_t text_color;

    EGUI_UNUSED(stable_id);

    if (item == NULL)
    {
        return;
    }

    root_w = EGUI_VIEW_OF(&item_view->root)->region.size.width > 0 ? EGUI_VIEW_OF(&item_view->root)->region.size.width : 96;
    root_h = EGUI_VIEW_OF(&item_view->root)->region.size.height > 0 ? EGUI_VIEW_OF(&item_view->root)->region.size.height : 72;
    card_w = root_w - GRID_BASIC_CARD_INSET_X * 2;
    card_h = root_h - GRID_BASIC_CARD_INSET_Y * 2;
    selected = item->stable_id == ctx->selected_id;

    if (selected)
    {
        fill = EGUI_COLOR_HEX(0x2F5E8A);
        border = EGUI_COLOR_HEX(0x224665);
        text_color = EGUI_COLOR_WHITE;
        item_view->progress.progress_color = EGUI_COLOR_WHITE;
    }
    else if (item->variant == GRID_BASIC_VARIANT_TALL)
    {
        fill = EGUI_COLOR_HEX(0xE9F3FF);
        border = EGUI_COLOR_HEX(0xB8D0E8);
        text_color = EGUI_COLOR_HEX(0x22384A);
        item_view->progress.progress_color = EGUI_COLOR_HEX(0x3E739A);
    }
    else if (item->variant == GRID_BASIC_VARIANT_STANDARD)
    {
        fill = EGUI_COLOR_HEX(0xEAF6F0);
        border = EGUI_COLOR_HEX(0xB6D9C7);
        text_color = EGUI_COLOR_HEX(0x22384A);
        item_view->progress.progress_color = EGUI_COLOR_HEX(0x338563);
    }
    else
    {
        fill = EGUI_COLOR_WHITE;
        border = EGUI_COLOR_HEX(0xC7D7E3);
        text_color = EGUI_COLOR_HEX(0x22384A);
        item_view->progress.progress_color = EGUI_COLOR_HEX(0xC28B3F);
    }

    snprintf(item_view->title_text, sizeof(item_view->title_text), "Tile %03lu", (unsigned long)index);
    snprintf(item_view->badge_text, sizeof(item_view->badge_text), "R%u", (unsigned)item->revision);
    snprintf(item_view->meta_text, sizeof(item_view->meta_text), "%s  %s", grid_basic_variant_names[item->variant], item->active ? "Live" : "Idle");

    egui_view_set_size(EGUI_VIEW_OF(&item_view->card), card_w, card_h);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&item_view->card), fill, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&item_view->card), 1, border);

    egui_view_set_size(EGUI_VIEW_OF(&item_view->title), card_w - 16, 14);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->title), item_view->title_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->title), text_color, EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->badge), card_w - 44, 10);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->badge), 36, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->badge), item_view->badge_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->badge), selected ? EGUI_COLOR_HEX(0xDCE8F2) : EGUI_COLOR_HEX(0x5A6F82), EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->meta), 8, 28);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->meta), card_w - 16, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->meta), item_view->meta_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->meta), selected ? EGUI_COLOR_HEX(0xDCE8F2) : EGUI_COLOR_HEX(0x5A6F82), EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->progress), 8, card_h - 16);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->progress), card_w - 16, 6);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&item_view->progress), item->progress);
    item_view->progress.bk_color = selected ? EGUI_COLOR_HEX(0x5B88B0) : EGUI_COLOR_HEX(0xD6E2EA);
}

static uint8_t grid_basic_ds_should_keep_alive(void *context, egui_view_t *view, uint32_t stable_id)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(view);

    return stable_id == grid_basic_ctx.selected_id;
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

static egui_view_t *grid_basic_find_visible_view_by_index(uint32_t index)
{
    uint32_t stable_id;
    const egui_view_virtual_grid_slot_t *slot;

    if (index >= GRID_BASIC_ITEM_COUNT)
    {
        return NULL;
    }

    stable_id = grid_basic_ctx.items[index].stable_id;
    slot = egui_view_virtual_grid_find_slot_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id);
    if (slot == NULL || !egui_view_virtual_viewport_is_slot_center_visible(EGUI_VIEW_OF(&grid_view), slot))
    {
        return NULL;
    }

    return egui_view_virtual_grid_find_view_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id);
}

static uint32_t grid_basic_get_first_visible_index(void)
{
    grid_basic_visible_summary_t summary;

    memset(&summary, 0, sizeof(summary));
    egui_view_virtual_grid_visit_visible_items(EGUI_VIEW_OF(&grid_view), grid_basic_visible_item_visitor, &summary);
    return summary.has_first ? summary.first_index : GRID_BASIC_INVALID_INDEX;
}

static void grid_basic_set_scroll_action(egui_sim_action_t *p_action, uint32_t interval_ms)
{
    egui_region_t *region = &EGUI_VIEW_OF(&grid_view)->region_screen;

    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = (egui_dim_t)(region->location.x + region->size.width / 2);
    p_action->y1 = (egui_dim_t)(region->location.y + region->size.height - 24);
    p_action->x2 = p_action->x1;
    p_action->y2 = (egui_dim_t)(region->location.y + 40);
    p_action->steps = 10;
    p_action->interval_ms = interval_ms;
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
            if (egui_view_virtual_grid_get_slot_count(EGUI_VIEW_OF(&grid_view)) == 0U)
            {
                report_runtime_failure("grid basic should materialize initial rows");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        view = grid_basic_find_visible_view_by_index(0);
        if (view == NULL)
        {
            report_runtime_failure("first grid tile was not visible");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 220);
        return true;
    case 2:
        if (grid_basic_ctx.selected_id != grid_basic_ctx.items[0].stable_id)
        {
            if (recording_click_verify_retry < GRID_BASIC_CLICK_VERIFY_RETRY_MAX)
            {
                view = grid_basic_find_visible_view_by_index(0);
                recording_click_verify_retry++;
                if (view != NULL)
                {
                    EGUI_SIM_SET_CLICK_VIEW(p_action, view, 220);
                    return true;
                }
                recording_request_snapshot();
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            report_runtime_failure("grid click did not update selected item");
        }
        recording_click_verify_retry = 0U;
        recording_patch_verify_retry = 0U;
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[GRID_BASIC_ACTION_PATCH]), 220);
        return true;
    case 3:
        if (grid_basic_ctx.patch_count == 0U)
        {
            if (recording_patch_verify_retry < GRID_BASIC_PATCH_VERIFY_RETRY_MAX)
            {
                recording_patch_verify_retry++;
                recording_request_snapshot();
                EGUI_SIM_SET_WAIT(p_action, 0);
                return true;
            }
            report_runtime_failure("grid patch did not mutate selected item");
        }
        recording_patch_verify_retry = 0U;
        grid_basic_set_scroll_action(p_action, 320);
        return true;
    case 4:
        if (first_call)
        {
            uint32_t first_visible = grid_basic_get_first_visible_index();
            if (first_visible == GRID_BASIC_INVALID_INDEX || first_visible == 0U)
            {
                report_runtime_failure("grid scroll did not move visible window");
            }
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[GRID_BASIC_ACTION_COLS]), 220);
        return true;
    case 5:
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 6:
        if (first_call)
        {
            if (grid_basic_get_column_count() != 3U)
            {
                report_runtime_failure("grid cols did not switch to 3 columns");
            }
            if (grid_basic_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID &&
                egui_view_virtual_grid_find_view_by_stable_id(EGUI_VIEW_OF(&grid_view), grid_basic_ctx.selected_id) == NULL)
            {
                report_runtime_failure("grid cols lost selected item visibility");
            }
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[GRID_BASIC_ACTION_RESET]), 220);
        return true;
    case 7:
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 8:
        if (first_call)
        {
            if (grid_basic_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
            {
                report_runtime_failure("grid reset did not clear selected item");
            }
            if (grid_basic_get_first_visible_index() != 0U)
            {
                report_runtime_failure("grid reset did not restore top position");
            }
            if (grid_basic_get_column_count() != 2U)
            {
                report_runtime_failure("grid reset did not restore 2 columns");
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
    egui_view_virtual_grid_setup_t setup = {
            .params = &grid_basic_view_params,
            .data_source = &grid_basic_data_source,
            .data_source_context = &grid_basic_ctx,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };

    memset(&grid_basic_ctx, 0, sizeof(grid_basic_ctx));
    grid_basic_init_items();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0U;
    recording_click_verify_retry = 0U;
    recording_patch_verify_retry = 0U;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&grid_basic_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &grid_basic_toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD1DEE7));

    for (i = 0; i < GRID_BASIC_ACTION_COUNT; i++)
    {
        grid_basic_style_action_button(&action_buttons[i], i);
        egui_view_set_position(EGUI_VIEW_OF(&action_buttons[i]), 10 + i * (GRID_BASIC_ACTION_W + GRID_BASIC_ACTION_GAP), 6);
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
    }

    egui_view_virtual_grid_init_with_setup(EGUI_VIEW_OF(&grid_view), &setup);
    egui_view_set_background(EGUI_VIEW_OF(&grid_view), EGUI_BG_OF(&grid_basic_view_bg));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}
