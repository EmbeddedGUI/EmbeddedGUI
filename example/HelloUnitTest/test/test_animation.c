#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_animation.h"

static egui_animation_translate_t test_anim_translate;
static egui_animation_alpha_t test_anim_alpha;
static egui_view_t test_anim_view;
static egui_interpolator_linear_t test_anim_interp;
static int test_anim_end_called;

static void test_anim_on_end(egui_animation_t *self)
{
    EGUI_UNUSED(self);
    test_anim_end_called++;
}

static const egui_animation_handle_t test_anim_handle = {
        .start = NULL,
        .repeat = NULL,
        .end = test_anim_on_end,
};

static egui_core_t *test_animation_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

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
    egui_view_init(&test_anim_view, test_animation_get_core());
    egui_animation_target_view_set(EGUI_ANIM_OF(&test_anim_translate), &test_anim_view);
    EGUI_TEST_ASSERT_TRUE(EGUI_ANIM_OF(&test_anim_translate)->target_view == &test_anim_view);
}

static void test_anim_start_uses_target_init_core_when_active_is_null(void)
{
    egui_core_t *core = test_animation_get_core();
    egui_view_group_t parent_view;

    egui_slist_init(&core->scene.anims);
    egui_view_group_init(EGUI_VIEW_OF(&parent_view), core);
    egui_view_init(&test_anim_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent_view), &test_anim_view);

    egui_animation_translate_init(EGUI_ANIM_OF(&test_anim_translate));
    egui_animation_target_view_set(EGUI_ANIM_OF(&test_anim_translate), &test_anim_view);

    EGUI_TEST_ASSERT_TRUE(test_anim_view.core == core);
    EGUI_TEST_ASSERT_TRUE(egui_slist_is_empty(&core->scene.anims));

    egui_animation_start(EGUI_ANIM_OF(&test_anim_translate));

    EGUI_TEST_ASSERT_TRUE(EGUI_ANIM_OF(&test_anim_translate)->is_running);
    EGUI_TEST_ASSERT_TRUE(egui_slist_peek_head(&core->scene.anims) == &EGUI_ANIM_OF(&test_anim_translate)->node);

    egui_animation_stop(EGUI_ANIM_OF(&test_anim_translate));
    EGUI_TEST_ASSERT_FALSE(EGUI_ANIM_OF(&test_anim_translate)->is_running);
    EGUI_TEST_ASSERT_TRUE(egui_slist_is_empty(&core->scene.anims));
}

static void test_anim_start_without_bound_core_is_noop(void)
{
    egui_core_t *core = test_animation_get_core();

    egui_slist_init(&core->scene.anims);
    egui_animation_translate_init(EGUI_ANIM_OF(&test_anim_translate));
    egui_view_init(&test_anim_view, core);
    test_anim_view.core = NULL;
    egui_animation_target_view_set(EGUI_ANIM_OF(&test_anim_translate), &test_anim_view);

    egui_animation_start(EGUI_ANIM_OF(&test_anim_translate));

    EGUI_TEST_ASSERT_FALSE(EGUI_ANIM_OF(&test_anim_translate)->is_running);
    EGUI_TEST_ASSERT_TRUE(egui_slist_is_empty(&core->scene.anims));

    egui_animation_stop(EGUI_ANIM_OF(&test_anim_translate));
    EGUI_TEST_ASSERT_TRUE(egui_slist_is_empty(&core->scene.anims));
}

static void test_anim_restart_reuses_single_queue_node(void)
{
    egui_core_t *core = test_animation_get_core();
    egui_view_group_t parent_view;

    egui_slist_init(&core->scene.anims);
    egui_view_group_init(EGUI_VIEW_OF(&parent_view), core);
    egui_view_init(&test_anim_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent_view), &test_anim_view);

    egui_animation_translate_init(EGUI_ANIM_OF(&test_anim_translate));
    egui_animation_target_view_set(EGUI_ANIM_OF(&test_anim_translate), &test_anim_view);

    egui_animation_start(EGUI_ANIM_OF(&test_anim_translate));
    egui_animation_start(EGUI_ANIM_OF(&test_anim_translate));

    EGUI_TEST_ASSERT_TRUE(EGUI_ANIM_OF(&test_anim_translate)->is_running);
    EGUI_TEST_ASSERT_TRUE(EGUI_ANIM_OF(&test_anim_translate)->queue_core == core);
    EGUI_TEST_ASSERT_TRUE(egui_slist_peek_head(&core->scene.anims) == &EGUI_ANIM_OF(&test_anim_translate)->node);
    EGUI_TEST_ASSERT_TRUE(egui_slist_peek_tail(&core->scene.anims) == &EGUI_ANIM_OF(&test_anim_translate)->node);
    EGUI_TEST_ASSERT_TRUE(EGUI_ANIM_OF(&test_anim_translate)->node.next == NULL);

    egui_animation_stop(EGUI_ANIM_OF(&test_anim_translate));
    EGUI_TEST_ASSERT_FALSE(EGUI_ANIM_OF(&test_anim_translate)->is_running);
    EGUI_TEST_ASSERT_TRUE(EGUI_ANIM_OF(&test_anim_translate)->queue_core == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_slist_is_empty(&core->scene.anims));
}

static void test_anim_complete_settles_translate_and_notifies_end(void)
{
    static const egui_animation_translate_params_t complete_params = {
            .from_x = 0,
            .to_x = 100,
            .from_y = 0,
            .to_y = 0,
    };
    egui_core_t *core = test_animation_get_core();
    egui_view_group_t parent_view;

    test_anim_end_called = 0;
    egui_slist_init(&core->scene.anims);
    egui_view_group_init(EGUI_VIEW_OF(&parent_view), core);
    egui_view_init(&test_anim_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent_view), &test_anim_view);

    egui_animation_translate_init(EGUI_ANIM_OF(&test_anim_translate));
    egui_animation_translate_params_set(&test_anim_translate, &complete_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&test_anim_translate), 100);
    egui_animation_handle_set(EGUI_ANIM_OF(&test_anim_translate), &test_anim_handle);
    egui_animation_is_fill_before_set(EGUI_ANIM_OF(&test_anim_translate), true);
    egui_animation_target_view_set(EGUI_ANIM_OF(&test_anim_translate), &test_anim_view);

    egui_animation_start(EGUI_ANIM_OF(&test_anim_translate));
    egui_animation_update(EGUI_ANIM_OF(&test_anim_translate), 0);
    egui_animation_update(EGUI_ANIM_OF(&test_anim_translate), 50);

    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)test_anim_view.region.location.x);
    EGUI_TEST_ASSERT_TRUE(EGUI_ANIM_OF(&test_anim_translate)->is_running);

    egui_animation_complete(EGUI_ANIM_OF(&test_anim_translate));

    EGUI_TEST_ASSERT_FALSE(EGUI_ANIM_OF(&test_anim_translate)->is_running);
    EGUI_TEST_ASSERT_TRUE(EGUI_ANIM_OF(&test_anim_translate)->queue_core == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_slist_is_empty(&core->scene.anims));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)test_anim_view.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_anim_end_called);
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
    EGUI_TEST_RUN(test_anim_start_uses_target_init_core_when_active_is_null);
    EGUI_TEST_RUN(test_anim_start_without_bound_core_is_noop);
    EGUI_TEST_RUN(test_anim_restart_reuses_single_queue_node);
    EGUI_TEST_RUN(test_anim_complete_settles_translate_and_notifies_end);

    EGUI_TEST_SUITE_END();
}
