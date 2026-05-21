#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_scroll_to_pos.h"

static egui_view_scroll_t s_scroll;
static egui_view_label_t  s_item0;
static egui_view_label_t  s_item1;
static egui_view_label_t  s_item2;

#define SCROLL_W  100
#define SCROLL_H  100
#define ITEM_H     80

/* Returns the current vertical scroll offset (positive = scrolled down). */
static egui_dim_t get_scroll_y(void)
{
    egui_view_t *container = EGUI_VIEW_OF(&s_scroll.container);
    egui_dim_t y = container->region.location.y;
    return (y < 0) ? (egui_dim_t)(-y) : 0;
}

static void setup_vertical(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_scroll, 0, sizeof(s_scroll));
    memset(&s_item0,  0, sizeof(s_item0));
    memset(&s_item1,  0, sizeof(s_item1));
    memset(&s_item2,  0, sizeof(s_item2));

    egui_view_scroll_init(EGUI_VIEW_OF(&s_scroll), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_scroll), SCROLL_W, SCROLL_H);

    egui_view_label_init(EGUI_VIEW_OF(&s_item0), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_item0), SCROLL_W, ITEM_H);

    egui_view_label_init(EGUI_VIEW_OF(&s_item1), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_item1), SCROLL_W, ITEM_H);

    egui_view_label_init(EGUI_VIEW_OF(&s_item2), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_item2), SCROLL_W, ITEM_H);

    egui_view_scroll_add_child(EGUI_VIEW_OF(&s_scroll), EGUI_VIEW_OF(&s_item0));
    egui_view_scroll_add_child(EGUI_VIEW_OF(&s_scroll), EGUI_VIEW_OF(&s_item1));
    egui_view_scroll_add_child(EGUI_VIEW_OF(&s_scroll), EGUI_VIEW_OF(&s_item2));

    /* Layout: item0 y=0, item1 y=80, item2 y=160. Container h = 240. */
    egui_view_scroll_layout_childs(EGUI_VIEW_OF(&s_scroll));
}

/* NULL self must not crash. */
static void test_scroll_to_y_null_safe(void)
{
    egui_view_scroll_to_y(NULL, 50, 0);
    EGUI_TEST_ASSERT_TRUE(1);
}

/* NULL self for x must not crash. */
static void test_scroll_to_x_null_safe(void)
{
    egui_view_scroll_to_x(NULL, 50, 0);
    EGUI_TEST_ASSERT_TRUE(1);
}

/* Scrolling to y=0 keeps offset at 0. */
static void test_scroll_to_y_zero(void)
{
    setup_vertical();
    egui_view_scroll_to_y(EGUI_VIEW_OF(&s_scroll), 0, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_scroll_y());
}

/* Scroll to a position within [0, max_scroll]. Content h=240, viewport=100 max=140. */
static void test_scroll_to_y_within_range(void)
{
    setup_vertical();
    egui_view_scroll_to_y(EGUI_VIEW_OF(&s_scroll), 80, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)get_scroll_y());
}

/* Scroll position is clamped to max_scroll (=140). */
static void test_scroll_to_y_clamped(void)
{
    setup_vertical();
    egui_view_scroll_to_y(EGUI_VIEW_OF(&s_scroll), 9999, 0);
    /* max = 240 - 100 = 140 */
    EGUI_TEST_ASSERT_EQUAL_INT(140, (int)get_scroll_y());
}

void test_scroll_to_pos_run(void)
{
    EGUI_TEST_SUITE_BEGIN(scroll_to_pos);

    EGUI_TEST_RUN(test_scroll_to_y_null_safe);
    EGUI_TEST_RUN(test_scroll_to_x_null_safe);
    EGUI_TEST_RUN(test_scroll_to_y_zero);
    EGUI_TEST_RUN(test_scroll_to_y_within_range);
    EGUI_TEST_RUN(test_scroll_to_y_clamped);
    EGUI_TEST_RUN(test_scroll_to_y_zero);
    EGUI_TEST_RUN(test_scroll_to_y_within_range);
    EGUI_TEST_RUN(test_scroll_to_y_clamped);

    EGUI_TEST_SUITE_END();
}
