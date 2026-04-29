#include "egui.h"
#include "core/egui_core_activity.h"
#include "uicode_disp0.h"

#define DIRTY_PASSTHROUGH_ACTIVITY_COUNT          4
#define DIRTY_PASSTHROUGH_ACTIVITY_SECTIONS       2
#define DIRTY_PASSTHROUGH_ACTIVITY_CARDS          3
#define DIRTY_PASSTHROUGH_ACTIVITY_VIEWPAGE_PAGES 2
#define DIRTY_PASSTHROUGH_ACTIVITY_MIXED_ROWS     5
#define DIRTY_PASSTHROUGH_ACTIVITY_NAV_HEIGHT     48
#define DIRTY_PASSTHROUGH_ACTIVITY_CONTENT_HEIGHT (EGUI_CONFIG_SCREEN_HEIGHT - DIRTY_PASSTHROUGH_ACTIVITY_NAV_HEIGHT)

typedef struct dirty_passthrough_activity
{
    egui_activity_t base;
    uint8_t index;
    char title_text[24];

    egui_view_group_t header;
    egui_view_label_t title;
    egui_view_button_t next_button;
    egui_view_button_t finish_button;

    egui_view_scroll_t scroll;
    egui_view_group_t sections[DIRTY_PASSTHROUGH_ACTIVITY_SECTIONS];
    egui_view_label_t cards[DIRTY_PASSTHROUGH_ACTIVITY_SECTIONS][DIRTY_PASSTHROUGH_ACTIVITY_CARDS];

    egui_view_linearlayout_t linear;
    egui_view_label_t linear_cards[DIRTY_PASSTHROUGH_ACTIVITY_CARDS];

    egui_view_viewpage_t viewpage;
    egui_view_group_t viewpage_pages[DIRTY_PASSTHROUGH_ACTIVITY_VIEWPAGE_PAGES];
    egui_view_label_t viewpage_cards[DIRTY_PASSTHROUGH_ACTIVITY_VIEWPAGE_PAGES][DIRTY_PASSTHROUGH_ACTIVITY_CARDS];

    egui_view_viewpage_t mixed_viewpage;
    egui_view_scroll_t mixed_scrolls[DIRTY_PASSTHROUGH_ACTIVITY_VIEWPAGE_PAGES];
    egui_view_linearlayout_t mixed_layouts[DIRTY_PASSTHROUGH_ACTIVITY_VIEWPAGE_PAGES];
    egui_view_label_t mixed_cards[DIRTY_PASSTHROUGH_ACTIVITY_VIEWPAGE_PAGES][DIRTY_PASSTHROUGH_ACTIVITY_MIXED_ROWS];
} dirty_passthrough_activity_t;

static egui_core_t *s_core;
static dirty_passthrough_activity_t s_activities[DIRTY_PASSTHROUGH_ACTIVITY_COUNT];

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_open_param, EGUI_CONFIG_SCREEN_WIDTH, 0, 0, 0);
static egui_animation_translate_t anim_start_open;

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_close_param, 0, -EGUI_CONFIG_SCREEN_WIDTH, 0, 0);
static egui_animation_translate_t anim_start_close;

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_finish_open_param, -EGUI_CONFIG_SCREEN_WIDTH, 0, 0, 0);
static egui_animation_translate_t anim_finish_open;

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_finish_close_param, 0, EGUI_CONFIG_SCREEN_WIDTH, 0, 0);
static egui_animation_translate_t anim_finish_close;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_activity_root_0_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_activity_root_0_params, &bg_activity_root_0_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_activity_root_0, &bg_activity_root_0_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_activity_primary_param, EGUI_THEME_PRIMARY, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_activity_primary_params, &bg_activity_primary_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_activity_primary, &bg_activity_primary_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_activity_success_param, EGUI_THEME_SUCCESS, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_activity_success_params, &bg_activity_success_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_activity_success, &bg_activity_success_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_activity_danger_param, EGUI_THEME_DANGER, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_activity_danger_params, &bg_activity_danger_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_activity_danger, &bg_activity_danger_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_activity_button_param, EGUI_THEME_TRACK_BG_DARK, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_activity_button_pressed_param, EGUI_THEME_PRIMARY_LIGHT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_activity_button_params, &bg_activity_button_param, &bg_activity_button_pressed_param, NULL);
static egui_background_color_t bg_activity_button;

static const char *s_activity_card_texts[DIRTY_PASSTHROUGH_ACTIVITY_COUNT][DIRTY_PASSTHROUGH_ACTIVITY_CARDS] = {
        {"Activity root", "Sparse cards", "Small sweeps"},
        {"Linear page", "Layout child", "No root wipe"},
        {"ViewPage", "Swipe child", "Leaf dirty"},
        {"Mixed stack", "Scroll rows", "Nested leaves"},
};

static const char *s_activity_viewpage_texts[DIRTY_PASSTHROUGH_ACTIVITY_VIEWPAGE_PAGES][DIRTY_PASSTHROUGH_ACTIVITY_CARDS] = {
        {"VP Activity 1", "Sparse child", "Swipe left"},
        {"VP Activity 2", "Child bounds", "Swipe right"},
};

static const char *s_activity_mixed_texts[DIRTY_PASSTHROUGH_ACTIVITY_VIEWPAGE_PAGES][DIRTY_PASSTHROUGH_ACTIVITY_MIXED_ROWS] = {
        {"Mix Activity 1", "Scroll row 1", "Linear row 2", "Nested row 3", "Leaf row 4"},
        {"Mix Activity 2", "Scroll row A", "Linear row B", "Nested row C", "Leaf row D"},
};

static egui_activity_t *dirty_passthrough_activity_get_current(void)
{
    if (s_core == NULL)
    {
        return NULL;
    }
    return egui_core_activity_get_current_active(s_core);
}

static egui_background_t *dirty_passthrough_activity_get_card_bg(int index)
{
    switch (index % 3)
    {
    case 0:
        return EGUI_BG_OF(&bg_activity_primary);
    case 1:
        return EGUI_BG_OF(&bg_activity_success);
    default:
        return EGUI_BG_OF(&bg_activity_danger);
    }
}

static void dirty_passthrough_activity_init_label(egui_core_t *core, egui_view_label_t *label, const char *text, egui_dim_t x, egui_dim_t y, egui_dim_t width,
                                                  egui_dim_t height, egui_color_t font_color, egui_background_t *background)
{
    egui_view_label_init(EGUI_VIEW_OF(label), core);
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), width, height);
    egui_view_label_set_text(EGUI_VIEW_OF(label), text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(label), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), font_color, EGUI_ALPHA_100);
    if (background != NULL)
    {
        egui_view_set_background(EGUI_VIEW_OF(label), background);
    }
}

static void dirty_passthrough_activity_init_button(egui_core_t *core, egui_view_button_t *button, const char *text, egui_dim_t x, egui_dim_t y,
                                                   egui_view_on_click_listener_t listener)
{
    egui_view_button_init(EGUI_VIEW_OF(button), core);
    egui_view_set_position(EGUI_VIEW_OF(button), x, y);
    egui_view_set_size(EGUI_VIEW_OF(button), 64, 28);
    egui_view_label_set_text(EGUI_VIEW_OF(button), text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(button), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(button), EGUI_BG_OF(&bg_activity_button));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), listener);
}

static void dirty_passthrough_activity_init_instance(dirty_passthrough_activity_t *activity, egui_core_t *core, uint8_t index);

static void dirty_passthrough_activity_start_next(egui_activity_t *prev_activity)
{
    dirty_passthrough_activity_t *activity;
    uint8_t next_index;

    if (s_core == NULL)
    {
        return;
    }

    if (prev_activity == NULL)
    {
        next_index = 0;
    }
    else
    {
        dirty_passthrough_activity_t *prev = (dirty_passthrough_activity_t *)prev_activity;
        if (prev->index + 1U >= DIRTY_PASSTHROUGH_ACTIVITY_COUNT)
        {
            return;
        }
        next_index = (uint8_t)(prev->index + 1U);
    }

    activity = &s_activities[next_index];
    if (egui_activity_check_in_process((egui_activity_t *)activity))
    {
        return;
    }

    dirty_passthrough_activity_init_instance(activity, s_core, next_index);
    egui_activity_start((egui_activity_t *)activity, prev_activity);
}

static void dirty_passthrough_activity_next_click(egui_view_t *self)
{
    dirty_passthrough_activity_start_next(egui_view_get_activity(self));
}

static void dirty_passthrough_activity_finish_click(egui_view_t *self)
{
    egui_activity_t *activity = egui_view_get_activity(self);
    dirty_passthrough_activity_t *local = (dirty_passthrough_activity_t *)activity;

    if (activity != NULL && local->index > 0)
    {
        egui_activity_finish(activity);
    }
}

static void dirty_passthrough_activity_init_section(dirty_passthrough_activity_t *activity, egui_core_t *core, uint8_t section_index)
{
    static const egui_region_t card_regions[DIRTY_PASSTHROUGH_ACTIVITY_CARDS] = {
            {{18, 20}, {128, 26}},
            {{76, 88}, {136, 26}},
            {{34, 158}, {148, 26}},
    };

    egui_view_group_init(EGUI_VIEW_OF(&activity->sections[section_index]), core);
    egui_view_set_size(EGUI_VIEW_OF(&activity->sections[section_index]), EGUI_CONFIG_SCREEN_WIDTH, 214);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&activity->sections[section_index]), 1);

    for (int i = 0; i < DIRTY_PASSTHROUGH_ACTIVITY_CARDS; i++)
    {
        int text_index = (i + section_index) % DIRTY_PASSTHROUGH_ACTIVITY_CARDS;
        dirty_passthrough_activity_init_label(core, &activity->cards[section_index][i], s_activity_card_texts[activity->index][text_index],
                                              card_regions[i].location.x, card_regions[i].location.y, card_regions[i].size.width, card_regions[i].size.height,
                                              EGUI_COLOR_WHITE, dirty_passthrough_activity_get_card_bg((int)activity->index + i));
        egui_view_group_add_child(EGUI_VIEW_OF(&activity->sections[section_index]), EGUI_VIEW_OF(&activity->cards[section_index][i]));
    }
}

static void dirty_passthrough_activity_init_sparse_scroll(dirty_passthrough_activity_t *activity, egui_core_t *core)
{
    egui_view_scroll_init(EGUI_VIEW_OF(&activity->scroll), core);
    egui_view_set_position(EGUI_VIEW_OF(&activity->scroll), 0, DIRTY_PASSTHROUGH_ACTIVITY_NAV_HEIGHT);
    egui_view_set_size(EGUI_VIEW_OF(&activity->scroll), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_ACTIVITY_CONTENT_HEIGHT);
    egui_view_scroll_set_scrollbar_enabled(EGUI_VIEW_OF(&activity->scroll), 1);

    for (uint8_t i = 0; i < DIRTY_PASSTHROUGH_ACTIVITY_SECTIONS; i++)
    {
        dirty_passthrough_activity_init_section(activity, core, i);
        egui_view_scroll_add_child(EGUI_VIEW_OF(&activity->scroll), EGUI_VIEW_OF(&activity->sections[i]));
    }
    egui_view_scroll_layout_childs(EGUI_VIEW_OF(&activity->scroll));
}

static void dirty_passthrough_activity_init_linear(dirty_passthrough_activity_t *activity, egui_core_t *core)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&activity->linear), core);
    egui_view_set_position(EGUI_VIEW_OF(&activity->linear), 0, DIRTY_PASSTHROUGH_ACTIVITY_NAV_HEIGHT);
    egui_view_set_size(EGUI_VIEW_OF(&activity->linear), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_ACTIVITY_CONTENT_HEIGHT);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&activity->linear), EGUI_ALIGN_CENTER);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&activity->linear), 1);

    for (int i = 0; i < DIRTY_PASSTHROUGH_ACTIVITY_CARDS; i++)
    {
        dirty_passthrough_activity_init_label(core, &activity->linear_cards[i], s_activity_card_texts[activity->index][i], 0, 0, 156 + i * 18, 38,
                                              EGUI_COLOR_WHITE, dirty_passthrough_activity_get_card_bg((int)activity->index + i));
        egui_view_group_add_child(EGUI_VIEW_OF(&activity->linear), EGUI_VIEW_OF(&activity->linear_cards[i]));
    }
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&activity->linear));
}

static void dirty_passthrough_activity_init_viewpage_card_group(dirty_passthrough_activity_t *activity, egui_core_t *core, uint8_t page_index)
{
    static const egui_region_t card_regions[DIRTY_PASSTHROUGH_ACTIVITY_CARDS] = {
            {{24, 24}, {138, 28}},
            {{78, 102}, {130, 28}},
            {{40, 182}, {148, 28}},
    };

    egui_view_group_init(EGUI_VIEW_OF(&activity->viewpage_pages[page_index]), core);
    egui_view_set_size(EGUI_VIEW_OF(&activity->viewpage_pages[page_index]), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_ACTIVITY_CONTENT_HEIGHT);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&activity->viewpage_pages[page_index]), 1);

    for (int i = 0; i < DIRTY_PASSTHROUGH_ACTIVITY_CARDS; i++)
    {
        dirty_passthrough_activity_init_label(core, &activity->viewpage_cards[page_index][i], s_activity_viewpage_texts[page_index][i],
                                              card_regions[i].location.x, card_regions[i].location.y, card_regions[i].size.width, card_regions[i].size.height,
                                              EGUI_COLOR_WHITE, dirty_passthrough_activity_get_card_bg((int)activity->index + page_index + i));
        egui_view_group_add_child(EGUI_VIEW_OF(&activity->viewpage_pages[page_index]), EGUI_VIEW_OF(&activity->viewpage_cards[page_index][i]));
    }
}

static void dirty_passthrough_activity_init_viewpage(dirty_passthrough_activity_t *activity, egui_core_t *core)
{
    egui_view_viewpage_init(EGUI_VIEW_OF(&activity->viewpage), core);
    egui_view_set_position(EGUI_VIEW_OF(&activity->viewpage), 0, DIRTY_PASSTHROUGH_ACTIVITY_NAV_HEIGHT);
    egui_view_viewpage_set_size(EGUI_VIEW_OF(&activity->viewpage), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_ACTIVITY_CONTENT_HEIGHT);
    egui_view_viewpage_set_scrollbar_enabled(EGUI_VIEW_OF(&activity->viewpage), 1);

    for (uint8_t i = 0; i < DIRTY_PASSTHROUGH_ACTIVITY_VIEWPAGE_PAGES; i++)
    {
        dirty_passthrough_activity_init_viewpage_card_group(activity, core, i);
        egui_view_viewpage_add_child(EGUI_VIEW_OF(&activity->viewpage), EGUI_VIEW_OF(&activity->viewpage_pages[i]));
    }
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&activity->viewpage));
}

static void dirty_passthrough_activity_init_mixed_page(dirty_passthrough_activity_t *activity, egui_core_t *core, uint8_t page_index)
{
    egui_view_scroll_init(EGUI_VIEW_OF(&activity->mixed_scrolls[page_index]), core);
    egui_view_set_size(EGUI_VIEW_OF(&activity->mixed_scrolls[page_index]), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_ACTIVITY_CONTENT_HEIGHT);
    egui_view_scroll_set_scrollbar_enabled(EGUI_VIEW_OF(&activity->mixed_scrolls[page_index]), 1);

    egui_view_linearlayout_init(EGUI_VIEW_OF(&activity->mixed_layouts[page_index]), core);
    egui_view_set_size(EGUI_VIEW_OF(&activity->mixed_layouts[page_index]), EGUI_CONFIG_SCREEN_WIDTH, 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&activity->mixed_layouts[page_index]), EGUI_ALIGN_HCENTER);
    egui_view_linearlayout_set_auto_height(EGUI_VIEW_OF(&activity->mixed_layouts[page_index]), 1);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&activity->mixed_layouts[page_index]), 1);

    for (int i = 0; i < DIRTY_PASSTHROUGH_ACTIVITY_MIXED_ROWS; i++)
    {
        dirty_passthrough_activity_init_label(core, &activity->mixed_cards[page_index][i], s_activity_mixed_texts[page_index][i], 0, 0, 142 + (i % 2) * 44, 58,
                                              EGUI_COLOR_WHITE, dirty_passthrough_activity_get_card_bg((int)activity->index + page_index + i));
        egui_view_group_add_child(EGUI_VIEW_OF(&activity->mixed_layouts[page_index]), EGUI_VIEW_OF(&activity->mixed_cards[page_index][i]));
    }

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&activity->mixed_layouts[page_index]));
    egui_view_scroll_add_child(EGUI_VIEW_OF(&activity->mixed_scrolls[page_index]), EGUI_VIEW_OF(&activity->mixed_layouts[page_index]));
    egui_view_scroll_layout_childs(EGUI_VIEW_OF(&activity->mixed_scrolls[page_index]));
}

static void dirty_passthrough_activity_init_mixed(dirty_passthrough_activity_t *activity, egui_core_t *core)
{
    egui_view_viewpage_init(EGUI_VIEW_OF(&activity->mixed_viewpage), core);
    egui_view_set_position(EGUI_VIEW_OF(&activity->mixed_viewpage), 0, DIRTY_PASSTHROUGH_ACTIVITY_NAV_HEIGHT);
    egui_view_viewpage_set_size(EGUI_VIEW_OF(&activity->mixed_viewpage), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_ACTIVITY_CONTENT_HEIGHT);
    egui_view_viewpage_set_scrollbar_enabled(EGUI_VIEW_OF(&activity->mixed_viewpage), 1);

    for (uint8_t i = 0; i < DIRTY_PASSTHROUGH_ACTIVITY_VIEWPAGE_PAGES; i++)
    {
        dirty_passthrough_activity_init_mixed_page(activity, core, i);
        egui_view_viewpage_add_child(EGUI_VIEW_OF(&activity->mixed_viewpage), EGUI_VIEW_OF(&activity->mixed_scrolls[i]));
    }
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&activity->mixed_viewpage));
}

static void dirty_passthrough_activity_on_create(egui_activity_t *self)
{
    dirty_passthrough_activity_t *activity = (dirty_passthrough_activity_t *)self;
    egui_core_t *core;

    egui_activity_on_create(self);
    core = egui_activity_get_core(self);

    egui_view_group_init(EGUI_VIEW_OF(&activity->header), core);
    egui_view_set_position(EGUI_VIEW_OF(&activity->header), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&activity->header), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_ACTIVITY_NAV_HEIGHT);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&activity->header), 1);

    dirty_passthrough_activity_init_label(core, &activity->title, activity->title_text, 74, 10, 92, 28, EGUI_COLOR_BLACK, NULL);
    dirty_passthrough_activity_init_button(core, &activity->finish_button, "Finish", 4, 10, dirty_passthrough_activity_finish_click);
    dirty_passthrough_activity_init_button(core, &activity->next_button, "Next", EGUI_CONFIG_SCREEN_WIDTH - 68, 10, dirty_passthrough_activity_next_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&activity->header), EGUI_VIEW_OF(&activity->finish_button));
    egui_view_group_add_child(EGUI_VIEW_OF(&activity->header), EGUI_VIEW_OF(&activity->title));
    egui_view_group_add_child(EGUI_VIEW_OF(&activity->header), EGUI_VIEW_OF(&activity->next_button));
    egui_activity_add_view(self, EGUI_VIEW_OF(&activity->header));

    switch (activity->index)
    {
    case 1:
        dirty_passthrough_activity_init_linear(activity, core);
        egui_activity_add_view(self, EGUI_VIEW_OF(&activity->linear));
        break;
    case 2:
        dirty_passthrough_activity_init_viewpage(activity, core);
        egui_activity_add_view(self, EGUI_VIEW_OF(&activity->viewpage));
        break;
    case 3:
        dirty_passthrough_activity_init_mixed(activity, core);
        egui_activity_add_view(self, EGUI_VIEW_OF(&activity->mixed_viewpage));
        break;
    default:
        dirty_passthrough_activity_init_sparse_scroll(activity, core);
        egui_activity_add_view(self, EGUI_VIEW_OF(&activity->scroll));
        break;
    }
}

static void dirty_passthrough_activity_on_destroy(egui_activity_t *self)
{
    egui_activity_on_destroy(self);
}

static const egui_activity_api_t EGUI_ACTIVITY_API_TABLE_NAME(dirty_passthrough_activity_t) = {
        .on_create = dirty_passthrough_activity_on_create,
        .on_start = egui_activity_on_start,
        .on_resume = egui_activity_on_resume,
        .on_pause = egui_activity_on_pause,
        .on_stop = egui_activity_on_stop,
        .on_destroy = dirty_passthrough_activity_on_destroy,
};

static void dirty_passthrough_activity_init_instance(dirty_passthrough_activity_t *activity, egui_core_t *core, uint8_t index)
{
    egui_activity_init((egui_activity_t *)activity, core);
    ((egui_activity_t *)activity)->api = &EGUI_ACTIVITY_API_TABLE_NAME(dirty_passthrough_activity_t);
    activity->index = index;
    egui_api_sprintf(activity->title_text, "Activity %d", index + 1);
    egui_activity_set_name((egui_activity_t *)activity, activity->title_text);
}

static void dirty_passthrough_activity_init_anims(egui_core_t *core)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_start_open));
    egui_animation_translate_params_set(&anim_start_open, &anim_start_open_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_start_open), 260);

    egui_animation_translate_init(EGUI_ANIM_OF(&anim_start_close));
    egui_animation_translate_params_set(&anim_start_close, &anim_start_close_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_start_close), 260);
    egui_animation_is_fill_before_set(EGUI_ANIM_OF(&anim_start_close), true);

    egui_animation_translate_init(EGUI_ANIM_OF(&anim_finish_open));
    egui_animation_translate_params_set(&anim_finish_open, &anim_finish_open_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_finish_open), 260);

    egui_animation_translate_init(EGUI_ANIM_OF(&anim_finish_close));
    egui_animation_translate_params_set(&anim_finish_close, &anim_finish_close_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_finish_close), 260);
    egui_animation_is_fill_before_set(EGUI_ANIM_OF(&anim_finish_close), true);
}

static void dirty_passthrough_activity_apply_anims(egui_core_t *core)
{
    egui_core_activity_set_start_anim(core, EGUI_ANIM_OF(&anim_start_open), EGUI_ANIM_OF(&anim_start_close));
    egui_core_activity_set_finish_anim(core, EGUI_ANIM_OF(&anim_finish_open), EGUI_ANIM_OF(&anim_finish_close));
}

void test_init_ui(egui_core_t *core)
{
    s_core = core;
    egui_background_color_init_with_params(EGUI_BG_OF(&bg_activity_button), &bg_activity_button_params);
    egui_view_set_background(EGUI_VIEW_OF(egui_core_get_user_root_view(core)), EGUI_BG_OF(&bg_activity_root_0));
    dirty_passthrough_activity_init_anims(core);
    dirty_passthrough_activity_start_next(NULL);
    dirty_passthrough_activity_apply_anims(core);
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static dirty_passthrough_activity_t *dirty_passthrough_activity_get_current_local(void)
{
    return (dirty_passthrough_activity_t *)dirty_passthrough_activity_get_current();
}

static void dirty_passthrough_activity_record_horizontal_swipe(egui_sim_action_t *p_action)
{
    p_action->type = EGUI_SIM_ACTION_SWIPE;
    p_action->x1 = EGUI_CONFIG_SCREEN_WIDTH * 3 / 4;
    p_action->y1 = DIRTY_PASSTHROUGH_ACTIVITY_NAV_HEIGHT + DIRTY_PASSTHROUGH_ACTIVITY_CONTENT_HEIGHT / 2;
    p_action->x2 = EGUI_CONFIG_SCREEN_WIDTH / 4;
    p_action->y2 = DIRTY_PASSTHROUGH_ACTIVITY_NAV_HEIGHT + DIRTY_PASSTHROUGH_ACTIVITY_CONTENT_HEIGHT / 2;
    p_action->steps = 8;
    p_action->interval_ms = 260;
}

static void dirty_passthrough_activity_record_vertical_swipe(egui_sim_action_t *p_action)
{
    p_action->type = EGUI_SIM_ACTION_SWIPE;
    p_action->x1 = EGUI_CONFIG_SCREEN_WIDTH / 2;
    p_action->y1 = EGUI_CONFIG_SCREEN_HEIGHT - 42;
    p_action->x2 = EGUI_CONFIG_SCREEN_WIDTH / 2;
    p_action->y2 = DIRTY_PASSTHROUGH_ACTIVITY_NAV_HEIGHT + 36;
    p_action->steps = 8;
    p_action->interval_ms = 260;
}

static int dirty_passthrough_activity_record_click_current_button(egui_sim_action_t *p_action, int is_next)
{
    dirty_passthrough_activity_t *activity = dirty_passthrough_activity_get_current_local();

    if (activity == NULL)
    {
        EGUI_SIM_SET_WAIT(p_action, 120);
        return 1;
    }

    EGUI_SIM_SET_CLICK_VIEW(p_action, is_next ? EGUI_VIEW_OF(&activity->next_button) : EGUI_VIEW_OF(&activity->finish_button), 280);
    return 1;
}

static int dirty_passthrough_activity_record_snapshot_wait(int first_call, egui_sim_action_t *p_action)
{
    if (first_call)
    {
        recording_request_snapshot();
    }
    EGUI_SIM_SET_WAIT(p_action, 360);
    return 1;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 340);
        return true;
    case 1:
    case 3:
    case 7:
    case 18:
        return dirty_passthrough_activity_record_click_current_button(p_action, 1);
    case 2:
    case 4:
    case 6:
    case 8:
    case 11:
    case 13:
    case 15:
    case 17:
    case 19:
        return dirty_passthrough_activity_record_snapshot_wait(first_call, p_action);
    case 5:
    case 9:
        dirty_passthrough_activity_record_horizontal_swipe(p_action);
        return true;
    case 10:
        dirty_passthrough_activity_record_vertical_swipe(p_action);
        return true;
    case 12:
    case 14:
    case 16:
        return dirty_passthrough_activity_record_click_current_button(p_action, 0);
    default:
        return false;
    }
}
#endif
