#include "egui.h"
#include "background/egui_background_color.h"
#include "widget/egui_style.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_view_state_styles.h"

#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES && EGUI_CONFIG_FUNCTION_STYLE_CASCADE

/* ------------------------------------------------------------------ */
/* Backgrounds (two distinct sentinel values)                          */
/* ------------------------------------------------------------------ */

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_bg_normal_param,  EGUI_THEME_PRIMARY,   EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_bg_normal_bp, &s_bg_normal_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_bg_normal, &s_bg_normal_bp);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_bg_pressed_param, EGUI_THEME_SECONDARY, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_bg_pressed_bp, &s_bg_pressed_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_bg_pressed, &s_bg_pressed_bp);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_bg_checked_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_bg_checked_bp, &s_bg_checked_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_bg_checked, &s_bg_checked_bp);

/* ------------------------------------------------------------------ */
/* Styles                                                              */
/* ------------------------------------------------------------------ */

/* state_mask = 0 → always applies */
static const egui_view_style_t s_sty_default = EGUI_STYLE_INIT_BACKGROUND(EGUI_BG_OF(&s_bg_normal));

/* state_mask = PRESSED → only when pressed */
static const egui_view_style_t s_sty_pressed = {
    .background = EGUI_BG_OF(&s_bg_pressed),
    .has_alpha  = 0,
    .state_mask = EGUI_VIEW_STATE_PRESSED
};

/* state_mask = CHECKED → only when checked */
static const egui_view_style_t s_sty_checked = {
    .background = EGUI_BG_OF(&s_bg_checked),
    .has_alpha  = 0,
    .state_mask = EGUI_VIEW_STATE_CHECKED
};

/* ------------------------------------------------------------------ */
/* View fixture                                                        */
/* ------------------------------------------------------------------ */

static egui_view_label_t s_view;

static egui_view_t *get_view(void) { return EGUI_VIEW_OF(&s_view); }

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_label_init(get_view(), core);
    egui_view_set_size(get_view(), 80, 30);
}

/* ------------------------------------------------------------------ */
/* Tests                                                               */
/* ------------------------------------------------------------------ */

/* NULL-safe: get_computed_state(NULL) must return 0. */
static void test_null_safe(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_computed_state(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_state_checked(NULL));
    /* Must not crash: */
    egui_view_set_state_checked(NULL, 1);
}

/* A style with state_mask=0 (DEFAULT) is always applied. */
static void test_default_style_always_applied(void)
{
    setup();
    egui_view_add_style(get_view(), &s_sty_default);
    EGUI_TEST_ASSERT_TRUE(
        egui_view_get_effective_background(get_view()) == EGUI_BG_OF(&s_bg_normal));
}

/* A PRESSED-state style is NOT applied when the view is not pressed. */
static void test_pressed_style_not_applied_when_not_pressed(void)
{
    setup();
    egui_view_add_style(get_view(), &s_sty_default);
    egui_view_add_style(get_view(), &s_sty_pressed);
    /* is_pressed == 0 after init */
    EGUI_TEST_ASSERT_TRUE(
        egui_view_get_effective_background(get_view()) == EGUI_BG_OF(&s_bg_normal));
}

/* A PRESSED-state style IS applied when the view IS pressed. */
static void test_pressed_style_applied_when_pressed(void)
{
    setup();
    egui_view_add_style(get_view(), &s_sty_default);
    egui_view_add_style(get_view(), &s_sty_pressed);
    egui_view_set_pressed(get_view(), 1);
    EGUI_TEST_ASSERT_TRUE(
        egui_view_get_effective_background(get_view()) == EGUI_BG_OF(&s_bg_pressed));
}

/* After releasing, the default style re-applies. */
static void test_style_reverts_after_release(void)
{
    setup();
    egui_view_add_style(get_view(), &s_sty_default);
    egui_view_add_style(get_view(), &s_sty_pressed);
    egui_view_set_pressed(get_view(), 1);
    egui_view_set_pressed(get_view(), 0);
    EGUI_TEST_ASSERT_TRUE(
        egui_view_get_effective_background(get_view()) == EGUI_BG_OF(&s_bg_normal));
}

/* DISABLED state is set when is_enable == 0. */
static void test_disabled_state_from_is_enable(void)
{
    setup();
    egui_view_set_enable(get_view(), 0);
    uint8_t state = egui_view_get_computed_state(get_view());
    EGUI_TEST_ASSERT_TRUE((state & EGUI_VIEW_STATE_DISABLED) != 0);
    EGUI_TEST_ASSERT_TRUE((state & EGUI_VIEW_STATE_PRESSED)  == 0);
}

/* set_state_checked / get_state_checked round-trip. */
static void test_checked_set_get(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_state_checked(get_view()));
    egui_view_set_state_checked(get_view(), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_state_checked(get_view()));
    egui_view_set_state_checked(get_view(), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_state_checked(get_view()));
}

/* CHECKED bit appears in get_computed_state. */
static void test_checked_appears_in_computed_state(void)
{
    setup();
    egui_view_set_state_checked(get_view(), 1);
    uint8_t state = egui_view_get_computed_state(get_view());
    EGUI_TEST_ASSERT_TRUE((state & EGUI_VIEW_STATE_CHECKED) != 0);
}

/* A style matching CHECKED is applied only when checked. */
static void test_checked_style_applied_only_when_checked(void)
{
    setup();
    egui_view_add_style(get_view(), &s_sty_default);
    egui_view_add_style(get_view(), &s_sty_checked);

    /* Not checked → default background. */
    EGUI_TEST_ASSERT_TRUE(
        egui_view_get_effective_background(get_view()) == EGUI_BG_OF(&s_bg_normal));

    /* Checked → checked background. */
    egui_view_set_state_checked(get_view(), 1);
    EGUI_TEST_ASSERT_TRUE(
        egui_view_get_effective_background(get_view()) == EGUI_BG_OF(&s_bg_checked));
}

void test_view_state_styles_run(void)
{
    EGUI_TEST_RUN(test_null_safe);
    EGUI_TEST_RUN(test_default_style_always_applied);
    EGUI_TEST_RUN(test_pressed_style_not_applied_when_not_pressed);
    EGUI_TEST_RUN(test_pressed_style_applied_when_pressed);
    EGUI_TEST_RUN(test_style_reverts_after_release);
    EGUI_TEST_RUN(test_disabled_state_from_is_enable);
    EGUI_TEST_RUN(test_checked_set_get);
    EGUI_TEST_RUN(test_checked_appears_in_computed_state);
    EGUI_TEST_RUN(test_checked_style_applied_only_when_checked);
}

#else /* feature not enabled */
void test_view_state_styles_run(void) {}
#endif /* EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES && EGUI_CONFIG_FUNCTION_STYLE_CASCADE */
