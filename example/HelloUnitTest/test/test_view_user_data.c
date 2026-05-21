#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_view_user_data.h"

#if EGUI_CONFIG_FUNCTION_VIEW_USER_DATA

static egui_view_t       s_view;
static egui_view_group_t s_parent;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_view, 0, sizeof(s_view));
    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_init(&s_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_view);
}

/* After init, user_data is NULL. */
static void test_user_data_init_null(void)
{
    setup();
    EGUI_TEST_ASSERT_NULL(egui_view_get_user_data(&s_view));
}

/* set_user_data stores and get_user_data retrieves the pointer. */
static void test_user_data_set_get(void)
{
    int value = 42;

    setup();
    egui_view_set_user_data(&s_view, &value);
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)&value, (int)(intptr_t)egui_view_get_user_data(&s_view));
}

/* set_user_data with NULL resets the pointer. */
static void test_user_data_set_null(void)
{
    int value = 7;

    setup();
    egui_view_set_user_data(&s_view, &value);
    egui_view_set_user_data(&s_view, NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_get_user_data(&s_view));
}

/* NULL self: set does not crash. */
static void test_user_data_null_self_set(void)
{
    egui_view_set_user_data(NULL, (void *)1);
    EGUI_TEST_ASSERT_TRUE(1);
}

/* NULL self: get returns NULL. */
static void test_user_data_null_self_get(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_get_user_data(NULL));
}

#endif /* EGUI_CONFIG_FUNCTION_VIEW_USER_DATA */

void test_view_user_data_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_user_data);

#if EGUI_CONFIG_FUNCTION_VIEW_USER_DATA
    EGUI_TEST_RUN(test_user_data_init_null);
    EGUI_TEST_RUN(test_user_data_set_get);
    EGUI_TEST_RUN(test_user_data_set_null);
    EGUI_TEST_RUN(test_user_data_null_self_set);
    EGUI_TEST_RUN(test_user_data_null_self_get);
#endif

    EGUI_TEST_SUITE_END();
}
