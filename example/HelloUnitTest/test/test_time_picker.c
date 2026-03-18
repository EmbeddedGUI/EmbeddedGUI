#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_time_picker.h"

#include "../../HelloCustomWidgets/input/time_picker/egui_view_time_picker.h"
#include "../../HelloCustomWidgets/input/time_picker/egui_view_time_picker.c"

static egui_view_time_picker_t test_time_picker;
static uint8_t g_time_changed_count;
static uint8_t g_last_hour24;
static uint8_t g_last_minute;
static uint8_t g_open_changed_count;
static uint8_t g_last_opened;

static void on_time_changed(egui_view_t *self, uint8_t hour24, uint8_t minute)
{
    EGUI_UNUSED(self);
    g_time_changed_count++;
    g_last_hour24 = hour24;
    g_last_minute = minute;
}

static void on_open_changed(egui_view_t *self, uint8_t opened)
{
    EGUI_UNUSED(self);
    g_open_changed_count++;
    g_last_opened = opened;
}

static void reset_listener_state(void)
{
    g_time_changed_count = 0;
    g_last_hour24 = 0xFF;
    g_last_minute = 0xFF;
    g_open_changed_count = 0;
    g_last_opened = 0xFF;
}

static void setup_time_picker(void)
{
    egui_view_time_picker_init(EGUI_VIEW_OF(&test_time_picker));
    egui_view_set_size(EGUI_VIEW_OF(&test_time_picker), 194, 126);
    egui_view_time_picker_set_label(EGUI_VIEW_OF(&test_time_picker), "Start time");
    egui_view_time_picker_set_helper(EGUI_VIEW_OF(&test_time_picker), "15 minute intervals, AM/PM");
    egui_view_time_picker_set_time(EGUI_VIEW_OF(&test_time_picker), 8, 30);
    egui_view_time_picker_set_on_time_changed_listener(EGUI_VIEW_OF(&test_time_picker), on_time_changed);
    egui_view_time_picker_set_on_open_changed_listener(EGUI_VIEW_OF(&test_time_picker), on_open_changed);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&test_time_picker), 1);
#endif
    reset_listener_state();
}

static void layout_time_picker(egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_time_picker), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_time_picker)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_time_picker)->api->on_touch_event(EGUI_VIEW_OF(&test_time_picker), &event);
}

static int send_key_action(uint8_t type, uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key_code = key_code;
    return EGUI_VIEW_OF(&test_time_picker)->api->on_key_event(EGUI_VIEW_OF(&test_time_picker), &event);
}

static int send_key(uint8_t key_code)
{
    int handled = 0;

    handled |= send_key_action(EGUI_KEY_EVENT_ACTION_DOWN, key_code);
    handled |= send_key_action(EGUI_KEY_EVENT_ACTION_UP, key_code);
    return handled;
}

static void get_metrics(egui_view_time_picker_metrics_t *metrics)
{
    time_picker_get_metrics(&test_time_picker, EGUI_VIEW_OF(&test_time_picker), metrics);
}

static void get_field_center(egui_dim_t *x, egui_dim_t *y)
{
    egui_view_time_picker_metrics_t metrics;

    get_metrics(&metrics);
    *x = metrics.field_region.location.x + metrics.field_region.size.width / 2;
    *y = metrics.field_region.location.y + metrics.field_region.size.height / 2;
}

static void get_panel_cell_center(uint8_t segment, uint8_t row, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_time_picker_metrics_t metrics;
    egui_dim_t row_h;

    get_metrics(&metrics);
    row_h = metrics.panel_columns[segment].size.height / 3;
    *x = metrics.panel_columns[segment].location.x + metrics.panel_columns[segment].size.width / 2;
    *y = metrics.panel_columns[segment].location.y + row_h * row + row_h / 2;
}

static void test_time_picker_time_step_and_listener_guards(void)
{
    setup_time_picker();

    egui_view_time_picker_set_time(EGUI_VIEW_OF(&test_time_picker), 99, 59);
    EGUI_TEST_ASSERT_EQUAL_INT(23, egui_view_time_picker_get_hour24(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(45, egui_view_time_picker_get_minute(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_time_changed_count);

    egui_view_time_picker_set_minute_step(EGUI_VIEW_OF(&test_time_picker), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_time_picker_get_minute_step(EGUI_VIEW_OF(&test_time_picker)));
    egui_view_time_picker_set_time(EGUI_VIEW_OF(&test_time_picker), 9, 7);
    EGUI_TEST_ASSERT_EQUAL_INT(7, egui_view_time_picker_get_minute(EGUI_VIEW_OF(&test_time_picker)));
    egui_view_time_picker_set_minute_step(EGUI_VIEW_OF(&test_time_picker), 31);
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_time_picker_get_minute_step(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_time_picker_get_minute(EGUI_VIEW_OF(&test_time_picker)));

    egui_view_time_picker_set_focused_segment(EGUI_VIEW_OF(&test_time_picker), EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD, egui_view_time_picker_get_focused_segment(EGUI_VIEW_OF(&test_time_picker)));
    egui_view_time_picker_set_use_24h(EGUI_VIEW_OF(&test_time_picker), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_time_picker_get_use_24h(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE, egui_view_time_picker_get_focused_segment(EGUI_VIEW_OF(&test_time_picker)));
    egui_view_time_picker_set_focused_segment(EGUI_VIEW_OF(&test_time_picker), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE, egui_view_time_picker_get_focused_segment(EGUI_VIEW_OF(&test_time_picker)));
    egui_view_time_picker_set_use_24h(EGUI_VIEW_OF(&test_time_picker), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_time_picker_get_use_24h(EGUI_VIEW_OF(&test_time_picker)));

    egui_view_time_picker_set_opened(EGUI_VIEW_OF(&test_time_picker), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_open_changed_count);

    egui_view_time_picker_set_compact_mode(EGUI_VIEW_OF(&test_time_picker), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_time_picker.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_open_changed_count);

    egui_view_time_picker_set_compact_mode(EGUI_VIEW_OF(&test_time_picker), 0);
    egui_view_time_picker_set_opened(EGUI_VIEW_OF(&test_time_picker), 1);
    egui_view_time_picker_set_read_only_mode(EGUI_VIEW_OF(&test_time_picker), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_time_picker.read_only_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_open_changed_count);

    egui_view_time_picker_set_opened(EGUI_VIEW_OF(&test_time_picker), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));
}

static void test_time_picker_font_palette_and_helpers(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    char buffer[8];

    setup_time_picker();

    egui_view_time_picker_set_font(EGUI_VIEW_OF(&test_time_picker), NULL);
    egui_view_time_picker_set_meta_font(EGUI_VIEW_OF(&test_time_picker), NULL);
    EGUI_TEST_ASSERT_TRUE(test_time_picker.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_time_picker.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_time_picker_set_palette(EGUI_VIEW_OF(&test_time_picker), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                      EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_time_picker.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_time_picker.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_time_picker.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_time_picker.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_time_picker.accent_color.full);

    EGUI_TEST_ASSERT_EQUAL_INT(1, time_picker_normalize_step(0));
    EGUI_TEST_ASSERT_EQUAL_INT(30, time_picker_normalize_step(31));
    EGUI_TEST_ASSERT_EQUAL_INT(12, time_picker_normalize_step(12));
    EGUI_TEST_ASSERT_EQUAL_INT(45, time_picker_round_minute(59, 15));
    EGUI_TEST_ASSERT_EQUAL_INT(7, time_picker_round_minute(7, 0));
    EGUI_TEST_ASSERT_EQUAL_INT(3, time_picker_visible_segment_count(&test_time_picker));
    test_time_picker.use_24h = 1;
    EGUI_TEST_ASSERT_EQUAL_INT(2, time_picker_visible_segment_count(&test_time_picker));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE, time_picker_normalize_focus(&test_time_picker, EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD));
    test_time_picker.use_24h = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD, time_picker_normalize_focus(&test_time_picker, 9));

    time_picker_format_2d(7, buffer);
    EGUI_TEST_ASSERT_TRUE(strcmp(buffer, "07") == 0);

    test_time_picker.hour24 = 0;
    test_time_picker.minute = 30;
    test_time_picker.minute_step = 15;
    test_time_picker.use_24h = 0;
    time_picker_get_segment_text(&test_time_picker, EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR, 0, buffer);
    EGUI_TEST_ASSERT_TRUE(strcmp(buffer, "12") == 0);
    time_picker_get_segment_text(&test_time_picker, EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE, 1, buffer);
    EGUI_TEST_ASSERT_TRUE(strcmp(buffer, "45") == 0);
    time_picker_get_segment_text(&test_time_picker, EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD, 0, buffer);
    EGUI_TEST_ASSERT_TRUE(strcmp(buffer, "AM") == 0);
    time_picker_get_segment_text(&test_time_picker, EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD, 1, buffer);
    EGUI_TEST_ASSERT_TRUE(strcmp(buffer, "PM") == 0);
    test_time_picker.use_24h = 1;
    test_time_picker.hour24 = 23;
    time_picker_get_segment_text(&test_time_picker, EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR, 0, buffer);
    EGUI_TEST_ASSERT_TRUE(strcmp(buffer, "23") == 0);

    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, time_picker_mix_disabled(sample).full);
}

static void test_time_picker_metrics_and_hit_testing(void)
{
    egui_view_time_picker_metrics_t metrics;
    egui_dim_t x;
    egui_dim_t y;

    setup_time_picker();
    egui_view_time_picker_set_opened(EGUI_VIEW_OF(&test_time_picker), 1);
    layout_time_picker(194, 126);
    get_metrics(&metrics);

    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_label);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_helper);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_panel);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_period);
    EGUI_TEST_ASSERT_EQUAL_INT(3, metrics.segment_count);
    EGUI_TEST_ASSERT_TRUE(metrics.field_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.panel_region.size.height > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.field_segment_regions[2].size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.chevron_region.size.width > 0);

    get_field_center(&x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_PART_FIELD, time_picker_hit_part(&test_time_picker, EGUI_VIEW_OF(&test_time_picker), x, y));

    get_panel_cell_center(0, 0, &x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_PART_HOUR, time_picker_hit_part(&test_time_picker, EGUI_VIEW_OF(&test_time_picker), x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, time_picker_panel_row(&metrics.panel_columns[0], y));

    get_panel_cell_center(1, 1, &x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_PART_MINUTE, time_picker_hit_part(&test_time_picker, EGUI_VIEW_OF(&test_time_picker), x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, time_picker_panel_row(&metrics.panel_columns[1], y));

    get_panel_cell_center(2, 2, &x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_PART_PERIOD, time_picker_hit_part(&test_time_picker, EGUI_VIEW_OF(&test_time_picker), x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, time_picker_panel_row(&metrics.panel_columns[2], y));

    egui_view_time_picker_set_use_24h(EGUI_VIEW_OF(&test_time_picker), 1);
    layout_time_picker(194, 126);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.show_period);
    EGUI_TEST_ASSERT_EQUAL_INT(2, metrics.segment_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.field_segment_regions[2].size.width);

    egui_view_time_picker_set_opened(EGUI_VIEW_OF(&test_time_picker), 1);
    egui_view_time_picker_set_compact_mode(EGUI_VIEW_OF(&test_time_picker), 1);
    egui_view_set_size(EGUI_VIEW_OF(&test_time_picker), 106, 58);
    layout_time_picker(106, 58);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.show_label);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.show_helper);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.show_panel);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.chevron_region.size.width);
}

static void test_time_picker_touch_toggle_and_panel_selection(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_time_picker();
    layout_time_picker(194, 126);
    get_field_center(&x, &y);

    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 0, 0));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_PART_FIELD, test_time_picker.pressed_part);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_time_picker)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_open_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_opened);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_PART_NONE, test_time_picker.pressed_part);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_time_picker)->is_pressed);

    get_panel_cell_center(EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR, 0, &x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_PART_HOUR, test_time_picker.pressed_part);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(7, egui_view_time_picker_get_hour24(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_time_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(7, g_last_hour24);
    EGUI_TEST_ASSERT_EQUAL_INT(30, g_last_minute);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR, egui_view_time_picker_get_focused_segment(EGUI_VIEW_OF(&test_time_picker)));

    get_panel_cell_center(EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE, 2, &x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(45, egui_view_time_picker_get_minute(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_time_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(45, g_last_minute);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE, egui_view_time_picker_get_focused_segment(EGUI_VIEW_OF(&test_time_picker)));

    get_panel_cell_center(EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD, 1, &x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_time_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_PART_NONE, test_time_picker.pressed_part);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_time_picker)->is_pressed);

    get_field_center(&x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_open_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_last_opened);
}

static void test_time_picker_keyboard_navigation_and_adjustment(void)
{
    setup_time_picker();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE, egui_view_time_picker_get_focused_segment(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_time_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD, egui_view_time_picker_get_focused_segment(EGUI_VIEW_OF(&test_time_picker)));

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR, egui_view_time_picker_get_focused_segment(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD, egui_view_time_picker_get_focused_segment(EGUI_VIEW_OF(&test_time_picker)));

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_time_picker_get_hour24(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_time_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(20, g_last_hour24);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(8, egui_view_time_picker_get_hour24(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_time_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE, egui_view_time_picker_get_focused_segment(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(15, egui_view_time_picker_get_minute(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_time_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(15, g_last_minute);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_time_picker_get_minute(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, g_time_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_open_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_opened);

    EGUI_TEST_ASSERT_TRUE(send_key_action(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ESCAPE));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_open_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_last_opened);

    EGUI_TEST_ASSERT_FALSE(send_key_action(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ESCAPE));
}

static void test_time_picker_compact_read_only_and_disabled_guards(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_time_picker();
    layout_time_picker(194, 126);
    get_field_center(&x, &y);

    egui_view_time_picker_set_read_only_mode(EGUI_VIEW_OF(&test_time_picker), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));

    egui_view_time_picker_set_read_only_mode(EGUI_VIEW_OF(&test_time_picker), 0);
    egui_view_time_picker_set_compact_mode(EGUI_VIEW_OF(&test_time_picker), 1);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));

    egui_view_time_picker_set_opened(EGUI_VIEW_OF(&test_time_picker), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));

    egui_view_time_picker_set_compact_mode(EGUI_VIEW_OF(&test_time_picker), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_time_picker), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
    egui_view_time_picker_set_opened(EGUI_VIEW_OF(&test_time_picker), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_time_picker_get_opened(EGUI_VIEW_OF(&test_time_picker)));
}

void test_time_picker_run(void)
{
    EGUI_TEST_SUITE_BEGIN(time_picker);
    EGUI_TEST_RUN(test_time_picker_time_step_and_listener_guards);
    EGUI_TEST_RUN(test_time_picker_font_palette_and_helpers);
    EGUI_TEST_RUN(test_time_picker_metrics_and_hit_testing);
    EGUI_TEST_RUN(test_time_picker_touch_toggle_and_panel_selection);
    EGUI_TEST_RUN(test_time_picker_keyboard_navigation_and_adjustment);
    EGUI_TEST_RUN(test_time_picker_compact_read_only_and_disabled_guards);
    EGUI_TEST_SUITE_END();
}
