#include "egui.h"
#include "test/egui_test.h"
#include "test_linearlayout.h"

static egui_view_linearlayout_t test_layout;
static egui_view_t test_child1;
static egui_view_t test_child2;
static egui_view_t test_child3;

static void test_ll_init_defaults(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&test_layout));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_linearlayout_is_orientation_horizontal(EGUI_VIEW_OF(&test_layout)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_linearlayout_is_auto_width(EGUI_VIEW_OF(&test_layout)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_linearlayout_is_auto_height(EGUI_VIEW_OF(&test_layout)));
}

static void test_ll_set_orientation(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&test_layout));

    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&test_layout), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_linearlayout_is_orientation_horizontal(EGUI_VIEW_OF(&test_layout)));

    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&test_layout), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_linearlayout_is_orientation_horizontal(EGUI_VIEW_OF(&test_layout)));
}

static void test_ll_vertical_layout(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&test_layout));
    egui_view_set_position(EGUI_VIEW_OF(&test_layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_layout), 100, 300);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&test_layout), EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP);

    egui_view_init(&test_child1);
    egui_view_set_size(&test_child1, 50, 30);
    egui_view_init(&test_child2);
    egui_view_set_size(&test_child2, 50, 40);
    egui_view_init(&test_child3);
    egui_view_set_size(&test_child3, 50, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child3);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&test_layout));

    // Vertical layout: children stacked top-to-bottom
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_child1.region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(30, test_child2.region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(70, test_child3.region.location.y);

    // Clean up
    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_layout));
}

static void test_ll_horizontal_layout(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&test_layout));
    egui_view_set_position(EGUI_VIEW_OF(&test_layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_layout), 300, 100);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&test_layout), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&test_layout), EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP);

    egui_view_init(&test_child1);
    egui_view_set_size(&test_child1, 30, 50);
    egui_view_init(&test_child2);
    egui_view_set_size(&test_child2, 40, 50);
    egui_view_init(&test_child3);
    egui_view_set_size(&test_child3, 50, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child3);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&test_layout));

    // Horizontal layout: children placed left-to-right
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_child1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(30, test_child2.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(70, test_child3.region.location.x);

    // Clean up
    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_layout));
}

static void test_ll_center_align(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&test_layout));
    egui_view_set_position(EGUI_VIEW_OF(&test_layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_layout), 100, 300);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&test_layout), EGUI_ALIGN_CENTER);

    egui_view_init(&test_child1);
    egui_view_set_size(&test_child1, 50, 30);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child1);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&test_layout));

    // Center aligned: child should be centered horizontally
    // (100 - 50) / 2 = 25
    EGUI_TEST_ASSERT_EQUAL_INT(25, test_child1.region.location.x);

    // Clean up
    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_layout));
}

static void test_ll_vertical_with_margin(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&test_layout));
    egui_view_set_position(EGUI_VIEW_OF(&test_layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_layout), 100, 300);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&test_layout), EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP);

    egui_view_init(&test_child1);
    egui_view_set_size(&test_child1, 50, 30);
    egui_view_set_margin(&test_child1, 5, 5, 10, 10);
    egui_view_init(&test_child2);
    egui_view_set_size(&test_child2, 50, 40);
    egui_view_set_margin(&test_child2, 5, 5, 5, 5);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child2);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&test_layout));

    // child1: margin_left=5, margin_top=10
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_child1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(10, test_child1.region.location.y);
    // child2: y = child1_height(30) + child1_margin_top(10) + child1_margin_bottom(10) + child2_margin_top(5)
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_child2.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(55, test_child2.region.location.y);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_layout));
}

static void test_ll_center_with_padding(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&test_layout));
    egui_view_set_position(EGUI_VIEW_OF(&test_layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_layout), 120, 300);
    egui_view_set_padding(EGUI_VIEW_OF(&test_layout), 10, 10, 0, 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&test_layout), EGUI_ALIGN_CENTER);

    egui_view_init(&test_child1);
    egui_view_set_size(&test_child1, 50, 30);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child1);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&test_layout));

    // Content area = 120 - 10 - 10 = 100
    // Center aligned: (100 - 50) / 2 = 25
    EGUI_TEST_ASSERT_EQUAL_INT(25, test_child1.region.location.x);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_layout));
}

static void test_ll_vertical_center_with_padding(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&test_layout));
    egui_view_set_position(EGUI_VIEW_OF(&test_layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_layout), 100, 200);
    egui_view_set_padding(EGUI_VIEW_OF(&test_layout), 0, 0, 20, 20);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&test_layout), EGUI_ALIGN_CENTER);

    egui_view_init(&test_child1);
    egui_view_set_size(&test_child1, 50, 40);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child1);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&test_layout));

    // Content area height = 200 - 20 - 20 = 160
    // VCENTER: (160 - 40) / 2 = 60
    EGUI_TEST_ASSERT_EQUAL_INT(60, test_child1.region.location.y);
    // Content area width = 100
    // HCENTER: (100 - 50) / 2 = 25
    EGUI_TEST_ASSERT_EQUAL_INT(25, test_child1.region.location.x);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_layout));
}

static void test_ll_gone_child_skipped(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&test_layout));
    egui_view_set_position(EGUI_VIEW_OF(&test_layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_layout), 100, 300);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&test_layout), EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP);

    egui_view_init(&test_child1);
    egui_view_set_size(&test_child1, 50, 30);
    egui_view_init(&test_child2);
    egui_view_set_size(&test_child2, 50, 40);
    egui_view_set_gone(&test_child2, 1);
    egui_view_init(&test_child3);
    egui_view_set_size(&test_child3, 50, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child3);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&test_layout));

    // child2 is gone, so child3 follows child1 directly
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_child1.region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(30, test_child3.region.location.y);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_layout));
}

static void test_ll_auto_size_with_padding(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&test_layout));
    egui_view_set_position(EGUI_VIEW_OF(&test_layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_layout), 200, 200);
    egui_view_set_padding(EGUI_VIEW_OF(&test_layout), 10, 10, 5, 5);
    egui_view_linearlayout_set_auto_width(EGUI_VIEW_OF(&test_layout), 1);
    egui_view_linearlayout_set_auto_height(EGUI_VIEW_OF(&test_layout), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&test_layout), EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP);

    egui_view_init(&test_child1);
    egui_view_set_size(&test_child1, 60, 30);
    egui_view_init(&test_child2);
    egui_view_set_size(&test_child2, 80, 40);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_layout), &test_child2);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&test_layout));

    // Vertical mode: total_child_width = max(60,80) = 80, total_child_height = 30+40 = 70
    // Auto size should include padding: width = 80 + 10 + 10 = 100, height = 70 + 5 + 5 = 80
    EGUI_TEST_ASSERT_EQUAL_INT(100, EGUI_VIEW_OF(&test_layout)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(80, EGUI_VIEW_OF(&test_layout)->region.size.height);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_layout));
}

void test_linearlayout_run(void)
{
    EGUI_TEST_SUITE_BEGIN(linearlayout);

    EGUI_TEST_RUN(test_ll_init_defaults);
    EGUI_TEST_RUN(test_ll_set_orientation);
    EGUI_TEST_RUN(test_ll_vertical_layout);
    EGUI_TEST_RUN(test_ll_horizontal_layout);
    EGUI_TEST_RUN(test_ll_center_align);
    EGUI_TEST_RUN(test_ll_vertical_with_margin);
    EGUI_TEST_RUN(test_ll_center_with_padding);
    EGUI_TEST_RUN(test_ll_vertical_center_with_padding);
    EGUI_TEST_RUN(test_ll_gone_child_skipped);
    EGUI_TEST_RUN(test_ll_auto_size_with_padding);

    EGUI_TEST_SUITE_END();
}
