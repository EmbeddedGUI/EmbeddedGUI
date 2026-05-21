#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_chips_get_state.h"

static egui_view_chips_t s_chips;
static const char *s_labels[] = {"All", "Open", "Closed"};
static const char *s_icons[] = {"a", "b", "c"};
static uint8_t s_selected_index;

static void on_chip_selected(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    s_selected_index = index;
}

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_chips, 0, sizeof(s_chips));
    s_selected_index = 0xFFu;
    egui_view_chips_init(EGUI_VIEW_OF(&s_chips), core);
}

static void layout_chips(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 120;
    region.size.height = 40;
    egui_view_layout(EGUI_VIEW_OF(&s_chips), &region);
    egui_region_copy(&EGUI_VIEW_OF(&s_chips)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&s_chips)->api->on_touch_event(EGUI_VIEW_OF(&s_chips), &event);
}

static void test_chips_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_chips_get_chips(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_NULL(egui_view_chips_get_chip_icons(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chips_get_chip_count(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_chips_get_cols(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_chips_get_gap(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE, (int)egui_view_chips_get_selected_index(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_DARK_GREY.full, (int)egui_view_chips_get_bg_color(EGUI_VIEW_OF(&s_chips)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_LIGHT_GREY.full, (int)egui_view_chips_get_selected_bg_color(EGUI_VIEW_OF(&s_chips)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)egui_view_chips_get_text_color(EGUI_VIEW_OF(&s_chips)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)egui_view_chips_get_border_color(EGUI_VIEW_OF(&s_chips)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_chips_get_corner_radius(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_TRUE(egui_view_chips_get_font(EGUI_VIEW_OF(&s_chips)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_NULL(egui_view_chips_get_icon_font(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_chips_get_icon_text_gap(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_NULL(egui_view_chips_get_on_selected_listener(EGUI_VIEW_OF(&s_chips)));
}

static void test_chips_get_state_after_setters(void)
{
    egui_color_t bg_color = {.full = 0x1234};
    egui_color_t selected_color = {.full = 0x2345};
    egui_color_t text_color = {.full = 0x3456};
    egui_color_t border_color = {.full = 0x4567};

    setup();
    egui_view_chips_set_chips(EGUI_VIEW_OF(&s_chips), s_labels, EGUI_ARRAY_SIZE(s_labels), 2);
    egui_view_chips_set_chip_icons(EGUI_VIEW_OF(&s_chips), s_icons);
    egui_view_chips_set_selected_index(EGUI_VIEW_OF(&s_chips), 1);
    egui_view_chips_set_gap(EGUI_VIEW_OF(&s_chips), 5);
    egui_view_chips_set_corner_radius(EGUI_VIEW_OF(&s_chips), 6);
    egui_view_chips_set_bg_color(EGUI_VIEW_OF(&s_chips), bg_color);
    egui_view_chips_set_selected_bg_color(EGUI_VIEW_OF(&s_chips), selected_color);
    egui_view_chips_set_text_color(EGUI_VIEW_OF(&s_chips), text_color);
    egui_view_chips_set_border_color(EGUI_VIEW_OF(&s_chips), border_color);
    egui_view_chips_set_font(EGUI_VIEW_OF(&s_chips), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_chips_set_icon_font(EGUI_VIEW_OF(&s_chips), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_chips_set_icon_text_gap(EGUI_VIEW_OF(&s_chips), 7);
    egui_view_chips_set_on_selected_listener(EGUI_VIEW_OF(&s_chips), on_chip_selected);

    EGUI_TEST_ASSERT_TRUE(egui_view_chips_get_chips(EGUI_VIEW_OF(&s_chips)) == s_labels);
    EGUI_TEST_ASSERT_TRUE(egui_view_chips_get_chip_icons(EGUI_VIEW_OF(&s_chips)) == s_icons);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ARRAY_SIZE(s_labels), (int)egui_view_chips_get_chip_count(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_chips_get_cols(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_chips_get_selected_index(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_chips_get_gap(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_chips_get_corner_radius(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)bg_color.full, (int)egui_view_chips_get_bg_color(EGUI_VIEW_OF(&s_chips)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)selected_color.full, (int)egui_view_chips_get_selected_bg_color(EGUI_VIEW_OF(&s_chips)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)text_color.full, (int)egui_view_chips_get_text_color(EGUI_VIEW_OF(&s_chips)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)border_color.full, (int)egui_view_chips_get_border_color(EGUI_VIEW_OF(&s_chips)).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_chips_get_font(EGUI_VIEW_OF(&s_chips)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_chips_get_icon_font(EGUI_VIEW_OF(&s_chips)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT(7, (int)egui_view_chips_get_icon_text_gap(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_TRUE(egui_view_chips_get_on_selected_listener(EGUI_VIEW_OF(&s_chips)) == on_chip_selected);
    EGUI_TEST_ASSERT_EQUAL_INT(0xFF, (int)s_selected_index);
}

static void test_chips_get_state_apply_params(void)
{
    static const egui_view_chips_params_t params = {
            .region = {{2, 3}, {90, 40}},
            .labels = s_labels,
            .icons = s_icons,
            .chip_count = EGUI_ARRAY_SIZE(s_labels),
            .cols = 0,
            .gap = 4,
    };

    setup();
    egui_view_chips_apply_params(EGUI_VIEW_OF(&s_chips), &params);

    EGUI_TEST_ASSERT_TRUE(egui_view_chips_get_chips(EGUI_VIEW_OF(&s_chips)) == s_labels);
    EGUI_TEST_ASSERT_TRUE(egui_view_chips_get_chip_icons(EGUI_VIEW_OF(&s_chips)) == s_icons);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ARRAY_SIZE(s_labels), (int)egui_view_chips_get_chip_count(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ARRAY_SIZE(s_labels), (int)egui_view_chips_get_cols(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_chips_get_gap(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_get_x(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_y(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_get_width(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(40, (int)egui_view_get_height(EGUI_VIEW_OF(&s_chips)));
}

static void test_chips_get_state_listener_fires_on_release(void)
{
    setup();
    egui_view_chips_set_chips(EGUI_VIEW_OF(&s_chips), s_labels, EGUI_ARRAY_SIZE(s_labels), EGUI_ARRAY_SIZE(s_labels));
    egui_view_chips_set_on_selected_listener(EGUI_VIEW_OF(&s_chips), on_chip_selected);
    layout_chips();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 55, 30));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 55, 30));

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_selected_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_chips_get_selected_index(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_TRUE(egui_view_chips_get_on_selected_listener(EGUI_VIEW_OF(&s_chips)) == on_chip_selected);
}

static void test_chips_get_state_clear_and_null_self(void)
{
    setup();
    egui_view_chips_set_chips(EGUI_VIEW_OF(&s_chips), s_labels, EGUI_ARRAY_SIZE(s_labels), 9);
    egui_view_chips_set_selected_index(EGUI_VIEW_OF(&s_chips), 1);
    egui_view_chips_set_on_selected_listener(EGUI_VIEW_OF(&s_chips), on_chip_selected);
    egui_view_chips_clear_selection(EGUI_VIEW_OF(&s_chips));
    egui_view_chips_set_on_selected_listener(EGUI_VIEW_OF(&s_chips), NULL);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ARRAY_SIZE(s_labels), (int)egui_view_chips_get_cols(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE, (int)egui_view_chips_get_selected_index(EGUI_VIEW_OF(&s_chips)));
    EGUI_TEST_ASSERT_NULL(egui_view_chips_get_on_selected_listener(EGUI_VIEW_OF(&s_chips)));

    EGUI_TEST_ASSERT_NULL(egui_view_chips_get_chips(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_chips_get_chip_icons(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chips_get_chip_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chips_get_cols(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chips_get_gap(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE, (int)egui_view_chips_get_selected_index(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chips_get_corner_radius(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chips_get_bg_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chips_get_selected_bg_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chips_get_text_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chips_get_border_color(NULL).full);
    EGUI_TEST_ASSERT_NULL(egui_view_chips_get_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_chips_get_icon_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chips_get_icon_text_gap(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_chips_get_on_selected_listener(NULL));
}

void test_chips_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(chips_get_state);

    EGUI_TEST_RUN(test_chips_get_state_defaults);
    EGUI_TEST_RUN(test_chips_get_state_after_setters);
    EGUI_TEST_RUN(test_chips_get_state_apply_params);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_chips_get_state_listener_fires_on_release);
#endif
    EGUI_TEST_RUN(test_chips_get_state_clear_and_null_self);

    EGUI_TEST_SUITE_END();
}
