#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_button_matrix_get_state.h"

static egui_view_button_matrix_t s_matrix;
static const char *s_labels[] = {"One", "Two", "Three", "Four"};
static const char *s_many_labels[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16"};
static const char *s_icons[] = {"a", "b", "c", "d"};
static uint8_t s_click_index;

static void on_matrix_click(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    s_click_index = index;
}

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_matrix, 0, sizeof(s_matrix));
    s_click_index = 0xFFu;
    egui_view_button_matrix_init(EGUI_VIEW_OF(&s_matrix), core);
}

static void test_button_matrix_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_labels(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_icons(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_button_count(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_button_matrix_get_cols(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_button_matrix_get_gap(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_selection_enabled(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE, (int)egui_view_button_matrix_get_selected_index(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_DARK_GREY.full, (int)egui_view_button_matrix_get_btn_color(EGUI_VIEW_OF(&s_matrix)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_LIGHT_GREY.full, (int)egui_view_button_matrix_get_btn_pressed_color(EGUI_VIEW_OF(&s_matrix)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)egui_view_button_matrix_get_text_color(EGUI_VIEW_OF(&s_matrix)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)egui_view_button_matrix_get_border_color(EGUI_VIEW_OF(&s_matrix)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_button_matrix_get_corner_radius(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_TRUE(egui_view_button_matrix_get_font(EGUI_VIEW_OF(&s_matrix)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_icon_font(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_button_matrix_get_icon_text_gap(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_on_click(EGUI_VIEW_OF(&s_matrix)));
}

static void test_button_matrix_get_state_after_setters(void)
{
    egui_color_t btn_color = {.full = 0x1234};
    egui_color_t pressed_color = {.full = 0x2345};
    egui_color_t text_color = {.full = 0x3456};
    egui_color_t border_color = {.full = 0x4567};

    setup();
    egui_view_button_matrix_set_labels(EGUI_VIEW_OF(&s_matrix), s_labels, EGUI_ARRAY_SIZE(s_labels), 2);
    egui_view_button_matrix_set_icons(EGUI_VIEW_OF(&s_matrix), s_icons);
    egui_view_button_matrix_set_selection_enabled(EGUI_VIEW_OF(&s_matrix), 1);
    egui_view_button_matrix_set_selected_index(EGUI_VIEW_OF(&s_matrix), 2);
    egui_view_button_matrix_set_btn_color(EGUI_VIEW_OF(&s_matrix), btn_color);
    egui_view_button_matrix_set_btn_pressed_color(EGUI_VIEW_OF(&s_matrix), pressed_color);
    egui_view_button_matrix_set_text_color(EGUI_VIEW_OF(&s_matrix), text_color);
    egui_view_button_matrix_set_border_color(EGUI_VIEW_OF(&s_matrix), border_color);
    egui_view_button_matrix_set_gap(EGUI_VIEW_OF(&s_matrix), 5);
    egui_view_button_matrix_set_corner_radius(EGUI_VIEW_OF(&s_matrix), 6);
    egui_view_button_matrix_set_font(EGUI_VIEW_OF(&s_matrix), NULL);
    egui_view_button_matrix_set_icon_font(EGUI_VIEW_OF(&s_matrix), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_button_matrix_set_icon_text_gap(EGUI_VIEW_OF(&s_matrix), 7);
    egui_view_button_matrix_set_on_click(EGUI_VIEW_OF(&s_matrix), on_matrix_click);

    EGUI_TEST_ASSERT_TRUE(egui_view_button_matrix_get_labels(EGUI_VIEW_OF(&s_matrix)) == s_labels);
    EGUI_TEST_ASSERT_TRUE(egui_view_button_matrix_get_icons(EGUI_VIEW_OF(&s_matrix)) == s_icons);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ARRAY_SIZE(s_labels), (int)egui_view_button_matrix_get_button_count(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_button_matrix_get_cols(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_button_matrix_get_selection_enabled(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_button_matrix_get_selected_index(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)btn_color.full, (int)egui_view_button_matrix_get_btn_color(EGUI_VIEW_OF(&s_matrix)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)pressed_color.full, (int)egui_view_button_matrix_get_btn_pressed_color(EGUI_VIEW_OF(&s_matrix)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)text_color.full, (int)egui_view_button_matrix_get_text_color(EGUI_VIEW_OF(&s_matrix)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)border_color.full, (int)egui_view_button_matrix_get_border_color(EGUI_VIEW_OF(&s_matrix)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_button_matrix_get_gap(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_button_matrix_get_corner_radius(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_font(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_TRUE(egui_view_button_matrix_get_icon_font(EGUI_VIEW_OF(&s_matrix)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT(7, (int)egui_view_button_matrix_get_icon_text_gap(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_TRUE(egui_view_button_matrix_get_on_click(EGUI_VIEW_OF(&s_matrix)) == on_matrix_click);

    egui_view_button_matrix_set_icons(EGUI_VIEW_OF(&s_matrix), NULL);
    egui_view_button_matrix_set_icon_font(EGUI_VIEW_OF(&s_matrix), NULL);
    egui_view_button_matrix_set_on_click(EGUI_VIEW_OF(&s_matrix), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_icons(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_icon_font(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_on_click(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(0xFF, (int)s_click_index);
}

static void test_button_matrix_get_state_selection_and_clamp(void)
{
    setup();
    egui_view_button_matrix_set_labels(EGUI_VIEW_OF(&s_matrix), s_labels, EGUI_ARRAY_SIZE(s_labels), 2);
    egui_view_button_matrix_set_selected_index(EGUI_VIEW_OF(&s_matrix), 99);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_button_matrix_get_selected_index(EGUI_VIEW_OF(&s_matrix)));

    egui_view_button_matrix_set_selection_enabled(EGUI_VIEW_OF(&s_matrix), 1);
    egui_view_button_matrix_set_selection_enabled(EGUI_VIEW_OF(&s_matrix), 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_selection_enabled(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE, (int)egui_view_button_matrix_get_selected_index(EGUI_VIEW_OF(&s_matrix)));

    egui_view_button_matrix_set_labels(EGUI_VIEW_OF(&s_matrix), s_many_labels, EGUI_ARRAY_SIZE(s_many_labels), 3);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BUTTON_MATRIX_MAX_BUTTONS, (int)egui_view_button_matrix_get_button_count(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_button_matrix_get_cols(EGUI_VIEW_OF(&s_matrix)));
}

static void test_button_matrix_get_state_apply_params(void)
{
    static const egui_view_button_matrix_params_t params = {
            .region = {{1, 2}, {90, 60}},
            .cols = 3,
            .gap = 4,
    };

    setup();
    egui_view_button_matrix_apply_params(EGUI_VIEW_OF(&s_matrix), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_button_matrix_get_cols(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_button_matrix_get_gap(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_get_x(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_get_y(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_get_width(EGUI_VIEW_OF(&s_matrix)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, (int)egui_view_get_height(EGUI_VIEW_OF(&s_matrix)));
}

static void test_button_matrix_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_labels(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_icons(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_button_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_cols(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_gap(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_selection_enabled(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE, (int)egui_view_button_matrix_get_selected_index(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_btn_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_btn_pressed_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_text_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_border_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_corner_radius(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_icon_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_matrix_get_icon_text_gap(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_button_matrix_get_on_click(NULL));
}

void test_button_matrix_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(button_matrix_get_state);

    EGUI_TEST_RUN(test_button_matrix_get_state_defaults);
    EGUI_TEST_RUN(test_button_matrix_get_state_after_setters);
    EGUI_TEST_RUN(test_button_matrix_get_state_selection_and_clamp);
    EGUI_TEST_RUN(test_button_matrix_get_state_apply_params);
    EGUI_TEST_RUN(test_button_matrix_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
