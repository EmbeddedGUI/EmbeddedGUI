#include "egui.h"
#include "uicode_disp0.h"

#ifndef DIRTY_PASSTHROUGH_CONTAINER_DEMO_BASELINE
#define DIRTY_PASSTHROUGH_CONTAINER_DEMO_BASELINE 0
#endif

#define SPARSE_SECTION_COUNT 2
#define SPARSE_CARD_COUNT    2
#define PAGER_PAGE_COUNT     3
#define MIXED_CHIP_COUNT     2

#define SPARSE_SECTION_HEIGHT 220
#define PAGER_SECTION_HEIGHT  220
#define MIXED_SECTION_HEIGHT  250

static egui_view_t backdrop;
static egui_view_scroll_t scroll_view;

static egui_view_group_t sparse_sections[SPARSE_SECTION_COUNT];
static egui_view_label_t sparse_cards[SPARSE_SECTION_COUNT][SPARSE_CARD_COUNT];

static egui_view_group_t pager_section;
static egui_view_group_t pager_pages[PAGER_PAGE_COUNT];
static egui_view_label_t pager_cards[PAGER_PAGE_COUNT];

static egui_view_group_t mixed_section;
static egui_view_group_t mixed_visual_panel;
static egui_view_label_t mixed_visual_labels[2];
static egui_view_group_t mixed_structural_group;
static egui_view_label_t mixed_chips[MIXED_CHIP_COUNT];

static const char *sparse_texts[SPARSE_SECTION_COUNT][SPARSE_CARD_COUNT] = {
        {"Sparse list", "Small dirty"},
        {"View root", "Only chips"},
};

static const char *pager_texts[PAGER_PAGE_COUNT] = {
        "Page A",
        "Page B",
        "Page C",
};

static const char *mixed_chip_texts[MIXED_CHIP_COUNT] = {
        "Swept chip",
        "Tiny chip",
};

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_screen_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_screen_params, &bg_screen_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_screen, &bg_screen_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_card_0_param, EGUI_THEME_PRIMARY, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_card_0_params, &bg_card_0_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_card_0, &bg_card_0_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_card_1_param, EGUI_THEME_SECONDARY, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_card_1_params, &bg_card_1_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_card_1, &bg_card_1_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_card_2_param, EGUI_THEME_WARNING, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_card_2_params, &bg_card_2_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_card_2, &bg_card_2_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_panel_param, EGUI_THEME_TRACK_BG, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_panel_params, &bg_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_panel, &bg_panel_params);

EGUI_VIEW_SCROLL_PARAMS_INIT(scroll_view_params, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);

static void set_structural_dirty_passthrough(egui_view_t *view)
{
#if DIRTY_PASSTHROUGH_CONTAINER_DEMO_BASELINE
    egui_view_set_dirty_passthrough(view, 0);
#else
    egui_view_set_dirty_passthrough(view, 1);
#endif
}

static egui_background_t *get_card_background(int index)
{
    switch (index % 3)
    {
    case 0:
        return EGUI_BG_OF(&bg_card_0);
    case 1:
        return EGUI_BG_OF(&bg_card_1);
    default:
        return EGUI_BG_OF(&bg_card_2);
    }
}

static void init_label(egui_core_t *core, egui_view_label_t *label, const char *text, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height,
                       egui_color_t font_color, egui_background_t *background)
{
    egui_view_label_init(EGUI_VIEW_OF(label), core);
    egui_view_label_set_font(EGUI_VIEW_OF(label), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), font_color, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), EGUI_ALIGN_CENTER);
    egui_view_label_set_text(EGUI_VIEW_OF(label), text);
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), width, height);
    if (background != NULL)
    {
        egui_view_set_background(EGUI_VIEW_OF(label), background);
    }
}

static void init_sparse_section(egui_core_t *core, int section_index)
{
    static const egui_region_t card_regions[SPARSE_CARD_COUNT] = {
            {{18, 22}, {124, 26}},
            {{60, 150}, {150, 24}},
    };

    egui_view_group_init(EGUI_VIEW_OF(&sparse_sections[section_index]), core);
    egui_view_set_size(EGUI_VIEW_OF(&sparse_sections[section_index]), EGUI_CONFIG_SCREEN_WIDTH, SPARSE_SECTION_HEIGHT);
    set_structural_dirty_passthrough(EGUI_VIEW_OF(&sparse_sections[section_index]));

    for (int i = 0; i < SPARSE_CARD_COUNT; i++)
    {
        init_label(core, &sparse_cards[section_index][i], sparse_texts[section_index][i], card_regions[i].location.x, card_regions[i].location.y,
                   card_regions[i].size.width, card_regions[i].size.height, EGUI_COLOR_WHITE, get_card_background(i));
        egui_view_group_add_child(EGUI_VIEW_OF(&sparse_sections[section_index]), EGUI_VIEW_OF(&sparse_cards[section_index][i]));
    }
}

static void init_pager_section(egui_core_t *core)
{
    egui_view_group_init(EGUI_VIEW_OF(&pager_section), core);
    egui_view_set_size(EGUI_VIEW_OF(&pager_section), EGUI_CONFIG_SCREEN_WIDTH, PAGER_SECTION_HEIGHT);
    set_structural_dirty_passthrough(EGUI_VIEW_OF(&pager_section));

    for (int i = 0; i < PAGER_PAGE_COUNT; i++)
    {
        egui_dim_t page_x = (egui_dim_t)(8 + i * 76);

        egui_view_group_init(EGUI_VIEW_OF(&pager_pages[i]), core);
        egui_view_set_position(EGUI_VIEW_OF(&pager_pages[i]), page_x, 28);
        egui_view_set_size(EGUI_VIEW_OF(&pager_pages[i]), 70, 150);
        set_structural_dirty_passthrough(EGUI_VIEW_OF(&pager_pages[i]));

        init_label(core, &pager_cards[i], pager_texts[i], 6, (egui_dim_t)(18 + i * 38), 58, 34, EGUI_COLOR_WHITE, get_card_background(i));
        egui_view_group_add_child(EGUI_VIEW_OF(&pager_pages[i]), EGUI_VIEW_OF(&pager_cards[i]));
        egui_view_group_add_child(EGUI_VIEW_OF(&pager_section), EGUI_VIEW_OF(&pager_pages[i]));
    }
}

static void init_mixed_section(egui_core_t *core)
{
    egui_view_group_init(EGUI_VIEW_OF(&mixed_section), core);
    egui_view_set_size(EGUI_VIEW_OF(&mixed_section), EGUI_CONFIG_SCREEN_WIDTH, MIXED_SECTION_HEIGHT);
    set_structural_dirty_passthrough(EGUI_VIEW_OF(&mixed_section));

    egui_view_group_init(EGUI_VIEW_OF(&mixed_visual_panel), core);
    egui_view_set_position(EGUI_VIEW_OF(&mixed_visual_panel), 18, 18);
    egui_view_set_size(EGUI_VIEW_OF(&mixed_visual_panel), 204, 82);
    egui_view_set_background(EGUI_VIEW_OF(&mixed_visual_panel), EGUI_BG_OF(&bg_panel));

    init_label(core, &mixed_visual_labels[0], "Mixed visual", 12, 12, 180, 24, EGUI_COLOR_BLACK, NULL);
    init_label(core, &mixed_visual_labels[1], "panel stays full", 24, 46, 156, 22, EGUI_COLOR_BLACK, NULL);
    egui_view_group_add_child(EGUI_VIEW_OF(&mixed_visual_panel), EGUI_VIEW_OF(&mixed_visual_labels[0]));
    egui_view_group_add_child(EGUI_VIEW_OF(&mixed_visual_panel), EGUI_VIEW_OF(&mixed_visual_labels[1]));
    egui_view_group_add_child(EGUI_VIEW_OF(&mixed_section), EGUI_VIEW_OF(&mixed_visual_panel));

    egui_view_group_init(EGUI_VIEW_OF(&mixed_structural_group), core);
    egui_view_set_position(EGUI_VIEW_OF(&mixed_structural_group), 0, 118);
    egui_view_set_size(EGUI_VIEW_OF(&mixed_structural_group), EGUI_CONFIG_SCREEN_WIDTH, 120);
    set_structural_dirty_passthrough(EGUI_VIEW_OF(&mixed_structural_group));

    init_label(core, &mixed_chips[0], mixed_chip_texts[0], 24, 12, 112, 24, EGUI_COLOR_WHITE, EGUI_BG_OF(&bg_card_1));
    init_label(core, &mixed_chips[1], mixed_chip_texts[1], 92, 78, 104, 24, EGUI_COLOR_WHITE, EGUI_BG_OF(&bg_card_2));
    for (int i = 0; i < MIXED_CHIP_COUNT; i++)
    {
        egui_view_group_add_child(EGUI_VIEW_OF(&mixed_structural_group), EGUI_VIEW_OF(&mixed_chips[i]));
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&mixed_section), EGUI_VIEW_OF(&mixed_structural_group));
}

void test_init_ui(egui_core_t *core)
{
    egui_view_init(&backdrop, core);
    egui_view_set_size(&backdrop, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
    egui_view_set_background(&backdrop, EGUI_BG_OF(&bg_screen));

    egui_view_scroll_init_with_params(EGUI_VIEW_OF(&scroll_view), core, &scroll_view_params);
    egui_view_scroll_set_scrollbar_enabled(EGUI_VIEW_OF(&scroll_view), 1);

#if DIRTY_PASSTHROUGH_CONTAINER_DEMO_BASELINE
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&scroll_view), 0);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&scroll_view.container), 0);
#endif

    init_sparse_section(core, 0);
    init_pager_section(core);
    init_mixed_section(core);
    init_sparse_section(core, 1);

    egui_view_scroll_add_child(EGUI_VIEW_OF(&scroll_view), EGUI_VIEW_OF(&sparse_sections[0]));
    egui_view_scroll_add_child(EGUI_VIEW_OF(&scroll_view), EGUI_VIEW_OF(&pager_section));
    egui_view_scroll_add_child(EGUI_VIEW_OF(&scroll_view), EGUI_VIEW_OF(&mixed_section));
    egui_view_scroll_add_child(EGUI_VIEW_OF(&scroll_view), EGUI_VIEW_OF(&sparse_sections[1]));
    egui_view_scroll_layout_childs(EGUI_VIEW_OF(&scroll_view));

    egui_core_add_user_root_view(&backdrop);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&scroll_view));
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCREEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCREEN_HEIGHT * 3 / 4;
        p_action->x2 = EGUI_CONFIG_SCREEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCREEN_HEIGHT / 4;
        p_action->steps = 10;
        p_action->interval_ms = 450;
        return true;
    case 1:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCREEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCREEN_HEIGHT * 3 / 4;
        p_action->x2 = EGUI_CONFIG_SCREEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCREEN_HEIGHT / 4;
        p_action->steps = 10;
        p_action->interval_ms = 450;
        return true;
    case 2:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCREEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCREEN_HEIGHT / 4;
        p_action->x2 = EGUI_CONFIG_SCREEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCREEN_HEIGHT * 3 / 4;
        p_action->steps = 10;
        p_action->interval_ms = 450;
        return true;
    default:
        return false;
    }
}
#endif
