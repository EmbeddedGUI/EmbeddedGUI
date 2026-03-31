#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_textblock.h"

static egui_view_textblock_t test_textblock;

static void setup_textblock(egui_dim_t width, egui_dim_t height, const char *text)
{
    egui_view_textblock_init(EGUI_VIEW_OF(&test_textblock));
    egui_view_set_padding(EGUI_VIEW_OF(&test_textblock), 0, 0, 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_textblock), width, height);
    egui_view_textblock_set_font(EGUI_VIEW_OF(&test_textblock), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_textblock_set_align_type(EGUI_VIEW_OF(&test_textblock), EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP);
    egui_view_textblock_set_text(EGUI_VIEW_OF(&test_textblock), text);
}

static void layout_textblock(egui_dim_t x, egui_dim_t y)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = EGUI_VIEW_OF(&test_textblock)->region.size.width;
    region.size.height = EGUI_VIEW_OF(&test_textblock)->region.size.height;
    egui_view_layout(EGUI_VIEW_OF(&test_textblock), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_textblock)->region_screen, &region);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_textblock)->api->on_touch_event(EGUI_VIEW_OF(&test_textblock), &event);
}
#endif

static void test_textblock_wraps_long_single_line_and_tracks_metrics(void)
{
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t width_three = 0;
    egui_dim_t line_height = 0;

    font->api->get_str_size(font, "WWW", 0, 0, &width_three, &line_height);
    setup_textblock(width_three, line_height * 4, "WWWWWWWW");

    EGUI_TEST_ASSERT_EQUAL_INT(3, test_textblock.content_line_count);
    EGUI_TEST_ASSERT_EQUAL_INT(width_three, test_textblock.content_width);
    EGUI_TEST_ASSERT_EQUAL_INT((line_height * 3) + (test_textblock.line_space * 2), test_textblock.content_height);
}

static void test_textblock_max_lines_clamps_wrapped_content(void)
{
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t width_three = 0;
    egui_dim_t line_height = 0;

    font->api->get_str_size(font, "WWW", 0, 0, &width_three, &line_height);
    setup_textblock(width_three, line_height * 4, "WWWWWWWW");
    egui_view_textblock_set_max_lines(EGUI_VIEW_OF(&test_textblock), 2);

    EGUI_TEST_ASSERT_EQUAL_INT(2, test_textblock.content_line_count);
    EGUI_TEST_ASSERT_EQUAL_INT(width_three, test_textblock.content_width);
    EGUI_TEST_ASSERT_EQUAL_INT((line_height * 2) + test_textblock.line_space, test_textblock.content_height);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_textblock_scroll_enabled_controls_drag_scroll(void)
{
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t width_three = 0;
    egui_dim_t line_height = 0;

    font->api->get_str_size(font, "WWW", 0, 0, &width_three, &line_height);
    setup_textblock(width_three, line_height * 2, "WWWWWWWWWWWW");
    layout_textblock(10, 20);

    egui_view_textblock_set_scroll_enabled(EGUI_VIEW_OF(&test_textblock), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 12, 22));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_textblock.scroll_offset_y);

    egui_view_textblock_set_scroll_enabled(EGUI_VIEW_OF(&test_textblock), 1);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 12, 32));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 12, 8));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 12, 8));
    EGUI_TEST_ASSERT_TRUE(test_textblock.scroll_offset_y > 0);
}
#endif

void test_textblock_run(void)
{
    EGUI_TEST_SUITE_BEGIN(textblock);
    EGUI_TEST_RUN(test_textblock_wraps_long_single_line_and_tracks_metrics);
    EGUI_TEST_RUN(test_textblock_max_lines_clamps_wrapped_content);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_textblock_scroll_enabled_controls_drag_scroll);
#endif
    EGUI_TEST_SUITE_END();
}
