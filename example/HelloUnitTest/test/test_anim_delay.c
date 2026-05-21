#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_anim_delay.h"

static egui_animation_translate_t s_anim_delay;
static egui_view_t s_anim_view;
static egui_view_group_t s_anim_parent;
static int s_complete_called;
static egui_animation_t *s_complete_self;

static const egui_animation_translate_params_t s_translate_params = {
    .from_x = 0, .to_x = 50, .from_y = 0, .to_y = 0
};

static void setup_anim_with_view(void)
{
    egui_core_t *core = uicode_get_core();

    egui_slist_init(&core->scene.anims);
    memset(&s_anim_parent, 0, sizeof(s_anim_parent));
    memset(&s_anim_view, 0, sizeof(s_anim_view));
    memset(&s_anim_delay, 0, sizeof(s_anim_delay));
    egui_view_group_init(EGUI_VIEW_OF(&s_anim_parent), core);
    egui_view_init(&s_anim_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_anim_parent), &s_anim_view);

    egui_animation_translate_init(EGUI_ANIM_OF(&s_anim_delay));
    egui_animation_translate_params_set(&s_anim_delay, &s_translate_params);
    egui_animation_target_view_set(EGUI_ANIM_OF(&s_anim_delay), &s_anim_view);
    egui_animation_duration_set(EGUI_ANIM_OF(&s_anim_delay), 100);
}

/* ---- R4-1: anim delay ---- */

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY

/* After init, delay_ms defaults to 0 and is_delay_passed should be effectively 1 after start. */
static void test_delay_init_default(void)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&s_anim_delay));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_anim_delay.base.delay_ms);
}

/* set_delay stores the value. */
static void test_delay_set_delay(void)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&s_anim_delay));
    egui_animation_set_delay(EGUI_ANIM_OF(&s_anim_delay), 200);
    EGUI_TEST_ASSERT_EQUAL_INT(200, (int)s_anim_delay.base.delay_ms);
}

/* With delay=0, animation progresses immediately on first update. */
static void test_delay_zero_progresses_immediately(void)
{
    setup_anim_with_view();
    egui_animation_set_delay(EGUI_ANIM_OF(&s_anim_delay), 0);
    egui_animation_start(EGUI_ANIM_OF(&s_anim_delay));
    /* Update at t=0: anim should start immediately */
    egui_animation_update(EGUI_ANIM_OF(&s_anim_delay), 0);
    EGUI_TEST_ASSERT_TRUE(s_anim_delay.base.is_started);
    egui_animation_stop(EGUI_ANIM_OF(&s_anim_delay));
}

/* With delay>0, animation does not start during the delay window. */
static void test_delay_nonzero_defers_start(void)
{
    setup_anim_with_view();
    egui_animation_set_delay(EGUI_ANIM_OF(&s_anim_delay), 50);
    egui_animation_start(EGUI_ANIM_OF(&s_anim_delay));
    /* First update at t=0: records entry time, but delay not expired */
    egui_animation_update(EGUI_ANIM_OF(&s_anim_delay), 0);
    EGUI_TEST_ASSERT_FALSE(s_anim_delay.base.is_started);
    /* Update at t=30: still within 50 ms delay */
    egui_animation_update(EGUI_ANIM_OF(&s_anim_delay), 30);
    EGUI_TEST_ASSERT_FALSE(s_anim_delay.base.is_started);
    egui_animation_stop(EGUI_ANIM_OF(&s_anim_delay));
}

/* After delay expires, animation starts on next update. */
static void test_delay_expired_then_starts(void)
{
    setup_anim_with_view();
    egui_animation_set_delay(EGUI_ANIM_OF(&s_anim_delay), 50);
    egui_animation_start(EGUI_ANIM_OF(&s_anim_delay));
    egui_animation_update(EGUI_ANIM_OF(&s_anim_delay), 0);
    egui_animation_update(EGUI_ANIM_OF(&s_anim_delay), 30);
    EGUI_TEST_ASSERT_FALSE(s_anim_delay.base.is_started);
    /* Update at t=60: delay of 50 ms has passed */
    egui_animation_update(EGUI_ANIM_OF(&s_anim_delay), 60);
    EGUI_TEST_ASSERT_TRUE(s_anim_delay.base.is_started);
    egui_animation_stop(EGUI_ANIM_OF(&s_anim_delay));
}

#endif /* EGUI_CONFIG_FUNCTION_ANIM_DELAY */

/* ---- R4-6: anim on_complete_cb ---- */

#if EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB

static void on_complete_cb(egui_animation_t *self, void *user_data)
{
    EGUI_UNUSED(user_data);
    s_complete_called++;
    s_complete_self = self;
}

/* After init, on_complete_cb is NULL. */
static void test_complete_cb_init_null(void)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&s_anim_delay));
    EGUI_TEST_ASSERT_NULL(s_anim_delay.base.on_complete_cb);
}

/* set_on_complete stores cb and user_data. */
static void test_complete_cb_set(void)
{
    int user_data = 42;
    egui_animation_translate_init(EGUI_ANIM_OF(&s_anim_delay));
    egui_animation_set_on_complete(EGUI_ANIM_OF(&s_anim_delay), on_complete_cb, &user_data);
    EGUI_TEST_ASSERT_TRUE(s_anim_delay.base.on_complete_cb == on_complete_cb);
    EGUI_TEST_ASSERT_TRUE(s_anim_delay.base.on_complete_user_data == &user_data);
}

/* on_complete_cb is called once when animation finishes via complete(). */
static void test_complete_cb_fired_on_complete(void)
{
    s_complete_called = 0;
    s_complete_self = NULL;
    setup_anim_with_view();
    egui_animation_set_on_complete(EGUI_ANIM_OF(&s_anim_delay), on_complete_cb, NULL);
    egui_animation_start(EGUI_ANIM_OF(&s_anim_delay));
    egui_animation_update(EGUI_ANIM_OF(&s_anim_delay), 0);
    egui_animation_complete(EGUI_ANIM_OF(&s_anim_delay));
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_complete_called);
    EGUI_TEST_ASSERT_TRUE(s_complete_self == EGUI_ANIM_OF(&s_anim_delay));
}

/* on_complete_cb is called once when animation finishes naturally (duration elapsed). */
static void test_complete_cb_fired_on_natural_end(void)
{
    s_complete_called = 0;
    setup_anim_with_view();
    egui_animation_duration_set(EGUI_ANIM_OF(&s_anim_delay), 100);
    egui_animation_is_fill_after_set(EGUI_ANIM_OF(&s_anim_delay), 1);
    egui_animation_set_on_complete(EGUI_ANIM_OF(&s_anim_delay), on_complete_cb, NULL);
    egui_animation_start(EGUI_ANIM_OF(&s_anim_delay));
    egui_animation_update(EGUI_ANIM_OF(&s_anim_delay), 0);
    egui_animation_update(EGUI_ANIM_OF(&s_anim_delay), 101);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_complete_called);
}

/* NULL on_complete_cb does not crash. */
static void test_complete_cb_null_no_crash(void)
{
    setup_anim_with_view();
    egui_animation_set_on_complete(EGUI_ANIM_OF(&s_anim_delay), NULL, NULL);
    egui_animation_start(EGUI_ANIM_OF(&s_anim_delay));
    egui_animation_update(EGUI_ANIM_OF(&s_anim_delay), 0);
    egui_animation_complete(EGUI_ANIM_OF(&s_anim_delay));
    EGUI_TEST_ASSERT_TRUE(1); /* no crash */
}

#endif /* EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB */

void test_anim_delay_run(void)
{
    EGUI_TEST_SUITE_BEGIN(anim_delay);

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY
    EGUI_TEST_RUN(test_delay_init_default);
    EGUI_TEST_RUN(test_delay_set_delay);
    EGUI_TEST_RUN(test_delay_zero_progresses_immediately);
    EGUI_TEST_RUN(test_delay_nonzero_defers_start);
    EGUI_TEST_RUN(test_delay_expired_then_starts);
#endif

#if EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB
    EGUI_TEST_RUN(test_complete_cb_init_null);
    EGUI_TEST_RUN(test_complete_cb_set);
    EGUI_TEST_RUN(test_complete_cb_fired_on_complete);
    EGUI_TEST_RUN(test_complete_cb_fired_on_natural_end);
    EGUI_TEST_RUN(test_complete_cb_null_no_crash);
#endif

    EGUI_TEST_SUITE_END();
}
