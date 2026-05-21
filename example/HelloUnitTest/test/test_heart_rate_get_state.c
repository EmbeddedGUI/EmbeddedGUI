#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_heart_rate_get_state.h"

static egui_view_heart_rate_t s_heart_rate;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_heart_rate, 0, sizeof(s_heart_rate));
    egui_view_heart_rate_init(EGUI_VIEW_OF(&s_heart_rate), core);
}

static void test_heart_rate_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_heart_rate_get_bpm(EGUI_VIEW_OF(&s_heart_rate)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_heart_rate_get_animate(EGUI_VIEW_OF(&s_heart_rate)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_DANGER.full, (int)egui_view_heart_rate_get_heart_color(EGUI_VIEW_OF(&s_heart_rate)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_heart_rate_get_ecg_offset(EGUI_VIEW_OF(&s_heart_rate)));
}

static void test_heart_rate_get_state_after_setters(void)
{
    egui_color_t color = {.full = 0x1234};

    setup();
    egui_view_heart_rate_set_bpm(EGUI_VIEW_OF(&s_heart_rate), 72);
    egui_view_heart_rate_set_animate(EGUI_VIEW_OF(&s_heart_rate), 1);
    egui_view_heart_rate_set_heart_color(EGUI_VIEW_OF(&s_heart_rate), color);
    egui_view_heart_rate_set_pulse_phase(EGUI_VIEW_OF(&s_heart_rate), 128);

    EGUI_TEST_ASSERT_EQUAL_INT(72, (int)egui_view_heart_rate_get_bpm(EGUI_VIEW_OF(&s_heart_rate)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_heart_rate_get_animate(EGUI_VIEW_OF(&s_heart_rate)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)egui_view_heart_rate_get_heart_color(EGUI_VIEW_OF(&s_heart_rate)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(16, (int)egui_view_heart_rate_get_ecg_offset(EGUI_VIEW_OF(&s_heart_rate)));

    egui_view_heart_rate_set_pulse_phase(EGUI_VIEW_OF(&s_heart_rate), 255);
    EGUI_TEST_ASSERT_EQUAL_INT(31, (int)egui_view_heart_rate_get_ecg_offset(EGUI_VIEW_OF(&s_heart_rate)));
}

static void test_heart_rate_get_state_apply_params(void)
{
    static const egui_view_heart_rate_params_t params = {
            .region = {{3, 4}, {80, 40}},
            .bpm = 88,
    };

    setup();
    egui_view_heart_rate_apply_params(EGUI_VIEW_OF(&s_heart_rate), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(88, (int)egui_view_heart_rate_get_bpm(EGUI_VIEW_OF(&s_heart_rate)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_heart_rate)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_heart_rate)));
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)egui_view_get_width(EGUI_VIEW_OF(&s_heart_rate)));
    EGUI_TEST_ASSERT_EQUAL_INT(40, (int)egui_view_get_height(EGUI_VIEW_OF(&s_heart_rate)));
}

static void test_heart_rate_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_heart_rate_get_bpm(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_heart_rate_get_animate(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_heart_rate_get_heart_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_heart_rate_get_ecg_offset(NULL));
}

void test_heart_rate_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(heart_rate_get_state);

    EGUI_TEST_RUN(test_heart_rate_get_state_defaults);
    EGUI_TEST_RUN(test_heart_rate_get_state_after_setters);
    EGUI_TEST_RUN(test_heart_rate_get_state_apply_params);
    EGUI_TEST_RUN(test_heart_rate_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
