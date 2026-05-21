#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_list_get_state.h"

static egui_view_list_t s_list;
static uint8_t s_clicked_index;

static void on_item_click(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    s_clicked_index = index;
}

static void setup(void)
{
    memset(&s_list, 0, sizeof(s_list));
    s_clicked_index = 0xFFu;
    egui_view_list_init(EGUI_VIEW_OF(&s_list), uicode_get_core());
}

static void test_list_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_list_get_item_count(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_list_get_item_height(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_LIST_SELECTED_NONE, (int)egui_view_list_get_selected_index(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(8, (int)egui_view_list_get_icon_text_gap(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_SECONDARY.full, (int)egui_view_list_get_icon_color(EGUI_VIEW_OF(&s_list)).full);
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_icon_font(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_on_item_click(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_item_text(EGUI_VIEW_OF(&s_list), 0));
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_item_icon(EGUI_VIEW_OF(&s_list), 0));
}

static void test_list_get_state_after_setters(void)
{
    egui_color_t icon_color = {.full = 0x1234};
    const char *text0 = "Alpha";
    const char *icon1 = "b";
    const char *text1 = "Beta";
    const char *icon2 = "g";

    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_list_add_item(EGUI_VIEW_OF(&s_list), text0));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_list_add_item_with_icon(EGUI_VIEW_OF(&s_list), icon1, text1));
    egui_view_list_set_item_height(EGUI_VIEW_OF(&s_list), 24);
    egui_view_list_set_item_icon(EGUI_VIEW_OF(&s_list), 0, icon2);
    egui_view_list_set_icon_font(EGUI_VIEW_OF(&s_list), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_list_set_icon_text_gap(EGUI_VIEW_OF(&s_list), 5);
    egui_view_list_set_icon_color(EGUI_VIEW_OF(&s_list), icon_color);
    egui_view_list_set_on_item_click(EGUI_VIEW_OF(&s_list), on_item_click);
    egui_view_list_set_selected_index(EGUI_VIEW_OF(&s_list), 1);

    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_list_get_item_count(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(24, (int)egui_view_list_get_item_height(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_list_get_selected_index(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_TRUE(egui_view_list_get_item_text(EGUI_VIEW_OF(&s_list), 0) == text0);
    EGUI_TEST_ASSERT_TRUE(egui_view_list_get_item_icon(EGUI_VIEW_OF(&s_list), 0) == icon2);
    EGUI_TEST_ASSERT_TRUE(egui_view_list_get_item_text(EGUI_VIEW_OF(&s_list), 1) == text1);
    EGUI_TEST_ASSERT_TRUE(egui_view_list_get_item_icon(EGUI_VIEW_OF(&s_list), 1) == icon1);
    EGUI_TEST_ASSERT_TRUE(egui_view_list_get_icon_font(EGUI_VIEW_OF(&s_list)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_list_get_icon_text_gap(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)icon_color.full, (int)egui_view_list_get_icon_color(EGUI_VIEW_OF(&s_list)).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_list_get_on_item_click(EGUI_VIEW_OF(&s_list)) == on_item_click);
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_item_text(EGUI_VIEW_OF(&s_list), 2));
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_item_icon(EGUI_VIEW_OF(&s_list), 2));
    EGUI_TEST_ASSERT_EQUAL_INT(0xFF, (int)s_clicked_index);
}

static void test_list_get_state_clear(void)
{
    setup();
    egui_view_list_add_item(EGUI_VIEW_OF(&s_list), "Alpha");
    egui_view_list_set_on_item_click(EGUI_VIEW_OF(&s_list), on_item_click);
    egui_view_list_clear(EGUI_VIEW_OF(&s_list));

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_list_get_item_count(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_LIST_SELECTED_NONE, (int)egui_view_list_get_selected_index(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_item_text(EGUI_VIEW_OF(&s_list), 0));
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_item_icon(EGUI_VIEW_OF(&s_list), 0));
    EGUI_TEST_ASSERT_TRUE(egui_view_list_get_on_item_click(EGUI_VIEW_OF(&s_list)) == on_item_click);

    egui_view_list_set_on_item_click(EGUI_VIEW_OF(&s_list), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_on_item_click(EGUI_VIEW_OF(&s_list)));
}

static void test_list_get_state_apply_params(void)
{
    static const egui_view_list_params_t params = {
            .region = {{3, 4}, {90, 60}},
            .item_height = 18,
    };

    setup();
    egui_view_list_apply_params(EGUI_VIEW_OF(&s_list), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_get_width(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, (int)egui_view_get_height(EGUI_VIEW_OF(&s_list)));
    EGUI_TEST_ASSERT_EQUAL_INT(18, (int)egui_view_list_get_item_height(EGUI_VIEW_OF(&s_list)));
}

static void test_list_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_list_get_item_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_list_get_item_height(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_LIST_SELECTED_NONE, (int)egui_view_list_get_selected_index(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_item_text(NULL, 0));
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_item_icon(NULL, 0));
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_icon_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_list_get_icon_text_gap(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_list_get_icon_color(NULL).full);
    EGUI_TEST_ASSERT_NULL(egui_view_list_get_on_item_click(NULL));
}

void test_list_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(list_get_state);

    EGUI_TEST_RUN(test_list_get_state_defaults);
    EGUI_TEST_RUN(test_list_get_state_after_setters);
    EGUI_TEST_RUN(test_list_get_state_clear);
    EGUI_TEST_RUN(test_list_get_state_apply_params);
    EGUI_TEST_RUN(test_list_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
