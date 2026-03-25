

#include "image/egui_image_rle.h"
#include "app_egui_resource_generate.h"

// clang-format off



/**
 * Image File : star.png
 * Format : rgb565
 * Alpha Type : 8
 * Re-sized : True
 * Rotation : 0.0
 * options: -i star.png -n star_120_ext -f rgb565 -a 8 -s 0 -ext 1 -d 120 120
 */





static const egui_image_rle_info_t egui_res_image_star_120_ext_rle_rgb565_8_bin_info = {
    .data_buf = (void *)EGUI_EXT_RES_ID_EGUI_RES_IMAGE_STAR_120_EXT_RLE_RGB565_8_DATA,
    .alpha_buf = (void *)EGUI_EXT_RES_ID_EGUI_RES_IMAGE_STAR_120_EXT_RLE_RGB565_8_ALPHA,
    .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
    .alpha_type = EGUI_IMAGE_ALPHA_TYPE_8,
    .res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
    .width = 120,
    .height = 120,
    .data_size = 3613,
    .alpha_size = 1584,
    .decompressed_size = 28800,
};

extern const egui_image_rle_t egui_res_image_star_120_ext_rle_rgb565_8_bin;
EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_rle_t, egui_res_image_star_120_ext_rle_rgb565_8_bin, &egui_res_image_star_120_ext_rle_rgb565_8_bin_info);




// clang-format on


