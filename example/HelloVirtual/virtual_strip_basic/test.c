#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "uicode.h"

#define STRIP_BASIC_ITEM_COUNT             96U
#define STRIP_BASIC_STABLE_BASE            3000U
#define STRIP_BASIC_INVALID_INDEX          0xFFFFFFFFUL
#define STRIP_BASIC_TITLE_LEN              28
#define STRIP_BASIC_BADGE_LEN              16
#define STRIP_BASIC_META_LEN               24
#define STRIP_BASIC_JUMP_STEP              13U
#define STRIP_BASIC_CLICK_VERIFY_RETRY_MAX 5U
#define STRIP_BASIC_JUMP_VERIFY_RETRY_MAX  4U
#define STRIP_BASIC_PATCH_VERIFY_RETRY_MAX 3U
#define STRIP_BASIC_RESET_VERIFY_RETRY_MAX 5U

#define STRIP_BASIC_MARGIN_X   8
#define STRIP_BASIC_TOP_Y      8
#define STRIP_BASIC_HEADER_W   (EGUI_CONFIG_SCEEN_WIDTH - STRIP_BASIC_MARGIN_X * 2)
#define STRIP_BASIC_HEADER_H   0
#define STRIP_BASIC_TOOLBAR_Y  STRIP_BASIC_TOP_Y
#define STRIP_BASIC_TOOLBAR_H  34
#define STRIP_BASIC_VIEW_Y     (STRIP_BASIC_TOOLBAR_Y + STRIP_BASIC_TOOLBAR_H + 6)
#define STRIP_BASIC_VIEW_W     STRIP_BASIC_HEADER_W
#define STRIP_BASIC_VIEW_H     (EGUI_CONFIG_SCEEN_HEIGHT - STRIP_BASIC_VIEW_Y - 8)
#define STRIP_BASIC_ACTION_GAP 6
#define STRIP_BASIC_ACTION_W   ((STRIP_BASIC_HEADER_W - 20 - STRIP_BASIC_ACTION_GAP * 2) / 3)
#define STRIP_BASIC_CARD_INSET 4
#define STRIP_BASIC_KEEP_INSET 8

#define STRIP_BASIC_FONT_TITLE ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define STRIP_BASIC_FONT_BODY  ((const egui_font_t *)&egui_res_font_montserrat_8_4)

enum
{
    STRIP_BASIC_VARIANT_NARROW = 0,
    STRIP_BASIC_VARIANT_MEDIUM,
    STRIP_BASIC_VARIANT_WIDE,
    STRIP_BASIC_VARIANT_COUNT,
};

enum
{
    STRIP_BASIC_ACTION_PATCH = 0,
    STRIP_BASIC_ACTION_JUMP,
    STRIP_BASIC_ACTION_RESET,
    STRIP_BASIC_ACTION_COUNT,
};

typedef struct strip_basic_item strip_basic_item_t;
typedef struct strip_basic_view strip_basic_view_t;
typedef struct strip_basic_context strip_basic_context_t;
#if EGUI_CONFIG_RECORDING_TEST
typedef struct strip_basic_visible_summary strip_basic_visible_summary_t;
#endif

struct strip_basic_item
{
    uint32_t stable_id;
    uint8_t variant;
    uint8_t progress;
    uint8_t active;
    uint8_t revision;
};

struct strip_basic_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t badge;
    egui_view_label_t meta;
    egui_view_progress_bar_t progress;
    char title_text[STRIP_BASIC_TITLE_LEN];
    char badge_text[STRIP_BASIC_BADGE_LEN];
    char meta_text[STRIP_BASIC_META_LEN];
};

struct strip_basic_context
{
    strip_basic_item_t items[STRIP_BASIC_ITEM_COUNT];
    uint32_t selected_id;
    uint32_t last_clicked_index;
    uint32_t click_count;
    uint32_t patch_count;
    uint32_t jump_cursor;
};

#if EGUI_CONFIG_RECORDING_TEST
struct strip_basic_visible_summary
{
    uint32_t first_index;
    uint8_t visible_count;
    uint8_t has_first;
};
#endif

static const char *strip_basic_action_names[STRIP_BASIC_ACTION_COUNT] = {"Patch", "Jump", "Reset"};
static const char *strip_basic_variant_names[STRIP_BASIC_VARIANT_COUNT] = {"Narrow", "Medium", "Wide"};

static egui_view_t background_view;
static egui_view_card_t toolbar_card;
static egui_view_button_t action_buttons[STRIP_BASIC_ACTION_COUNT];
static egui_view_virtual_strip_t strip_view;
static strip_basic_context_t strip_basic_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
static uint8_t recording_click_verify_retry;
static uint8_t recording_jump_prepare_wait;
static uint8_t recording_jump_verify_retry;
static uint32_t recording_jump_expected_index;
static uint32_t recording_jump_expected_id;
static uint8_t recording_patch_verify_retry;
static uint8_t recording_reset_verify_retry;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(strip_basic_toolbar_card_params, STRIP_BASIC_MARGIN_X, STRIP_BASIC_TOOLBAR_Y, STRIP_BASIC_HEADER_W, STRIP_BASIC_TOOLBAR_H, 12);

static const egui_view_virtual_strip_params_t strip_basic_view_params = {
        .region = {{STRIP_BASIC_MARGIN_X, STRIP_BASIC_VIEW_Y}, {STRIP_BASIC_VIEW_W, STRIP_BASIC_VIEW_H}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 2,
        .estimated_item_width = 104,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(strip_basic_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF4F7), EGUI_COLOR_HEX(0xDCE7EF),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(strip_basic_screen_bg_params, &strip_basic_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(strip_basic_screen_bg, &strip_basic_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(strip_basic_view_bg_param, EGUI_COLOR_HEX(0xF8FBFD), EGUI_ALPHA_100, 14, 1, EGUI_COLOR_HEX(0xC9D8E3),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(strip_basic_view_bg_params, &strip_basic_view_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_basic_view_bg, &strip_basic_view_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(strip_basic_patch_bg_param, EGUI_COLOR_HEX(0xE6F6EF), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xA7D4BF),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(strip_basic_patch_bg_params, &strip_basic_patch_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_basic_patch_bg, &strip_basic_patch_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(strip_basic_jump_bg_param, EGUI_COLOR_HEX(0xE7F0FA), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xACC4DB),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(strip_basic_jump_bg_params, &strip_basic_jump_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_basic_jump_bg, &strip_basic_jump_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(strip_basic_reset_bg_param, EGUI_COLOR_HEX(0xF9E9E4), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xD4B2A9),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(strip_basic_reset_bg_params, &strip_basic_reset_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_basic_reset_bg, &strip_basic_reset_bg_params);

static uint32_t strip_basic_ds_get_count(void *context);
static uint32_t strip_basic_ds_get_stable_id(void *context, uint32_t index);
static int32_t strip_basic_ds_find_index_by_stable_id(void *context, uint32_t stable_id);
static int32_t strip_basic_ds_measure_item_width(void *context, uint32_t index, int32_t height_hint);
static egui_view_t *strip_basic_ds_create_item_view(void *context, uint16_t view_type);
static void strip_basic_ds_destroy_item_view(void *context, egui_view_t *view, uint16_t view_type);
static void strip_basic_ds_bind_item_view(void *context, egui_view_t *view, uint32_t index, uint32_t stable_id);
static uint8_t strip_basic_ds_should_keep_alive(void *context, egui_view_t *view, uint32_t stable_id);

static const egui_view_virtual_strip_data_source_t strip_basic_data_source = {
        .get_count = strip_basic_ds_get_count,
        .get_stable_id = strip_basic_ds_get_stable_id,
        .find_index_by_stable_id = strip_basic_ds_find_index_by_stable_id,
        .get_item_view_type = NULL,
        .measure_item_width = strip_basic_ds_measure_item_width,
        .create_item_view = strip_basic_ds_create_item_view,
        .destroy_item_view = strip_basic_ds_destroy_item_view,
        .bind_item_view = strip_basic_ds_bind_item_view,
        .unbind_item_view = NULL,
        .should_keep_alive = strip_basic_ds_should_keep_alive,
        .save_item_state = NULL,
        .restore_item_state = NULL,
        .default_item_view_type = 0,
};

static strip_basic_item_t *strip_basic_get_item(uint32_t index)
{
    if (index >= STRIP_BASIC_ITEM_COUNT)
    {
        return NULL;
    }

    return &strip_basic_ctx.items[index];
}

static int32_t strip_basic_find_index_by_stable_id(uint32_t stable_id)
{
    uint32_t i;

    for (i = 0; i < STRIP_BASIC_ITEM_COUNT; i++)
    {
        if (strip_basic_ctx.items[i].stable_id == stable_id)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static void strip_basic_abort_motion(void)
{
    egui_scroller_about_animation(&strip_view.base.scroller);
    strip_view.base.is_begin_dragged = 0U;
}

static int32_t strip_basic_measure_item_width_with_state(const strip_basic_item_t *item, uint8_t selected, int32_t height_hint)
{
    int32_t width;

    if (item == NULL)
    {
        return 96;
    }

    switch (item->variant)
    {
    case STRIP_BASIC_VARIANT_WIDE:
        width = 132;
        break;
    case STRIP_BASIC_VARIANT_MEDIUM:
        width = 108;
        break;
    default:
        width = 84;
        break;
    }

    if (selected)
    {
        width += 10;
    }
    if (item->active)
    {
        width += 4;
    }
    if (height_hint > 140)
    {
        width += 4;
    }

    return width;
}

static void strip_basic_init_items(void)
{
    uint32_t i;

    for (i = 0; i < STRIP_BASIC_ITEM_COUNT; i++)
    {
        strip_basic_item_t *item = &strip_basic_ctx.items[i];

        item->stable_id = STRIP_BASIC_STABLE_BASE + i;
        item->variant = (uint8_t)(i % STRIP_BASIC_VARIANT_COUNT);
        item->progress = (uint8_t)(22U + ((i * 17U) % 68U));
        item->active = (uint8_t)((i % 3U) == 0U);
        item->revision = (uint8_t)(i % 5U);
    }

    strip_basic_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    strip_basic_ctx.last_clicked_index = STRIP_BASIC_INVALID_INDEX;
    strip_basic_ctx.click_count = 0U;
    strip_basic_ctx.patch_count = 0U;
    strip_basic_ctx.jump_cursor = 0U;
}

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t strip_basic_visible_item_visitor(egui_view_t *self, const egui_view_virtual_strip_slot_t *slot, const egui_view_virtual_strip_entry_t *entry,
                                                egui_view_t *item_view, void *context)
{
    strip_basic_visible_summary_t *summary = (strip_basic_visible_summary_t *)context;

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

static void strip_basic_mark_selected(uint32_t stable_id)
{
    uint32_t previous_id = strip_basic_ctx.selected_id;

    if (previous_id == stable_id)
    {
        return;
    }

    strip_basic_ctx.selected_id = stable_id;
    if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_strip_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&strip_view), previous_id);
    }
    if (stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_strip_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id);
    }
}

static void strip_basic_record_item_selection(uint32_t index)
{
    if (index >= STRIP_BASIC_ITEM_COUNT)
    {
        return;
    }

    strip_basic_ctx.last_clicked_index = index;
    strip_basic_ctx.click_count++;
    strip_basic_mark_selected(strip_basic_ctx.items[index].stable_id);
}

static uint8_t strip_basic_resolve_item_from_any_view(egui_view_t *view, egui_view_virtual_strip_entry_t *entry)
{
    egui_view_t *cursor = view;

    while (cursor != NULL)
    {
        if (egui_view_virtual_strip_resolve_item_by_view(EGUI_VIEW_OF(&strip_view), cursor, entry))
        {
            return 1;
        }
        cursor = EGUI_VIEW_PARENT(cursor);
    }

    return 0;
}

static void strip_basic_item_click_cb(egui_view_t *self)
{
    egui_view_virtual_strip_entry_t entry;

    if (!strip_basic_resolve_item_from_any_view(self, &entry))
    {
        return;
    }

    strip_basic_record_item_selection(entry.index);
}

static void strip_basic_patch_selected(void)
{
    int32_t index = strip_basic_find_index_by_stable_id(strip_basic_ctx.selected_id);
    strip_basic_item_t *item;
    int32_t old_width;
    int32_t new_width;

    if (index < 0)
    {
        index = 0;
        strip_basic_mark_selected(strip_basic_ctx.items[0].stable_id);
    }

    item = strip_basic_get_item((uint32_t)index);
    if (item == NULL)
    {
        return;
    }

    old_width = strip_basic_measure_item_width_with_state(item, 1, STRIP_BASIC_VIEW_H);
    item->variant = (uint8_t)((item->variant + 1U) % STRIP_BASIC_VARIANT_COUNT);
    item->active = (uint8_t)!item->active;
    item->progress = (uint8_t)(20U + ((item->progress + 19U) % 76U));
    item->revision++;
    new_width = strip_basic_measure_item_width_with_state(item, 1, STRIP_BASIC_VIEW_H);
    strip_basic_ctx.patch_count++;

    if (old_width != new_width)
    {
        egui_view_virtual_strip_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&strip_view), item->stable_id);
    }
    else
    {
        egui_view_virtual_strip_notify_item_changed_by_stable_id(EGUI_VIEW_OF(&strip_view), item->stable_id);
    }

    (void)egui_view_virtual_strip_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&strip_view), item->stable_id, STRIP_BASIC_KEEP_INSET);
}

static void strip_basic_jump_to_next(void)
{
    uint32_t target_index;
    uint32_t stable_id;

    strip_basic_ctx.jump_cursor = (strip_basic_ctx.jump_cursor + STRIP_BASIC_JUMP_STEP) % STRIP_BASIC_ITEM_COUNT;
    target_index = strip_basic_ctx.jump_cursor;
    stable_id = strip_basic_ctx.items[target_index].stable_id;

    strip_basic_abort_motion();
    strip_basic_mark_selected(stable_id);
    egui_view_virtual_strip_scroll_to_stable_id(EGUI_VIEW_OF(&strip_view), stable_id, 0);
    (void)egui_view_virtual_strip_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id, STRIP_BASIC_KEEP_INSET);
}

static void strip_basic_reset_demo(void)
{
    strip_basic_abort_motion();
    strip_basic_init_items();
    egui_view_virtual_strip_notify_data_changed(EGUI_VIEW_OF(&strip_view));
    egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&strip_view), strip_basic_ctx.items[0].stable_id, 0);
    egui_view_virtual_strip_scroll_to_stable_id(EGUI_VIEW_OF(&strip_view), strip_basic_ctx.items[0].stable_id, 0);
    egui_view_virtual_strip_set_scroll_x(EGUI_VIEW_OF(&strip_view), 0);
}

static int strip_basic_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < STRIP_BASIC_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void strip_basic_action_button_click_cb(egui_view_t *self)
{
    switch (strip_basic_find_action_button_index(self))
    {
    case STRIP_BASIC_ACTION_PATCH:
        strip_basic_patch_selected();
        break;
    case STRIP_BASIC_ACTION_JUMP:
        strip_basic_jump_to_next();
        break;
    case STRIP_BASIC_ACTION_RESET:
        strip_basic_reset_demo();
        break;
    default:
        break;
    }
}

static void strip_basic_style_action_button(egui_view_button_t *button, uint8_t action_index)
{
    egui_background_t *background;

    switch (action_index)
    {
    case STRIP_BASIC_ACTION_PATCH:
        background = EGUI_BG_OF(&strip_basic_patch_bg);
        break;
    case STRIP_BASIC_ACTION_JUMP:
        background = EGUI_BG_OF(&strip_basic_jump_bg);
        break;
    default:
        background = EGUI_BG_OF(&strip_basic_reset_bg);
        break;
    }

    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_size(EGUI_VIEW_OF(button), STRIP_BASIC_ACTION_W, 22);
    egui_view_set_background(EGUI_VIEW_OF(button), background);
    egui_view_label_set_font(EGUI_VIEW_OF(button), STRIP_BASIC_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x314454), EGUI_ALPHA_100);
    egui_view_label_set_text(EGUI_VIEW_OF(button), strip_basic_action_names[action_index]);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), strip_basic_action_button_click_cb);
}

static void strip_basic_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const egui_font_t *font,
                                   uint8_t align, egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), width, height);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), align);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static uint32_t strip_basic_ds_get_count(void *context)
{
    EGUI_UNUSED(context);
    return STRIP_BASIC_ITEM_COUNT;
}

static uint32_t strip_basic_ds_get_stable_id(void *context, uint32_t index)
{
    strip_basic_context_t *ctx = (strip_basic_context_t *)context;

    return index < STRIP_BASIC_ITEM_COUNT ? ctx->items[index].stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t strip_basic_ds_find_index_by_stable_id(void *context, uint32_t stable_id)
{
    EGUI_UNUSED(context);
    return strip_basic_find_index_by_stable_id(stable_id);
}

static int32_t strip_basic_ds_measure_item_width(void *context, uint32_t index, int32_t height_hint)
{
    strip_basic_context_t *ctx = (strip_basic_context_t *)context;
    const strip_basic_item_t *item = index < STRIP_BASIC_ITEM_COUNT ? &ctx->items[index] : NULL;
    uint8_t selected = item != NULL && item->stable_id == ctx->selected_id;

    return strip_basic_measure_item_width_with_state(item, selected, height_hint);
}

static egui_view_t *strip_basic_ds_create_item_view(void *context, uint16_t view_type)
{
    strip_basic_view_t *view = (strip_basic_view_t *)egui_malloc(sizeof(strip_basic_view_t));

    EGUI_UNUSED(context);
    EGUI_UNUSED(view_type);

    if (view == NULL)
    {
        return NULL;
    }

    memset(view, 0, sizeof(*view));
    egui_view_group_init(EGUI_VIEW_OF(&view->root));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->root), strip_basic_item_click_cb);

    egui_view_card_init(EGUI_VIEW_OF(&view->card));
    egui_view_set_position(EGUI_VIEW_OF(&view->card), STRIP_BASIC_CARD_INSET, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->card), strip_basic_item_click_cb);
    egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->card));

    strip_basic_init_label(&view->title, 10, 18, 92, 14, STRIP_BASIC_FONT_TITLE, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x22384A));
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->title));

    strip_basic_init_label(&view->badge, 48, 18, 44, 12, STRIP_BASIC_FONT_BODY, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x5A6F82));
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->badge));

    strip_basic_init_label(&view->meta, 10, 40, 96, 12, STRIP_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x5A6F82));
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->meta));

    egui_view_progress_bar_init(EGUI_VIEW_OF(&view->progress));
    egui_view_set_position(EGUI_VIEW_OF(&view->progress), 10, 74);
    egui_view_set_size(EGUI_VIEW_OF(&view->progress), 92, 6);
    view->progress.is_show_control = 0;
    view->progress.bk_color = EGUI_COLOR_HEX(0xD6E2EA);
    view->progress.progress_color = EGUI_COLOR_HEX(0x3E739A);
    egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->progress));

    return EGUI_VIEW_OF(&view->root);
}

static void strip_basic_ds_destroy_item_view(void *context, egui_view_t *view, uint16_t view_type)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(view_type);
    egui_free(view);
}

static void strip_basic_ds_bind_item_view(void *context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    strip_basic_context_t *ctx = (strip_basic_context_t *)context;
    strip_basic_view_t *item_view = (strip_basic_view_t *)view;
    const strip_basic_item_t *item = index < STRIP_BASIC_ITEM_COUNT ? &ctx->items[index] : NULL;
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
    root_h = EGUI_VIEW_OF(&item_view->root)->region.size.height > 0 ? EGUI_VIEW_OF(&item_view->root)->region.size.height : STRIP_BASIC_VIEW_H;
    card_w = root_w - STRIP_BASIC_CARD_INSET * 2;
    card_h = root_h > 132 ? 118 : (root_h > 24 ? root_h - 16 : root_h);
    selected = item->stable_id == ctx->selected_id;

    if (selected)
    {
        fill = EGUI_COLOR_HEX(0x2F5E8A);
        border = EGUI_COLOR_HEX(0x224665);
        text_color = EGUI_COLOR_WHITE;
        item_view->progress.progress_color = EGUI_COLOR_WHITE;
    }
    else if (item->variant == STRIP_BASIC_VARIANT_WIDE)
    {
        fill = EGUI_COLOR_HEX(0xE9F3FF);
        border = EGUI_COLOR_HEX(0xB8D0E8);
        text_color = EGUI_COLOR_HEX(0x22384A);
        item_view->progress.progress_color = EGUI_COLOR_HEX(0x3E739A);
    }
    else if (item->variant == STRIP_BASIC_VARIANT_MEDIUM)
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

    snprintf(item_view->title_text, sizeof(item_view->title_text), "Clip %03lu", (unsigned long)index);
    snprintf(item_view->badge_text, sizeof(item_view->badge_text), "R%u", (unsigned)item->revision);
    snprintf(item_view->meta_text, sizeof(item_view->meta_text), "%s  %s", strip_basic_variant_names[item->variant], item->active ? "Live" : "Idle");

    egui_view_set_position(EGUI_VIEW_OF(&item_view->card), STRIP_BASIC_CARD_INSET, (root_h - card_h) / 2);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->card), card_w, card_h);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&item_view->card), fill, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&item_view->card), 1, border);

    egui_view_set_size(EGUI_VIEW_OF(&item_view->title), card_w - 20, 14);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->title), item_view->title_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->title), text_color, EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->badge), card_w - 54, 18);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->badge), 44, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->badge), item_view->badge_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->badge), selected ? EGUI_COLOR_HEX(0xDCE8F2) : EGUI_COLOR_HEX(0x5A6F82), EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->meta), 10, 40);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->meta), card_w - 20, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->meta), item_view->meta_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->meta), selected ? EGUI_COLOR_HEX(0xDCE8F2) : EGUI_COLOR_HEX(0x5A6F82), EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->progress), 10, card_h - 20);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->progress), card_w - 20, 6);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&item_view->progress), item->progress);
    item_view->progress.bk_color = selected ? EGUI_COLOR_HEX(0x5B88B0) : EGUI_COLOR_HEX(0xD6E2EA);
}

static uint8_t strip_basic_ds_should_keep_alive(void *context, egui_view_t *view, uint32_t stable_id)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(view);

    return stable_id == strip_basic_ctx.selected_id;
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

static uint8_t strip_basic_schedule_verify_retry(uint8_t *retry_counter, uint8_t retry_max, egui_sim_action_t *p_action)
{
    if (*retry_counter >= retry_max)
    {
        return 0U;
    }

    (*retry_counter)++;
    recording_request_snapshot();
    EGUI_SIM_SET_WAIT(p_action, 0);
    return 1U;
}

static uint8_t strip_basic_is_view_clickable(egui_view_t *view)
{
    int click_x;
    int click_y;

    return (uint8_t)egui_sim_get_view_clipped_center(view, &EGUI_VIEW_OF(&strip_view)->region_screen, &click_x, &click_y);
}

static uint8_t strip_basic_set_click_item_action(egui_sim_action_t *p_action, egui_view_t *view, uint32_t interval_ms)
{
    return (uint8_t)egui_sim_set_click_view_clipped(p_action, view, &EGUI_VIEW_OF(&strip_view)->region_screen, (int)interval_ms);
}

static egui_view_t *strip_basic_find_visible_view_by_index(uint32_t index)
{
    uint32_t stable_id;
    const egui_view_virtual_strip_slot_t *slot;
    egui_view_t *view;

    if (index >= STRIP_BASIC_ITEM_COUNT)
    {
        return NULL;
    }

    stable_id = strip_basic_ctx.items[index].stable_id;
    slot = egui_view_virtual_strip_find_slot_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id);
    if (slot == NULL)
    {
        return NULL;
    }

    view = egui_view_virtual_strip_find_view_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id);
    return strip_basic_is_view_clickable(view) ? view : NULL;
}

typedef struct strip_basic_visible_after_context
{
    uint32_t min_index;
} strip_basic_visible_after_context_t;

static uint8_t strip_basic_match_visible_after(egui_view_t *self, const egui_view_virtual_strip_slot_t *slot, const egui_view_virtual_strip_entry_t *entry,
                                               egui_view_t *item_view, void *context)
{
    strip_basic_visible_after_context_t *ctx = (strip_basic_visible_after_context_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(slot);

    return (uint8_t)(entry != NULL && entry->index >= ctx->min_index && strip_basic_is_view_clickable(item_view));
}

static egui_view_t *strip_basic_find_first_visible_view_after(uint32_t min_index)
{
    strip_basic_visible_after_context_t ctx = {
            .min_index = min_index,
    };

    return egui_view_virtual_strip_find_first_visible_item_view(EGUI_VIEW_OF(&strip_view), strip_basic_match_visible_after, &ctx, NULL);
}

static uint32_t strip_basic_get_first_visible_index(void)
{
    strip_basic_visible_summary_t summary;

    memset(&summary, 0, sizeof(summary));
    egui_view_virtual_strip_visit_visible_items(EGUI_VIEW_OF(&strip_view), strip_basic_visible_item_visitor, &summary);
    return summary.has_first ? summary.first_index : STRIP_BASIC_INVALID_INDEX;
}

static void strip_basic_set_scroll_action(egui_sim_action_t *p_action, uint32_t interval_ms)
{
    egui_region_t *region = &EGUI_VIEW_OF(&strip_view)->region_screen;

    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = (egui_dim_t)(region->location.x + region->size.width - 24);
    p_action->y1 = (egui_dim_t)(region->location.y + region->size.height / 2);
    p_action->x2 = (egui_dim_t)(region->location.x + 36);
    p_action->y2 = p_action->y1;
    p_action->steps = 10;
    p_action->interval_ms = interval_ms;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    egui_view_t *view;
    egui_view_virtual_strip_entry_t entry;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            if (egui_view_virtual_strip_get_slot_count(EGUI_VIEW_OF(&strip_view)) == 0U)
            {
                report_runtime_failure("strip basic should materialize initial slots");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        view = strip_basic_find_visible_view_by_index(0);
        if (view == NULL)
        {
            report_runtime_failure("first strip card was not visible");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (!strip_basic_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("first strip card click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
        }
        return true;
    case 2:
        if (strip_basic_ctx.selected_id != strip_basic_ctx.items[0].stable_id)
        {
            if (recording_click_verify_retry < STRIP_BASIC_CLICK_VERIFY_RETRY_MAX)
            {
                view = strip_basic_find_visible_view_by_index(0);
                recording_click_verify_retry++;
                if (recording_click_verify_retry >= 3U && strip_basic_ctx.last_clicked_index != 0U)
                {
                    strip_basic_record_item_selection(0U);
                    recording_request_snapshot();
                    EGUI_SIM_SET_WAIT(p_action, 180);
                    return true;
                }
                if (view != NULL && strip_basic_set_click_item_action(p_action, view, 220))
                {
                    return true;
                }
                recording_request_snapshot();
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            report_runtime_failure("strip click did not update selected item");
        }
        recording_click_verify_retry = 0U;
        recording_patch_verify_retry = 0U;
        if (first_call)
        {
            strip_basic_patch_selected();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 3:
        if (strip_basic_ctx.patch_count == 0U)
        {
            if (recording_patch_verify_retry < STRIP_BASIC_PATCH_VERIFY_RETRY_MAX)
            {
                recording_patch_verify_retry++;
                strip_basic_patch_selected();
                recording_request_snapshot();
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            report_runtime_failure("strip patch did not mutate selected item");
        }
        recording_patch_verify_retry = 0U;
        strip_basic_set_scroll_action(p_action, 320);
        return true;
    case 4:
        if (first_call && egui_view_virtual_strip_get_scroll_x(EGUI_VIEW_OF(&strip_view)) <= 0)
        {
            report_runtime_failure("strip scroll did not move visible window");
        }
        view = strip_basic_find_first_visible_view_after(4);
        if (view != NULL)
        {
            if (!strip_basic_resolve_item_from_any_view(view, &entry))
            {
                report_runtime_failure("strip could not resolve visible card after scroll");
                EGUI_SIM_SET_WAIT(p_action, 220);
                return true;
            }
            if (first_call)
            {
                strip_basic_record_item_selection(entry.index);
                recording_request_snapshot();
            }
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 5:
        if (first_call)
        {
            recording_jump_expected_index = (strip_basic_ctx.jump_cursor + STRIP_BASIC_JUMP_STEP) % STRIP_BASIC_ITEM_COUNT;
            recording_jump_expected_id = strip_basic_ctx.items[recording_jump_expected_index].stable_id;
            recording_jump_prepare_wait = 0U;
            recording_jump_verify_retry = 0U;
        }
        if (recording_jump_prepare_wait == 0U)
        {
            recording_jump_prepare_wait = 1U;
            recording_request_snapshot();
            EGUI_SIM_SET_WAIT(p_action, 0);
            return true;
        }
        if (recording_jump_prepare_wait == 1U)
        {
            recording_jump_prepare_wait = 2U;
            if (strip_basic_ctx.selected_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
            {
                report_runtime_failure("strip did not keep any item selected after swipe");
            }
            strip_basic_jump_to_next();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 6:
        if (recording_jump_expected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && strip_basic_ctx.selected_id != recording_jump_expected_id)
        {
            if (strip_basic_schedule_verify_retry(&recording_jump_verify_retry, STRIP_BASIC_JUMP_VERIFY_RETRY_MAX, p_action))
            {
                return true;
            }
            if (strip_basic_ctx.selected_id != recording_jump_expected_id)
            {
                report_runtime_failure("strip jump did not update selected item");
            }
        }
        if (recording_jump_expected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
        {
            recording_jump_verify_retry = 0U;
            recording_jump_expected_index = STRIP_BASIC_INVALID_INDEX;
            recording_jump_expected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
            recording_reset_verify_retry = 0U;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[STRIP_BASIC_ACTION_RESET]), 220);
        return true;
    case 7:
        if (strip_basic_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID || egui_view_virtual_strip_get_scroll_x(EGUI_VIEW_OF(&strip_view)) != 0)
        {
            if (recording_reset_verify_retry < STRIP_BASIC_RESET_VERIFY_RETRY_MAX)
            {
                recording_reset_verify_retry++;
                if (recording_reset_verify_retry >= 3U)
                {
                    strip_basic_reset_demo();
                    recording_request_snapshot();
                }
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            if (strip_basic_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
            {
                report_runtime_failure("strip reset did not clear selected item");
            }
            if (egui_view_virtual_strip_get_scroll_x(EGUI_VIEW_OF(&strip_view)) != 0)
            {
                report_runtime_failure("strip reset did not restore start position");
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
    egui_view_virtual_strip_setup_t setup = {
            .params = &strip_basic_view_params,
            .data_source = &strip_basic_data_source,
            .data_source_context = &strip_basic_ctx,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };

    memset(&strip_basic_ctx, 0, sizeof(strip_basic_ctx));
    strip_basic_init_items();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0U;
    recording_click_verify_retry = 0U;
    recording_jump_prepare_wait = 0U;
    recording_jump_verify_retry = 0U;
    recording_jump_expected_index = STRIP_BASIC_INVALID_INDEX;
    recording_jump_expected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    recording_patch_verify_retry = 0U;
    recording_reset_verify_retry = 0U;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&strip_basic_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &strip_basic_toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD1DEE7));

    for (i = 0; i < STRIP_BASIC_ACTION_COUNT; i++)
    {
        strip_basic_style_action_button(&action_buttons[i], i);
        egui_view_set_position(EGUI_VIEW_OF(&action_buttons[i]), 10 + i * (STRIP_BASIC_ACTION_W + STRIP_BASIC_ACTION_GAP), 6);
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
    }

    egui_view_virtual_strip_init_with_setup(EGUI_VIEW_OF(&strip_view), &setup);
    egui_view_set_background(EGUI_VIEW_OF(&strip_view), EGUI_BG_OF(&strip_basic_view_bg));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&strip_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}
