#include <string.h>

#include "egui.h"
#include "widget/egui_view_viewpage.h"
#include "widget/egui_view_label.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_viewpage_get_current_page.h"

static egui_view_viewpage_t s_vp;
static egui_view_label_t    s_page0;
static egui_view_label_t    s_page1;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_vp,    0, sizeof(s_vp));
    memset(&s_page0, 0, sizeof(s_page0));
    memset(&s_page1, 0, sizeof(s_page1));
    egui_view_viewpage_init(EGUI_VIEW_OF(&s_vp), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_vp), 100, 100);
    egui_view_label_init(EGUI_VIEW_OF(&s_page0), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_page0), 100, 100);
    egui_view_label_init(EGUI_VIEW_OF(&s_page1), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_page1), 100, 100);
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&s_vp), EGUI_VIEW_OF(&s_page0));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&s_vp), EGUI_VIEW_OF(&s_page1));
}

/* Default current page after init with children is 0. */
static void test_viewpage_get_current_page_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_viewpage_get_current_page(EGUI_VIEW_OF(&s_vp)));
}

/* After set_current_page(1) the getter reflects the change. */
static void test_viewpage_get_current_page_after_set(void)
{
    setup();
    egui_view_viewpage_set_current_page(EGUI_VIEW_OF(&s_vp), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_viewpage_get_current_page(EGUI_VIEW_OF(&s_vp)));
}

/* set_current_page back to 0 is reflected. */
static void test_viewpage_get_current_page_back_to_zero(void)
{
    setup();
    egui_view_viewpage_set_current_page(EGUI_VIEW_OF(&s_vp), 1);
    egui_view_viewpage_set_current_page(EGUI_VIEW_OF(&s_vp), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_viewpage_get_current_page(EGUI_VIEW_OF(&s_vp)));
}

/* NULL self returns 0 without crash. */
static void test_viewpage_get_current_page_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_viewpage_get_current_page(NULL));
}

void test_viewpage_get_current_page_run(void)
{
    EGUI_TEST_SUITE_BEGIN(viewpage_get_current_page);

    EGUI_TEST_RUN(test_viewpage_get_current_page_default);
    EGUI_TEST_RUN(test_viewpage_get_current_page_after_set);
    EGUI_TEST_RUN(test_viewpage_get_current_page_back_to_zero);
    EGUI_TEST_RUN(test_viewpage_get_current_page_null_self);

    EGUI_TEST_SUITE_END();
}
