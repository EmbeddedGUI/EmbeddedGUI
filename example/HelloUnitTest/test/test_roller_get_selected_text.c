#include <string.h>

#include "egui.h"
#include "widget/egui_view_roller.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_roller_get_selected_text.h"

static egui_view_roller_t s_roller;

static const char *s_items[] = {"Alpha", "Beta", "Gamma"};

static void setup(void)
{
    memset(&s_roller, 0, sizeof(s_roller));
    egui_view_roller_init(EGUI_VIEW_OF(&s_roller), uicode_get_core());
    egui_view_roller_set_items(EGUI_VIEW_OF(&s_roller), s_items, 3);
}

/* Default index 0 returns the first item text. */
static void test_roller_get_selected_text_default(void)
{
    setup();
    EGUI_TEST_ASSERT_TRUE(egui_view_roller_get_selected_text(EGUI_VIEW_OF(&s_roller)) == s_items[0]);
}

/* After setting index 2, returns the third item. */
static void test_roller_get_selected_text_after_set(void)
{
    setup();
    egui_view_roller_set_current_index(EGUI_VIEW_OF(&s_roller), 2);
    EGUI_TEST_ASSERT_TRUE(egui_view_roller_get_selected_text(EGUI_VIEW_OF(&s_roller)) == s_items[2]);
}

/* NULL items returns NULL. */
static void test_roller_get_selected_text_no_items(void)
{
    memset(&s_roller, 0, sizeof(s_roller));
    egui_view_roller_init(EGUI_VIEW_OF(&s_roller), uicode_get_core());
    EGUI_TEST_ASSERT_NULL(egui_view_roller_get_selected_text(EGUI_VIEW_OF(&s_roller)));
}

/* NULL self returns NULL without crash. */
static void test_roller_get_selected_text_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_roller_get_selected_text(NULL));
}

void test_roller_get_selected_text_run(void)
{
    EGUI_TEST_SUITE_BEGIN(roller_get_selected_text);

    EGUI_TEST_RUN(test_roller_get_selected_text_default);
    EGUI_TEST_RUN(test_roller_get_selected_text_after_set);
    EGUI_TEST_RUN(test_roller_get_selected_text_no_items);
    EGUI_TEST_RUN(test_roller_get_selected_text_null_self);

    EGUI_TEST_SUITE_END();
}
