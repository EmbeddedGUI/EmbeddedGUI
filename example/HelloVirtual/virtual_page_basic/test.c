#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "uicode.h"

#define PAGE_BASIC_SECTION_COUNT          96U
#define PAGE_BASIC_STABLE_BASE            2000U
#define PAGE_BASIC_INVALID_INDEX          0xFFFFFFFFUL
#define PAGE_BASIC_TITLE_TEXT_LEN         40
#define PAGE_BASIC_BODY_TEXT_LEN          64
#define PAGE_BASIC_BADGE_TEXT_LEN         16
#define PAGE_BASIC_VALUE_TEXT_LEN         16
#define PAGE_BASIC_ROW_TEXT_LEN           32
#define PAGE_BASIC_JUMP_STEP              5U
#define PAGE_BASIC_CLICK_VERIFY_RETRY_MAX 3U
#define PAGE_BASIC_JUMP_VERIFY_RETRY_MAX  3U

#define PAGE_BASIC_MARGIN_X   8
#define PAGE_BASIC_TOP_Y      8
#define PAGE_BASIC_HEADER_W   (EGUI_CONFIG_SCEEN_WIDTH - PAGE_BASIC_MARGIN_X * 2)
#define PAGE_BASIC_HEADER_H   0
#define PAGE_BASIC_TOOLBAR_Y  PAGE_BASIC_TOP_Y
#define PAGE_BASIC_TOOLBAR_H  34
#define PAGE_BASIC_VIEW_Y     (PAGE_BASIC_TOOLBAR_Y + PAGE_BASIC_TOOLBAR_H + 6)
#define PAGE_BASIC_VIEW_W     PAGE_BASIC_HEADER_W
#define PAGE_BASIC_VIEW_H     (EGUI_CONFIG_SCEEN_HEIGHT - PAGE_BASIC_VIEW_Y - 8)
#define PAGE_BASIC_ACTION_GAP 6
#define PAGE_BASIC_ACTION_W   ((PAGE_BASIC_HEADER_W - 20 - PAGE_BASIC_ACTION_GAP * 2) / 3)

#define PAGE_BASIC_HERO_H          84
#define PAGE_BASIC_METRIC_H        96
#define PAGE_BASIC_CHECKLIST_H     86
#define PAGE_BASIC_CHECKLIST_EXP_H 114

#define PAGE_BASIC_FONT_TITLE ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define PAGE_BASIC_FONT_BODY  ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define PAGE_BASIC_FONT_HERO  ((const egui_font_t *)&egui_res_font_montserrat_12_4)

enum
{
    PAGE_BASIC_SECTION_HERO = 1,
    PAGE_BASIC_SECTION_METRIC = 2,
    PAGE_BASIC_SECTION_CHECKLIST = 3,
};

enum
{
    PAGE_BASIC_ACTION_PATCH = 0,
    PAGE_BASIC_ACTION_JUMP,
    PAGE_BASIC_ACTION_RESET,
    PAGE_BASIC_ACTION_COUNT,
};

typedef struct page_basic_section page_basic_section_t;
typedef struct page_basic_hero_view page_basic_hero_view_t;
typedef struct page_basic_metric_view page_basic_metric_view_t;
typedef struct page_basic_checklist_view page_basic_checklist_view_t;
typedef struct page_basic_context page_basic_context_t;
#if EGUI_CONFIG_RECORDING_TEST
typedef struct page_basic_visible_summary page_basic_visible_summary_t;
#endif

struct page_basic_section
{
    uint32_t stable_id;
    uint8_t kind;
    uint8_t progress;
    uint8_t expanded;
    uint8_t revision;
};

struct page_basic_hero_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t eyebrow;
    egui_view_label_t title;
    egui_view_label_t body;
    egui_view_label_t badge;
    egui_view_progress_bar_t progress;
    char eyebrow_text[PAGE_BASIC_BADGE_TEXT_LEN];
    char title_text[PAGE_BASIC_TITLE_TEXT_LEN];
    char body_text[PAGE_BASIC_BODY_TEXT_LEN];
    char badge_text[PAGE_BASIC_BADGE_TEXT_LEN];
};

struct page_basic_metric_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_card_t left_panel;
    egui_view_card_t right_panel;
    egui_view_label_t left_title;
    egui_view_label_t left_value;
    egui_view_label_t right_title;
    egui_view_label_t right_value;
    egui_view_label_t footer;
    char title_text[PAGE_BASIC_TITLE_TEXT_LEN];
    char left_title_text[PAGE_BASIC_BADGE_TEXT_LEN];
    char left_value_text[PAGE_BASIC_VALUE_TEXT_LEN];
    char right_title_text[PAGE_BASIC_BADGE_TEXT_LEN];
    char right_value_text[PAGE_BASIC_VALUE_TEXT_LEN];
    char footer_text[PAGE_BASIC_BODY_TEXT_LEN];
};

struct page_basic_checklist_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_label_t title;
    egui_view_label_t state;
    egui_view_label_t row1;
    egui_view_label_t row2;
    egui_view_label_t row3;
    egui_view_label_t row4;
    egui_view_label_t footer;
    char title_text[PAGE_BASIC_TITLE_TEXT_LEN];
    char state_text[PAGE_BASIC_BADGE_TEXT_LEN];
    char row1_text[PAGE_BASIC_ROW_TEXT_LEN];
    char row2_text[PAGE_BASIC_ROW_TEXT_LEN];
    char row3_text[PAGE_BASIC_ROW_TEXT_LEN];
    char row4_text[PAGE_BASIC_ROW_TEXT_LEN];
    char footer_text[PAGE_BASIC_BODY_TEXT_LEN];
};

struct page_basic_context
{
    page_basic_section_t sections[PAGE_BASIC_SECTION_COUNT];
    uint32_t selected_id;
    uint32_t last_clicked_index;
    uint32_t click_count;
    uint32_t patch_count;
    uint32_t jump_cursor;
};

#if EGUI_CONFIG_RECORDING_TEST
struct page_basic_visible_summary
{
    uint32_t first_index;
    uint8_t visible_count;
    uint8_t has_first;
};
#endif

static const char *page_basic_action_names[PAGE_BASIC_ACTION_COUNT] = {"Patch", "Jump", "Reset"};
static egui_view_t background_view;
static egui_view_card_t toolbar_card;
static egui_view_button_t action_buttons[PAGE_BASIC_ACTION_COUNT];
static egui_view_virtual_page_t page_view;
static page_basic_context_t page_basic_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
static uint8_t recording_click_verify_retry;
static uint8_t recording_jump_verify_retry;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(page_basic_toolbar_card_params, PAGE_BASIC_MARGIN_X, PAGE_BASIC_TOOLBAR_Y, PAGE_BASIC_HEADER_W, PAGE_BASIC_TOOLBAR_H, 12);

static const egui_view_virtual_page_params_t page_basic_view_params = {
        .region = {{PAGE_BASIC_MARGIN_X, PAGE_BASIC_VIEW_Y}, {PAGE_BASIC_VIEW_W, PAGE_BASIC_VIEW_H}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 2,
        .estimated_section_height = 90,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(page_basic_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEFF4F7), EGUI_COLOR_HEX(0xDDE9F0),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(page_basic_screen_bg_params, &page_basic_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(page_basic_screen_bg, &page_basic_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(page_basic_view_bg_param, EGUI_COLOR_HEX(0xF8FBFD), EGUI_ALPHA_100, 14, 1, EGUI_COLOR_HEX(0xC9D9E4),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(page_basic_view_bg_params, &page_basic_view_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(page_basic_view_bg, &page_basic_view_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(page_basic_action_patch_param, EGUI_COLOR_HEX(0xE6F6EF), EGUI_ALPHA_100, 10, 1,
                                                        EGUI_COLOR_HEX(0xA7D4BF), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(page_basic_action_patch_params, &page_basic_action_patch_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(page_basic_action_patch_bg, &page_basic_action_patch_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(page_basic_action_jump_param, EGUI_COLOR_HEX(0xE7F0FA), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xACC4DB),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(page_basic_action_jump_params, &page_basic_action_jump_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(page_basic_action_jump_bg, &page_basic_action_jump_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(page_basic_action_reset_param, EGUI_COLOR_HEX(0xF9E9E4), EGUI_ALPHA_100, 10, 1,
                                                        EGUI_COLOR_HEX(0xD4B2A9), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(page_basic_action_reset_params, &page_basic_action_reset_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(page_basic_action_reset_bg, &page_basic_action_reset_params);

static uint32_t page_basic_ds_get_count(void *context);
static uint32_t page_basic_ds_get_stable_id(void *context, uint32_t index);
static int32_t page_basic_ds_find_index_by_stable_id(void *context, uint32_t stable_id);
static uint16_t page_basic_ds_get_section_type(void *context, uint32_t index);
static int32_t page_basic_ds_measure_section_height(void *context, uint32_t index, int32_t width_hint);
static egui_view_t *page_basic_ds_create_section_view(void *context, uint16_t section_type);
static void page_basic_ds_destroy_section_view(void *context, egui_view_t *view, uint16_t section_type);
static void page_basic_ds_bind_section_view(void *context, egui_view_t *view, uint32_t index, uint32_t stable_id);
static uint8_t page_basic_ds_should_keep_alive(void *context, egui_view_t *view, uint32_t stable_id);

static const egui_view_virtual_page_data_source_t page_basic_data_source = {
        .get_count = page_basic_ds_get_count,
        .get_stable_id = page_basic_ds_get_stable_id,
        .find_index_by_stable_id = page_basic_ds_find_index_by_stable_id,
        .get_section_type = page_basic_ds_get_section_type,
        .measure_section_height = page_basic_ds_measure_section_height,
        .create_section_view = page_basic_ds_create_section_view,
        .destroy_section_view = page_basic_ds_destroy_section_view,
        .bind_section_view = page_basic_ds_bind_section_view,
        .unbind_section_view = NULL,
        .should_keep_alive = page_basic_ds_should_keep_alive,
        .save_section_state = NULL,
        .restore_section_state = NULL,
        .default_section_type = PAGE_BASIC_SECTION_HERO,
};

static page_basic_section_t *page_basic_get_section(uint32_t index)
{
    if (index >= PAGE_BASIC_SECTION_COUNT)
    {
        return NULL;
    }

    return &page_basic_ctx.sections[index];
}

static int32_t page_basic_find_index_by_stable_id(uint32_t stable_id)
{
    uint32_t i;

    for (i = 0; i < PAGE_BASIC_SECTION_COUNT; i++)
    {
        if (page_basic_ctx.sections[i].stable_id == stable_id)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static void page_basic_abort_motion(void)
{
    egui_scroller_about_animation(&page_view.base.scroller);
    page_view.base.is_begin_dragged = 0U;
}

static uint8_t page_basic_is_checklist_expanded(const page_basic_section_t *section)
{
    return section != NULL && section->kind == PAGE_BASIC_SECTION_CHECKLIST && section->expanded != 0U;
}

static int32_t page_basic_measure_section_height_by_kind(const page_basic_section_t *section)
{
    if (section == NULL)
    {
        return PAGE_BASIC_HERO_H;
    }

    switch (section->kind)
    {
    case PAGE_BASIC_SECTION_METRIC:
        return PAGE_BASIC_METRIC_H;
    case PAGE_BASIC_SECTION_CHECKLIST:
        return page_basic_is_checklist_expanded(section) ? PAGE_BASIC_CHECKLIST_EXP_H : PAGE_BASIC_CHECKLIST_H;
    default:
        return PAGE_BASIC_HERO_H;
    }
}

static void page_basic_init_sections(void)
{
    uint32_t i;

    for (i = 0; i < PAGE_BASIC_SECTION_COUNT; i++)
    {
        page_basic_section_t *section = &page_basic_ctx.sections[i];

        section->stable_id = PAGE_BASIC_STABLE_BASE + i;
        section->kind = (uint8_t)((i % 3U) + 1U);
        section->progress = (uint8_t)((i * 19U + 17U) % 100U);
        section->expanded = (uint8_t)(section->kind == PAGE_BASIC_SECTION_CHECKLIST && ((i % 6U) == 2U));
        section->revision = (uint8_t)(i % 5U);
    }

    page_basic_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    page_basic_ctx.last_clicked_index = PAGE_BASIC_INVALID_INDEX;
    page_basic_ctx.click_count = 0U;
    page_basic_ctx.patch_count = 0U;
    page_basic_ctx.jump_cursor = 0U;
}

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t page_basic_visible_section_visitor(egui_view_t *self, const egui_view_virtual_page_slot_t *slot, const egui_view_virtual_page_entry_t *entry,
                                                  egui_view_t *section_view, void *context)
{
    page_basic_visible_summary_t *summary = (page_basic_visible_summary_t *)context;

    EGUI_UNUSED(section_view);

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

static void page_basic_mark_selected(uint32_t stable_id)
{
    uint32_t previous_id = page_basic_ctx.selected_id;

    if (previous_id == stable_id)
    {
        return;
    }

    page_basic_ctx.selected_id = stable_id;
    if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_page_notify_section_changed_by_stable_id(EGUI_VIEW_OF(&page_view), previous_id);
    }
    if (stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_page_notify_section_changed_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id);
    }
}

static uint8_t page_basic_resolve_section_from_any_view(egui_view_t *view, egui_view_virtual_page_entry_t *entry)
{
    egui_view_t *cursor = view;

    while (cursor != NULL)
    {
        if (egui_view_virtual_page_resolve_section_by_view(EGUI_VIEW_OF(&page_view), cursor, entry))
        {
            return 1;
        }
        cursor = EGUI_VIEW_PARENT(cursor);
    }

    return 0;
}

static void page_basic_section_click_cb(egui_view_t *self)
{
    egui_view_virtual_page_entry_t entry;

    if (!page_basic_resolve_section_from_any_view(self, &entry))
    {
        return;
    }

    page_basic_ctx.last_clicked_index = entry.index;
    page_basic_ctx.click_count++;
    page_basic_mark_selected(entry.stable_id);
}

static void page_basic_patch_selected(void)
{
    int32_t index = page_basic_find_index_by_stable_id(page_basic_ctx.selected_id);
    page_basic_section_t *section;

    if (index < 0)
    {
        index = 0;
        page_basic_mark_selected(page_basic_ctx.sections[0].stable_id);
    }

    section = page_basic_get_section((uint32_t)index);
    if (section == NULL)
    {
        return;
    }

    section->progress = (uint8_t)((section->progress + 17U) % 100U);
    section->revision++;
    page_basic_ctx.patch_count++;

    if (section->kind == PAGE_BASIC_SECTION_CHECKLIST)
    {
        section->expanded = (uint8_t)!section->expanded;
        egui_view_virtual_page_notify_section_resized_by_stable_id(EGUI_VIEW_OF(&page_view), section->stable_id);
    }
    else
    {
        egui_view_virtual_page_notify_section_changed_by_stable_id(EGUI_VIEW_OF(&page_view), section->stable_id);
    }

    (void)egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&page_view), section->stable_id, 6);
}

static void page_basic_jump_to_next(void)
{
    uint32_t target_index;
    uint32_t stable_id;

    page_basic_ctx.jump_cursor = (page_basic_ctx.jump_cursor + PAGE_BASIC_JUMP_STEP) % PAGE_BASIC_SECTION_COUNT;
    target_index = page_basic_ctx.jump_cursor;
    stable_id = page_basic_ctx.sections[target_index].stable_id;

    page_basic_abort_motion();
    page_basic_mark_selected(stable_id);
    egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&page_view), stable_id, 0);
    egui_view_virtual_page_scroll_to_section_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id, 0);
    (void)egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id, 6);
}

static void page_basic_reset_demo(void)
{
    uint32_t first_stable_id;

    page_basic_abort_motion();
    page_basic_init_sections();
    first_stable_id = page_basic_ctx.sections[0].stable_id;
    egui_view_virtual_page_notify_data_changed(EGUI_VIEW_OF(&page_view));
    egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&page_view), first_stable_id, 0);
    egui_view_virtual_page_scroll_to_section_by_stable_id(EGUI_VIEW_OF(&page_view), first_stable_id, 0);
    egui_view_virtual_page_set_scroll_y(EGUI_VIEW_OF(&page_view), 0);
    (void)egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&page_view), first_stable_id, 0);
}

static int page_basic_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < PAGE_BASIC_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void page_basic_action_button_click_cb(egui_view_t *self)
{
    switch (page_basic_find_action_button_index(self))
    {
    case PAGE_BASIC_ACTION_PATCH:
        page_basic_patch_selected();
        break;
    case PAGE_BASIC_ACTION_JUMP:
        page_basic_jump_to_next();
        break;
    case PAGE_BASIC_ACTION_RESET:
        page_basic_reset_demo();
        break;
    default:
        break;
    }
}

static void page_basic_style_action_button(egui_view_button_t *button, uint8_t action_index)
{
    egui_background_t *background;

    switch (action_index)
    {
    case PAGE_BASIC_ACTION_PATCH:
        background = EGUI_BG_OF(&page_basic_action_patch_bg);
        break;
    case PAGE_BASIC_ACTION_JUMP:
        background = EGUI_BG_OF(&page_basic_action_jump_bg);
        break;
    default:
        background = EGUI_BG_OF(&page_basic_action_reset_bg);
        break;
    }

    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_size(EGUI_VIEW_OF(button), PAGE_BASIC_ACTION_W, 22);
    egui_view_set_background(EGUI_VIEW_OF(button), background);
    egui_view_label_set_font(EGUI_VIEW_OF(button), PAGE_BASIC_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x314454), EGUI_ALPHA_100);
    egui_view_label_set_text(EGUI_VIEW_OF(button), page_basic_action_names[action_index]);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), page_basic_action_button_click_cb);
}

static void page_basic_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, const egui_font_t *font, uint8_t align,
                                  egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), w, h);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), align);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static uint32_t page_basic_ds_get_count(void *context)
{
    EGUI_UNUSED(context);
    return PAGE_BASIC_SECTION_COUNT;
}

static uint32_t page_basic_ds_get_stable_id(void *context, uint32_t index)
{
    page_basic_context_t *ctx = (page_basic_context_t *)context;

    return index < PAGE_BASIC_SECTION_COUNT ? ctx->sections[index].stable_id : EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static int32_t page_basic_ds_find_index_by_stable_id(void *context, uint32_t stable_id)
{
    EGUI_UNUSED(context);
    return page_basic_find_index_by_stable_id(stable_id);
}

static uint16_t page_basic_ds_get_section_type(void *context, uint32_t index)
{
    page_basic_context_t *ctx = (page_basic_context_t *)context;

    if (index >= PAGE_BASIC_SECTION_COUNT)
    {
        return PAGE_BASIC_SECTION_HERO;
    }

    return ctx->sections[index].kind;
}

static int32_t page_basic_ds_measure_section_height(void *context, uint32_t index, int32_t width_hint)
{
    page_basic_context_t *ctx = (page_basic_context_t *)context;

    EGUI_UNUSED(width_hint);

    if (index >= PAGE_BASIC_SECTION_COUNT)
    {
        return PAGE_BASIC_HERO_H;
    }

    return page_basic_measure_section_height_by_kind(&ctx->sections[index]);
}

static egui_color_t page_basic_kind_fill_color(const page_basic_section_t *section, uint8_t selected)
{
    if (selected)
    {
        return EGUI_COLOR_HEX(0xEAF2FA);
    }

    switch (section->kind)
    {
    case PAGE_BASIC_SECTION_METRIC:
        return EGUI_COLOR_HEX(0xFFF8EE);
    case PAGE_BASIC_SECTION_CHECKLIST:
        return EGUI_COLOR_HEX(0xF4FBF7);
    default:
        return EGUI_COLOR_WHITE;
    }
}

static egui_color_t page_basic_kind_border_color(const page_basic_section_t *section, uint8_t selected)
{
    if (selected)
    {
        return EGUI_COLOR_HEX(0x2F5E8A);
    }

    switch (section->kind)
    {
    case PAGE_BASIC_SECTION_METRIC:
        return EGUI_COLOR_HEX(0xE2C688);
    case PAGE_BASIC_SECTION_CHECKLIST:
        return EGUI_COLOR_HEX(0x9ED0B3);
    default:
        return EGUI_COLOR_HEX(0xB7CBDB);
    }
}

static void page_basic_bind_hero_view(page_basic_hero_view_t *view, uint32_t index, const page_basic_section_t *section)
{
    uint8_t selected = (uint8_t)(section->stable_id == page_basic_ctx.selected_id);
    egui_color_t border = page_basic_kind_border_color(section, selected);

    snprintf(view->eyebrow_text, sizeof(view->eyebrow_text), "Hero");
    snprintf(view->title_text, sizeof(view->title_text), "Overview %03lu", (unsigned long)index);
    snprintf(view->body_text, sizeof(view->body_text), "Focus lane %u, revision %u, progress sync ready", (unsigned)((index % 5U) + 1U),
             (unsigned)section->revision);
    snprintf(view->badge_text, sizeof(view->badge_text), "%u%%", (unsigned)section->progress);

    egui_view_set_size(EGUI_VIEW_OF(&view->card), PAGE_BASIC_VIEW_W - 16, PAGE_BASIC_HERO_H - 8);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->card), page_basic_kind_fill_color(section, selected), EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&view->card), 1, border);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->eyebrow), view->eyebrow_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->title), view->title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->body), view->body_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->badge), view->badge_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&view->title), selected ? EGUI_COLOR_HEX(0x224665) : EGUI_COLOR_HEX(0x24384A), EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&view->badge), selected ? EGUI_COLOR_HEX(0x2F5E8A) : EGUI_COLOR_HEX(0x526678), EGUI_ALPHA_100);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&view->progress), section->progress);
    view->progress.bk_color = EGUI_COLOR_HEX(0xD8E4EC);
    view->progress.progress_color = selected ? EGUI_COLOR_HEX(0x2F5E8A) : EGUI_COLOR_HEX(0x5D8FC2);
    view->progress.control_color = view->progress.progress_color;
    view->progress.is_show_control = 0;
}

static void page_basic_bind_metric_view(page_basic_metric_view_t *view, uint32_t index, const page_basic_section_t *section)
{
    uint8_t selected = (uint8_t)(section->stable_id == page_basic_ctx.selected_id);
    egui_color_t border = page_basic_kind_border_color(section, selected);
    uint8_t left_value = (uint8_t)((section->progress + index * 3U) % 100U);
    uint8_t right_value = (uint8_t)((section->progress + 17U + index * 5U) % 100U);

    snprintf(view->title_text, sizeof(view->title_text), "Metrics %03lu", (unsigned long)index);
    snprintf(view->left_title_text, sizeof(view->left_title_text), "Load");
    snprintf(view->left_value_text, sizeof(view->left_value_text), "%u", (unsigned)left_value);
    snprintf(view->right_title_text, sizeof(view->right_title_text), "Yield");
    snprintf(view->right_value_text, sizeof(view->right_value_text), "%u", (unsigned)right_value);
    snprintf(view->footer_text, sizeof(view->footer_text), "Two KPI panels inside one section, revision %u", (unsigned)section->revision);

    egui_view_set_size(EGUI_VIEW_OF(&view->card), PAGE_BASIC_VIEW_W - 16, PAGE_BASIC_METRIC_H - 8);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->card), page_basic_kind_fill_color(section, selected), EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&view->card), 1, border);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->left_panel), selected ? EGUI_COLOR_HEX(0xFFF3D8) : EGUI_COLOR_HEX(0xFFF7E8), EGUI_ALPHA_100);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->right_panel), selected ? EGUI_COLOR_HEX(0xFBE7C9) : EGUI_COLOR_HEX(0xFFF2DC), EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&view->left_panel), 1, EGUI_COLOR_HEX(0xE0C78A));
    egui_view_card_set_border(EGUI_VIEW_OF(&view->right_panel), 1, EGUI_COLOR_HEX(0xD8B56E));
    egui_view_label_set_text(EGUI_VIEW_OF(&view->title), view->title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->left_title), view->left_title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->left_value), view->left_value_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->right_title), view->right_title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->right_value), view->right_value_text);
    egui_view_set_position(EGUI_VIEW_OF(&view->footer), 12, 72);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->footer), view->footer_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&view->title), selected ? EGUI_COLOR_HEX(0x644A15) : EGUI_COLOR_HEX(0x4A3A20), EGUI_ALPHA_100);
}

static void page_basic_bind_checklist_view(page_basic_checklist_view_t *view, uint32_t index, const page_basic_section_t *section)
{
    uint8_t selected = (uint8_t)(section->stable_id == page_basic_ctx.selected_id);
    uint8_t expanded = page_basic_is_checklist_expanded(section);
    egui_dim_t card_height = (egui_dim_t)(page_basic_measure_section_height_by_kind(section) - 8);
    egui_color_t border = page_basic_kind_border_color(section, selected);

    snprintf(view->title_text, sizeof(view->title_text), "Checklist %03lu", (unsigned long)index);
    snprintf(view->state_text, sizeof(view->state_text), expanded ? "Open" : "Fold");
    snprintf(view->row1_text, sizeof(view->row1_text), "1  stage inputs");
    snprintf(view->row2_text, sizeof(view->row2_text), "2  patch section");
    snprintf(view->row3_text, sizeof(view->row3_text), expanded ? "3  verify spacing" : "");
    snprintf(view->row4_text, sizeof(view->row4_text), expanded ? "4  confirm hit item" : "");
    snprintf(view->footer_text, sizeof(view->footer_text), expanded ? "Patch collapses this module" : "Patch expands this module");

    egui_view_set_position(EGUI_VIEW_OF(&view->card), 20, 4);
    egui_view_set_size(EGUI_VIEW_OF(&view->card), PAGE_BASIC_VIEW_W - 40, card_height);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&view->card), page_basic_kind_fill_color(section, selected), EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&view->card), 1, border);
    egui_view_set_position(EGUI_VIEW_OF(&view->row1), 14, 32);
    egui_view_set_position(EGUI_VIEW_OF(&view->row2), 14, 46);
    egui_view_set_position(EGUI_VIEW_OF(&view->row3), 14, 60);
    egui_view_set_position(EGUI_VIEW_OF(&view->row4), 14, 74);
    egui_view_set_position(EGUI_VIEW_OF(&view->footer), 14, expanded ? 90 : 62);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->title), view->title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->state), view->state_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->row1), view->row1_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->row2), view->row2_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->row3), view->row3_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->row4), view->row4_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&view->footer), view->footer_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&view->state), selected ? EGUI_COLOR_HEX(0x2F5E8A) : EGUI_COLOR_HEX(0x4F7C61), EGUI_ALPHA_100);
}

static egui_view_t *page_basic_ds_create_section_view(void *context, uint16_t section_type)
{
    EGUI_UNUSED(context);

    if (section_type == PAGE_BASIC_SECTION_METRIC)
    {
        page_basic_metric_view_t *view = (page_basic_metric_view_t *)egui_malloc(sizeof(page_basic_metric_view_t));
        if (view == NULL)
        {
            return NULL;
        }

        memset(view, 0, sizeof(*view));
        egui_view_group_init(EGUI_VIEW_OF(&view->root));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->root), page_basic_section_click_cb);

        egui_view_card_init(EGUI_VIEW_OF(&view->card));
        egui_view_set_position(EGUI_VIEW_OF(&view->card), 8, 4);
        egui_view_set_size(EGUI_VIEW_OF(&view->card), PAGE_BASIC_VIEW_W - 16, PAGE_BASIC_METRIC_H - 8);
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->card), page_basic_section_click_cb);
        egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->card));

        page_basic_init_label(&view->title, 12, 10, 200, 14, PAGE_BASIC_FONT_TITLE, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x4A3A20));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->title));

        egui_view_card_init(EGUI_VIEW_OF(&view->left_panel));
        egui_view_set_position(EGUI_VIEW_OF(&view->left_panel), 12, 30);
        egui_view_set_size(EGUI_VIEW_OF(&view->left_panel), (PAGE_BASIC_VIEW_W - 44) / 2, 40);
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->left_panel), page_basic_section_click_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->left_panel));

        egui_view_card_init(EGUI_VIEW_OF(&view->right_panel));
        egui_view_set_position(EGUI_VIEW_OF(&view->right_panel), 18 + (PAGE_BASIC_VIEW_W - 44) / 2, 30);
        egui_view_set_size(EGUI_VIEW_OF(&view->right_panel), (PAGE_BASIC_VIEW_W - 44) / 2, 40);
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->right_panel), page_basic_section_click_cb);
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->right_panel));

        page_basic_init_label(&view->left_title, 8, 6, 48, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x7A6242));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->left_panel), EGUI_VIEW_OF(&view->left_title));
        page_basic_init_label(&view->left_value, 8, 20, 72, 14, PAGE_BASIC_FONT_HERO, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x4A3A20));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->left_panel), EGUI_VIEW_OF(&view->left_value));

        page_basic_init_label(&view->right_title, 8, 6, 48, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x7A6242));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->right_panel), EGUI_VIEW_OF(&view->right_title));
        page_basic_init_label(&view->right_value, 8, 20, 72, 14, PAGE_BASIC_FONT_HERO, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x4A3A20));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->right_panel), EGUI_VIEW_OF(&view->right_value));

        page_basic_init_label(&view->footer, 12, 72, PAGE_BASIC_VIEW_W - 40, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                              EGUI_COLOR_HEX(0x7A6242));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->footer));

        return EGUI_VIEW_OF(&view->root);
    }

    if (section_type == PAGE_BASIC_SECTION_CHECKLIST)
    {
        page_basic_checklist_view_t *view = (page_basic_checklist_view_t *)egui_malloc(sizeof(page_basic_checklist_view_t));
        if (view == NULL)
        {
            return NULL;
        }

        memset(view, 0, sizeof(*view));
        egui_view_group_init(EGUI_VIEW_OF(&view->root));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->root), page_basic_section_click_cb);

        egui_view_card_init(EGUI_VIEW_OF(&view->card));
        egui_view_set_position(EGUI_VIEW_OF(&view->card), 20, 4);
        egui_view_set_size(EGUI_VIEW_OF(&view->card), PAGE_BASIC_VIEW_W - 40, PAGE_BASIC_CHECKLIST_H - 8);
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->card), page_basic_section_click_cb);
        egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->card));

        page_basic_init_label(&view->title, 12, 10, 160, 14, PAGE_BASIC_FONT_TITLE, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x264A37));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->title));
        page_basic_init_label(&view->state, 150, 10, 56, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x4F7C61));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->state));

        page_basic_init_label(&view->row1, 14, 32, 180, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x315342));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->row1));
        page_basic_init_label(&view->row2, 14, 46, 180, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x315342));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->row2));
        page_basic_init_label(&view->row3, 14, 60, 180, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x315342));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->row3));
        page_basic_init_label(&view->row4, 14, 74, 180, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x315342));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->row4));
        page_basic_init_label(&view->footer, 14, 62, 180, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x597564));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->footer));

        return EGUI_VIEW_OF(&view->root);
    }

    {
        page_basic_hero_view_t *view = (page_basic_hero_view_t *)egui_malloc(sizeof(page_basic_hero_view_t));
        if (view == NULL)
        {
            return NULL;
        }

        memset(view, 0, sizeof(*view));
        egui_view_group_init(EGUI_VIEW_OF(&view->root));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->root), page_basic_section_click_cb);

        egui_view_card_init(EGUI_VIEW_OF(&view->card));
        egui_view_set_position(EGUI_VIEW_OF(&view->card), 8, 4);
        egui_view_set_size(EGUI_VIEW_OF(&view->card), PAGE_BASIC_VIEW_W - 16, PAGE_BASIC_HERO_H - 8);
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&view->card), page_basic_section_click_cb);
        egui_view_group_add_child(EGUI_VIEW_OF(&view->root), EGUI_VIEW_OF(&view->card));

        page_basic_init_label(&view->eyebrow, 12, 8, 48, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x57748E));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->eyebrow));
        page_basic_init_label(&view->badge, PAGE_BASIC_VIEW_W - 92, 8, 56, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER,
                              EGUI_COLOR_HEX(0x526678));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->badge));
        page_basic_init_label(&view->title, 12, 24, PAGE_BASIC_VIEW_W - 48, 16, PAGE_BASIC_FONT_HERO, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                              EGUI_COLOR_HEX(0x24384A));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->title));
        page_basic_init_label(&view->body, 12, 44, PAGE_BASIC_VIEW_W - 48, 12, PAGE_BASIC_FONT_BODY, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                              EGUI_COLOR_HEX(0x526678));
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->body));

        egui_view_progress_bar_init(EGUI_VIEW_OF(&view->progress));
        egui_view_set_position(EGUI_VIEW_OF(&view->progress), 12, 62);
        egui_view_set_size(EGUI_VIEW_OF(&view->progress), PAGE_BASIC_VIEW_W - 40, 8);
        egui_view_card_add_child(EGUI_VIEW_OF(&view->card), EGUI_VIEW_OF(&view->progress));

        return EGUI_VIEW_OF(&view->root);
    }
}

static void page_basic_ds_destroy_section_view(void *context, egui_view_t *view, uint16_t section_type)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(section_type);
    egui_free(view);
}

static void page_basic_ds_bind_section_view(void *context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    page_basic_context_t *ctx = (page_basic_context_t *)context;
    const page_basic_section_t *section = index < PAGE_BASIC_SECTION_COUNT ? &ctx->sections[index] : NULL;

    EGUI_UNUSED(stable_id);

    if (section == NULL)
    {
        return;
    }

    switch (section->kind)
    {
    case PAGE_BASIC_SECTION_METRIC:
        page_basic_bind_metric_view((page_basic_metric_view_t *)view, index, section);
        break;
    case PAGE_BASIC_SECTION_CHECKLIST:
        page_basic_bind_checklist_view((page_basic_checklist_view_t *)view, index, section);
        break;
    default:
        page_basic_bind_hero_view((page_basic_hero_view_t *)view, index, section);
        break;
    }
}

static uint8_t page_basic_ds_should_keep_alive(void *context, egui_view_t *view, uint32_t stable_id)
{
    EGUI_UNUSED(context);
    EGUI_UNUSED(view);

    return stable_id == page_basic_ctx.selected_id;
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

static uint8_t page_basic_is_view_clickable(egui_view_t *view)
{
    int click_x;
    int click_y;

    return (uint8_t)egui_sim_get_view_clipped_center(view, &EGUI_VIEW_OF(&page_view)->region_screen, &click_x, &click_y);
}

static uint8_t page_basic_set_click_item_action(egui_sim_action_t *p_action, egui_view_t *view, uint32_t interval_ms)
{
    return (uint8_t)egui_sim_set_click_view_clipped(p_action, view, &EGUI_VIEW_OF(&page_view)->region_screen, (int)interval_ms);
}

static egui_view_t *page_basic_find_visible_view_by_index(uint32_t index)
{
    uint32_t stable_id;
    const egui_view_virtual_page_slot_t *slot;
    egui_view_t *view;

    if (index >= PAGE_BASIC_SECTION_COUNT)
    {
        return NULL;
    }

    stable_id = page_basic_ctx.sections[index].stable_id;
    slot = egui_view_virtual_page_find_slot_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id);
    if (slot == NULL)
    {
        return NULL;
    }

    view = egui_view_virtual_page_find_view_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id);
    return page_basic_is_view_clickable(view) ? view : NULL;
}

static void page_basic_set_scroll_action(egui_sim_action_t *p_action, uint32_t interval_ms)
{
    egui_region_t *region = &EGUI_VIEW_OF(&page_view)->region_screen;

    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = (egui_dim_t)(region->location.x + region->size.width / 2);
    p_action->y1 = (egui_dim_t)(region->location.y + region->size.height - 24);
    p_action->x2 = p_action->x1;
    p_action->y2 = (egui_dim_t)(region->location.y + 42);
    p_action->steps = 10;
    p_action->interval_ms = interval_ms;
}

static uint32_t page_basic_get_first_visible_index(void)
{
    page_basic_visible_summary_t summary;

    memset(&summary, 0, sizeof(summary));
    egui_view_virtual_page_visit_visible_sections(EGUI_VIEW_OF(&page_view), page_basic_visible_section_visitor, &summary);
    return summary.has_first ? summary.first_index : PAGE_BASIC_INVALID_INDEX;
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
            if (egui_view_virtual_page_get_slot_count(EGUI_VIEW_OF(&page_view)) == 0U)
            {
                report_runtime_failure("page basic should materialize initial slots");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        view = page_basic_find_visible_view_by_index(0);
        if (view == NULL)
        {
            report_runtime_failure("first hero section was not visible");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        if (!page_basic_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("first hero section click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
        }
        return true;
    case 2:
        view = page_basic_find_visible_view_by_index(0);
        if (page_basic_ctx.selected_id != page_basic_ctx.sections[0].stable_id)
        {
            if (recording_click_verify_retry < PAGE_BASIC_CLICK_VERIFY_RETRY_MAX)
            {
                recording_click_verify_retry++;
                if (view != NULL && page_basic_set_click_item_action(p_action, view, 220))
                {
                    return true;
                }
                recording_request_snapshot();
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            report_runtime_failure("section click did not update selected section");
        }
        recording_click_verify_retry = 0U;
        recording_jump_verify_retry = 0U;
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[PAGE_BASIC_ACTION_JUMP]), 220);
        return true;
    case 3:
        view = page_basic_find_visible_view_by_index(page_basic_ctx.jump_cursor);
        if (page_basic_ctx.selected_id != page_basic_ctx.sections[page_basic_ctx.jump_cursor].stable_id)
        {
            if (recording_jump_verify_retry < PAGE_BASIC_JUMP_VERIFY_RETRY_MAX)
            {
                uint32_t stable_id = page_basic_ctx.sections[page_basic_ctx.jump_cursor].stable_id;

                page_basic_abort_motion();
                egui_view_virtual_viewport_set_anchor(EGUI_VIEW_OF(&page_view), stable_id, 0);
                egui_view_virtual_page_scroll_to_section_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id, 0);
                (void)egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id, 0);
                recording_jump_verify_retry++;
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            report_runtime_failure("jump action did not select target section");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        recording_jump_verify_retry = 0U;
        if (view != NULL && !page_basic_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("target section click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
            return true;
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 4:
        if (first_call && page_basic_ctx.selected_id != page_basic_ctx.sections[page_basic_ctx.jump_cursor].stable_id)
        {
            report_runtime_failure("target section was not selected correctly after jump");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[PAGE_BASIC_ACTION_PATCH]), 220);
        return true;
    case 5:
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 6:
        if (first_call && !page_basic_ctx.sections[page_basic_ctx.jump_cursor].expanded)
        {
            report_runtime_failure("patch action did not expand checklist section");
        }
        page_basic_set_scroll_action(p_action, 320);
        return true;
    case 7:
        if (first_call && egui_view_virtual_page_get_scroll_y(EGUI_VIEW_OF(&page_view)) <= 0)
        {
            report_runtime_failure("scroll action did not move page viewport");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[PAGE_BASIC_ACTION_RESET]), 220);
        return true;
    case 8:
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 9:
        if (first_call)
        {
            if (page_basic_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
            {
                report_runtime_failure("reset action did not clear selected section");
            }
            if (egui_view_virtual_page_get_scroll_y(EGUI_VIEW_OF(&page_view)) != 0)
            {
                report_runtime_failure("reset action did not restore top position");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 10:
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
    egui_view_virtual_page_setup_t setup = {
            .params = &page_basic_view_params,
            .data_source = &page_basic_data_source,
            .data_source_context = &page_basic_ctx,
            .state_cache_max_entries = 0,
            .state_cache_max_bytes = 0,
    };

    memset(&page_basic_ctx, 0, sizeof(page_basic_ctx));
    page_basic_init_sections();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0U;
    recording_click_verify_retry = 0U;
    recording_jump_verify_retry = 0U;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&page_basic_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &page_basic_toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD1DEE7));

    for (i = 0; i < PAGE_BASIC_ACTION_COUNT; i++)
    {
        page_basic_style_action_button(&action_buttons[i], i);
        egui_view_set_position(EGUI_VIEW_OF(&action_buttons[i]), 10 + i * (PAGE_BASIC_ACTION_W + PAGE_BASIC_ACTION_GAP), 6);
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
    }

    egui_view_virtual_page_init_with_setup(EGUI_VIEW_OF(&page_view), &setup);
    egui_view_set_background(EGUI_VIEW_OF(&page_view), EGUI_BG_OF(&page_basic_view_bg));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&page_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
}
