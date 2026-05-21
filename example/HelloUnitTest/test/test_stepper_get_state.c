#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_stepper_get_state.h"

static egui_view_stepper_t s_stepper;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_stepper, 0, sizeof(s_stepper));
    egui_view_stepper_init(EGUI_VIEW_OF(&s_stepper), core);
}

static void test_stepper_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_stepper_get_total_steps(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_stepper_get_current_step(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_STEPPER_MARK_STYLE_DOT, (int)egui_view_stepper_get_mark_style(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(EGUI_ICON_MS_DONE, egui_view_stepper_get_completed_icon(EGUI_VIEW_OF(&s_stepper))));
    EGUI_TEST_ASSERT_NULL(egui_view_stepper_get_icon_font(EGUI_VIEW_OF(&s_stepper)));
}

static void test_stepper_get_state_after_setters(void)
{
    const char *completed_icon = "ok";

    setup();
    egui_view_stepper_set_total_steps(EGUI_VIEW_OF(&s_stepper), 4);
    egui_view_stepper_set_current_step(EGUI_VIEW_OF(&s_stepper), 2);
    egui_view_stepper_set_mark_style(EGUI_VIEW_OF(&s_stepper), EGUI_VIEW_STEPPER_MARK_STYLE_ICON);
    egui_view_stepper_set_completed_icon(EGUI_VIEW_OF(&s_stepper), completed_icon);
    egui_view_stepper_set_icon_font(EGUI_VIEW_OF(&s_stepper), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_stepper_get_total_steps(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_stepper_get_current_step(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_STEPPER_MARK_STYLE_ICON, (int)egui_view_stepper_get_mark_style(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_TRUE(egui_view_stepper_get_completed_icon(EGUI_VIEW_OF(&s_stepper)) == completed_icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_stepper_get_icon_font(EGUI_VIEW_OF(&s_stepper)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
}

static void test_stepper_get_state_clamp(void)
{
    setup();
    egui_view_stepper_set_total_steps(EGUI_VIEW_OF(&s_stepper), 0);
    egui_view_stepper_set_current_step(EGUI_VIEW_OF(&s_stepper), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_stepper_get_total_steps(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_stepper_get_current_step(EGUI_VIEW_OF(&s_stepper)));

    egui_view_stepper_set_total_steps(EGUI_VIEW_OF(&s_stepper), 3);
    egui_view_stepper_set_current_step(EGUI_VIEW_OF(&s_stepper), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_stepper_get_total_steps(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_stepper_get_current_step(EGUI_VIEW_OF(&s_stepper)));
}

static void test_stepper_get_state_apply_params(void)
{
    static const egui_view_stepper_params_t params = {
            .region = {{3, 4}, {80, 24}},
            .total_steps = 5,
            .current_step = 4,
    };
    static const egui_view_stepper_params_t zero_params = {
            .region = {{5, 6}, {90, 28}},
            .total_steps = 0,
            .current_step = 7,
    };

    setup();
    egui_view_stepper_apply_params(EGUI_VIEW_OF(&s_stepper), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)egui_view_get_width(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(24, (int)egui_view_get_height(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_stepper_get_total_steps(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_stepper_get_current_step(EGUI_VIEW_OF(&s_stepper)));

    egui_view_stepper_apply_params(EGUI_VIEW_OF(&s_stepper), &zero_params);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_get_x(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_get_y(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_get_width(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(28, (int)egui_view_get_height(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_stepper_get_total_steps(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_stepper_get_current_step(EGUI_VIEW_OF(&s_stepper)));
}

static void test_stepper_get_state_clear_and_null_self(void)
{
    setup();
    egui_view_stepper_set_completed_icon(EGUI_VIEW_OF(&s_stepper), NULL);
    egui_view_stepper_set_icon_font(EGUI_VIEW_OF(&s_stepper), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_stepper_set_icon_font(EGUI_VIEW_OF(&s_stepper), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_stepper_get_completed_icon(EGUI_VIEW_OF(&s_stepper)));
    EGUI_TEST_ASSERT_NULL(egui_view_stepper_get_icon_font(EGUI_VIEW_OF(&s_stepper)));

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_stepper_get_total_steps(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_stepper_get_current_step(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_STEPPER_MARK_STYLE_DOT, (int)egui_view_stepper_get_mark_style(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_stepper_get_completed_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_stepper_get_icon_font(NULL));
}

void test_stepper_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(stepper_get_state);

    EGUI_TEST_RUN(test_stepper_get_state_defaults);
    EGUI_TEST_RUN(test_stepper_get_state_after_setters);
    EGUI_TEST_RUN(test_stepper_get_state_clamp);
    EGUI_TEST_RUN(test_stepper_get_state_apply_params);
    EGUI_TEST_RUN(test_stepper_get_state_clear_and_null_self);

    EGUI_TEST_SUITE_END();
}
