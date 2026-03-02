#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

#define TEST_ANIMATION_DURATION 1500

// ============================================================================
// View 1: Translate + Bounce interpolator (red circle, moves up/down)
// ============================================================================
#define VIEW1_RADIUS 10
static egui_view_t view_translate;

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(bg_translate_param, EGUI_COLOR_RED, EGUI_ALPHA_100, VIEW1_RADIUS);
EGUI_BACKGROUND_PARAM_INIT(bg_translate_params, &bg_translate_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_translate, &bg_translate_params);

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_translate_param, 0, 0, 0, EGUI_CONFIG_SCEEN_HEIGHT - VIEW1_RADIUS * 2);
static egui_animation_translate_t anim_translate;
static egui_interpolator_bounce_t interp_bounce;

// ============================================================================
// View 2: Alpha + Linear interpolator (green rect, fades in/out)
// ============================================================================
#define VIEW2_SIZE 40
static egui_view_t view_alpha;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_alpha_param, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_alpha_params, &bg_alpha_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha, &bg_alpha_params);

EGUI_ANIMATION_ALPHA_PARAMS_INIT(anim_alpha_param, EGUI_ALPHA_100, EGUI_ALPHA_0);
static egui_animation_alpha_t anim_alpha;
static egui_interpolator_linear_t interp_linear;

// ============================================================================
// View 3: Scale + Overshoot interpolator (blue circle, scales)
// ============================================================================
#define VIEW3_SIZE 30
static egui_view_t view_scale;

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(bg_scale_param, EGUI_COLOR_BLUE, EGUI_ALPHA_100, VIEW3_SIZE / 2);
EGUI_BACKGROUND_PARAM_INIT(bg_scale_params, &bg_scale_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_scale, &bg_scale_params);

EGUI_ANIMATION_SCALE_SIZE_PARAMS_INIT(anim_scale_param, EGUI_FLOAT_VALUE(0.3f), EGUI_FLOAT_VALUE(1.5f));
static egui_animation_scale_size_t anim_scale;
static egui_interpolator_overshoot_t interp_overshoot;

// ============================================================================
// View 4: AnimationSet (translate + alpha) + AccelerateDecelerate interpolator
// ============================================================================
#define VIEW4_SIZE 30
static egui_view_t view_set;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_set_param, EGUI_COLOR_ORANGE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_set_params, &bg_set_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_set, &bg_set_params);

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_set_translate_param, 0, 0, 0, EGUI_CONFIG_SCEEN_HEIGHT - VIEW4_SIZE);
static egui_animation_translate_t anim_set_translate;
EGUI_ANIMATION_ALPHA_PARAMS_INIT(anim_set_alpha_param, EGUI_ALPHA_100, EGUI_ALPHA_20);
static egui_animation_alpha_t anim_set_alpha;
static egui_animation_set_t anim_set;
static egui_interpolator_accelerate_decelerate_t interp_accel_decel;

// ============================================================================
// Animation callbacks
// ============================================================================
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

static const egui_animation_handle_t anim_handle = {
        .start = on_animation_start,
        .end = on_animation_end,
        .repeat = on_animation_repeat,
};

// ============================================================================
// Column layout helper
// ============================================================================
#define COL_COUNT 4
#define COL_WIDTH (EGUI_CONFIG_SCEEN_WIDTH / COL_COUNT)

void test_init_ui(void)
{
    // ---- View 1: Translate + Bounce ----
    egui_view_init(EGUI_VIEW_OF(&view_translate));
    egui_view_set_position(EGUI_VIEW_OF(&view_translate), COL_WIDTH * 0 + COL_WIDTH / 2 - VIEW1_RADIUS, 0);
    egui_view_set_size(EGUI_VIEW_OF(&view_translate), VIEW1_RADIUS * 2 + 1, VIEW1_RADIUS * 2 + 1);
    egui_view_set_background(EGUI_VIEW_OF(&view_translate), EGUI_BG_OF(&bg_translate));

    egui_animation_translate_init(EGUI_ANIM_OF(&anim_translate));
    egui_animation_translate_params_set(&anim_translate, &anim_translate_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_translate), TEST_ANIMATION_DURATION);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&anim_translate), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&anim_translate), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_interpolator_bounce_init((egui_interpolator_t *)&interp_bounce);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_translate), (egui_interpolator_t *)&interp_bounce);
    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_translate), EGUI_VIEW_OF(&view_translate));
    egui_animation_start(EGUI_ANIM_OF(&anim_translate));

    // ---- View 2: Alpha + Linear ----
    egui_view_init(EGUI_VIEW_OF(&view_alpha));
    egui_view_set_position(EGUI_VIEW_OF(&view_alpha), COL_WIDTH * 1 + COL_WIDTH / 2 - VIEW2_SIZE / 2, EGUI_CONFIG_SCEEN_HEIGHT / 2 - VIEW2_SIZE / 2);
    egui_view_set_size(EGUI_VIEW_OF(&view_alpha), VIEW2_SIZE, VIEW2_SIZE);
    egui_view_set_background(EGUI_VIEW_OF(&view_alpha), EGUI_BG_OF(&bg_alpha));

    egui_animation_alpha_init(EGUI_ANIM_OF(&anim_alpha));
    egui_animation_alpha_params_set(&anim_alpha, &anim_alpha_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_alpha), TEST_ANIMATION_DURATION);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&anim_alpha), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&anim_alpha), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_interpolator_linear_init((egui_interpolator_t *)&interp_linear);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_alpha), (egui_interpolator_t *)&interp_linear);
    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_alpha), EGUI_VIEW_OF(&view_alpha));
    egui_animation_start(EGUI_ANIM_OF(&anim_alpha));

    // ---- View 3: Scale + Overshoot ----
    egui_view_init(EGUI_VIEW_OF(&view_scale));
    egui_view_set_position(EGUI_VIEW_OF(&view_scale), COL_WIDTH * 2 + COL_WIDTH / 2 - VIEW3_SIZE / 2, EGUI_CONFIG_SCEEN_HEIGHT / 2 - VIEW3_SIZE / 2);
    egui_view_set_size(EGUI_VIEW_OF(&view_scale), VIEW3_SIZE + 1, VIEW3_SIZE + 1);
    egui_view_set_background(EGUI_VIEW_OF(&view_scale), EGUI_BG_OF(&bg_scale));

    egui_animation_scale_size_init(EGUI_ANIM_OF(&anim_scale));
    egui_animation_scale_size_params_set(&anim_scale, &anim_scale_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_scale), TEST_ANIMATION_DURATION);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&anim_scale), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&anim_scale), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_interpolator_overshoot_init((egui_interpolator_t *)&interp_overshoot);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_scale), (egui_interpolator_t *)&interp_overshoot);
    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_scale), EGUI_VIEW_OF(&view_scale));
    egui_animation_start(EGUI_ANIM_OF(&anim_scale));

    // ---- View 4: AnimationSet (translate + alpha) + AccelerateDecelerate ----
    egui_view_init(EGUI_VIEW_OF(&view_set));
    egui_view_set_position(EGUI_VIEW_OF(&view_set), COL_WIDTH * 3 + COL_WIDTH / 2 - VIEW4_SIZE / 2, 0);
    egui_view_set_size(EGUI_VIEW_OF(&view_set), VIEW4_SIZE, VIEW4_SIZE);
    egui_view_set_background(EGUI_VIEW_OF(&view_set), EGUI_BG_OF(&bg_set));

    // Sub-animation: translate
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_set_translate));
    egui_animation_translate_params_set(&anim_set_translate, &anim_set_translate_param);

    // Sub-animation: alpha
    egui_animation_alpha_init(EGUI_ANIM_OF(&anim_set_alpha));
    egui_animation_alpha_params_set(&anim_set_alpha, &anim_set_alpha_param);

    // Animation set
    egui_animation_set_init(EGUI_ANIM_OF(&anim_set));
    egui_animation_set_add_animation(&anim_set, EGUI_ANIM_OF(&anim_set_translate));
    egui_animation_set_add_animation(&anim_set, EGUI_ANIM_OF(&anim_set_alpha));
    egui_animation_set_set_mask(&anim_set, 1, 1, 1, 1, 1);

    egui_animation_duration_set(EGUI_ANIM_OF(&anim_set), TEST_ANIMATION_DURATION);
    egui_animation_repeat_count_set(EGUI_ANIM_OF(&anim_set), -1);
    egui_animation_repeat_mode_set(EGUI_ANIM_OF(&anim_set), EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_interpolator_accelerate_decelerate_init((egui_interpolator_t *)&interp_accel_decel);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_set), (egui_interpolator_t *)&interp_accel_decel);
    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_set), EGUI_VIEW_OF(&view_set));
    egui_animation_handle_set(EGUI_ANIM_OF(&anim_set), &anim_handle);
    egui_animation_start(EGUI_ANIM_OF(&anim_set));

    // Add all views to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&view_translate));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&view_alpha));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&view_scale));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&view_set));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    last_action = action_index;

    if (action_index >= 3)
    {
        return false;
    }
    if (first_call)
        recording_request_snapshot();
    EGUI_SIM_SET_WAIT(p_action, 200);
    return true;
}
#endif
