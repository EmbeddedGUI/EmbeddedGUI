

#include "font/egui_font_std.h"
#include "app_egui_resource_generate.h"

// clang-format off



/**
 * Font size: 16
 * Font bit size: 8
 * TTF file: test.ttf
 * options: -i test.ttf -n test -p 16 -s 8 -ext 1 -t supported_text_test.txt
 */


/**
 * Total character count: 93 Support Text: 
abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789(_+=./-:;@%!#',?)<>"*&^$|\[]{}
 */

static const egui_font_std_code_descriptor_t egui_res_font_test_16_8_bin_code_array[] = {

    {.code=0x00000020}, /* " " */
    {.code=0x00000021}, /* "!" */
    {.code=0x00000022}, /* """ */
    {.code=0x00000023}, /* "#" */
    {.code=0x00000024}, /* "$" */
    {.code=0x00000025}, /* "%" */
    {.code=0x00000026}, /* "&" */
    {.code=0x00000027}, /* "'" */
    {.code=0x00000028}, /* "(" */
    {.code=0x00000029}, /* ")" */
    {.code=0x0000002a}, /* "*" */
    {.code=0x0000002b}, /* "+" */
    {.code=0x0000002c}, /* "," */
    {.code=0x0000002d}, /* "-" */
    {.code=0x0000002e}, /* "." */
    {.code=0x0000002f}, /* "/" */
    {.code=0x00000030}, /* "0" */
    {.code=0x00000031}, /* "1" */
    {.code=0x00000032}, /* "2" */
    {.code=0x00000033}, /* "3" */
    {.code=0x00000034}, /* "4" */
    {.code=0x00000035}, /* "5" */
    {.code=0x00000036}, /* "6" */
    {.code=0x00000037}, /* "7" */
    {.code=0x00000038}, /* "8" */
    {.code=0x00000039}, /* "9" */
    {.code=0x0000003a}, /* ":" */
    {.code=0x0000003b}, /* ";" */
    {.code=0x0000003c}, /* "<" */
    {.code=0x0000003d}, /* "=" */
    {.code=0x0000003e}, /* ">" */
    {.code=0x0000003f}, /* "?" */
    {.code=0x00000040}, /* "@" */
    {.code=0x00000041}, /* "A" */
    {.code=0x00000042}, /* "B" */
    {.code=0x00000043}, /* "C" */
    {.code=0x00000044}, /* "D" */
    {.code=0x00000045}, /* "E" */
    {.code=0x00000046}, /* "F" */
    {.code=0x00000047}, /* "G" */
    {.code=0x00000048}, /* "H" */
    {.code=0x00000049}, /* "I" */
    {.code=0x0000004a}, /* "J" */
    {.code=0x0000004b}, /* "K" */
    {.code=0x0000004c}, /* "L" */
    {.code=0x0000004d}, /* "M" */
    {.code=0x0000004e}, /* "N" */
    {.code=0x0000004f}, /* "O" */
    {.code=0x00000050}, /* "P" */
    {.code=0x00000051}, /* "Q" */
    {.code=0x00000052}, /* "R" */
    {.code=0x00000053}, /* "S" */
    {.code=0x00000054}, /* "T" */
    {.code=0x00000055}, /* "U" */
    {.code=0x00000056}, /* "V" */
    {.code=0x00000057}, /* "W" */
    {.code=0x00000058}, /* "X" */
    {.code=0x00000059}, /* "Y" */
    {.code=0x0000005a}, /* "Z" */
    {.code=0x0000005b}, /* "[" */
    {.code=0x0000005c}, /* "\" */
    {.code=0x0000005d}, /* "]" */
    {.code=0x0000005e}, /* "^" */
    {.code=0x0000005f}, /* "_" */
    {.code=0x00000061}, /* "a" */
    {.code=0x00000062}, /* "b" */
    {.code=0x00000063}, /* "c" */
    {.code=0x00000064}, /* "d" */
    {.code=0x00000065}, /* "e" */
    {.code=0x00000066}, /* "f" */
    {.code=0x00000067}, /* "g" */
    {.code=0x00000068}, /* "h" */
    {.code=0x00000069}, /* "i" */
    {.code=0x0000006a}, /* "j" */
    {.code=0x0000006b}, /* "k" */
    {.code=0x0000006c}, /* "l" */
    {.code=0x0000006d}, /* "m" */
    {.code=0x0000006e}, /* "n" */
    {.code=0x0000006f}, /* "o" */
    {.code=0x00000070}, /* "p" */
    {.code=0x00000071}, /* "q" */
    {.code=0x00000072}, /* "r" */
    {.code=0x00000073}, /* "s" */
    {.code=0x00000074}, /* "t" */
    {.code=0x00000075}, /* "u" */
    {.code=0x00000076}, /* "v" */
    {.code=0x00000077}, /* "w" */
    {.code=0x00000078}, /* "x" */
    {.code=0x00000079}, /* "y" */
    {.code=0x0000007a}, /* "z" */
    {.code=0x0000007b}, /* "{" */
    {.code=0x0000007c}, /* "|" */
    {.code=0x0000007d}, /* "}" */
};



static const egui_font_std_info_t egui_res_font_test_16_8_bin_info = {
    .font_size = 16,
    .font_bit_mode = 8,
    .height = 21,
    .res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
    .count = 93,
    .code_array = egui_res_font_test_16_8_bin_code_array,
    .char_array = (void *)EGUI_EXT_RES_ID_EGUI_RES_FONT_TEST_16_8_CHAR_DESC,
    .pixel_buffer = (void *)EGUI_EXT_RES_ID_EGUI_RES_FONT_TEST_16_8_PIXEL_BUFFER,
};

extern const egui_font_std_t egui_res_font_test_16_8_bin;
EGUI_FONT_SUB_DEFINE_CONST(egui_font_std_t, egui_res_font_test_16_8_bin, &egui_res_font_test_16_8_bin_info);




// clang-format on


