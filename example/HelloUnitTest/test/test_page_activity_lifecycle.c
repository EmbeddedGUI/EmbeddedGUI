#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_page_activity_lifecycle.h"

typedef struct test_lifecycle_page test_lifecycle_page_t;
typedef struct test_lifecycle_activity test_lifecycle_activity_t;

struct test_lifecycle_page
{
    egui_page_base_t base;
    egui_view_lyric_scroller_t lyric;
    egui_view_textblock_t textblock;
};

struct test_lifecycle_activity
{
    egui_activity_t base;
    egui_view_lyric_scroller_t lyric;
    egui_view_textblock_t textblock;
};

static test_lifecycle_page_t test_page_instance;
static test_lifecycle_activity_t test_activity_a;
static test_lifecycle_activity_t test_activity_b;

static const char *test_lifecycle_long_line = "A long single line lyric should overflow and scroll once it becomes attached inside the page or activity root.";

static void test_lifecycle_prepare_lyric(egui_view_t *view, egui_dim_t x, egui_dim_t y, egui_dim_t width)
{
    egui_view_lyric_scroller_init(view);
    egui_view_set_position(view, x, y);
    egui_view_set_size(view, width, 24);
    egui_view_set_padding(view, 2, 2, 0, 0);
    egui_view_lyric_scroller_set_font(view, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_lyric_scroller_set_interval_ms(view, 50);
    egui_view_lyric_scroller_set_scroll_step(view, 1);
    egui_view_lyric_scroller_set_pause_duration_ms(view, 0);
    egui_view_lyric_scroller_set_text(view, test_lifecycle_long_line);
}

static void test_lifecycle_prepare_textblock(egui_view_t *view, egui_dim_t x, egui_dim_t y, egui_dim_t width)
{
    egui_view_textblock_init(view);
    egui_view_set_position(view, x, y);
    egui_view_set_size(view, width, 28);
    egui_view_set_padding(view, 2, 2, 0, 0);
    egui_view_textblock_set_font(view, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_textblock_set_text(view, "Editable lifecycle text");
    egui_view_textblock_set_editable(view, 1);
}

static void test_lifecycle_page_on_open(egui_page_base_t *self)
{
    test_lifecycle_page_t *local = (test_lifecycle_page_t *)self;

    egui_page_base_on_open(self);
    test_lifecycle_prepare_lyric(EGUI_VIEW_OF(&local->lyric), 8, 8, 48);
    test_lifecycle_prepare_textblock(EGUI_VIEW_OF(&local->textblock), 8, 40, 112);

    egui_page_base_add_view(self, EGUI_VIEW_OF(&local->lyric));
    egui_page_base_add_view(self, EGUI_VIEW_OF(&local->textblock));
    egui_view_request_focus(EGUI_VIEW_OF(&local->textblock));
}

static void test_lifecycle_page_on_close(egui_page_base_t *self)
{
    egui_page_base_on_close(self);
}

static const egui_page_base_api_t test_lifecycle_page_api = {
        .on_open = test_lifecycle_page_on_open,
        .on_close = test_lifecycle_page_on_close,
        .on_refresh = NULL,
        .on_key_pressed = egui_page_base_on_key_pressed,
};

static void test_lifecycle_page_init(test_lifecycle_page_t *page)
{
    memset(page, 0, sizeof(*page));
    egui_page_base_init(&page->base);
    page->base.api = &test_lifecycle_page_api;
}

static void test_lifecycle_activity_on_create(egui_activity_t *self)
{
    test_lifecycle_activity_t *local = (test_lifecycle_activity_t *)self;

    egui_activity_on_create(self);
    test_lifecycle_prepare_lyric(EGUI_VIEW_OF(&local->lyric), 8, 8, 48);
    test_lifecycle_prepare_textblock(EGUI_VIEW_OF(&local->textblock), 8, 40, 112);

    egui_activity_add_view(self, EGUI_VIEW_OF(&local->lyric));
    egui_activity_add_view(self, EGUI_VIEW_OF(&local->textblock));
    egui_view_request_focus(EGUI_VIEW_OF(&local->textblock));
}

static void test_lifecycle_activity_on_destroy(egui_activity_t *self)
{
    egui_activity_on_destroy(self);
}

static const egui_activity_api_t test_lifecycle_activity_api = {
        .on_create = test_lifecycle_activity_on_create,
        .on_start = egui_activity_on_start,
        .on_resume = egui_activity_on_resume,
        .on_pause = egui_activity_on_pause,
        .on_stop = egui_activity_on_stop,
        .on_destroy = test_lifecycle_activity_on_destroy,
};

static void test_lifecycle_activity_init(test_lifecycle_activity_t *activity)
{
    memset(activity, 0, sizeof(*activity));
    egui_activity_init(&activity->base);
    activity->base.api = &test_lifecycle_activity_api;
}

static void test_page_open_close_propagates_attach_detach_to_dynamic_children(void)
{
    test_lifecycle_page_init(&test_page_instance);

    egui_page_base_open(&test_page_instance.base);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_page_instance.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_page_instance.lyric)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_page_instance.lyric.scroll_timer));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_page_instance.textblock)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_page_instance.textblock)->is_focused);
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_page_instance.textblock.cursor_timer));

    egui_page_base_close(&test_page_instance.base);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_page_instance.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_page_instance.lyric)->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_page_instance.lyric.scroll_timer));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_page_instance.textblock)->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_page_instance.textblock)->is_focused);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_page_instance.textblock.cursor_timer));
}

static void test_activity_switch_detaches_background_resources_and_resume_reattaches(void)
{
    test_lifecycle_activity_init(&test_activity_a);
    test_lifecycle_activity_init(&test_activity_b);

    egui_core_activity_start((egui_activity_t *)&test_activity_a, NULL);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_activity_a.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_activity_a.lyric.scroll_timer));
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_activity_a.textblock.cursor_timer));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_activity_a.textblock)->is_focused);

    egui_core_activity_start((egui_activity_t *)&test_activity_b, (egui_activity_t *)&test_activity_a);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ACTIVITY_STATE_STOP, test_activity_a.base.state);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_activity_a.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_activity_a.lyric.scroll_timer));
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_activity_a.textblock.cursor_timer));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_activity_a.textblock)->is_focused);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_activity_b.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_activity_b.lyric.scroll_timer));

    egui_core_activity_finish((egui_activity_t *)&test_activity_b);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_activity_a.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_activity_a.lyric.scroll_timer));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_activity_a.textblock)->is_focused);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_activity_a.textblock.cursor_timer));

    egui_core_activity_finish((egui_activity_t *)&test_activity_a);
    egui_focus_manager_clear_focus();
}

void test_page_activity_lifecycle_run(void)
{
    EGUI_TEST_SUITE_BEGIN(page_activity_lifecycle);
    EGUI_TEST_RUN(test_page_open_close_propagates_attach_detach_to_dynamic_children);
    EGUI_TEST_RUN(test_activity_switch_detaches_background_resources_and_resume_reattaches);
    EGUI_TEST_SUITE_END();
}
