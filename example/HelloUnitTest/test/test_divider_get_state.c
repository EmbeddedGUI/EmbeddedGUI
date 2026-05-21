#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_divider_get_state.h"

static egui_view_divider_t s_divider;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_divider, 0, sizeof(s_divider));
    egui_view_divider_init(EGUI_VIEW_OF(&s_divider), core);
}

static void test_divider_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_BORDER.full, (int)egui_view_divider_get_color(EGUI_VIEW_OF(&s_divider)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100, (int)egui_view_divider_get_alpha(EGUI_VIEW_OF(&s_divider)));
}

static void test_divider_get_state_after_setters(void)
{
    egui_color_t color = {.full = 0x2345};

    setup();
    egui_view_divider_set_color(EGUI_VIEW_OF(&s_divider), color);
    egui_view_divider_set_alpha(EGUI_VIEW_OF(&s_divider), EGUI_ALPHA_40);

    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)egui_view_divider_get_color(EGUI_VIEW_OF(&s_divider)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_40, (int)egui_view_divider_get_alpha(EGUI_VIEW_OF(&s_divider)));
}

static void test_divider_get_state_apply_params(void)
{
    static const egui_view_divider_params_t params = {
            .region = {{1, 2}, {30, 3}},
            .color = {.full = 0x4567},
    };

    setup();
    egui_view_divider_apply_params(EGUI_VIEW_OF(&s_divider), &params);

    EGUI_TEST_ASSERT_EQUAL_INT((int)params.color.full, (int)egui_view_divider_get_color(EGUI_VIEW_OF(&s_divider)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100, (int)egui_view_divider_get_alpha(EGUI_VIEW_OF(&s_divider)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_get_x(EGUI_VIEW_OF(&s_divider)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_get_y(EGUI_VIEW_OF(&s_divider)));
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_get_width(EGUI_VIEW_OF(&s_divider)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_height(EGUI_VIEW_OF(&s_divider)));
}

static void test_divider_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_divider_get_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_divider_get_alpha(NULL));
}

void test_divider_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(divider_get_state);

    EGUI_TEST_RUN(test_divider_get_state_defaults);
    EGUI_TEST_RUN(test_divider_get_state_after_setters);
    EGUI_TEST_RUN(test_divider_get_state_apply_params);
    EGUI_TEST_RUN(test_divider_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
