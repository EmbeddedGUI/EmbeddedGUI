#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_app_timer_helpers.h"

static void test_app_timer_helpers_noop_cb(egui_timer_t *timer)
{
    EGUI_UNUSED(timer);
}

static egui_core_t *test_app_timer_helpers_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void test_page_base_timer_helpers_use_init_core(void)
{
    egui_core_t *core = test_app_timer_helpers_get_core();
    egui_page_base_t page;
    egui_timer_t timer;

    egui_page_base_init(&page, core);
    egui_timer_init_timer(&timer, NULL, test_app_timer_helpers_noop_cb);

    EGUI_TEST_ASSERT_FALSE(egui_page_base_check_timer_start(&page, &timer));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_page_base_start_timer(&page, &timer, 100, 0));
    EGUI_TEST_ASSERT_TRUE(egui_page_base_check_timer_start(&page, &timer));

    egui_page_base_stop_timer(&page, &timer);
    EGUI_TEST_ASSERT_FALSE(egui_page_base_check_timer_start(&page, &timer));
}

static void test_activity_timer_helpers_use_init_core(void)
{
    egui_core_t *core = test_app_timer_helpers_get_core();
    egui_activity_t activity;
    egui_timer_t timer;

    egui_activity_init(&activity, core);
    egui_timer_init_timer(&timer, NULL, test_app_timer_helpers_noop_cb);

    EGUI_TEST_ASSERT_FALSE(egui_activity_check_timer_start(&activity, &timer));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_activity_start_timer(&activity, &timer, 100, 0));
    EGUI_TEST_ASSERT_TRUE(egui_activity_check_timer_start(&activity, &timer));

    egui_activity_stop_timer(&activity, &timer);
    EGUI_TEST_ASSERT_FALSE(egui_activity_check_timer_start(&activity, &timer));
}

static void test_dialog_timer_helpers_use_init_core(void)
{
    egui_core_t *core = test_app_timer_helpers_get_core();
    egui_dialog_t dialog;
    egui_timer_t timer;

    egui_dialog_init(&dialog, core);
    egui_timer_init_timer(&timer, NULL, test_app_timer_helpers_noop_cb);

    EGUI_TEST_ASSERT_FALSE(egui_dialog_check_timer_start(&dialog, &timer));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_dialog_start_timer(&dialog, &timer, 100, 0));
    EGUI_TEST_ASSERT_TRUE(egui_dialog_check_timer_start(&dialog, &timer));

    egui_dialog_stop_timer(&dialog, &timer);
    EGUI_TEST_ASSERT_FALSE(egui_dialog_check_timer_start(&dialog, &timer));
}

static void test_app_timer_helpers_require_initialized_core(void)
{
    egui_page_base_t page;
    egui_activity_t activity;
    egui_dialog_t dialog;
    egui_timer_t page_timer;
    egui_timer_t activity_timer;
    egui_timer_t dialog_timer;

    egui_api_memset(&page, 0, sizeof(page));
    egui_api_memset(&activity, 0, sizeof(activity));
    egui_api_memset(&dialog, 0, sizeof(dialog));

    egui_timer_init_timer(&page_timer, NULL, test_app_timer_helpers_noop_cb);
    egui_timer_init_timer(&activity_timer, NULL, test_app_timer_helpers_noop_cb);
    egui_timer_init_timer(&dialog_timer, NULL, test_app_timer_helpers_noop_cb);

    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_page_base_start_timer(&page, &page_timer, 80, 0));
    EGUI_TEST_ASSERT_FALSE(egui_page_base_check_timer_start(&page, &page_timer));
    egui_page_base_stop_timer(&page, &page_timer);

    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_activity_start_timer(&activity, &activity_timer, 80, 0));
    EGUI_TEST_ASSERT_FALSE(egui_activity_check_timer_start(&activity, &activity_timer));
    egui_activity_stop_timer(&activity, &activity_timer);

    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_dialog_start_timer(&dialog, &dialog_timer, 80, 0));
    EGUI_TEST_ASSERT_FALSE(egui_dialog_check_timer_start(&dialog, &dialog_timer));
    egui_dialog_stop_timer(&dialog, &dialog_timer);
}

void test_app_timer_helpers_run(void)
{
    EGUI_TEST_SUITE_BEGIN(app_timer_helpers);
    EGUI_TEST_RUN(test_page_base_timer_helpers_use_init_core);
    EGUI_TEST_RUN(test_activity_timer_helpers_use_init_core);
    EGUI_TEST_RUN(test_dialog_timer_helpers_use_init_core);
    EGUI_TEST_RUN(test_app_timer_helpers_require_initialized_core);
    EGUI_TEST_SUITE_END();
}
