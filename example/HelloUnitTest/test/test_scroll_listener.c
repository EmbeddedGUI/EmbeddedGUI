#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_scroll_listener.h"

#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER

static egui_view_scroll_t s_scroll;
static egui_view_label_t  s_item;
static egui_dim_t         s_last_scroll_y;
static int                s_cb_count;

static egui_core_t *get_core(void)
{
    egui_core_t *core = uicode_get_core();
    EGUI_ASSERT(core != NULL);
    return core;
}

static void on_scroll_cb(egui_view_t *self, egui_dim_t scroll_y)
{
    EGUI_UNUSED(self);
    s_last_scroll_y = scroll_y;
    s_cb_count++;
}

static void setup(void)
{
    egui_core_t *core = get_core();

    memset(&s_scroll, 0, sizeof(s_scroll));
    memset(&s_item,   0, sizeof(s_item));
    s_last_scroll_y = 0;
    s_cb_count = 0;

    egui_view_scroll_init(EGUI_VIEW_OF(&s_scroll), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_scroll), 100, 100);

    /* Lay out the scroll view so region_screen is populated. */
    egui_region_t r;
    egui_region_init(&r, 0, 0, 100, 100);
    egui_view_layout(EGUI_VIEW_OF(&s_scroll), &r);
    egui_region_copy(&EGUI_VIEW_OF(&s_scroll)->region_screen, &r);

    /* Add a tall item so there is room to scroll. */
    egui_view_label_init(EGUI_VIEW_OF(&s_item), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_item), 100, 300);
    egui_view_scroll_add_child(EGUI_VIEW_OF(&s_scroll), EGUI_VIEW_OF(&s_item));
    egui_view_scroll_layout_childs(EGUI_VIEW_OF(&s_scroll));
}

/* on_scroll listener is NULL by default. */
static void test_listener_default_null(void)
{
    setup();
    EGUI_TEST_ASSERT_NULL(egui_view_scroll_get_on_scroll_listener(EGUI_VIEW_OF(&s_scroll)));
    /* Just verify get_scroll_y works when nothing has scrolled. */
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_scroll_y(EGUI_VIEW_OF(&s_scroll)));
}

/* NULL-safe get_scroll_y. */
static void test_listener_null_safe(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_scroll_get_on_scroll_listener(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_scroll_y(NULL));
}

static void test_listener_get_on_scroll_listener(void)
{
    setup();
    egui_view_scroll_set_on_scroll_listener(EGUI_VIEW_OF(&s_scroll), on_scroll_cb);
    EGUI_TEST_ASSERT_TRUE(egui_view_scroll_get_on_scroll_listener(EGUI_VIEW_OF(&s_scroll)) == on_scroll_cb);

    egui_view_scroll_set_on_scroll_listener(EGUI_VIEW_OF(&s_scroll), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_scroll_get_on_scroll_listener(EGUI_VIEW_OF(&s_scroll)));
}

/* Callback fires when scroll occurs (diff_y < 0 path). */
static void test_listener_fires_on_scroll_up(void)
{
    setup();
    egui_view_scroll_set_on_scroll_listener(EGUI_VIEW_OF(&s_scroll), on_scroll_cb);
    egui_view_scroll_start_container_scroll(EGUI_VIEW_OF(&s_scroll), -30);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_cb_count);
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)s_last_scroll_y);
}

/* Callback fires when scroll occurs (diff_y > 0 path, scroll back down). */
static void test_listener_fires_on_scroll_down(void)
{
    setup();
    egui_view_scroll_set_on_scroll_listener(EGUI_VIEW_OF(&s_scroll), on_scroll_cb);
    /* First scroll up to create room to scroll down. */
    egui_view_scroll_start_container_scroll(EGUI_VIEW_OF(&s_scroll), -50);
    s_cb_count = 0;
    /* Scroll back down. */
    egui_view_scroll_start_container_scroll(EGUI_VIEW_OF(&s_scroll), 20);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_cb_count);
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)s_last_scroll_y);
}

/* No callback when listener is NULL (no crash). */
static void test_listener_null_no_crash(void)
{
    setup();
    /* Listener intentionally left as NULL. */
    egui_view_scroll_start_container_scroll(EGUI_VIEW_OF(&s_scroll), -30);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_cb_count); /* no callback fired */
}

/* get_scroll_y reflects actual scroll offset. */
static void test_listener_get_scroll_y(void)
{
    setup();
    egui_view_scroll_start_container_scroll(EGUI_VIEW_OF(&s_scroll), -40);
    EGUI_TEST_ASSERT_EQUAL_INT(40, (int)egui_view_scroll_get_scroll_y(EGUI_VIEW_OF(&s_scroll)));
}

/* Multiple scrolls accumulate correctly. */
static void test_listener_multiple_scrolls(void)
{
    setup();
    egui_view_scroll_set_on_scroll_listener(EGUI_VIEW_OF(&s_scroll), on_scroll_cb);
    egui_view_scroll_start_container_scroll(EGUI_VIEW_OF(&s_scroll), -20);
    egui_view_scroll_start_container_scroll(EGUI_VIEW_OF(&s_scroll), -20);
    EGUI_TEST_ASSERT_EQUAL_INT(2, s_cb_count);
    EGUI_TEST_ASSERT_EQUAL_INT(40, (int)s_last_scroll_y);
}

#endif /* EGUI_CONFIG_FUNCTION_SCROLL_LISTENER */

void test_scroll_listener_run(void)
{
#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
    EGUI_TEST_RUN(test_listener_default_null);
    EGUI_TEST_RUN(test_listener_null_safe);
    EGUI_TEST_RUN(test_listener_get_on_scroll_listener);
    EGUI_TEST_RUN(test_listener_fires_on_scroll_up);
    EGUI_TEST_RUN(test_listener_fires_on_scroll_down);
    EGUI_TEST_RUN(test_listener_null_no_crash);
    EGUI_TEST_RUN(test_listener_get_scroll_y);
    EGUI_TEST_RUN(test_listener_multiple_scrolls);
#endif
}
