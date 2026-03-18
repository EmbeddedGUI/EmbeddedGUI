

#include "image/egui_image_std.h"
#include "app_egui_resource_generate.h"

// clang-format off



/**
 * Image File : test_img_2_1280_1280.png
 * Format : rgb565
 * Alpha Type : 0
 * Re-sized : True
 * Rotation : 0.0
 * options: -i test_img_2_1280_1280.png -n test_perf_480_ext -f rgb565 -a 0 -s 0 -ext 1 -d 480 480
 */





static const egui_image_std_info_t egui_res_image_test_perf_480_ext_rgb565_0_bin_info = {
    .data_buf = (void *)EGUI_EXT_RES_ID_EGUI_RES_IMAGE_TEST_PERF_480_EXT_RGB565_0_DATA,
    .alpha_buf = (void *)NULL,
    .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
    .alpha_type = EGUI_IMAGE_ALPHA_TYPE_8,
    .res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
    .width = 480,
    .height = 480,
};

extern const egui_image_std_t egui_res_image_test_perf_480_ext_rgb565_0_bin;
EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_std_t, egui_res_image_test_perf_480_ext_rgb565_0_bin, &egui_res_image_test_perf_480_ext_rgb565_0_bin_info);




// clang-format on


