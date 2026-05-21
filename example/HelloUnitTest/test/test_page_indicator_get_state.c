#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_page_indicator_get_state.h"

static egui_view_page_indicator_t s_indicator;
static const char *const          s_icons[] = {"a", "b", "c"};

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_indicator, 0, sizeof(s_indicator));
    egui_view_page_indicator_init(EGUI_VIEW_OF(&s_indicator), core);
}

static void test_page_indicator_get_state_defaults(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_page_indicator_get_total_count(EGUI_VIEW_OF(&s_indicator)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_page_indicator_get_current_index(EGUI_VIEW_OF(&s_indicator)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_PAGE_INDICATOR_MARK_STYLE_DOT,
                               (int)egui_view_page_indicator_get_mark_style(EGUI_VIEW_OF(&s_indicator)));
    EGUI_TEST_ASSERT_NULL(egui_view_page_indicator_get_icons(EGUI_VIEW_OF(&s_indicator)));
    EGUI_TEST_ASSERT_NULL(egui_view_page_indicator_get_icon_font(EGUI_VIEW_OF(&s_indicator)));
}

static void test_page_indicator_get_state_after_setters(void)
{
    setup();
    egui_view_page_indicator_set_total_count(EGUI_VIEW_OF(&s_indicator), 3);
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&s_indicator), 2);
    egui_view_page_indicator_set_mark_style(EGUI_VIEW_OF(&s_indicator), EGUI_VIEW_PAGE_INDICATOR_MARK_STYLE_ICON);
    egui_view_page_indicator_set_icons(EGUI_VIEW_OF(&s_indicator), s_icons);
    egui_view_page_indicator_set_icon_font(EGUI_VIEW_OF(&s_indicator), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_page_indicator_get_total_count(EGUI_VIEW_OF(&s_indicator)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_page_indicator_get_current_index(EGUI_VIEW_OF(&s_indicator)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_PAGE_INDICATOR_MARK_STYLE_ICON,
                               (int)egui_view_page_indicator_get_mark_style(EGUI_VIEW_OF(&s_indicator)));
    EGUI_TEST_ASSERT_TRUE(egui_view_page_indicator_get_icons(EGUI_VIEW_OF(&s_indicator)) == s_icons);
    EGUI_TEST_ASSERT_TRUE(egui_view_page_indicator_get_icon_font(EGUI_VIEW_OF(&s_indicator)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
}

static void test_page_indicator_get_state_clamps_current_index(void)
{
    setup();
    egui_view_page_indicator_set_total_count(EGUI_VIEW_OF(&s_indicator), 3);
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&s_indicator), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_page_indicator_get_current_index(EGUI_VIEW_OF(&s_indicator)));

    egui_view_page_indicator_set_total_count(EGUI_VIEW_OF(&s_indicator), 0);
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&s_indicator), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_page_indicator_get_current_index(EGUI_VIEW_OF(&s_indicator)));
}

static void test_page_indicator_get_state_can_clear_icon_config(void)
{
    setup();
    egui_view_page_indicator_set_icons(EGUI_VIEW_OF(&s_indicator), s_icons);
    egui_view_page_indicator_set_icon_font(EGUI_VIEW_OF(&s_indicator), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_page_indicator_set_icons(EGUI_VIEW_OF(&s_indicator), NULL);
    egui_view_page_indicator_set_icon_font(EGUI_VIEW_OF(&s_indicator), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_page_indicator_get_icons(EGUI_VIEW_OF(&s_indicator)));
    EGUI_TEST_ASSERT_NULL(egui_view_page_indicator_get_icon_font(EGUI_VIEW_OF(&s_indicator)));
}

static void test_page_indicator_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_page_indicator_get_total_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_page_indicator_get_current_index(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_PAGE_INDICATOR_MARK_STYLE_DOT, (int)egui_view_page_indicator_get_mark_style(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_page_indicator_get_icons(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_page_indicator_get_icon_font(NULL));
}

void test_page_indicator_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(page_indicator_get_state);

    EGUI_TEST_RUN(test_page_indicator_get_state_defaults);
    EGUI_TEST_RUN(test_page_indicator_get_state_after_setters);
    EGUI_TEST_RUN(test_page_indicator_get_state_clamps_current_index);
    EGUI_TEST_RUN(test_page_indicator_get_state_can_clear_icon_config);
    EGUI_TEST_RUN(test_page_indicator_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
