#include <string.h>

#include "egui.h"
#include "widget/egui_view_spangroup.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_spangroup_get_state.h"

static egui_view_spangroup_t s_group;

static void setup(void)
{
    memset(&s_group, 0, sizeof(s_group));
    egui_view_spangroup_init(EGUI_VIEW_OF(&s_group), uicode_get_core());
}

static void test_spangroup_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_spangroup_get_span_count(EGUI_VIEW_OF(&s_group)));
    EGUI_TEST_ASSERT_NULL(egui_view_spangroup_get_span(EGUI_VIEW_OF(&s_group), 0));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALIGN_LEFT, (int)egui_view_spangroup_get_align(EGUI_VIEW_OF(&s_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_spangroup_get_line_spacing(EGUI_VIEW_OF(&s_group)));
}

static void test_spangroup_get_state_after_add(void)
{
    const char *text0 = "A";
    const char *text1 = "B";
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_color_t color0 = {.full = 0x1234};
    egui_color_t color1 = {.full = 0x5678};
    const egui_view_span_t *span;

    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_spangroup_add_span(EGUI_VIEW_OF(&s_group), text0, font, color0));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_spangroup_add_span(EGUI_VIEW_OF(&s_group), text1, NULL, color1));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_spangroup_get_span_count(EGUI_VIEW_OF(&s_group)));

    span = egui_view_spangroup_get_span(EGUI_VIEW_OF(&s_group), 0);
    EGUI_TEST_ASSERT_TRUE(span != NULL);
    EGUI_TEST_ASSERT_TRUE(span->text == text0);
    EGUI_TEST_ASSERT_TRUE(span->font == font);
    EGUI_TEST_ASSERT_EQUAL_INT((int)color0.full, (int)span->color.full);

    span = egui_view_spangroup_get_span(EGUI_VIEW_OF(&s_group), 1);
    EGUI_TEST_ASSERT_TRUE(span != NULL);
    EGUI_TEST_ASSERT_TRUE(span->text == text1);
    EGUI_TEST_ASSERT_NULL(span->font);
    EGUI_TEST_ASSERT_EQUAL_INT((int)color1.full, (int)span->color.full);
}

static void test_spangroup_get_state_clear_and_capacity(void)
{
    egui_color_t color = {.full = 0x2468};

    setup();
    for (int i = 0; i < EGUI_VIEW_SPANGROUP_MAX_SPANS; i++)
    {
        EGUI_TEST_ASSERT_EQUAL_INT(i, egui_view_spangroup_add_span(EGUI_VIEW_OF(&s_group), "x", NULL, color));
    }
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPANGROUP_MAX_SPANS, (int)egui_view_spangroup_get_span_count(EGUI_VIEW_OF(&s_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_spangroup_add_span(EGUI_VIEW_OF(&s_group), "overflow", NULL, color));
    EGUI_TEST_ASSERT_NULL(egui_view_spangroup_get_span(EGUI_VIEW_OF(&s_group), EGUI_VIEW_SPANGROUP_MAX_SPANS));

    egui_view_spangroup_clear(EGUI_VIEW_OF(&s_group));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_spangroup_get_span_count(EGUI_VIEW_OF(&s_group)));
    EGUI_TEST_ASSERT_NULL(egui_view_spangroup_get_span(EGUI_VIEW_OF(&s_group), 0));
}

static void test_spangroup_get_state_align_and_spacing(void)
{
    setup();

    egui_view_spangroup_set_align(EGUI_VIEW_OF(&s_group), EGUI_ALIGN_CENTER);
    egui_view_spangroup_set_line_spacing(EGUI_VIEW_OF(&s_group), 7);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALIGN_CENTER, (int)egui_view_spangroup_get_align(EGUI_VIEW_OF(&s_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(7, (int)egui_view_spangroup_get_line_spacing(EGUI_VIEW_OF(&s_group)));
}

static void test_spangroup_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_spangroup_get_span_count(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_spangroup_get_span(NULL, 0));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_spangroup_get_align(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_spangroup_get_line_spacing(NULL));
}

void test_spangroup_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(spangroup_get_state);

    EGUI_TEST_RUN(test_spangroup_get_state_defaults);
    EGUI_TEST_RUN(test_spangroup_get_state_after_add);
    EGUI_TEST_RUN(test_spangroup_get_state_clear_and_capacity);
    EGUI_TEST_RUN(test_spangroup_get_state_align_and_spacing);
    EGUI_TEST_RUN(test_spangroup_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
