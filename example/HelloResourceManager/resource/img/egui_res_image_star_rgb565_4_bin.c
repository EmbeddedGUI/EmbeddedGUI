

#include "image/egui_image_std.h"
#include "app_egui_resource_generate.h"

// clang-format off



/**
 * Image File : star.png
 * Format : rgb565
 * Alpha Type : 4
 * Re-sized : False
 * Rotation : 0.0
 * options: -i star.png -n star -f rgb565 -a 4 -s 0 -ext 1 -s 0
 */





static const egui_image_std_info_t egui_res_image_star_rgb565_4_bin_info = {
    .data_buf = (void *)EGUI_EXT_RES_ID_EGUI_RES_IMAGE_STAR_RGB565_4_DATA,
    .alpha_buf = (void *)EGUI_EXT_RES_ID_EGUI_RES_IMAGE_STAR_RGB565_4_ALPHA,
    .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
    .alpha_type = EGUI_IMAGE_ALPHA_TYPE_4,
    .res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
    .width = 100,
    .height = 100,
};

extern const egui_image_std_t egui_res_image_star_rgb565_4_bin;
EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_std_t, egui_res_image_star_rgb565_4_bin, &egui_res_image_star_rgb565_4_bin_info);




// clang-format on

