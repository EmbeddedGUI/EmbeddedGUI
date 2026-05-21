#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_gridlayout_get_state.h"

static egui_view_gridlayout_t s_gridlayout;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_gridlayout, 0, sizeof(s_gridlayout));
    egui_view_gridlayout_init(EGUI_VIEW_OF(&s_gridlayout), core);
}

static void test_gridlayout_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_gridlayout_get_col_count(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gridlayout_get_align_type(EGUI_VIEW_OF(&s_gridlayout)));
}

static void test_gridlayout_get_state_after_setters(void)
{
    setup();
    egui_view_gridlayout_set_col_count(EGUI_VIEW_OF(&s_gridlayout), 4);
    egui_view_gridlayout_set_align_type(EGUI_VIEW_OF(&s_gridlayout), EGUI_ALIGN_RIGHT | EGUI_ALIGN_BOTTOM);

    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_gridlayout_get_col_count(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALIGN_RIGHT | EGUI_ALIGN_BOTTOM, (int)egui_view_gridlayout_get_align_type(EGUI_VIEW_OF(&s_gridlayout)));
}

static void test_gridlayout_get_state_col_count_clamp(void)
{
    setup();
    egui_view_gridlayout_set_col_count(EGUI_VIEW_OF(&s_gridlayout), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_gridlayout_get_col_count(EGUI_VIEW_OF(&s_gridlayout)));
}

static void test_gridlayout_get_state_apply_params(void)
{
    static const egui_view_gridlayout_params_t params = {
            .region = {{3, 4}, {100, 60}},
            .col_count = 3,
            .align_type = EGUI_ALIGN_CENTER,
    };
    static const egui_view_gridlayout_params_t zero_params = {
            .region = {{5, 6}, {120, 80}},
            .col_count = 0,
            .align_type = EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP,
    };

    setup();
    egui_view_gridlayout_apply_params(EGUI_VIEW_OF(&s_gridlayout), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_get_width(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, (int)egui_view_get_height(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_gridlayout_get_col_count(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALIGN_CENTER, (int)egui_view_gridlayout_get_align_type(EGUI_VIEW_OF(&s_gridlayout)));

    egui_view_gridlayout_apply_params(EGUI_VIEW_OF(&s_gridlayout), &zero_params);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_get_x(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_get_y(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(120, (int)egui_view_get_width(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)egui_view_get_height(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gridlayout_get_col_count(EGUI_VIEW_OF(&s_gridlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, (int)egui_view_gridlayout_get_align_type(EGUI_VIEW_OF(&s_gridlayout)));
}

static void test_gridlayout_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gridlayout_get_col_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gridlayout_get_align_type(NULL));
}

void test_gridlayout_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(gridlayout_get_state);

    EGUI_TEST_RUN(test_gridlayout_get_state_defaults);
    EGUI_TEST_RUN(test_gridlayout_get_state_after_setters);
    EGUI_TEST_RUN(test_gridlayout_get_state_col_count_clamp);
    EGUI_TEST_RUN(test_gridlayout_get_state_apply_params);
    EGUI_TEST_RUN(test_gridlayout_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
