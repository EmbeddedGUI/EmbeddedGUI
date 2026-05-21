#include <string.h>

#include "egui.h"
#include "widget/egui_view_viewpage.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_viewpage_get_state.h"

static egui_view_viewpage_t s_vp;
static int s_changed_page;

static void on_page_changed(egui_view_t *self, int page_index)
{
    EGUI_UNUSED(self);
    s_changed_page = page_index;
}

static void setup(void)
{
    memset(&s_vp, 0, sizeof(s_vp));
    s_changed_page = -1;
    egui_view_viewpage_init(EGUI_VIEW_OF(&s_vp), uicode_get_core());
}

static void test_viewpage_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_get_on_page_changed(EGUI_VIEW_OF(&s_vp)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_viewpage_get_scrollbar_enabled(EGUI_VIEW_OF(&s_vp)));
}

static void test_viewpage_get_state_on_page_changed(void)
{
    setup();

    egui_view_viewpage_set_on_page_changed(EGUI_VIEW_OF(&s_vp), on_page_changed);
    EGUI_TEST_ASSERT_TRUE(egui_view_viewpage_get_on_page_changed(EGUI_VIEW_OF(&s_vp)) == on_page_changed);

    egui_view_viewpage_set_on_page_changed(EGUI_VIEW_OF(&s_vp), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_get_on_page_changed(EGUI_VIEW_OF(&s_vp)));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, s_changed_page);
}

static void test_viewpage_get_state_scrollbar_enabled(void)
{
    setup();

    egui_view_viewpage_set_scrollbar_enabled(EGUI_VIEW_OF(&s_vp), 1);
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_viewpage_get_scrollbar_enabled(EGUI_VIEW_OF(&s_vp)));
#else
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_viewpage_get_scrollbar_enabled(EGUI_VIEW_OF(&s_vp)));
#endif

    egui_view_viewpage_set_scrollbar_enabled(EGUI_VIEW_OF(&s_vp), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_viewpage_get_scrollbar_enabled(EGUI_VIEW_OF(&s_vp)));
}

static void test_viewpage_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_get_on_page_changed(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_viewpage_get_scrollbar_enabled(NULL));
}

void test_viewpage_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(viewpage_get_state);

    EGUI_TEST_RUN(test_viewpage_get_state_defaults);
    EGUI_TEST_RUN(test_viewpage_get_state_on_page_changed);
    EGUI_TEST_RUN(test_viewpage_get_state_scrollbar_enabled);
    EGUI_TEST_RUN(test_viewpage_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
