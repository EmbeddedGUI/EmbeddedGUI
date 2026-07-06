

#include "font/egui_font_std.h"
#include "app_egui_resource_generate.h"

// clang-format off



/**
 * Font size: 16
 * Font bit size: 8
 * TTF file: Montserrat-Medium.ttf
 * options: -i Montserrat-Medium.ttf -n test -p 16 -s 8 -ext 1 --external-code 1 -t supported_text_test.txt
 */


/**
 * Total character count: 93 Support Text: 
abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789(_+=./-:;@%!#',?)<>"*&^$|\[]{}
 */




static const egui_font_std_info_t egui_res_font_test_16_8_bin_info = {
    .font_size = 16,
    .font_bit_mode = 8,
    .height = 21,
    .res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
    .code_res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
    .bitmap_codec = EGUI_FONT_STD_BITMAP_CODEC_RAW,
    .count = 93,
    .code_array = (void *)EGUI_EXT_RES_ID_EGUI_RES_FONT_TEST_16_8_CODE_DESC,
    .char_array = (void *)EGUI_EXT_RES_ID_EGUI_RES_FONT_TEST_16_8_CHAR_DESC,
    .pixel_buffer = (void *)EGUI_EXT_RES_ID_EGUI_RES_FONT_TEST_16_8_PIXEL_BUFFER,
};

extern const egui_font_std_t egui_res_font_test_16_8_bin;
EGUI_FONT_SUB_DEFINE_CONST(egui_font_std_t, egui_res_font_test_16_8_bin, &egui_res_font_test_16_8_bin_info);




// clang-format on


