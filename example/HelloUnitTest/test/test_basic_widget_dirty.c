#include <string.h>

#include "egui.h"
#include "background/egui_background_color.h"
#include "test/egui_test.h"
#include "test_basic_widget_dirty.h"

static egui_view_slider_t test_slider;
static egui_view_progress_bar_t test_progress_bar;
static egui_view_page_indicator_t test_indicator;
static egui_view_checkbox_t test_checkbox;
static egui_view_radio_button_t test_radio_button;
static egui_view_led_t test_led;
static egui_view_button_matrix_t test_button_matrix;
static egui_view_mini_calendar_t test_calendar;
static egui_view_number_picker_t test_picker;
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static egui_view_textinput_t test_textinput;
#endif
static egui_view_stopwatch_t test_stopwatch;

static const char *const s_indicator_icons[] = {"a", "b", "c", "d", "e"};
static const char *s_button_matrix_labels[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I"};

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_pressed_bg_normal_param, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_pressed_bg_pressed_param, EGUI_COLOR_RED, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_pressed_bg_params, &s_pressed_bg_normal_param, &s_pressed_bg_pressed_param, NULL);
static egui_background_color_t s_pressed_background;
static uint8_t s_pressed_background_ready;

static int32_t region_area(const egui_region_t *region)
{
    return (int32_t)region->size.width * region->size.height;
}

static void collect_dirty_union(egui_region_t *out_region, uint8_t *out_count)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();
    uint8_t count = 0;
    uint8_t i;

    egui_region_init_empty(out_region);
    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if (egui_region_is_empty(&arr[i]))
        {
            continue;
        }

        if (count == 0)
        {
            egui_region_copy(out_region, &arr[i]);
        }
        else
        {
            egui_region_union(out_region, &arr[i], out_region);
        }
        count++;
    }

    if (out_count != NULL)
    {
        *out_count = count;
    }
}

static void assert_partial_dirty_region(const egui_view_t *view)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();
    int32_t dirty_area = region_area(&arr[0]);
    int32_t full_area = region_area(&view->region_screen);

    EGUI_TEST_ASSERT_FALSE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_FALSE(egui_region_equal(&arr[0], &view->region_screen));
    EGUI_TEST_ASSERT_TRUE(dirty_area < full_area);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void assert_full_dirty_region(const egui_view_t *view)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    EGUI_TEST_ASSERT_REGION_EQUAL(&view->region_screen, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void assert_dirty_area_less_than(const egui_view_t *view, int32_t area_limit)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    EGUI_TEST_ASSERT_FALSE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_TRUE(region_area(&arr[0]) < area_limit);
}

static void setup_slider(void)
{
    egui_region_t region;

    egui_view_slider_init(EGUI_VIEW_OF(&test_slider));
    egui_view_set_size(EGUI_VIEW_OF(&test_slider), 160, 30);

    egui_region_init(&region, 10, 20, 160, 30);
    egui_view_layout(EGUI_VIEW_OF(&test_slider), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_slider)->region_screen, &region);
}

static void setup_progress_bar(void)
{
    egui_region_t region;

    egui_view_progress_bar_init(EGUI_VIEW_OF(&test_progress_bar));
    egui_view_set_size(EGUI_VIEW_OF(&test_progress_bar), 160, 24);

    egui_region_init(&region, 10, 20, 160, 24);
    egui_view_layout(EGUI_VIEW_OF(&test_progress_bar), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_progress_bar)->region_screen, &region);
}

static void setup_page_indicator(void)
{
    egui_region_t region;

    egui_view_page_indicator_init(EGUI_VIEW_OF(&test_indicator));
    egui_view_set_size(EGUI_VIEW_OF(&test_indicator), 180, 24);
    egui_view_page_indicator_set_total_count(EGUI_VIEW_OF(&test_indicator), 5);

    egui_region_init(&region, 10, 20, 180, 24);
    egui_view_layout(EGUI_VIEW_OF(&test_indicator), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_indicator)->region_screen, &region);
}

static void setup_checkbox(void)
{
    egui_region_t region;

    egui_view_checkbox_init(EGUI_VIEW_OF(&test_checkbox));
    egui_view_set_size(EGUI_VIEW_OF(&test_checkbox), 160, 30);
    egui_view_checkbox_set_text(EGUI_VIEW_OF(&test_checkbox), "Checked");

    egui_region_init(&region, 10, 20, 160, 30);
    egui_view_layout(EGUI_VIEW_OF(&test_checkbox), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_checkbox)->region_screen, &region);
}

static void setup_radio_button(void)
{
    egui_region_t region;

    egui_view_radio_button_init(EGUI_VIEW_OF(&test_radio_button));
    egui_view_set_size(EGUI_VIEW_OF(&test_radio_button), 160, 30);
    egui_view_radio_button_set_text(EGUI_VIEW_OF(&test_radio_button), "Option");

    egui_region_init(&region, 10, 20, 160, 30);
    egui_view_layout(EGUI_VIEW_OF(&test_radio_button), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_radio_button)->region_screen, &region);
}

static void setup_led(void)
{
    egui_region_t region;

    egui_view_led_init(EGUI_VIEW_OF(&test_led));
    egui_view_set_size(EGUI_VIEW_OF(&test_led), 90, 40);

    egui_region_init(&region, 10, 20, 90, 40);
    egui_view_layout(EGUI_VIEW_OF(&test_led), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_led)->region_screen, &region);
}

static void setup_button_matrix(void)
{
    egui_region_t region;

    egui_view_button_matrix_init(EGUI_VIEW_OF(&test_button_matrix));
    egui_view_set_size(EGUI_VIEW_OF(&test_button_matrix), 180, 120);
    egui_view_button_matrix_set_labels(EGUI_VIEW_OF(&test_button_matrix), s_button_matrix_labels, EGUI_ARRAY_SIZE(s_button_matrix_labels), 3);
    egui_view_button_matrix_set_selection_enabled(EGUI_VIEW_OF(&test_button_matrix), 1);
    egui_view_button_matrix_set_selected_index(EGUI_VIEW_OF(&test_button_matrix), 0);

    egui_region_init(&region, 10, 20, 180, 120);
    egui_view_layout(EGUI_VIEW_OF(&test_button_matrix), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_button_matrix)->region_screen, &region);
}

static void setup_mini_calendar(void)
{
    egui_region_t region;

    egui_view_mini_calendar_init(EGUI_VIEW_OF(&test_calendar));
    egui_view_set_size(EGUI_VIEW_OF(&test_calendar), 210, 160);
    egui_view_mini_calendar_set_date(EGUI_VIEW_OF(&test_calendar), 2026, 3, 5);

    egui_region_init(&region, 10, 20, 210, 160);
    egui_view_layout(EGUI_VIEW_OF(&test_calendar), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_calendar)->region_screen, &region);
}

static void setup_number_picker(void)
{
    egui_region_t region;

    egui_view_number_picker_init(EGUI_VIEW_OF(&test_picker));
    egui_view_set_size(EGUI_VIEW_OF(&test_picker), 60, 90);
    egui_view_number_picker_set_range(EGUI_VIEW_OF(&test_picker), 0, 100);
    egui_view_number_picker_set_value(EGUI_VIEW_OF(&test_picker), 10);
    egui_view_number_picker_set_step(EGUI_VIEW_OF(&test_picker), 5);

    egui_region_init(&region, 10, 20, 60, 90);
    egui_view_layout(EGUI_VIEW_OF(&test_picker), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_picker)->region_screen, &region);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void setup_textinput(void)
{
    egui_region_t region;

    egui_view_textinput_init(EGUI_VIEW_OF(&test_textinput));
    egui_view_set_size(EGUI_VIEW_OF(&test_textinput), 180, 32);
    egui_view_textinput_set_font(EGUI_VIEW_OF(&test_textinput), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_textinput_set_text(EGUI_VIEW_OF(&test_textinput), "Hello");

    egui_region_init(&region, 10, 20, 180, 32);
    egui_view_layout(EGUI_VIEW_OF(&test_textinput), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_textinput)->region_screen, &region);

    EGUI_VIEW_OF(&test_textinput)->is_focused = true;
    test_textinput.cursor_visible = 1;
}
#endif

static void setup_stopwatch(void)
{
    egui_region_t region;

    egui_view_stopwatch_init(EGUI_VIEW_OF(&test_stopwatch));
    egui_view_set_size(EGUI_VIEW_OF(&test_stopwatch), 160, 32);

    egui_region_init(&region, 10, 20, 160, 32);
    egui_view_layout(EGUI_VIEW_OF(&test_stopwatch), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_stopwatch)->region_screen, &region);
}

static void attach_pressed_background(egui_view_t *view)
{
    if (!s_pressed_background_ready)
    {
        egui_background_color_init_with_params((egui_background_t *)&s_pressed_background, &s_pressed_bg_params);
        s_pressed_background_ready = 1;
    }

    egui_view_set_background(view, (egui_background_t *)&s_pressed_background.base);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void send_slider_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;

    EGUI_VIEW_OF(&test_slider)->api->on_touch_event(EGUI_VIEW_OF(&test_slider), &event);
}

static void get_slider_thumb_center(egui_dim_t *x, egui_dim_t *y)
{
    egui_dim_t thumb_radius = (EGUI_VIEW_OF(&test_slider)->region.size.height / 2) - 1;
    egui_dim_t thumb_margin;
    egui_dim_t usable_width;
    egui_dim_t start_x;

    if (thumb_radius > EGUI_THEME_RADIUS_LG)
    {
        thumb_radius = EGUI_THEME_RADIUS_LG;
    }
    if (thumb_radius < EGUI_THEME_RADIUS_SM)
    {
        thumb_radius = EGUI_THEME_RADIUS_SM;
    }

    thumb_margin = thumb_radius + 1;
    usable_width = EGUI_VIEW_OF(&test_slider)->region.size.width - 2 * thumb_margin;
    start_x = EGUI_VIEW_OF(&test_slider)->region_screen.location.x + thumb_margin;

    *x = start_x + (egui_dim_t)((uint32_t)usable_width * test_slider.value / 100);
    *y = EGUI_VIEW_OF(&test_slider)->region_screen.location.y + EGUI_VIEW_OF(&test_slider)->region.size.height / 2;
}

static void send_button_matrix_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;

    EGUI_VIEW_OF(&test_button_matrix)->api->on_touch_event(EGUI_VIEW_OF(&test_button_matrix), &event);
}

static uint8_t test_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint8_t day_count = days[month - 1];

    if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
    {
        day_count = 29;
    }
    return day_count;
}

static uint8_t test_day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint16_t y = year;

    if (month < 3)
    {
        y--;
    }
    return (uint8_t)((y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7);
}

static void get_calendar_day_screen_region(uint8_t day, egui_region_t *region)
{
    egui_dim_t cell_w = EGUI_VIEW_OF(&test_calendar)->region.size.width / 7;
    egui_dim_t header_h = EGUI_VIEW_OF(&test_calendar)->region.size.height / 8;
    egui_dim_t cell_h = (EGUI_VIEW_OF(&test_calendar)->region.size.height - header_h * 2) / 6;
    uint8_t start_col = (uint8_t)((test_day_of_week(test_calendar.year, test_calendar.month, 1) - test_calendar.first_day_of_week + 7) % 7);
    uint8_t total_days = test_days_in_month(test_calendar.year, test_calendar.month);
    uint8_t pos;
    uint8_t col;
    uint8_t row;

    EGUI_TEST_ASSERT_TRUE(day >= 1);
    EGUI_TEST_ASSERT_TRUE(day <= total_days);

    pos = (uint8_t)(start_col + day - 1);
    col = (uint8_t)(pos % 7);
    row = (uint8_t)(pos / 7);

    egui_region_init(region, EGUI_VIEW_OF(&test_calendar)->region_screen.location.x + col * cell_w,
                     EGUI_VIEW_OF(&test_calendar)->region_screen.location.y + header_h * 2 + row * cell_h, cell_w, cell_h);
}

static void get_calendar_day_center(uint8_t day, egui_dim_t *x, egui_dim_t *y)
{
    egui_region_t day_region;

    get_calendar_day_screen_region(day, &day_region);
    *x = day_region.location.x + day_region.size.width / 2;
    *y = day_region.location.y + day_region.size.height / 2;
}

static void send_calendar_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;

    EGUI_VIEW_OF(&test_calendar)->api->on_touch_event(EGUI_VIEW_OF(&test_calendar), &event);
}

static void get_number_picker_zone_screen_region(int8_t zone, egui_region_t *region)
{
    egui_dim_t third_h = EGUI_VIEW_OF(&test_picker)->region.size.height / 3;
    egui_dim_t x = EGUI_VIEW_OF(&test_picker)->region_screen.location.x;
    egui_dim_t y = EGUI_VIEW_OF(&test_picker)->region_screen.location.y;
    egui_dim_t width = EGUI_VIEW_OF(&test_picker)->region.size.width;
    egui_dim_t height = EGUI_VIEW_OF(&test_picker)->region.size.height;

    if (zone > 0)
    {
        egui_region_init(region, x, y, width, third_h);
    }
    else if (zone < 0)
    {
        egui_region_init(region, x, y + height - third_h, width, third_h);
    }
    else
    {
        egui_region_init(region, x, y + third_h, width, height - third_h * 2);
    }
}

static void get_number_picker_zone_center(int8_t zone, egui_dim_t *x, egui_dim_t *y)
{
    egui_region_t zone_region;

    get_number_picker_zone_screen_region(zone, &zone_region);
    *x = zone_region.location.x + zone_region.size.width / 2;
    *y = zone_region.location.y + zone_region.size.height / 2;
}

static void send_number_picker_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;

    EGUI_VIEW_OF(&test_picker)->api->on_touch_event(EGUI_VIEW_OF(&test_picker), &event);
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static egui_dim_t get_text_width_to_pos(const egui_view_textinput_t *textinput, uint8_t pos)
{
    char tmp[EGUI_CONFIG_TEXTINPUT_MAX_LENGTH + 1];
    egui_dim_t width = 0;
    egui_dim_t height = 0;
    uint8_t len = pos;

    if (textinput->font == NULL || pos == 0)
    {
        return 0;
    }

    if (len > textinput->text_len)
    {
        len = textinput->text_len;
    }

    memcpy(tmp, textinput->text, len);
    tmp[len] = '\0';
    textinput->font->api->get_str_size(textinput->font, tmp, 0, 0, &width, &height);
    return width;
}

static void get_textinput_cursor_screen_region(uint8_t pos, egui_region_t *region)
{
    egui_dim_t cursor_width = 1;
    egui_dim_t cursor_height = 0;
    egui_dim_t dummy_width = 0;
    egui_dim_t cursor_x;
    egui_dim_t cursor_y;

    test_textinput.font->api->get_str_size(test_textinput.font, "A", 0, 0, &dummy_width, &cursor_height);
    cursor_x = EGUI_VIEW_OF(&test_textinput)->region_screen.location.x + get_text_width_to_pos(&test_textinput, pos) - test_textinput.scroll_offset_x;
    cursor_y = EGUI_VIEW_OF(&test_textinput)->region_screen.location.y + (EGUI_VIEW_OF(&test_textinput)->region.size.height - cursor_height) / 2;
    egui_region_init(region, cursor_x, cursor_y, cursor_width, cursor_height);
}
#endif

static void get_stopwatch_text_screen_region(const char *text, egui_region_t *region)
{
    egui_region_t work_region;
    egui_region_t work_region_screen;
    egui_dim_t text_width = 0;
    egui_dim_t text_height = 0;
    egui_dim_t offset_x = 0;
    egui_dim_t offset_y = 0;

    egui_view_get_work_region(EGUI_VIEW_OF(&test_stopwatch), &work_region);
    test_stopwatch.base.font->api->get_str_size(test_stopwatch.base.font, text, 1, test_stopwatch.base.line_space, &text_width, &text_height);
    text_width = EGUI_MIN(text_width, work_region.size.width);
    text_height = EGUI_MIN(text_height, work_region.size.height);

    egui_common_align_get_x_y(work_region.size.width, work_region.size.height, text_width, text_height, test_stopwatch.base.align_type, &offset_x, &offset_y);

    region->location.x = EGUI_VIEW_OF(&test_stopwatch)->region_screen.location.x + work_region.location.x + offset_x;
    region->location.y = EGUI_VIEW_OF(&test_stopwatch)->region_screen.location.y + work_region.location.y + offset_y;
    region->size.width = text_width;
    region->size.height = text_height;

    if (region->location.x > EGUI_VIEW_OF(&test_stopwatch)->region_screen.location.x + work_region.location.x)
    {
        region->location.x--;
        region->size.width++;
    }
    if (region->location.y > EGUI_VIEW_OF(&test_stopwatch)->region_screen.location.y + work_region.location.y)
    {
        region->location.y--;
        region->size.height++;
    }
    if (region->location.x + region->size.width < EGUI_VIEW_OF(&test_stopwatch)->region_screen.location.x + work_region.location.x + work_region.size.width)
    {
        region->size.width++;
    }
    if (region->location.y + region->size.height < EGUI_VIEW_OF(&test_stopwatch)->region_screen.location.y + work_region.location.y + work_region.size.height)
    {
        region->size.height++;
    }

    egui_region_init(&work_region_screen, EGUI_VIEW_OF(&test_stopwatch)->region_screen.location.x + work_region.location.x,
                     EGUI_VIEW_OF(&test_stopwatch)->region_screen.location.y + work_region.location.y, work_region.size.width, work_region.size.height);
    egui_region_intersect(region, &work_region_screen, region);
}

static void test_slider_value_change_uses_partial_dirty_region(void)
{
    setup_slider();
    egui_view_slider_set_value(EGUI_VIEW_OF(&test_slider), 20);

    egui_core_clear_region_dirty();
    egui_view_slider_set_value(EGUI_VIEW_OF(&test_slider), 75);

    assert_partial_dirty_region(EGUI_VIEW_OF(&test_slider));
}

static void test_slider_repeated_value_change_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_slider();
    egui_view_slider_set_value(EGUI_VIEW_OF(&test_slider), 20);

    egui_core_clear_region_dirty();
    egui_view_slider_set_value(EGUI_VIEW_OF(&test_slider), 75);
    egui_view_slider_set_value(EGUI_VIEW_OF(&test_slider), 35);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_slider));
}

static void test_progress_bar_process_change_uses_partial_dirty_region(void)
{
    setup_progress_bar();
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 20);

    egui_core_clear_region_dirty();
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 70);

    assert_partial_dirty_region(EGUI_VIEW_OF(&test_progress_bar));
}

static void test_progress_bar_repeated_process_change_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_progress_bar();
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 20);

    egui_core_clear_region_dirty();
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 70);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 35);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_progress_bar));
}

static void test_progress_bar_with_control_repeated_process_change_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_progress_bar();
    test_progress_bar.is_show_control = 1;
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 20);

    egui_core_clear_region_dirty();
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 70);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 35);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_progress_bar));
}

static void test_page_indicator_dot_index_change_uses_full_dirty_region(void)
{
    setup_page_indicator();
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&test_indicator), 1);

    egui_core_clear_region_dirty();
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&test_indicator), 3);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_indicator));
}

static void test_page_indicator_repeated_index_change_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_page_indicator();
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&test_indicator), 1);

    egui_core_clear_region_dirty();
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&test_indicator), 3);
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&test_indicator), 4);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_indicator));
}

static void test_page_indicator_icon_index_change_uses_full_dirty_region(void)
{
    setup_page_indicator();
    egui_view_page_indicator_set_mark_style(EGUI_VIEW_OF(&test_indicator), EGUI_VIEW_PAGE_INDICATOR_MARK_STYLE_ICON);
    egui_view_page_indicator_set_icons(EGUI_VIEW_OF(&test_indicator), s_indicator_icons);
    egui_view_page_indicator_set_icon_font(EGUI_VIEW_OF(&test_indicator), EGUI_FONT_ICON_MS_16);
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&test_indicator), 1);

    egui_core_clear_region_dirty();
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&test_indicator), 4);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_indicator));
}

static void test_page_indicator_icon_repeated_index_change_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_page_indicator();
    egui_view_page_indicator_set_mark_style(EGUI_VIEW_OF(&test_indicator), EGUI_VIEW_PAGE_INDICATOR_MARK_STYLE_ICON);
    egui_view_page_indicator_set_icons(EGUI_VIEW_OF(&test_indicator), s_indicator_icons);
    egui_view_page_indicator_set_icon_font(EGUI_VIEW_OF(&test_indicator), EGUI_FONT_ICON_MS_16);
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&test_indicator), 1);

    egui_core_clear_region_dirty();
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&test_indicator), 3);
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&test_indicator), 4);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_indicator));
}

static void test_checkbox_checked_change_uses_partial_dirty_region(void)
{
    setup_checkbox();

    egui_core_clear_region_dirty();
    egui_view_checkbox_set_checked(EGUI_VIEW_OF(&test_checkbox), 1);

    assert_partial_dirty_region(EGUI_VIEW_OF(&test_checkbox));
}

static void test_radio_button_checked_change_uses_partial_dirty_region(void)
{
    setup_radio_button();

    egui_core_clear_region_dirty();
    egui_view_radio_button_set_checked(EGUI_VIEW_OF(&test_radio_button), 1);

    assert_partial_dirty_region(EGUI_VIEW_OF(&test_radio_button));
}

static void test_led_on_change_uses_partial_dirty_region(void)
{
    setup_led();

    egui_core_clear_region_dirty();
    egui_view_led_set_on(EGUI_VIEW_OF(&test_led));

    assert_partial_dirty_region(EGUI_VIEW_OF(&test_led));
}

static void test_button_matrix_selected_change_uses_partial_dirty_region(void)
{
    setup_button_matrix();

    egui_core_clear_region_dirty();
    egui_view_button_matrix_set_selected_index(EGUI_VIEW_OF(&test_button_matrix), 4);

    assert_partial_dirty_region(EGUI_VIEW_OF(&test_button_matrix));
    assert_dirty_area_less_than(EGUI_VIEW_OF(&test_button_matrix), region_area(&EGUI_VIEW_OF(&test_button_matrix)->region_screen) / 2);
}

static void test_button_matrix_repeated_selected_change_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_button_matrix();

    egui_core_clear_region_dirty();
    egui_view_button_matrix_set_selected_index(EGUI_VIEW_OF(&test_button_matrix), 4);
    egui_view_button_matrix_set_selected_index(EGUI_VIEW_OF(&test_button_matrix), 8);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_button_matrix));
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_button_matrix_touch_release_uses_partial_dirty_region(void)
{
    egui_dim_t touch_x;
    egui_dim_t touch_y;

    setup_button_matrix();

    touch_x = EGUI_VIEW_OF(&test_button_matrix)->region_screen.location.x + EGUI_VIEW_OF(&test_button_matrix)->region_screen.size.width / 2;
    touch_y = EGUI_VIEW_OF(&test_button_matrix)->region_screen.location.y + EGUI_VIEW_OF(&test_button_matrix)->region_screen.size.height / 2;

    egui_core_clear_region_dirty();
    send_button_matrix_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);
    send_button_matrix_touch(EGUI_MOTION_EVENT_ACTION_UP, touch_x, touch_y);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_button_matrix));
}

static void test_slider_touch_down_uses_partial_dirty_region(void)
{
    egui_dim_t touch_x;
    egui_dim_t touch_y;

    setup_slider();
    egui_view_slider_set_value(EGUI_VIEW_OF(&test_slider), 50);
    get_slider_thumb_center(&touch_x, &touch_y);

    egui_core_clear_region_dirty();
    send_slider_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);

    assert_partial_dirty_region(EGUI_VIEW_OF(&test_slider));
    assert_dirty_area_less_than(EGUI_VIEW_OF(&test_slider), region_area(&EGUI_VIEW_OF(&test_slider)->region_screen) / 4);
}

static void test_slider_touch_up_uses_partial_dirty_region(void)
{
    egui_dim_t touch_x;
    egui_dim_t touch_y;

    setup_slider();
    egui_view_slider_set_value(EGUI_VIEW_OF(&test_slider), 50);
    get_slider_thumb_center(&touch_x, &touch_y);
    send_slider_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);

    egui_core_clear_region_dirty();
    send_slider_touch(EGUI_MOTION_EVENT_ACTION_UP, touch_x, touch_y);

    assert_partial_dirty_region(EGUI_VIEW_OF(&test_slider));
    assert_dirty_area_less_than(EGUI_VIEW_OF(&test_slider), region_area(&EGUI_VIEW_OF(&test_slider)->region_screen) / 4);
}

static void test_slider_touch_down_with_pressed_background_uses_full_dirty_region(void)
{
    egui_dim_t touch_x;
    egui_dim_t touch_y;

    setup_slider();
    attach_pressed_background(EGUI_VIEW_OF(&test_slider));
    egui_view_slider_set_value(EGUI_VIEW_OF(&test_slider), 50);
    get_slider_thumb_center(&touch_x, &touch_y);

    egui_core_clear_region_dirty();
    send_slider_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_slider));
}

static void test_slider_touch_move_then_programmatic_change_same_frame_uses_full_dirty_region(void)
{
    egui_dim_t touch_x;
    egui_dim_t touch_y;
    egui_dim_t move_x;

    setup_slider();
    egui_view_slider_set_value(EGUI_VIEW_OF(&test_slider), 50);
    get_slider_thumb_center(&touch_x, &touch_y);
    move_x = EGUI_VIEW_OF(&test_slider)->region_screen.location.x + EGUI_VIEW_OF(&test_slider)->region_screen.size.width - 4;

    egui_core_clear_region_dirty();
    send_slider_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);
    send_slider_touch(EGUI_MOTION_EVENT_ACTION_MOVE, move_x, touch_y);
    egui_view_slider_set_value(EGUI_VIEW_OF(&test_slider), 20);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_slider));
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_mini_calendar_selected_day_change_uses_partial_dirty_region(void)
{
    egui_region_t old_day_region;
    egui_region_t new_day_region;
    egui_region_t expected_union;
    egui_region_t dirty_union;
    uint8_t dirty_count = 0;
    egui_dim_t touch_x;
    egui_dim_t touch_y;

    setup_mini_calendar();
    get_calendar_day_screen_region(5, &old_day_region);
    get_calendar_day_screen_region(21, &new_day_region);
    egui_region_union(&old_day_region, &new_day_region, &expected_union);
    get_calendar_day_center(21, &touch_x, &touch_y);

    egui_core_clear_region_dirty();
    send_calendar_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);
    send_calendar_touch(EGUI_MOTION_EVENT_ACTION_UP, touch_x, touch_y);

    collect_dirty_union(&dirty_union, &dirty_count);

    EGUI_TEST_ASSERT_EQUAL_INT(21, test_calendar.day);
    EGUI_TEST_ASSERT_EQUAL_INT(2, dirty_count);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected_union, &dirty_union);
    EGUI_TEST_ASSERT_TRUE(region_area(&dirty_union) < region_area(&EGUI_VIEW_OF(&test_calendar)->region_screen));
}

static void test_mini_calendar_touch_down_with_pressed_background_uses_full_dirty_region(void)
{
    egui_dim_t touch_x;
    egui_dim_t touch_y;

    setup_mini_calendar();
    attach_pressed_background(EGUI_VIEW_OF(&test_calendar));
    get_calendar_day_center(21, &touch_x, &touch_y);

    egui_core_clear_region_dirty();
    send_calendar_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_calendar));
}

static void test_number_picker_touch_increment_uses_partial_dirty_region(void)
{
    egui_region_t top_region;
    egui_region_t middle_region;
    egui_region_t expected_union;
    egui_region_t dirty_union;
    uint8_t dirty_count = 0;
    egui_dim_t touch_x;
    egui_dim_t touch_y;

    setup_number_picker();
    get_number_picker_zone_center(1, &touch_x, &touch_y);
    send_number_picker_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);
    get_number_picker_zone_screen_region(1, &top_region);
    get_number_picker_zone_screen_region(0, &middle_region);
    egui_region_union(&top_region, &middle_region, &expected_union);

    egui_core_clear_region_dirty();
    send_number_picker_touch(EGUI_MOTION_EVENT_ACTION_UP, touch_x, touch_y);

    collect_dirty_union(&dirty_union, &dirty_count);

    EGUI_TEST_ASSERT_EQUAL_INT(15, egui_view_number_picker_get_value(EGUI_VIEW_OF(&test_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, dirty_count);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected_union, &dirty_union);
    EGUI_TEST_ASSERT_TRUE(region_area(&dirty_union) < region_area(&EGUI_VIEW_OF(&test_picker)->region_screen));
}

static void test_number_picker_touch_down_with_pressed_background_uses_full_dirty_region(void)
{
    egui_dim_t touch_x;
    egui_dim_t touch_y;

    setup_number_picker();
    attach_pressed_background(EGUI_VIEW_OF(&test_picker));
    get_number_picker_zone_center(1, &touch_x, &touch_y);

    egui_core_clear_region_dirty();
    send_number_picker_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_picker));
}
#endif

static void test_number_picker_value_change_uses_middle_zone_dirty_region(void)
{
    egui_region_t expected_region;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_number_picker();
    get_number_picker_zone_screen_region(0, &expected_region);

    egui_core_clear_region_dirty();
    egui_view_number_picker_set_value(EGUI_VIEW_OF(&test_picker), 15);

    EGUI_TEST_ASSERT_REGION_EQUAL(&expected_region, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
    EGUI_TEST_ASSERT_TRUE(region_area(&arr[0]) < region_area(&EGUI_VIEW_OF(&test_picker)->region_screen));
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void test_textinput_cursor_move_uses_partial_dirty_region(void)
{
    egui_region_t old_cursor_region;
    egui_region_t new_cursor_region;
    egui_region_t expected_union;
    egui_region_t dirty_union;
    uint8_t dirty_count = 0;

    setup_textinput();
    get_textinput_cursor_screen_region(test_textinput.cursor_pos, &old_cursor_region);
    get_textinput_cursor_screen_region(2, &new_cursor_region);
    egui_region_union(&old_cursor_region, &new_cursor_region, &expected_union);

    egui_core_clear_region_dirty();
    egui_view_textinput_set_cursor_pos(EGUI_VIEW_OF(&test_textinput), 2);

    collect_dirty_union(&dirty_union, &dirty_count);

    EGUI_TEST_ASSERT_EQUAL_INT(2, test_textinput.cursor_pos);
    EGUI_TEST_ASSERT_EQUAL_INT(2, dirty_count);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected_union, &dirty_union);
    EGUI_TEST_ASSERT_TRUE(region_area(&dirty_union) < region_area(&EGUI_VIEW_OF(&test_textinput)->region_screen));
}

static void test_textinput_cursor_blink_uses_partial_dirty_region(void)
{
    egui_region_t expected_region;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_textinput();
    get_textinput_cursor_screen_region(test_textinput.cursor_pos, &expected_region);

    egui_core_clear_region_dirty();
    test_textinput.cursor_timer.callback(&test_textinput.cursor_timer);

    EGUI_TEST_ASSERT_REGION_EQUAL(&expected_region, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
    EGUI_TEST_ASSERT_TRUE(region_area(&arr[0]) < region_area(&EGUI_VIEW_OF(&test_textinput)->region_screen));
}
#endif

static void test_stopwatch_elapsed_change_uses_partial_dirty_region(void)
{
    egui_region_t old_text_region;
    egui_region_t new_text_region;
    egui_region_t expected_region;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_stopwatch();
    get_stopwatch_text_screen_region(test_stopwatch.base.text, &old_text_region);

    egui_core_clear_region_dirty();
    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&test_stopwatch), 12340);
    get_stopwatch_text_screen_region(test_stopwatch.base.text, &new_text_region);
    egui_region_union(&old_text_region, &new_text_region, &expected_region);

    EGUI_TEST_ASSERT_REGION_EQUAL(&expected_region, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
    EGUI_TEST_ASSERT_TRUE(region_area(&arr[0]) < region_area(&EGUI_VIEW_OF(&test_stopwatch)->region_screen));
}

static void test_stopwatch_repeated_elapsed_change_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_stopwatch();
    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&test_stopwatch), 12340);

    egui_core_clear_region_dirty();
    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&test_stopwatch), 12350);
    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&test_stopwatch), 12400);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_stopwatch));
}

void test_basic_widget_dirty_run(void)
{
    EGUI_TEST_SUITE_BEGIN(basic_widget_dirty);
    EGUI_TEST_RUN(test_slider_value_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_slider_repeated_value_change_same_frame_falls_back_to_full_dirty_region);
    EGUI_TEST_RUN(test_progress_bar_process_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_progress_bar_repeated_process_change_same_frame_falls_back_to_full_dirty_region);
    EGUI_TEST_RUN(test_progress_bar_with_control_repeated_process_change_same_frame_falls_back_to_full_dirty_region);
    EGUI_TEST_RUN(test_page_indicator_dot_index_change_uses_full_dirty_region);
    EGUI_TEST_RUN(test_page_indicator_repeated_index_change_same_frame_falls_back_to_full_dirty_region);
    EGUI_TEST_RUN(test_page_indicator_icon_index_change_uses_full_dirty_region);
    EGUI_TEST_RUN(test_page_indicator_icon_repeated_index_change_same_frame_falls_back_to_full_dirty_region);
    EGUI_TEST_RUN(test_checkbox_checked_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_radio_button_checked_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_led_on_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_button_matrix_selected_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_button_matrix_repeated_selected_change_same_frame_falls_back_to_full_dirty_region);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_slider_touch_down_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_slider_touch_up_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_slider_touch_down_with_pressed_background_uses_full_dirty_region);
    EGUI_TEST_RUN(test_slider_touch_move_then_programmatic_change_same_frame_uses_full_dirty_region);
    EGUI_TEST_RUN(test_button_matrix_touch_release_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_mini_calendar_selected_day_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_mini_calendar_touch_down_with_pressed_background_uses_full_dirty_region);
    EGUI_TEST_RUN(test_number_picker_touch_increment_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_number_picker_touch_down_with_pressed_background_uses_full_dirty_region);
#endif
    EGUI_TEST_RUN(test_number_picker_value_change_uses_middle_zone_dirty_region);
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    EGUI_TEST_RUN(test_textinput_cursor_move_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_textinput_cursor_blink_uses_partial_dirty_region);
#endif
    EGUI_TEST_RUN(test_stopwatch_elapsed_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_stopwatch_repeated_elapsed_change_same_frame_falls_back_to_full_dirty_region);
    EGUI_TEST_SUITE_END();
}
