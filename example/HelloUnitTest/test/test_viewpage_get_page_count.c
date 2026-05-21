#include <string.h>

#include "egui.h"
#include "widget/egui_view_viewpage.h"
#include "widget/egui_view.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_viewpage_get_page_count.h"

static egui_view_viewpage_t s_vp;
static egui_view_t s_page1;
static egui_view_t s_page2;

static void setup(void)
{
    memset(&s_vp, 0, sizeof(s_vp));
    memset(&s_page1, 0, sizeof(s_page1));
    memset(&s_page2, 0, sizeof(s_page2));
    egui_view_viewpage_init(EGUI_VIEW_OF(&s_vp), uicode_get_core());
    egui_view_init(&s_page1, uicode_get_core());
    egui_view_init(&s_page2, uicode_get_core());
}

/* Default page count after init is 0. */
static void test_viewpage_get_page_count_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_viewpage_get_page_count(EGUI_VIEW_OF(&s_vp)));
}

/* After adding one child, getter returns 1. */
static void test_viewpage_get_page_count_one(void)
{
    setup();
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&s_vp), &s_page1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_viewpage_get_page_count(EGUI_VIEW_OF(&s_vp)));
}

/* After adding two children, getter returns 2. */
static void test_viewpage_get_page_count_two(void)
{
    setup();
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&s_vp), &s_page1);
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&s_vp), &s_page2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_viewpage_get_page_count(EGUI_VIEW_OF(&s_vp)));
}

/* NULL self returns 0 without crash. */
static void test_viewpage_get_page_count_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_viewpage_get_page_count(NULL));
}

void test_viewpage_get_page_count_run(void)
{
    EGUI_TEST_SUITE_BEGIN(viewpage_get_page_count);

    EGUI_TEST_RUN(test_viewpage_get_page_count_default);
    EGUI_TEST_RUN(test_viewpage_get_page_count_one);
    EGUI_TEST_RUN(test_viewpage_get_page_count_two);
    EGUI_TEST_RUN(test_viewpage_get_page_count_null_self);

    EGUI_TEST_SUITE_END();
}
