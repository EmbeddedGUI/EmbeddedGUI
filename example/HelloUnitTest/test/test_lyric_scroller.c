#include "egui.h"
#include "test/egui_test.h"
#include "test_lyric_scroller.h"

static egui_view_lyric_scroller_t test_scroller;

static void setup_scroller(egui_dim_t width, egui_dim_t height, const char *text)
{
    egui_region_t region;

    egui_view_lyric_scroller_init(EGUI_VIEW_OF(&test_scroller));
    egui_view_set_padding(EGUI_VIEW_OF(&test_scroller), 0, 0, 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_scroller), width, height);
    egui_view_lyric_scroller_set_font(EGUI_VIEW_OF(&test_scroller), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_lyric_scroller_set_interval_ms(EGUI_VIEW_OF(&test_scroller), 10);
    egui_view_lyric_scroller_set_scroll_step(EGUI_VIEW_OF(&test_scroller), 2);
    egui_view_lyric_scroller_set_pause_duration_ms(EGUI_VIEW_OF(&test_scroller), 0);
    egui_view_lyric_scroller_set_text(EGUI_VIEW_OF(&test_scroller), text);

    egui_region_init(&region, 10, 20, width, height);
    egui_view_layout(EGUI_VIEW_OF(&test_scroller), &region);
    EGUI_VIEW_OF(&test_scroller)->api->calculate_layout(EGUI_VIEW_OF(&test_scroller));
}

static void attach_scroller(void)
{
    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&test_scroller));
}

static void detach_scroller(void)
{
    egui_view_dispatch_detach_from_window(EGUI_VIEW_OF(&test_scroller));
}

static void test_lyric_scroller_overflow_attach_starts_and_moves(void)
{
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t width_two = 0;
    egui_dim_t line_height = 0;

    font->api->get_str_size(font, "WW", 0, 0, &width_two, &line_height);
    setup_scroller(width_two, line_height + 4, "WWWWWW");

    EGUI_TEST_ASSERT_TRUE(test_scroller.max_scroll_offset > 0);
    attach_scroller();
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_scroller.scroll_timer));

    test_scroller.scroll_timer.callback(&test_scroller.scroll_timer);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_scroller.scroll_offset_x);
    EGUI_TEST_ASSERT_EQUAL_INT(-2, EGUI_VIEW_OF(&test_scroller.label)->region.location.x);

    detach_scroller();
}

static void test_lyric_scroller_set_text_restarts_from_left(void)
{
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t width_two = 0;
    egui_dim_t line_height = 0;

    font->api->get_str_size(font, "WW", 0, 0, &width_two, &line_height);
    setup_scroller(width_two, line_height + 4, "WWWWWW");
    attach_scroller();

    test_scroller.scroll_timer.callback(&test_scroller.scroll_timer);
    test_scroller.scroll_timer.callback(&test_scroller.scroll_timer);
    EGUI_TEST_ASSERT_TRUE(test_scroller.scroll_offset_x > 0);

    egui_view_lyric_scroller_set_text(EGUI_VIEW_OF(&test_scroller), "HELLOHELLO");
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_scroller.scroll_offset_x);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_scroller.scroll_direction);
    EGUI_TEST_ASSERT_EQUAL_INT(0, EGUI_VIEW_OF(&test_scroller.label)->region.location.x);
    EGUI_TEST_ASSERT_TRUE(test_scroller.max_scroll_offset > 0);

    detach_scroller();
}

static void test_lyric_scroller_short_text_stops_timer_until_overflow(void)
{
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t width_six = 0;
    egui_dim_t line_height = 0;

    font->api->get_str_size(font, "WWWWWW", 0, 0, &width_six, &line_height);
    setup_scroller(width_six + 10, line_height + 4, "WW");
    attach_scroller();

    EGUI_TEST_ASSERT_EQUAL_INT(0, test_scroller.max_scroll_offset);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_scroller.scroll_timer));

    egui_view_lyric_scroller_set_text(EGUI_VIEW_OF(&test_scroller), "WWWWWWWW");
    EGUI_TEST_ASSERT_TRUE(test_scroller.max_scroll_offset > 0);
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_scroller.scroll_timer));

    detach_scroller();
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_scroller.scroll_timer));
}

void test_lyric_scroller_run(void)
{
    EGUI_TEST_SUITE_BEGIN(lyric_scroller);
    EGUI_TEST_RUN(test_lyric_scroller_overflow_attach_starts_and_moves);
    EGUI_TEST_RUN(test_lyric_scroller_set_text_restarts_from_left);
    EGUI_TEST_RUN(test_lyric_scroller_short_text_stops_timer_until_overflow);
    EGUI_TEST_SUITE_END();
}
