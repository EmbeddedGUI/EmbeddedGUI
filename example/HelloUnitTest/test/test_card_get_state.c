#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_card_get_state.h"

static egui_view_card_t s_card;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_card, 0, sizeof(s_card));
    egui_view_card_init(EGUI_VIEW_OF(&s_card), core);
}

static void test_card_get_state_defaults(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_RADIUS_LG, (int)egui_view_card_get_corner_radius(EGUI_VIEW_OF(&s_card)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_STROKE_WIDTH, (int)egui_view_card_get_border_width(EGUI_VIEW_OF(&s_card)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_BORDER.full, (int)egui_view_card_get_border_color(EGUI_VIEW_OF(&s_card)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_SURFACE.full, (int)egui_view_card_get_bg_color(EGUI_VIEW_OF(&s_card)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100, (int)egui_view_card_get_bg_alpha(EGUI_VIEW_OF(&s_card)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_card_get_bg_color_custom(EGUI_VIEW_OF(&s_card)));
}

static void test_card_get_state_after_setters(void)
{
    egui_color_t border = {.full = 0x2345};
    egui_color_t bg = {.full = 0x4567};

    setup();
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&s_card), 12);
    egui_view_card_set_border(EGUI_VIEW_OF(&s_card), 3, border);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&s_card), bg, EGUI_ALPHA_80);

    EGUI_TEST_ASSERT_EQUAL_INT(12, (int)egui_view_card_get_corner_radius(EGUI_VIEW_OF(&s_card)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_card_get_border_width(EGUI_VIEW_OF(&s_card)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)border.full, (int)egui_view_card_get_border_color(EGUI_VIEW_OF(&s_card)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)bg.full, (int)egui_view_card_get_bg_color(EGUI_VIEW_OF(&s_card)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_80, (int)egui_view_card_get_bg_alpha(EGUI_VIEW_OF(&s_card)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_card_get_bg_color_custom(EGUI_VIEW_OF(&s_card)));
}

static void test_card_get_state_apply_params_updates_radius(void)
{
    static const egui_view_card_params_t params = {
            .region = {{1, 2}, {30, 40}},
            .corner_radius = 7,
    };

    setup();
    egui_view_card_apply_params(EGUI_VIEW_OF(&s_card), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(7, (int)egui_view_card_get_corner_radius(EGUI_VIEW_OF(&s_card)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_get_x(EGUI_VIEW_OF(&s_card)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_get_y(EGUI_VIEW_OF(&s_card)));
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_get_width(EGUI_VIEW_OF(&s_card)));
    EGUI_TEST_ASSERT_EQUAL_INT(40, (int)egui_view_get_height(EGUI_VIEW_OF(&s_card)));
}

static void test_card_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_card_get_corner_radius(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_card_get_border_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_card_get_border_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_card_get_bg_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_card_get_bg_alpha(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_card_get_bg_color_custom(NULL));
}

void test_card_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(card_get_state);

    EGUI_TEST_RUN(test_card_get_state_defaults);
    EGUI_TEST_RUN(test_card_get_state_after_setters);
    EGUI_TEST_RUN(test_card_get_state_apply_params_updates_radius);
    EGUI_TEST_RUN(test_card_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
