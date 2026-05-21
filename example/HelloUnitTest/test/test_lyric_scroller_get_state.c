#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_lyric_scroller_get_state.h"

static egui_view_lyric_scroller_t s_scroller;

static void setup(void)
{
    memset(&s_scroller, 0, sizeof(s_scroller));
    egui_view_lyric_scroller_init(EGUI_VIEW_OF(&s_scroller), uicode_get_core());
}

static void layout_scroller(egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    egui_view_set_padding(EGUI_VIEW_OF(&s_scroller), 0, 0, 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&s_scroller), width, height);
    egui_region_init(&region, 0, 0, width, height);
    egui_view_layout(EGUI_VIEW_OF(&s_scroller), &region);
    EGUI_VIEW_OF(&s_scroller)->api->calculate_layout(EGUI_VIEW_OF(&s_scroller));
}

static void setup_overflow_scroller(void)
{
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t width_two = 0;
    egui_dim_t line_height = 0;

    setup();
    font->api->get_str_size(font, "WW", 0, 0, &width_two, &line_height);
    egui_view_lyric_scroller_set_font(EGUI_VIEW_OF(&s_scroller), font);
    egui_view_lyric_scroller_set_text(EGUI_VIEW_OF(&s_scroller), "WWWWWW");
    layout_scroller(width_two, line_height + 4);
}

static void test_lyric_scroller_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_lyric_scroller_get_text(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_NULL(egui_view_lyric_scroller_get_font(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_PRIMARY.full, (int)egui_view_lyric_scroller_get_font_color(EGUI_VIEW_OF(&s_scroller)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100, (int)egui_view_lyric_scroller_get_font_alpha(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_lyric_scroller_get_scroll_step(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_lyric_scroller_get_interval_ms(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(400, (int)egui_view_lyric_scroller_get_pause_duration_ms(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_text_width(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_text_height(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_scroll_offset_x(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_max_scroll_offset(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_is_scrolling(EGUI_VIEW_OF(&s_scroller)));
}

static void test_lyric_scroller_get_state_after_setters(void)
{
    const char *text = "WWWWWW";
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_color_t color = {.full = 0x1234};
    egui_dim_t expected_width = 0;
    egui_dim_t expected_height = 0;

    setup();
    egui_view_lyric_scroller_set_text(EGUI_VIEW_OF(&s_scroller), text);
    egui_view_lyric_scroller_set_font(EGUI_VIEW_OF(&s_scroller), font);
    egui_view_lyric_scroller_set_font_color(EGUI_VIEW_OF(&s_scroller), color, 88);
    egui_view_lyric_scroller_set_scroll_step(EGUI_VIEW_OF(&s_scroller), 3);
    egui_view_lyric_scroller_set_interval_ms(EGUI_VIEW_OF(&s_scroller), 25);
    egui_view_lyric_scroller_set_pause_duration_ms(EGUI_VIEW_OF(&s_scroller), 150);
    font->api->get_str_size(font, text, 0, 0, &expected_width, &expected_height);
    layout_scroller(expected_width + 10, expected_height + 4);

    EGUI_TEST_ASSERT_TRUE(egui_view_lyric_scroller_get_text(EGUI_VIEW_OF(&s_scroller)) == text);
    EGUI_TEST_ASSERT_TRUE(egui_view_lyric_scroller_get_font(EGUI_VIEW_OF(&s_scroller)) == font);
    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)egui_view_lyric_scroller_get_font_color(EGUI_VIEW_OF(&s_scroller)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(88, (int)egui_view_lyric_scroller_get_font_alpha(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_lyric_scroller_get_scroll_step(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(25, (int)egui_view_lyric_scroller_get_interval_ms(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(150, (int)egui_view_lyric_scroller_get_pause_duration_ms(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)expected_width, (int)egui_view_lyric_scroller_get_text_width(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)expected_height, (int)egui_view_lyric_scroller_get_text_height(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_max_scroll_offset(EGUI_VIEW_OF(&s_scroller)));
}

static void test_lyric_scroller_get_state_clamps_config(void)
{
    setup();
    egui_view_lyric_scroller_set_scroll_step(EGUI_VIEW_OF(&s_scroller), 0);
    egui_view_lyric_scroller_set_interval_ms(EGUI_VIEW_OF(&s_scroller), 0);

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_lyric_scroller_get_scroll_step(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_lyric_scroller_get_interval_ms(EGUI_VIEW_OF(&s_scroller)));
}

static void test_lyric_scroller_get_state_apply_params(void)
{
    const char *text = "PARAMS";
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_color_t color = {.full = 0x4321};
    egui_view_lyric_scroller_params_t params = {
            .region = {{4, 5}, {60, 20}},
            .text = text,
            .font = font,
            .color = color,
            .alpha = 66,
            .scroll_step = 0,
            .interval_ms = 0,
            .pause_duration_ms = 90,
    };

    setup();
    egui_view_lyric_scroller_apply_params(EGUI_VIEW_OF(&s_scroller), &params);

    EGUI_TEST_ASSERT_TRUE(egui_view_lyric_scroller_get_text(EGUI_VIEW_OF(&s_scroller)) == text);
    EGUI_TEST_ASSERT_TRUE(egui_view_lyric_scroller_get_font(EGUI_VIEW_OF(&s_scroller)) == font);
    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)egui_view_lyric_scroller_get_font_color(EGUI_VIEW_OF(&s_scroller)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(66, (int)egui_view_lyric_scroller_get_font_alpha(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_lyric_scroller_get_scroll_step(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_lyric_scroller_get_interval_ms(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_lyric_scroller_get_pause_duration_ms(EGUI_VIEW_OF(&s_scroller)));
}

static void test_lyric_scroller_get_state_running_scroll(void)
{
    setup_overflow_scroller();
    EGUI_TEST_ASSERT_TRUE(egui_view_lyric_scroller_get_max_scroll_offset(EGUI_VIEW_OF(&s_scroller)) > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_is_scrolling(EGUI_VIEW_OF(&s_scroller)));

    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&s_scroller));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_lyric_scroller_is_scrolling(EGUI_VIEW_OF(&s_scroller)));

    s_scroller.scroll_timer.callback(&s_scroller.scroll_timer);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_lyric_scroller_is_scrolling(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)(egui_view_lyric_scroller_get_scroll_offset_x(EGUI_VIEW_OF(&s_scroller)) > 0));

    egui_view_lyric_scroller_stop(EGUI_VIEW_OF(&s_scroller));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_is_scrolling(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)(egui_view_lyric_scroller_get_scroll_offset_x(EGUI_VIEW_OF(&s_scroller)) > 0));

    egui_view_lyric_scroller_restart(EGUI_VIEW_OF(&s_scroller));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_scroll_offset_x(EGUI_VIEW_OF(&s_scroller)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_lyric_scroller_is_scrolling(EGUI_VIEW_OF(&s_scroller)));

    egui_view_dispatch_detach_from_window(EGUI_VIEW_OF(&s_scroller));
}

static void test_lyric_scroller_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_lyric_scroller_get_text(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_lyric_scroller_get_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_font_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_font_alpha(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_scroll_step(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_interval_ms(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_pause_duration_ms(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_text_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_text_height(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_scroll_offset_x(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_get_max_scroll_offset(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_lyric_scroller_is_scrolling(NULL));
}

void test_lyric_scroller_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(lyric_scroller_get_state);

    EGUI_TEST_RUN(test_lyric_scroller_get_state_defaults);
    EGUI_TEST_RUN(test_lyric_scroller_get_state_after_setters);
    EGUI_TEST_RUN(test_lyric_scroller_get_state_clamps_config);
    EGUI_TEST_RUN(test_lyric_scroller_get_state_apply_params);
    EGUI_TEST_RUN(test_lyric_scroller_get_state_running_scroll);
    EGUI_TEST_RUN(test_lyric_scroller_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
