#include <string.h>

#include "egui.h"
#include "anim/egui_animation_translate.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_anim_is_running.h"

static egui_animation_translate_t s_anim;
static egui_view_t                s_view;
static egui_view_group_t          s_parent;

static const egui_animation_translate_params_t s_params = {
    .from_x = 0, .to_x = 10, .from_y = 0, .to_y = 0
};

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    egui_slist_init(&core->scene.anims);

    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_view,   0, sizeof(s_view));
    memset(&s_anim,   0, sizeof(s_anim));

    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_init(&s_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_view);

    egui_animation_translate_init(EGUI_ANIM_OF(&s_anim));
    egui_animation_translate_params_set(&s_anim, &s_params);
    egui_animation_target_view_set(EGUI_ANIM_OF(&s_anim), &s_view);
    egui_animation_duration_set(EGUI_ANIM_OF(&s_anim), 200);
}

/* Before start, is_running returns 0. */
static void test_anim_not_running_before_start(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_animation_is_running(EGUI_ANIM_OF(&s_anim)));
}

/* After start, is_running returns 1. */
static void test_anim_running_after_start(void)
{
    setup();
    egui_animation_start(EGUI_ANIM_OF(&s_anim));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_animation_is_running(EGUI_ANIM_OF(&s_anim)));
}

/* After stop, is_running returns 0. */
static void test_anim_not_running_after_stop(void)
{
    setup();
    egui_animation_start(EGUI_ANIM_OF(&s_anim));
    egui_animation_stop(EGUI_ANIM_OF(&s_anim));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_animation_is_running(EGUI_ANIM_OF(&s_anim)));
}

/* After complete, is_running returns 0. */
static void test_anim_not_running_after_complete(void)
{
    setup();
    egui_animation_start(EGUI_ANIM_OF(&s_anim));
    egui_animation_complete(EGUI_ANIM_OF(&s_anim));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_animation_is_running(EGUI_ANIM_OF(&s_anim)));
}

/* NULL self: is_running returns 0 without crash. */
static void test_anim_is_running_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_animation_is_running(NULL));
}

void test_anim_is_running_run(void)
{
    EGUI_TEST_SUITE_BEGIN(anim_is_running);

    EGUI_TEST_RUN(test_anim_not_running_before_start);
    EGUI_TEST_RUN(test_anim_running_after_start);
    EGUI_TEST_RUN(test_anim_not_running_after_stop);
    EGUI_TEST_RUN(test_anim_not_running_after_complete);
    EGUI_TEST_RUN(test_anim_is_running_null_self);

    EGUI_TEST_SUITE_END();
}
