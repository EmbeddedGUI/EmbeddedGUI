#include <string.h>

#include "egui.h"
#include "widget/egui_view_scroll.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_scroll_is_at_top_bottom.h"

static egui_view_scroll_t s_scroll;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_scroll, 0, sizeof(s_scroll));
    egui_view_scroll_init(EGUI_VIEW_OF(&s_scroll), core);
}

/* Freshly initialized scroll is at top (scroll_y == 0). */
static void test_scroll_at_top_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_scroll_is_at_top(EGUI_VIEW_OF(&s_scroll)));
}

/* When content fits viewport completely, view is at bottom too. */
static void test_scroll_at_bottom_small_content(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_scroll_is_at_bottom(EGUI_VIEW_OF(&s_scroll)));
}

/* NULL self for is_at_top returns 0 without crash. */
static void test_scroll_null_self_top(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_is_at_top(NULL));
}

/* NULL self for is_at_bottom returns 0 without crash. */
static void test_scroll_null_self_bottom(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_is_at_bottom(NULL));
}

void test_scroll_is_at_top_bottom_run(void)
{
    EGUI_TEST_SUITE_BEGIN(scroll_is_at_top_bottom);

    EGUI_TEST_RUN(test_scroll_at_top_default);
    EGUI_TEST_RUN(test_scroll_at_bottom_small_content);
    EGUI_TEST_RUN(test_scroll_null_self_top);
    EGUI_TEST_RUN(test_scroll_null_self_bottom);

    EGUI_TEST_SUITE_END();
}
