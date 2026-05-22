#include <string.h>

#include "egui.h"
#include "anim/egui_animation_translate.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_anim_repeat_mode_get.h"

static egui_animation_translate_t s_anim;
static egui_view_t s_view;
static egui_view_group_t s_parent;

static const egui_animation_translate_params_t s_params = {.from_x = 0, .to_x = 10, .from_y = 0, .to_y = 0};

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    egui_slist_init(&core->scene.anims);
    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_view, 0, sizeof(s_view));
    memset(&s_anim, 0, sizeof(s_anim));
    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_init(&s_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_view);
    egui_animation_translate_init(EGUI_ANIM_OF(&s_anim));
    egui_animation_translate_params_set(&s_anim, &s_params);
    egui_animation_target_view_set(EGUI_ANIM_OF(&s_anim), &s_view);
}

/* Default repeat mode after init is 0 (RESTART). */
static void test_anim_repeat_mode_get_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_animation_repeat_mode_get(EGUI_ANIM_OF(&s_anim)));
}

/* After setting REVERSE mode, getter returns REVERSE. */
static void test_anim_repeat_mode_get_reverse(void)
{
    setup();
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&s_anim), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ANIMATION_REPEAT_MODE_REVERSE, (int)egui_animation_repeat_mode_get(EGUI_ANIM_OF(&s_anim)));
}

/* After setting RESTART mode, getter returns RESTART. */
static void test_anim_repeat_mode_get_restart(void)
{
    setup();
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&s_anim), EGUI_ANIMATION_REPEAT_MODE_RESTART);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ANIMATION_REPEAT_MODE_RESTART, (int)egui_animation_repeat_mode_get(EGUI_ANIM_OF(&s_anim)));
}

/* NULL self returns 0 without crash. */
static void test_anim_repeat_mode_get_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_animation_repeat_mode_get(NULL));
}

void test_anim_repeat_mode_get_run(void)
{
    EGUI_TEST_SUITE_BEGIN(anim_repeat_mode_get);

    EGUI_TEST_RUN(test_anim_repeat_mode_get_default);
    EGUI_TEST_RUN(test_anim_repeat_mode_get_reverse);
    EGUI_TEST_RUN(test_anim_repeat_mode_get_restart);
    EGUI_TEST_RUN(test_anim_repeat_mode_get_null_self);

    EGUI_TEST_SUITE_END();
}
