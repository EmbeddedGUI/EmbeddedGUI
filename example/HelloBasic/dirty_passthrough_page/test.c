#include "egui.h"
#include "uicode_disp0.h"

#define DIRTY_PASSTHROUGH_PAGE_COUNT          4
#define DIRTY_PASSTHROUGH_PAGE_SECTIONS       2
#define DIRTY_PASSTHROUGH_PAGE_CARDS          3
#define DIRTY_PASSTHROUGH_PAGE_VIEWPAGE_PAGES 2
#define DIRTY_PASSTHROUGH_PAGE_MIXED_ROWS     5
#define DIRTY_PASSTHROUGH_PAGE_NAV_HEIGHT     46
#define DIRTY_PASSTHROUGH_PAGE_CONTENT_HEIGHT (EGUI_CONFIG_SCREEN_HEIGHT - DIRTY_PASSTHROUGH_PAGE_NAV_HEIGHT)

typedef struct dirty_passthrough_page
{
    egui_page_base_t base;
    uint8_t index;

    egui_view_group_t header;
    egui_view_label_t title;
    egui_view_button_t prev_button;
    egui_view_button_t next_button;

    egui_view_scroll_t scroll;
    egui_view_group_t sections[DIRTY_PASSTHROUGH_PAGE_SECTIONS];
    egui_view_label_t cards[DIRTY_PASSTHROUGH_PAGE_SECTIONS][DIRTY_PASSTHROUGH_PAGE_CARDS];

    egui_view_linearlayout_t linear;
    egui_view_label_t linear_cards[DIRTY_PASSTHROUGH_PAGE_CARDS];

    egui_view_viewpage_t viewpage;
    egui_view_group_t viewpage_pages[DIRTY_PASSTHROUGH_PAGE_VIEWPAGE_PAGES];
    egui_view_label_t viewpage_cards[DIRTY_PASSTHROUGH_PAGE_VIEWPAGE_PAGES][DIRTY_PASSTHROUGH_PAGE_CARDS];

    egui_view_viewpage_t mixed_viewpage;
    egui_view_scroll_t mixed_scrolls[DIRTY_PASSTHROUGH_PAGE_VIEWPAGE_PAGES];
    egui_view_linearlayout_t mixed_layouts[DIRTY_PASSTHROUGH_PAGE_VIEWPAGE_PAGES];
    egui_view_label_t mixed_cards[DIRTY_PASSTHROUGH_PAGE_VIEWPAGE_PAGES][DIRTY_PASSTHROUGH_PAGE_MIXED_ROWS];
} dirty_passthrough_page_t;

static const egui_page_base_api_t EGUI_VIEW_API_TABLE_NAME(dirty_passthrough_page_t);

static egui_core_t *s_core;
static dirty_passthrough_page_t s_pages[DIRTY_PASSTHROUGH_PAGE_COUNT];
static egui_page_base_t *s_current_page;
static uint8_t s_page_index;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_page_root_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_page_root_params, &bg_page_root_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_root, &bg_page_root_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_page_primary_param, EGUI_THEME_PRIMARY, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_page_primary_params, &bg_page_primary_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_primary, &bg_page_primary_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_page_secondary_param, EGUI_THEME_SECONDARY, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_page_secondary_params, &bg_page_secondary_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_secondary, &bg_page_secondary_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_page_warning_param, EGUI_THEME_WARNING, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_page_warning_params, &bg_page_warning_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_warning, &bg_page_warning_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_page_button_param, EGUI_THEME_TRACK_BG_DARK, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_page_button_pressed_param, EGUI_THEME_PRIMARY_LIGHT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_page_button_params, &bg_page_button_param, &bg_page_button_pressed_param, NULL);
static egui_background_color_t bg_page_button;

static const char *s_page_titles[DIRTY_PASSTHROUGH_PAGE_COUNT] = {
        "Sparse List",
        "LinearLayout",
        "ViewPage",
        "Mixed Stack",
};

static const char *s_page_card_texts[DIRTY_PASSTHROUGH_PAGE_COUNT][DIRTY_PASSTHROUGH_PAGE_CARDS] = {
        {"Sparse A", "Tiny dirty", "Root stable"},
        {"Linear A", "Layout child", "No root wipe"},
        {"ViewPage A", "Swipe child", "Leaf dirty"},
        {"Mixed A", "Scroll rows", "Nested leaves"},
};

static const char *s_page_viewpage_texts[DIRTY_PASSTHROUGH_PAGE_VIEWPAGE_PAGES][DIRTY_PASSTHROUGH_PAGE_CARDS] = {
        {"VP page 1", "Sparse card", "Swipe left"},
        {"VP page 2", "Child bounds", "Swipe right"},
};

static const char *s_page_mixed_texts[DIRTY_PASSTHROUGH_PAGE_VIEWPAGE_PAGES][DIRTY_PASSTHROUGH_PAGE_MIXED_ROWS] = {
        {"Mix page 1", "Scroll row 1", "Linear row 2", "Nested row 3", "Leaf row 4"},
        {"Mix page 2", "Scroll row A", "Linear row B", "Nested row C", "Leaf row D"},
};

static egui_background_t *dirty_passthrough_page_get_card_bg(int index)
{
    switch (index % 3)
    {
    case 0:
        return EGUI_BG_OF(&bg_page_primary);
    case 1:
        return EGUI_BG_OF(&bg_page_secondary);
    default:
        return EGUI_BG_OF(&bg_page_warning);
    }
}

static void dirty_passthrough_page_init_label(egui_core_t *core, egui_view_label_t *label, const char *text, egui_dim_t x, egui_dim_t y, egui_dim_t width,
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

static void dirty_passthrough_page_init_button(egui_core_t *core, egui_view_button_t *button, const char *text, egui_dim_t x, egui_dim_t y,
                                               egui_view_on_click_listener_t listener)
{
    egui_view_button_init(EGUI_VIEW_OF(button), core);
    egui_view_set_position(EGUI_VIEW_OF(button), x, y);
    egui_view_set_size(EGUI_VIEW_OF(button), 62, 28);
    egui_view_label_set_text(EGUI_VIEW_OF(button), text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(button), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(button), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color(EGUI_VIEW_OF(button), EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(button), EGUI_BG_OF(&bg_page_button));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(button), listener);
}

static void dirty_passthrough_page_switch(uint8_t page_index)
{
    if (page_index >= DIRTY_PASSTHROUGH_PAGE_COUNT || s_core == NULL)
    {
        return;
    }

    if (s_current_page != NULL)
    {
        egui_page_base_close(s_current_page);
    }

    s_page_index = page_index;
    s_pages[page_index].index = page_index;
    egui_page_base_init((egui_page_base_t *)&s_pages[page_index], s_core);
    s_current_page = (egui_page_base_t *)&s_pages[page_index];
    s_current_page->api = &EGUI_VIEW_API_TABLE_NAME(dirty_passthrough_page_t);
    egui_page_base_open(s_current_page);
}

static void dirty_passthrough_page_prev_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    if (s_page_index > 0)
    {
        dirty_passthrough_page_switch((uint8_t)(s_page_index - 1));
    }
}

static void dirty_passthrough_page_next_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    if (s_page_index + 1U < DIRTY_PASSTHROUGH_PAGE_COUNT)
    {
        dirty_passthrough_page_switch((uint8_t)(s_page_index + 1U));
    }
}

static void dirty_passthrough_page_init_section(dirty_passthrough_page_t *page, egui_core_t *core, uint8_t section_index)
{
    static const egui_region_t card_regions[DIRTY_PASSTHROUGH_PAGE_CARDS] = {
            {{18, 18}, {118, 26}},
            {{80, 86}, {130, 26}},
            {{34, 154}, {150, 26}},
    };

    egui_view_group_init(EGUI_VIEW_OF(&page->sections[section_index]), core);
    egui_view_set_size(EGUI_VIEW_OF(&page->sections[section_index]), EGUI_CONFIG_SCREEN_WIDTH, 210);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&page->sections[section_index]), 1);

    for (int i = 0; i < DIRTY_PASSTHROUGH_PAGE_CARDS; i++)
    {
        int text_index = (i + section_index) % DIRTY_PASSTHROUGH_PAGE_CARDS;
        dirty_passthrough_page_init_label(core, &page->cards[section_index][i], s_page_card_texts[page->index][text_index], card_regions[i].location.x,
                                          card_regions[i].location.y, card_regions[i].size.width, card_regions[i].size.height, EGUI_COLOR_WHITE,
                                          dirty_passthrough_page_get_card_bg((int)page->index + i));
        egui_view_group_add_child(EGUI_VIEW_OF(&page->sections[section_index]), EGUI_VIEW_OF(&page->cards[section_index][i]));
    }
}

static void dirty_passthrough_page_init_sparse_scroll(dirty_passthrough_page_t *page, egui_core_t *core)
{
    egui_view_scroll_init(EGUI_VIEW_OF(&page->scroll), core);
    egui_view_set_position(EGUI_VIEW_OF(&page->scroll), 0, DIRTY_PASSTHROUGH_PAGE_NAV_HEIGHT);
    egui_view_set_size(EGUI_VIEW_OF(&page->scroll), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_PAGE_CONTENT_HEIGHT);
    egui_view_scroll_set_scrollbar_enabled(EGUI_VIEW_OF(&page->scroll), 1);

    for (uint8_t i = 0; i < DIRTY_PASSTHROUGH_PAGE_SECTIONS; i++)
    {
        dirty_passthrough_page_init_section(page, core, i);
        egui_view_scroll_add_child(EGUI_VIEW_OF(&page->scroll), EGUI_VIEW_OF(&page->sections[i]));
    }
    egui_view_scroll_layout_childs(EGUI_VIEW_OF(&page->scroll));
}

static void dirty_passthrough_page_init_linear(dirty_passthrough_page_t *page, egui_core_t *core)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&page->linear), core);
    egui_view_set_position(EGUI_VIEW_OF(&page->linear), 0, DIRTY_PASSTHROUGH_PAGE_NAV_HEIGHT);
    egui_view_set_size(EGUI_VIEW_OF(&page->linear), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_PAGE_CONTENT_HEIGHT);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&page->linear), EGUI_ALIGN_CENTER);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&page->linear), 1);

    for (int i = 0; i < DIRTY_PASSTHROUGH_PAGE_CARDS; i++)
    {
        dirty_passthrough_page_init_label(core, &page->linear_cards[i], s_page_card_texts[page->index][i], 0, 0, 156 + i * 18, 38, EGUI_COLOR_WHITE,
                                          dirty_passthrough_page_get_card_bg((int)page->index + i));
        egui_view_group_add_child(EGUI_VIEW_OF(&page->linear), EGUI_VIEW_OF(&page->linear_cards[i]));
    }
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&page->linear));
}

static void dirty_passthrough_page_init_viewpage_card_group(dirty_passthrough_page_t *page, egui_core_t *core, uint8_t page_index)
{
    static const egui_region_t card_regions[DIRTY_PASSTHROUGH_PAGE_CARDS] = {
            {{24, 24}, {138, 28}},
            {{78, 102}, {130, 28}},
            {{40, 182}, {148, 28}},
    };

    egui_view_group_init(EGUI_VIEW_OF(&page->viewpage_pages[page_index]), core);
    egui_view_set_size(EGUI_VIEW_OF(&page->viewpage_pages[page_index]), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_PAGE_CONTENT_HEIGHT);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&page->viewpage_pages[page_index]), 1);

    for (int i = 0; i < DIRTY_PASSTHROUGH_PAGE_CARDS; i++)
    {
        dirty_passthrough_page_init_label(core, &page->viewpage_cards[page_index][i], s_page_viewpage_texts[page_index][i], card_regions[i].location.x,
                                          card_regions[i].location.y, card_regions[i].size.width, card_regions[i].size.height, EGUI_COLOR_WHITE,
                                          dirty_passthrough_page_get_card_bg((int)page->index + page_index + i));
        egui_view_group_add_child(EGUI_VIEW_OF(&page->viewpage_pages[page_index]), EGUI_VIEW_OF(&page->viewpage_cards[page_index][i]));
    }
}

static void dirty_passthrough_page_init_viewpage(dirty_passthrough_page_t *page, egui_core_t *core)
{
    egui_view_viewpage_init(EGUI_VIEW_OF(&page->viewpage), core);
    egui_view_set_position(EGUI_VIEW_OF(&page->viewpage), 0, DIRTY_PASSTHROUGH_PAGE_NAV_HEIGHT);
    egui_view_viewpage_set_size(EGUI_VIEW_OF(&page->viewpage), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_PAGE_CONTENT_HEIGHT);
    egui_view_viewpage_set_scrollbar_enabled(EGUI_VIEW_OF(&page->viewpage), 1);

    for (uint8_t i = 0; i < DIRTY_PASSTHROUGH_PAGE_VIEWPAGE_PAGES; i++)
    {
        dirty_passthrough_page_init_viewpage_card_group(page, core, i);
        egui_view_viewpage_add_child(EGUI_VIEW_OF(&page->viewpage), EGUI_VIEW_OF(&page->viewpage_pages[i]));
    }
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&page->viewpage));
}

static void dirty_passthrough_page_init_mixed_page(dirty_passthrough_page_t *page, egui_core_t *core, uint8_t page_index)
{
    egui_view_scroll_init(EGUI_VIEW_OF(&page->mixed_scrolls[page_index]), core);
    egui_view_set_size(EGUI_VIEW_OF(&page->mixed_scrolls[page_index]), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_PAGE_CONTENT_HEIGHT);
    egui_view_scroll_set_scrollbar_enabled(EGUI_VIEW_OF(&page->mixed_scrolls[page_index]), 1);

    egui_view_linearlayout_init(EGUI_VIEW_OF(&page->mixed_layouts[page_index]), core);
    egui_view_set_size(EGUI_VIEW_OF(&page->mixed_layouts[page_index]), EGUI_CONFIG_SCREEN_WIDTH, 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&page->mixed_layouts[page_index]), EGUI_ALIGN_HCENTER);
    egui_view_linearlayout_set_auto_height(EGUI_VIEW_OF(&page->mixed_layouts[page_index]), 1);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&page->mixed_layouts[page_index]), 1);

    for (int i = 0; i < DIRTY_PASSTHROUGH_PAGE_MIXED_ROWS; i++)
    {
        dirty_passthrough_page_init_label(core, &page->mixed_cards[page_index][i], s_page_mixed_texts[page_index][i], 0, 0, 142 + (i % 2) * 44, 58,
                                          EGUI_COLOR_WHITE, dirty_passthrough_page_get_card_bg((int)page->index + page_index + i));
        egui_view_group_add_child(EGUI_VIEW_OF(&page->mixed_layouts[page_index]), EGUI_VIEW_OF(&page->mixed_cards[page_index][i]));
    }

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&page->mixed_layouts[page_index]));
    egui_view_scroll_add_child(EGUI_VIEW_OF(&page->mixed_scrolls[page_index]), EGUI_VIEW_OF(&page->mixed_layouts[page_index]));
    egui_view_scroll_layout_childs(EGUI_VIEW_OF(&page->mixed_scrolls[page_index]));
}

static void dirty_passthrough_page_init_mixed(dirty_passthrough_page_t *page, egui_core_t *core)
{
    egui_view_viewpage_init(EGUI_VIEW_OF(&page->mixed_viewpage), core);
    egui_view_set_position(EGUI_VIEW_OF(&page->mixed_viewpage), 0, DIRTY_PASSTHROUGH_PAGE_NAV_HEIGHT);
    egui_view_viewpage_set_size(EGUI_VIEW_OF(&page->mixed_viewpage), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_PAGE_CONTENT_HEIGHT);
    egui_view_viewpage_set_scrollbar_enabled(EGUI_VIEW_OF(&page->mixed_viewpage), 1);

    for (uint8_t i = 0; i < DIRTY_PASSTHROUGH_PAGE_VIEWPAGE_PAGES; i++)
    {
        dirty_passthrough_page_init_mixed_page(page, core, i);
        egui_view_viewpage_add_child(EGUI_VIEW_OF(&page->mixed_viewpage), EGUI_VIEW_OF(&page->mixed_scrolls[i]));
    }
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&page->mixed_viewpage));
}

static void dirty_passthrough_page_on_open(egui_page_base_t *self)
{
    dirty_passthrough_page_t *page = (dirty_passthrough_page_t *)self;
    egui_core_t *core;

    egui_page_base_on_open(self);
    core = egui_page_base_get_core(self);

    egui_view_group_init(EGUI_VIEW_OF(&page->header), core);
    egui_view_set_position(EGUI_VIEW_OF(&page->header), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&page->header), EGUI_CONFIG_SCREEN_WIDTH, DIRTY_PASSTHROUGH_PAGE_NAV_HEIGHT);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&page->header), 1);

    dirty_passthrough_page_init_label(core, &page->title, s_page_titles[page->index], 70, 9, 100, 28, EGUI_COLOR_BLACK, NULL);
    dirty_passthrough_page_init_button(core, &page->prev_button, "Prev", 4, 9, dirty_passthrough_page_prev_click);
    dirty_passthrough_page_init_button(core, &page->next_button, "Next", EGUI_CONFIG_SCREEN_WIDTH - 66, 9, dirty_passthrough_page_next_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&page->header), EGUI_VIEW_OF(&page->prev_button));
    egui_view_group_add_child(EGUI_VIEW_OF(&page->header), EGUI_VIEW_OF(&page->title));
    egui_view_group_add_child(EGUI_VIEW_OF(&page->header), EGUI_VIEW_OF(&page->next_button));
    egui_page_base_add_view(self, EGUI_VIEW_OF(&page->header));

    switch (page->index)
    {
    case 1:
        dirty_passthrough_page_init_linear(page, core);
        egui_page_base_add_view(self, EGUI_VIEW_OF(&page->linear));
        break;
    case 2:
        dirty_passthrough_page_init_viewpage(page, core);
        egui_page_base_add_view(self, EGUI_VIEW_OF(&page->viewpage));
        break;
    case 3:
        dirty_passthrough_page_init_mixed(page, core);
        egui_page_base_add_view(self, EGUI_VIEW_OF(&page->mixed_viewpage));
        break;
    default:
        dirty_passthrough_page_init_sparse_scroll(page, core);
        egui_page_base_add_view(self, EGUI_VIEW_OF(&page->scroll));
        break;
    }
}

static void dirty_passthrough_page_on_close(egui_page_base_t *self)
{
    egui_page_base_on_close(self);
}

static const egui_page_base_api_t EGUI_VIEW_API_TABLE_NAME(dirty_passthrough_page_t) = {
        .on_open = dirty_passthrough_page_on_open,
        .on_close = dirty_passthrough_page_on_close,
        .on_key_pressed = egui_page_base_on_key_pressed,
};

void test_init_ui(egui_core_t *core)
{
    s_core = core;
    s_current_page = NULL;
    s_page_index = 0;
    egui_background_color_init_with_params(EGUI_BG_OF(&bg_page_button), &bg_page_button_params);
    egui_view_set_background(EGUI_VIEW_OF(egui_core_get_user_root_view(core)), EGUI_BG_OF(&bg_page_root));

    for (uint8_t i = 0; i < DIRTY_PASSTHROUGH_PAGE_COUNT; i++)
    {
        s_pages[i].index = i;
    }

    dirty_passthrough_page_switch(0);
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static dirty_passthrough_page_t *dirty_passthrough_page_get_current(void)
{
    if (s_current_page == NULL)
    {
        return NULL;
    }
    return (dirty_passthrough_page_t *)s_current_page;
}

static void dirty_passthrough_page_record_horizontal_swipe(egui_sim_action_t *p_action)
{
    p_action->type = EGUI_SIM_ACTION_SWIPE;
    p_action->x1 = EGUI_CONFIG_SCREEN_WIDTH * 3 / 4;
    p_action->y1 = DIRTY_PASSTHROUGH_PAGE_NAV_HEIGHT + DIRTY_PASSTHROUGH_PAGE_CONTENT_HEIGHT / 2;
    p_action->x2 = EGUI_CONFIG_SCREEN_WIDTH / 4;
    p_action->y2 = DIRTY_PASSTHROUGH_PAGE_NAV_HEIGHT + DIRTY_PASSTHROUGH_PAGE_CONTENT_HEIGHT / 2;
    p_action->steps = 8;
    p_action->interval_ms = 260;
}

static void dirty_passthrough_page_record_vertical_swipe(egui_sim_action_t *p_action)
{
    p_action->type = EGUI_SIM_ACTION_SWIPE;
    p_action->x1 = EGUI_CONFIG_SCREEN_WIDTH / 2;
    p_action->y1 = EGUI_CONFIG_SCREEN_HEIGHT - 42;
    p_action->x2 = EGUI_CONFIG_SCREEN_WIDTH / 2;
    p_action->y2 = DIRTY_PASSTHROUGH_PAGE_NAV_HEIGHT + 36;
    p_action->steps = 8;
    p_action->interval_ms = 260;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    dirty_passthrough_page_t *page = dirty_passthrough_page_get_current();
    static int last_action = -1;
    int first_call = (action_index != last_action);

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
    case 3:
    case 7:
        page = dirty_passthrough_page_get_current();
        if (page != NULL)
        {
            EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&page->next_button), 260);
        }
        return true;
    case 2:
    case 4:
    case 6:
    case 8:
    case 11:
    case 13:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 5:
    case 9:
        dirty_passthrough_page_record_horizontal_swipe(p_action);
        return true;
    case 10:
        dirty_passthrough_page_record_vertical_swipe(p_action);
        return true;
    case 12:
        page = dirty_passthrough_page_get_current();
        if (page != NULL)
        {
            EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&page->prev_button), 260);
        }
        return true;
    default:
        return false;
    }
}
#endif
