#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_image_button_get_state.h"

static egui_view_image_button_t s_button;
static egui_image_t s_image;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_button, 0, sizeof(s_button));
    memset(&s_image, 0, sizeof(s_image));
    egui_view_image_button_init(EGUI_VIEW_OF(&s_button), core);
}

static void test_image_button_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_icon(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_text(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_font(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_icon_font(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_PRIMARY.full, (int)egui_view_image_button_get_content_color(EGUI_VIEW_OF(&s_button)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100, (int)egui_view_image_button_get_content_alpha(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_image_button_get_icon_text_gap(EGUI_VIEW_OF(&s_button)));
}

static void test_image_button_get_state_after_setters(void)
{
    egui_color_t color = {.full = 0x2345};
    const char *icon = "i";
    const char *text = "Play";

    setup();
    egui_view_image_button_set_icon(EGUI_VIEW_OF(&s_button), icon);
    egui_view_image_button_set_text(EGUI_VIEW_OF(&s_button), text);
    egui_view_image_button_set_font(EGUI_VIEW_OF(&s_button), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_image_button_set_icon_font(EGUI_VIEW_OF(&s_button), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_image_button_set_content_color(EGUI_VIEW_OF(&s_button), color, EGUI_ALPHA_60);
    egui_view_image_button_set_icon_text_gap(EGUI_VIEW_OF(&s_button), 7);

    EGUI_TEST_ASSERT_TRUE(egui_view_image_button_get_icon(EGUI_VIEW_OF(&s_button)) == icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_image_button_get_text(EGUI_VIEW_OF(&s_button)) == text);
    EGUI_TEST_ASSERT_TRUE(egui_view_image_button_get_font(EGUI_VIEW_OF(&s_button)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_image_button_get_icon_font(EGUI_VIEW_OF(&s_button)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)egui_view_image_button_get_content_color(EGUI_VIEW_OF(&s_button)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_60, (int)egui_view_image_button_get_content_alpha(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(7, (int)egui_view_image_button_get_icon_text_gap(EGUI_VIEW_OF(&s_button)));

    egui_view_image_button_set_icon(EGUI_VIEW_OF(&s_button), NULL);
    egui_view_image_button_set_text(EGUI_VIEW_OF(&s_button), NULL);
    egui_view_image_button_set_font(EGUI_VIEW_OF(&s_button), NULL);
    egui_view_image_button_set_icon_font(EGUI_VIEW_OF(&s_button), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_icon(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_text(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_font(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_icon_font(EGUI_VIEW_OF(&s_button)));
}

static void test_image_button_get_state_apply_image_params(void)
{
    const egui_view_image_params_t params = {
            .region = {{3, 4}, {40, 30}},
            .image = &s_image,
    };

    setup();
    egui_view_image_button_apply_params(EGUI_VIEW_OF(&s_button), &params);

    EGUI_TEST_ASSERT_TRUE(egui_view_image_get_image(EGUI_VIEW_OF(&s_button)) == &s_image);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(40, (int)egui_view_get_width(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_get_height(EGUI_VIEW_OF(&s_button)));
}

static void test_image_button_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_text(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_image_button_get_icon_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_image_button_get_content_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_image_button_get_content_alpha(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_image_button_get_icon_text_gap(NULL));
}

void test_image_button_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(image_button_get_state);

    EGUI_TEST_RUN(test_image_button_get_state_defaults);
    EGUI_TEST_RUN(test_image_button_get_state_after_setters);
    EGUI_TEST_RUN(test_image_button_get_state_apply_image_params);
    EGUI_TEST_RUN(test_image_button_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
