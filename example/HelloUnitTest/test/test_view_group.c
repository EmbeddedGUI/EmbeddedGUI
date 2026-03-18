#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_view_group.h"

static egui_view_group_t test_group;
static egui_view_t test_child1;
static egui_view_t test_child2;
static egui_view_t test_child3;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int g_child1_click_count;
static int g_child2_click_count;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_view_group_child1_click_cb(egui_view_t *self)
{
    EGUI_UNUSED(self);
    g_child1_click_count++;
}

static void test_view_group_child2_click_cb(egui_view_t *self)
{
    EGUI_UNUSED(self);
    g_child2_click_count++;
}

static void test_view_group_setup_touch_children(void)
{
    egui_region_t root_region = {{0, 0}, {220, 80}};

    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);

    egui_view_layout(EGUI_VIEW_OF(&test_group), &root_region);
    egui_view_set_position(&test_child1, 10, 10);
    egui_view_set_size(&test_child1, 80, 40);
    egui_view_set_position(&test_child2, 120, 10);
    egui_view_set_size(&test_child2, 80, 40);
    egui_view_set_on_click_listener(&test_child1, test_view_group_child1_click_cb);
    egui_view_set_on_click_listener(&test_child2, test_view_group_child2_click_cb);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2);
    EGUI_VIEW_OF(&test_group)->api->calculate_layout(EGUI_VIEW_OF(&test_group));

    g_child1_click_count = 0;
    g_child2_click_count = 0;
}

static int test_view_group_send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_group)->api->dispatch_touch_event(EGUI_VIEW_OF(&test_group), &event);
}
#endif

static void test_vg_init_defaults(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_NULL(egui_view_group_get_first_child(EGUI_VIEW_OF(&test_group)));
}

static void test_vg_add_child(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_first_child(EGUI_VIEW_OF(&test_group)) == &test_child1);
}

static void test_vg_add_multiple(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);
    egui_view_init(&test_child3);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child3);

    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
}

static void test_vg_parent_link(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    EGUI_TEST_ASSERT_TRUE(test_child1.parent == &test_group);
}

static void test_vg_remove_child(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));

    egui_view_group_remove_child(EGUI_VIEW_OF(&test_group), &test_child1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
}

static void test_vg_clear_childs(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);
    egui_view_init(&test_child3);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_group));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_vg_release_over_sibling_does_not_trigger_click(void)
{
    test_view_group_setup_touch_children();

    EGUI_TEST_ASSERT_TRUE(test_view_group_send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 20, 20));
    EGUI_TEST_ASSERT_TRUE(test_child1.is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_child1_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_child2_click_count);

    EGUI_TEST_ASSERT_TRUE(test_view_group_send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 140, 20));
    EGUI_TEST_ASSERT_FALSE(test_child1.is_pressed);
    EGUI_TEST_ASSERT_FALSE(test_child2.is_pressed);

    EGUI_TEST_ASSERT_TRUE(test_view_group_send_touch(EGUI_MOTION_EVENT_ACTION_UP, 140, 20));
    EGUI_TEST_ASSERT_FALSE(test_child1.is_pressed);
    EGUI_TEST_ASSERT_FALSE(test_child2.is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_child1_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_child2_click_count);
}
#endif

void test_view_group_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_group);

    EGUI_TEST_RUN(test_vg_init_defaults);
    EGUI_TEST_RUN(test_vg_add_child);
    EGUI_TEST_RUN(test_vg_add_multiple);
    EGUI_TEST_RUN(test_vg_parent_link);
    EGUI_TEST_RUN(test_vg_remove_child);
    EGUI_TEST_RUN(test_vg_clear_childs);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_vg_release_over_sibling_does_not_trigger_click);
#endif

    EGUI_TEST_SUITE_END();
}
