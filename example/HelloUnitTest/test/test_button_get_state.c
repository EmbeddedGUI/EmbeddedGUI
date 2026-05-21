#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_button_get_state.h"

static egui_view_button_t s_button;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_button, 0, sizeof(s_button));
    egui_view_button_init(EGUI_VIEW_OF(&s_button), core);
}

static void test_button_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_button_get_icon(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_NULL(egui_view_button_get_icon_font(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_button_get_icon_text_gap(EGUI_VIEW_OF(&s_button)));
}

static void test_button_get_state_after_setters(void)
{
    const char *icon = "i";

    setup();
    egui_view_button_set_icon(EGUI_VIEW_OF(&s_button), icon);
    egui_view_button_set_icon_font(EGUI_VIEW_OF(&s_button), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_button_set_icon_text_gap(EGUI_VIEW_OF(&s_button), 9);

    EGUI_TEST_ASSERT_TRUE(egui_view_button_get_icon(EGUI_VIEW_OF(&s_button)) == icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_button_get_icon_font(EGUI_VIEW_OF(&s_button)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT(9, (int)egui_view_button_get_icon_text_gap(EGUI_VIEW_OF(&s_button)));

    egui_view_button_set_icon(EGUI_VIEW_OF(&s_button), NULL);
    egui_view_button_set_icon_font(EGUI_VIEW_OF(&s_button), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_button_get_icon(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_NULL(egui_view_button_get_icon_font(EGUI_VIEW_OF(&s_button)));
}

static void test_button_get_state_apply_label_params(void)
{
    static const egui_view_label_params_t params = {
            .region = {{1, 2}, {50, 20}},
            .align_type = EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
            .text = "Run",
            .font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT,
            .color = EGUI_COLOR_WHITE_INIT,
            .alpha = EGUI_ALPHA_80,
    };

    setup();
    egui_view_button_apply_params(EGUI_VIEW_OF(&s_button), &params);

    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_text(EGUI_VIEW_OF(&s_button)) == params.text);
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_font(EGUI_VIEW_OF(&s_button)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT((int)params.align_type, (int)egui_view_label_get_align_type(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)params.color.full, (int)egui_view_label_get_font_color(EGUI_VIEW_OF(&s_button)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)params.alpha, (int)egui_view_label_get_alpha(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_get_x(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_get_y(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_get_width(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(20, (int)egui_view_get_height(EGUI_VIEW_OF(&s_button)));
}

static void test_button_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_button_get_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_button_get_icon_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_button_get_icon_text_gap(NULL));
}

void test_button_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(button_get_state);

    EGUI_TEST_RUN(test_button_get_state_defaults);
    EGUI_TEST_RUN(test_button_get_state_after_setters);
    EGUI_TEST_RUN(test_button_get_state_apply_label_params);
    EGUI_TEST_RUN(test_button_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
