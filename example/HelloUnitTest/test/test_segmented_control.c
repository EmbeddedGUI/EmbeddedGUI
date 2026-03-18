#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_segmented_control.h"

#include "../../HelloCustomWidgets/input/segmented_control/egui_view_segmented_control.h"
#include "../../HelloCustomWidgets/input/segmented_control/egui_view_segmented_control.c"

static egui_view_segmented_control_t test_control;
static uint8_t g_changed_count;
static uint8_t g_last_index;

static const char *g_segments_primary[] = {"Overview", "Team", "Usage", "Access"};
static const char *g_segments_secondary[] = {"Live", "Pending"};
static const char *g_segments_overflow[] = {"One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine"};
static const char *g_icons_secondary[] = {"A", "B"};
static const char *g_icons_overflow[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I"};

static void on_segment_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    g_changed_count++;
    g_last_index = index;
}

static void reset_listener_state(void)
{
    g_changed_count = 0;
    g_last_index = EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE;
}

static void setup_control(void)
{
    egui_view_segmented_control_init(EGUI_VIEW_OF(&test_control));
    egui_view_set_size(EGUI_VIEW_OF(&test_control), 180, 36);
    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&test_control), g_segments_primary, 4);
    egui_view_segmented_control_set_on_segment_changed_listener(EGUI_VIEW_OF(&test_control), on_segment_changed);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&test_control), 1);
#endif
    reset_listener_state();
}

static void layout_control(egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_control), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_control)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_control)->api->on_touch_event(EGUI_VIEW_OF(&test_control), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_control)->api->on_key_event(EGUI_VIEW_OF(&test_control), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_control)->api->on_key_event(EGUI_VIEW_OF(&test_control), &event);
    return handled;
}

static void get_segment_center(uint8_t item_index, egui_dim_t *x, egui_dim_t *y)
{
    egui_region_t work_region;
    egui_dim_t padding;
    egui_dim_t gap;
    egui_dim_t content_width;
    egui_dim_t content_height;
    egui_dim_t available_width;
    egui_dim_t base_width;
    egui_dim_t remainder;
    egui_dim_t cursor_x;
    uint8_t count;
    uint8_t i;

    egui_view_get_work_region(EGUI_VIEW_OF(&test_control), &work_region);
    count = test_control.segment_count;
    if (count > EGUI_VIEW_SEGMENTED_CONTROL_MAX_SEGMENTS)
    {
        count = EGUI_VIEW_SEGMENTED_CONTROL_MAX_SEGMENTS;
    }

    EGUI_TEST_ASSERT_TRUE(count > 0);
    EGUI_TEST_ASSERT_TRUE(item_index < count);

    padding = test_control.horizontal_padding;
    if (padding * 2 >= work_region.size.width || padding * 2 >= work_region.size.height)
    {
        padding = 0;
    }

    content_width = work_region.size.width - padding * 2;
    content_height = work_region.size.height - padding * 2;
    gap = (count <= 1) ? 0 : test_control.segment_gap;
    while (gap > 0 && (content_width - gap * (count - 1)) < count)
    {
        gap--;
    }

    available_width = content_width - gap * (count - 1);
    base_width = available_width / count;
    remainder = available_width % count;
    cursor_x = work_region.location.x + padding;

    for (i = 0; i < count; i++)
    {
        egui_dim_t segment_width = base_width;

        if (remainder > 0)
        {
            segment_width++;
            remainder--;
        }
        if (i == item_index)
        {
            *x = EGUI_VIEW_OF(&test_control)->region_screen.location.x + cursor_x + segment_width / 2;
            *y = EGUI_VIEW_OF(&test_control)->region_screen.location.y + work_region.location.y + padding + content_height / 2;
            return;
        }
        cursor_x += segment_width + gap;
    }

    *x = EGUI_VIEW_OF(&test_control)->region_screen.location.x + EGUI_VIEW_OF(&test_control)->region_screen.size.width / 2;
    *y = EGUI_VIEW_OF(&test_control)->region_screen.location.y + EGUI_VIEW_OF(&test_control)->region_screen.size.height / 2;
}

static void test_segmented_control_set_segments_clamps_and_resets_state(void)
{
    setup_control();

    test_control.current_index = 7;
    test_control.pressed_index = 3;
    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&test_control), g_segments_overflow, 9);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_MAX_SEGMENTS, test_control.segment_count);
    EGUI_TEST_ASSERT_EQUAL_INT(7, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, test_control.pressed_index);
    EGUI_TEST_ASSERT_TRUE(test_control.segment_texts == g_segments_overflow);

    test_control.current_index = 6;
    test_control.pressed_index = 1;
    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&test_control), g_segments_secondary, 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_control.segment_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, test_control.pressed_index);
    EGUI_TEST_ASSERT_TRUE(test_control.segment_texts == g_segments_secondary);

    egui_view_segmented_control_set_segment_icons(EGUI_VIEW_OF(&test_control), g_icons_secondary);
    EGUI_TEST_ASSERT_TRUE(test_control.segment_icons == g_icons_secondary);

    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&test_control), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_control.segment_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, test_control.pressed_index);
}

static void test_segmented_control_current_index_listener_and_guards(void)
{
    setup_control();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));

    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&test_control), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_last_index);

    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&test_control), 2);
    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&test_control), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));

    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&test_control), NULL, 0);
    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&test_control), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
}

static void test_segmented_control_style_helpers_and_params(void)
{
    egui_view_segmented_control_t params_control;
    egui_view_segmented_control_params_t params = {
            .region = {{1, 2}, {96, 28}},
            .segment_texts = g_segments_secondary,
            .segment_icons = g_icons_secondary,
            .segment_count = 2,
    };
    egui_view_segmented_control_params_t init_params = {
            .region = {{4, 5}, {120, 32}},
            .segment_texts = g_segments_overflow,
            .segment_icons = g_icons_overflow,
            .segment_count = 9,
    };

    setup_control();

    hcw_segmented_control_apply_standard_style(EGUI_VIEW_OF(&test_control));
    EGUI_TEST_ASSERT_EQUAL_INT(10, test_control.corner_radius);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_control.segment_gap);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_control.horizontal_padding);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x2563EB).full, test_control.selected_bg_color.full);

    hcw_segmented_control_apply_compact_style(EGUI_VIEW_OF(&test_control));
    EGUI_TEST_ASSERT_EQUAL_INT(8, test_control.corner_radius);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_control.segment_gap);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_control.horizontal_padding);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x0C7C73).full, test_control.selected_bg_color.full);

    hcw_segmented_control_apply_read_only_style(EGUI_VIEW_OF(&test_control));
    EGUI_TEST_ASSERT_EQUAL_INT(8, test_control.corner_radius);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x97A4B4).full, test_control.selected_bg_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xDBE2E8).full, test_control.border_color.full);

    egui_view_segmented_control_set_bg_color(EGUI_VIEW_OF(&test_control), EGUI_COLOR_HEX(0x101112));
    egui_view_segmented_control_set_selected_bg_color(EGUI_VIEW_OF(&test_control), EGUI_COLOR_HEX(0x202122));
    egui_view_segmented_control_set_text_color(EGUI_VIEW_OF(&test_control), EGUI_COLOR_HEX(0x303132));
    egui_view_segmented_control_set_selected_text_color(EGUI_VIEW_OF(&test_control), EGUI_COLOR_HEX(0x404142));
    egui_view_segmented_control_set_border_color(EGUI_VIEW_OF(&test_control), EGUI_COLOR_HEX(0x505152));
    egui_view_segmented_control_set_corner_radius(EGUI_VIEW_OF(&test_control), 6);
    egui_view_segmented_control_set_segment_gap(EGUI_VIEW_OF(&test_control), 4);
    egui_view_segmented_control_set_horizontal_padding(EGUI_VIEW_OF(&test_control), 3);
    egui_view_segmented_control_set_font(EGUI_VIEW_OF(&test_control), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_segmented_control_set_icon_font(EGUI_VIEW_OF(&test_control), EGUI_FONT_ICON_MS_16);
    egui_view_segmented_control_set_icon_text_gap(EGUI_VIEW_OF(&test_control), 5);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_control.bg_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_control.selected_bg_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_control.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_control.selected_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_control.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(6, test_control.corner_radius);
    EGUI_TEST_ASSERT_EQUAL_INT(4, test_control.segment_gap);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_control.horizontal_padding);
    EGUI_TEST_ASSERT_TRUE(test_control.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_control.icon_font == EGUI_FONT_ICON_MS_16);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_control.icon_text_gap);

    test_control.current_index = 7;
    test_control.pressed_index = 1;
    egui_view_segmented_control_apply_params(EGUI_VIEW_OF(&test_control), &params);
    EGUI_TEST_ASSERT_EQUAL_INT(1, EGUI_VIEW_OF(&test_control)->region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(2, EGUI_VIEW_OF(&test_control)->region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(96, EGUI_VIEW_OF(&test_control)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(28, EGUI_VIEW_OF(&test_control)->region.size.height);
    EGUI_TEST_ASSERT_TRUE(test_control.segment_texts == g_segments_secondary);
    EGUI_TEST_ASSERT_TRUE(test_control.segment_icons == g_icons_secondary);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_control.segment_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, test_control.pressed_index);

    egui_view_segmented_control_init_with_params(EGUI_VIEW_OF(&params_control), &init_params);
    EGUI_TEST_ASSERT_EQUAL_INT(4, EGUI_VIEW_OF(&params_control)->region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(5, EGUI_VIEW_OF(&params_control)->region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(120, EGUI_VIEW_OF(&params_control)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(32, EGUI_VIEW_OF(&params_control)->region.size.height);
    EGUI_TEST_ASSERT_TRUE(params_control.segment_texts == g_segments_overflow);
    EGUI_TEST_ASSERT_TRUE(params_control.segment_icons == g_icons_overflow);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_MAX_SEGMENTS, params_control.segment_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, params_control.current_index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, params_control.pressed_index);
    EGUI_TEST_ASSERT_TRUE(params_control.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(params_control.icon_font == NULL);
}

static void test_segmented_control_touch_interaction_and_hit_layout(void)
{
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x1;
    egui_dim_t y1;
    egui_dim_t x2;
    egui_dim_t y2;

    setup_control();
    layout_control(180, 36);
    get_segment_center(0, &x0, &y0);
    get_segment_center(1, &x1, &y1);
    get_segment_center(2, &x2, &y2);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, EGUI_VIEW_OF(&test_control)->region_screen.location.x, y0));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, test_control.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_control)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_control.pressed_index);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_control)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_control.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_control)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, test_control.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_control)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_control.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x1, y1));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_control)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x2, y2));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_control)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_last_index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, test_control.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_control)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_control.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, test_control.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_control)->is_pressed);

    egui_view_set_enable(EGUI_VIEW_OF(&test_control), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x0, y0));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x0, y0));
}

static void test_segmented_control_keyboard_navigation_and_guards(void)
{
    setup_control();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_index);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, g_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, g_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, g_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, g_changed_count);

    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ESCAPE));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&test_control)));

    egui_view_set_enable(EGUI_VIEW_OF(&test_control), 0);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
}

void test_segmented_control_run(void)
{
    EGUI_TEST_SUITE_BEGIN(segmented_control);
    EGUI_TEST_RUN(test_segmented_control_set_segments_clamps_and_resets_state);
    EGUI_TEST_RUN(test_segmented_control_current_index_listener_and_guards);
    EGUI_TEST_RUN(test_segmented_control_style_helpers_and_params);
    EGUI_TEST_RUN(test_segmented_control_touch_interaction_and_hit_layout);
    EGUI_TEST_RUN(test_segmented_control_keyboard_navigation_and_guards);
    EGUI_TEST_SUITE_END();
}
