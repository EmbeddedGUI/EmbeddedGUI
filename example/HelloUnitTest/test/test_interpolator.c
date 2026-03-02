#include "egui.h"
#include "test/egui_test.h"
#include "test_interpolator.h"

#define FX_TOLERANCE 100 // Tolerance for fixed-point comparisons

static egui_interpolator_linear_t interp_linear;
static egui_interpolator_accelerate_t interp_accel;
static egui_interpolator_decelerate_t interp_decel;
static egui_interpolator_accelerate_decelerate_t interp_accel_decel;
static egui_interpolator_anticipate_t interp_anticipate;
static egui_interpolator_overshoot_t interp_overshoot;
static egui_interpolator_anticipate_overshoot_t interp_anticipate_overshoot;
static egui_interpolator_bounce_t interp_bounce;
static egui_interpolator_cycle_t interp_cycle;

// Helper: check boundary values for an interpolator
// At input=0, output should be 0; at input=1.0, output should be 1.0
static void check_interpolator_boundaries(egui_interpolator_t *interp, const char *name)
{
    egui_float_t val_0 = EGUI_FLOAT_VALUE_INT(0);
    egui_float_t val_1 = EGUI_FLOAT_VALUE_INT(1);

    egui_float_t result_at_0 = interp->api->get_interpolation(interp, val_0);
    egui_float_t result_at_1 = interp->api->get_interpolation(interp, val_1);

    // At 0, result should be near 0
    EGUI_TEST_ASSERT_TRUE(abs((int)result_at_0) <= FX_TOLERANCE);
    // At 1.0, result should be near 1.0
    EGUI_TEST_ASSERT_TRUE(abs((int)(result_at_1 - val_1)) <= FX_TOLERANCE);
}

static void test_interp_linear(void)
{
    egui_interpolator_linear_init(EGUI_INTERP_OF(&interp_linear));
    check_interpolator_boundaries(EGUI_INTERP_OF(&interp_linear), "linear");

    // Linear at 0.5 should be 0.5
    egui_float_t val_half = EGUI_FLOAT_VALUE(0.5f);
    egui_float_t result = EGUI_INTERP_OF(&interp_linear)->api->get_interpolation(EGUI_INTERP_OF(&interp_linear), val_half);
    EGUI_TEST_ASSERT_TRUE(abs((int)(result - val_half)) <= FX_TOLERANCE);
}

static void test_interp_accelerate(void)
{
    egui_interpolator_accelerate_init(EGUI_INTERP_OF(&interp_accel));
    check_interpolator_boundaries(EGUI_INTERP_OF(&interp_accel), "accelerate");

    // At 0.5, accelerate should return less than 0.5
    egui_float_t val_half = EGUI_FLOAT_VALUE(0.5f);
    egui_float_t result = EGUI_INTERP_OF(&interp_accel)->api->get_interpolation(EGUI_INTERP_OF(&interp_accel), val_half);
    EGUI_TEST_ASSERT_TRUE(result < val_half);
}

static void test_interp_decelerate(void)
{
    egui_interpolator_decelerate_init(EGUI_INTERP_OF(&interp_decel));
    check_interpolator_boundaries(EGUI_INTERP_OF(&interp_decel), "decelerate");

    // At 0.5, decelerate should return more than 0.5
    egui_float_t val_half = EGUI_FLOAT_VALUE(0.5f);
    egui_float_t result = EGUI_INTERP_OF(&interp_decel)->api->get_interpolation(EGUI_INTERP_OF(&interp_decel), val_half);
    EGUI_TEST_ASSERT_TRUE(result > val_half);
}

static void test_interp_accelerate_decelerate(void)
{
    egui_interpolator_accelerate_decelerate_init(EGUI_INTERP_OF(&interp_accel_decel));

    egui_float_t val_0 = EGUI_FLOAT_VALUE_INT(0);
    egui_float_t val_1 = EGUI_FLOAT_VALUE_INT(1);

    // At 0, result should be near 0 (wider tolerance for cos fixed-point precision)
    egui_float_t result_at_0 = EGUI_INTERP_OF(&interp_accel_decel)->api->get_interpolation(EGUI_INTERP_OF(&interp_accel_decel), val_0);
    EGUI_TEST_ASSERT_TRUE(abs((int)result_at_0) <= 1000);

    // At 1.0, result should be near 1.0
    egui_float_t result_at_1 = EGUI_INTERP_OF(&interp_accel_decel)->api->get_interpolation(EGUI_INTERP_OF(&interp_accel_decel), val_1);
    EGUI_TEST_ASSERT_TRUE(abs((int)(result_at_1 - val_1)) <= 1000);
}

static void test_interp_anticipate(void)
{
    egui_interpolator_anticipate_init(EGUI_INTERP_OF(&interp_anticipate));

    egui_float_t val_1 = EGUI_FLOAT_VALUE_INT(1);

    // At 1.0, result should be near 1.0
    egui_float_t result_at_1 = EGUI_INTERP_OF(&interp_anticipate)->api->get_interpolation(EGUI_INTERP_OF(&interp_anticipate), val_1);
    EGUI_TEST_ASSERT_TRUE(abs((int)(result_at_1 - val_1)) <= FX_TOLERANCE);

    // Anticipate goes negative at start (pulls back before going forward)
    egui_float_t val_small = EGUI_FLOAT_VALUE(0.1f);
    egui_float_t result_small = EGUI_INTERP_OF(&interp_anticipate)->api->get_interpolation(EGUI_INTERP_OF(&interp_anticipate), val_small);
    EGUI_TEST_ASSERT_TRUE(result_small < 0);
}

static void test_interp_overshoot(void)
{
    egui_interpolator_overshoot_init(EGUI_INTERP_OF(&interp_overshoot));
    check_interpolator_boundaries(EGUI_INTERP_OF(&interp_overshoot), "overshoot");
}

static void test_interp_anticipate_overshoot(void)
{
    egui_interpolator_anticipate_overshoot_init(EGUI_INTERP_OF(&interp_anticipate_overshoot));
    check_interpolator_boundaries(EGUI_INTERP_OF(&interp_anticipate_overshoot), "anticipate_overshoot");
}

static void test_interp_bounce(void)
{
    egui_interpolator_bounce_init(EGUI_INTERP_OF(&interp_bounce));
    check_interpolator_boundaries(EGUI_INTERP_OF(&interp_bounce), "bounce");
}

static void test_interp_cycle(void)
{
    egui_interpolator_cycle_init(EGUI_INTERP_OF(&interp_cycle));

    // Cycle interpolator at 0 should be 0
    egui_float_t val_0 = EGUI_FLOAT_VALUE_INT(0);
    egui_float_t result = EGUI_INTERP_OF(&interp_cycle)->api->get_interpolation(EGUI_INTERP_OF(&interp_cycle), val_0);
    EGUI_TEST_ASSERT_TRUE(abs((int)result) <= FX_TOLERANCE);

    // Cycle at 1.0 should be near 0 (sin(2*pi) = 0)
    egui_float_t val_1 = EGUI_FLOAT_VALUE_INT(1);
    result = EGUI_INTERP_OF(&interp_cycle)->api->get_interpolation(EGUI_INTERP_OF(&interp_cycle), val_1);
    EGUI_TEST_ASSERT_TRUE(abs((int)result) <= FX_TOLERANCE);
}

void test_interpolator_run(void)
{
    EGUI_TEST_SUITE_BEGIN(interpolator);

    EGUI_TEST_RUN(test_interp_linear);
    EGUI_TEST_RUN(test_interp_accelerate);
    EGUI_TEST_RUN(test_interp_decelerate);
    EGUI_TEST_RUN(test_interp_accelerate_decelerate);
    EGUI_TEST_RUN(test_interp_anticipate);
    EGUI_TEST_RUN(test_interp_overshoot);
    EGUI_TEST_RUN(test_interp_anticipate_overshoot);
    EGUI_TEST_RUN(test_interp_bounce);
    EGUI_TEST_RUN(test_interp_cycle);

    EGUI_TEST_SUITE_END();
}
