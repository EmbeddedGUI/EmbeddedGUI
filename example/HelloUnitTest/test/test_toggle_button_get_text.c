#include <string.h>

#include "egui.h"
#include "widget/egui_view_toggle_button.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_toggle_button_get_text.h"

static egui_view_toggle_button_t s_btn;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_btn, 0, sizeof(s_btn));
    egui_view_toggle_button_init(EGUI_VIEW_OF(&s_btn), core);
}

/* Default text after init is NULL. */
static void test_toggle_button_get_text_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&s_btn)));
    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_icon(EGUI_VIEW_OF(&s_btn)));
    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_text(EGUI_VIEW_OF(&s_btn)));
    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_font(EGUI_VIEW_OF(&s_btn)));
    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_icon_font(EGUI_VIEW_OF(&s_btn)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_toggle_button_get_icon_text_gap(EGUI_VIEW_OF(&s_btn)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT.full, (int)egui_view_toggle_button_get_text_color(EGUI_VIEW_OF(&s_btn)).full);
}

/* After set_text, get_text returns the same pointer. */
static void test_toggle_button_get_text_after_set(void)
{
    setup();
    egui_view_toggle_button_set_text(EGUI_VIEW_OF(&s_btn), "hello");
    EGUI_TEST_ASSERT_NOT_NULL(egui_view_toggle_button_get_text(EGUI_VIEW_OF(&s_btn)));
}

/* Text can be updated; get_text follows the last set. */
static void test_toggle_button_get_text_update(void)
{
    const char *t1 = "first";
    const char *t2 = "second";
    setup();
    egui_view_toggle_button_set_text(EGUI_VIEW_OF(&s_btn), t1);
    egui_view_toggle_button_set_text(EGUI_VIEW_OF(&s_btn), t2);
    EGUI_TEST_ASSERT_TRUE(egui_view_toggle_button_get_text(EGUI_VIEW_OF(&s_btn)) == t2);
}

static void test_toggle_button_get_content_state_after_setters(void)
{
    egui_color_t text_color = {.full = 0x2468};
    const char *icon = "i";

    setup();
    egui_view_toggle_button_set_toggled(EGUI_VIEW_OF(&s_btn), 1);
    egui_view_toggle_button_set_icon(EGUI_VIEW_OF(&s_btn), icon);
    egui_view_toggle_button_set_font(EGUI_VIEW_OF(&s_btn), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&s_btn), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_toggle_button_set_icon_text_gap(EGUI_VIEW_OF(&s_btn), 9);
    egui_view_toggle_button_set_text_color(EGUI_VIEW_OF(&s_btn), text_color);

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&s_btn)));
    EGUI_TEST_ASSERT_TRUE(egui_view_toggle_button_get_icon(EGUI_VIEW_OF(&s_btn)) == icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_toggle_button_get_font(EGUI_VIEW_OF(&s_btn)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_toggle_button_get_icon_font(EGUI_VIEW_OF(&s_btn)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT(9, (int)egui_view_toggle_button_get_icon_text_gap(EGUI_VIEW_OF(&s_btn)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)text_color.full, (int)egui_view_toggle_button_get_text_color(EGUI_VIEW_OF(&s_btn)).full);
}

static void test_toggle_button_get_content_state_clear_optional_pointers(void)
{
    setup();
    egui_view_toggle_button_set_icon(EGUI_VIEW_OF(&s_btn), "i");
    egui_view_toggle_button_set_font(EGUI_VIEW_OF(&s_btn), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&s_btn), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_toggle_button_set_icon(EGUI_VIEW_OF(&s_btn), NULL);
    egui_view_toggle_button_set_font(EGUI_VIEW_OF(&s_btn), NULL);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&s_btn), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_icon(EGUI_VIEW_OF(&s_btn)));
    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_font(EGUI_VIEW_OF(&s_btn)));
    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_icon_font(EGUI_VIEW_OF(&s_btn)));
}

/* NULL self returns NULL without crash. */
static void test_toggle_button_get_text_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_toggle_button_is_toggled(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_text(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_icon_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_toggle_button_get_icon_text_gap(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_toggle_button_get_text_color(NULL).full);
}

void test_toggle_button_get_text_run(void)
{
    EGUI_TEST_SUITE_BEGIN(toggle_button_get_text);

    EGUI_TEST_RUN(test_toggle_button_get_text_default);
    EGUI_TEST_RUN(test_toggle_button_get_text_after_set);
    EGUI_TEST_RUN(test_toggle_button_get_text_update);
    EGUI_TEST_RUN(test_toggle_button_get_content_state_after_setters);
    EGUI_TEST_RUN(test_toggle_button_get_content_state_clear_optional_pointers);
    EGUI_TEST_RUN(test_toggle_button_get_text_null_self);

    EGUI_TEST_SUITE_END();
}
