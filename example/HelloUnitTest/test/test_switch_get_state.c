#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_switch_get_state.h"

static egui_view_switch_t s_switch;
static const char s_icon_on[] = "1";
static const char s_icon_off[] = "0";
static int s_checked_value;

static void on_checked(egui_view_t *self, int is_checked)
{
    EGUI_UNUSED(self);
    s_checked_value = is_checked;
}

static void setup(void)
{
    memset(&s_switch, 0, sizeof(s_switch));
    s_checked_value = -1;
    egui_view_switch_init(EGUI_VIEW_OF(&s_switch), uicode_get_core());
}

static void test_switch_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_on_checked_listener(EGUI_VIEW_OF(&s_switch)));
    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_icon_on(EGUI_VIEW_OF(&s_switch)));
    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_icon_off(EGUI_VIEW_OF(&s_switch)));
    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_icon_font(EGUI_VIEW_OF(&s_switch)));
}

static void test_switch_get_state_after_setters(void)
{
    setup();
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&s_switch), on_checked);
    egui_view_switch_set_state_icons(EGUI_VIEW_OF(&s_switch), s_icon_on, s_icon_off);
    egui_view_switch_set_icon_font(EGUI_VIEW_OF(&s_switch), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    EGUI_TEST_ASSERT_TRUE(egui_view_switch_get_on_checked_listener(EGUI_VIEW_OF(&s_switch)) == on_checked);
    EGUI_TEST_ASSERT_TRUE(egui_view_switch_get_icon_on(EGUI_VIEW_OF(&s_switch)) == s_icon_on);
    EGUI_TEST_ASSERT_TRUE(egui_view_switch_get_icon_off(EGUI_VIEW_OF(&s_switch)) == s_icon_off);
    EGUI_TEST_ASSERT_TRUE(egui_view_switch_get_icon_font(EGUI_VIEW_OF(&s_switch)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, s_checked_value);
}

static void test_switch_get_state_clear(void)
{
    setup();
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&s_switch), on_checked);
    egui_view_switch_set_state_icons(EGUI_VIEW_OF(&s_switch), s_icon_on, s_icon_off);
    egui_view_switch_set_icon_font(EGUI_VIEW_OF(&s_switch), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&s_switch), NULL);
    egui_view_switch_set_state_icons(EGUI_VIEW_OF(&s_switch), NULL, NULL);
    egui_view_switch_set_icon_font(EGUI_VIEW_OF(&s_switch), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_on_checked_listener(EGUI_VIEW_OF(&s_switch)));
    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_icon_on(EGUI_VIEW_OF(&s_switch)));
    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_icon_off(EGUI_VIEW_OF(&s_switch)));
    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_icon_font(EGUI_VIEW_OF(&s_switch)));
}

static void test_switch_get_state_listener_fires_on_change(void)
{
    setup();
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&s_switch), on_checked);
    egui_view_switch_set_checked(EGUI_VIEW_OF(&s_switch), 1);

    EGUI_TEST_ASSERT_EQUAL_INT(1, s_checked_value);
    EGUI_TEST_ASSERT_TRUE(egui_view_switch_get_on_checked_listener(EGUI_VIEW_OF(&s_switch)) == on_checked);
}

static void test_switch_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_on_checked_listener(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_icon_on(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_icon_off(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_switch_get_icon_font(NULL));
}

void test_switch_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(switch_get_state);

    EGUI_TEST_RUN(test_switch_get_state_defaults);
    EGUI_TEST_RUN(test_switch_get_state_after_setters);
    EGUI_TEST_RUN(test_switch_get_state_clear);
    EGUI_TEST_RUN(test_switch_get_state_listener_fires_on_change);
    EGUI_TEST_RUN(test_switch_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
