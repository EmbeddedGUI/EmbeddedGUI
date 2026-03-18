#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_number_box.h"

#include "../../HelloCustomWidgets/input/number_box/egui_view_number_box.h"
#include "../../HelloCustomWidgets/input/number_box/egui_view_number_box.c"

static egui_view_number_box_t test_box;
static uint8_t changed_count;
static int16_t changed_value;

static void on_value_changed(egui_view_t *self, int16_t value)
{
    EGUI_UNUSED(self);
    changed_count++;
    changed_value = value;
}

static void setup_number_box(void)
{
    egui_view_number_box_init(EGUI_VIEW_OF(&test_box));
    egui_view_set_size(EGUI_VIEW_OF(&test_box), 196, 70);
    egui_view_number_box_set_label(EGUI_VIEW_OF(&test_box), "Spacing");
    egui_view_number_box_set_suffix(EGUI_VIEW_OF(&test_box), "px");
    egui_view_number_box_set_helper(EGUI_VIEW_OF(&test_box), "0 to 64, step 4");
    egui_view_number_box_set_range(EGUI_VIEW_OF(&test_box), 0, 64);
    egui_view_number_box_set_step(EGUI_VIEW_OF(&test_box), 4);
    egui_view_number_box_set_value(EGUI_VIEW_OF(&test_box), 24);
    egui_view_number_box_set_on_value_changed_listener(EGUI_VIEW_OF(&test_box), on_value_changed);
    changed_count = 0;
    changed_value = -1;
}

static void layout_number_box(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_box), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_box)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_box)->api->on_touch_event(EGUI_VIEW_OF(&test_box), &event);
}

static void test_number_box_range_and_value_clamp(void)
{
    setup_number_box();

    egui_view_number_box_set_range(EGUI_VIEW_OF(&test_box), 20, 10);
    EGUI_TEST_ASSERT_EQUAL_INT(10, test_box.min_value);
    EGUI_TEST_ASSERT_EQUAL_INT(20, test_box.max_value);
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_number_box_get_value(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_number_box_set_value(EGUI_VIEW_OF(&test_box), -1);
    EGUI_TEST_ASSERT_EQUAL_INT(10, egui_view_number_box_get_value(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(10, changed_value);

    egui_view_number_box_set_value(EGUI_VIEW_OF(&test_box), 99);
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_number_box_get_value(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(20, changed_value);

    egui_view_number_box_set_value(EGUI_VIEW_OF(&test_box), 20);
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);
}

static void test_number_box_step_normalization(void)
{
    setup_number_box();

    egui_view_number_box_set_step(EGUI_VIEW_OF(&test_box), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_box.step);

    egui_view_number_box_set_step(EGUI_VIEW_OF(&test_box), -3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_box.step);

    egui_view_number_box_set_step(EGUI_VIEW_OF(&test_box), 5);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_box.step);
}

static void test_number_box_font_modes_and_palette(void)
{
    static const char *value_label = "Value";
    static const char *value_suffix = "ms";
    static const char *value_helper = "Helper";

    setup_number_box();

    egui_view_number_box_set_font(EGUI_VIEW_OF(&test_box), NULL);
    egui_view_number_box_set_meta_font(EGUI_VIEW_OF(&test_box), NULL);
    EGUI_TEST_ASSERT_TRUE(test_box.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_box.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_number_box_set_label(EGUI_VIEW_OF(&test_box), value_label);
    egui_view_number_box_set_suffix(EGUI_VIEW_OF(&test_box), value_suffix);
    egui_view_number_box_set_helper(EGUI_VIEW_OF(&test_box), value_helper);
    EGUI_TEST_ASSERT_TRUE(test_box.label == value_label);
    EGUI_TEST_ASSERT_TRUE(test_box.suffix == value_suffix);
    EGUI_TEST_ASSERT_TRUE(test_box.helper == value_helper);

    egui_view_number_box_set_compact_mode(EGUI_VIEW_OF(&test_box), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_box.compact_mode);

    test_box.pressed_part = EGUI_VIEW_NUMBER_BOX_PART_INC;
    egui_view_number_box_set_locked_mode(EGUI_VIEW_OF(&test_box), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_box.locked_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NUMBER_BOX_PART_NONE, test_box.pressed_part);

    egui_view_number_box_set_palette(EGUI_VIEW_OF(&test_box), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                     EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_box.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_box.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_box.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_box.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_box.accent_color.full);
}

static void test_number_box_touch_increment_and_decrement(void)
{
    egui_view_number_box_metrics_t metrics;
    egui_dim_t inc_x;
    egui_dim_t inc_y;
    egui_dim_t dec_x;
    egui_dim_t dec_y;

    setup_number_box();
    layout_number_box(10, 20, 196, 70);
    egui_view_number_box_get_metrics(&test_box, EGUI_VIEW_OF(&test_box), &metrics);

    inc_x = metrics.inc_region.location.x + metrics.inc_region.size.width / 2;
    inc_y = metrics.inc_region.location.y + metrics.inc_region.size.height / 2;
    dec_x = metrics.dec_region.location.x + metrics.dec_region.size.width / 2;
    dec_y = metrics.dec_region.location.y + metrics.dec_region.size.height / 2;

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NUMBER_BOX_PART_INC, egui_view_number_box_hit_part(&test_box, EGUI_VIEW_OF(&test_box), inc_x, inc_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NUMBER_BOX_PART_DEC, egui_view_number_box_hit_part(&test_box, EGUI_VIEW_OF(&test_box), dec_x, dec_y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, inc_x, inc_y));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_box)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NUMBER_BOX_PART_INC, test_box.pressed_part);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, inc_x, inc_y));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_box)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NUMBER_BOX_PART_NONE, test_box.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(28, egui_view_number_box_get_value(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(28, changed_value);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, dec_x, dec_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NUMBER_BOX_PART_DEC, test_box.pressed_part);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, dec_x, dec_y));
    EGUI_TEST_ASSERT_EQUAL_INT(24, egui_view_number_box_get_value(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(24, changed_value);
}

static void test_number_box_locked_mode_ignores_touch(void)
{
    egui_view_number_box_metrics_t metrics;
    egui_dim_t inc_x;
    egui_dim_t inc_y;

    setup_number_box();
    layout_number_box(10, 20, 196, 70);
    egui_view_number_box_get_metrics(&test_box, EGUI_VIEW_OF(&test_box), &metrics);
    inc_x = metrics.inc_region.location.x + metrics.inc_region.size.width / 2;
    inc_y = metrics.inc_region.location.y + metrics.inc_region.size.height / 2;

    egui_view_number_box_set_locked_mode(EGUI_VIEW_OF(&test_box), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, inc_x, inc_y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, inc_x, inc_y));
    EGUI_TEST_ASSERT_EQUAL_INT(24, egui_view_number_box_get_value(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_box)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NUMBER_BOX_PART_NONE, test_box.pressed_part);
}

static void test_number_box_touch_cancel_clears_pressed_state(void)
{
    egui_view_number_box_metrics_t metrics;
    egui_dim_t dec_x;
    egui_dim_t dec_y;

    setup_number_box();
    layout_number_box(10, 20, 196, 70);
    egui_view_number_box_get_metrics(&test_box, EGUI_VIEW_OF(&test_box), &metrics);
    dec_x = metrics.dec_region.location.x + metrics.dec_region.size.width / 2;
    dec_y = metrics.dec_region.location.y + metrics.dec_region.size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, dec_x, dec_y));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_box)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NUMBER_BOX_PART_DEC, test_box.pressed_part);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, dec_x, dec_y));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_box)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NUMBER_BOX_PART_NONE, test_box.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(24, egui_view_number_box_get_value(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);
}

void test_number_box_run(void)
{
    EGUI_TEST_SUITE_BEGIN(number_box);
    EGUI_TEST_RUN(test_number_box_range_and_value_clamp);
    EGUI_TEST_RUN(test_number_box_step_normalization);
    EGUI_TEST_RUN(test_number_box_font_modes_and_palette);
    EGUI_TEST_RUN(test_number_box_touch_increment_and_decrement);
    EGUI_TEST_RUN(test_number_box_locked_mode_ignores_touch);
    EGUI_TEST_RUN(test_number_box_touch_cancel_clears_pressed_state);
    EGUI_TEST_SUITE_END();
}
