#include "egui.h"
#include "test/egui_test.h"
#include "test_animation.h"

static egui_animation_translate_t test_anim_translate;
static egui_animation_alpha_t test_anim_alpha;
static egui_view_t test_anim_view;
static egui_interpolator_linear_t test_anim_interp;

static void test_anim_translate_init(void)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&test_anim_translate));
    EGUI_TEST_ASSERT_NOT_NULL(EGUI_ANIM_OF(&test_anim_translate)->api);
}

static void test_anim_alpha_init(void)
{
    egui_animation_alpha_init(EGUI_ANIM_OF(&test_anim_alpha));
    EGUI_TEST_ASSERT_NOT_NULL(EGUI_ANIM_OF(&test_anim_alpha)->api);
}

static void test_anim_duration_set(void)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&test_anim_translate));
    egui_animation_duration_set(EGUI_ANIM_OF(&test_anim_translate), 1000);
    EGUI_TEST_ASSERT_EQUAL_INT(1000, (int)EGUI_ANIM_OF(&test_anim_translate)->duration);
}

static void test_anim_repeat_set(void)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&test_anim_translate));
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&test_anim_translate), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)EGUI_ANIM_OF(&test_anim_translate)->repeat_count);

    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&test_anim_translate), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ANIMATION_REPEAT_MODE_REVERSE, (int)EGUI_ANIM_OF(&test_anim_translate)->repeat_mode);
}

static void test_anim_interpolator_set(void)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&test_anim_translate));
    egui_interpolator_linear_init(EGUI_INTERP_OF(&test_anim_interp));
    egui_animation_interpolator_set(EGUI_ANIM_OF(&test_anim_translate), EGUI_INTERP_OF(&test_anim_interp));
    EGUI_TEST_ASSERT_TRUE(EGUI_ANIM_OF(&test_anim_translate)->interpolator == EGUI_INTERP_OF(&test_anim_interp));
}

static void test_anim_target_view_set(void)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&test_anim_translate));
    egui_view_init(&test_anim_view);
    egui_animation_target_view_set(EGUI_ANIM_OF(&test_anim_translate), &test_anim_view);
    EGUI_TEST_ASSERT_TRUE(EGUI_ANIM_OF(&test_anim_translate)->target_view == &test_anim_view);
}

void test_animation_run(void)
{
    EGUI_TEST_SUITE_BEGIN(animation);

    EGUI_TEST_RUN(test_anim_translate_init);
    EGUI_TEST_RUN(test_anim_alpha_init);
    EGUI_TEST_RUN(test_anim_duration_set);
    EGUI_TEST_RUN(test_anim_repeat_set);
    EGUI_TEST_RUN(test_anim_interpolator_set);
    EGUI_TEST_RUN(test_anim_target_view_set);

    EGUI_TEST_SUITE_END();
}
