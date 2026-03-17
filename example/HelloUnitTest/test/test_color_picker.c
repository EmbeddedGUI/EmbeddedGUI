#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_color_picker.h"

#include "../../HelloCustomWidgets/input/color_picker/egui_view_color_picker.h"
#include "../../HelloCustomWidgets/input/color_picker/egui_view_color_picker.c"

static egui_view_color_picker_t test_color_picker;
static uint8_t g_changed_count = 0;
static uint8_t g_changed_hue = 0xFF;
static uint8_t g_changed_saturation = 0xFF;
static uint8_t g_changed_value = 0xFF;
static uint8_t g_changed_part = EGUI_VIEW_COLOR_PICKER_PART_NONE;

static void reset_changed_state(void)
{
    g_changed_count = 0;
    g_changed_hue = 0xFF;
    g_changed_saturation = 0xFF;
    g_changed_value = 0xFF;
    g_changed_part = EGUI_VIEW_COLOR_PICKER_PART_NONE;
}

static void on_color_changed(egui_view_t *self, egui_color_t color, uint8_t hue_index, uint8_t saturation_index, uint8_t value_index, uint8_t part)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(color);
    g_changed_count++;
    g_changed_hue = hue_index;
    g_changed_saturation = saturation_index;
    g_changed_value = value_index;
    g_changed_part = part;
}

static void setup_color_picker(uint8_t hue_index, uint8_t saturation_index, uint8_t value_index)
{
    egui_view_color_picker_init(EGUI_VIEW_OF(&test_color_picker));
    egui_view_set_size(EGUI_VIEW_OF(&test_color_picker), 196, 112);
    egui_view_color_picker_set_on_changed_listener(EGUI_VIEW_OF(&test_color_picker), on_color_changed);
    egui_view_color_picker_set_selection(EGUI_VIEW_OF(&test_color_picker), hue_index, saturation_index, value_index);
    reset_changed_state();
}

static void layout_color_picker(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_color_picker), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_color_picker)->region_screen, &region);
}

static int send_touch_event(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_color_picker)->api->on_touch_event(EGUI_VIEW_OF(&test_color_picker), &event);
}

static int send_key_event(uint8_t type, uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key_code = key_code;
    return EGUI_VIEW_OF(&test_color_picker)->api->on_key_event(EGUI_VIEW_OF(&test_color_picker), &event);
}

static void test_color_picker_tab_cycles_between_parts(void)
{
    setup_color_picker(7, 4, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_PART_PALETTE, egui_view_color_picker_get_current_part(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_handle_navigation_key(EGUI_VIEW_OF(&test_color_picker), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_PART_HUE, egui_view_color_picker_get_current_part(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_handle_navigation_key(EGUI_VIEW_OF(&test_color_picker), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_PART_PALETTE, egui_view_color_picker_get_current_part(EGUI_VIEW_OF(&test_color_picker)));
}

static void test_color_picker_palette_navigation_updates_selection(void)
{
    setup_color_picker(7, 4, 1);

    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_handle_navigation_key(EGUI_VIEW_OF(&test_color_picker), EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_color_picker_get_saturation_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_color_picker_get_value_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_PART_PALETTE, g_changed_part);

    reset_changed_state();
    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_handle_navigation_key(EGUI_VIEW_OF(&test_color_picker), EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_color_picker_get_value_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_value);
}

static void test_color_picker_right_from_palette_end_moves_to_hue(void)
{
    setup_color_picker(7, EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT - 1, 1);
    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_handle_navigation_key(EGUI_VIEW_OF(&test_color_picker), EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_PART_HUE, egui_view_color_picker_get_current_part(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_color_picker_hue_navigation_home_end(void)
{
    setup_color_picker(7, 4, 1);
    egui_view_color_picker_set_current_part(EGUI_VIEW_OF(&test_color_picker), EGUI_VIEW_COLOR_PICKER_PART_HUE);

    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_handle_navigation_key(EGUI_VIEW_OF(&test_color_picker), EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(8, egui_view_color_picker_get_hue_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_PART_HUE, g_changed_part);

    reset_changed_state();
    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_handle_navigation_key(EGUI_VIEW_OF(&test_color_picker), EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_color_picker_get_hue_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_hue);

    reset_changed_state();
    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_handle_navigation_key(EGUI_VIEW_OF(&test_color_picker), EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_HUE_COUNT - 1, egui_view_color_picker_get_hue_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
}

static void test_color_picker_touch_palette_selects_bottom_left_cell(void)
{
    egui_region_t palette_region;

    setup_color_picker(7, 4, 1);
    layout_color_picker(10, 20, 196, 112);
    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_get_part_region(EGUI_VIEW_OF(&test_color_picker), EGUI_VIEW_COLOR_PICKER_PART_PALETTE, &palette_region));

    EGUI_TEST_ASSERT_TRUE(
            send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, palette_region.location.x + 1, palette_region.location.y + palette_region.size.height - 1));
    EGUI_TEST_ASSERT_TRUE(
            send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, palette_region.location.x + 1, palette_region.location.y + palette_region.size.height - 1));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_color_picker_get_saturation_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_VALUE_COUNT - 1, egui_view_color_picker_get_value_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_PART_PALETTE, egui_view_color_picker_get_current_part(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_TRUE(g_changed_count >= 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_PART_PALETTE, g_changed_part);
}

static void test_color_picker_touch_hue_selects_last_segment(void)
{
    egui_region_t hue_region;

    setup_color_picker(7, 4, 1);
    layout_color_picker(10, 20, 196, 112);
    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_get_part_region(EGUI_VIEW_OF(&test_color_picker), EGUI_VIEW_COLOR_PICKER_PART_HUE, &hue_region));

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, hue_region.location.x + hue_region.size.width / 2,
                                           hue_region.location.y + hue_region.size.height - 1));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, hue_region.location.x + hue_region.size.width / 2,
                                           hue_region.location.y + hue_region.size.height - 1));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_HUE_COUNT - 1, egui_view_color_picker_get_hue_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_PART_HUE, egui_view_color_picker_get_current_part(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_TRUE(g_changed_count >= 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_PART_HUE, g_changed_part);
}

static void test_color_picker_selection_clamps_and_hex_exists(void)
{
    setup_color_picker(0, 0, 0);
    egui_view_color_picker_set_selection(EGUI_VIEW_OF(&test_color_picker), 99, 99, 99);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_HUE_COUNT - 1, egui_view_color_picker_get_hue_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT - 1, egui_view_color_picker_get_saturation_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COLOR_PICKER_VALUE_COUNT - 1, egui_view_color_picker_get_value_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_get_hex_text(EGUI_VIEW_OF(&test_color_picker))[0] == '#');
}

static void test_color_picker_read_only_ignores_navigation_and_touch(void)
{
    egui_region_t palette_region;

    setup_color_picker(7, 4, 1);
    egui_view_color_picker_set_read_only_mode(EGUI_VIEW_OF(&test_color_picker), 1);
    layout_color_picker(10, 20, 196, 112);
    EGUI_TEST_ASSERT_FALSE(egui_view_color_picker_handle_navigation_key(EGUI_VIEW_OF(&test_color_picker), EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_TRUE(egui_view_color_picker_get_part_region(EGUI_VIEW_OF(&test_color_picker), EGUI_VIEW_COLOR_PICKER_PART_PALETTE, &palette_region));
    EGUI_TEST_ASSERT_FALSE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, palette_region.location.x + 1, palette_region.location.y + 1));
    EGUI_TEST_ASSERT_EQUAL_INT(7, egui_view_color_picker_get_hue_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_color_picker_get_saturation_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_color_picker_get_value_index(EGUI_VIEW_OF(&test_color_picker)));
}

static void test_color_picker_key_event_down_consumes_without_commit_and_up_changes_value(void)
{
    setup_color_picker(7, 4, 1);
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_color_picker_get_saturation_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_color_picker_get_saturation_index(EGUI_VIEW_OF(&test_color_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
}

void test_color_picker_run(void)
{
    EGUI_TEST_SUITE_BEGIN(color_picker);
    EGUI_TEST_RUN(test_color_picker_tab_cycles_between_parts);
    EGUI_TEST_RUN(test_color_picker_palette_navigation_updates_selection);
    EGUI_TEST_RUN(test_color_picker_right_from_palette_end_moves_to_hue);
    EGUI_TEST_RUN(test_color_picker_hue_navigation_home_end);
    EGUI_TEST_RUN(test_color_picker_touch_palette_selects_bottom_left_cell);
    EGUI_TEST_RUN(test_color_picker_touch_hue_selects_last_segment);
    EGUI_TEST_RUN(test_color_picker_selection_clamps_and_hex_exists);
    EGUI_TEST_RUN(test_color_picker_read_only_ignores_navigation_and_touch);
    EGUI_TEST_RUN(test_color_picker_key_event_down_consumes_without_commit_and_up_changes_value);
    EGUI_TEST_SUITE_END();
}
