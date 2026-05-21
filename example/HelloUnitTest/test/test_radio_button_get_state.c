#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_radio_button_get_state.h"

static egui_view_radio_button_t s_radio_a;
static egui_view_radio_button_t s_radio_b;
static egui_view_radio_group_t s_group;
static const char s_text[] = "Radio";
static const char s_mark_icon[] = "x";
static int s_changed_index;

static void on_group_changed(egui_view_t *self, int index)
{
    EGUI_UNUSED(self);
    s_changed_index = index;
}

static void setup_radio(egui_view_radio_button_t *radio)
{
    memset(radio, 0, sizeof(*radio));
    egui_view_radio_button_init(EGUI_VIEW_OF(radio), uicode_get_core());
}

static void setup_group(void)
{
    setup_radio(&s_radio_a);
    setup_radio(&s_radio_b);
    memset(&s_group, 0, sizeof(s_group));
    egui_view_radio_group_init(&s_group);
    s_changed_index = -1;
}

static void test_radio_button_get_state_defaults(void)
{
    setup_radio(&s_radio_a);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_radio_button_get_checked(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_group(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_text(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_font(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT.full, (int)egui_view_radio_button_get_text_color(EGUI_VIEW_OF(&s_radio_a)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_DOT, (int)egui_view_radio_button_get_mark_style(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_NOT_NULL(egui_view_radio_button_get_mark_icon(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_icon_font(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_radio_button_get_icon_text_gap(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_group_get_on_changed_listener(NULL));
}

static void test_radio_button_get_state_after_setters(void)
{
    egui_color_t text_color = {.full = 0x2345};

    setup_group();
    egui_view_radio_button_set_text(EGUI_VIEW_OF(&s_radio_a), s_text);
    egui_view_radio_button_set_font(EGUI_VIEW_OF(&s_radio_a), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_radio_button_set_text_color(EGUI_VIEW_OF(&s_radio_a), text_color);
    egui_view_radio_button_set_mark_style(EGUI_VIEW_OF(&s_radio_a), EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_ICON);
    egui_view_radio_button_set_mark_icon(EGUI_VIEW_OF(&s_radio_a), s_mark_icon);
    egui_view_radio_button_set_icon_font(EGUI_VIEW_OF(&s_radio_a), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_radio_button_set_icon_text_gap(EGUI_VIEW_OF(&s_radio_a), 9);
    egui_view_radio_group_add(&s_group, EGUI_VIEW_OF(&s_radio_a));
    egui_view_radio_group_set_on_changed_listener(&s_group, on_group_changed);

    EGUI_TEST_ASSERT_TRUE(egui_view_radio_button_get_text(EGUI_VIEW_OF(&s_radio_a)) == s_text);
    EGUI_TEST_ASSERT_TRUE(egui_view_radio_button_get_font(EGUI_VIEW_OF(&s_radio_a)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT((int)text_color.full, (int)egui_view_radio_button_get_text_color(EGUI_VIEW_OF(&s_radio_a)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_ICON, (int)egui_view_radio_button_get_mark_style(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_TRUE(egui_view_radio_button_get_mark_icon(EGUI_VIEW_OF(&s_radio_a)) == s_mark_icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_radio_button_get_icon_font(EGUI_VIEW_OF(&s_radio_a)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT(9, (int)egui_view_radio_button_get_icon_text_gap(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_TRUE(egui_view_radio_button_get_group(EGUI_VIEW_OF(&s_radio_a)) == &s_group);
    EGUI_TEST_ASSERT_TRUE(egui_view_radio_group_get_on_changed_listener(&s_group) == on_group_changed);
}

static void test_radio_button_get_state_clear(void)
{
    setup_radio(&s_radio_a);
    egui_view_radio_button_set_text(EGUI_VIEW_OF(&s_radio_a), s_text);
    egui_view_radio_button_set_font(EGUI_VIEW_OF(&s_radio_a), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_radio_button_set_mark_icon(EGUI_VIEW_OF(&s_radio_a), s_mark_icon);
    egui_view_radio_button_set_icon_font(EGUI_VIEW_OF(&s_radio_a), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_radio_button_set_text(EGUI_VIEW_OF(&s_radio_a), NULL);
    egui_view_radio_button_set_font(EGUI_VIEW_OF(&s_radio_a), NULL);
    egui_view_radio_button_set_mark_icon(EGUI_VIEW_OF(&s_radio_a), NULL);
    egui_view_radio_button_set_icon_font(EGUI_VIEW_OF(&s_radio_a), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_text(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_font(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_mark_icon(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_icon_font(EGUI_VIEW_OF(&s_radio_a)));
}

static void test_radio_button_get_state_group_selection(void)
{
    setup_group();
    egui_view_radio_group_add(&s_group, EGUI_VIEW_OF(&s_radio_a));
    egui_view_radio_group_add(&s_group, EGUI_VIEW_OF(&s_radio_b));
    egui_view_radio_group_set_on_changed_listener(&s_group, on_group_changed);

    egui_view_radio_button_set_checked(EGUI_VIEW_OF(&s_radio_a), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_radio_button_get_checked(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_radio_button_get_checked(EGUI_VIEW_OF(&s_radio_b)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_changed_index);

    egui_view_radio_button_set_checked(EGUI_VIEW_OF(&s_radio_b), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_radio_button_get_checked(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_radio_button_get_checked(EGUI_VIEW_OF(&s_radio_b)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_changed_index);
}

static void test_radio_button_get_state_apply_params(void)
{
    static const egui_view_radio_button_params_t params = {
            .region = {{3, 4}, {50, 20}},
            .is_checked = 1,
            .text = s_text,
    };

    setup_radio(&s_radio_a);
    egui_view_radio_button_apply_params(EGUI_VIEW_OF(&s_radio_a), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_get_width(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_EQUAL_INT(20, (int)egui_view_get_height(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_radio_button_get_checked(EGUI_VIEW_OF(&s_radio_a)));
    EGUI_TEST_ASSERT_TRUE(egui_view_radio_button_get_text(EGUI_VIEW_OF(&s_radio_a)) == s_text);
}

static void test_radio_button_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_radio_button_get_checked(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_group(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_text(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_radio_button_get_text_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_DOT, (int)egui_view_radio_button_get_mark_style(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_mark_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_radio_button_get_icon_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_radio_button_get_icon_text_gap(NULL));
}

void test_radio_button_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(radio_button_get_state);

    EGUI_TEST_RUN(test_radio_button_get_state_defaults);
    EGUI_TEST_RUN(test_radio_button_get_state_after_setters);
    EGUI_TEST_RUN(test_radio_button_get_state_clear);
    EGUI_TEST_RUN(test_radio_button_get_state_group_selection);
    EGUI_TEST_RUN(test_radio_button_get_state_apply_params);
    EGUI_TEST_RUN(test_radio_button_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
