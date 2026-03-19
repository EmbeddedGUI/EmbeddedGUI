

#include "image/egui_image_std.h"
#include "app_egui_resource_generate.h"

// clang-format off



/**
 * Image File : star.png
 * Format : rgb565
 * Alpha Type : 8
 * Re-sized : True
 * Rotation : 0.0
 * options: -i star.png -n test_perf_240_ext_a8 -f rgb565 -a 8 -s 0 -ext 1 -d 240 240
 */





static const egui_image_std_info_t egui_res_image_test_perf_240_ext_a8_rgb565_8_bin_info = {
    .data_buf = (void *)EGUI_EXT_RES_ID_EGUI_RES_IMAGE_TEST_PERF_240_EXT_A8_RGB565_8_DATA,
    .alpha_buf = (void *)EGUI_EXT_RES_ID_EGUI_RES_IMAGE_TEST_PERF_240_EXT_A8_RGB565_8_ALPHA,
    .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
    .alpha_type = EGUI_IMAGE_ALPHA_TYPE_8,
    .res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
    .width = 240,
    .height = 240,
};

extern const egui_image_std_t egui_res_image_test_perf_240_ext_a8_rgb565_8_bin;
EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_std_t, egui_res_image_test_perf_240_ext_a8_rgb565_8_bin, &egui_res_image_test_perf_240_ext_a8_rgb565_8_bin_info);




// clang-format on


