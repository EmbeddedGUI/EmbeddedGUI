#include "egui.h"
#include "font/egui_font_std.h"
#include "test/egui_test.h"
#include "test_font_std.h"

#include <string.h>

#define TEST_FONT_CANVAS_W 16
#define TEST_FONT_CANVAS_H 16

static egui_color_int_t test_font_pfb[TEST_FONT_CANVAS_W * TEST_FONT_CANVAS_H];
static egui_color_int_t expected_font_pfb[TEST_FONT_CANVAS_W * TEST_FONT_CANVAS_H];

static const egui_font_std_code_descriptor_t test_font_code_array[] = {
        {.code = 'A'},
        {.code = 'B'},
};

static const egui_font_std_char_descriptor_t test_font_char_array_raw[] = {
        {.idx = 0, .size = 2, .box_w = 2, .box_h = 2, .adv = 2, .off_x = 0, .off_y = 0},
        {.idx = 2, .size = 4, .box_w = 3, .box_h = 2, .adv = 3, .off_x = 0, .off_y = 0},
};

static const egui_font_std_char_descriptor_t test_font_char_array_rle4[] = {
        {.idx = 0, .size = 2, .box_w = 2, .box_h = 2, .adv = 2, .off_x = 0, .off_y = 0},
        {.idx = 2, .size = 4, .box_w = 3, .box_h = 2, .adv = 3, .off_x = 0, .off_y = 0},
};

static const egui_font_std_char_descriptor_t test_font_char_array_rle4xor[] = {
        {.idx = 0, .size = 3, .box_w = 2, .box_h = 2, .adv = 2, .off_x = 0, .off_y = 0},
        {.idx = 3, .size = 4, .box_w = 3, .box_h = 2, .adv = 3, .off_x = 0, .off_y = 0},
};

static const egui_font_std_char_descriptor_t test_font_char_array_invalid_offscreen[] = {
        {.idx = 0, .size = 1, .box_w = 2, .box_h = 2, .adv = 2, .off_x = 32, .off_y = 0},
};

static const uint8_t test_font_pixel_buffer_raw[] = {
        0xFF, 0xFF, 0x0F, 0x0F, 0xF0, 0x00,
};

static const uint8_t test_font_pixel_buffer_rle4[] = {
        0x83, 0x0F, 0x05, 0x0F, 0x0F, 0x0F,
};

static const uint8_t test_font_pixel_buffer_rle4xor[] = {
        0x03, 0x0F, 0x0F, 0x05, 0xFF, 0x0F, 0xFF,
};

static const uint8_t test_font_pixel_buffer_invalid[] = {
        0x80,
};

static const egui_font_std_info_t test_font_info_raw = {
        .font_size = 2,
        .font_bit_mode = 4,
        .height = 2,
        .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
        .bitmap_codec = EGUI_FONT_STD_BITMAP_CODEC_RAW,
        .count = 2,
        .code_array = test_font_code_array,
        .char_array = test_font_char_array_raw,
        .pixel_buffer = test_font_pixel_buffer_raw,
};

static const egui_font_std_info_t test_font_info_rle4 = {
        .font_size = 2,
        .font_bit_mode = 4,
        .height = 2,
        .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
        .bitmap_codec = EGUI_FONT_STD_BITMAP_CODEC_RLE4,
        .count = 2,
        .code_array = test_font_code_array,
        .char_array = test_font_char_array_rle4,
        .pixel_buffer = test_font_pixel_buffer_rle4,
};

static const egui_font_std_info_t test_font_info_rle4xor = {
        .font_size = 2,
        .font_bit_mode = 4,
        .height = 2,
        .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
        .bitmap_codec = EGUI_FONT_STD_BITMAP_CODEC_RLE4_XOR,
        .count = 2,
        .code_array = test_font_code_array,
        .char_array = test_font_char_array_rle4xor,
        .pixel_buffer = test_font_pixel_buffer_rle4xor,
};

static const egui_font_std_info_t test_font_info_invalid_offscreen = {
        .font_size = 2,
        .font_bit_mode = 4,
        .height = 2,
        .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
        .bitmap_codec = EGUI_FONT_STD_BITMAP_CODEC_RLE4,
        .count = 1,
        .code_array = test_font_code_array,
        .char_array = test_font_char_array_invalid_offscreen,
        .pixel_buffer = test_font_pixel_buffer_invalid,
};

EGUI_FONT_SUB_DEFINE_STATIC(egui_font_std_t, test_font_raw, &test_font_info_raw);
EGUI_FONT_SUB_DEFINE_STATIC(egui_font_std_t, test_font_rle4, &test_font_info_rle4);
EGUI_FONT_SUB_DEFINE_STATIC(egui_font_std_t, test_font_rle4xor, &test_font_info_rle4xor);
EGUI_FONT_SUB_DEFINE_STATIC(egui_font_std_t, test_font_invalid_offscreen, &test_font_info_invalid_offscreen);

static void test_font_setup_canvas(void)
{
    egui_region_t pfb_region;
    egui_region_t base_region;

    egui_font_std_release_frame_cache();
    memset(test_font_pfb, 0, sizeof(test_font_pfb));
    egui_region_init(&pfb_region, 0, 0, TEST_FONT_CANVAS_W, TEST_FONT_CANVAS_H);
    egui_canvas_init(test_font_pfb, &pfb_region);
    egui_region_init(&base_region, 0, 0, TEST_FONT_CANVAS_W, TEST_FONT_CANVAS_H);
    egui_canvas_calc_work_region(&base_region);
}

static void test_font_draw_and_capture_char(const egui_font_t *font, const char *text)
{
    egui_region_t rect;

    test_font_setup_canvas();
    egui_region_init(&rect, 4, 5, 6, 4);
    egui_canvas_draw_text_in_rect(font, text, &rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void test_font_draw_and_capture_repeat_in_rect(const egui_font_t *font, const char *text)
{
    egui_region_t rect;

    test_font_setup_canvas();
    egui_region_init(&rect, 1, 1, 12, 6);
    egui_canvas_draw_text_in_rect(font, text, &rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void test_font_std_compressed_draw_matches_raw(void)
{
    test_font_draw_and_capture_char((const egui_font_t *)&test_font_raw, "A");
    memcpy(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb));

    test_font_draw_and_capture_char((const egui_font_t *)&test_font_rle4, "A");
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);

    test_font_draw_and_capture_char((const egui_font_t *)&test_font_rle4xor, "A");
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);
}

static void test_font_std_odd_width_compressed_draw_matches_raw(void)
{
    test_font_draw_and_capture_char((const egui_font_t *)&test_font_raw, "B");
    memcpy(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb));

    test_font_draw_and_capture_char((const egui_font_t *)&test_font_rle4, "B");
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);

    test_font_draw_and_capture_char((const egui_font_t *)&test_font_rle4xor, "B");
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);
}

static void test_font_std_compressed_size_matches_raw(void)
{
    egui_dim_t raw_w = 0;
    egui_dim_t raw_h = 0;
    egui_dim_t rle4_w = 0;
    egui_dim_t rle4_h = 0;
    egui_dim_t rle4xor_w = 0;
    egui_dim_t rle4xor_h = 0;

    test_font_raw.base.api->get_str_size((const egui_font_t *)&test_font_raw, "A", 0, 0, &raw_w, &raw_h);
    test_font_rle4.base.api->get_str_size((const egui_font_t *)&test_font_rle4, "A", 0, 0, &rle4_w, &rle4_h);
    test_font_rle4xor.base.api->get_str_size((const egui_font_t *)&test_font_rle4xor, "A", 0, 0, &rle4xor_w, &rle4xor_h);

    EGUI_TEST_ASSERT_EQUAL_INT((int)raw_w, (int)rle4_w);
    EGUI_TEST_ASSERT_EQUAL_INT((int)raw_h, (int)rle4_h);
    EGUI_TEST_ASSERT_EQUAL_INT((int)raw_w, (int)rle4xor_w);
    EGUI_TEST_ASSERT_EQUAL_INT((int)raw_h, (int)rle4xor_h);
}

static void test_font_std_offscreen_compressed_glyph_skips_decode(void)
{
    memset(expected_font_pfb, 0, sizeof(expected_font_pfb));
    test_font_setup_canvas();
    test_font_invalid_offscreen.base.api->draw_string((const egui_font_t *)&test_font_invalid_offscreen, "A", 0, 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);
}

static void test_font_std_repeated_compressed_draw_in_rect_matches_raw(void)
{
    static const char repeat_text[] = "ABAB\nBABA";

    test_font_draw_and_capture_repeat_in_rect((const egui_font_t *)&test_font_raw, repeat_text);
    memcpy(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb));

    test_font_draw_and_capture_repeat_in_rect((const egui_font_t *)&test_font_rle4, repeat_text);
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);

    test_font_draw_and_capture_repeat_in_rect((const egui_font_t *)&test_font_rle4xor, repeat_text);
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);
}

void test_font_std_run(void)
{
    EGUI_TEST_SUITE_BEGIN(font_std);
    EGUI_TEST_RUN(test_font_std_compressed_draw_matches_raw);
    EGUI_TEST_RUN(test_font_std_odd_width_compressed_draw_matches_raw);
    EGUI_TEST_RUN(test_font_std_compressed_size_matches_raw);
    EGUI_TEST_RUN(test_font_std_offscreen_compressed_glyph_skips_decode);
    EGUI_TEST_RUN(test_font_std_repeated_compressed_draw_in_rect_matches_raw);
    EGUI_TEST_SUITE_END();
}
