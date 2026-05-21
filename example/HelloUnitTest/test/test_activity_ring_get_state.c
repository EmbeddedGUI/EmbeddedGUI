#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_activity_ring_get_state.h"

static egui_view_activity_ring_t s_ring;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_ring, 0, sizeof(s_ring));
    egui_view_activity_ring_init(EGUI_VIEW_OF(&s_ring), core);
}

static void test_activity_ring_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_activity_ring_get_ring_count(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_value(EGUI_VIEW_OF(&s_ring), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_value(EGUI_VIEW_OF(&s_ring), 1));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_value(EGUI_VIEW_OF(&s_ring), 2));
    EGUI_TEST_ASSERT_EQUAL_INT(8, (int)egui_view_activity_ring_get_stroke_width(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_activity_ring_get_ring_gap(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(-90, (int)egui_view_activity_ring_get_start_angle(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_activity_ring_get_show_round_cap(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_DANGER.full, (int)egui_view_activity_ring_get_ring_color(EGUI_VIEW_OF(&s_ring), 0).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_SUCCESS.full, (int)egui_view_activity_ring_get_ring_color(EGUI_VIEW_OF(&s_ring), 1).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full, (int)egui_view_activity_ring_get_ring_color(EGUI_VIEW_OF(&s_ring), 2).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TRACK_BG.full, (int)egui_view_activity_ring_get_ring_bg_color(EGUI_VIEW_OF(&s_ring), 0).full);
}

static void test_activity_ring_get_state_after_setters(void)
{
    egui_color_t ring_color = {.full = 0x1234};
    egui_color_t ring_bg_color = {.full = 0x2345};

    setup();
    egui_view_activity_ring_set_ring_count(EGUI_VIEW_OF(&s_ring), 2);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&s_ring), 1, 67);
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&s_ring), 1, ring_color);
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&s_ring), 1, ring_bg_color);
    egui_view_activity_ring_set_stroke_width(EGUI_VIEW_OF(&s_ring), 12);
    egui_view_activity_ring_set_ring_gap(EGUI_VIEW_OF(&s_ring), 5);
    egui_view_activity_ring_set_start_angle(EGUI_VIEW_OF(&s_ring), 45);
    egui_view_activity_ring_set_show_round_cap(EGUI_VIEW_OF(&s_ring), 0);

    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_activity_ring_get_ring_count(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(67, (int)egui_view_activity_ring_get_value(EGUI_VIEW_OF(&s_ring), 1));
    EGUI_TEST_ASSERT_EQUAL_INT((int)ring_color.full, (int)egui_view_activity_ring_get_ring_color(EGUI_VIEW_OF(&s_ring), 1).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)ring_bg_color.full, (int)egui_view_activity_ring_get_ring_bg_color(EGUI_VIEW_OF(&s_ring), 1).full);
    EGUI_TEST_ASSERT_EQUAL_INT(12, (int)egui_view_activity_ring_get_stroke_width(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_activity_ring_get_ring_gap(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(45, (int)egui_view_activity_ring_get_start_angle(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_show_round_cap(EGUI_VIEW_OF(&s_ring)));
}

static void test_activity_ring_get_state_clamp_and_invalid_index(void)
{
    egui_color_t color = {.full = 0x3456};

    setup();
    egui_view_activity_ring_set_ring_count(EGUI_VIEW_OF(&s_ring), 9);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&s_ring), 0, 250);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&s_ring), EGUI_VIEW_ACTIVITY_RING_MAX_RINGS, 50);
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&s_ring), EGUI_VIEW_ACTIVITY_RING_MAX_RINGS, color);
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&s_ring), EGUI_VIEW_ACTIVITY_RING_MAX_RINGS, color);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_ACTIVITY_RING_MAX_RINGS, (int)egui_view_activity_ring_get_ring_count(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_activity_ring_get_value(EGUI_VIEW_OF(&s_ring), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_value(EGUI_VIEW_OF(&s_ring), EGUI_VIEW_ACTIVITY_RING_MAX_RINGS));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_ring_color(EGUI_VIEW_OF(&s_ring), EGUI_VIEW_ACTIVITY_RING_MAX_RINGS).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_ring_bg_color(EGUI_VIEW_OF(&s_ring), EGUI_VIEW_ACTIVITY_RING_MAX_RINGS).full);
}

static void test_activity_ring_get_state_apply_params(void)
{
    static const egui_view_activity_ring_params_t params = {
            .region = {{3, 4}, {50, 60}},
    };

    setup();
    egui_view_activity_ring_apply_params(EGUI_VIEW_OF(&s_ring), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_get_width(EGUI_VIEW_OF(&s_ring)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, (int)egui_view_get_height(EGUI_VIEW_OF(&s_ring)));
}

static void test_activity_ring_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_ring_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_value(NULL, 0));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_ring_color(NULL, 0).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_ring_bg_color(NULL, 0).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_stroke_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_ring_gap(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_start_angle(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_activity_ring_get_show_round_cap(NULL));
}

void test_activity_ring_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(activity_ring_get_state);

    EGUI_TEST_RUN(test_activity_ring_get_state_defaults);
    EGUI_TEST_RUN(test_activity_ring_get_state_after_setters);
    EGUI_TEST_RUN(test_activity_ring_get_state_clamp_and_invalid_index);
    EGUI_TEST_RUN(test_activity_ring_get_state_apply_params);
    EGUI_TEST_RUN(test_activity_ring_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
