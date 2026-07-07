#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_sprintf.h"

static void test_sprintf_assert_cstr_equal(const char *expected, const char *actual)
{
    EGUI_TEST_ASSERT_TRUE(strcmp(expected, actual) == 0);
}

static void test_sprintf_integer_width_and_literals(void)
{
    char buf[64];

    egui_api_sprintf(buf, "n=%03d u=%lu s=%s c=%c %%", 7, 42UL, "ok", 'X');
    test_sprintf_assert_cstr_equal("n=007 u=42 s=ok c=X %", buf);
}

static void test_sprintf_float_format_gate(void)
{
    char buf[64];

    egui_api_sprintf(buf, "p=%06.2f", 12.345);
#if EGUI_CONFIG_PLATFORM_CUSTOM_SPRINTF_FLOAT
    test_sprintf_assert_cstr_equal("p=012.35", buf);
#else
    test_sprintf_assert_cstr_equal("p=%06.2f", buf);
#endif
}

static void test_sprintf_float_negative_zero_pad(void)
{
#if EGUI_CONFIG_PLATFORM_CUSTOM_SPRINTF_FLOAT
    char buf[64];

    egui_api_sprintf(buf, "v=%06.2f", -3.4);
    test_sprintf_assert_cstr_equal("v=-03.40", buf);
#else
    EGUI_TEST_ASSERT_TRUE(1);
#endif
}

void test_sprintf_run(void)
{
    EGUI_TEST_SUITE_BEGIN(sprintf);

    EGUI_TEST_RUN(test_sprintf_integer_width_and_literals);
    EGUI_TEST_RUN(test_sprintf_float_format_gate);
    EGUI_TEST_RUN(test_sprintf_float_negative_zero_pad);

    EGUI_TEST_SUITE_END();
}
