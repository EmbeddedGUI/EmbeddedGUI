#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_segmented_control_get_state.h"

static egui_view_segmented_control_t s_control;
static const char *s_segments[] = {"Day", "Week", "Month"};
static const char *s_icons[] = {"d", "w", "m"};
static const char *s_many_segments[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};
static uint8_t s_last_index;
static uint8_t s_change_count;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_control, 0, sizeof(s_control));
    s_last_index = 0;
    s_change_count = 0;
    egui_view_segmented_control_init(EGUI_VIEW_OF(&s_control), core);
}

static void on_segment_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    s_last_index = index;
    s_change_count++;
}

static void test_segmented_control_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_segmented_control_get_segments(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_NULL(egui_view_segmented_control_get_segment_icons(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_segment_count(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, (int)egui_view_segmented_control_get_pressed_index(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_segmented_control_get_horizontal_padding(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_segmented_control_get_segment_gap(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_RADIUS_MD, (int)egui_view_segmented_control_get_corner_radius(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_SURFACE_VARIANT.full, (int)egui_view_segmented_control_get_bg_color(EGUI_VIEW_OF(&s_control)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full, (int)egui_view_segmented_control_get_selected_bg_color(EGUI_VIEW_OF(&s_control)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_SECONDARY.full, (int)egui_view_segmented_control_get_text_color(EGUI_VIEW_OF(&s_control)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)egui_view_segmented_control_get_selected_text_color(EGUI_VIEW_OF(&s_control)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_BORDER.full, (int)egui_view_segmented_control_get_border_color(EGUI_VIEW_OF(&s_control)).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_segmented_control_get_font(EGUI_VIEW_OF(&s_control)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_NULL(egui_view_segmented_control_get_icon_font(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_segmented_control_get_icon_text_gap(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_NULL(egui_view_segmented_control_get_on_segment_changed_listener(EGUI_VIEW_OF(&s_control)));
}

static void test_segmented_control_get_state_after_setters(void)
{
    egui_color_t bg_color = {.full = 0x1234};
    egui_color_t selected_bg_color = {.full = 0x2345};
    egui_color_t text_color = {.full = 0x3456};
    egui_color_t selected_text_color = {.full = 0x4567};
    egui_color_t border_color = {.full = 0x5678};

    setup();
    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&s_control), s_segments, EGUI_ARRAY_SIZE(s_segments));
    egui_view_segmented_control_set_segment_icons(EGUI_VIEW_OF(&s_control), s_icons);
    egui_view_segmented_control_set_on_segment_changed_listener(EGUI_VIEW_OF(&s_control), on_segment_changed);
    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&s_control), 2);
    egui_view_segmented_control_set_horizontal_padding(EGUI_VIEW_OF(&s_control), 4);
    egui_view_segmented_control_set_segment_gap(EGUI_VIEW_OF(&s_control), 5);
    egui_view_segmented_control_set_corner_radius(EGUI_VIEW_OF(&s_control), 6);
    egui_view_segmented_control_set_bg_color(EGUI_VIEW_OF(&s_control), bg_color);
    egui_view_segmented_control_set_selected_bg_color(EGUI_VIEW_OF(&s_control), selected_bg_color);
    egui_view_segmented_control_set_text_color(EGUI_VIEW_OF(&s_control), text_color);
    egui_view_segmented_control_set_selected_text_color(EGUI_VIEW_OF(&s_control), selected_text_color);
    egui_view_segmented_control_set_border_color(EGUI_VIEW_OF(&s_control), border_color);
    egui_view_segmented_control_set_font(EGUI_VIEW_OF(&s_control), NULL);
    egui_view_segmented_control_set_icon_font(EGUI_VIEW_OF(&s_control), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_segmented_control_set_icon_text_gap(EGUI_VIEW_OF(&s_control), 7);

    EGUI_TEST_ASSERT_TRUE(egui_view_segmented_control_get_segments(EGUI_VIEW_OF(&s_control)) == s_segments);
    EGUI_TEST_ASSERT_TRUE(egui_view_segmented_control_get_segment_icons(EGUI_VIEW_OF(&s_control)) == s_icons);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ARRAY_SIZE(s_segments), (int)egui_view_segmented_control_get_segment_count(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, (int)egui_view_segmented_control_get_pressed_index(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_segmented_control_get_horizontal_padding(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_segmented_control_get_segment_gap(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_segmented_control_get_corner_radius(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)bg_color.full, (int)egui_view_segmented_control_get_bg_color(EGUI_VIEW_OF(&s_control)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)selected_bg_color.full, (int)egui_view_segmented_control_get_selected_bg_color(EGUI_VIEW_OF(&s_control)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)text_color.full, (int)egui_view_segmented_control_get_text_color(EGUI_VIEW_OF(&s_control)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)selected_text_color.full, (int)egui_view_segmented_control_get_selected_text_color(EGUI_VIEW_OF(&s_control)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)border_color.full, (int)egui_view_segmented_control_get_border_color(EGUI_VIEW_OF(&s_control)).full);
    EGUI_TEST_ASSERT_NULL(egui_view_segmented_control_get_font(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_TRUE(egui_view_segmented_control_get_icon_font(EGUI_VIEW_OF(&s_control)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT(7, (int)egui_view_segmented_control_get_icon_text_gap(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_TRUE(egui_view_segmented_control_get_on_segment_changed_listener(EGUI_VIEW_OF(&s_control)) == on_segment_changed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_change_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)s_last_index);
}

static void test_segmented_control_get_state_clamp_and_invalid_index(void)
{
    setup();
    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&s_control), s_segments, EGUI_ARRAY_SIZE(s_segments));
    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&s_control), 2);
    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&s_control), s_segments, 2);

    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_segmented_control_get_segment_count(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&s_control)));

    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&s_control), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&s_control)));

    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&s_control), s_many_segments, EGUI_ARRAY_SIZE(s_many_segments));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_MAX_SEGMENTS, (int)egui_view_segmented_control_get_segment_count(EGUI_VIEW_OF(&s_control)));

    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&s_control), NULL, 0);
    EGUI_TEST_ASSERT_NULL(egui_view_segmented_control_get_segments(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_segment_count(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, (int)egui_view_segmented_control_get_pressed_index(EGUI_VIEW_OF(&s_control)));
}

static void test_segmented_control_get_state_apply_params(void)
{
    static const egui_view_segmented_control_params_t params = {
            .region = {{3, 4}, {120, 30}},
            .segment_texts = s_segments,
            .segment_icons = s_icons,
            .segment_count = EGUI_ARRAY_SIZE(s_segments),
    };

    setup();
    egui_view_segmented_control_apply_params(EGUI_VIEW_OF(&s_control), &params);

    EGUI_TEST_ASSERT_TRUE(egui_view_segmented_control_get_segments(EGUI_VIEW_OF(&s_control)) == s_segments);
    EGUI_TEST_ASSERT_TRUE(egui_view_segmented_control_get_segment_icons(EGUI_VIEW_OF(&s_control)) == s_icons);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ARRAY_SIZE(s_segments), (int)egui_view_segmented_control_get_segment_count(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(120, (int)egui_view_get_width(EGUI_VIEW_OF(&s_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_get_height(EGUI_VIEW_OF(&s_control)));
}

static void test_segmented_control_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_segmented_control_get_segments(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_segmented_control_get_segment_icons(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_segment_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_current_index(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE, (int)egui_view_segmented_control_get_pressed_index(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_horizontal_padding(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_segment_gap(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_corner_radius(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_bg_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_selected_bg_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_text_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_selected_text_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_border_color(NULL).full);
    EGUI_TEST_ASSERT_NULL(egui_view_segmented_control_get_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_segmented_control_get_icon_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_segmented_control_get_icon_text_gap(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_segmented_control_get_on_segment_changed_listener(NULL));
}

void test_segmented_control_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(segmented_control_get_state);

    EGUI_TEST_RUN(test_segmented_control_get_state_defaults);
    EGUI_TEST_RUN(test_segmented_control_get_state_after_setters);
    EGUI_TEST_RUN(test_segmented_control_get_state_clamp_and_invalid_index);
    EGUI_TEST_RUN(test_segmented_control_get_state_apply_params);
    EGUI_TEST_RUN(test_segmented_control_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
