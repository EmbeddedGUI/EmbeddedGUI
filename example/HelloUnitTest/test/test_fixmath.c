#include "egui.h"
#include "test/egui_test.h"
#include "test_fixmath.h"

#define TEST_FX_FRAC      16
#define TEST_FX_TOLERANCE 2 // Allow 2 ulp tolerance for rounding

static void test_fx_itox(void)
{
    // Integer 1 to fixed point
    int32_t result = EGUI_FX_ITOX(1, TEST_FX_FRAC);
    EGUI_TEST_ASSERT_EQUAL_INT(1 << TEST_FX_FRAC, result);

    // Integer 0
    result = EGUI_FX_ITOX(0, TEST_FX_FRAC);
    EGUI_TEST_ASSERT_EQUAL_INT(0, result);

    // Negative
    result = EGUI_FX_ITOX(-1, TEST_FX_FRAC);
    EGUI_TEST_ASSERT_EQUAL_INT(-(1 << TEST_FX_FRAC), result);

    // Larger value
    result = EGUI_FX_ITOX(100, TEST_FX_FRAC);
    EGUI_TEST_ASSERT_EQUAL_INT(100 << TEST_FX_FRAC, result);
}

static void test_fx_ftox(void)
{
    // 0.5f
    int32_t result = EGUI_FX_FTOX(0.5f, TEST_FX_FRAC);
    int32_t expected = 1 << (TEST_FX_FRAC - 1); // 0.5 = 1<<15 in Q16.16
    EGUI_TEST_ASSERT_TRUE(abs(result - expected) <= TEST_FX_TOLERANCE);

    // 1.0f
    result = EGUI_FX_FTOX(1.0f, TEST_FX_FRAC);
    expected = 1 << TEST_FX_FRAC;
    EGUI_TEST_ASSERT_TRUE(abs(result - expected) <= TEST_FX_TOLERANCE);

    // -0.5f
    result = EGUI_FX_FTOX(-0.5f, TEST_FX_FRAC);
    expected = -(1 << (TEST_FX_FRAC - 1));
    EGUI_TEST_ASSERT_TRUE(abs(result - expected) <= TEST_FX_TOLERANCE);
}

static void test_fx_smul(void)
{
    // 2.0 * 3.0 = 6.0
    int32_t a = EGUI_FX_ITOX(2, TEST_FX_FRAC);
    int32_t b = EGUI_FX_ITOX(3, TEST_FX_FRAC);
    int32_t result = (int32_t)EGUI_FX_SMUL(a, b, TEST_FX_FRAC);
    int32_t expected = EGUI_FX_ITOX(6, TEST_FX_FRAC);
    EGUI_TEST_ASSERT_EQUAL_INT(expected, result);

    // 0.5 * 0.5 = 0.25
    a = EGUI_FX_FTOX(0.5f, TEST_FX_FRAC);
    b = EGUI_FX_FTOX(0.5f, TEST_FX_FRAC);
    result = (int32_t)EGUI_FX_SMUL(a, b, TEST_FX_FRAC);
    expected = EGUI_FX_FTOX(0.25f, TEST_FX_FRAC);
    EGUI_TEST_ASSERT_TRUE(abs(result - expected) <= TEST_FX_TOLERANCE);
}

static void test_fx_divx(void)
{
    // 6.0 / 2.0 = 3.0
    int32_t a = EGUI_FX_ITOX(6, TEST_FX_FRAC);
    int32_t b = EGUI_FX_ITOX(2, TEST_FX_FRAC);
    int32_t result = EGUI_FX_DIVX(a, b, TEST_FX_FRAC);
    int32_t expected = EGUI_FX_ITOX(3, TEST_FX_FRAC);
    EGUI_TEST_ASSERT_TRUE(abs(result - expected) <= TEST_FX_TOLERANCE);

    // 1.0 / 4.0 = 0.25
    a = EGUI_FX_ITOX(1, TEST_FX_FRAC);
    b = EGUI_FX_ITOX(4, TEST_FX_FRAC);
    result = EGUI_FX_DIVX(a, b, TEST_FX_FRAC);
    expected = EGUI_FX_FTOX(0.25f, TEST_FX_FRAC);
    EGUI_TEST_ASSERT_TRUE(abs(result - expected) <= TEST_FX_TOLERANCE);
}

static void test_float_value_int(void)
{
    // EGUI_FLOAT_VALUE_INT should convert integer to fixed
    egui_float_t val = EGUI_FLOAT_VALUE_INT(1);
    EGUI_TEST_ASSERT_EQUAL_INT(1 << EGUI_FLOAT_FRAC, (int)val);

    val = EGUI_FLOAT_VALUE_INT(0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)val);
}

static void test_float_mult(void)
{
    // 2.0 * 3.0 = 6.0
    egui_float_t a = EGUI_FLOAT_VALUE_INT(2);
    egui_float_t b = EGUI_FLOAT_VALUE_INT(3);
    egui_float_t result = EGUI_FLOAT_MULT(a, b);
    egui_float_t expected = EGUI_FLOAT_VALUE_INT(6);
    EGUI_TEST_ASSERT_TRUE(abs((int)(result - expected)) <= TEST_FX_TOLERANCE);
}

static void test_float_div(void)
{
    // 6.0 / 2.0 = 3.0
    egui_float_t a = EGUI_FLOAT_VALUE_INT(6);
    egui_float_t b = EGUI_FLOAT_VALUE_INT(2);
    egui_float_t result = EGUI_FLOAT_DIV(a, b);
    egui_float_t expected = EGUI_FLOAT_VALUE_INT(3);
    EGUI_TEST_ASSERT_TRUE(abs((int)(result - expected)) <= TEST_FX_TOLERANCE);
}

void test_fixmath_run(void)
{
    EGUI_TEST_SUITE_BEGIN(fixmath);

    EGUI_TEST_RUN(test_fx_itox);
    EGUI_TEST_RUN(test_fx_ftox);
    EGUI_TEST_RUN(test_fx_smul);
    EGUI_TEST_RUN(test_fx_divx);
    EGUI_TEST_RUN(test_float_value_int);
    EGUI_TEST_RUN(test_float_mult);
    EGUI_TEST_RUN(test_float_div);

    EGUI_TEST_SUITE_END();
}
