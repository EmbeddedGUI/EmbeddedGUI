#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_anim_value.h"

static egui_animation_value_t s_anim;
static int32_t s_last_value;
static int s_cb_count;

static void on_value_cb(egui_animation_t *self, int32_t value)
{
    EGUI_UNUSED(self);
    s_last_value = value;
    s_cb_count++;
}

static void reset_counters(void)
{
    s_last_value = 0;
    s_cb_count = 0;
}

/* init sets from/to to 0 and on_value to NULL. */
static void test_value_init_defaults(void)
{
    egui_animation_value_init(EGUI_ANIM_OF(&s_anim));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_anim.from);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_anim.to);
    EGUI_TEST_ASSERT_NULL(s_anim.on_value);
    EGUI_TEST_ASSERT_NOT_NULL(EGUI_ANIM_OF(&s_anim)->api);
}

/* set_range stores from and to. */
static void test_value_set_range(void)
{
    egui_animation_value_init(EGUI_ANIM_OF(&s_anim));
    egui_animation_value_set_range(&s_anim, 10, 90);
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)s_anim.from);
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)s_anim.to);
}

/* set_on_value stores the callback pointer. */
static void test_value_set_on_value(void)
{
    egui_animation_value_init(EGUI_ANIM_OF(&s_anim));
    egui_animation_value_set_on_value(&s_anim, on_value_cb);
    EGUI_TEST_ASSERT_TRUE(s_anim.on_value == on_value_cb);
}

/* on_update(fraction=0) fires callback with from value. */
static void test_value_on_update_fraction_zero(void)
{
    reset_counters();
    egui_animation_value_init(EGUI_ANIM_OF(&s_anim));
    egui_animation_value_set_range(&s_anim, 0, 100);
    egui_animation_value_set_on_value(&s_anim, on_value_cb);
    EGUI_ANIM_OF(&s_anim)->api->on_update(EGUI_ANIM_OF(&s_anim), EGUI_FLOAT_VALUE(0.0f));
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_cb_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_last_value);
}

/* on_update(fraction=1) fires callback with to value. */
static void test_value_on_update_fraction_one(void)
{
    reset_counters();
    egui_animation_value_init(EGUI_ANIM_OF(&s_anim));
    egui_animation_value_set_range(&s_anim, 0, 100);
    egui_animation_value_set_on_value(&s_anim, on_value_cb);
    EGUI_ANIM_OF(&s_anim)->api->on_update(EGUI_ANIM_OF(&s_anim), EGUI_FLOAT_VALUE(1.0f));
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_cb_count);
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)s_last_value);
}

/* on_update(fraction=0.5) fires callback with midpoint value (within +-1 tolerance). */
static void test_value_on_update_midpoint(void)
{
    reset_counters();
    egui_animation_value_init(EGUI_ANIM_OF(&s_anim));
    egui_animation_value_set_range(&s_anim, 0, 100);
    egui_animation_value_set_on_value(&s_anim, on_value_cb);
    EGUI_ANIM_OF(&s_anim)->api->on_update(EGUI_ANIM_OF(&s_anim), EGUI_FLOAT_VALUE(0.5f));
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_cb_count);
    /* Fixed-point midpoint: allow ±1 rounding. */
    EGUI_TEST_ASSERT_TRUE(s_last_value >= 49 && s_last_value <= 51);
}

/* on_update with NULL callback does not crash. */
static void test_value_null_callback_no_crash(void)
{
    egui_animation_value_init(EGUI_ANIM_OF(&s_anim));
    egui_animation_value_set_range(&s_anim, 0, 100);
    /* on_value deliberately left NULL */
    EGUI_ANIM_OF(&s_anim)->api->on_update(EGUI_ANIM_OF(&s_anim), EGUI_FLOAT_VALUE(0.5f));
    EGUI_TEST_ASSERT_TRUE(1); /* reaching here means no crash */
}

/* Negative range: from=100 to=0, fraction=1 gives 0. */
static void test_value_negative_range(void)
{
    reset_counters();
    egui_animation_value_init(EGUI_ANIM_OF(&s_anim));
    egui_animation_value_set_range(&s_anim, 100, 0);
    egui_animation_value_set_on_value(&s_anim, on_value_cb);
    EGUI_ANIM_OF(&s_anim)->api->on_update(EGUI_ANIM_OF(&s_anim), EGUI_FLOAT_VALUE(1.0f));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_last_value);
}

/* Multiple on_update calls accumulate cb_count. */
static void test_value_multiple_updates(void)
{
    reset_counters();
    egui_animation_value_init(EGUI_ANIM_OF(&s_anim));
    egui_animation_value_set_range(&s_anim, 0, 10);
    egui_animation_value_set_on_value(&s_anim, on_value_cb);
    EGUI_ANIM_OF(&s_anim)->api->on_update(EGUI_ANIM_OF(&s_anim), EGUI_FLOAT_VALUE(0.0f));
    EGUI_ANIM_OF(&s_anim)->api->on_update(EGUI_ANIM_OF(&s_anim), EGUI_FLOAT_VALUE(0.5f));
    EGUI_ANIM_OF(&s_anim)->api->on_update(EGUI_ANIM_OF(&s_anim), EGUI_FLOAT_VALUE(1.0f));
    EGUI_TEST_ASSERT_EQUAL_INT(3, s_cb_count);
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)s_last_value);
}

void test_anim_value_run(void)
{
    EGUI_TEST_RUN(test_value_init_defaults);
    EGUI_TEST_RUN(test_value_set_range);
    EGUI_TEST_RUN(test_value_set_on_value);
    EGUI_TEST_RUN(test_value_on_update_fraction_zero);
    EGUI_TEST_RUN(test_value_on_update_fraction_one);
    EGUI_TEST_RUN(test_value_on_update_midpoint);
    EGUI_TEST_RUN(test_value_null_callback_no_crash);
    EGUI_TEST_RUN(test_value_negative_range);
    EGUI_TEST_RUN(test_value_multiple_updates);
}
