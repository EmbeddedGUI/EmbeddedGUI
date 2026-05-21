#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_scroll_horizontal.h"

#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL

static egui_view_scroll_t s_scroll;
static egui_view_label_t  s_item;

static egui_core_t *get_core(void)
{
    egui_core_t *core = uicode_get_core();
    EGUI_ASSERT(core != NULL);
    return core;
}

static void setup_vertical(void)
{
    egui_core_t *core = get_core();

    memset(&s_scroll, 0, sizeof(s_scroll));
    memset(&s_item,   0, sizeof(s_item));

    egui_view_scroll_init(EGUI_VIEW_OF(&s_scroll), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_scroll), 100, 100);
}

static void setup_horizontal(void)
{
    egui_core_t *core = get_core();

    memset(&s_scroll, 0, sizeof(s_scroll));
    memset(&s_item,   0, sizeof(s_item));

    egui_view_scroll_init(EGUI_VIEW_OF(&s_scroll), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_scroll), 100, 100);
    egui_view_scroll_set_horizontal(EGUI_VIEW_OF(&s_scroll), 1);
}

/* Default scroll is vertical (is_horizontal == 0). */
static void test_scroll_default_vertical(void)
{
    setup_vertical();
    EGUI_TEST_ASSERT_FALSE(s_scroll.is_horizontal);
}

/* set_horizontal(1) sets is_horizontal to 1. */
static void test_scroll_set_horizontal_flag(void)
{
    setup_horizontal();
    EGUI_TEST_ASSERT_TRUE(s_scroll.is_horizontal);
}

/* set_horizontal(0) resets flag back to 0. */
static void test_scroll_set_horizontal_reset(void)
{
    setup_horizontal();
    egui_view_scroll_set_horizontal(EGUI_VIEW_OF(&s_scroll), 0);
    EGUI_TEST_ASSERT_FALSE(s_scroll.is_horizontal);
}

/* NULL-safe: set_horizontal on NULL does not crash. */
static void test_scroll_set_horizontal_null_safe(void)
{
    egui_view_scroll_set_horizontal(NULL, 1);
    EGUI_TEST_ASSERT_TRUE(1); /* no crash */
}

/* Horizontal scroll: set_size does not crash with horizontal flag. */
static void test_scroll_horizontal_set_size_no_crash(void)
{
    setup_horizontal();
    egui_view_label_init(EGUI_VIEW_OF(&s_item), get_core());
    egui_view_scroll_add_child(EGUI_VIEW_OF(&s_scroll), EGUI_VIEW_OF(&s_item));
    egui_view_scroll_set_size(EGUI_VIEW_OF(&s_scroll), 100, 100);
    EGUI_TEST_ASSERT_TRUE(1); /* no crash */
}

#endif /* EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL */

void test_scroll_horizontal_run(void)
{
    EGUI_TEST_SUITE_BEGIN(scroll_horizontal);

#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
    EGUI_TEST_RUN(test_scroll_default_vertical);
    EGUI_TEST_RUN(test_scroll_set_horizontal_flag);
    EGUI_TEST_RUN(test_scroll_set_horizontal_reset);
    EGUI_TEST_RUN(test_scroll_set_horizontal_null_safe);
    EGUI_TEST_RUN(test_scroll_horizontal_set_size_no_crash);
#endif

    EGUI_TEST_SUITE_END();
}
