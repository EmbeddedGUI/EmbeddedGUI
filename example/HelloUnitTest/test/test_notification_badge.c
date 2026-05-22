#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_notification_badge.h"

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t);

static egui_view_notification_badge_t test_badge;

static egui_core_t *test_notification_badge_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void setup_badge(void)
{
    egui_view_notification_badge_init(EGUI_VIEW_OF(&test_badge), test_notification_badge_get_core());
    egui_view_set_size(EGUI_VIEW_OF(&test_badge), 34, 28);
}

static void test_notification_badge_init_uses_default_count_path(void)
{
    setup_badge();
    EGUI_TEST_ASSERT_NULL(test_badge.font);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_notification_badge_get_count(EGUI_VIEW_OF(&test_badge)));
    EGUI_TEST_ASSERT_EQUAL_INT(99, (int)egui_view_notification_badge_get_max_display(EGUI_VIEW_OF(&test_badge)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT, test_badge.content_style);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT,
                               (int)egui_view_notification_badge_get_content_style(EGUI_VIEW_OF(&test_badge)));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_badge)->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t));
}

static void test_notification_badge_icon_mode_keeps_single_draw_path(void)
{
    setup_badge();
    egui_view_notification_badge_set_content_style(EGUI_VIEW_OF(&test_badge), EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_ICON);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_ICON, test_badge.content_style);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_ICON, (int)egui_view_notification_badge_get_content_style(EGUI_VIEW_OF(&test_badge)));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_badge)->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t));

    egui_view_notification_badge_set_content_style(EGUI_VIEW_OF(&test_badge), EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT, test_badge.content_style);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT,
                               (int)egui_view_notification_badge_get_content_style(EGUI_VIEW_OF(&test_badge)));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_badge)->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t));
}

static void test_notification_badge_custom_font_keeps_single_draw_path(void)
{
    setup_badge();
    egui_view_notification_badge_set_font(EGUI_VIEW_OF(&test_badge), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_badge.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_notification_badge_get_font(EGUI_VIEW_OF(&test_badge)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_badge)->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t));

    egui_view_notification_badge_set_font(EGUI_VIEW_OF(&test_badge), NULL);
    EGUI_TEST_ASSERT_NULL(test_badge.font);
    EGUI_TEST_ASSERT_NULL(egui_view_notification_badge_get_font(EGUI_VIEW_OF(&test_badge)));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_badge)->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t));
}

static void test_notification_badge_state_getters_after_setters(void)
{
    egui_color_t badge_color = {.full = 0x1234};
    egui_color_t text_color = {.full = 0x5678};
    const char *icon = "x";

    setup_badge();
    egui_view_notification_badge_set_count(EGUI_VIEW_OF(&test_badge), 120);
    egui_view_notification_badge_set_max_display(EGUI_VIEW_OF(&test_badge), 42);
    egui_view_notification_badge_set_badge_color(EGUI_VIEW_OF(&test_badge), badge_color);
    egui_view_notification_badge_set_text_color(EGUI_VIEW_OF(&test_badge), text_color);
    egui_view_notification_badge_set_icon(EGUI_VIEW_OF(&test_badge), icon);
    egui_view_notification_badge_set_icon_font(EGUI_VIEW_OF(&test_badge), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    EGUI_TEST_ASSERT_EQUAL_INT(120, (int)egui_view_notification_badge_get_count(EGUI_VIEW_OF(&test_badge)));
    EGUI_TEST_ASSERT_EQUAL_INT(42, (int)egui_view_notification_badge_get_max_display(EGUI_VIEW_OF(&test_badge)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)badge_color.full, (int)egui_view_notification_badge_get_badge_color(EGUI_VIEW_OF(&test_badge)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)text_color.full, (int)egui_view_notification_badge_get_text_color(EGUI_VIEW_OF(&test_badge)).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_notification_badge_get_icon(EGUI_VIEW_OF(&test_badge)) == icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_notification_badge_get_icon_font(EGUI_VIEW_OF(&test_badge)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
}

static void test_notification_badge_max_display_clamps_to_one(void)
{
    setup_badge();
    egui_view_notification_badge_set_max_display(EGUI_VIEW_OF(&test_badge), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_notification_badge_get_max_display(EGUI_VIEW_OF(&test_badge)));
}

static void test_notification_badge_state_getters_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_notification_badge_get_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_notification_badge_get_max_display(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_notification_badge_get_badge_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_notification_badge_get_text_color(NULL).full);
    EGUI_TEST_ASSERT_NULL(egui_view_notification_badge_get_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT, (int)egui_view_notification_badge_get_content_style(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_notification_badge_get_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_notification_badge_get_icon_font(NULL));
}

void test_notification_badge_run(void)
{
    EGUI_TEST_SUITE_BEGIN(notification_badge);
    EGUI_TEST_RUN(test_notification_badge_init_uses_default_count_path);
    EGUI_TEST_RUN(test_notification_badge_icon_mode_keeps_single_draw_path);
    EGUI_TEST_RUN(test_notification_badge_custom_font_keeps_single_draw_path);
    EGUI_TEST_RUN(test_notification_badge_state_getters_after_setters);
    EGUI_TEST_RUN(test_notification_badge_max_display_clamps_to_one);
    EGUI_TEST_RUN(test_notification_badge_state_getters_null_self);
    EGUI_TEST_SUITE_END();
}
