

#include "image/egui_image_rle.h"
#include "app_egui_resource_generate.h"

// clang-format off



/**
 * Image File : test_img_2_1280_1280.png
 * Format : rgb565
 * Alpha Type : 0
 * Re-sized : True
 * Rotation : 0.0
 * options: -i test_img_2_1280_1280.png -n test_perf_120_ext -f rgb565 -a 0 -s 0 -ext 1 -d 120 120
 */





static const egui_image_rle_info_t egui_res_image_test_perf_120_ext_rle_rgb565_0_bin_info = {
    .data_buf = (void *)EGUI_EXT_RES_ID_EGUI_RES_IMAGE_TEST_PERF_120_EXT_RLE_RGB565_0_DATA,
    .alpha_buf = (void *)NULL,
    .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
    .alpha_type = EGUI_IMAGE_ALPHA_TYPE_1,
    .res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
    .width = 120,
    .height = 120,
    .data_size = 26792,
    .alpha_size = 0,
    .decompressed_size = 28800,
};

extern const egui_image_rle_t egui_res_image_test_perf_120_ext_rle_rgb565_0_bin;
EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_rle_t, egui_res_image_test_perf_120_ext_rle_rgb565_0_bin, &egui_res_image_test_perf_120_ext_rle_rgb565_0_bin_info);




// clang-format on


