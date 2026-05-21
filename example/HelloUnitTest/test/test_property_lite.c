#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_property_lite.h"
#include "uicode_disp0.h"

#if EGUI_CONFIG_FUNCTION_PROPERTY_LITE

static void test_property_lite_base_properties(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_t view;
    egui_property_value_t value;

    egui_view_init(&view, core);

    value.type = EGUI_PROPERTY_TYPE_INT;
    value.data.i32 = 11;
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_set_property(&view, EGUI_PROPERTY_X, &value));
    value.data.i32 = 22;
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_set_property(&view, EGUI_PROPERTY_Y, &value));
    value.data.i32 = 33;
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_set_property(&view, EGUI_PROPERTY_WIDTH, &value));
    value.data.i32 = 44;
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_set_property(&view, EGUI_PROPERTY_HEIGHT, &value));

    EGUI_TEST_ASSERT_EQUAL_INT(11, egui_view_get_x(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(22, egui_view_get_y(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(33, egui_view_get_width(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(44, egui_view_get_height(&view));

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_property(&view, EGUI_PROPERTY_WIDTH, &value));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_PROPERTY_TYPE_INT, value.type);
    EGUI_TEST_ASSERT_EQUAL_INT(33, value.data.i32);
}

static void test_property_lite_label_text(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_label_t label;
    egui_property_value_t value;

    egui_view_label_init(EGUI_VIEW_OF(&label), core);
    value.type = EGUI_PROPERTY_TYPE_STRING;
    value.data.str = "Text";

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_set_property(EGUI_VIEW_OF(&label), EGUI_PROPERTY_TEXT, &value));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_property(EGUI_VIEW_OF(&label), EGUI_PROPERTY_TEXT, &value));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_PROPERTY_TYPE_STRING, value.type);
    EGUI_TEST_ASSERT_TRUE(strcmp(value.data.str, "Text") == 0);
}

static void test_property_lite_spacing_properties(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_t view;
    egui_property_value_t value;

    egui_view_init(&view, core);
    value.type = EGUI_PROPERTY_TYPE_INT;
    value.data.i32 = 9;
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_set_property(&view, EGUI_PROPERTY_PADDING_LEFT, &value));
    value.data.i32 = 10;
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_set_property(&view, EGUI_PROPERTY_MARGIN_BOTTOM, &value));

    EGUI_TEST_ASSERT_EQUAL_INT(9, egui_view_get_padding_left(&view));
    EGUI_TEST_ASSERT_EQUAL_INT(10, egui_view_get_margin_bottom(&view));

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_property(&view, EGUI_PROPERTY_PADDING_LEFT, &value));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_PROPERTY_TYPE_INT, value.type);
    EGUI_TEST_ASSERT_EQUAL_INT(9, value.data.i32);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_property(&view, EGUI_PROPERTY_MARGIN_BOTTOM, &value));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_PROPERTY_TYPE_INT, value.type);
    EGUI_TEST_ASSERT_EQUAL_INT(10, value.data.i32);
}

#endif /* EGUI_CONFIG_FUNCTION_PROPERTY_LITE */

void test_property_lite_run(void)
{
    EGUI_TEST_SUITE_BEGIN(property_lite);

#if EGUI_CONFIG_FUNCTION_PROPERTY_LITE
    EGUI_TEST_RUN(test_property_lite_base_properties);
    EGUI_TEST_RUN(test_property_lite_label_text);
    EGUI_TEST_RUN(test_property_lite_spacing_properties);
#endif

    EGUI_TEST_SUITE_END();
}
