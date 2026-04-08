#include "egui.h"

#include <stdio.h>
#include <string.h>

#include "uicode.h"

#define STRIP_MAX_ITEMS         420U
#define STRIP_GALLERY_ITEMS     260U
#define STRIP_QUEUE_ITEMS       180U
#define STRIP_TIMELINE_ITEMS    320U
#define STRIP_INVALID_INDEX     0xFFFFFFFFUL
#define STRIP_VIEW_POOL_CAP     EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS
#define STRIP_STATE_CACHE_COUNT 96U

#define STRIP_STATUS_TEXT_LEN 96
#define STRIP_TITLE_TEXT_LEN  20
#define STRIP_META_TEXT_LEN   20
#define STRIP_BADGE_TEXT_LEN  12
#define STRIP_TAG_TEXT_LEN    12

#define STRIP_MARGIN_X           8
#define STRIP_TOP_Y              8
#define STRIP_CONTENT_W          (EGUI_CONFIG_SCEEN_WIDTH - STRIP_MARGIN_X * 2)
#define STRIP_HEADER_H           78
#define STRIP_TOOLBAR_Y          (STRIP_TOP_Y + STRIP_HEADER_H + 6)
#define STRIP_TOOLBAR_H          32
#define STRIP_VIEW_Y             (STRIP_TOOLBAR_Y + STRIP_TOOLBAR_H + 6)
#define STRIP_VIEW_H             (EGUI_CONFIG_SCEEN_HEIGHT - STRIP_VIEW_Y - 8)
#define STRIP_SCENE_BUTTON_GAP   6
#define STRIP_ACTION_BUTTON_GAP  4
#define STRIP_SCENE_BUTTON_W     ((STRIP_CONTENT_W - 24 - STRIP_SCENE_BUTTON_GAP * 2) / 3)
#define STRIP_ACTION_BUTTON_W    ((STRIP_CONTENT_W - 20 - STRIP_ACTION_BUTTON_GAP * 3) / 4)
#define STRIP_BUTTON_H           20
#define STRIP_CARD_SIDE_INSET    4
#define STRIP_KEEP_VISIBLE_INSET 6
#define STRIP_CLICK_VERIFY_RETRY_MAX    3U
#define STRIP_MUTATION_VERIFY_RETRY_MAX 4U
#define STRIP_SCENE_VERIFY_RETRY_MAX    4U

#define STRIP_FONT_HEADER ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define STRIP_FONT_TITLE  ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define STRIP_FONT_BODY   ((const egui_font_t *)&egui_res_font_montserrat_8_4)
#define STRIP_FONT_CAP    ((const egui_font_t *)&egui_res_font_montserrat_8_4)

enum
{
    STRIP_SCENE_GALLERY = 0,
    STRIP_SCENE_QUEUE,
    STRIP_SCENE_TIMELINE,
    STRIP_SCENE_COUNT,
};

enum
{
    STRIP_ACTION_ADD = 0,
    STRIP_ACTION_DEL,
    STRIP_ACTION_PATCH,
    STRIP_ACTION_JUMP,
    STRIP_ACTION_COUNT,
};

enum
{
    STRIP_ITEM_VARIANT_PRIMARY = 0,
    STRIP_ITEM_VARIANT_DETAIL,
    STRIP_ITEM_VARIANT_COMPACT,
    STRIP_ITEM_VARIANT_COUNT,
};

enum
{
    STRIP_ITEM_STATE_IDLE = 0,
    STRIP_ITEM_STATE_LIVE,
    STRIP_ITEM_STATE_WARN,
    STRIP_ITEM_STATE_DONE,
    STRIP_ITEM_STATE_COUNT,
};

typedef struct strip_demo_item strip_demo_item_t;
typedef struct strip_demo_item_state strip_demo_item_state_t;
typedef struct strip_demo_item_view strip_demo_item_view_t;
typedef struct strip_demo_context strip_demo_context_t;

struct strip_demo_item
{
    uint32_t stable_id;
    uint16_t revision;
    uint8_t variant;
    uint8_t state;
    uint8_t progress;
    uint8_t reserved;
};

struct strip_demo_item_state
{
    uint16_t pulse_elapsed_ms;
    uint8_t pulse_running;
    uint8_t pulse_alpha;
    uint8_t pulse_cycle_flip;
    uint8_t pulse_repeated;
};

struct strip_demo_item_view
{
    egui_view_group_t root;
    egui_view_card_t card;
    egui_view_t accent;
    egui_view_label_t tag;
    egui_view_label_t title;
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

struct strip_demo_context
{
    uint8_t scene;
    uint32_t item_count;
    uint32_t next_stable_id;
    uint32_t selected_id;
    uint32_t last_clicked_index;
    uint32_t click_count;
    uint32_t action_count;
    uint32_t mutation_count;
    uint32_t scene_switch_count;
    uint32_t jump_cursor;
    uint8_t created_count;
    strip_demo_item_t items[STRIP_MAX_ITEMS];
    char title_texts[STRIP_VIEW_POOL_CAP][STRIP_TITLE_TEXT_LEN];
    char meta_texts[STRIP_VIEW_POOL_CAP][STRIP_META_TEXT_LEN];
    char badge_texts[STRIP_VIEW_POOL_CAP][STRIP_BADGE_TEXT_LEN];
    char tag_texts[STRIP_VIEW_POOL_CAP][STRIP_TAG_TEXT_LEN];
    char header_title_text[STRIP_STATUS_TEXT_LEN];
    char header_detail_text[STRIP_STATUS_TEXT_LEN];
    char header_hint_text[STRIP_STATUS_TEXT_LEN];
    char last_action_text[STRIP_STATUS_TEXT_LEN];
    strip_demo_item_view_t item_views[STRIP_VIEW_POOL_CAP];
};

static const char *strip_demo_scene_names[STRIP_SCENE_COUNT] = {"Gallery", "Queue", "Timeline"};
static const char *strip_demo_action_names[STRIP_ACTION_COUNT] = {"Add", "Del", "Patch", "Jump"};
static const char *strip_demo_state_names[STRIP_ITEM_STATE_COUNT] = {"Idle", "Live", "Warn", "Done"};
static const char *strip_demo_gallery_titles[STRIP_ITEM_VARIANT_COUNT] = {"Poster", "Focus", "Wide"};
static const char *strip_demo_queue_titles[STRIP_ITEM_VARIANT_COUNT] = {"Deck", "Stem", "Cue"};
static const char *strip_demo_timeline_titles[STRIP_ITEM_VARIANT_COUNT] = {"Mark", "Beat", "Drop"};

static egui_view_t background_view;
static egui_view_card_t header_card;
static egui_view_card_t toolbar_card;
static egui_view_label_t header_title;
static egui_view_label_t header_detail;
static egui_view_label_t header_hint;
static egui_view_button_t scene_buttons[STRIP_SCENE_COUNT];
static egui_view_button_t action_buttons[STRIP_ACTION_COUNT];
static egui_view_virtual_strip_t strip_view;
static egui_view_group_t strip_overlay_view;
static egui_view_card_t strip_overlay_line;
static egui_view_card_t strip_overlay_dot;
static strip_demo_context_t strip_demo_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
static uint8_t recording_click_verify_retry;
static uint8_t recording_mutation_verify_retry;
static uint8_t recording_scene_verify_retry;
static uint8_t recording_swipe_verify_retry;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(strip_header_card_params, STRIP_MARGIN_X, STRIP_TOP_Y, STRIP_CONTENT_W, STRIP_HEADER_H, 14);
EGUI_VIEW_CARD_PARAMS_INIT(strip_toolbar_card_params, STRIP_MARGIN_X, STRIP_TOOLBAR_Y, STRIP_CONTENT_W, STRIP_TOOLBAR_H, 12);
static const egui_view_virtual_strip_params_t strip_view_params = {
        .region = {{STRIP_MARGIN_X, STRIP_VIEW_Y}, {STRIP_CONTENT_W, STRIP_VIEW_H}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 6,
        .estimated_item_width = 88,
};

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(strip_screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xF7F2EA), EGUI_COLOR_HEX(0xDDE9F5),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(strip_screen_bg_params, &strip_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(strip_screen_bg, &strip_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_view_bg_param, EGUI_COLOR_HEX(0xFBFCFD), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(strip_view_bg_params, &strip_view_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_view_bg, &strip_view_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_scene_idle_param, EGUI_COLOR_HEX(0xE8EEF5), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(strip_scene_idle_params, &strip_scene_idle_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_scene_idle_bg, &strip_scene_idle_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_scene_active_param, EGUI_COLOR_HEX(0x335F8A), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(strip_scene_active_params, &strip_scene_active_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_scene_active_bg, &strip_scene_active_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_action_add_param, EGUI_COLOR_HEX(0xDFF3E9), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(strip_action_add_params, &strip_action_add_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_action_add_bg, &strip_action_add_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_action_del_param, EGUI_COLOR_HEX(0xFFF1E1), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(strip_action_del_params, &strip_action_del_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_action_del_bg, &strip_action_del_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_action_patch_param, EGUI_COLOR_HEX(0xE3F6F7), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(strip_action_patch_params, &strip_action_patch_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_action_patch_bg, &strip_action_patch_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_action_jump_param, EGUI_COLOR_HEX(0xE5EEFF), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(strip_action_jump_params, &strip_action_jump_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_action_jump_bg, &strip_action_jump_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_badge_selected_param, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(strip_badge_selected_params, &strip_badge_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_badge_selected_bg, &strip_badge_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_badge_idle_param, EGUI_COLOR_HEX(0x8796A4), EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(strip_badge_idle_params, &strip_badge_idle_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_badge_idle_bg, &strip_badge_idle_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_badge_live_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(strip_badge_live_params, &strip_badge_live_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_badge_live_bg, &strip_badge_live_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_badge_warn_param, EGUI_COLOR_HEX(0xD08A2E), EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(strip_badge_warn_params, &strip_badge_warn_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_badge_warn_bg, &strip_badge_warn_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(strip_badge_done_param, EGUI_COLOR_HEX(0x56789A), EGUI_ALPHA_100, 8);
EGUI_BACKGROUND_PARAM_INIT(strip_badge_done_params, &strip_badge_done_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_badge_done_bg, &strip_badge_done_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(strip_pulse_selected_param, EGUI_COLOR_HEX(0x335F8A), EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_PARAM_INIT(strip_pulse_selected_params, &strip_pulse_selected_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_pulse_selected_bg, &strip_pulse_selected_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(strip_pulse_live_param, EGUI_COLOR_HEX(0x2E9A6F), EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_PARAM_INIT(strip_pulse_live_params, &strip_pulse_live_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_pulse_live_bg, &strip_pulse_live_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(strip_pulse_warn_param, EGUI_COLOR_HEX(0xD08A2E), EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_PARAM_INIT(strip_pulse_warn_params, &strip_pulse_warn_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_pulse_warn_bg, &strip_pulse_warn_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(strip_pulse_done_param, EGUI_COLOR_HEX(0x6887B1), EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_PARAM_INIT(strip_pulse_done_params, &strip_pulse_done_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(strip_pulse_done_bg, &strip_pulse_done_params);

EGUI_SHADOW_PARAM_INIT_ROUND(strip_demo_card_shadow, 10, 0, 3, EGUI_COLOR_BLACK, EGUI_ALPHA_20, 12);
EGUI_ANIMATION_ALPHA_PARAMS_INIT(strip_pulse_anim_param, EGUI_ALPHA_100, EGUI_ALPHA_20);

static void strip_demo_refresh_header(void);
static void strip_demo_style_scene_buttons(void);
static void strip_demo_style_overlay(void);
static void strip_demo_apply_action(uint8_t action);
static void strip_demo_switch_scene(uint8_t scene);

static uint32_t strip_demo_scene_default_count(uint8_t scene)
{
    switch (scene)
    {
    case STRIP_SCENE_QUEUE:
        return STRIP_QUEUE_ITEMS;
    case STRIP_SCENE_TIMELINE:
        return STRIP_TIMELINE_ITEMS;
    default:
        return STRIP_GALLERY_ITEMS;
    }
}

static uint32_t strip_demo_scene_base_id(uint8_t scene)
{
    return (uint32_t)(scene + 6U) * 100000U;
}

static const char *strip_demo_variant_title(uint8_t scene, uint8_t variant)
{
    variant = (uint8_t)(variant % STRIP_ITEM_VARIANT_COUNT);

    switch (scene)
    {
    case STRIP_SCENE_QUEUE:
        return strip_demo_queue_titles[variant];
    case STRIP_SCENE_TIMELINE:
        return strip_demo_timeline_titles[variant];
    default:
        return strip_demo_gallery_titles[variant];
    }
}

static const char *strip_demo_variant_code(uint8_t scene, uint8_t variant)
{
    static const char *gallery_codes[STRIP_ITEM_VARIANT_COUNT] = {"P", "F", "W"};
    static const char *queue_codes[STRIP_ITEM_VARIANT_COUNT] = {"D", "S", "C"};
    static const char *timeline_codes[STRIP_ITEM_VARIANT_COUNT] = {"M", "B", "D"};

    variant = (uint8_t)(variant % STRIP_ITEM_VARIANT_COUNT);

    switch (scene)
    {
    case STRIP_SCENE_QUEUE:
        return queue_codes[variant];
    case STRIP_SCENE_TIMELINE:
        return timeline_codes[variant];
    default:
        return gallery_codes[variant];
    }
}

static const char *strip_demo_state_short_name(uint8_t state)
{
    static const char *state_short_names[STRIP_ITEM_STATE_COUNT] = {"I", "L", "W", "D"};

    return state_short_names[state % STRIP_ITEM_STATE_COUNT];
}

static void strip_demo_copy_best_fit(egui_view_t *label, char *dst, size_t dst_size, egui_dim_t max_width, const char *primary, const char *secondary,
                                     const char *fallback)
{
    const char *candidates[3] = {primary, secondary, fallback};
    const char *selected = "";
    size_t i;

    if (dst_size == 0U)
    {
        return;
    }

    for (i = 0; i < 3U; i++)
    {
        egui_dim_t text_w = 0;
        egui_dim_t text_h = 0;
        const char *candidate = candidates[i];

        if (candidate == NULL || candidate[0] == '\0')
        {
            continue;
        }
        selected = candidate;
        if (max_width <= 0)
        {
            break;
        }
        if (label != NULL && !egui_view_label_get_str_size(label, candidate, &text_w, &text_h) && text_w <= max_width)
        {
            break;
        }
    }

    snprintf(dst, dst_size, "%s", selected);
}

static int32_t strip_demo_find_index_by_stable_id(uint32_t stable_id)
{
    uint32_t i;

    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return -1;
    }

    for (i = 0; i < strip_demo_ctx.item_count; i++)
    {
        if (strip_demo_ctx.items[i].stable_id == stable_id)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static void strip_demo_fill_item(strip_demo_item_t *item, uint8_t scene, uint32_t stable_id, uint32_t ordinal)
{
    uint8_t progress;

    memset(item, 0, sizeof(*item));
    item->stable_id = stable_id;
    item->revision = (uint16_t)(1U + ((ordinal * 3U + scene * 5U) % 7U));
    item->variant = (uint8_t)((ordinal + scene) % STRIP_ITEM_VARIANT_COUNT);
    item->state = (uint8_t)(((ordinal * 5U) + scene) % STRIP_ITEM_STATE_COUNT);
    progress = (uint8_t)(18U + ((ordinal * 11U + scene * 9U) % 73U));

    if (scene == STRIP_SCENE_GALLERY && item->variant == STRIP_ITEM_VARIANT_PRIMARY && progress < 46U)
    {
        progress = (uint8_t)(progress + 18U);
    }
    if (scene == STRIP_SCENE_QUEUE && item->state == STRIP_ITEM_STATE_WARN && progress < 62U)
    {
        progress = (uint8_t)(progress + 14U);
    }
    if (scene == STRIP_SCENE_TIMELINE && item->variant == STRIP_ITEM_VARIANT_COMPACT)
    {
        item->state = (uint8_t)(item->state == STRIP_ITEM_STATE_DONE ? STRIP_ITEM_STATE_DONE : STRIP_ITEM_STATE_IDLE);
    }
    if (item->state == STRIP_ITEM_STATE_DONE)
    {
        progress = 100U;
    }

    item->progress = progress;
}

static void strip_demo_reset_scene_model(uint8_t scene)
{
    uint32_t i;
    uint32_t count = strip_demo_scene_default_count(scene);

    strip_demo_ctx.scene = scene;
    strip_demo_ctx.item_count = count;
    strip_demo_ctx.next_stable_id = strip_demo_scene_base_id(scene) + 1U;
    strip_demo_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    strip_demo_ctx.last_clicked_index = STRIP_INVALID_INDEX;
    strip_demo_ctx.jump_cursor = 0;

    for (i = 0; i < count; i++)
    {
        strip_demo_fill_item(&strip_demo_ctx.items[i], scene, strip_demo_ctx.next_stable_id++, i);
    }

    snprintf(strip_demo_ctx.last_action_text, sizeof(strip_demo_ctx.last_action_text), "Tap cards, swipe horizontally, then try Add / Del / Patch / Jump.");
}

static int32_t strip_demo_get_view_height_hint(void)
{
    int32_t height = EGUI_VIEW_OF(&strip_view)->region.size.height;

    return height > 0 ? height : STRIP_VIEW_H;
}

static int32_t strip_demo_measure_item_width_with_state(const strip_demo_item_t *item, uint8_t selected, int32_t height_hint)
{
    int32_t width;

    if (item == NULL)
    {
        return 84;
    }

    switch (strip_demo_ctx.scene)
    {
    case STRIP_SCENE_QUEUE:
        width = item->variant == STRIP_ITEM_VARIANT_PRIMARY ? 112 : item->variant == STRIP_ITEM_VARIANT_DETAIL ? 92 : 126;
        if (height_hint > 150)
        {
            width += 6;
        }
        break;
    case STRIP_SCENE_TIMELINE:
        width = item->variant == STRIP_ITEM_VARIANT_PRIMARY ? 46 : item->variant == STRIP_ITEM_VARIANT_DETAIL ? 58 : 40;
        if (height_hint > 150)
        {
            width += 2;
        }
        break;
    default:
        width = item->variant == STRIP_ITEM_VARIANT_PRIMARY ? 82 : item->variant == STRIP_ITEM_VARIANT_DETAIL ? 72 : 96;
        if (height_hint > 150)
        {
            width += 4;
        }
        break;
    }

    if (item->state == STRIP_ITEM_STATE_LIVE)
    {
        width += 4;
    }
    else if (item->state == STRIP_ITEM_STATE_WARN)
    {
        width += 8;
    }
    if (selected)
    {
        width += strip_demo_ctx.scene == STRIP_SCENE_TIMELINE ? 12 : 12;
    }

    return width;
}

static uint8_t strip_demo_item_has_pulse(const strip_demo_item_t *item, uint8_t selected)
{
    if (item == NULL)
    {
        return 0;
    }

    return (uint8_t)(selected || item->state == STRIP_ITEM_STATE_LIVE || item->state == STRIP_ITEM_STATE_WARN);
}

static int strip_demo_get_view_pool_index(strip_demo_item_view_t *item_view)
{
    uint8_t i;

    for (i = 0; i < strip_demo_ctx.created_count; i++)
    {
        if (item_view == &strip_demo_ctx.item_views[i])
        {
            return (int)i;
        }
    }

    return -1;
}

static strip_demo_item_view_t *strip_demo_find_view_by_root(egui_view_t *view)
{
    uint8_t i;

    for (i = 0; i < strip_demo_ctx.created_count; i++)
    {
        if (view == EGUI_VIEW_OF(&strip_demo_ctx.item_views[i].root))
        {
            return &strip_demo_ctx.item_views[i];
        }
    }

    return NULL;
}

static egui_background_t *strip_demo_get_badge_background(uint8_t state, uint8_t selected)
{
    if (selected)
    {
        return EGUI_BG_OF(&strip_badge_selected_bg);
    }

    switch (state)
    {
    case STRIP_ITEM_STATE_LIVE:
        return EGUI_BG_OF(&strip_badge_live_bg);
    case STRIP_ITEM_STATE_WARN:
        return EGUI_BG_OF(&strip_badge_warn_bg);
    case STRIP_ITEM_STATE_DONE:
        return EGUI_BG_OF(&strip_badge_done_bg);
    default:
        return EGUI_BG_OF(&strip_badge_idle_bg);
    }
}

static egui_background_t *strip_demo_get_pulse_background(uint8_t state, uint8_t selected)
{
    if (selected)
    {
        return EGUI_BG_OF(&strip_pulse_selected_bg);
    }

    switch (state)
    {
    case STRIP_ITEM_STATE_LIVE:
        return EGUI_BG_OF(&strip_pulse_live_bg);
    case STRIP_ITEM_STATE_WARN:
        return EGUI_BG_OF(&strip_pulse_warn_bg);
    case STRIP_ITEM_STATE_DONE:
        return EGUI_BG_OF(&strip_pulse_done_bg);
    default:
        return EGUI_BG_OF(&strip_pulse_live_bg);
    }
}

static egui_color_t strip_demo_get_card_color(uint8_t scene, uint8_t variant, uint8_t selected)
{
    if (selected)
    {
        return scene == STRIP_SCENE_TIMELINE ? EGUI_COLOR_HEX(0xEDF3F8) : EGUI_COLOR_HEX(0xE4F1FF);
    }

    switch (scene)
    {
    case STRIP_SCENE_QUEUE:
        return variant == STRIP_ITEM_VARIANT_PRIMARY  ? EGUI_COLOR_HEX(0xF6F8FA)
               : variant == STRIP_ITEM_VARIANT_DETAIL ? EGUI_COLOR_HEX(0xFBF7FF)
                                                      : EGUI_COLOR_HEX(0xF5FBF7);
    case STRIP_SCENE_TIMELINE:
        return variant == STRIP_ITEM_VARIANT_PRIMARY  ? EGUI_COLOR_HEX(0xF8FAFC)
               : variant == STRIP_ITEM_VARIANT_DETAIL ? EGUI_COLOR_HEX(0xF3F9FB)
                                                      : EGUI_COLOR_HEX(0xFFF9F1);
    default:
        return variant == STRIP_ITEM_VARIANT_PRIMARY  ? EGUI_COLOR_HEX(0xFFF8EC)
               : variant == STRIP_ITEM_VARIANT_DETAIL ? EGUI_COLOR_HEX(0xF2F6FF)
                                                      : EGUI_COLOR_HEX(0xEEF9F1);
    }
}

static egui_color_t strip_demo_get_border_color(uint8_t state, uint8_t selected)
{
    if (selected)
    {
        return EGUI_COLOR_HEX(0x335F8A);
    }

    switch (state)
    {
    case STRIP_ITEM_STATE_LIVE:
        return EGUI_COLOR_HEX(0x4C9F7E);
    case STRIP_ITEM_STATE_WARN:
        return EGUI_COLOR_HEX(0xC98C36);
    case STRIP_ITEM_STATE_DONE:
        return EGUI_COLOR_HEX(0x6A85A5);
    default:
        return EGUI_COLOR_HEX(0xCED8E1);
    }
}

static void strip_demo_set_item_pulse(strip_demo_item_view_t *item_view, const strip_demo_item_t *item, uint8_t visible, uint8_t selected)
{
    if (!visible || item == NULL)
    {
        if (item_view->pulse_running)
        {
            egui_animation_stop(EGUI_ANIM_OF(&item_view->pulse_anim));
            item_view->pulse_running = 0;
        }
        egui_view_set_gone(EGUI_VIEW_OF(&item_view->pulse), 1);
        egui_view_set_alpha(EGUI_VIEW_OF(&item_view->pulse), EGUI_ALPHA_100);
        return;
    }

    egui_view_set_gone(EGUI_VIEW_OF(&item_view->pulse), 0);
    egui_view_set_background(EGUI_VIEW_OF(&item_view->pulse), strip_demo_get_pulse_background(item->state, selected));
    if (!item_view->pulse_running)
    {
        egui_animation_start(EGUI_ANIM_OF(&item_view->pulse_anim));
        item_view->pulse_running = 1;
    }
}

static void strip_demo_capture_view_state(strip_demo_item_view_t *item_view, strip_demo_item_state_t *state)
{
    egui_animation_t *anim = EGUI_ANIM_OF(&item_view->pulse_anim);

    memset(state, 0, sizeof(*state));
    state->pulse_running = item_view->pulse_running ? 1U : 0U;
    state->pulse_alpha = EGUI_VIEW_OF(&item_view->pulse)->alpha;

    if (!item_view->pulse_running)
    {
        return;
    }

    state->pulse_cycle_flip = anim->is_cycle_flip ? 1U : 0U;
    state->pulse_repeated = (uint8_t)anim->repeated;

    if (anim->start_time != (uint32_t)-1 && anim->duration > 0)
    {
        uint32_t elapsed_ms = egui_api_timer_get_current() - anim->start_time;

        if (elapsed_ms >= anim->duration)
        {
            elapsed_ms = anim->duration > 1U ? (uint32_t)anim->duration - 1U : 0U;
        }

        state->pulse_elapsed_ms = (uint16_t)elapsed_ms;
    }
}

static void strip_demo_restore_view_state(strip_demo_item_view_t *item_view, const strip_demo_item_state_t *state)
{
    egui_animation_t *anim;

    if (state == NULL || !state->pulse_running)
    {
        return;
    }

    anim = EGUI_ANIM_OF(&item_view->pulse_anim);
    if (!anim->is_running)
    {
        egui_animation_start(anim);
    }

    item_view->pulse_running = 1;
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->pulse), 0);
    egui_view_set_alpha(EGUI_VIEW_OF(&item_view->pulse), state->pulse_alpha);
    anim->is_started = 1;
    anim->is_ended = 0;
    anim->is_cycle_flip = state->pulse_cycle_flip ? 1U : 0U;
    anim->repeated = (int8_t)state->pulse_repeated;
    anim->start_time = egui_api_timer_get_current() - state->pulse_elapsed_ms;
}

static void strip_demo_fill_item_texts(strip_demo_item_view_t *item_view, int pool_index, const strip_demo_item_t *item, uint32_t index, uint8_t selected,
                                       egui_dim_t tag_w, egui_dim_t title_w, egui_dim_t meta_w, egui_dim_t badge_w)
{
    char full_text[STRIP_STATUS_TEXT_LEN];
    char medium_text[STRIP_STATUS_TEXT_LEN];
    char short_text[STRIP_STATUS_TEXT_LEN];
    uint32_t short_index = (index + 1U) % 100U;
    uint8_t state = item != NULL ? item->state : STRIP_ITEM_STATE_IDLE;

    if (pool_index < 0 || pool_index >= STRIP_VIEW_POOL_CAP || item == NULL || item_view == NULL)
    {
        return;
    }

    switch (strip_demo_ctx.scene)
    {
    case STRIP_SCENE_QUEUE:
        snprintf(full_text, sizeof(full_text), "Q%02lu", (unsigned long)short_index);
        snprintf(medium_text, sizeof(medium_text), "Q%lu", (unsigned long)short_index);
        snprintf(short_text, sizeof(short_text), "%lu", (unsigned long)short_index);
        strip_demo_copy_best_fit(EGUI_VIEW_OF(&item_view->tag), strip_demo_ctx.tag_texts[pool_index], sizeof(strip_demo_ctx.tag_texts[pool_index]), tag_w,
                                 full_text, medium_text, short_text);

        snprintf(full_text, sizeof(full_text), "%s %02lu", strip_demo_variant_title(strip_demo_ctx.scene, item->variant), (unsigned long)short_index);
        snprintf(medium_text, sizeof(medium_text), "%s", strip_demo_variant_title(strip_demo_ctx.scene, item->variant));
        snprintf(short_text, sizeof(short_text), "%s%02lu", strip_demo_variant_code(strip_demo_ctx.scene, item->variant), (unsigned long)short_index);
        strip_demo_copy_best_fit(EGUI_VIEW_OF(&item_view->title), strip_demo_ctx.title_texts[pool_index], sizeof(strip_demo_ctx.title_texts[pool_index]),
                                 title_w, full_text, medium_text, short_text);

        snprintf(full_text, sizeof(full_text), "%02ums r%u", (unsigned)(48U + item->progress * 2U), (unsigned)item->revision);
        snprintf(medium_text, sizeof(medium_text), "%02ums", (unsigned)(48U + item->progress * 2U));
        snprintf(short_text, sizeof(short_text), "r%u", (unsigned)item->revision);
        break;
    case STRIP_SCENE_TIMELINE:
        snprintf(full_text, sizeof(full_text), "%02u:%02u", (unsigned)((index / 12U) % 24U), (unsigned)((index * 5U) % 60U));
        snprintf(medium_text, sizeof(medium_text), "%u:%02u", (unsigned)((index / 12U) % 24U), (unsigned)((index * 5U) % 60U));
        snprintf(short_text, sizeof(short_text), "%02lu", (unsigned long)short_index);
        strip_demo_copy_best_fit(EGUI_VIEW_OF(&item_view->tag), strip_demo_ctx.tag_texts[pool_index], sizeof(strip_demo_ctx.tag_texts[pool_index]), tag_w,
                                 full_text, medium_text, short_text);

        snprintf(full_text, sizeof(full_text), "%s %02lu", strip_demo_variant_title(strip_demo_ctx.scene, item->variant), (unsigned long)short_index);
        snprintf(medium_text, sizeof(medium_text), "%s", strip_demo_variant_title(strip_demo_ctx.scene, item->variant));
        snprintf(short_text, sizeof(short_text), "%s%02lu", strip_demo_variant_code(strip_demo_ctx.scene, item->variant), (unsigned long)short_index);
        strip_demo_copy_best_fit(EGUI_VIEW_OF(&item_view->title), strip_demo_ctx.title_texts[pool_index], sizeof(strip_demo_ctx.title_texts[pool_index]),
                                 title_w, full_text, medium_text, short_text);

        snprintf(full_text, sizeof(full_text), "%s", strip_demo_state_names[state]);
        snprintf(medium_text, sizeof(medium_text), "%s", strip_demo_state_names[state]);
        snprintf(short_text, sizeof(short_text), "%s", strip_demo_state_short_name(state));
        break;
    default:
        snprintf(full_text, sizeof(full_text), "EP%02lu", (unsigned long)short_index);
        snprintf(medium_text, sizeof(medium_text), "E%02lu", (unsigned long)short_index);
        snprintf(short_text, sizeof(short_text), "%02lu", (unsigned long)short_index);
        strip_demo_copy_best_fit(EGUI_VIEW_OF(&item_view->tag), strip_demo_ctx.tag_texts[pool_index], sizeof(strip_demo_ctx.tag_texts[pool_index]), tag_w,
                                 full_text, medium_text, short_text);

        snprintf(full_text, sizeof(full_text), "%s %02lu", strip_demo_variant_title(strip_demo_ctx.scene, item->variant), (unsigned long)short_index);
        snprintf(medium_text, sizeof(medium_text), "%s", strip_demo_variant_title(strip_demo_ctx.scene, item->variant));
        snprintf(short_text, sizeof(short_text), "%s%02lu", strip_demo_variant_code(strip_demo_ctx.scene, item->variant), (unsigned long)short_index);
        strip_demo_copy_best_fit(EGUI_VIEW_OF(&item_view->title), strip_demo_ctx.title_texts[pool_index], sizeof(strip_demo_ctx.title_texts[pool_index]),
                                 title_w, full_text, medium_text, short_text);

        snprintf(full_text, sizeof(full_text), "%u%% focus", (unsigned)item->progress);
        snprintf(medium_text, sizeof(medium_text), "%u%%", (unsigned)item->progress);
        snprintf(short_text, sizeof(short_text), "%u", (unsigned)item->progress);
        break;
    }

    strip_demo_copy_best_fit(EGUI_VIEW_OF(&item_view->meta), strip_demo_ctx.meta_texts[pool_index], sizeof(strip_demo_ctx.meta_texts[pool_index]), meta_w,
                             full_text, medium_text, short_text);

    snprintf(full_text, sizeof(full_text), "%s", selected ? "Sel" : strip_demo_state_names[state]);
    snprintf(medium_text, sizeof(medium_text), "%s", selected ? "Sel" : strip_demo_state_short_name(state));
    strip_demo_copy_best_fit(EGUI_VIEW_OF(&item_view->badge), strip_demo_ctx.badge_texts[pool_index], sizeof(strip_demo_ctx.badge_texts[pool_index]),
                             badge_w - 6, full_text, medium_text, medium_text);
}

static void strip_demo_layout_item_view(strip_demo_item_view_t *item_view, const strip_demo_item_t *item, int pool_index, uint32_t index, uint8_t selected)
{
    egui_dim_t width = EGUI_VIEW_OF(&item_view->root)->region.size.width > 0 ? EGUI_VIEW_OF(&item_view->root)->region.size.width : 84;
    egui_dim_t height = EGUI_VIEW_OF(&item_view->root)->region.size.height > 0 ? EGUI_VIEW_OF(&item_view->root)->region.size.height : STRIP_VIEW_H;
    egui_dim_t card_w;
    egui_dim_t card_h;
    egui_dim_t card_x = STRIP_CARD_SIDE_INSET;
    egui_dim_t card_y;
    egui_dim_t inset;
    egui_dim_t badge_w;
    egui_dim_t badge_h = 16;
    egui_dim_t progress_h = 4;
    egui_dim_t pulse_size = 6;
    egui_dim_t tag_x;
    egui_dim_t tag_w;
    egui_dim_t title_x;
    egui_dim_t title_w;
    egui_dim_t meta_x;
    egui_dim_t meta_w;
    egui_dim_t text_limit_x;
    egui_dim_t title_y;
    egui_dim_t meta_y;
    egui_dim_t title_h;
    egui_dim_t meta_h;
    egui_dim_t tag_h;
    egui_dim_t content_w;
    uint8_t show_meta;
    uint8_t show_badge;
    uint8_t show_progress;
    egui_color_t card_color;
    egui_color_t border_color;
    egui_color_t title_color = EGUI_COLOR_HEX(0x243646);
    egui_color_t meta_color = EGUI_COLOR_HEX(0x5B6C7A);
    egui_color_t badge_text_color = selected ? EGUI_COLOR_HEX(0x335F8A) : EGUI_COLOR_WHITE;

    card_w = width - STRIP_CARD_SIDE_INSET * 2;
    if (card_w < 40)
    {
        card_w = 40;
    }

    switch (strip_demo_ctx.scene)
    {
    case STRIP_SCENE_QUEUE:
        card_h = (egui_dim_t)(item->variant == STRIP_ITEM_VARIANT_COMPACT ? 112 : item->variant == STRIP_ITEM_VARIANT_DETAIL ? 88 : 98);
        if (selected)
        {
            card_h += 6;
        }
        inset = card_w < 84 ? 6 : 9;
        break;
    case STRIP_SCENE_TIMELINE:
        card_h = (egui_dim_t)(item->variant == STRIP_ITEM_VARIANT_COMPACT ? 74 : item->variant == STRIP_ITEM_VARIANT_DETAIL ? 86 : 80);
        if (selected)
        {
            card_h += 8;
        }
        inset = card_w < 56 ? 4 : 5;
        break;
    default:
        card_h = (egui_dim_t)(item->variant == STRIP_ITEM_VARIANT_COMPACT ? 164 : item->variant == STRIP_ITEM_VARIANT_DETAIL ? 148 : 156);
        if (selected)
        {
            card_h += 10;
        }
        inset = card_w < 72 ? 6 : 8;
        break;
    }

    if (card_h > (height - 8))
    {
        card_h = height - 8;
    }

    card_y = (egui_dim_t)((height - card_h) / 2);
    badge_w = selected ? 30 : 34;
    if (strip_demo_ctx.scene == STRIP_SCENE_TIMELINE)
    {
        show_badge = 0;
        show_meta = (uint8_t)(selected || card_w >= 54);
    }
    else if (strip_demo_ctx.scene == STRIP_SCENE_QUEUE)
    {
        show_badge = (uint8_t)(selected || card_w >= 108);
        show_meta = (uint8_t)(selected || card_w >= 88);
    }
    else
    {
        show_badge = (uint8_t)(selected || card_w >= 92);
        show_meta = (uint8_t)(selected || card_w >= 80);
    }
    show_progress = strip_demo_ctx.scene != STRIP_SCENE_TIMELINE;
    content_w = card_w - inset * 2;
    if (show_badge && content_w > (badge_w + 6))
    {
        content_w -= badge_w + 4;
    }
    if (content_w < 24)
    {
        content_w = 24;
    }

    if (strip_demo_ctx.scene == STRIP_SCENE_TIMELINE)
    {
        title_h = 11;
        meta_h = 10;
        tag_h = 10;
        title_y = 28;
        meta_y = (egui_dim_t)(card_h - 16);
    }
    else if (strip_demo_ctx.scene == STRIP_SCENE_QUEUE)
    {
        title_h = 14;
        meta_h = 12;
        tag_h = 12;
        title_y = 30;
        meta_y = 48;
    }
    else
    {
        title_h = 14;
        meta_h = 12;
        tag_h = 12;
        title_y = (egui_dim_t)(card_h - 44);
        meta_y = (egui_dim_t)(card_h - 26);
    }

    card_color = strip_demo_get_card_color(strip_demo_ctx.scene, item->variant, selected);
    border_color = strip_demo_get_border_color(item->state, selected);
    if (strip_demo_ctx.scene == STRIP_SCENE_TIMELINE)
    {
        title_color = EGUI_COLOR_HEX(0x314554);
        meta_color = EGUI_COLOR_HEX(0x627482);
    }

    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->tag), STRIP_FONT_CAP);
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->title), strip_demo_ctx.scene == STRIP_SCENE_TIMELINE || card_w < 86 ? STRIP_FONT_BODY : STRIP_FONT_TITLE);
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->meta), STRIP_FONT_CAP);
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->badge), STRIP_FONT_CAP);

    egui_view_set_position(EGUI_VIEW_OF(&item_view->card), card_x, card_y);
    egui_view_set_size(EGUI_VIEW_OF(&item_view->card), card_w, card_h);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&item_view->card), card_color, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&item_view->card), selected ? 2 : 1, border_color);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&item_view->card),
                                     strip_demo_ctx.scene == STRIP_SCENE_GALLERY ? 16 : (strip_demo_ctx.scene == STRIP_SCENE_QUEUE ? 10 : 18));
    egui_view_set_shadow(EGUI_VIEW_OF(&item_view->card), strip_demo_ctx.scene == STRIP_SCENE_TIMELINE ? NULL : &strip_demo_card_shadow);

    if (strip_demo_ctx.scene == STRIP_SCENE_QUEUE)
    {
        egui_view_set_position(EGUI_VIEW_OF(&item_view->accent), 8, 10);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->accent), 6, card_h - 20);
    }
    else if (strip_demo_ctx.scene == STRIP_SCENE_TIMELINE)
    {
        egui_view_set_position(EGUI_VIEW_OF(&item_view->accent), card_w / 2 - 2, 10);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->accent), 5, card_h - 20);
    }
    else
    {
        egui_view_set_position(EGUI_VIEW_OF(&item_view->accent), inset, 8);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->accent), card_w - inset * 2, card_h - 64);
    }
    egui_view_set_background(EGUI_VIEW_OF(&item_view->accent), strip_demo_get_badge_background(item->state, selected));

    if (strip_demo_ctx.scene == STRIP_SCENE_TIMELINE)
    {
        egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->tag), show_badge ? (EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER) : EGUI_ALIGN_CENTER);
        egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->title), EGUI_ALIGN_CENTER);
        egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->meta), EGUI_ALIGN_CENTER);
        tag_x = show_badge ? 4 : 0;
        tag_w = show_badge ? (egui_dim_t)(card_w - badge_w - 8) : card_w;
        egui_view_set_position(EGUI_VIEW_OF(&item_view->tag), tag_x, 8);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->tag), tag_w, tag_h);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->tag), EGUI_COLOR_HEX(0x6C7D89), EGUI_ALPHA_100);
    }
    else
    {
        egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->tag), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
        egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
        egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->meta), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
        tag_x = strip_demo_ctx.scene == STRIP_SCENE_GALLERY ? (egui_dim_t)(inset + 6) : (egui_dim_t)(inset + 12);
        tag_w = card_w - tag_x - inset;
        egui_view_set_position(EGUI_VIEW_OF(&item_view->tag), tag_x, 12);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->tag), tag_w, tag_h);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->tag), strip_demo_ctx.scene == STRIP_SCENE_GALLERY ? EGUI_COLOR_WHITE : EGUI_COLOR_HEX(0x5D7282),
                                       EGUI_ALPHA_100);
    }
    text_limit_x = show_badge ? (egui_dim_t)(card_w - inset - badge_w - 6) : (egui_dim_t)(card_w - inset - 4);

    if (strip_demo_ctx.scene == STRIP_SCENE_QUEUE)
    {
        title_x = inset + 12;
        title_w = text_limit_x - title_x;
        if (title_w < 12)
        {
            title_w = 12;
        }
        egui_view_set_position(EGUI_VIEW_OF(&item_view->title), title_x, title_y);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->title), title_w, title_h);
    }
    else if (strip_demo_ctx.scene == STRIP_SCENE_TIMELINE)
    {
        title_x = 2;
        title_w = card_w - 4;
        egui_view_set_position(EGUI_VIEW_OF(&item_view->title), title_x, title_y);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->title), title_w, title_h);
    }
    else
    {
        title_x = inset + 2;
        title_w = text_limit_x - title_x;
        if (title_w < 12)
        {
            title_w = 12;
        }
        egui_view_set_position(EGUI_VIEW_OF(&item_view->title), title_x, title_y);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->title), title_w, title_h);
    }
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->title), title_color, EGUI_ALPHA_100);

    if (strip_demo_ctx.scene == STRIP_SCENE_QUEUE)
    {
        meta_x = inset + 12;
        meta_w = text_limit_x - meta_x;
        if (meta_w < 12)
        {
            meta_w = 12;
        }
        egui_view_set_position(EGUI_VIEW_OF(&item_view->meta), meta_x, meta_y);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->meta), meta_w, meta_h);
    }
    else if (strip_demo_ctx.scene == STRIP_SCENE_TIMELINE)
    {
        meta_x = 0;
        meta_w = card_w;
        egui_view_set_position(EGUI_VIEW_OF(&item_view->meta), meta_x, meta_y);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->meta), meta_w, meta_h);
    }
    else
    {
        meta_x = inset + 2;
        meta_w = text_limit_x - meta_x;
        if (meta_w < 12)
        {
            meta_w = 12;
        }
        egui_view_set_position(EGUI_VIEW_OF(&item_view->meta), meta_x, meta_y);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->meta), meta_w, meta_h);
    }
    strip_demo_fill_item_texts(item_view, pool_index, item, index, selected, tag_w, title_w, meta_w, badge_w);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->tag), strip_demo_ctx.tag_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->title), strip_demo_ctx.title_texts[pool_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&item_view->meta), strip_demo_ctx.meta_texts[pool_index]);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->meta), meta_color, EGUI_ALPHA_100);
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->meta), show_meta ? 0 : 1);

    if (show_badge)
    {
        egui_view_set_position(EGUI_VIEW_OF(&item_view->badge), card_w - inset - badge_w, strip_demo_ctx.scene == STRIP_SCENE_TIMELINE ? 10 : 12);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->badge), badge_w, badge_h);
        egui_view_set_background(EGUI_VIEW_OF(&item_view->badge), strip_demo_get_badge_background(item->state, selected));
        egui_view_label_set_text(EGUI_VIEW_OF(&item_view->badge), strip_demo_ctx.badge_texts[pool_index]);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&item_view->badge), badge_text_color, EGUI_ALPHA_100);
        egui_view_set_gone(EGUI_VIEW_OF(&item_view->badge), 0);
    }
    else
    {
        egui_view_set_gone(EGUI_VIEW_OF(&item_view->badge), 1);
    }

    if (show_progress)
    {
        egui_dim_t progress_y = strip_demo_ctx.scene == STRIP_SCENE_QUEUE ? (egui_dim_t)(card_h - 14) : (egui_dim_t)(card_h - 10);

        egui_view_set_position(EGUI_VIEW_OF(&item_view->progress), strip_demo_ctx.scene == STRIP_SCENE_QUEUE ? (egui_dim_t)(inset + 12) : inset, progress_y);
        egui_view_set_size(EGUI_VIEW_OF(&item_view->progress),
                           strip_demo_ctx.scene == STRIP_SCENE_QUEUE ? (egui_dim_t)(card_w - inset * 2 - 12) : (egui_dim_t)(card_w - inset * 2), progress_h);
        egui_view_progress_bar_set_process(EGUI_VIEW_OF(&item_view->progress), item->progress);
        egui_view_set_gone(EGUI_VIEW_OF(&item_view->progress), 0);
    }
    else
    {
        egui_view_set_gone(EGUI_VIEW_OF(&item_view->progress), 1);
    }

    egui_view_set_position(EGUI_VIEW_OF(&item_view->pulse),
                           strip_demo_ctx.scene == STRIP_SCENE_TIMELINE ? (egui_dim_t)(card_w / 2 - pulse_size / 2) : (egui_dim_t)(card_w - inset - pulse_size),
                           strip_demo_ctx.scene == STRIP_SCENE_TIMELINE ? (egui_dim_t)(card_h / 2 - pulse_size / 2)
                                                                        : (egui_dim_t)(card_h - inset - pulse_size));
    egui_view_set_size(EGUI_VIEW_OF(&item_view->pulse), pulse_size, pulse_size);
    if (strip_demo_item_has_pulse(item, selected))
    {
        strip_demo_set_item_pulse(item_view, item, 1, selected);
    }
    else
    {
        strip_demo_set_item_pulse(item_view, NULL, 0, 0);
    }
}

static void strip_demo_refresh_header(void)
{
    int32_t selected_index = strip_demo_find_index_by_stable_id(strip_demo_ctx.selected_id);
    int32_t item_x = -1;
    int32_t item_w = -1;

    snprintf(strip_demo_ctx.header_title_text, sizeof(strip_demo_ctx.header_title_text), "Virtual Strip Demo");
    if (selected_index >= 0)
    {
        item_x = egui_view_virtual_strip_get_item_x_by_stable_id(EGUI_VIEW_OF(&strip_view), strip_demo_ctx.selected_id);
        item_w = egui_view_virtual_strip_get_item_width_by_stable_id(EGUI_VIEW_OF(&strip_view), strip_demo_ctx.selected_id);
        snprintf(strip_demo_ctx.header_detail_text, sizeof(strip_demo_ctx.header_detail_text), "%s %lu | #%05lu | x%d w%d",
                 strip_demo_scene_names[strip_demo_ctx.scene], (unsigned long)strip_demo_ctx.item_count, (unsigned long)strip_demo_ctx.selected_id, (int)item_x,
                 (int)item_w);
    }
    else
    {
        snprintf(strip_demo_ctx.header_detail_text, sizeof(strip_demo_ctx.header_detail_text), "%s %lu | sel none | click %lu",
                 strip_demo_scene_names[strip_demo_ctx.scene], (unsigned long)strip_demo_ctx.item_count, (unsigned long)strip_demo_ctx.click_count);
    }
    snprintf(strip_demo_ctx.header_hint_text, sizeof(strip_demo_ctx.header_hint_text), "%s", strip_demo_ctx.last_action_text);

    if (EGUI_VIEW_OF(&header_title)->api == NULL)
    {
        return;
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&header_title), strip_demo_ctx.header_title_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_detail), strip_demo_ctx.header_detail_text);
    egui_view_label_set_text(EGUI_VIEW_OF(&header_hint), strip_demo_ctx.header_hint_text);
}

static void strip_demo_style_overlay(void)
{
    uint8_t show_timeline = strip_demo_ctx.scene == STRIP_SCENE_TIMELINE ? 1U : 0U;
    egui_dim_t center_x = STRIP_CONTENT_W / 2;
    egui_dim_t dot_y = (egui_dim_t)(STRIP_VIEW_H - 42);

    egui_view_set_gone(EGUI_VIEW_OF(&strip_overlay_view), show_timeline ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&strip_overlay_line), show_timeline ? 0 : 1);
    egui_view_set_gone(EGUI_VIEW_OF(&strip_overlay_dot), show_timeline ? 0 : 1);
    if (!show_timeline)
    {
        return;
    }

    egui_view_set_position(EGUI_VIEW_OF(&strip_overlay_line), center_x - 1, 18);
    egui_view_set_size(EGUI_VIEW_OF(&strip_overlay_line), 2, STRIP_VIEW_H - 36);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&strip_overlay_line), EGUI_COLOR_HEX(0x7CA0C5), EGUI_ALPHA_30);
    egui_view_card_set_border(EGUI_VIEW_OF(&strip_overlay_line), 0, EGUI_COLOR_HEX(0x7CA0C5));
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&strip_overlay_line), 1);

    egui_view_set_position(EGUI_VIEW_OF(&strip_overlay_dot), center_x - 3, dot_y);
    egui_view_set_size(EGUI_VIEW_OF(&strip_overlay_dot), 6, 6);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&strip_overlay_dot), EGUI_COLOR_HEX(0x3E74A8), EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&strip_overlay_dot), 0, EGUI_COLOR_HEX(0x3E74A8));
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&strip_overlay_dot), 3);
}

static void strip_demo_update_selection(uint32_t stable_id, uint8_t ensure_visible)
{
    uint32_t previous_id = strip_demo_ctx.selected_id;

    strip_demo_ctx.selected_id = stable_id;
    if (previous_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && previous_id != strip_demo_ctx.selected_id)
    {
        egui_view_virtual_strip_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&strip_view), previous_id);
    }
    if (strip_demo_ctx.selected_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_strip_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&strip_view), strip_demo_ctx.selected_id);
        if (ensure_visible)
        {
            egui_view_virtual_strip_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&strip_view), strip_demo_ctx.selected_id, STRIP_KEEP_VISIBLE_INSET);
        }
    }
}

static void strip_demo_item_click_cb(egui_view_t *self)
{
    egui_view_virtual_strip_entry_t entry;

    if (!egui_view_virtual_strip_resolve_item_by_view(EGUI_VIEW_OF(&strip_view), self, &entry))
    {
        return;
    }

    strip_demo_ctx.last_clicked_index = entry.index;
    strip_demo_ctx.click_count++;
    strip_demo_update_selection(entry.stable_id, 1);
    snprintf(strip_demo_ctx.last_action_text, sizeof(strip_demo_ctx.last_action_text), "Click #%05lu at index %lu.", (unsigned long)entry.stable_id,
             (unsigned long)entry.index);
    strip_demo_refresh_header();
}

static void strip_demo_style_scene_buttons(void)
{
    uint8_t i;

    for (i = 0; i < STRIP_SCENE_COUNT; i++)
    {
        uint8_t active = strip_demo_ctx.scene == i;

        egui_view_set_background(EGUI_VIEW_OF(&scene_buttons[i]), active ? EGUI_BG_OF(&strip_scene_active_bg) : EGUI_BG_OF(&strip_scene_idle_bg));
        egui_view_label_set_font_color(EGUI_VIEW_OF(&scene_buttons[i]), active ? EGUI_COLOR_WHITE : EGUI_COLOR_HEX(0x304557), EGUI_ALPHA_100);
    }
}

static uint32_t strip_demo_pick_action_index(void)
{
    int32_t selected_index = strip_demo_find_index_by_stable_id(strip_demo_ctx.selected_id);

    if (selected_index >= 0)
    {
        return (uint32_t)selected_index;
    }
    if (strip_demo_ctx.item_count == 0)
    {
        return 0;
    }

    return strip_demo_ctx.jump_cursor % strip_demo_ctx.item_count;
}

static uint32_t strip_demo_get_count(void *data_source_context)
{
    return ((strip_demo_context_t *)data_source_context)->item_count;
}

static uint32_t strip_demo_get_stable_id(void *data_source_context, uint32_t index)
{
    strip_demo_context_t *ctx = (strip_demo_context_t *)data_source_context;
    return ctx->items[index].stable_id;
}

static int32_t strip_demo_find_index_adapter(void *data_source_context, uint32_t stable_id)
{
    strip_demo_context_t *ctx = (strip_demo_context_t *)data_source_context;
    uint32_t i;

    for (i = 0; i < ctx->item_count; i++)
    {
        if (ctx->items[i].stable_id == stable_id)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

static int32_t strip_demo_measure_item_width(void *data_source_context, uint32_t index, int32_t height_hint)
{
    strip_demo_context_t *ctx = (strip_demo_context_t *)data_source_context;
    const strip_demo_item_t *item = index < ctx->item_count ? &ctx->items[index] : NULL;

    return strip_demo_measure_item_width_with_state(item, item != NULL && item->stable_id == ctx->selected_id, height_hint);
}

static egui_view_t *strip_demo_create_item_view(void *data_source_context, uint16_t view_type)
{
    strip_demo_context_t *ctx = (strip_demo_context_t *)data_source_context;
    strip_demo_item_view_t *item_view;

    EGUI_UNUSED(view_type);

    if (ctx->created_count >= STRIP_VIEW_POOL_CAP)
    {
        return NULL;
    }

    item_view = &ctx->item_views[ctx->created_count];
    memset(item_view, 0, sizeof(*item_view));

    egui_view_group_init(EGUI_VIEW_OF(&item_view->root));
    egui_view_card_init(EGUI_VIEW_OF(&item_view->card));
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&item_view->card), 14);
    egui_view_init(EGUI_VIEW_OF(&item_view->accent));
    egui_view_label_init(EGUI_VIEW_OF(&item_view->tag));
    egui_view_label_init(EGUI_VIEW_OF(&item_view->title));
    egui_view_label_init(EGUI_VIEW_OF(&item_view->meta));
    egui_view_label_init(EGUI_VIEW_OF(&item_view->badge));
    egui_view_progress_bar_init(EGUI_VIEW_OF(&item_view->progress));
    egui_view_init(EGUI_VIEW_OF(&item_view->pulse));

    egui_view_group_add_child(EGUI_VIEW_OF(&item_view->root), EGUI_VIEW_OF(&item_view->card));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->accent));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->tag));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->title));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->meta));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->badge));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->progress));
    egui_view_card_add_child(EGUI_VIEW_OF(&item_view->card), EGUI_VIEW_OF(&item_view->pulse));

    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->tag), STRIP_FONT_CAP);
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->title), STRIP_FONT_TITLE);
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->meta), STRIP_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&item_view->badge), STRIP_FONT_CAP);

    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->tag), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->meta), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&item_view->badge), EGUI_ALIGN_CENTER);

    egui_view_set_shadow(EGUI_VIEW_OF(&item_view->card), &strip_demo_card_shadow);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&item_view->root), strip_demo_item_click_cb);

    egui_animation_alpha_init(EGUI_ANIM_OF(&item_view->pulse_anim));
    egui_animation_alpha_params_set(&item_view->pulse_anim, &strip_pulse_anim_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&item_view->pulse_anim), 860);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&item_view->pulse_anim), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&item_view->pulse_anim), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_interpolator_linear_init((egui_interpolator_t *)&item_view->pulse_interp);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&item_view->pulse_anim), (egui_interpolator_t *)&item_view->pulse_interp);
    egui_animation_target_view_set(EGUI_ANIM_OF(&item_view->pulse_anim), EGUI_VIEW_OF(&item_view->pulse));
    egui_view_set_gone(EGUI_VIEW_OF(&item_view->pulse), 1);

    item_view->bound_index = STRIP_INVALID_INDEX;
    item_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    ctx->created_count++;
    return EGUI_VIEW_OF(&item_view->root);
}

static void strip_demo_bind_item_view(void *data_source_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    strip_demo_context_t *ctx = (strip_demo_context_t *)data_source_context;
    strip_demo_item_view_t *item_view = strip_demo_find_view_by_root(view);
    int pool_index;

    if (item_view == NULL || index >= ctx->item_count)
    {
        return;
    }

    pool_index = strip_demo_get_view_pool_index(item_view);
    if (pool_index < 0)
    {
        return;
    }

    item_view->bound_index = index;
    item_view->stable_id = stable_id;
    strip_demo_layout_item_view(item_view, &ctx->items[index], pool_index, index, stable_id == ctx->selected_id);
}

static void strip_demo_unbind_item_view(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    strip_demo_item_view_t *item_view = strip_demo_find_view_by_root(view);

    EGUI_UNUSED(data_source_context);
    EGUI_UNUSED(stable_id);

    if (item_view == NULL)
    {
        return;
    }

    strip_demo_set_item_pulse(item_view, NULL, 0, 0);
    item_view->bound_index = STRIP_INVALID_INDEX;
    item_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static uint8_t strip_demo_should_keep_alive(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    strip_demo_context_t *ctx = (strip_demo_context_t *)data_source_context;
    int32_t index;

    EGUI_UNUSED(view);

    if (stable_id == ctx->selected_id)
    {
        return 1;
    }

    index = strip_demo_find_index_adapter(data_source_context, stable_id);
    if (index < 0)
    {
        return 0;
    }

    return strip_demo_item_has_pulse(&ctx->items[index], 0);
}

static void strip_demo_save_item_state(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    strip_demo_item_view_t *item_view = strip_demo_find_view_by_root(view);
    strip_demo_item_state_t state;

    EGUI_UNUSED(data_source_context);

    if (item_view == NULL)
    {
        return;
    }

    strip_demo_capture_view_state(item_view, &state);
    if (!state.pulse_running)
    {
        egui_view_virtual_strip_remove_item_state_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id);
        return;
    }

    (void)egui_view_virtual_strip_write_item_state_for_view(view, stable_id, &state, sizeof(state));
}

static void strip_demo_restore_item_state(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    strip_demo_item_view_t *item_view = strip_demo_find_view_by_root(view);
    strip_demo_item_state_t state;

    EGUI_UNUSED(data_source_context);

    if (item_view == NULL)
    {
        return;
    }

    if (egui_view_virtual_strip_read_item_state_for_view(view, stable_id, &state, sizeof(state)) != sizeof(state))
    {
        return;
    }

    strip_demo_restore_view_state(item_view, &state);
}

static const egui_view_virtual_strip_data_source_t strip_demo_data_source = {
        .get_count = strip_demo_get_count,
        .get_stable_id = strip_demo_get_stable_id,
        .find_index_by_stable_id = strip_demo_find_index_adapter,
        .get_item_view_type = NULL,
        .measure_item_width = strip_demo_measure_item_width,
        .create_item_view = strip_demo_create_item_view,
        .destroy_item_view = NULL,
        .bind_item_view = strip_demo_bind_item_view,
        .unbind_item_view = strip_demo_unbind_item_view,
        .should_keep_alive = strip_demo_should_keep_alive,
        .save_item_state = strip_demo_save_item_state,
        .restore_item_state = strip_demo_restore_item_state,
        .default_item_view_type = 0,
};

static int strip_demo_find_scene_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < STRIP_SCENE_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&scene_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static int strip_demo_find_action_button_index(egui_view_t *self)
{
    uint8_t i;

    for (i = 0; i < STRIP_ACTION_COUNT; i++)
    {
        if (self == EGUI_VIEW_OF(&action_buttons[i]))
        {
            return (int)i;
        }
    }

    return -1;
}

static void strip_demo_action_add(void)
{
    uint32_t index;
    uint32_t stable_id;

    if (strip_demo_ctx.item_count >= STRIP_MAX_ITEMS)
    {
        snprintf(strip_demo_ctx.last_action_text, sizeof(strip_demo_ctx.last_action_text), "Add ignored. Strip is full.");
        strip_demo_refresh_header();
        return;
    }

    index = strip_demo_ctx.item_count == 0 ? 0U : strip_demo_pick_action_index() + 1U;
    if (index > strip_demo_ctx.item_count)
    {
        index = strip_demo_ctx.item_count;
    }

    if (index < strip_demo_ctx.item_count)
    {
        memmove(&strip_demo_ctx.items[index + 1], &strip_demo_ctx.items[index], (strip_demo_ctx.item_count - index) * sizeof(strip_demo_ctx.items[0]));
    }

    stable_id = strip_demo_ctx.next_stable_id++;
    strip_demo_fill_item(&strip_demo_ctx.items[index], strip_demo_ctx.scene, stable_id, stable_id);
    strip_demo_ctx.items[index].state = STRIP_ITEM_STATE_LIVE;
    strip_demo_ctx.items[index].progress = 24U;
    strip_demo_ctx.item_count++;
    strip_demo_ctx.jump_cursor = index;
    strip_demo_ctx.action_count++;
    strip_demo_ctx.mutation_count++;

    egui_view_virtual_strip_notify_item_inserted(EGUI_VIEW_OF(&strip_view), index, 1);
    strip_demo_update_selection(stable_id, 1);
    snprintf(strip_demo_ctx.last_action_text, sizeof(strip_demo_ctx.last_action_text), "Inserted #%05lu.", (unsigned long)stable_id);
    strip_demo_refresh_header();
}

static void strip_demo_action_del(void)
{
    uint32_t index;
    uint32_t stable_id;
    uint8_t removed_selected;

    if (strip_demo_ctx.item_count == 0)
    {
        snprintf(strip_demo_ctx.last_action_text, sizeof(strip_demo_ctx.last_action_text), "Del ignored. Strip is empty.");
        strip_demo_refresh_header();
        return;
    }

    index = strip_demo_pick_action_index();
    stable_id = strip_demo_ctx.items[index].stable_id;
    removed_selected = stable_id == strip_demo_ctx.selected_id ? 1U : 0U;

    egui_view_virtual_strip_remove_item_state_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id);
    if ((index + 1U) < strip_demo_ctx.item_count)
    {
        memmove(&strip_demo_ctx.items[index], &strip_demo_ctx.items[index + 1], (strip_demo_ctx.item_count - index - 1U) * sizeof(strip_demo_ctx.items[0]));
    }

    strip_demo_ctx.item_count--;
    strip_demo_ctx.action_count++;
    strip_demo_ctx.mutation_count++;
    if (strip_demo_ctx.item_count == 0)
    {
        strip_demo_ctx.jump_cursor = 0;
    }
    else if (strip_demo_ctx.jump_cursor >= strip_demo_ctx.item_count)
    {
        strip_demo_ctx.jump_cursor = 0;
    }
    if (removed_selected)
    {
        strip_demo_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    egui_view_virtual_strip_notify_item_removed(EGUI_VIEW_OF(&strip_view), index, 1);
    snprintf(strip_demo_ctx.last_action_text, sizeof(strip_demo_ctx.last_action_text), "Deleted #%05lu.", (unsigned long)stable_id);
    strip_demo_refresh_header();
}

static void strip_demo_action_patch(void)
{
    uint32_t index;
    strip_demo_item_t *item;
    uint8_t selected;
    int32_t old_width;
    int32_t new_width;

    if (strip_demo_ctx.item_count == 0)
    {
        snprintf(strip_demo_ctx.last_action_text, sizeof(strip_demo_ctx.last_action_text), "Patch ignored. Strip is empty.");
        strip_demo_refresh_header();
        return;
    }

    index = strip_demo_pick_action_index();
    item = &strip_demo_ctx.items[index];
    selected = item->stable_id == strip_demo_ctx.selected_id ? 1U : 0U;
    old_width = strip_demo_measure_item_width_with_state(item, selected, strip_demo_get_view_height_hint());

    item->revision++;
    item->variant = (uint8_t)((item->variant + 1U) % STRIP_ITEM_VARIANT_COUNT);
    item->state = (uint8_t)((item->state + 1U) % STRIP_ITEM_STATE_COUNT);
    if (item->state == STRIP_ITEM_STATE_DONE)
    {
        item->progress = 100U;
    }
    else
    {
        item->progress = (uint8_t)(18U + ((item->progress + 17U) % 73U));
    }

    new_width = strip_demo_measure_item_width_with_state(item, selected, strip_demo_get_view_height_hint());
    strip_demo_ctx.jump_cursor = (index + 17U) % strip_demo_ctx.item_count;
    strip_demo_ctx.action_count++;
    strip_demo_ctx.mutation_count++;

    if (new_width != old_width)
    {
        egui_view_virtual_strip_notify_item_resized(EGUI_VIEW_OF(&strip_view), index);
    }
    else
    {
        egui_view_virtual_strip_notify_item_changed(EGUI_VIEW_OF(&strip_view), index);
    }
    if (selected)
    {
        egui_view_virtual_strip_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&strip_view), item->stable_id, STRIP_KEEP_VISIBLE_INSET);
    }

    snprintf(strip_demo_ctx.last_action_text, sizeof(strip_demo_ctx.last_action_text), "Patched #%05lu.", (unsigned long)item->stable_id);
    strip_demo_refresh_header();
}

static void strip_demo_action_jump(void)
{
    uint32_t index;
    uint32_t stable_id;

    if (strip_demo_ctx.item_count == 0)
    {
        snprintf(strip_demo_ctx.last_action_text, sizeof(strip_demo_ctx.last_action_text), "Jump ignored. Strip is empty.");
        strip_demo_refresh_header();
        return;
    }

    strip_demo_ctx.jump_cursor = (strip_demo_ctx.jump_cursor + 23U) % strip_demo_ctx.item_count;
    index = strip_demo_ctx.jump_cursor;
    stable_id = strip_demo_ctx.items[index].stable_id;
    strip_demo_ctx.action_count++;

    strip_demo_update_selection(stable_id, 0);
    egui_view_virtual_strip_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id, STRIP_KEEP_VISIBLE_INSET);
    snprintf(strip_demo_ctx.last_action_text, sizeof(strip_demo_ctx.last_action_text), "Jump to #%05lu.", (unsigned long)stable_id);
    strip_demo_refresh_header();
}

static void strip_demo_apply_action(uint8_t action)
{
    switch (action)
    {
    case STRIP_ACTION_ADD:
        strip_demo_action_add();
        break;
    case STRIP_ACTION_DEL:
        strip_demo_action_del();
        break;
    case STRIP_ACTION_PATCH:
        strip_demo_action_patch();
        break;
    case STRIP_ACTION_JUMP:
        strip_demo_action_jump();
        break;
    default:
        break;
    }
}

static void strip_demo_switch_scene(uint8_t scene)
{
    if (scene >= STRIP_SCENE_COUNT)
    {
        return;
    }

    if (strip_demo_ctx.scene != scene)
    {
        strip_demo_ctx.scene_switch_count++;
    }

    strip_demo_reset_scene_model(scene);
    egui_view_virtual_strip_clear_item_state_cache(EGUI_VIEW_OF(&strip_view));
    egui_view_virtual_strip_set_scroll_x(EGUI_VIEW_OF(&strip_view), 0);
    egui_view_virtual_strip_notify_data_changed(EGUI_VIEW_OF(&strip_view));
    strip_demo_style_scene_buttons();
    strip_demo_style_overlay();
    strip_demo_refresh_header();
}

static void strip_demo_button_click_cb(egui_view_t *self)
{
    int scene = strip_demo_find_scene_button_index(self);
    int action;

    if (scene >= 0)
    {
        strip_demo_switch_scene((uint8_t)scene);
        return;
    }

    action = strip_demo_find_action_button_index(self);
    if (action >= 0)
    {
        strip_demo_apply_action((uint8_t)action);
    }
}

static void strip_demo_init_button(egui_view_button_t *button, egui_dim_t x, egui_dim_t y, egui_dim_t width, const char *text)
{
    egui_view_button_init(EGUI_VIEW_OF(button));
    egui_view_set_position(EGUI_VIEW_OF(button), x, y);
    egui_view_set_size(EGUI_VIEW_OF(button), width, STRIP_BUTTON_H);
    egui_view_label_set_text(EGUI_VIEW_OF(button), text);
    egui_view_label_set_font(EGUI_VIEW_OF(button), STRIP_FONT_CAP);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_HEX(0x304557), EGUI_ALPHA_100);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), strip_demo_button_click_cb);
}

void test_init_ui(void)
{
    uint8_t i;
    egui_dim_t x;

    memset(&strip_demo_ctx, 0, sizeof(strip_demo_ctx));
    strip_demo_ctx.selected_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    strip_demo_ctx.last_clicked_index = STRIP_INVALID_INDEX;
    strip_demo_reset_scene_model(STRIP_SCENE_GALLERY);

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
    recording_click_verify_retry = 0U;
    recording_mutation_verify_retry = 0U;
    recording_scene_verify_retry = 0U;
    recording_swipe_verify_retry = 0U;
#endif

    egui_view_init(EGUI_VIEW_OF(&background_view));
    egui_view_set_size(EGUI_VIEW_OF(&background_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&background_view), EGUI_BG_OF(&strip_screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&header_card), &strip_header_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&header_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&header_card), 1, EGUI_COLOR_HEX(0xD8E1EA));
    egui_view_set_shadow(EGUI_VIEW_OF(&header_card), &strip_demo_card_shadow);

    egui_view_label_init(EGUI_VIEW_OF(&header_title));
    egui_view_set_position(EGUI_VIEW_OF(&header_title), 12, 10);
    egui_view_set_size(EGUI_VIEW_OF(&header_title), STRIP_CONTENT_W - 24, 16);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_title), STRIP_FONT_HEADER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_title), EGUI_COLOR_HEX(0x243646), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_title));

    egui_view_label_init(EGUI_VIEW_OF(&header_detail));
    egui_view_set_position(EGUI_VIEW_OF(&header_detail), 12, 28);
    egui_view_set_size(EGUI_VIEW_OF(&header_detail), STRIP_CONTENT_W - 24, 14);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_detail), STRIP_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_detail), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_detail), EGUI_COLOR_HEX(0x5B6C7A), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_detail));

    egui_view_label_init(EGUI_VIEW_OF(&header_hint));
    egui_view_set_position(EGUI_VIEW_OF(&header_hint), 12, 42);
    egui_view_set_size(EGUI_VIEW_OF(&header_hint), STRIP_CONTENT_W - 24, 14);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_hint), STRIP_FONT_BODY);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_hint), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_hint), EGUI_COLOR_HEX(0x687B8D), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_hint));

    x = 12;
    for (i = 0; i < STRIP_SCENE_COUNT; i++)
    {
        strip_demo_init_button(&scene_buttons[i], x, 54, STRIP_SCENE_BUTTON_W, strip_demo_scene_names[i]);
        egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&scene_buttons[i]));
        x += STRIP_SCENE_BUTTON_W + STRIP_SCENE_BUTTON_GAP;
    }

    egui_view_card_init_with_params(EGUI_VIEW_OF(&toolbar_card), &strip_toolbar_card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&toolbar_card), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&toolbar_card), 1, EGUI_COLOR_HEX(0xD8E1EA));

    x = 10;
    for (i = 0; i < STRIP_ACTION_COUNT; i++)
    {
        strip_demo_init_button(&action_buttons[i], x, 6, STRIP_ACTION_BUTTON_W, strip_demo_action_names[i]);
        egui_view_set_background(EGUI_VIEW_OF(&action_buttons[i]), i == STRIP_ACTION_ADD     ? EGUI_BG_OF(&strip_action_add_bg)
                                                                   : i == STRIP_ACTION_DEL   ? EGUI_BG_OF(&strip_action_del_bg)
                                                                   : i == STRIP_ACTION_PATCH ? EGUI_BG_OF(&strip_action_patch_bg)
                                                                                             : EGUI_BG_OF(&strip_action_jump_bg));
        egui_view_card_add_child(EGUI_VIEW_OF(&toolbar_card), EGUI_VIEW_OF(&action_buttons[i]));
        x += STRIP_ACTION_BUTTON_W + STRIP_ACTION_BUTTON_GAP;
    }

    {
        const egui_view_virtual_strip_setup_t strip_view_setup = {
                .params = &strip_view_params,
                .data_source = &strip_demo_data_source,
                .data_source_context = &strip_demo_ctx,
                .state_cache_max_entries = STRIP_STATE_CACHE_COUNT,
                .state_cache_max_bytes = STRIP_STATE_CACHE_COUNT * (uint32_t)sizeof(strip_demo_item_state_t),
        };

        egui_view_virtual_strip_init_with_setup(EGUI_VIEW_OF(&strip_view), &strip_view_setup);
    }
    egui_view_set_background(EGUI_VIEW_OF(&strip_view), EGUI_BG_OF(&strip_view_bg));
    egui_view_set_shadow(EGUI_VIEW_OF(&strip_view), &strip_demo_card_shadow);

    egui_view_group_init(EGUI_VIEW_OF(&strip_overlay_view));
    egui_view_set_position(EGUI_VIEW_OF(&strip_overlay_view), STRIP_MARGIN_X, STRIP_VIEW_Y);
    egui_view_set_size(EGUI_VIEW_OF(&strip_overlay_view), STRIP_CONTENT_W, STRIP_VIEW_H);
    egui_view_set_clickable(EGUI_VIEW_OF(&strip_overlay_view), 0);

    egui_view_card_init(EGUI_VIEW_OF(&strip_overlay_line));
    egui_view_set_clickable(EGUI_VIEW_OF(&strip_overlay_line), 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&strip_overlay_view), EGUI_VIEW_OF(&strip_overlay_line));

    egui_view_card_init(EGUI_VIEW_OF(&strip_overlay_dot));
    egui_view_set_clickable(EGUI_VIEW_OF(&strip_overlay_dot), 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&strip_overlay_view), EGUI_VIEW_OF(&strip_overlay_dot));

    strip_demo_style_scene_buttons();
    strip_demo_style_overlay();
    strip_demo_refresh_header();

    egui_core_add_user_root_view(EGUI_VIEW_OF(&background_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&strip_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&strip_overlay_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&header_card));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_card));
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

static uint8_t strip_demo_is_view_clickable(egui_view_t *view)
{
    int click_x;
    int click_y;

    return (uint8_t)egui_sim_get_view_clipped_center(view, &EGUI_VIEW_OF(&strip_view)->region_screen, &click_x, &click_y);
}

static uint8_t strip_demo_set_click_item_action(egui_sim_action_t *p_action, egui_view_t *view, uint32_t interval_ms)
{
    return (uint8_t)egui_sim_set_click_view_clipped(p_action, view, &EGUI_VIEW_OF(&strip_view)->region_screen, (int)interval_ms);
}

typedef struct strip_demo_visible_search_context
{
    uint32_t min_index;
} strip_demo_visible_search_context_t;

static uint8_t strip_demo_match_visible_item(egui_view_t *self, const egui_view_virtual_strip_slot_t *slot, const egui_view_virtual_strip_entry_t *entry,
                                             egui_view_t *item_view, void *context)
{
    strip_demo_visible_search_context_t *ctx = (strip_demo_visible_search_context_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(slot);
    return (uint8_t)(entry != NULL && entry->index >= ctx->min_index && strip_demo_is_view_clickable(item_view));
}

static egui_view_t *strip_demo_find_visible_view_by_index(uint32_t index)
{
    uint32_t stable_id;
    const egui_view_virtual_strip_slot_t *slot;
    egui_view_t *view;

    if (index >= strip_demo_ctx.item_count)
    {
        return NULL;
    }

    stable_id = strip_demo_ctx.items[index].stable_id;
    slot = egui_view_virtual_strip_find_slot_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id);
    if (slot == NULL)
    {
        return NULL;
    }

    view = egui_view_virtual_strip_find_view_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id);
    return strip_demo_is_view_clickable(view) ? view : NULL;
}

static egui_view_t *strip_demo_find_first_visible_view_after(uint32_t min_index)
{
    strip_demo_visible_search_context_t ctx = {
            .min_index = min_index,
    };

    return egui_view_virtual_strip_find_first_visible_item_view(EGUI_VIEW_OF(&strip_view), strip_demo_match_visible_item, &ctx, NULL);
}

static uint8_t strip_demo_schedule_verify_retry(uint8_t *retry_counter, uint8_t retry_max, egui_sim_action_t *p_action)
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

static void strip_demo_set_swipe_action(egui_sim_action_t *p_action, uint32_t interval_ms)
{
    p_action->type = EGUI_SIM_ACTION_SWIPE;
    p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH * 4 / 5;
    p_action->y1 = STRIP_VIEW_Y + STRIP_VIEW_H / 2;
    p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 3;
    p_action->y2 = STRIP_VIEW_Y + STRIP_VIEW_H / 2;
    p_action->steps = 5;
    p_action->interval_ms = interval_ms;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    egui_view_t *view;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call && strip_demo_ctx.created_count > STRIP_VIEW_POOL_CAP)
        {
            report_runtime_failure("virtual strip created more views than slot capacity");
        }
        if (first_call)
        {
            recording_request_snapshot();
        }
        view = strip_demo_find_visible_view_by_index(1);
        if (view == NULL)
        {
            report_runtime_failure("initial strip item was not visible");
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        if (!strip_demo_set_click_item_action(p_action, view, 220))
        {
            report_runtime_failure("initial strip item click point was not clickable");
            EGUI_SIM_SET_WAIT(p_action, 220);
        }
        return true;
    case 1:
        if (strip_demo_ctx.last_clicked_index != 1U)
        {
            if (recording_click_verify_retry < STRIP_CLICK_VERIFY_RETRY_MAX)
            {
                view = strip_demo_find_visible_view_by_index(1);
                recording_click_verify_retry++;
                if (view != NULL && strip_demo_set_click_item_action(p_action, view, 220))
                {
                    return true;
                }
                recording_request_snapshot();
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            report_runtime_failure("gallery item click did not update selected index");
        }
        recording_click_verify_retry = 0U;
        recording_mutation_verify_retry = 0U;
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[STRIP_ACTION_PATCH]), 200);
        return true;
    case 2:
        if (strip_demo_ctx.mutation_count < 1U)
        {
            if (strip_demo_schedule_verify_retry(&recording_mutation_verify_retry, STRIP_MUTATION_VERIFY_RETRY_MAX, p_action))
            {
                return true;
            }
            report_runtime_failure("gallery patch did not mutate data");
        }
        recording_mutation_verify_retry = 0U;
        if (first_call)
        {
            recording_swipe_verify_retry = 0U;
            recording_request_snapshot();
        }
        strip_demo_set_swipe_action(p_action, 520);
        return true;
    case 3:
        if (egui_view_virtual_strip_get_scroll_x(EGUI_VIEW_OF(&strip_view)) <= 0)
        {
            if (recording_swipe_verify_retry < 3U)
            {
                recording_swipe_verify_retry++;
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            report_runtime_failure("gallery swipe did not move strip viewport");
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        view = strip_demo_find_first_visible_view_after(3);
        if (view != NULL)
        {
            recording_swipe_verify_retry = 0U;
            if (!strip_demo_set_click_item_action(p_action, view, 220))
            {
                EGUI_SIM_SET_WAIT(p_action, 220);
                return true;
            }
            return true;
        }
        recording_swipe_verify_retry = 0U;
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 4:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[STRIP_ACTION_JUMP]), 220);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[STRIP_ACTION_ADD]), 220);
        return true;
    case 6:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[STRIP_ACTION_DEL]), 220);
        return true;
    case 7:
        if (strip_demo_ctx.mutation_count < 3U)
        {
            if (strip_demo_schedule_verify_retry(&recording_mutation_verify_retry, STRIP_MUTATION_VERIFY_RETRY_MAX, p_action))
            {
                return true;
            }
            report_runtime_failure("gallery add/del flow did not mutate data");
        }
        recording_mutation_verify_retry = 0U;
        recording_scene_verify_retry = 0U;
        if (strip_demo_ctx.scene != STRIP_SCENE_QUEUE)
        {
            strip_demo_switch_scene(STRIP_SCENE_QUEUE);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 8:
        if (strip_demo_ctx.scene != STRIP_SCENE_QUEUE)
        {
            if (strip_demo_schedule_verify_retry(&recording_scene_verify_retry, STRIP_SCENE_VERIFY_RETRY_MAX, p_action))
            {
                return true;
            }
            report_runtime_failure("scene switch to queue failed");
        }
        recording_scene_verify_retry = 0U;
        if (first_call)
        {
            recording_swipe_verify_retry = 0U;
        }
        strip_demo_set_swipe_action(p_action, 520);
        return true;
    case 9:
        if (egui_view_virtual_strip_get_scroll_x(EGUI_VIEW_OF(&strip_view)) <= 0)
        {
            if (recording_swipe_verify_retry < 3U)
            {
                recording_swipe_verify_retry++;
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            report_runtime_failure("queue swipe did not move strip viewport");
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        view = strip_demo_find_first_visible_view_after(2);
        if (view != NULL)
        {
            recording_swipe_verify_retry = 0U;
            if (!strip_demo_set_click_item_action(p_action, view, 220))
            {
                EGUI_SIM_SET_WAIT(p_action, 220);
                return true;
            }
            return true;
        }
        recording_swipe_verify_retry = 0U;
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 10:
        recording_mutation_verify_retry = 0U;
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[STRIP_ACTION_PATCH]), 220);
        return true;
    case 11:
        if (strip_demo_ctx.mutation_count < 4U)
        {
            if (strip_demo_schedule_verify_retry(&recording_mutation_verify_retry, STRIP_MUTATION_VERIFY_RETRY_MAX, p_action))
            {
                return true;
            }
            report_runtime_failure("queue patch did not mutate data");
        }
        recording_mutation_verify_retry = 0U;
        recording_scene_verify_retry = 0U;
        if (strip_demo_ctx.scene != STRIP_SCENE_TIMELINE)
        {
            strip_demo_switch_scene(STRIP_SCENE_TIMELINE);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 12:
        if (strip_demo_ctx.scene != STRIP_SCENE_TIMELINE)
        {
            if (strip_demo_schedule_verify_retry(&recording_scene_verify_retry, STRIP_SCENE_VERIFY_RETRY_MAX, p_action))
            {
                return true;
            }
            report_runtime_failure("scene switch to timeline failed");
        }
        recording_scene_verify_retry = 0U;
        if (first_call)
        {
            recording_swipe_verify_retry = 0U;
        }
        strip_demo_set_swipe_action(p_action, 520);
        return true;
    case 13:
        if (egui_view_virtual_strip_get_scroll_x(EGUI_VIEW_OF(&strip_view)) <= 0)
        {
            if (recording_swipe_verify_retry < 3U)
            {
                recording_swipe_verify_retry++;
                EGUI_SIM_SET_WAIT(p_action, 180);
                return true;
            }
            report_runtime_failure("timeline swipe did not move strip viewport");
            EGUI_SIM_SET_WAIT(p_action, 180);
            return true;
        }
        view = strip_demo_find_first_visible_view_after(4);
        if (view != NULL)
        {
            recording_swipe_verify_retry = 0U;
            if (!strip_demo_set_click_item_action(p_action, view, 220))
            {
                EGUI_SIM_SET_WAIT(p_action, 220);
                return true;
            }
            return true;
        }
        recording_swipe_verify_retry = 0U;
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 14:
        recording_mutation_verify_retry = 0U;
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[STRIP_ACTION_PATCH]), 220);
        return true;
    case 15:
        if (strip_demo_ctx.mutation_count < 5U)
        {
            if (strip_demo_schedule_verify_retry(&recording_mutation_verify_retry, STRIP_MUTATION_VERIFY_RETRY_MAX, p_action))
            {
                return true;
            }
            report_runtime_failure("timeline patch did not mutate data");
        }
        recording_mutation_verify_retry = 0U;
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&action_buttons[STRIP_ACTION_JUMP]), 220);
        return true;
    case 16:
        if (first_call)
        {
            if (strip_demo_ctx.mutation_count < 5U)
            {
                report_runtime_failure("runtime flow missed data mutations");
            }
            if (strip_demo_ctx.scene_switch_count < 2U)
            {
                report_runtime_failure("runtime flow did not switch scenes twice");
            }
            if (strip_demo_ctx.selected_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
            {
                report_runtime_failure("runtime flow did not keep any item selected");
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
