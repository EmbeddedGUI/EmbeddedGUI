#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_flexlayout_get_state.h"

#if EGUI_CONFIG_FUNCTION_FLEXLAYOUT

static egui_view_flexlayout_t s_flexlayout;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_flexlayout, 0, sizeof(s_flexlayout));
    egui_view_flexlayout_init(EGUI_VIEW_OF(&s_flexlayout), core);
}

static void test_flexlayout_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_FLEX_DIRECTION_ROW, (int)egui_view_flexlayout_get_direction(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_FLEX_WRAP_NOWRAP, (int)egui_view_flexlayout_get_wrap(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_FLEX_JUSTIFY_START, (int)egui_view_flexlayout_get_justify_content(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_FLEX_ALIGN_STRETCH, (int)egui_view_flexlayout_get_align_items(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_FLEX_JUSTIFY_START, (int)egui_view_flexlayout_get_align_content(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_flexlayout_get_row_gap(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_flexlayout_get_col_gap(EGUI_VIEW_OF(&s_flexlayout)));
}

static void test_flexlayout_get_state_after_setters(void)
{
    setup();
    egui_view_flexlayout_set_direction(EGUI_VIEW_OF(&s_flexlayout), EGUI_FLEX_DIRECTION_COLUMN);
    egui_view_flexlayout_set_wrap(EGUI_VIEW_OF(&s_flexlayout), EGUI_FLEX_WRAP_WRAP);
    egui_view_flexlayout_set_justify_content(EGUI_VIEW_OF(&s_flexlayout), EGUI_FLEX_JUSTIFY_SPACE_BETWEEN);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&s_flexlayout), EGUI_FLEX_ALIGN_CENTER);
    egui_view_flexlayout_set_align_content(EGUI_VIEW_OF(&s_flexlayout), EGUI_FLEX_JUSTIFY_SPACE_EVENLY);
    egui_view_flexlayout_set_gap(EGUI_VIEW_OF(&s_flexlayout), 3, 5);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_FLEX_DIRECTION_COLUMN, (int)egui_view_flexlayout_get_direction(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_FLEX_WRAP_WRAP, (int)egui_view_flexlayout_get_wrap(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_FLEX_JUSTIFY_SPACE_BETWEEN, (int)egui_view_flexlayout_get_justify_content(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_FLEX_ALIGN_CENTER, (int)egui_view_flexlayout_get_align_items(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_FLEX_JUSTIFY_SPACE_EVENLY, (int)egui_view_flexlayout_get_align_content(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_flexlayout_get_row_gap(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_flexlayout_get_col_gap(EGUI_VIEW_OF(&s_flexlayout)));
}

static void test_flexlayout_get_state_update(void)
{
    setup();
    egui_view_flexlayout_set_direction(EGUI_VIEW_OF(&s_flexlayout), EGUI_FLEX_DIRECTION_COLUMN);
    egui_view_flexlayout_set_direction(EGUI_VIEW_OF(&s_flexlayout), EGUI_FLEX_DIRECTION_ROW);
    egui_view_flexlayout_set_gap(EGUI_VIEW_OF(&s_flexlayout), 2, 4);
    egui_view_flexlayout_set_gap(EGUI_VIEW_OF(&s_flexlayout), 6, 8);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_FLEX_DIRECTION_ROW, (int)egui_view_flexlayout_get_direction(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_flexlayout_get_row_gap(EGUI_VIEW_OF(&s_flexlayout)));
    EGUI_TEST_ASSERT_EQUAL_INT(8, (int)egui_view_flexlayout_get_col_gap(EGUI_VIEW_OF(&s_flexlayout)));
}

static void test_flexlayout_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_flexlayout_get_direction(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_flexlayout_get_wrap(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_flexlayout_get_justify_content(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_flexlayout_get_align_items(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_flexlayout_get_align_content(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_flexlayout_get_row_gap(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_flexlayout_get_col_gap(NULL));
}

void test_flexlayout_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(flexlayout_get_state);

    EGUI_TEST_RUN(test_flexlayout_get_state_defaults);
    EGUI_TEST_RUN(test_flexlayout_get_state_after_setters);
    EGUI_TEST_RUN(test_flexlayout_get_state_update);
    EGUI_TEST_RUN(test_flexlayout_get_state_null_self);

    EGUI_TEST_SUITE_END();
}

#else /* EGUI_CONFIG_FUNCTION_FLEXLAYOUT */

void test_flexlayout_get_state_run(void)
{
}

#endif /* EGUI_CONFIG_FUNCTION_FLEXLAYOUT */
