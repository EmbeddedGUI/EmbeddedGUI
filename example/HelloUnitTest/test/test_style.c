#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "background/egui_background_color.h"
#include "test_style.h"

#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE

#include "widget/egui_style.h"

/* ------------------------------------------------------------------ */
/* File-scope background objects (static const ROM data)               */
/* ------------------------------------------------------------------ */

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_tc_bgA_param, EGUI_THEME_PRIMARY, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_tc_bgA_bp, &s_tc_bgA_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_tc_bgA, &s_tc_bgA_bp);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_tc_bgB_param, EGUI_THEME_SECONDARY, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_tc_bgB_bp, &s_tc_bgB_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_tc_bgB, &s_tc_bgB_bp);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_tc_bgC_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_tc_bgC_bp, &s_tc_bgC_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_tc_bgC, &s_tc_bgC_bp);

/* ------------------------------------------------------------------ */
/* File-scope style objects                                             */
/* ------------------------------------------------------------------ */

static const egui_view_style_t s_sty_A = EGUI_STYLE_INIT_BACKGROUND(EGUI_BG_OF(&s_tc_bgA));
static const egui_view_style_t s_sty_B = EGUI_STYLE_INIT_BACKGROUND(EGUI_BG_OF(&s_tc_bgB));
static const egui_view_style_t s_sty_alpha50 = EGUI_STYLE_INIT_ALPHA(EGUI_ALPHA_50);
static const egui_view_style_t s_sty_alpha20 = EGUI_STYLE_INIT_ALPHA(EGUI_ALPHA_20);

/* Extra distinct style pointers for the capacity overflow test. */
static const egui_view_style_t s_sty_cap0 = EGUI_STYLE_INIT_BACKGROUND(EGUI_BG_OF(&s_tc_bgA));
static const egui_view_style_t s_sty_cap1 = EGUI_STYLE_INIT_BACKGROUND(EGUI_BG_OF(&s_tc_bgA));
static const egui_view_style_t s_sty_cap2 = EGUI_STYLE_INIT_BACKGROUND(EGUI_BG_OF(&s_tc_bgA));
static const egui_view_style_t s_sty_cap3 = EGUI_STYLE_INIT_BACKGROUND(EGUI_BG_OF(&s_tc_bgA));
static const egui_view_style_t s_sty_cap4 = EGUI_STYLE_INIT_BACKGROUND(EGUI_BG_OF(&s_tc_bgA));

/* ------------------------------------------------------------------ */
/* View fixture                                                         */
/* ------------------------------------------------------------------ */

static egui_view_t test_view;

static egui_core_t *test_style_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

/* ------------------------------------------------------------------ */
/* Tests                                                               */
/* ------------------------------------------------------------------ */

static void test_style_view_defaults(void)
{
    egui_view_init(&test_view, test_style_get_core());

    EGUI_TEST_ASSERT_EQUAL_INT(0, test_view.style_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)(test_view.background != NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)test_view.has_own_alpha);
}

static void test_style_add_single(void)
{
    egui_view_init(&test_view, test_style_get_core());

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_add_style(&test_view, &s_sty_A));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_view.style_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)(test_view.styles[0] == &s_sty_A));
}

static void test_style_remove_existing(void)
{
    egui_view_init(&test_view, test_style_get_core());
    egui_view_add_style(&test_view, &s_sty_A);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_remove_style(&test_view, &s_sty_A));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_view.style_count);
}

static void test_style_remove_missing(void)
{
    egui_view_init(&test_view, test_style_get_core());
    egui_view_add_style(&test_view, &s_sty_A);

    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_remove_style(&test_view, &s_sty_B));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_view.style_count);
}

static void test_style_clear(void)
{
    egui_view_init(&test_view, test_style_get_core());
    egui_view_add_style(&test_view, &s_sty_A);
    egui_view_add_style(&test_view, &s_sty_B);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_view.style_count);

    egui_view_clear_styles(&test_view);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_view.style_count);
}

static void test_style_cascade_background_priority(void)
{
    egui_view_init(&test_view, test_style_get_core());
    egui_view_add_style(&test_view, &s_sty_A); /* lower priority */
    egui_view_add_style(&test_view, &s_sty_B); /* higher priority */

    const egui_background_t *bg = egui_view_get_effective_background(&test_view);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)(bg == EGUI_BG_OF(&s_tc_bgB)));
}

static void test_style_inline_background_wins(void)
{
    egui_view_init(&test_view, test_style_get_core());
    egui_view_set_background(&test_view, EGUI_BG_OF(&s_tc_bgC));
    egui_view_add_style(&test_view, &s_sty_A);
    egui_view_add_style(&test_view, &s_sty_B);

    const egui_background_t *bg = egui_view_get_effective_background(&test_view);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)(bg == EGUI_BG_OF(&s_tc_bgC)));
}

static void test_style_alpha_cascade(void)
{
    egui_view_init(&test_view, test_style_get_core());
    egui_view_add_style(&test_view, &s_sty_alpha50); /* lower priority */
    egui_view_add_style(&test_view, &s_sty_alpha20); /* higher priority */

    egui_alpha_t alpha = egui_view_get_effective_alpha(&test_view);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_20, (int)alpha);
}

static void test_style_own_alpha_wins(void)
{
    egui_view_init(&test_view, test_style_get_core());
    egui_view_add_style(&test_view, &s_sty_alpha50);
    egui_view_set_alpha(&test_view, EGUI_ALPHA_100);

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)test_view.has_own_alpha);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100, (int)egui_view_get_effective_alpha(&test_view));
}

static void test_style_max_capacity(void)
{
    const egui_view_style_t *caps[] = {&s_sty_cap0, &s_sty_cap1, &s_sty_cap2, &s_sty_cap3, &s_sty_cap4};
    int i;

    egui_view_init(&test_view, test_style_get_core());
    for (i = 0; i < EGUI_CONFIG_STYLE_MAX_PER_VIEW; i++)
    {
        EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_add_style(&test_view, caps[i]));
    }
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_add_style(&test_view, caps[EGUI_CONFIG_STYLE_MAX_PER_VIEW]));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_CONFIG_STYLE_MAX_PER_VIEW, test_view.style_count);
}

static void test_style_no_background_returns_null(void)
{
    egui_view_init(&test_view, test_style_get_core());
    egui_view_add_style(&test_view, &s_sty_alpha50);

    const egui_background_t *bg = egui_view_get_effective_background(&test_view);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)(bg != NULL));
}

/* ------------------------------------------------------------------ */
/* Runner                                                              */
/* ------------------------------------------------------------------ */

void test_style_run(void)
{
    EGUI_TEST_SUITE_BEGIN(style);

    EGUI_TEST_CASE(test_style_view_defaults);
    EGUI_TEST_CASE(test_style_add_single);
    EGUI_TEST_CASE(test_style_remove_existing);
    EGUI_TEST_CASE(test_style_remove_missing);
    EGUI_TEST_CASE(test_style_clear);
    EGUI_TEST_CASE(test_style_cascade_background_priority);
    EGUI_TEST_CASE(test_style_inline_background_wins);
    EGUI_TEST_CASE(test_style_alpha_cascade);
    EGUI_TEST_CASE(test_style_own_alpha_wins);
    EGUI_TEST_CASE(test_style_max_capacity);
    EGUI_TEST_CASE(test_style_no_background_returns_null);

    EGUI_TEST_SUITE_END();
}

#else /* EGUI_CONFIG_FUNCTION_STYLE_CASCADE */

void test_style_run(void)
{
    /* Style cascade disabled; nothing to run. */
}

#endif /* EGUI_CONFIG_FUNCTION_STYLE_CASCADE */
