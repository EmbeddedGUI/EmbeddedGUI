#include <string.h>

#include "egui.h"
#include "widget/egui_view_viewpage_cache.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_viewpage_cache_get_state.h"

static egui_view_viewpage_cache_t s_vp;
static egui_view_t s_pages[3];
static int s_changed_page;
static int s_load_count;
static int s_free_count;

static void on_page_changed(egui_view_t *self, int current_page_index)
{
    EGUI_UNUSED(self);
    s_changed_page = current_page_index;
}

static void *on_page_load(egui_view_t *self, int current_page_index)
{
    EGUI_UNUSED(self);
    if (current_page_index < 0 || current_page_index >= 3)
    {
        return NULL;
    }
    s_load_count++;
    return &s_pages[current_page_index];
}

static void on_page_free(egui_view_t *self, int current_page_index, egui_view_t *page)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(current_page_index);
    EGUI_UNUSED(page);
    s_free_count++;
}

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_vp, 0, sizeof(s_vp));
    memset(s_pages, 0, sizeof(s_pages));
    s_changed_page = -1;
    s_load_count = 0;
    s_free_count = 0;

    egui_view_viewpage_cache_init(EGUI_VIEW_OF(&s_vp), core);
    egui_view_viewpage_cache_set_size(EGUI_VIEW_OF(&s_vp), 100, 80);
    for (int i = 0; i < 3; i++)
    {
        egui_view_init(&s_pages[i], core);
    }
}

static void test_viewpage_cache_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_viewpage_cache_get_child_total_cnt(EGUI_VIEW_OF(&s_vp)));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_viewpage_cache_get_current_page(EGUI_VIEW_OF(&s_vp)));
    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_cache_get_on_page_changed_listener(EGUI_VIEW_OF(&s_vp)));
    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_cache_get_on_page_load_listener(EGUI_VIEW_OF(&s_vp)));
    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_cache_get_on_page_free_listener(EGUI_VIEW_OF(&s_vp)));
}

static void test_viewpage_cache_get_state_total_count(void)
{
    setup();

    egui_view_viewpage_cache_set_child_total_cnt(EGUI_VIEW_OF(&s_vp), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_viewpage_cache_get_child_total_cnt(EGUI_VIEW_OF(&s_vp)));
}

static void test_viewpage_cache_get_state_listeners(void)
{
    setup();

    egui_view_viewpage_cache_set_on_page_changed_listener(EGUI_VIEW_OF(&s_vp), on_page_changed);
    egui_view_viewpage_cache_set_on_page_load_listener(EGUI_VIEW_OF(&s_vp), on_page_load);
    egui_view_viewpage_cache_set_on_page_free_listener(EGUI_VIEW_OF(&s_vp), on_page_free);
    EGUI_TEST_ASSERT_TRUE(egui_view_viewpage_cache_get_on_page_changed_listener(EGUI_VIEW_OF(&s_vp)) == on_page_changed);
    EGUI_TEST_ASSERT_TRUE(egui_view_viewpage_cache_get_on_page_load_listener(EGUI_VIEW_OF(&s_vp)) == on_page_load);
    EGUI_TEST_ASSERT_TRUE(egui_view_viewpage_cache_get_on_page_free_listener(EGUI_VIEW_OF(&s_vp)) == on_page_free);

    egui_view_viewpage_cache_set_on_page_changed_listener(EGUI_VIEW_OF(&s_vp), NULL);
    egui_view_viewpage_cache_set_on_page_load_listener(EGUI_VIEW_OF(&s_vp), NULL);
    egui_view_viewpage_cache_set_on_page_free_listener(EGUI_VIEW_OF(&s_vp), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_cache_get_on_page_changed_listener(EGUI_VIEW_OF(&s_vp)));
    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_cache_get_on_page_load_listener(EGUI_VIEW_OF(&s_vp)));
    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_cache_get_on_page_free_listener(EGUI_VIEW_OF(&s_vp)));
}

static void test_viewpage_cache_get_state_current_page(void)
{
    setup();

    egui_view_viewpage_cache_set_child_total_cnt(EGUI_VIEW_OF(&s_vp), 3);
    egui_view_viewpage_cache_set_on_page_changed_listener(EGUI_VIEW_OF(&s_vp), on_page_changed);
    egui_view_viewpage_cache_set_on_page_load_listener(EGUI_VIEW_OF(&s_vp), on_page_load);
    egui_view_viewpage_cache_set_on_page_free_listener(EGUI_VIEW_OF(&s_vp), on_page_free);

    egui_view_viewpage_cache_set_current_page(EGUI_VIEW_OF(&s_vp), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_viewpage_cache_get_current_page(EGUI_VIEW_OF(&s_vp)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_changed_page);
    EGUI_TEST_ASSERT_EQUAL_INT(2, s_load_count);

    egui_view_viewpage_cache_set_current_page(EGUI_VIEW_OF(&s_vp), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_viewpage_cache_get_current_page(EGUI_VIEW_OF(&s_vp)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, s_changed_page);
    EGUI_TEST_ASSERT_EQUAL_INT(4, s_load_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, s_free_count);
}

static void test_viewpage_cache_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_viewpage_cache_get_child_total_cnt(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_viewpage_cache_get_current_page(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_cache_get_on_page_changed_listener(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_cache_get_on_page_load_listener(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_viewpage_cache_get_on_page_free_listener(NULL));
}

void test_viewpage_cache_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(viewpage_cache_get_state);

    EGUI_TEST_RUN(test_viewpage_cache_get_state_defaults);
    EGUI_TEST_RUN(test_viewpage_cache_get_state_total_count);
    EGUI_TEST_RUN(test_viewpage_cache_get_state_listeners);
    EGUI_TEST_RUN(test_viewpage_cache_get_state_current_page);
    EGUI_TEST_RUN(test_viewpage_cache_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
