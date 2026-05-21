#include "egui.h"
#include "test/egui_test.h"
#include "test_event_lite.h"
#include "uicode_disp0.h"

#if EGUI_CONFIG_FUNCTION_EVENT_LITE

static int s_event_count;
static egui_event_code_t s_last_code;
static egui_view_t *s_last_target;
static void *s_last_param;
static void *s_last_user_data;

static void test_event_lite_cb(egui_event_t *event)
{
    s_event_count++;
    s_last_code = event->code;
    s_last_target = event->target;
    s_last_param = event->param;
    s_last_user_data = event->user_data;
}

static void test_event_lite_reset(void)
{
    s_event_count = 0;
    s_last_code = EGUI_EVENT_ALL;
    s_last_target = NULL;
    s_last_param = NULL;
    s_last_user_data = NULL;
}

static void test_event_lite_pressed_clicked_and_remove(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_t view;
    int user_data = 7;

    egui_view_init(&view, core);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_add_event_listener(&view, EGUI_EVENT_PRESSED, test_event_lite_cb, &user_data));

    test_event_lite_reset();
    egui_view_set_pressed(&view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_event_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_EVENT_PRESSED, s_last_code);
    EGUI_TEST_ASSERT_TRUE(s_last_target == &view);
    EGUI_TEST_ASSERT_TRUE(s_last_user_data == &user_data);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_remove_event_listener(&view, EGUI_EVENT_PRESSED, test_event_lite_cb, &user_data));
    test_event_lite_reset();
    egui_view_set_pressed(&view, 0);
    egui_view_set_pressed(&view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_event_count);
}

static void test_event_lite_click_and_size_events(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_t view;

    egui_view_init(&view, core);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_add_event_listener(&view, EGUI_EVENT_ALL, test_event_lite_cb, NULL));

    test_event_lite_reset();
    egui_view_perform_click(&view);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_event_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_EVENT_CLICKED, s_last_code);

    test_event_lite_reset();
    egui_view_set_size(&view, 24, 12);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_event_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_EVENT_SIZE_CHANGED, s_last_code);
    EGUI_TEST_ASSERT_TRUE(s_last_param == &view.region.size);
}

static void test_event_lite_tree_broadcast(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_group_t parent;
    egui_view_t child;

    egui_view_group_init(EGUI_VIEW_OF(&parent), core);
    egui_view_init(&child, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent), &child);
    egui_view_add_event_listener(EGUI_VIEW_OF(&parent), EGUI_EVENT_LANGUAGE_CHANGED, test_event_lite_cb, NULL);
    egui_view_add_event_listener(&child, EGUI_EVENT_LANGUAGE_CHANGED, test_event_lite_cb, NULL);

    test_event_lite_reset();
    egui_view_send_event_to_tree(EGUI_VIEW_OF(&parent), EGUI_EVENT_LANGUAGE_CHANGED, NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(2, s_event_count);

    egui_view_group_remove_child(EGUI_VIEW_OF(&parent), &child);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void test_event_lite_focus_events(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_t view;

    egui_focus_manager_clear_focus(core);
    egui_view_init(&view, core);
    egui_view_set_focusable(&view, 1);
    egui_view_add_event_listener(&view, EGUI_EVENT_FOCUSED, test_event_lite_cb, NULL);

    test_event_lite_reset();
    egui_focus_manager_set_focus(core, &view);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_event_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_EVENT_FOCUSED, s_last_code);

    egui_focus_manager_clear_focus(core);
}
#endif

#endif /* EGUI_CONFIG_FUNCTION_EVENT_LITE */

void test_event_lite_run(void)
{
    EGUI_TEST_SUITE_BEGIN(event_lite);

#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    EGUI_TEST_RUN(test_event_lite_pressed_clicked_and_remove);
    EGUI_TEST_RUN(test_event_lite_click_and_size_events);
    EGUI_TEST_RUN(test_event_lite_tree_broadcast);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    EGUI_TEST_RUN(test_event_lite_focus_events);
#endif
#endif

    EGUI_TEST_SUITE_END();
}
