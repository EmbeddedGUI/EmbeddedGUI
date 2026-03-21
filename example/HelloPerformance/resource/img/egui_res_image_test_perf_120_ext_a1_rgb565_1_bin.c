

#include "image/egui_image_std.h"
#include "app_egui_resource_generate.h"

// clang-format off



/**
 * Image File : star.png
 * Format : rgb565
 * Alpha Type : 1
 * Re-sized : True
 * Rotation : 0.0
 * options: -i star.png -n test_perf_120_ext_a1 -f rgb565 -a 1 -s 0 -ext 1 -d 120 120
 */





static const egui_image_std_info_t egui_res_image_test_perf_120_ext_a1_rgb565_1_bin_info = {
    .data_buf = (void *)EGUI_EXT_RES_ID_EGUI_RES_IMAGE_TEST_PERF_120_EXT_A1_RGB565_1_DATA,
    .alpha_buf = (void *)EGUI_EXT_RES_ID_EGUI_RES_IMAGE_TEST_PERF_120_EXT_A1_RGB565_1_ALPHA,
    .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
    .alpha_type = EGUI_IMAGE_ALPHA_TYPE_1,
    .res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
    .opaque_alpha_hint = EGUI_IMAGE_OPAQUE_ALPHA_HINT_NON_OPAQUE,
    .width = 120,
    .height = 120,
};

extern const egui_image_std_t egui_res_image_test_perf_120_ext_a1_rgb565_1_bin;
EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_std_t, egui_res_image_test_perf_120_ext_a1_rgb565_1_bin, &egui_res_image_test_perf_120_ext_a1_rgb565_1_bin_info);




// clang-format on


