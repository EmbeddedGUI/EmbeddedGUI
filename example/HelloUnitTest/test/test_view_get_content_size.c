#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_view_get_content_size.h"

static egui_view_t s_view;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_view, 0, sizeof(s_view));
    egui_view_init(&s_view, core);
}

/* No padding: content size equals full size. */
static void test_view_content_size_no_padding(void)
{
    setup();
    egui_view_set_size(&s_view, 100, 80);
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_get_content_width(&s_view));
    EGUI_TEST_ASSERT_EQUAL_INT(80,  (int)egui_view_get_content_height(&s_view));
}

/* With padding: content size is reduced by pad amounts. */
static void test_view_content_size_with_padding(void)
{
    setup();
    egui_view_set_size(&s_view, 100, 80);
    egui_view_set_padding(&s_view, 5, 10, 5, 10);
    EGUI_TEST_ASSERT_EQUAL_INT(85, (int)egui_view_get_content_width(&s_view));
    EGUI_TEST_ASSERT_EQUAL_INT(65, (int)egui_view_get_content_height(&s_view));
}

/* Padding larger than size clamps to 0. */
static void test_view_content_size_oversized_padding(void)
{
    setup();
    egui_view_set_size(&s_view, 10, 10);
    egui_view_set_padding(&s_view, 8, 8, 8, 8);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_content_width(&s_view));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_content_height(&s_view));
}

/* NULL self returns 0 without crash. */
static void test_view_content_size_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_content_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_content_height(NULL));
}

void test_view_get_content_size_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_get_content_size);

    EGUI_TEST_RUN(test_view_content_size_no_padding);
    EGUI_TEST_RUN(test_view_content_size_with_padding);
    EGUI_TEST_RUN(test_view_content_size_oversized_padding);
    EGUI_TEST_RUN(test_view_content_size_null_self);

    EGUI_TEST_SUITE_END();
}
