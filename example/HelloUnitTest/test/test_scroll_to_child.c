#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_scroll_to_child.h"

static egui_view_scroll_t s_scroll;
static egui_view_label_t  s_item0; /* y=0,   h=60 → bottom=60  */
static egui_view_label_t  s_item1; /* y=60,  h=60 → bottom=120 */
static egui_view_label_t  s_item2; /* y=120, h=60 → bottom=180 */

#define SCROLL_W 100
#define SCROLL_H 100   /* viewport height */
#define ITEM_H   60

/* Returns current scroll offset (pixels from top). */
static egui_dim_t get_scroll_y(void)
{
    egui_view_t *container = EGUI_VIEW_OF(&s_scroll.container);
    egui_dim_t y = container->region.location.y;
    return (y < 0) ? (egui_dim_t)(-y) : 0;
}

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

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

    egui_view_scroll_layout_childs(EGUI_VIEW_OF(&s_scroll));
    /* After layout: item0 y=0, item1 y=60, item2 y=120 (container h=180). */
}

/* NULL self must not crash. */
static void test_null_safe_self(void)
{
    setup();
    egui_view_scroll_scroll_to_child(NULL, EGUI_VIEW_OF(&s_item0), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_scroll_y());
}

/* NULL child must not crash. */
static void test_null_safe_child(void)
{
    setup();
    egui_view_scroll_scroll_to_child(EGUI_VIEW_OF(&s_scroll), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_scroll_y());
}

/* item0 is already fully visible at scroll_y=0 — no scroll should occur. */
static void test_already_visible(void)
{
    setup();
    egui_view_scroll_scroll_to_child(EGUI_VIEW_OF(&s_scroll), EGUI_VIEW_OF(&s_item0), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_scroll_y());
}

/* item2 (y=120, h=60, bottom=180) is below the viewport (h=100) at scroll_y=0.
 * Minimum scroll needed: 180-100=80. */
static void test_child_below(void)
{
    setup();
    egui_view_scroll_scroll_to_child(EGUI_VIEW_OF(&s_scroll), EGUI_VIEW_OF(&s_item2), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)get_scroll_y());
}

/* item1 (y=60, h=60, bottom=120) is partially below the viewport at scroll_y=0.
 * Minimum scroll needed: 120-100=20. */
static void test_child_partial_below(void)
{
    setup();
    egui_view_scroll_scroll_to_child(EGUI_VIEW_OF(&s_scroll), EGUI_VIEW_OF(&s_item1), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(20, (int)get_scroll_y());
}

/* item0 (y=0) is above the viewport when we have scrolled to scroll_y=50.
 * Scroll back to 0 to reveal the top edge of item0. */
static void test_child_above(void)
{
    setup();
    /* Scroll down by 50 first. */
    egui_view_scroll_start_container_scroll(EGUI_VIEW_OF(&s_scroll), -50);
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)get_scroll_y());

    egui_view_scroll_scroll_to_child(EGUI_VIEW_OF(&s_scroll), EGUI_VIEW_OF(&s_item0), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_scroll_y());
}

/* Calling scroll_to_child twice to the same child must be idempotent
 * (second call is a no-op because child is already visible). */
static void test_idempotent(void)
{
    setup();
    egui_view_scroll_scroll_to_child(EGUI_VIEW_OF(&s_scroll), EGUI_VIEW_OF(&s_item2), 0);
    egui_dim_t first = get_scroll_y();
    egui_view_scroll_scroll_to_child(EGUI_VIEW_OF(&s_scroll), EGUI_VIEW_OF(&s_item2), 0);
    EGUI_TEST_ASSERT_EQUAL_INT((int)first, (int)get_scroll_y());
}

void test_scroll_to_child_run(void)
{
    EGUI_TEST_RUN(test_null_safe_self);
    EGUI_TEST_RUN(test_null_safe_child);
    EGUI_TEST_RUN(test_already_visible);
    EGUI_TEST_RUN(test_child_below);
    EGUI_TEST_RUN(test_child_partial_below);
    EGUI_TEST_RUN(test_child_above);
    EGUI_TEST_RUN(test_idempotent);
}
