#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// views in root
static egui_view_t view_1;

#define TEST_ANIMATION_DURATION 1000

#define TEST_RADIUS_1 10
EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(bg_1_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_100, TEST_RADIUS_1);
EGUI_BACKGROUND_PARAM_INIT(bg_1_params, &bg_1_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_1, &bg_1_params);

static void on_animation_start(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_start\n");
}

static void on_animation_end(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_end\n");
}

static void on_animation_repeat(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_repeat\n");
}

static const egui_animation_handle_t anim_hanlde = {
        .start = on_animation_start,
        .end = on_animation_end,
        .repeat = on_animation_repeat,
};

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_translate_param, 0, 0, 0, EGUI_CONFIG_SCEEN_HEIGHT - TEST_RADIUS_1*2);
egui_animation_translate_t anim_translate;

egui_interpolator_overshoot_t anim_overshoot_interpolator;
egui_interpolator_linear_t anim_linear_interpolator;

void test_init_ui(void)
{
    // Init all views
    // view_1
    egui_view_init((egui_view_t *)&view_1);
    egui_view_set_position((egui_view_t *)&view_1, EGUI_CONFIG_SCEEN_WIDTH/2 - TEST_RADIUS_1, 0);
    egui_view_set_size((egui_view_t *)&view_1, 2*TEST_RADIUS_1 + 1, 2*TEST_RADIUS_1 + 1);

    // background
    egui_view_set_background((egui_view_t *)&view_1, (egui_background_t *)&bg_1);

    // anim_translate
    egui_animation_translate_init((egui_animation_t *)&anim_translate);
    egui_animation_translate_params_set(&anim_translate, &anim_translate_param);

    egui_animation_duration_set((egui_animation_t *)&anim_translate, TEST_ANIMATION_DURATION);
    egui_animation_repeat_count_set((egui_animation_t *)&anim_translate, 1);
    egui_animation_repeat_mode_set((egui_animation_t *)&anim_translate, EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_animation_handle_set((egui_animation_t *)&anim_translate, &anim_hanlde);

    // interpolator set
    // egui_interpolator_overshoot_init((egui_interpolator_t *)&anim_overshoot_interpolator);
    // egui_animation_interpolator_set((egui_animation_t *)&anim_translate, (egui_interpolator_t *)&anim_overshoot_interpolator);

    egui_interpolator_linear_init((egui_interpolator_t *)&anim_linear_interpolator);
    egui_animation_interpolator_set((egui_animation_t *)&anim_translate, (egui_interpolator_t *)&anim_linear_interpolator);
    

    egui_animation_target_view_set((egui_animation_t *)&anim_translate, (egui_view_t *)&view_1);

    egui_animation_start((egui_animation_t *)&anim_translate);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&view_1);
}
