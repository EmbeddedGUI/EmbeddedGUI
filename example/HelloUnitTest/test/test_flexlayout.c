#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_flexlayout.h"

#if EGUI_CONFIG_FUNCTION_FLEXLAYOUT

static egui_view_flexlayout_t test_fl;
static egui_view_t             test_child1;
static egui_view_t             test_child2;
static egui_view_t             test_child3;

static egui_core_t *test_fl_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

/* ------------------------------------------------------------------ */

static void test_fl_init_defaults(void)
{
    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), test_fl_get_core());

    egui_view_flexlayout_t *fl = &test_fl;

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_FLEX_DIRECTION_ROW,    fl->direction);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_FLEX_WRAP_NOWRAP,      fl->wrap);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_FLEX_JUSTIFY_START,    fl->justify_content);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_FLEX_ALIGN_STRETCH,    fl->align_items);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_FLEX_JUSTIFY_START,    fl->align_content);
    EGUI_TEST_ASSERT_EQUAL_INT(0, fl->row_gap);
    EGUI_TEST_ASSERT_EQUAL_INT(0, fl->col_gap);
}

static void test_fl_row_nowrap(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 60);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 30, 50);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 40, 50);
    egui_view_init(&test_child3, core);
    egui_view_set_size(&test_child3, 50, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child3);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* ROW + START: children placed left-to-right */
    EGUI_TEST_ASSERT_EQUAL_INT(0,  test_child1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(30, test_child2.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(70, test_child3.region.location.x);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_column_nowrap(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 60, 200);
    egui_view_flexlayout_set_direction(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_DIRECTION_COLUMN);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 50, 30);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 50, 40);
    egui_view_init(&test_child3, core);
    egui_view_set_size(&test_child3, 50, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child3);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* COLUMN + START: children stacked top-to-bottom */
    EGUI_TEST_ASSERT_EQUAL_INT(0,  test_child1.region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(30, test_child2.region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(70, test_child3.region.location.y);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_justify_center(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 60);
    egui_view_flexlayout_set_justify_content(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_JUSTIFY_CENTER);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 40, 50);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 40, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* main_size=80, free=120, leading=60 */
    EGUI_TEST_ASSERT_EQUAL_INT(60,  test_child1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(100, test_child2.region.location.x);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_justify_end(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 60);
    egui_view_flexlayout_set_justify_content(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_JUSTIFY_END);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 40, 50);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 40, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* main_size=80, free=120, leading=120 */
    EGUI_TEST_ASSERT_EQUAL_INT(120, test_child1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(160, test_child2.region.location.x);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_justify_space_between(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 60);
    egui_view_flexlayout_set_justify_content(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_JUSTIFY_SPACE_BETWEEN);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 30, 50);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 30, 50);
    egui_view_init(&test_child3, core);
    egui_view_set_size(&test_child3, 30, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child3);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* main_size=90, free=110, between=55 (110/2) */
    EGUI_TEST_ASSERT_EQUAL_INT(0,   test_child1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(85,  test_child2.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(170, test_child3.region.location.x);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_justify_space_around(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 60);
    egui_view_flexlayout_set_justify_content(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_JUSTIFY_SPACE_AROUND);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 30, 50);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 30, 50);
    egui_view_init(&test_child3, core);
    egui_view_set_size(&test_child3, 30, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child3);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* main_size=90, free=110, between=36(=110/3), leading=18(=36/2) */
    EGUI_TEST_ASSERT_EQUAL_INT(18,  test_child1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(84,  test_child2.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(150, test_child3.region.location.x);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_justify_space_evenly(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 60);
    egui_view_flexlayout_set_justify_content(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_JUSTIFY_SPACE_EVENLY);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 30, 50);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 30, 50);
    egui_view_init(&test_child3, core);
    egui_view_set_size(&test_child3, 30, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child3);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* main_size=90, free=110, between=27(=110/4), leading=27 */
    EGUI_TEST_ASSERT_EQUAL_INT(27,  test_child1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(84,  test_child2.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(141, test_child3.region.location.x);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_align_items_center(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 80);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_CENTER);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 30, 40);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 30, 60);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* cross_size=60; child1 center=(60-40)/2=10; child2 center=0 */
    EGUI_TEST_ASSERT_EQUAL_INT(10, test_child1.region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(0,  test_child2.region.location.y);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_align_items_stretch(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 80);
    /* align_items defaults to STRETCH */

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 30, 40);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 30, 60);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* cross_size=60; STRETCH => child1 height resized to 60 */
    EGUI_TEST_ASSERT_EQUAL_INT(60, test_child1.region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(60, test_child2.region.size.height);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_wrap(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 90, 120);
    egui_view_flexlayout_set_wrap(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_WRAP_WRAP);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);
    egui_view_flexlayout_set_align_content(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_JUSTIFY_START);

    /* 3 children each 50 wide — container is only 90 wide, so line 1=[c1,c2 would overflow], */
    /* Actually 50+50=100 > 90, so each child wraps to its own line */
    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 50, 30);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 50, 30);
    egui_view_init(&test_child3, core);
    egui_view_set_size(&test_child3, 50, 30);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child3);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* Each child on its own line, y-positions = 0, 30, 60 */
    EGUI_TEST_ASSERT_EQUAL_INT(0,  test_child1.region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(30, test_child2.region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(60, test_child3.region.location.y);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_flex_grow_single(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 60);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 50, 50);
    egui_view_set_flex_grow(&test_child1, 1);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 50, 50);
    /* child2 has flex_grow=0 (default) */

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* free=100; child1 grows by 100; child1.width=150; child2 stays at 50 */
    EGUI_TEST_ASSERT_EQUAL_INT(150, test_child1.region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(50,  test_child2.region.size.width);
    /* positions: child1@0, child2@150 */
    EGUI_TEST_ASSERT_EQUAL_INT(0,   test_child1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(150, test_child2.region.location.x);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_flex_grow_ratio(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 60);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 20, 50);
    egui_view_set_flex_grow(&test_child1, 2);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 20, 50);
    egui_view_set_flex_grow(&test_child2, 1);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* main_size=40, free=160; total_grow=3; child1+=106(=160*2/3), child2+=53(=160*1/3) */
    /* Note: integer division: 160*2/3=106, 160*1/3=53 */
    EGUI_TEST_ASSERT_EQUAL_INT(126, test_child1.region.size.width);  /* 20+106 */
    EGUI_TEST_ASSERT_EQUAL_INT(73,  test_child2.region.size.width);  /* 20+53 */

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_gap(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 60);
    egui_view_flexlayout_set_gap(EGUI_VIEW_OF(&test_fl), 0, 10);  /* col_gap=10 */
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 30, 50);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 40, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* ROW with col_gap=10: x[0]=0, x[1]=30+10=40 */
    EGUI_TEST_ASSERT_EQUAL_INT(0,  test_child1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(40, test_child2.region.location.x);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

static void test_fl_gone_skipped(void)
{
    egui_core_t *core = test_fl_get_core();

    egui_view_flexlayout_init(EGUI_VIEW_OF(&test_fl), core);
    egui_view_set_size(EGUI_VIEW_OF(&test_fl), 200, 60);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&test_fl), EGUI_FLEX_ALIGN_START);

    egui_view_init(&test_child1, core);
    egui_view_set_size(&test_child1, 30, 50);
    egui_view_init(&test_child2, core);
    egui_view_set_size(&test_child2, 30, 50);
    egui_view_set_gone(&test_child2, 1);
    egui_view_init(&test_child3, core);
    egui_view_set_size(&test_child3, 40, 50);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_fl), &test_child3);

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&test_fl));

    /* child2 is gone: child3 follows child1 directly at x=30 */
    EGUI_TEST_ASSERT_EQUAL_INT(0,  test_child1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(30, test_child3.region.location.x);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_fl));
}

/* ------------------------------------------------------------------ */

void test_flexlayout_run(void)
{
    EGUI_TEST_RUN(test_fl_init_defaults);
    EGUI_TEST_RUN(test_fl_row_nowrap);
    EGUI_TEST_RUN(test_fl_column_nowrap);
    EGUI_TEST_RUN(test_fl_justify_center);
    EGUI_TEST_RUN(test_fl_justify_end);
    EGUI_TEST_RUN(test_fl_justify_space_between);
    EGUI_TEST_RUN(test_fl_justify_space_around);
    EGUI_TEST_RUN(test_fl_justify_space_evenly);
    EGUI_TEST_RUN(test_fl_align_items_center);
    EGUI_TEST_RUN(test_fl_align_items_stretch);
    EGUI_TEST_RUN(test_fl_wrap);
    EGUI_TEST_RUN(test_fl_flex_grow_single);
    EGUI_TEST_RUN(test_fl_flex_grow_ratio);
    EGUI_TEST_RUN(test_fl_gap);
    EGUI_TEST_RUN(test_fl_gone_skipped);
}

#else /* EGUI_CONFIG_FUNCTION_FLEXLAYOUT */

void test_flexlayout_run(void)
{
    /* FlexLayout disabled — no tests */
}

#endif /* EGUI_CONFIG_FUNCTION_FLEXLAYOUT */
