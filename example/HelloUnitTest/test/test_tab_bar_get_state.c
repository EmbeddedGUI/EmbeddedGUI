#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_tab_bar_get_state.h"

static egui_view_tab_bar_t s_tab_bar;
static const char *s_tabs[] = {"Home", "Settings", "About"};
static const char *s_tabs_short[] = {"Home", "Settings"};
static const char *s_icons[] = {"h", "s", "a"};
static uint8_t s_last_index;
static uint8_t s_change_count;

static void on_tab_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    s_last_index = index;
    s_change_count++;
}

static void setup(void)
{
    memset(&s_tab_bar, 0, sizeof(s_tab_bar));
    s_last_index = EGUI_VIEW_TAB_BAR_PRESSED_NONE;
    s_change_count = 0;
    egui_view_tab_bar_init(EGUI_VIEW_OF(&s_tab_bar), uicode_get_core());
}

static void layout_tab_bar(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 180;
    region.size.height = 40;
    egui_view_layout(EGUI_VIEW_OF(&s_tab_bar), &region);
    egui_region_copy(&EGUI_VIEW_OF(&s_tab_bar)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&s_tab_bar)->api->on_touch_event(EGUI_VIEW_OF(&s_tab_bar), &event);
}

static void test_tab_bar_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_tabs(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_tab_icons(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tab_bar_get_tab_count(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tab_bar_get_current_index(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_TRUE(egui_view_tab_bar_get_font(EGUI_VIEW_OF(&s_tab_bar)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_icon_font(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_on_tab_changed_listener(EGUI_VIEW_OF(&s_tab_bar)));
}

static void test_tab_bar_get_state_after_setters(void)
{
    setup();
    egui_view_tab_bar_set_tabs(EGUI_VIEW_OF(&s_tab_bar), s_tabs, EGUI_ARRAY_SIZE(s_tabs));
    egui_view_tab_bar_set_tab_icons(EGUI_VIEW_OF(&s_tab_bar), s_icons);
    egui_view_tab_bar_set_on_tab_changed_listener(EGUI_VIEW_OF(&s_tab_bar), on_tab_changed);
    egui_view_tab_bar_set_font(EGUI_VIEW_OF(&s_tab_bar), NULL);
    egui_view_tab_bar_set_icon_font(EGUI_VIEW_OF(&s_tab_bar), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_tab_bar_set_current_index(EGUI_VIEW_OF(&s_tab_bar), 2);

    EGUI_TEST_ASSERT_TRUE(egui_view_tab_bar_get_tabs(EGUI_VIEW_OF(&s_tab_bar)) == s_tabs);
    EGUI_TEST_ASSERT_TRUE(egui_view_tab_bar_get_tab_icons(EGUI_VIEW_OF(&s_tab_bar)) == s_icons);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ARRAY_SIZE(s_tabs), (int)egui_view_tab_bar_get_tab_count(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_tab_bar_get_current_index(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_font(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_TRUE(egui_view_tab_bar_get_icon_font(EGUI_VIEW_OF(&s_tab_bar)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_tab_bar_get_on_tab_changed_listener(EGUI_VIEW_OF(&s_tab_bar)) == on_tab_changed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_change_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)s_last_index);
}

static void test_tab_bar_get_state_clear_and_clamp(void)
{
    setup();
    egui_view_tab_bar_set_tabs(EGUI_VIEW_OF(&s_tab_bar), s_tabs, EGUI_ARRAY_SIZE(s_tabs));
    egui_view_tab_bar_set_tab_icons(EGUI_VIEW_OF(&s_tab_bar), s_icons);
    egui_view_tab_bar_set_on_tab_changed_listener(EGUI_VIEW_OF(&s_tab_bar), on_tab_changed);
    egui_view_tab_bar_set_font(EGUI_VIEW_OF(&s_tab_bar), NULL);
    egui_view_tab_bar_set_icon_font(EGUI_VIEW_OF(&s_tab_bar), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_tab_bar_set_current_index(EGUI_VIEW_OF(&s_tab_bar), 2);

    egui_view_tab_bar_set_tabs(EGUI_VIEW_OF(&s_tab_bar), s_tabs_short, EGUI_ARRAY_SIZE(s_tabs_short));
    EGUI_TEST_ASSERT_TRUE(egui_view_tab_bar_get_tabs(EGUI_VIEW_OF(&s_tab_bar)) == s_tabs_short);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ARRAY_SIZE(s_tabs_short), (int)egui_view_tab_bar_get_tab_count(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tab_bar_get_current_index(EGUI_VIEW_OF(&s_tab_bar)));

    egui_view_tab_bar_set_tabs(EGUI_VIEW_OF(&s_tab_bar), NULL, 0);
    egui_view_tab_bar_set_tab_icons(EGUI_VIEW_OF(&s_tab_bar), NULL);
    egui_view_tab_bar_set_on_tab_changed_listener(EGUI_VIEW_OF(&s_tab_bar), NULL);
    egui_view_tab_bar_set_icon_font(EGUI_VIEW_OF(&s_tab_bar), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_tabs(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_tab_icons(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tab_bar_get_tab_count(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tab_bar_get_current_index(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_icon_font(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_on_tab_changed_listener(EGUI_VIEW_OF(&s_tab_bar)));
}

static void test_tab_bar_get_state_apply_params(void)
{
    static const egui_view_tab_bar_params_t params = {
            .region = {{3, 4}, {150, 32}},
            .tab_texts = s_tabs,
            .tab_count = EGUI_ARRAY_SIZE(s_tabs),
    };

    setup();
    egui_view_tab_bar_apply_params(EGUI_VIEW_OF(&s_tab_bar), &params);

    EGUI_TEST_ASSERT_TRUE(egui_view_tab_bar_get_tabs(EGUI_VIEW_OF(&s_tab_bar)) == s_tabs);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ARRAY_SIZE(s_tabs), (int)egui_view_tab_bar_get_tab_count(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(150, (int)egui_view_get_width(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(32, (int)egui_view_get_height(EGUI_VIEW_OF(&s_tab_bar)));
}

static void test_tab_bar_get_state_listener_fires_on_release(void)
{
    setup();
    egui_view_tab_bar_set_tabs(EGUI_VIEW_OF(&s_tab_bar), s_tabs, EGUI_ARRAY_SIZE(s_tabs));
    egui_view_tab_bar_set_on_tab_changed_listener(EGUI_VIEW_OF(&s_tab_bar), on_tab_changed);
    layout_tab_bar();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 100, 40));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 100, 40));

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_change_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_last_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_tab_bar_get_current_index(EGUI_VIEW_OF(&s_tab_bar)));
    EGUI_TEST_ASSERT_TRUE(egui_view_tab_bar_get_on_tab_changed_listener(EGUI_VIEW_OF(&s_tab_bar)) == on_tab_changed);
}

static void test_tab_bar_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_tabs(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_tab_icons(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_icon_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_tab_bar_get_on_tab_changed_listener(NULL));
}

void test_tab_bar_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(tab_bar_get_state);

    EGUI_TEST_RUN(test_tab_bar_get_state_defaults);
    EGUI_TEST_RUN(test_tab_bar_get_state_after_setters);
    EGUI_TEST_RUN(test_tab_bar_get_state_clear_and_clamp);
    EGUI_TEST_RUN(test_tab_bar_get_state_apply_params);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_tab_bar_get_state_listener_fires_on_release);
#endif
    EGUI_TEST_RUN(test_tab_bar_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
