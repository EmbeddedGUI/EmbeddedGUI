#include "egui.h"
#include "test/egui_test.h"
#include "test_notification_badge.h"

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t);

static egui_view_notification_badge_t test_badge;

static void setup_badge(void)
{
    egui_view_notification_badge_init(EGUI_VIEW_OF(&test_badge));
    egui_view_set_size(EGUI_VIEW_OF(&test_badge), 34, 28);
}

static void test_notification_badge_init_uses_default_count_path(void)
{
    setup_badge();
    EGUI_TEST_ASSERT_NULL(test_badge.font);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT, test_badge.content_style);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_badge)->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t));
}

static void test_notification_badge_icon_mode_keeps_single_draw_path(void)
{
    setup_badge();
    egui_view_notification_badge_set_content_style(EGUI_VIEW_OF(&test_badge), EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_ICON);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_ICON, test_badge.content_style);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_badge)->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t));

    egui_view_notification_badge_set_content_style(EGUI_VIEW_OF(&test_badge), EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT, test_badge.content_style);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_badge)->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t));
}

static void test_notification_badge_custom_font_keeps_single_draw_path(void)
{
    setup_badge();
    egui_view_notification_badge_set_font(EGUI_VIEW_OF(&test_badge), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_badge.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_badge)->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t));

    egui_view_notification_badge_set_font(EGUI_VIEW_OF(&test_badge), NULL);
    EGUI_TEST_ASSERT_NULL(test_badge.font);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_badge)->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t));
}

void test_notification_badge_run(void)
{
    EGUI_TEST_SUITE_BEGIN(notification_badge);
    EGUI_TEST_RUN(test_notification_badge_init_uses_default_count_path);
    EGUI_TEST_RUN(test_notification_badge_icon_mode_keeps_single_draw_path);
    EGUI_TEST_RUN(test_notification_badge_custom_font_keeps_single_draw_path);
    EGUI_TEST_SUITE_END();
}
