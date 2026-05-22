#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_dynamic_label_get_text.h"

static egui_view_dynamic_label_t s_label;

static void setup(void)
{
    memset(&s_label, 0, sizeof(s_label));
    egui_view_dynamic_label_init(EGUI_VIEW_OF(&s_label), uicode_get_core());
}

static void test_dynamic_label_get_text_default(void)
{
    setup();

    EGUI_TEST_ASSERT_TRUE(egui_view_dynamic_label_get_text(EGUI_VIEW_OF(&s_label)) == s_label.text_buffer);
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_dynamic_label_get_text(EGUI_VIEW_OF(&s_label)), ""));
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_text(EGUI_VIEW_OF(&s_label)) == s_label.text_buffer);
}

static void test_dynamic_label_get_text_after_set(void)
{
    setup();
    egui_view_dynamic_label_set_text(EGUI_VIEW_OF(&s_label), "dynamic");

    EGUI_TEST_ASSERT_TRUE(egui_view_dynamic_label_get_text(EGUI_VIEW_OF(&s_label)) == s_label.text_buffer);
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_dynamic_label_get_text(EGUI_VIEW_OF(&s_label)), "dynamic"));
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_text(EGUI_VIEW_OF(&s_label)) == s_label.text_buffer);
}

static void test_dynamic_label_get_text_truncates_to_buffer(void)
{
    setup();
    egui_view_dynamic_label_set_text(EGUI_VIEW_OF(&s_label), "abcdefghijklmnopqrstuvwxyz");

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE - 1, (int)strlen(egui_view_dynamic_label_get_text(EGUI_VIEW_OF(&s_label))));
    EGUI_TEST_ASSERT_EQUAL_INT(0,
                               strncmp(egui_view_dynamic_label_get_text(EGUI_VIEW_OF(&s_label)), "abcdefghijklmnopqrs", EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE - 1));
}

static void test_dynamic_label_get_text_rebinds_after_params(void)
{
    static const egui_view_label_params_t params = {
            .region = {{1, 2}, {30, 10}},
            .align_type = EGUI_ALIGN_CENTER,
            .text = "borrowed",
            .font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT,
            .color = EGUI_COLOR_WHITE_INIT,
            .alpha = EGUI_ALPHA_100,
    };

    setup();
    egui_view_dynamic_label_apply_params(EGUI_VIEW_OF(&s_label), &params);
    EGUI_TEST_ASSERT_TRUE(egui_view_dynamic_label_get_text(EGUI_VIEW_OF(&s_label)) == s_label.text_buffer);
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_text(EGUI_VIEW_OF(&s_label)) == s_label.text_buffer);
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_dynamic_label_get_text(EGUI_VIEW_OF(&s_label)), params.text));

    egui_view_label_apply_params(EGUI_VIEW_OF(&s_label), &params);
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_text(EGUI_VIEW_OF(&s_label)) == params.text);

    egui_view_dynamic_label_set_text(EGUI_VIEW_OF(&s_label), "owned");
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_text(EGUI_VIEW_OF(&s_label)) == s_label.text_buffer);
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_dynamic_label_get_text(EGUI_VIEW_OF(&s_label)), "owned"));
}

static void test_dynamic_label_get_text_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_dynamic_label_get_text(NULL));
}

void test_dynamic_label_get_text_run(void)
{
    EGUI_TEST_SUITE_BEGIN(dynamic_label_get_text);

    EGUI_TEST_RUN(test_dynamic_label_get_text_default);
    EGUI_TEST_RUN(test_dynamic_label_get_text_after_set);
    EGUI_TEST_RUN(test_dynamic_label_get_text_truncates_to_buffer);
    EGUI_TEST_RUN(test_dynamic_label_get_text_rebinds_after_params);
    EGUI_TEST_RUN(test_dynamic_label_get_text_null_self);

    EGUI_TEST_SUITE_END();
}
