#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_date_picker.h"

#include "../../HelloCustomWidgets/input/date_picker/egui_view_date_picker.h"
#include "../../HelloCustomWidgets/input/date_picker/egui_view_date_picker.c"

static egui_view_date_picker_t test_date_picker;
static uint8_t g_date_changed_count;
static uint16_t g_last_year;
static uint8_t g_last_month;
static uint8_t g_last_day;
static uint8_t g_open_changed_count;
static uint8_t g_last_opened;
static uint8_t g_display_changed_count;
static uint16_t g_last_display_year;
static uint8_t g_last_display_month;

static void on_date_changed(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day)
{
    EGUI_UNUSED(self);
    g_date_changed_count++;
    g_last_year = year;
    g_last_month = month;
    g_last_day = day;
}

static void on_open_changed(egui_view_t *self, uint8_t opened)
{
    EGUI_UNUSED(self);
    g_open_changed_count++;
    g_last_opened = opened;
}

static void on_display_month_changed(egui_view_t *self, uint16_t year, uint8_t month)
{
    EGUI_UNUSED(self);
    g_display_changed_count++;
    g_last_display_year = year;
    g_last_display_month = month;
}

static void reset_listener_state(void)
{
    g_date_changed_count = 0;
    g_last_year = 0;
    g_last_month = 0;
    g_last_day = 0;
    g_open_changed_count = 0;
    g_last_opened = 0xFF;
    g_display_changed_count = 0;
    g_last_display_year = 0;
    g_last_display_month = 0;
}

static void setup_date_picker(void)
{
    egui_view_date_picker_init(EGUI_VIEW_OF(&test_date_picker));
    egui_view_set_size(EGUI_VIEW_OF(&test_date_picker), 194, 180);
    egui_view_date_picker_set_label(EGUI_VIEW_OF(&test_date_picker), "Ship date");
    egui_view_date_picker_set_helper(EGUI_VIEW_OF(&test_date_picker), "Mon first week, tap days");
    egui_view_date_picker_set_today(EGUI_VIEW_OF(&test_date_picker), 2026, 3, 15);
    egui_view_date_picker_set_first_day_of_week(EGUI_VIEW_OF(&test_date_picker), 1);
    egui_view_date_picker_set_on_date_changed_listener(EGUI_VIEW_OF(&test_date_picker), on_date_changed);
    egui_view_date_picker_set_on_open_changed_listener(EGUI_VIEW_OF(&test_date_picker), on_open_changed);
    egui_view_date_picker_set_on_display_month_changed_listener(EGUI_VIEW_OF(&test_date_picker), on_display_month_changed);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&test_date_picker), 1);
#endif
    reset_listener_state();
}

static void layout_date_picker(egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_date_picker), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_date_picker)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_date_picker)->api->on_touch_event(EGUI_VIEW_OF(&test_date_picker), &event);
}

static int send_key_action(uint8_t type, uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key_code = key_code;
    return EGUI_VIEW_OF(&test_date_picker)->api->on_key_event(EGUI_VIEW_OF(&test_date_picker), &event);
}

static int send_key(uint8_t key_code)
{
    int handled = 0;

    handled |= send_key_action(EGUI_KEY_EVENT_ACTION_DOWN, key_code);
    handled |= send_key_action(EGUI_KEY_EVENT_ACTION_UP, key_code);
    return handled;
}

static void get_metrics(egui_view_date_picker_metrics_t *metrics)
{
    date_picker_get_metrics(&test_date_picker, EGUI_VIEW_OF(&test_date_picker), metrics);
}

static void get_field_center(egui_dim_t *x, egui_dim_t *y)
{
    egui_view_date_picker_metrics_t metrics;

    get_metrics(&metrics);
    *x = metrics.field_region.location.x + metrics.field_region.size.width / 2;
    *y = metrics.field_region.location.y + metrics.field_region.size.height / 2;
}

static void get_prev_center(egui_dim_t *x, egui_dim_t *y)
{
    egui_view_date_picker_metrics_t metrics;

    get_metrics(&metrics);
    *x = metrics.panel_prev_region.location.x + metrics.panel_prev_region.size.width / 2;
    *y = metrics.panel_prev_region.location.y + metrics.panel_prev_region.size.height / 2;
}

static void get_next_center(egui_dim_t *x, egui_dim_t *y)
{
    egui_view_date_picker_metrics_t metrics;

    get_metrics(&metrics);
    *x = metrics.panel_next_region.location.x + metrics.panel_next_region.size.width / 2;
    *y = metrics.panel_next_region.location.y + metrics.panel_next_region.size.height / 2;
}

static void get_day_center(uint8_t day, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_date_picker_metrics_t metrics;
    uint8_t start_cell;
    uint8_t pos;
    uint8_t col;
    uint8_t row;
    egui_dim_t cell_w;
    egui_dim_t cell_h;

    get_metrics(&metrics);
    start_cell = date_picker_get_start_cell(&test_date_picker);
    pos = (uint8_t)(start_cell + day - 1);
    col = (uint8_t)(pos % 7);
    row = (uint8_t)(pos / 7);
    cell_w = metrics.panel_grid_region.size.width / 7;
    cell_h = metrics.panel_grid_region.size.height / 6;
    *x = metrics.panel_grid_region.location.x + col * cell_w + cell_w / 2;
    *y = metrics.panel_grid_region.location.y + row * cell_h + cell_h / 2;
}

static void test_date_picker_setters_and_listener_guards(void)
{
    setup_date_picker();

    egui_view_date_picker_set_date(EGUI_VIEW_OF(&test_date_picker), 2026, 2, 30);
    EGUI_TEST_ASSERT_EQUAL_INT(2026, egui_view_date_picker_get_year(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_date_picker_get_month(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(28, egui_view_date_picker_get_day(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_date_changed_count);

    egui_view_date_picker_set_today(EGUI_VIEW_OF(&test_date_picker), 2026, 2, 30);
    EGUI_TEST_ASSERT_EQUAL_INT(2026, test_date_picker.today_year);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_date_picker.today_month);
    EGUI_TEST_ASSERT_EQUAL_INT(28, test_date_picker.today_day);

    egui_view_date_picker_set_first_day_of_week(EGUI_VIEW_OF(&test_date_picker), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_date_picker_get_first_day_of_week(EGUI_VIEW_OF(&test_date_picker)));
    egui_view_date_picker_set_first_day_of_week(EGUI_VIEW_OF(&test_date_picker), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_date_picker_get_first_day_of_week(EGUI_VIEW_OF(&test_date_picker)));

    egui_view_date_picker_set_display_month(EGUI_VIEW_OF(&test_date_picker), 2027, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_display_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2027, g_last_display_year);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_display_month);
    EGUI_TEST_ASSERT_EQUAL_INT(2027, egui_view_date_picker_get_display_year(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_date_picker_get_display_month(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_date_picker.preserve_display_month_on_open);

    egui_view_date_picker_set_opened(EGUI_VIEW_OF(&test_date_picker), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2027, egui_view_date_picker_get_display_year(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_date_picker_get_display_month(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_date_picker.preserve_display_month_on_open);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_open_changed_count);

    egui_view_date_picker_set_opened(EGUI_VIEW_OF(&test_date_picker), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2026, egui_view_date_picker_get_display_year(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_date_picker_get_display_month(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_open_changed_count);
}

static void test_date_picker_font_palette_and_internal_helpers(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    uint16_t year;
    uint8_t month;
    uint8_t day;
    char buffer[16];

    setup_date_picker();

    egui_view_date_picker_set_font(EGUI_VIEW_OF(&test_date_picker), NULL);
    egui_view_date_picker_set_meta_font(EGUI_VIEW_OF(&test_date_picker), NULL);
    EGUI_TEST_ASSERT_TRUE(test_date_picker.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_date_picker.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_date_picker_set_palette(EGUI_VIEW_OF(&test_date_picker), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                      EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_date_picker.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_date_picker.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_date_picker.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_date_picker.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_date_picker.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_date_picker.today_color.full);

    EGUI_TEST_ASSERT_EQUAL_INT(29, date_picker_days_in_month(2024, 2));
    EGUI_TEST_ASSERT_EQUAL_INT(28, date_picker_days_in_month(2025, 2));
    EGUI_TEST_ASSERT_EQUAL_INT(0, date_picker_day_of_week(2026, 3, 1));

    year = 2026;
    month = 13;
    day = 40;
    date_picker_normalize_date(&year, &month, &day);
    EGUI_TEST_ASSERT_EQUAL_INT(2026, year);
    EGUI_TEST_ASSERT_EQUAL_INT(12, month);
    EGUI_TEST_ASSERT_EQUAL_INT(31, day);

    year = 0;
    month = 0;
    date_picker_normalize_display_month(&year, &month);
    EGUI_TEST_ASSERT_EQUAL_INT(1, year);
    EGUI_TEST_ASSERT_EQUAL_INT(1, month);

    date_picker_format_date_field(2026, 3, 18, 0, buffer);
    EGUI_TEST_ASSERT_TRUE(strcmp(buffer, "2026-03-18") == 0);
    date_picker_format_date_field(2026, 3, 18, 1, buffer);
    EGUI_TEST_ASSERT_TRUE(strcmp(buffer, "Mar 18") == 0);
    date_picker_format_month_title(2027, 1, buffer);
    EGUI_TEST_ASSERT_TRUE(strcmp(buffer, "Jan 2027") == 0);

    test_date_picker.year = 2026;
    test_date_picker.month = 3;
    test_date_picker.day = 31;
    test_date_picker.panel_year = 2026;
    test_date_picker.panel_month = 4;
    test_date_picker.first_day_of_week = 1;
    EGUI_TEST_ASSERT_EQUAL_INT(2, date_picker_get_start_cell(&test_date_picker));
    EGUI_TEST_ASSERT_EQUAL_INT(30, date_picker_get_display_anchor_day(&test_date_picker));
    test_date_picker.panel_month = 3;
    EGUI_TEST_ASSERT_EQUAL_INT(0, date_picker_get_display_anchor_day(&test_date_picker));

    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, date_picker_mix_disabled(sample).full);
}

static void test_date_picker_metrics_and_hit_testing(void)
{
    egui_view_date_picker_metrics_t metrics;
    egui_dim_t x;
    egui_dim_t y;

    setup_date_picker();
    egui_view_date_picker_set_opened(EGUI_VIEW_OF(&test_date_picker), 1);
    layout_date_picker(194, 180);
    get_metrics(&metrics);

    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_label);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_helper);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_panel);
    EGUI_TEST_ASSERT_TRUE(metrics.field_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.panel_grid_region.size.height > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.chevron_region.size.width > 0);

    get_field_center(&x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATE_PICKER_PART_FIELD, date_picker_hit_part(&test_date_picker, EGUI_VIEW_OF(&test_date_picker), x, y));

    get_prev_center(&x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATE_PICKER_PART_PREV, date_picker_hit_part(&test_date_picker, EGUI_VIEW_OF(&test_date_picker), x, y));

    get_next_center(&x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATE_PICKER_PART_NEXT, date_picker_hit_part(&test_date_picker, EGUI_VIEW_OF(&test_date_picker), x, y));

    get_day_center(18, &x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATE_PICKER_PART_DAY, date_picker_hit_part(&test_date_picker, EGUI_VIEW_OF(&test_date_picker), x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(18, date_picker_get_hit_day(&test_date_picker, EGUI_VIEW_OF(&test_date_picker), x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, date_picker_get_hit_day(&test_date_picker, EGUI_VIEW_OF(&test_date_picker), metrics.panel_grid_region.location.x + 1,
                                                          metrics.panel_grid_region.location.y + 1));

    egui_view_date_picker_set_read_only_mode(EGUI_VIEW_OF(&test_date_picker), 1);
    layout_date_picker(194, 180);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.show_panel);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.chevron_region.size.width);

    egui_view_date_picker_set_read_only_mode(EGUI_VIEW_OF(&test_date_picker), 0);
    egui_view_date_picker_set_compact_mode(EGUI_VIEW_OF(&test_date_picker), 1);
    egui_view_set_size(EGUI_VIEW_OF(&test_date_picker), 106, 48);
    layout_date_picker(106, 48);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.show_label);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.show_helper);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.show_panel);
}

static void test_date_picker_touch_toggle_and_day_selection(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_date_picker();
    layout_date_picker(194, 180);
    get_field_center(&x, &y);

    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 0, 0));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATE_PICKER_PART_FIELD, test_date_picker.pressed_part);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_date_picker)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_open_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_opened);

    get_prev_center(&x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_date_picker_get_display_month(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_display_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2026, g_last_display_year);
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_last_display_month);

    get_next_center(&x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_date_picker_get_display_month(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_display_changed_count);

    get_day_center(24, &x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATE_PICKER_PART_DAY, test_date_picker.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(24, test_date_picker.pressed_day);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(24, egui_view_date_picker_get_day(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_date_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2026, g_last_year);
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_last_month);
    EGUI_TEST_ASSERT_EQUAL_INT(24, g_last_day);

    get_day_center(25, &x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 0, 0));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_date_picker.pressed_day);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, 0, 0));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATE_PICKER_PART_NONE, test_date_picker.pressed_part);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_date_picker)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_date_changed_count);

    get_field_center(&x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_open_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_last_opened);
}

static void test_date_picker_keyboard_navigation_and_browse_commit(void)
{
    setup_date_picker();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(17, egui_view_date_picker_get_day(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_date_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(18, egui_view_date_picker_get_day(EGUI_VIEW_OF(&test_date_picker)));

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(11, egui_view_date_picker_get_day(EGUI_VIEW_OF(&test_date_picker)));

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(18, egui_view_date_picker_get_day(EGUI_VIEW_OF(&test_date_picker)));

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_open_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_PLUS));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_date_picker_get_display_month(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_display_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(2026, egui_view_date_picker_get_year(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_date_picker_get_month(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(18, egui_view_date_picker_get_day(EGUI_VIEW_OF(&test_date_picker)));

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_date_picker_get_day(EGUI_VIEW_OF(&test_date_picker)));

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_date_picker_get_day(EGUI_VIEW_OF(&test_date_picker)));

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_open_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_last_opened);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_TRUE(send_key_action(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ESCAPE));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_FALSE(send_key_action(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ESCAPE));
}

static void test_date_picker_compact_read_only_disabled_and_focus_guards(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_date_picker();
    layout_date_picker(194, 180);
    get_field_center(&x, &y);

    egui_view_date_picker_set_read_only_mode(EGUI_VIEW_OF(&test_date_picker), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));

    setup_date_picker();
    egui_view_date_picker_set_compact_mode(EGUI_VIEW_OF(&test_date_picker), 1);
    egui_view_set_size(EGUI_VIEW_OF(&test_date_picker), 106, 48);
    layout_date_picker(106, 48);
    get_field_center(&x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));
    egui_view_date_picker_set_opened(EGUI_VIEW_OF(&test_date_picker), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));

    setup_date_picker();
    layout_date_picker(194, 180);
    get_field_center(&x, &y);
    egui_view_set_enable(EGUI_VIEW_OF(&test_date_picker), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
    egui_view_date_picker_set_opened(EGUI_VIEW_OF(&test_date_picker), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    setup_date_picker();
    date_picker_set_open_inner(EGUI_VIEW_OF(&test_date_picker), 1, 1);
    test_date_picker.pressed_part = EGUI_VIEW_DATE_PICKER_PART_FIELD;
    test_date_picker.pressed_day = 18;
    egui_view_set_pressed(EGUI_VIEW_OF(&test_date_picker), true);
    egui_view_date_picker_on_focus_change(EGUI_VIEW_OF(&test_date_picker), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_date_picker_get_opened(EGUI_VIEW_OF(&test_date_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATE_PICKER_PART_NONE, test_date_picker.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_date_picker.pressed_day);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_date_picker)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_open_changed_count);
#endif
}

void test_date_picker_run(void)
{
    EGUI_TEST_SUITE_BEGIN(date_picker);
    EGUI_TEST_RUN(test_date_picker_setters_and_listener_guards);
    EGUI_TEST_RUN(test_date_picker_font_palette_and_internal_helpers);
    EGUI_TEST_RUN(test_date_picker_metrics_and_hit_testing);
    EGUI_TEST_RUN(test_date_picker_touch_toggle_and_day_selection);
    EGUI_TEST_RUN(test_date_picker_keyboard_navigation_and_browse_commit);
    EGUI_TEST_RUN(test_date_picker_compact_read_only_disabled_and_focus_guards);
    EGUI_TEST_SUITE_END();
}
