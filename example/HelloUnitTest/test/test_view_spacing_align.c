#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_view_spacing_align.h"
#include "uicode_disp0.h"

static void test_view_spacing_getters(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_t view;

    memset(&view, 0, sizeof(view));
    egui_view_init(&view, core);
    egui_view_set_padding(&view, 1, 2, 3, 4);
    egui_view_set_margin(&view, 5, 6, 7, 8);

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_padding_left(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_get_padding_right(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_get_padding_top(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_get_padding_bottom(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_get_margin_left(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(6, egui_view_get_margin_right(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(7, egui_view_get_margin_top(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(8, egui_view_get_margin_bottom(&view));
}

static void test_view_spacing_getters_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_padding_left(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_padding_right(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_padding_top(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_padding_bottom(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_margin_left(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_margin_right(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_margin_top(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_margin_bottom(NULL));
}

static void test_view_align_to_parent_center_with_padding(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_group_t parent;
    egui_view_t child;

    egui_view_group_init(EGUI_VIEW_OF(&parent), core);
    egui_view_init(&child, core);
    egui_view_set_size(EGUI_VIEW_OF(&parent), 100, 80);
    egui_view_set_padding(EGUI_VIEW_OF(&parent), 10, 10, 5, 5);
    egui_view_set_size(&child, 20, 10);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent), &child);

    egui_view_align_to_parent(&child, EGUI_ALIGN_CENTER, 0, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_get_x(&child));
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_get_y(&child));

    egui_view_group_remove_child(EGUI_VIEW_OF(&parent), &child);
}

static void test_view_align_to_parent_bottom_right_with_offset(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_group_t parent;
    egui_view_t child;

    egui_view_group_init(EGUI_VIEW_OF(&parent), core);
    egui_view_init(&child, core);
    egui_view_set_size(EGUI_VIEW_OF(&parent), 100, 80);
    egui_view_set_padding(EGUI_VIEW_OF(&parent), 10, 0, 5, 0);
    egui_view_set_size(&child, 20, 10);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent), &child);

    egui_view_align_to_parent(&child, EGUI_ALIGN_BOTTOM_RIGHT, -3, -4);
    EGUI_TEST_ASSERT_EQUAL_INT(67, egui_view_get_x(&child));
    EGUI_TEST_ASSERT_EQUAL_INT(61, egui_view_get_y(&child));

    egui_view_group_remove_child(EGUI_VIEW_OF(&parent), &child);
}

static void test_view_align_to_screen_without_parent(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_t view;
    egui_dim_t expected_x;
    egui_dim_t expected_y;

    memset(&view, 0, sizeof(view));
    egui_view_init(&view, core);
    egui_view_set_size(&view, 20, 10);
    egui_view_align_to_parent(&view, EGUI_ALIGN_BOTTOM_RIGHT, 0, 0);

    expected_x = (egui_dim_t)(core->screen_width - 20);
    expected_y = (egui_dim_t)(core->screen_height - 10);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_x, egui_view_get_x(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(expected_y, egui_view_get_y(&view));
}

void test_view_spacing_align_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_spacing_align);

    EGUI_TEST_RUN(test_view_spacing_getters);
    EGUI_TEST_RUN(test_view_spacing_getters_null_self);
    EGUI_TEST_RUN(test_view_align_to_parent_center_with_padding);
    EGUI_TEST_RUN(test_view_align_to_parent_bottom_right_with_offset);
    EGUI_TEST_RUN(test_view_align_to_screen_without_parent);

    EGUI_TEST_SUITE_END();
}
