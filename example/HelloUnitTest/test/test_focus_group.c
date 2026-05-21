#include "egui.h"
#include "test/egui_test.h"
#include "test_focus_group.h"
#include "uicode_disp0.h"

#if EGUI_CONFIG_FUNCTION_FOCUS_GROUP

static void test_focus_group_next_prev_and_remove(void)
{
    egui_core_t *core = uicode_get_core();
    egui_focus_group_t group;
    egui_view_t first;
    egui_view_t second;

    egui_focus_manager_clear_focus(core);
    egui_view_init(&first, core);
    egui_view_init(&second, core);
    egui_view_set_focusable(&first, 1);
    egui_view_set_focusable(&second, 1);

    egui_focus_group_init(&group, core);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_focus_group_add_view(&group, &first));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_focus_group_add_view(&group, &second));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_focus_group_get_count(&group));

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_focus_group_focus_index(&group, 0));
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == &first);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_focus_group_focus_next(&group));
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == &second);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_focus_group_get_index(&group));

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_focus_group_focus_next(&group));
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == &first);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_focus_group_remove_view(&group, &first));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_focus_group_get_count(&group));

    egui_focus_manager_clear_focus(core);
}

static void test_focus_group_clamp_mode(void)
{
    egui_core_t *core = uicode_get_core();
    egui_focus_group_t group;
    egui_view_t first;
    egui_view_t second;

    egui_focus_manager_clear_focus(core);
    egui_view_init(&first, core);
    egui_view_init(&second, core);
    egui_view_set_focusable(&first, 1);
    egui_view_set_focusable(&second, 1);

    egui_focus_group_init(&group, core);
    egui_focus_group_add_view(&group, &first);
    egui_focus_group_add_view(&group, &second);
    egui_focus_group_set_wrap_mode(&group, EGUI_FOCUS_GROUP_CLAMP);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_focus_group_focus_index(&group, 1));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_focus_group_focus_next(&group));
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == &second);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_focus_group_get_index(&group));

    egui_focus_manager_clear_focus(core);
}

#endif /* EGUI_CONFIG_FUNCTION_FOCUS_GROUP */

void test_focus_group_run(void)
{
    EGUI_TEST_SUITE_BEGIN(focus_group);

#if EGUI_CONFIG_FUNCTION_FOCUS_GROUP
    EGUI_TEST_RUN(test_focus_group_next_prev_and_remove);
    EGUI_TEST_RUN(test_focus_group_clamp_mode);
#endif

    EGUI_TEST_SUITE_END();
}
