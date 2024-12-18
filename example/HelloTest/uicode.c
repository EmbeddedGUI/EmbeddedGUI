#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

#include "egui_view_test.h"

// views in test_view_group_1
static egui_view_test_t test_view;
static egui_view_group_t test_view_group_1;
static egui_view_button_t button_1;
static egui_view_button_t button_2;
static egui_view_image_t image_1;

// views in test_view_group_1
static egui_view_test_t test_view_1;
static egui_view_label_t label_1;
static egui_view_label_t label_2;

extern const LATTICE_FONT_INFO Consolas_19;
EGUI_FONT_SUB_DEFINE(egui_font_lattice_t, font_consolas, &Consolas_19);

EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(bg_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 30);
EGUI_BACKGROUND_PARAM_INIT(bg_params, &bg_param_normal, NULL, NULL);
static egui_background_color_t bg;

extern const egui_image_std_t egui_res_image_star_rgb32_8;
extern const egui_image_std_t egui_res_image_star_rgb565_1;
extern const egui_image_std_t egui_res_image_star_rgb565_2;
extern const egui_image_std_t egui_res_image_star_rgb565_4;
extern const egui_image_std_t egui_res_image_star_rgb565_8;

extern const egui_image_std_t egui_res_image_test_rgb565_8;
extern const egui_image_std_t egui_res_image_test_rgb32_8;

EGUI_BACKGROUND_IMAGE_PARAM_INIT(bg_img_param_normal, (egui_image_t *)&egui_res_image_test_rgb565_8);
EGUI_BACKGROUND_IMAGE_PARAM_INIT(bg_img_param_pressed, (egui_image_t *)&egui_res_image_star_rgb565_8);
EGUI_BACKGROUND_PARAM_INIT(bg_img_params, &bg_img_param_normal, &bg_img_param_pressed, NULL);
static egui_background_image_t bg_img;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_button_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_button_param_pressed, EGUI_COLOR_GREEN, EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_button_param_disabled, EGUI_COLOR_RED, EGUI_ALPHA_100, 5);
EGUI_BACKGROUND_PARAM_INIT(bg_button_params, &bg_button_param_normal, &bg_button_param_pressed, &bg_button_param_disabled);
static egui_background_color_t bg_button;

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

static const egui_animation_handle_t anim_1_hanlde = {
        .start = on_animation_start,
        .end = on_animation_end,
        .repeat = on_animation_repeat,
};

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_1_param, 0, 50, 0, 50);
egui_animation_translate_t anim_1;

// EGUI_ANIMATION_ALPHA_PARAMS_INIT(anim_1_param, EGUI_ALPHA_0, EGUI_ALPHA_100);
// egui_animation_alpha_t anim_1;

// EGUI_ANIMATION_SCALE_SIZE_PARAMS_INIT(anim_1_param, EGUI_FLOAT_VALUE(0.5f), EGUI_FLOAT_VALUE(1.0f));
// egui_animation_scale_size_t anim_1;

// egui_interpolator_accelerate_t anim_1_interpolator;
// egui_interpolator_accelerate_decelerate_t anim_1_interpolator;
// egui_interpolator_anticipate_t anim_1_interpolator;
// egui_interpolator_anticipate_overshoot_t anim_1_interpolator;
// egui_interpolator_bounce_t anim_1_interpolator;
// egui_interpolator_decelerate_t anim_1_interpolator;
egui_interpolator_overshoot_t anim_1_interpolator;

static void on_animation_set_start(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_set_start\n");
}

static void on_animation_set_end(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_set_end\n");
}

static void on_animation_set_repeat(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_set_repeat\n");
}

static const egui_animation_handle_t anim_set_hanlde = {
        .start = on_animation_set_start,
        .end = on_animation_set_end,
        .repeat = on_animation_set_repeat,
};

egui_animation_set_t anim_set;

static void on_animation_translate_start(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_translate_start\n");
}

static void on_animation_translate_end(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_translate_end\n");
}

static void on_animation_translate_repeat(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_translate_repeat\n");
}

static const egui_animation_handle_t anim_translate_hanlde = {
        .start = on_animation_translate_start,
        .end = on_animation_translate_end,
        .repeat = on_animation_translate_repeat,
};

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_translate_param, 0, 50, 0, 50);
egui_animation_translate_t anim_translate;

static void on_animation_alpha_start(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_alpha_start\n");
}

static void on_animation_alpha_end(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_alpha_end\n");
}

static void on_animation_alpha_repeat(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_alpha_repeat\n");
}

static const egui_animation_handle_t anim_alpha_hanlde = {
        .start = on_animation_alpha_start,
        .end = on_animation_alpha_end,
        .repeat = on_animation_alpha_repeat,
};

EGUI_ANIMATION_ALPHA_PARAMS_INIT(anim_alpha_param, EGUI_ALPHA_0, EGUI_ALPHA_100);
egui_animation_alpha_t anim_alpha;

static void on_animation_scale_size_start(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_scale_size_start\n");
}

static void on_animation_scale_size_end(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_scale_size_end\n");
}

static void on_animation_scale_size_repeat(egui_animation_t *self)
{
    EGUI_LOG_INF("on_animation_scale_size_repeat\n");
}

static const egui_animation_handle_t anim_scale_size_hanlde = {
        .start = on_animation_scale_size_start,
        .end = on_animation_scale_size_end,
        .repeat = on_animation_scale_size_repeat,
};

EGUI_ANIMATION_SCALE_SIZE_PARAMS_INIT(anim_scale_size_param, EGUI_FLOAT_VALUE(0.5f), EGUI_FLOAT_VALUE(1.0f));
egui_animation_scale_size_t anim_scale_size;

static uint32_t cnt = 1;
static char button_str[20] = "Click me!";
static void button_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("Clicked\n");

    egui_api_sprintf(button_str, "Clicked %ds", cnt);
    EGUI_LOG_INF("button_str: %s\n", button_str);

    egui_view_label_set_text((egui_view_t *)self, button_str);
    cnt++;
}

static uint32_t cnt_2 = 1;
static char button_str_2[20] = "Img";
static void button_2_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("Clicked 2\n");

    egui_api_sprintf(button_str_2, "Img %ds", cnt_2);
    EGUI_LOG_INF("button_str_2: %s\n", button_str_2);

    egui_view_label_set_text((egui_view_t *)self, button_str_2);
    cnt_2++;
}

static egui_location_t last_press_location;
int test_on_touch_event_cb(egui_view_t *self, egui_motion_event_t *event)
{
    egui_dim_t df_x, df_y;
    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_UP:
        break;
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        last_press_location.x = event->location.x;
        last_press_location.y = event->location.y;
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        df_x = event->location.x - last_press_location.x;
        df_y = event->location.y - last_press_location.y;
        egui_view_scroll_by(self, df_x, df_y);

        last_press_location.x = event->location.x;
        last_press_location.y = event->location.y;
        break;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        break;
    default:
        break;
    }

    return 1;
}

static void test_animation_basic(void)
{
    // anim_1
    egui_animation_translate_init((egui_animation_t *)&anim_1);
    egui_animation_translate_params_set(&anim_1, &anim_1_param);

    // egui_animation_alpha_init((egui_animation_t *)&anim_1);
    // egui_animation_alpha_params_set(&anim_1, &anim_1_param);

    // egui_animation_scale_size_init((egui_animation_t *)&anim_1);
    // egui_animation_scale_size_params_set(&anim_1, &anim_1_param);

    // egui_animation_target_view_set((egui_animation_t *)&anim_1, (egui_view_t *)&test_view_1);
    egui_animation_target_view_set((egui_animation_t *)&anim_1, (egui_view_t *)&test_view_group_1);
    // egui_animation_target_view_set((egui_animation_t *)&anim_1, (egui_view_t *)&button_1);

    egui_animation_repeat_count_set((egui_animation_t *)&anim_1, 4);
    egui_animation_repeat_mode_set((egui_animation_t *)&anim_1, EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_animation_handle_set((egui_animation_t *)&anim_1, &anim_1_hanlde);
    egui_animation_duration_set((egui_animation_t *)&anim_1, 1000);
    // egui_animation_is_fill_before_set((egui_animation_t *)&anim_1, true);

    // egui_interpolator_accelerate_init((egui_interpolator_t *)&anim_1_interpolator);
    // // egui_interpolator_accelerate_factor_set((egui_interpolator_t *)&anim_1_interpolator, EGUI_FLOAT_VALUE(0.2f));

    // egui_interpolator_accelerate_decelerate_init((egui_interpolator_t *)&anim_1_interpolator);

    // egui_interpolator_anticipate_init((egui_interpolator_t *)&anim_1_interpolator);

    // egui_interpolator_anticipate_overshoot_init((egui_interpolator_t *)&anim_1_interpolator);

    // egui_interpolator_bounce_init((egui_interpolator_t *)&anim_1_interpolator);

    // egui_interpolator_cycle_init((egui_interpolator_t *)&anim_1_interpolator);

    // egui_interpolator_decelerate_init((egui_interpolator_t *)&anim_1_interpolator);

    egui_interpolator_overshoot_init((egui_interpolator_t *)&anim_1_interpolator);

    egui_animation_interpolator_set((egui_animation_t *)&anim_1, (egui_interpolator_t *)&anim_1_interpolator);

    egui_animation_start((egui_animation_t *)&anim_1);
}

static void test_animation_set(void)
{

    // anim_translate
    egui_animation_translate_init((egui_animation_t *)&anim_translate);
    egui_animation_translate_params_set(&anim_translate, &anim_translate_param);
    egui_animation_repeat_count_set((egui_animation_t *)&anim_translate, 4);
    egui_animation_repeat_mode_set((egui_animation_t *)&anim_translate, EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_animation_handle_set((egui_animation_t *)&anim_translate, &anim_translate_hanlde);
    egui_animation_duration_set((egui_animation_t *)&anim_translate, 1000);

    egui_animation_alpha_init((egui_animation_t *)&anim_alpha);
    egui_animation_alpha_params_set(&anim_alpha, &anim_alpha_param);
    egui_animation_repeat_count_set((egui_animation_t *)&anim_alpha, 4);
    egui_animation_repeat_mode_set((egui_animation_t *)&anim_alpha, EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_animation_handle_set((egui_animation_t *)&anim_alpha, &anim_alpha_hanlde);
    egui_animation_duration_set((egui_animation_t *)&anim_alpha, 1000);

    egui_animation_scale_size_init((egui_animation_t *)&anim_scale_size);
    egui_animation_scale_size_params_set(&anim_scale_size, &anim_scale_size_param);
    egui_animation_repeat_count_set((egui_animation_t *)&anim_scale_size, 4);
    egui_animation_repeat_mode_set((egui_animation_t *)&anim_scale_size, EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_animation_handle_set((egui_animation_t *)&anim_scale_size, &anim_scale_size_hanlde);
    egui_animation_duration_set((egui_animation_t *)&anim_scale_size, 1000);

    egui_animation_target_view_set((egui_animation_t *)&anim_translate, (egui_view_t *)&test_view_group_1);
    egui_animation_target_view_set((egui_animation_t *)&anim_alpha, (egui_view_t *)&test_view_group_1);
    egui_animation_target_view_set((egui_animation_t *)&anim_scale_size, (egui_view_t *)&test_view_group_1);

    egui_animation_set_init((egui_animation_t *)&anim_set);
    egui_animation_handle_set((egui_animation_t *)&anim_set, &anim_set_hanlde);
    egui_animation_set_add_animation(&anim_set, (egui_animation_t *)&anim_translate);
    egui_animation_set_add_animation(&anim_set, (egui_animation_t *)&anim_alpha);
    egui_animation_set_add_animation(&anim_set, (egui_animation_t *)&anim_scale_size);
    egui_animation_target_view_set((egui_animation_t *)&anim_set, (egui_view_t *)&test_view_group_1);

    egui_animation_start((egui_animation_t *)&anim_set);
}

static void test_animation_set_1(void)
{
    // anim_translate
    egui_animation_translate_init((egui_animation_t *)&anim_translate);
    egui_animation_translate_params_set(&anim_translate, &anim_translate_param);

    egui_animation_alpha_init((egui_animation_t *)&anim_alpha);
    egui_animation_alpha_params_set(&anim_alpha, &anim_alpha_param);

    egui_animation_scale_size_init((egui_animation_t *)&anim_scale_size);
    egui_animation_scale_size_params_set(&anim_scale_size, &anim_scale_size_param);

    egui_animation_set_init((egui_animation_t *)&anim_set);
    egui_animation_repeat_count_set((egui_animation_t *)&anim_set, 3);
    egui_animation_repeat_mode_set((egui_animation_t *)&anim_set, EGUI_ANIMATION_REPEAT_MODE_REVERSE);
    egui_animation_handle_set((egui_animation_t *)&anim_set, &anim_set_hanlde);
    egui_animation_duration_set((egui_animation_t *)&anim_set, 1000);

    egui_animation_target_view_set((egui_animation_t *)&anim_set, (egui_view_t *)&test_view_group_1);

    egui_animation_set_set_mask(&anim_set, 1, 1, 1, 1, 1);

    // egui_interpolator_overshoot_init((egui_interpolator_t *)&anim_1_interpolator);
    // egui_animation_interpolator_set((egui_animation_t *)&anim_set, (egui_interpolator_t *)&anim_1_interpolator);

    // egui_animation_set_add_animation(&anim_set, (egui_animation_t *)&anim_translate);
    egui_animation_set_add_animation(&anim_set, (egui_animation_t *)&anim_alpha);
    egui_animation_set_add_animation(&anim_set, (egui_animation_t *)&anim_scale_size);

    egui_animation_start((egui_animation_t *)&anim_set);
}

void uicode_init_ui(void)
{
    // Init all views
    // test_view
    egui_view_test_init((egui_view_t *)&test_view);
    egui_view_set_position((egui_view_t *)&test_view, 0, 0);
    egui_view_set_size((egui_view_t *)&test_view, 200, 200);
    egui_view_set_on_touch_listener((egui_view_t *)&test_view, test_on_touch_event_cb);

    // test_view_group_1
    egui_view_group_init((egui_view_t *)&test_view_group_1);
    egui_view_set_position((egui_view_t *)&test_view_group_1, 50, 50);
    egui_view_set_size((egui_view_t *)&test_view_group_1, 150, 150);

    // button_1
    egui_view_button_init((egui_view_t *)&button_1);
    egui_view_set_position((egui_view_t *)&button_1, 10, 200);
    egui_view_set_size((egui_view_t *)&button_1, 150, 30);
    egui_view_label_set_text((egui_view_t *)&button_1, button_str);
    egui_view_label_set_align_type((egui_view_t *)&button_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&button_1, (egui_font_t *)&egui_res_font_montserrat_18_4);
    egui_view_label_set_font_color((egui_view_t *)&button_1, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
    egui_view_set_on_click_listener((egui_view_t *)&button_1, button_click_cb);

    // button_2
    egui_view_button_init((egui_view_t *)&button_2);
    egui_view_set_position((egui_view_t *)&button_2, 150, 200);
    egui_view_set_size((egui_view_t *)&button_2, 60, 60);
    egui_view_label_set_text((egui_view_t *)&button_2, button_str_2);
    egui_view_label_set_align_type((egui_view_t *)&button_2, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&button_2, (egui_font_t *)&egui_res_font_montserrat_14_4);
    egui_view_label_set_font_color((egui_view_t *)&button_2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_set_on_click_listener((egui_view_t *)&button_2, button_2_click_cb);

    // image_1
    extern const egui_image_std_t egui_res_image_test_rgb565_8;
    egui_view_image_init((egui_view_t *)&image_1);
    egui_view_set_position((egui_view_t *)&image_1, 150, 10);
    egui_view_set_size((egui_view_t *)&image_1, 60, 60);
    egui_view_image_set_image_type((egui_view_t *)&image_1, EGUI_VIEW_IMAGE_TYPE_RESIZE);
    egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_test_rgb565_8);

    // test_view_1
    egui_view_test_init((egui_view_t *)&test_view_1);
    egui_view_set_position((egui_view_t *)&test_view_1, 0, 0);
    egui_view_set_size((egui_view_t *)&test_view_1, 150, 150);

    // label_1
    egui_view_label_init((egui_view_t *)&label_1);
    egui_view_set_position((egui_view_t *)&label_1, 10, 0);
    egui_view_set_size((egui_view_t *)&label_1, 100, 100);
    egui_view_label_set_text((egui_view_t *)&label_1, "Hello!");
    egui_view_label_set_align_type((egui_view_t *)&label_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_1, (egui_font_t *)&font_consolas);
    egui_view_label_set_font_color((egui_view_t *)&label_1, EGUI_COLOR_RED, EGUI_ALPHA_100);

    // label_2
    egui_view_label_init((egui_view_t *)&label_2);
    egui_view_set_position((egui_view_t *)&label_2, 10, 50);
    egui_view_set_size((egui_view_t *)&label_2, 150, 50);
    egui_view_label_set_text((egui_view_t *)&label_2, "Test!");
    egui_view_label_set_align_type((egui_view_t *)&label_2, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_2, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_2, EGUI_COLOR_RED, EGUI_ALPHA_100);

    // bg
    egui_background_color_init((egui_background_t *)&bg);
    egui_background_set_params((egui_background_t *)&bg, &bg_params);
    egui_view_set_background((egui_view_t *)&label_1, (egui_background_t *)&bg);

    // bg_img
    egui_background_image_init((egui_background_t *)&bg_img);
    egui_background_set_params((egui_background_t *)&bg_img, &bg_img_params);
    egui_view_set_background((egui_view_t *)&button_2, (egui_background_t *)&bg_img);

    // bg_button
    egui_background_color_init((egui_background_t *)&bg_button);
    egui_background_set_params((egui_background_t *)&bg_button, &bg_button_params);
    egui_view_set_background((egui_view_t *)&button_1, (egui_background_t *)&bg_button);

    // Disable Button, for test
    // egui_view_set_enable((egui_view_t *)&button_1, false);

    // test_animation_basic();
    // test_animation_set();
    test_animation_set_1();

    // Add To Group
    egui_view_group_add_child((egui_view_t *)&test_view_group_1, (egui_view_t *)&test_view_1);
    egui_view_group_add_child((egui_view_t *)&test_view_group_1, (egui_view_t *)&label_1);
    egui_view_group_add_child((egui_view_t *)&test_view_group_1, (egui_view_t *)&label_2);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&test_view);
    egui_core_add_user_root_view((egui_view_t *)&test_view_group_1);
    egui_core_add_user_root_view((egui_view_t *)&button_1);
    egui_core_add_user_root_view((egui_view_t *)&button_2);
    egui_core_add_user_root_view((egui_view_t *)&image_1);
}

static egui_timer_t ui_timer;
void egui_view_test_timer_callback(egui_timer_t *timer)
{
    // test scroll
    // egui_view_scroll_to((egui_view_t*)&test_view, 50, 10);
    // egui_view_scroll_by((egui_view_t*)&test_view, 50, 50);

    // egui_view_scroll_by((egui_view_t *)&test_view_group_1, 5, 5);

    // egui_view_set_visible((egui_view_t *)&test_view_1, !egui_view_get_visible((egui_view_t *)&test_view_1));

    // egui_view_set_enable((egui_view_t *)&button_1, !egui_view_get_enable((egui_view_t *)&button_1));

    // egui_view_set_alpha((egui_view_t *)&test_view_group_1, EGUI_ALPHA_50);

    // egui_view_set_alpha((egui_view_t *)&test_view_1, EGUI_ALPHA_50);
}

void uicode_create_ui(void)
{
    uicode_init_ui();

    ui_timer.callback = egui_view_test_timer_callback;
    egui_timer_start_timer(&ui_timer, 1000, 1000);
}
