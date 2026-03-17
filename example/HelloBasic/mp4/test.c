#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

#include "app_egui_resource_generate.h"

// views in root
static egui_view_mp4_t view_mp4;

#define MP4_WIDTH  320
#define MP4_HEIGHT 240

// View params
EGUI_VIEW_MP4_PARAMS_INIT(view_mp4_params, 0, 0, MP4_WIDTH, MP4_HEIGHT);

#if defined(EGUI_PORT_PC)
#define MP4_IMAGE_COUNT_TEST 50
static const egui_image_t *mp4_arr_test[MP4_IMAGE_COUNT_TEST] = {
        (const egui_image_t *)&egui_res_image_test_frame_test_0001_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0002_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0003_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0004_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0005_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0006_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0007_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0008_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0009_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0010_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0011_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0012_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0013_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0014_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0015_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0016_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0017_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0018_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0019_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0020_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0021_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0022_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0023_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0024_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0025_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0026_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0027_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0028_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0029_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0030_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0031_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0032_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0033_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0034_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0035_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0036_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0037_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0038_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0039_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0040_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0041_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0042_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0043_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0044_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0045_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0046_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0047_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0048_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0049_rgb565_0, (const egui_image_t *)&egui_res_image_test_frame_test_0050_rgb565_0,
};
#else
#define MP4_IMAGE_COUNT_TEST 2
static const egui_image_t *mp4_arr_test[MP4_IMAGE_COUNT_TEST] = {
        (const egui_image_t *)&egui_res_image_test_frame_test_0001_rgb565_0,
        (const egui_image_t *)&egui_res_image_test_frame_test_0002_rgb565_0,
};
#endif

static void mp4_callback(egui_view_mp4_t *view, int is_end)
{
    EGUI_LOG_INF("MP4 Callback, is_end: %d\n", is_end);
    if (is_end)
    {
        // replay
        egui_view_mp4_start_work(EGUI_VIEW_OF(view), (1000 / 10));
    }
}

void test_init_ui(void)
{
    // Init all views
    // view_mp4
    egui_view_mp4_init_with_params(EGUI_VIEW_OF(&view_mp4), &view_mp4_params);
    view_mp4.mp4_image_list = mp4_arr_test;
    view_mp4.mp4_image_count = MP4_IMAGE_COUNT_TEST;
    view_mp4.callback = mp4_callback;

    // Work in 10 fps
    egui_view_mp4_start_work(EGUI_VIEW_OF(&view_mp4), (1000 / 10));

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&view_mp4));

    // Layout childrens of root view
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
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
