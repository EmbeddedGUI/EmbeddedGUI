#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_view_get_parent.h"

static egui_view_t       s_view;
static egui_view_group_t s_parent;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_view,   0, sizeof(s_view));
    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_init(&s_view, core);
}

/* Before adding to a group, parent is NULL. */
static void test_get_parent_null_before_add(void)
{
    setup();
    EGUI_TEST_ASSERT_NULL(egui_view_get_parent(&s_view));
}

/* After adding to a group, parent is that group. */
static void test_get_parent_after_add(void)
{
    setup();
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_view);
    EGUI_TEST_ASSERT_EQUAL_INT((int)(uintptr_t)&s_parent,
                               (int)(uintptr_t)egui_view_get_parent(&s_view));
}

/* NULL self returns NULL without crash. */
static void test_get_parent_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_get_parent(NULL));
}

/* After set_parent, get_parent returns the supplied group. */
static void test_get_parent_after_set_parent(void)
{
    setup();
    egui_view_set_parent(&s_view, &s_parent);
    EGUI_TEST_ASSERT_EQUAL_INT((int)(uintptr_t)&s_parent,
                               (int)(uintptr_t)egui_view_get_parent(&s_view));
}

void test_view_get_parent_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_get_parent);

    EGUI_TEST_RUN(test_get_parent_null_before_add);
    EGUI_TEST_RUN(test_get_parent_after_add);
    EGUI_TEST_RUN(test_get_parent_null_self);
    EGUI_TEST_RUN(test_get_parent_after_set_parent);

    EGUI_TEST_SUITE_END();
}
