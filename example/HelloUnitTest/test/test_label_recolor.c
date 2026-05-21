#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_recolor.h"

#if EGUI_CONFIG_FUNCTION_LABEL_RECOLOR

static egui_view_label_t s_label;

static egui_core_t *get_core(void)
{
    egui_core_t *core = uicode_get_core();
    EGUI_ASSERT(core != NULL);
    return core;
}

/* Default: is_recolor is 0 after init. */
static void test_recolor_init_default(void)
{
    egui_view_label_init(EGUI_VIEW_OF(&s_label), get_core());
    EGUI_TEST_ASSERT_FALSE(egui_view_label_get_recolor(EGUI_VIEW_OF(&s_label)));
}

/* set_recolor(1) enables recolor. */
static void test_recolor_set_enable(void)
{
    egui_view_label_init(EGUI_VIEW_OF(&s_label), get_core());
    egui_view_label_set_recolor(EGUI_VIEW_OF(&s_label), 1);
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_recolor(EGUI_VIEW_OF(&s_label)));
}

/* set_recolor(0) disables recolor. */
static void test_recolor_set_disable(void)
{
    egui_view_label_init(EGUI_VIEW_OF(&s_label), get_core());
    egui_view_label_set_recolor(EGUI_VIEW_OF(&s_label), 1);
    egui_view_label_set_recolor(EGUI_VIEW_OF(&s_label), 0);
    EGUI_TEST_ASSERT_FALSE(egui_view_label_get_recolor(EGUI_VIEW_OF(&s_label)));
}

/* NULL-safe: set_recolor on NULL does not crash. */
static void test_recolor_null_safe(void)
{
    egui_view_label_set_recolor(NULL, 1);
    EGUI_TEST_ASSERT_FALSE(egui_view_label_get_recolor(NULL));
}

/* Repeated set with same value is idempotent. */
static void test_recolor_idempotent(void)
{
    egui_view_label_init(EGUI_VIEW_OF(&s_label), get_core());
    egui_view_label_set_recolor(EGUI_VIEW_OF(&s_label), 1);
    egui_view_label_set_recolor(EGUI_VIEW_OF(&s_label), 1);
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_recolor(EGUI_VIEW_OF(&s_label)));
}

#endif /* EGUI_CONFIG_FUNCTION_LABEL_RECOLOR */

void test_label_recolor_run(void)
{
    EGUI_TEST_SUITE_BEGIN(label_recolor);

#if EGUI_CONFIG_FUNCTION_LABEL_RECOLOR
    EGUI_TEST_RUN(test_recolor_init_default);
    EGUI_TEST_RUN(test_recolor_set_enable);
    EGUI_TEST_RUN(test_recolor_set_disable);
    EGUI_TEST_RUN(test_recolor_null_safe);
    EGUI_TEST_RUN(test_recolor_idempotent);
#endif

    EGUI_TEST_SUITE_END();
}
