#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// views
static egui_view_animated_image_t anim_img;
static egui_view_label_t label_title;
static egui_timer_t anim_timer;

extern const egui_image_std_t egui_res_image_star_rgb565_8;
extern const egui_image_std_t egui_res_image_test_rgb565_8;

#define ANIM_IMG_WIDTH  96
#define ANIM_IMG_HEIGHT 96

// Frame array with 2 frames for visible animation
static const egui_image_t *frame_list[] = {
        (const egui_image_t *)&egui_res_image_star_rgb565_8,
        (const egui_image_t *)&egui_res_image_test_rgb565_8,
};

static void anim_timer_callback(egui_timer_t *timer)
{
    (void)timer;
    egui_view_animated_image_update(EGUI_VIEW_OF(&anim_img), 100);
}

// View params
EGUI_VIEW_ANIMATED_IMAGE_PARAMS_INIT(anim_img_params, 0, 0, ANIM_IMG_WIDTH, ANIM_IMG_HEIGHT, 100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_title_params, 0, 0, 200, 32, "AnimatedImage", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_PRIMARY_DARK, EGUI_ALPHA_100);

void test_init_ui(void)
{
    // Init label
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_title), &label_title_params);

    // Init animated image
    egui_view_animated_image_init_with_params(EGUI_VIEW_OF(&anim_img), &anim_img_params);
    egui_view_animated_image_set_frames(EGUI_VIEW_OF(&anim_img), frame_list, 2);
    egui_view_set_margin_all(EGUI_VIEW_OF(&anim_img), 6);
    egui_view_animated_image_set_loop(EGUI_VIEW_OF(&anim_img), 1);
    egui_view_animated_image_play(EGUI_VIEW_OF(&anim_img));

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_title));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&anim_img));

    // Layout childrens of root view
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);

    // Drive frame update by timer
    egui_timer_init_timer(&anim_timer, NULL, anim_timer_callback);
    egui_timer_start_timer(&anim_timer, 120, 120);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 1)
    {
        return false;
    }
    EGUI_SIM_SET_WAIT(p_action, 1500);
    return true;
}
#endif
