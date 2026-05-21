#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_long_mode.h"

#if EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE

static egui_view_label_t s_label;

static egui_core_t *get_core(void)
{
    egui_core_t *core = uicode_get_core();
    EGUI_ASSERT(core != NULL);
    return core;
}

static void init_label(void)
{
    egui_view_label_init(EGUI_VIEW_OF(&s_label), get_core());
}

/* long_mode defaults to EGUI_LABEL_LONG_CLIP after init. */
static void test_long_mode_default(void)
{
    init_label();
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_LABEL_LONG_CLIP, (int)egui_view_label_get_long_mode(EGUI_VIEW_OF(&s_label)));
}

/* set_long_mode stores DOTS mode. */
static void test_long_mode_set_dots(void)
{
    init_label();
    egui_view_label_set_long_mode(EGUI_VIEW_OF(&s_label), EGUI_LABEL_LONG_DOTS);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_LABEL_LONG_DOTS, (int)egui_view_label_get_long_mode(EGUI_VIEW_OF(&s_label)));
}

/* set_long_mode is idempotent. */
static void test_long_mode_idempotent(void)
{
    init_label();
    egui_view_label_set_long_mode(EGUI_VIEW_OF(&s_label), EGUI_LABEL_LONG_DOTS);
    egui_view_label_set_long_mode(EGUI_VIEW_OF(&s_label), EGUI_LABEL_LONG_DOTS);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_LABEL_LONG_DOTS, (int)egui_view_label_get_long_mode(EGUI_VIEW_OF(&s_label)));
}

/* Reset from DOTS back to CLIP. */
static void test_long_mode_reset(void)
{
    init_label();
    egui_view_label_set_long_mode(EGUI_VIEW_OF(&s_label), EGUI_LABEL_LONG_DOTS);
    egui_view_label_set_long_mode(EGUI_VIEW_OF(&s_label), EGUI_LABEL_LONG_CLIP);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_LABEL_LONG_CLIP, (int)egui_view_label_get_long_mode(EGUI_VIEW_OF(&s_label)));
}

/* DOTS mode with NULL font does not crash on draw (guard inside on_draw). */
static void test_long_mode_dots_null_font_no_crash(void)
{
    init_label();
    egui_view_label_set_long_mode(EGUI_VIEW_OF(&s_label), EGUI_LABEL_LONG_DOTS);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_label), "Hello world overflow text");
    /* font is NULL by default — on_draw must not crash */
    egui_view_label_on_draw(EGUI_VIEW_OF(&s_label)); /* no canvas active: expected no-op */
    EGUI_TEST_ASSERT_TRUE(1);
}

/* CLIP mode returns the original pointer from get_text (not modified). */
static void test_long_mode_clip_text_unchanged(void)
{
    static const char *msg = "Hello";
    init_label();
    egui_view_label_set_long_mode(EGUI_VIEW_OF(&s_label), EGUI_LABEL_LONG_CLIP);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_label), msg);
    EGUI_TEST_ASSERT_TRUE(s_label.text == msg);
}

/* NULL self returns the default CLIP mode without crash. */
static void test_long_mode_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_LABEL_LONG_CLIP, (int)egui_view_label_get_long_mode(NULL));
}

#endif /* EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE */

void test_label_long_mode_run(void)
{
#if EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
    EGUI_TEST_RUN(test_long_mode_default);
    EGUI_TEST_RUN(test_long_mode_set_dots);
    EGUI_TEST_RUN(test_long_mode_idempotent);
    EGUI_TEST_RUN(test_long_mode_reset);
    EGUI_TEST_RUN(test_long_mode_dots_null_font_no_crash);
    EGUI_TEST_RUN(test_long_mode_clip_text_unchanged);
    EGUI_TEST_RUN(test_long_mode_null_self);
#endif
}
