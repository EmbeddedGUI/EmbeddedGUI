#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_auto_suggest_box.h"

#include "../../HelloCustomWidgets/input/auto_suggest_box/egui_view_auto_suggest_box.h"
#include "../../HelloCustomWidgets/input/auto_suggest_box/egui_view_auto_suggest_box.c"

static egui_view_autocomplete_t test_box;
static uint8_t g_selected_count;
static uint8_t g_last_selected;

static const char *g_people[] = {"Alice Chen", "Alicia Gomez", "Allen Park", "Amelia Stone"};
static const char *g_people_icons[] = {"A", "B", "C", "D"};
static const char *g_commands[] = {"Deploy API", "Deploy Docs"};

static void on_selected(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    g_selected_count++;
    g_last_selected = index;
}

static void reset_listener_state(void)
{
    g_selected_count = 0;
    g_last_selected = 0xFF;
}

static void setup_box(void)
{
    egui_view_autocomplete_init(EGUI_VIEW_OF(&test_box));
    egui_view_set_size(EGUI_VIEW_OF(&test_box), 180, 34);
    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&test_box), g_people, 4);
    egui_view_autocomplete_set_on_selected_listener(EGUI_VIEW_OF(&test_box), on_selected);
    hcw_auto_suggest_box_apply_standard_style(EGUI_VIEW_OF(&test_box));
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&test_box), 1);
#endif
    reset_listener_state();
}

static void layout_box(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_box), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_box)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_box)->api->on_touch_event(EGUI_VIEW_OF(&test_box), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_box)->api->dispatch_key_event(EGUI_VIEW_OF(&test_box), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_box)->api->dispatch_key_event(EGUI_VIEW_OF(&test_box), &event);
    return handled;
}

static void get_view_center(egui_dim_t *x, egui_dim_t *y)
{
    *x = EGUI_VIEW_OF(&test_box)->region_screen.location.x + EGUI_VIEW_OF(&test_box)->region_screen.size.width / 2;
    *y = EGUI_VIEW_OF(&test_box)->region_screen.location.y + ((egui_view_combobox_t *)EGUI_VIEW_OF(&test_box))->collapsed_height / 2;
}

static uint8_t get_visible_count(void)
{
    egui_view_combobox_t *local = (egui_view_combobox_t *)EGUI_VIEW_OF(&test_box);
    uint8_t visible_count = local->item_count;
    egui_dim_t item_space;
    uint8_t fit_count;

    if (visible_count > local->max_visible_items)
    {
        visible_count = local->max_visible_items;
    }
    if (!local->is_expanded || visible_count == 0 || local->item_height <= 0 || EGUI_VIEW_OF(&test_box)->region_screen.size.height <= local->collapsed_height)
    {
        return visible_count;
    }

    item_space = EGUI_VIEW_OF(&test_box)->region_screen.size.height - local->collapsed_height;
    fit_count = (uint8_t)(item_space / local->item_height);
    if (fit_count < visible_count)
    {
        visible_count = fit_count;
    }

    return visible_count;
}

static uint8_t get_visible_start_index(uint8_t visible_count)
{
    egui_view_combobox_t *local = (egui_view_combobox_t *)EGUI_VIEW_OF(&test_box);
    uint8_t start_index = 0;

    if (visible_count == 0 || local->item_count <= visible_count)
    {
        return 0;
    }
    if (local->current_index >= visible_count)
    {
        start_index = (uint8_t)(local->current_index + 1 - visible_count);
    }
    if ((uint16_t)start_index + visible_count > local->item_count)
    {
        start_index = (uint8_t)(local->item_count - visible_count);
    }

    return start_index;
}

static void get_dropdown_item_center(uint8_t item_index, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_combobox_t *local = (egui_view_combobox_t *)EGUI_VIEW_OF(&test_box);
    uint8_t visible_count = get_visible_count();
    uint8_t start_index = get_visible_start_index(visible_count);
    uint8_t row_index;

    EGUI_TEST_ASSERT_TRUE(local->is_expanded);
    EGUI_TEST_ASSERT_TRUE(visible_count > 0);

    if (item_index < start_index)
    {
        row_index = 0;
    }
    else if (item_index >= (uint8_t)(start_index + visible_count))
    {
        row_index = (uint8_t)(visible_count - 1);
    }
    else
    {
        row_index = (uint8_t)(item_index - start_index);
    }

    *x = EGUI_VIEW_OF(&test_box)->region_screen.location.x + EGUI_VIEW_OF(&test_box)->region_screen.size.width / 2;
    *y = EGUI_VIEW_OF(&test_box)->region_screen.location.y + local->collapsed_height + row_index * local->item_height + local->item_height / 2;
}

static void test_auto_suggest_box_suggestions_current_index_and_text(void)
{
    setup_box();

    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_autocomplete_get_suggestion_count(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_TRUE(strcmp("Alice Chen", egui_view_autocomplete_get_current_text(EGUI_VIEW_OF(&test_box))) == 0);

    egui_view_autocomplete_set_current_index(EGUI_VIEW_OF(&test_box), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_TRUE(strcmp("Allen Park", egui_view_autocomplete_get_current_text(EGUI_VIEW_OF(&test_box))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_selected_count);

    egui_view_autocomplete_set_current_index(EGUI_VIEW_OF(&test_box), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&test_box)));

    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&test_box), g_commands, 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_autocomplete_get_suggestion_count(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_TRUE(strcmp("Deploy API", egui_view_autocomplete_get_current_text(EGUI_VIEW_OF(&test_box))) == 0);

    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&test_box), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_autocomplete_get_suggestion_count(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_NULL(egui_view_autocomplete_get_current_text(EGUI_VIEW_OF(&test_box)));
}

static void test_auto_suggest_box_style_helpers_and_params(void)
{
    egui_view_autocomplete_t params_box;
    egui_view_combobox_t *local = (egui_view_combobox_t *)EGUI_VIEW_OF(&test_box);
    const egui_font_t *font_before;
    egui_view_autocomplete_params_t params = {
            .region = {{1, 2}, {96, 28}},
            .suggestions = g_commands,
            .suggestion_count = 2,
            .current_index = 1,
    };
    egui_view_autocomplete_params_t init_params = {
            .region = {{4, 5}, {120, 30}},
            .suggestions = g_commands,
            .suggestion_count = 2,
            .current_index = 9,
    };

    setup_box();

    hcw_auto_suggest_box_apply_standard_style(EGUI_VIEW_OF(&test_box));
    EGUI_TEST_ASSERT_EQUAL_INT(34, local->collapsed_height);
    EGUI_TEST_ASSERT_EQUAL_INT(24, local->item_height);
    EGUI_TEST_ASSERT_EQUAL_INT(4, local->max_visible_items);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xD7DEE7).full, local->border_color.full);

    hcw_auto_suggest_box_apply_compact_style(EGUI_VIEW_OF(&test_box));
    EGUI_TEST_ASSERT_EQUAL_INT(28, local->collapsed_height);
    EGUI_TEST_ASSERT_EQUAL_INT(21, local->item_height);
    EGUI_TEST_ASSERT_EQUAL_INT(3, local->max_visible_items);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xCCFBF1).full, local->highlight_color.full);

    hcw_auto_suggest_box_apply_read_only_style(EGUI_VIEW_OF(&test_box));
    EGUI_TEST_ASSERT_EQUAL_INT(28, local->collapsed_height);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xFBFCFD).full, local->bg_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x8996A4).full, local->arrow_color.full);

    font_before = local->font;
    egui_view_autocomplete_set_font(EGUI_VIEW_OF(&test_box), NULL);
    EGUI_TEST_ASSERT_TRUE(local->font == font_before);
    egui_view_autocomplete_set_font(EGUI_VIEW_OF(&test_box), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(local->font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_autocomplete_set_max_visible_items(EGUI_VIEW_OF(&test_box), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, local->max_visible_items);
    egui_view_combobox_set_item_icons(EGUI_VIEW_OF(&test_box), g_people_icons);
    EGUI_TEST_ASSERT_TRUE(local->item_icons == g_people_icons);
    egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&test_box), EGUI_FONT_ICON_MS_16);
    EGUI_TEST_ASSERT_TRUE(local->icon_font == EGUI_FONT_ICON_MS_16);
    egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&test_box), ">", "<");
    EGUI_TEST_ASSERT_TRUE(strcmp(">", local->expand_icon) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("<", local->collapse_icon) == 0);
    egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&test_box), NULL, NULL);
    egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&test_box), EGUI_FONT_ICON_MS_16);
    EGUI_TEST_ASSERT_TRUE(strcmp(local->expand_icon, EGUI_ICON_MS_EXPAND_MORE) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp(local->collapse_icon, EGUI_ICON_MS_EXPAND_LESS) == 0);
    egui_view_combobox_set_icon_text_gap(EGUI_VIEW_OF(&test_box), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, local->icon_text_gap);

    egui_view_autocomplete_apply_params(EGUI_VIEW_OF(&test_box), &params);
    EGUI_TEST_ASSERT_EQUAL_INT(1, EGUI_VIEW_OF(&test_box)->region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(2, EGUI_VIEW_OF(&test_box)->region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(96, EGUI_VIEW_OF(&test_box)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(28, EGUI_VIEW_OF(&test_box)->region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(28, local->collapsed_height);
    EGUI_TEST_ASSERT_EQUAL_INT(2, local->item_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, local->current_index);
    EGUI_TEST_ASSERT_TRUE(strcmp("Deploy Docs", egui_view_autocomplete_get_current_text(EGUI_VIEW_OF(&test_box))) == 0);

    egui_view_autocomplete_init_with_params(EGUI_VIEW_OF(&params_box), &init_params);
    EGUI_TEST_ASSERT_EQUAL_INT(4, EGUI_VIEW_OF(&params_box)->region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(5, EGUI_VIEW_OF(&params_box)->region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(120, EGUI_VIEW_OF(&params_box)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(30, EGUI_VIEW_OF(&params_box)->region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(30, params_box.combobox.collapsed_height);
    EGUI_TEST_ASSERT_EQUAL_INT(2, params_box.combobox.item_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, params_box.combobox.current_index);
    EGUI_TEST_ASSERT_TRUE(strcmp("Deploy API", egui_view_autocomplete_get_current_text(EGUI_VIEW_OF(&params_box))) == 0);
}

static void test_auto_suggest_box_touch_expand_select_and_fit_height(void)
{
    egui_view_combobox_t *local = (egui_view_combobox_t *)EGUI_VIEW_OF(&test_box);
    egui_dim_t x;
    egui_dim_t y;

    setup_box();
    layout_box(10, 20, 180, 34);
    get_view_center(&x, &y);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(34 + 4 * 24, EGUI_VIEW_OF(&test_box)->region_screen.size.height);

    get_dropdown_item_center(2, &x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_FALSE(egui_view_autocomplete_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selected_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_last_selected);
    EGUI_TEST_ASSERT_EQUAL_INT(local->collapsed_height, EGUI_VIEW_OF(&test_box)->region_screen.size.height);

    setup_box();
    layout_box(10, EGUI_CONFIG_SCEEN_HEIGHT - 80, 180, 34);
    get_view_center(&x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(34 + 24, EGUI_VIEW_OF(&test_box)->region_screen.size.height);
}

static void test_auto_suggest_box_keyboard_navigation_and_commit(void)
{
    setup_box();
    layout_box(10, 20, 180, 34);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_selected_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selected_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_selected);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_selected_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_selected_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_FALSE(egui_view_autocomplete_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, g_selected_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, g_selected_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ESCAPE));
    EGUI_TEST_ASSERT_FALSE(egui_view_autocomplete_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, g_selected_count);
}

static void test_auto_suggest_box_disabled_and_empty_guard_input(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_box();
    layout_box(10, 20, 180, 34);
    get_view_center(&x, &y);

    egui_view_set_enable(EGUI_VIEW_OF(&test_box), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_FALSE(egui_view_autocomplete_is_expanded(EGUI_VIEW_OF(&test_box)));

    setup_box();
    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&test_box), NULL, 0);
    layout_box(10, 20, 180, 34);
    get_view_center(&x, &y);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_FALSE(egui_view_autocomplete_is_expanded(EGUI_VIEW_OF(&test_box)));
}

void test_auto_suggest_box_run(void)
{
    EGUI_TEST_SUITE_BEGIN(auto_suggest_box);
    EGUI_TEST_RUN(test_auto_suggest_box_suggestions_current_index_and_text);
    EGUI_TEST_RUN(test_auto_suggest_box_style_helpers_and_params);
    EGUI_TEST_RUN(test_auto_suggest_box_touch_expand_select_and_fit_height);
    EGUI_TEST_RUN(test_auto_suggest_box_keyboard_navigation_and_commit);
    EGUI_TEST_RUN(test_auto_suggest_box_disabled_and_empty_guard_input);
    EGUI_TEST_SUITE_END();
}
