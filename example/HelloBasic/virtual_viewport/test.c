#include "egui.h"
#include <stdio.h>
#include <string.h>
#include "uicode.h"

#define DEMO_ITEM_COUNT       1200U
#define DEMO_HEADER_X         8
#define DEMO_HEADER_Y         8
#define DEMO_HEADER_W         (EGUI_CONFIG_SCEEN_WIDTH - 16)
#define DEMO_HEADER_H         66
#define DEMO_LIST_X           8
#define DEMO_LIST_Y           (DEMO_HEADER_Y + DEMO_HEADER_H + 8)
#define DEMO_LIST_W           (EGUI_CONFIG_SCEEN_WIDTH - 16)
#define DEMO_LIST_H           (EGUI_CONFIG_SCEEN_HEIGHT - DEMO_LIST_Y - 8)
#define DEMO_ROW_GAP          8
#define DEMO_ROW_SIDE_INSET   8
#define DEMO_INVALID_INDEX    0xFFFFFFFFUL
#define DEMO_TITLE_TEXT_LEN   48
#define DEMO_SUBTEXT_LEN      72
#define DEMO_META_TEXT_LEN    72
#define DEMO_BADGE_TEXT_LEN   20
#define DEMO_STATUS_TEXT_LEN  96

#define DEMO_FONT_TITLE   ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define DEMO_FONT_BODY    ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define DEMO_FONT_CAPTION ((const egui_font_t *)&egui_res_font_montserrat_8_4)

enum
{
    DEMO_ROW_MODE_HERO = 0,
    DEMO_ROW_MODE_DETAIL,
    DEMO_ROW_MODE_COMPACT,
};

typedef struct demo_virtual_row demo_virtual_row_t;
typedef struct demo_virtual_viewport_context demo_virtual_viewport_context_t;

struct demo_virtual_row
{
    egui_view_group_t root;
    egui_view_card_t surface;
    egui_view_label_t title;
    egui_view_label_t subtitle;
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

struct demo_virtual_viewport_context
{
    uint32_t item_count;
    uint32_t selected_id;
    uint32_t click_count;
    uint32_t last_clicked_index;
    uint8_t created_count;
    demo_virtual_row_t rows[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
    char title_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][DEMO_TITLE_TEXT_LEN];
    char subtitle_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][DEMO_SUBTEXT_LEN];
    char meta_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][DEMO_META_TEXT_LEN];
    char badge_texts[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS][DEMO_BADGE_TEXT_LEN];
    char status_detail[DEMO_STATUS_TEXT_LEN];
    char status_hint[DEMO_STATUS_TEXT_LEN];
};

static egui_view_t background_view;
static egui_view_card_t header_card;
static egui_view_label_t header_title;
static egui_view_label_t header_detail;
static egui_view_label_t header_hint;
static egui_view_virtual_list_t viewport_1;
static demo_virtual_viewport_context_t viewport_context;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(header_card_params, DEMO_HEADER_X, DEMO_HEADER_Y, DEMO_HEADER_W, DEMO_HEADER_H, 14);
EGUI_VIEW_VIRTUAL_LIST_PARAMS_INIT(viewport_1_params, DEMO_LIST_X, DEMO_LIST_Y, DEMO_LIST_W, DEMO_LIST_H);

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_THEME_SURFACE_VARIANT, EGUI_THEME_SURFACE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(screen_bg_params, &screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(screen_bg, &screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(list_bg_param, EGUI_THEME_SURFACE, EGUI_ALPHA_80, 14);
EGUI_BACKGROUND_PARAM_INIT(list_bg_params, &list_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(list_bg, &list_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_success_param, EGUI_THEME_SUCCESS, EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_success_params, &badge_success_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_success_bg, &badge_success_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_warning_param, EGUI_THEME_WARNING, EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_warning_params, &badge_warning_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_warning_bg, &badge_warning_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_secondary_param, EGUI_THEME_SECONDARY, EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_secondary_params, &badge_secondary_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_secondary_bg, &badge_secondary_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(badge_selected_param, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(badge_selected_params, &badge_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(badge_selected_bg, &badge_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(pulse_live_param, EGUI_THEME_SUCCESS, EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(pulse_live_params, &pulse_live_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(pulse_live_bg, &pulse_live_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(pulse_selected_param, EGUI_THEME_PRIMARY, EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(pulse_selected_params, &pulse_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(pulse_selected_bg, &pulse_selected_params);

EGUI_SHADOW_PARAM_INIT_ROUND(demo_card_shadow, 10, 0, 3, EGUI_COLOR_BLACK, EGUI_ALPHA_20, 12);
EGUI_ANIMATION_ALPHA_PARAMS_INIT(pulse_anim_param, EGUI_ALPHA_100, EGUI_ALPHA_20);

static uint32_t demo_make_stable_id(uint32_t index)
{
    return 10000U + index;
}

static uint8_t demo_get_item_mode(uint32_t index)
{
    return (uint8_t)(index % 3U);
}

static uint8_t demo_is_live_index(uint32_t index)
{
    return (index % 7U) == 0U;
}

static uint8_t demo_is_selected_id(uint32_t stable_id)
{
    return stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && viewport_context.selected_id == stable_id;
}

static int32_t demo_find_index_by_stable_id(uint32_t stable_id)
{
    if (stable_id < 10000U)
    {
        return -1;
    }
    if ((stable_id - 10000U) >= viewport_context.item_count)
    {
        return -1;
    }

    return (int32_t)(stable_id - 10000U);
}

static int32_t demo_get_item_height_by_stable_id(uint32_t stable_id)
{
    int32_t base_height;
    int32_t selected_extra = demo_is_selected_id(stable_id) ? 28 : 0;
    uint8_t mode;
    int32_t index = demo_find_index_by_stable_id(stable_id);

    if (index < 0)
    {
        return 68;
    }

    mode = demo_get_item_mode((uint32_t)index);
    switch (mode)
    {
    case DEMO_ROW_MODE_HERO:
        base_height = 96;
        break;
    case DEMO_ROW_MODE_DETAIL:
        base_height = 76;
        break;
    default:
        base_height = 58;
        break;
    }

    return base_height + selected_extra;
}

static egui_dim_t demo_get_list_width(void)
{
    egui_dim_t width = EGUI_VIEW_OF(&viewport_1)->region.size.width;
    return width > 0 ? width : DEMO_LIST_W;
}

static int demo_get_row_pool_index(demo_virtual_row_t *row)
{
    uint8_t i;

    for (i = 0; i < viewport_context.created_count; i++)
    {
        if (row == &viewport_context.rows[i])
        {
            return (int)i;
        }
    }

    return -1;
}

static demo_virtual_row_t *demo_find_row_by_root_view(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < viewport_context.created_count; i++)
    {
        if (view == EGUI_VIEW_OF(&viewport_context.rows[i].root))
        {
            return &viewport_context.rows[i];
        }
    }

    return NULL;
}

static void demo_update_status_labels(void)
{
    int32_t selected_index = demo_find_index_by_stable_id(viewport_context.selected_id);

    if (selected_index >= 0)
    {
        snprintf(viewport_context.status_detail, sizeof(viewport_context.status_detail), "sel #%04ld  id=%05lu  slots=%u/%u", (long)selected_index,
                 (unsigned long)viewport_context.selected_id, (unsigned)viewport_context.created_count, (unsigned)EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS);
        snprintf(viewport_context.status_hint, sizeof(viewport_context.status_hint), "Tap rows. Live and selected items stay alive.");
    }
    else
    {
        snprintf(viewport_context.status_detail, sizeof(viewport_context.status_detail), "Tap row. items=%lu  slots=%u",
                 (unsigned long)viewport_context.item_count, (unsigned)EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS);
        snprintf(viewport_context.status_hint, sizeof(viewport_context.status_hint), "Mixed heights, clicks, pulse and slot reuse.");
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&header_detail), viewport_context.status_detail);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_hint), viewport_context.status_hint);
}

static void demo_set_row_pulse(demo_virtual_row_t *row, uint8_t visible, uint8_t selected)
{
    if (!visible)
    {
        if (row->pulse_running)
        {
            egui_animation_stop(EGUI_ANIM_OF(&row->pulse_anim));
            row->pulse_running = 0;
        }
        egui_view_set_gone(EGUI_VIEW_OF(&row->pulse), 1);
        egui_view_set_alpha(EGUI_VIEW_OF(&row->pulse), EGUI_ALPHA_100);
        return;
    }

    egui_view_set_gone(EGUI_VIEW_OF(&row->pulse), 0);
    egui_view_set_background(EGUI_VIEW_OF(&row->pulse), selected ? EGUI_BG_OF(&pulse_selected_bg) : EGUI_BG_OF(&pulse_live_bg));

    if (!row->pulse_running)
    {
        egui_animation_start(EGUI_ANIM_OF(&row->pulse_anim));
        row->pulse_running = 1;
    }
}

static void demo_apply_row_theme(demo_virtual_row_t *row, uint8_t mode, uint8_t selected, uint8_t live)
{
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t title_color = EGUI_THEME_TEXT_PRIMARY;
    egui_color_t subtitle_color = EGUI_THEME_TEXT_SECONDARY;
    egui_color_t meta_color = live ? EGUI_THEME_SUCCESS : EGUI_THEME_TEXT_SECONDARY;
    egui_background_t *badge_bg = EGUI_BG_OF(&badge_secondary_bg);
    egui_color_t badge_text_color = EGUI_COLOR_WHITE;

    if (selected)
    {
        surface_color = EGUI_THEME_PRIMARY_DARK;
        border_color = EGUI_COLOR_WHITE;
        title_color = EGUI_COLOR_WHITE;
        subtitle_color = EGUI_COLOR_WHITE;
        meta_color = EGUI_COLOR_WHITE;
        badge_bg = EGUI_BG_OF(&badge_selected_bg);
        badge_text_color = EGUI_THEME_PRIMARY_DARK;
    }
    else
    {
        switch (mode)
        {
        case DEMO_ROW_MODE_HERO:
            surface_color = EGUI_THEME_SURFACE_VARIANT;
            border_color = EGUI_THEME_BORDER;
            badge_bg = EGUI_BG_OF(&badge_warning_bg);
            badge_text_color = EGUI_COLOR_BLACK;
            meta_color = live ? EGUI_THEME_SUCCESS : EGUI_THEME_WARNING;
            break;
        case DEMO_ROW_MODE_DETAIL:
            surface_color = EGUI_THEME_SURFACE;
            border_color = EGUI_THEME_BORDER;
            badge_bg = EGUI_BG_OF(&badge_success_bg);
            break;
        default:
            surface_color = EGUI_THEME_TRACK_BG;
            border_color = EGUI_THEME_TRACK_OFF;
            badge_bg = EGUI_BG_OF(&badge_secondary_bg);
            break;
        }
    }

    egui_view_card_set_bg_color(EGUI_VIEW_OF(&row->surface), surface_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&row->surface), selected ? 3 : 1, border_color);
    egui_view_set_background(EGUI_VIEW_OF(&row->badge), badge_bg);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->title), title_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->subtitle), subtitle_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->meta), meta_color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&row->badge), badge_text_color, EGUI_ALPHA_100);
}

static void demo_layout_row(demo_virtual_row_t *row, uint8_t mode, int32_t total_height, uint8_t selected, uint8_t live)
{
    egui_dim_t row_width = demo_get_list_width();
    egui_dim_t surface_x = DEMO_ROW_SIDE_INSET;
    egui_dim_t surface_y = DEMO_ROW_GAP / 2;
    egui_dim_t surface_w = row_width - DEMO_ROW_SIDE_INSET * 2;
    egui_dim_t surface_h = (egui_dim_t)(total_height - DEMO_ROW_GAP);
    egui_dim_t badge_w = selected ? 56 : 46;
    egui_dim_t badge_h = 16;
    egui_dim_t pulse_size = 8;
    egui_dim_t progress_h = 5;
    egui_dim_t content_left = 12;
    egui_dim_t content_right = 12;
    egui_dim_t badge_x;
    egui_dim_t title_w;
    egui_dim_t subtitle_w;
    egui_dim_t meta_w;
    egui_dim_t title_y = 8;
    egui_dim_t subtitle_y = 27;
    egui_dim_t meta_y;
    uint8_t show_subtitle = (mode != DEMO_ROW_MODE_COMPACT) || selected;
    uint8_t show_progress = (mode == DEMO_ROW_MODE_HERO) || selected;

    if (surface_w < 80)
    {
        surface_w = 80;
    }
    if (surface_h < 40)
    {
        surface_h = 40;
    }

    badge_x = surface_w - badge_w - 12;
    title_w = badge_x - content_left - (live || selected ? 18 : 8);
    if (title_w < 70)
    {
        title_w = 70;
    }
    subtitle_w = surface_w - content_left - content_right;
    meta_w = surface_w - content_left - content_right;

    meta_y = show_progress ? (surface_h - 24) : (surface_h - 16);

    egui_view_set_position(EGUI_VIEW_OF(&row->surface), surface_x, surface_y);
    egui_view_set_size(EGUI_VIEW_OF(&row->surface), surface_w, surface_h);

    egui_view_set_position(EGUI_VIEW_OF(&row->badge), badge_x, 8);
    egui_view_set_size(EGUI_VIEW_OF(&row->badge), badge_w, badge_h);

    egui_view_set_position(EGUI_VIEW_OF(&row->pulse), badge_x - 16, 12);
    egui_view_set_size(EGUI_VIEW_OF(&row->pulse), pulse_size + 1, pulse_size + 1);

    egui_view_set_position(EGUI_VIEW_OF(&row->title), content_left, title_y);
    egui_view_set_size(EGUI_VIEW_OF(&row->title), title_w, 12);

    egui_view_set_position(EGUI_VIEW_OF(&row->subtitle), content_left, subtitle_y);
    egui_view_set_size(EGUI_VIEW_OF(&row->subtitle), subtitle_w, 10);

    egui_view_set_position(EGUI_VIEW_OF(&row->meta), content_left, meta_y);
    egui_view_set_size(EGUI_VIEW_OF(&row->meta), meta_w, 10);

    egui_view_set_position(EGUI_VIEW_OF(&row->progress), content_left, surface_h - 12);
    egui_view_set_size(EGUI_VIEW_OF(&row->progress), surface_w - content_left - content_right, progress_h);

    egui_view_set_gone(EGUI_VIEW_OF(&row->subtitle), show_subtitle ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&row->progress), show_progress ? 0 : 1);
    demo_set_row_pulse(row, live || selected, selected);
}

static void demo_bind_row_text(demo_virtual_row_t *row, uint32_t index, uint32_t stable_id)
{
    int pool_index = demo_get_row_pool_index(row);
    uint8_t mode = demo_get_item_mode(index);
    uint8_t selected = demo_is_selected_id(stable_id);
    uint8_t live = demo_is_live_index(index);
    uint8_t progress = (uint8_t)((index * 13U + 17U) % 100U);

    if (pool_index < 0)
    {
        return;
    }

    switch (mode)
    {
    case DEMO_ROW_MODE_HERO:
        snprintf(viewport_context.title_texts[pool_index], sizeof(viewport_context.title_texts[pool_index]), "Pipeline #%04lu", (unsigned long)index);
        snprintf(viewport_context.subtitle_texts[pool_index], sizeof(viewport_context.subtitle_texts[pool_index]), "%s", selected
                                                                                                                                ? "Expanded hero row. Pulse and progress kept."
                                                                                                                                : "Large row with pulse and roomy spacing.");
        snprintf(viewport_context.badge_texts[pool_index], sizeof(viewport_context.badge_texts[pool_index]), "%s", selected ? "SELECT" : "HERO");
        break;
    case DEMO_ROW_MODE_DETAIL:
        snprintf(viewport_context.title_texts[pool_index], sizeof(viewport_context.title_texts[pool_index]), "Scene #%04lu", (unsigned long)index);
        snprintf(viewport_context.subtitle_texts[pool_index], sizeof(viewport_context.subtitle_texts[pool_index]), "%s", selected
                                                                                                                                ? "Expanded row. Keepalive preserves state."
                                                                                                                                : "Tap to expand and verify slot reuse.");
        snprintf(viewport_context.badge_texts[pool_index], sizeof(viewport_context.badge_texts[pool_index]), "%s", selected ? "ACTIVE" : "DETAIL");
        break;
    default:
        snprintf(viewport_context.title_texts[pool_index], sizeof(viewport_context.title_texts[pool_index]), "Note #%04lu", (unsigned long)index);
        snprintf(viewport_context.subtitle_texts[pool_index], sizeof(viewport_context.subtitle_texts[pool_index]), "%s", selected
                                                                                                                                ? "Compact row opened with more detail."
                                                                                                                                : "Compact row. Tap to reveal detail.");
        snprintf(viewport_context.badge_texts[pool_index], sizeof(viewport_context.badge_texts[pool_index]), "%s", selected ? "OPEN" : "COMPACT");
        break;
    }

    snprintf(viewport_context.meta_texts[pool_index], sizeof(viewport_context.meta_texts[pool_index]), "#%05lu  %u%%  h=%ld%s",
             (unsigned long)stable_id, (unsigned)progress, (long)demo_get_item_height_by_stable_id(stable_id), live ? "  live" : "");

    egui_view_label_set_text(EGUI_VIEW_OF(&row->title), viewport_context.title_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->subtitle), viewport_context.subtitle_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->meta), viewport_context.meta_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&row->badge), viewport_context.badge_texts[pool_index]);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&row->progress), progress);

    demo_apply_row_theme(row, mode, selected, live);
    demo_layout_row(row, mode, demo_get_item_height_by_stable_id(stable_id), selected, live);
}

static void virtual_item_click_cb(egui_view_t *self)
{
    demo_virtual_row_t *row = demo_find_row_by_root_view(self);
    uint32_t previous_selected = viewport_context.selected_id;
    int32_t previous_index = demo_find_index_by_stable_id(previous_selected);

    if (row == NULL || row->stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return;
    }

    viewport_context.selected_id = row->stable_id;
    viewport_context.click_count++;
    viewport_context.last_clicked_index = row->bound_index;

    if (previous_selected != viewport_context.selected_id && previous_index >= 0)
    {
        egui_view_virtual_list_notify_item_resized(EGUI_VIEW_OF(&viewport_1), (uint32_t)previous_index);
    }
    egui_view_virtual_list_notify_item_resized(EGUI_VIEW_OF(&viewport_1), row->bound_index);
    demo_update_status_labels();

    EGUI_LOG_INF("Virtual row clicked: index=%d stable_id=%lu\n", (int)row->bound_index, (unsigned long)row->stable_id);
}

static uint32_t demo_get_count(void *adapter_context)
{
    return ((demo_virtual_viewport_context_t *)adapter_context)->item_count;
}

static uint32_t demo_get_stable_id(void *adapter_context, uint32_t index)
{
    EGUI_UNUSED(adapter_context);
    return demo_make_stable_id(index);
}

static int32_t demo_adapter_find_index_by_stable_id(void *adapter_context, uint32_t stable_id)
{
    demo_virtual_viewport_context_t *ctx = (demo_virtual_viewport_context_t *)adapter_context;

    if (stable_id < 10000U)
    {
        return -1;
    }
    if ((stable_id - 10000U) >= ctx->item_count)
    {
        return -1;
    }

    return (int32_t)(stable_id - 10000U);
}

static uint16_t demo_get_view_type(void *adapter_context, uint32_t index)
{
    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(index);
    return 0;
}

static int32_t demo_measure_main_size(void *adapter_context, uint32_t index, int32_t cross_size_hint)
{
    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(cross_size_hint);
    return demo_get_item_height_by_stable_id(demo_make_stable_id(index));
}

static egui_view_t *demo_create_view(void *adapter_context, uint16_t view_type)
{
    demo_virtual_viewport_context_t *ctx = (demo_virtual_viewport_context_t *)adapter_context;
    demo_virtual_row_t *row;

    EGUI_UNUSED(view_type);

    if (ctx->created_count >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
    {
        return NULL;
    }

    row = &ctx->rows[ctx->created_count];
    memset(row, 0, sizeof(*row));
    row->bound_index = DEMO_INVALID_INDEX;
    row->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;

    egui_view_group_init(EGUI_VIEW_OF(&row->root));
    egui_view_card_init(EGUI_VIEW_OF(&row->surface));
    egui_view_label_init(EGUI_VIEW_OF(&row->title));
    egui_view_label_init(EGUI_VIEW_OF(&row->subtitle));
    egui_view_label_init(EGUI_VIEW_OF(&row->meta));
    egui_view_label_init(EGUI_VIEW_OF(&row->badge));
    egui_view_progress_bar_init(EGUI_VIEW_OF(&row->progress));
    egui_view_init(EGUI_VIEW_OF(&row->pulse));

    egui_view_label_set_font(EGUI_VIEW_OF(&row->title), DEMO_FONT_TITLE);
    egui_view_label_set_font(EGUI_VIEW_OF(&row->subtitle), DEMO_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&row->meta), DEMO_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&row->badge), DEMO_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->subtitle), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->meta), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&row->badge), EGUI_ALIGN_CENTER);

    egui_view_group_add_child(EGUI_VIEW_OF(&row->root), EGUI_VIEW_OF(&row->surface));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->title));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->subtitle));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->meta));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->badge));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->progress));
    egui_view_card_add_child(EGUI_VIEW_OF(&row->surface), EGUI_VIEW_OF(&row->pulse));

    egui_view_set_shadow(EGUI_VIEW_OF(&row->surface), &demo_card_shadow);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&row->root), virtual_item_click_cb);

    egui_animation_alpha_init(EGUI_ANIM_OF(&row->pulse_anim));
    egui_animation_alpha_params_set(&row->pulse_anim, &pulse_anim_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&row->pulse_anim), 900);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&row->pulse_anim), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&row->pulse_anim), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_interpolator_linear_init((egui_interpolator_t *)&row->pulse_interp);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&row->pulse_anim), (egui_interpolator_t *)&row->pulse_interp);
    egui_animation_target_view_set(EGUI_ANIM_OF(&row->pulse_anim), EGUI_VIEW_OF(&row->pulse));
    egui_view_set_gone(EGUI_VIEW_OF(&row->pulse), 1);

    ctx->created_count++;
    return EGUI_VIEW_OF(&row->root);
}

static void demo_bind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    demo_virtual_row_t *row = demo_find_row_by_root_view(view);

    EGUI_UNUSED(adapter_context);

    if (row == NULL)
    {
        return;
    }

    row->bound_index = index;
    row->stable_id = stable_id;
    demo_bind_row_text(row, index, stable_id);
}

static void demo_unbind_view(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    demo_virtual_row_t *row = demo_find_row_by_root_view(view);

    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(stable_id);

    if (row == NULL)
    {
        return;
    }

    demo_set_row_pulse(row, 0, 0);
    row->bound_index = DEMO_INVALID_INDEX;
    row->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static uint8_t demo_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    int32_t index;

    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(view);

    if (stable_id == viewport_context.selected_id)
    {
        return 1;
    }

    index = demo_find_index_by_stable_id(stable_id);
    if (index < 0)
    {
        return 0;
    }

    return demo_is_live_index((uint32_t)index);
}

static const egui_view_virtual_list_adapter_t viewport_adapter = {
        .get_count = demo_get_count,
        .get_stable_id = demo_get_stable_id,
        .find_index_by_stable_id = demo_adapter_find_index_by_stable_id,
        .get_view_type = demo_get_view_type,
        .measure_main_size = demo_measure_main_size,
        .create_view = demo_create_view,
        .destroy_view = NULL,
        .bind_view = demo_bind_view,
        .unbind_view = demo_unbind_view,
        .should_keep_alive = demo_should_keep_alive,
        .save_state = NULL,
        .restore_state = NULL,
};

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

static uint8_t is_slot_rendered_in_viewport(const egui_view_virtual_viewport_slot_t *slot)
{
    int16_t main_center;
    int16_t viewport_extent;

    if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE)
    {
        return 0;
    }

    main_center = slot->render_region.location.y + slot->render_region.size.height / 2;
    viewport_extent = EGUI_VIEW_OF(&viewport_1)->region.size.height;

    return (main_center >= 0) && (main_center < viewport_extent);
}

static egui_view_t *find_visible_view_by_item_index(uint32_t index)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        const egui_view_virtual_list_slot_t *slot = egui_view_virtual_list_get_slot(EGUI_VIEW_OF(&viewport_1), i);

        if (slot != NULL && slot->index == index && is_slot_rendered_in_viewport(slot))
        {
            return slot->view;
        }
    }

    return NULL;
}

static egui_view_t *find_first_visible_view_after(uint32_t min_index)
{
    uint8_t i;
    const egui_view_virtual_viewport_slot_t *best = NULL;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        const egui_view_virtual_list_slot_t *slot = egui_view_virtual_list_get_slot(EGUI_VIEW_OF(&viewport_1), i);

        if (slot == NULL || slot->index < min_index || !is_slot_rendered_in_viewport(slot))
        {
            continue;
        }
        if (best == NULL || slot->index < best->index)
        {
            best = slot;
        }
    }

    return best != NULL ? best->view : NULL;
}
#endif

void test_init_ui(void)
{
    memset(&viewport_context, 0, sizeof(viewport_context));
    viewport_context.item_count = DEMO_ITEM_COUNT;
    viewport_context.selected_id = demo_make_stable_id(2);
    viewport_context.last_clicked_index = DEMO_INVALID_INDEX;

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&header_card), &header_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_card), EGUI_THEME_SURFACE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_card), 1, EGUI_THEME_BORDER);
    egui_view_set_shadow(EGUI_VIEW_OF(&header_card), &demo_card_shadow);

    egui_view_label_init(EGUI_VIEW_OF(&header_title));
    egui_view_label_init(EGUI_VIEW_OF(&header_detail));
    egui_view_label_init(EGUI_VIEW_OF(&header_hint));

    egui_view_label_set_font(EGUI_VIEW_OF(&header_title), DEMO_FONT_TITLE);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_detail), DEMO_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_hint), DEMO_FONT_CAPTION);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_title), "Virtual List | 1200 items");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_detail), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_hint), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_title), EGUI_THEME_PRIMARY_DARK, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_detail), EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_hint), EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&header_title), 14, 10);
    egui_view_set_size(EGUI_VIEW_OF(&header_title), DEMO_HEADER_W - 28, 12);
    egui_view_set_position(EGUI_VIEW_OF(&header_detail), 14, 26);
    egui_view_set_size(EGUI_VIEW_OF(&header_detail), DEMO_HEADER_W - 28, 10);
    egui_view_set_position(EGUI_VIEW_OF(&header_hint), 14, 38);
    egui_view_set_size(EGUI_VIEW_OF(&header_hint), DEMO_HEADER_W - 28, 10);

    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_title));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_detail));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_hint));

    egui_view_virtual_list_init_with_params(EGUI_VIEW_OF(&viewport_1), &viewport_1_params);
    egui_view_set_background(EGUI_VIEW_OF(&viewport_1), EGUI_BG_OF(&list_bg));
    egui_view_set_shadow(EGUI_VIEW_OF(&viewport_1), &demo_card_shadow);
    egui_view_virtual_list_set_adapter(EGUI_VIEW_OF(&viewport_1), &viewport_adapter, &viewport_context);
    egui_view_virtual_list_set_estimated_item_height(EGUI_VIEW_OF(&viewport_1), 80);
    egui_view_virtual_list_set_overscan(EGUI_VIEW_OF(&viewport_1), 1, 1);
    egui_view_virtual_list_set_keepalive_limit(EGUI_VIEW_OF(&viewport_1), 4);

    demo_update_status_labels();

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewport_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&header_card));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    egui_view_t *view;
    int first_call = (action_index != last_action);

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call && viewport_context.created_count > EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
        {
            report_runtime_failure("virtual list created more views than slot capacity");
        }
        view = find_visible_view_by_item_index(1);
        if (view == NULL)
        {
            report_runtime_failure("initial visible row 1 was not bound");
            EGUI_SIM_SET_WAIT(p_action, 200);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 320);
        return true;
    case 1:
        if (first_call && viewport_context.last_clicked_index != 1)
        {
            report_runtime_failure("initial row click did not update selected index");
        }
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 4 / 5;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 3;
        p_action->steps = 6;
        p_action->interval_ms = 900;
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 1200);
        return true;
    case 3:
        if (first_call && viewport_context.created_count > EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
        {
            report_runtime_failure("virtual list exceeded slot capacity after scroll");
        }
        view = find_first_visible_view_after(6);
        if (view == NULL)
        {
            report_runtime_failure("scrolled row was not visible");
            EGUI_SIM_SET_WAIT(p_action, 200);
            return true;
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, view, 120);
        return true;
    case 4:
        if (first_call)
        {
            if (viewport_context.click_count < 2)
            {
                report_runtime_failure("virtual list click coverage incomplete");
            }
            if (viewport_context.last_clicked_index < 6)
            {
                report_runtime_failure("scrolled click did not land on a new virtual row");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    default:
        return false;
    }
}
#endif
