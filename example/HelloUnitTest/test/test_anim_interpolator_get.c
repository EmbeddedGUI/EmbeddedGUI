#include <string.h>

#include "egui.h"
#include "anim/egui_animation_translate.h"
#include "anim/egui_interpolator_linear.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_anim_interpolator_get.h"

static egui_animation_translate_t s_anim;
static egui_view_t s_view;
static egui_view_group_t s_parent;
static egui_interpolator_linear_t s_interp;

static const egui_animation_translate_params_t s_params = {.from_x = 0, .to_x = 10, .from_y = 0, .to_y = 0};

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    egui_slist_init(&core->scene.anims);
    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_view, 0, sizeof(s_view));
    memset(&s_anim, 0, sizeof(s_anim));
    memset(&s_interp, 0, sizeof(s_interp));
    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_init(&s_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_view);
    egui_animation_translate_init(EGUI_ANIM_OF(&s_anim));
    egui_animation_translate_params_set(&s_anim, &s_params);
    egui_animation_target_view_set(EGUI_ANIM_OF(&s_anim), &s_view);
    egui_interpolator_linear_init(EGUI_CAST_TO(egui_interpolator_t, &s_interp));
}

/* Default interpolator after plain init is NULL. */
static void test_anim_interpolator_get_default(void)
{
    setup();
    EGUI_TEST_ASSERT_NULL(egui_animation_interpolator_get(EGUI_ANIM_OF(&s_anim)));
}

/* After set, getter returns the same pointer. */
static void test_anim_interpolator_get_after_set(void)
{
    setup();
    egui_animation_interpolator_set(EGUI_ANIM_OF(&s_anim), EGUI_CAST_TO(egui_interpolator_t, &s_interp));
    EGUI_TEST_ASSERT_TRUE(egui_animation_interpolator_get(EGUI_ANIM_OF(&s_anim)) == EGUI_CAST_TO(egui_interpolator_t, &s_interp));
}

/* Clearing to NULL works. */
static void test_anim_interpolator_get_clear(void)
{
    setup();
    egui_animation_interpolator_set(EGUI_ANIM_OF(&s_anim), EGUI_CAST_TO(egui_interpolator_t, &s_interp));
    egui_animation_interpolator_set(EGUI_ANIM_OF(&s_anim), NULL);
    EGUI_TEST_ASSERT_NULL(egui_animation_interpolator_get(EGUI_ANIM_OF(&s_anim)));
}

/* NULL self returns NULL without crash. */
static void test_anim_interpolator_get_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_animation_interpolator_get(NULL));
}

void test_anim_interpolator_get_run(void)
{
    EGUI_TEST_SUITE_BEGIN(anim_interpolator_get);

    EGUI_TEST_RUN(test_anim_interpolator_get_default);
    EGUI_TEST_RUN(test_anim_interpolator_get_after_set);
    EGUI_TEST_RUN(test_anim_interpolator_get_clear);
    EGUI_TEST_RUN(test_anim_interpolator_get_null_self);

    EGUI_TEST_SUITE_END();
}
