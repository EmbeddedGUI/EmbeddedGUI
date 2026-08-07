#include "egui.h"
#include "uicode_disp0.h"
#include "canvas/egui_canvas_compact.h"
#include "font/egui_font_std.h"
#include "port_api.h"
#include "test/egui_test.h"
#include "test_font_std.h"

#include <string.h>

#define TEST_FONT_CANVAS_W        16
#define TEST_FONT_CANVAS_H        16
#define TEST_FONT_STD_EXPECT_LOCK (EGUI_CONFIG_EXTERNAL_RESOURCE_SHARED_DISPLAY_BUS ? 1 : 0)

static egui_color_int_t test_font_pfb[TEST_FONT_CANVAS_W * TEST_FONT_CANVAS_H];
static egui_color_int_t expected_font_pfb[TEST_FONT_CANVAS_W * TEST_FONT_CANVAS_H];
static egui_canvas_t test_font_canvas;

typedef struct test_font_std_external_load_record
{
    int total_count;
    int code_count;
    int char_count;
    int pixel_count;
    int locked_count;
    int unlocked_count;
    egui_core_t *last_core;
} test_font_std_external_load_record_t;

static test_font_std_external_load_record_t s_font_std_external_load_record;

static egui_core_t *test_font_std_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

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

static const egui_font_std_info_t test_font_info_raw_tall = {
        .font_size = 8,
        .font_bit_mode = 4,
        .height = 8,
        .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
        .bitmap_codec = EGUI_FONT_STD_BITMAP_CODEC_RAW,
        .count = 2,
        .code_array = test_font_code_array,
        .char_array = test_font_char_array_raw,
        .pixel_buffer = test_font_pixel_buffer_raw,
};

static const egui_font_std_info_t test_font_info_external_raw = {
        .font_size = 2,
        .font_bit_mode = 4,
        .height = 2,
        .res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
        .code_res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
        .bitmap_codec = EGUI_FONT_STD_BITMAP_CODEC_RAW,
        .count = 2,
        .code_array = test_font_code_array,
        .char_array = test_font_char_array_raw,
        .pixel_buffer = test_font_pixel_buffer_raw,
};

EGUI_FONT_SUB_DEFINE_STATIC(egui_font_std_t, test_font_raw, &test_font_info_raw);
EGUI_FONT_SUB_DEFINE_STATIC(egui_font_std_t, test_font_rle4, &test_font_info_rle4);
EGUI_FONT_SUB_DEFINE_STATIC(egui_font_std_t, test_font_rle4xor, &test_font_info_rle4xor);
EGUI_FONT_SUB_DEFINE_STATIC(egui_font_std_t, test_font_invalid_offscreen, &test_font_info_invalid_offscreen);
EGUI_FONT_SUB_DEFINE_STATIC(egui_font_std_t, test_font_raw_tall, &test_font_info_raw_tall);
EGUI_FONT_SUB_DEFINE_STATIC(egui_font_std_t, test_font_external_raw, &test_font_info_external_raw);

static void test_font_std_external_load(egui_core_t *core, void *dest, egui_uintptr_t res_id, uint32_t start_offset, uint32_t size)
{
    s_font_std_external_load_record.total_count++;
    s_font_std_external_load_record.last_core = core;
    if (core != NULL && core->render.pfb_mgr.bus_locked)
    {
        s_font_std_external_load_record.locked_count++;
    }
    else
    {
        s_font_std_external_load_record.unlocked_count++;
    }

    if (res_id == (egui_uintptr_t)test_font_code_array)
    {
        s_font_std_external_load_record.code_count++;
    }
    else if (res_id == (egui_uintptr_t)test_font_char_array_raw)
    {
        s_font_std_external_load_record.char_count++;
    }
    else if (res_id == (egui_uintptr_t)test_font_pixel_buffer_raw)
    {
        s_font_std_external_load_record.pixel_count++;
    }

    if (dest != NULL && res_id != (egui_uintptr_t)0 && size > 0U)
    {
        const uint8_t *src = (const uint8_t *)res_id;
        memcpy(dest, src + start_offset, size);
    }
}

static egui_platform_ops_t s_font_std_external_platform_ops;

static void test_font_std_external_reset_load_record(void)
{
    memset(&s_font_std_external_load_record, 0, sizeof(s_font_std_external_load_record));
}

static const egui_platform_ops_t *test_font_std_external_install_platform(void)
{
    egui_platform_t *platform = egui_platform_get();
    const egui_platform_ops_t *old_ops;

    if (platform == NULL)
    {
        return NULL;
    }

    old_ops = platform->ops;
    test_font_std_external_reset_load_record();
    if (old_ops != NULL)
    {
        s_font_std_external_platform_ops = *old_ops;
    }
    else
    {
        memset(&s_font_std_external_platform_ops, 0, sizeof(s_font_std_external_platform_ops));
    }
    s_font_std_external_platform_ops.load_external_resource = test_font_std_external_load;
    platform->ops = &s_font_std_external_platform_ops;
    return old_ops;
}

static void test_font_std_external_restore_platform(const egui_platform_ops_t *old_ops)
{
    egui_platform_t *platform = egui_platform_get();

    EGUI_TEST_ASSERT_NOT_NULL(platform);
    platform->ops = old_ops;
}

static void test_font_std_external_assert_core_and_lock(egui_core_t *core)
{
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.total_count > 0);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.last_core == core);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_FONT_STD_EXPECT_LOCK ? s_font_std_external_load_record.total_count : 0, s_font_std_external_load_record.locked_count);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_FONT_STD_EXPECT_LOCK ? 0 : s_font_std_external_load_record.total_count, s_font_std_external_load_record.unlocked_count);
}

static void test_font_setup_canvas(void)
{
    egui_core_t *core = test_font_std_get_core();
    egui_region_t pfb_region;
    egui_region_t base_region;

    egui_font_std_release_frame_cache(core);
    memset(test_font_pfb, 0, sizeof(test_font_pfb));
    egui_region_init(&pfb_region, 0, 0, TEST_FONT_CANVAS_W, TEST_FONT_CANVAS_H);
    egui_canvas_init(&test_font_canvas, core, test_font_pfb, &pfb_region);
    egui_region_init(&base_region, 0, 0, TEST_FONT_CANVAS_W, TEST_FONT_CANVAS_H);
    egui_canvas_calc_work_region(&test_font_canvas, &base_region);
}

static void test_font_draw_and_capture_char(egui_canvas_t *canvas, const egui_font_t *font, const char *text)
{
    egui_region_t rect;

    test_font_setup_canvas();
    egui_region_init(&rect, 4, 5, 6, 4);
    egui_canvas_draw_text_in_rect(canvas, font, text, &rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void test_font_draw_and_capture_repeat_in_rect(egui_canvas_t *canvas, const egui_font_t *font, const char *text)
{
    egui_region_t rect;

    test_font_setup_canvas();
    egui_region_init(&rect, 1, 1, 12, 6);
    egui_canvas_draw_text_in_rect(canvas, font, text, &rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static int test_font_count_drawn_pixels(void)
{
    int count = 0;

    for (int i = 0; i < TEST_FONT_CANVAS_W * TEST_FONT_CANVAS_H; i++)
    {
        if (test_font_pfb[i] != 0)
        {
            count++;
        }
    }

    return count;
}

static void test_font_std_compressed_draw_matches_raw(void)
{
    test_font_draw_and_capture_char(&test_font_canvas, (const egui_font_t *)&test_font_raw, "A");
    memcpy(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb));

    test_font_draw_and_capture_char(&test_font_canvas, (const egui_font_t *)&test_font_rle4, "A");
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);

    test_font_draw_and_capture_char(&test_font_canvas, (const egui_font_t *)&test_font_rle4xor, "A");
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);
}

static void test_font_std_odd_width_compressed_draw_matches_raw(void)
{
    test_font_draw_and_capture_char(&test_font_canvas, (const egui_font_t *)&test_font_raw, "B");
    memcpy(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb));

    test_font_draw_and_capture_char(&test_font_canvas, (const egui_font_t *)&test_font_rle4, "B");
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);

    test_font_draw_and_capture_char(&test_font_canvas, (const egui_font_t *)&test_font_rle4xor, "B");
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
    test_font_invalid_offscreen.base.api->draw_string((const egui_font_t *)&test_font_invalid_offscreen, &test_font_canvas, "A", 0, 5, EGUI_COLOR_WHITE,
                                                      EGUI_ALPHA_100);
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);
}

static void test_font_std_repeated_compressed_draw_in_rect_matches_raw(void)
{
    static const char repeat_text[] = "ABAB\nBABA";

    test_font_draw_and_capture_repeat_in_rect(&test_font_canvas, (const egui_font_t *)&test_font_raw, repeat_text);
    memcpy(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb));

    test_font_draw_and_capture_repeat_in_rect(&test_font_canvas, (const egui_font_t *)&test_font_rle4, repeat_text);
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);

    test_font_draw_and_capture_repeat_in_rect(&test_font_canvas, (const egui_font_t *)&test_font_rle4xor, repeat_text);
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);
}

static void test_font_std_fast_4_draws_fallback_box_for_unsupported_char(void)
{
    int bytes;

    test_font_setup_canvas();
    bytes = test_font_raw_tall.base.api->draw_string((const egui_font_t *)&test_font_raw_tall, &test_font_canvas, "?", 3, 4, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    EGUI_TEST_ASSERT_EQUAL_INT(1, bytes);
    EGUI_TEST_ASSERT_TRUE(test_font_count_drawn_pixels() > 0);
}

static void test_font_std_external_code_desc_lookup_uses_ext_load(void)
{
    egui_core_t *core;
    const egui_platform_ops_t *old_ops;
    egui_font_std_char_descriptor_t scratch;
    const egui_font_std_char_descriptor_t *desc;

    test_font_setup_canvas();
    core = test_font_std_get_core();
    old_ops = test_font_std_external_install_platform();

    desc = egui_font_std_get_desc_fast_api(&test_font_canvas, &test_font_info_external_raw, 'B', &scratch);

    test_font_std_external_restore_platform(old_ops);

    EGUI_TEST_ASSERT_NOT_NULL(desc);
    EGUI_TEST_ASSERT_EQUAL_INT(3, desc->adv);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.code_count > 0);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.char_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_font_std_external_load_record.pixel_count);
    test_font_std_external_assert_core_and_lock(core);
}

static void test_font_std_external_measure_with_canvas_uses_ext_load(void)
{
    egui_core_t *core;
    const egui_platform_ops_t *old_ops;
    egui_dim_t width = 0;
    egui_dim_t height = 0;
    int ret;

    test_font_setup_canvas();
    core = test_font_std_get_core();
    old_ops = test_font_std_external_install_platform();

    ret = egui_font_get_str_size_with_canvas((const egui_font_t *)&test_font_external_raw, &test_font_canvas, "AB", 0, 0, &width, &height);

    test_font_std_external_restore_platform(old_ops);

    EGUI_TEST_ASSERT_EQUAL_INT(0, ret);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)width);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)height);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.code_count > 0);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.char_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_font_std_external_load_record.pixel_count);
    test_font_std_external_assert_core_and_lock(core);
}

static void test_font_std_external_measure_with_core_uses_ext_load(void)
{
    egui_core_t *core;
    const egui_platform_ops_t *old_ops;
    egui_dim_t width = 0;
    egui_dim_t height = 0;
    int ret;

    test_font_setup_canvas();
    core = test_font_std_get_core();
    old_ops = test_font_std_external_install_platform();

    ret = egui_font_get_str_size_with_core((const egui_font_t *)&test_font_external_raw, core, "AB", 0, 0, &width, &height);

    test_font_std_external_restore_platform(old_ops);

    EGUI_TEST_ASSERT_EQUAL_INT(0, ret);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)width);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)height);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.code_count > 0);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.char_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_font_std_external_load_record.pixel_count);
    test_font_std_external_assert_core_and_lock(core);
}

static void test_font_std_external_measure_multiline_with_core_uses_ext_load(void)
{
    egui_core_t *core;
    const egui_platform_ops_t *old_ops;
    egui_dim_t expected_width = 0;
    egui_dim_t expected_height = 0;
    egui_dim_t width = 0;
    egui_dim_t height = 0;
    int ret;

    test_font_setup_canvas();
    core = test_font_std_get_core();
    ret = egui_font_get_str_size_with_core((const egui_font_t *)&test_font_raw, core, "A\nBB", 1, 1, &expected_width, &expected_height);
    EGUI_TEST_ASSERT_EQUAL_INT(0, ret);

    old_ops = test_font_std_external_install_platform();

    ret = egui_font_get_str_size_with_core((const egui_font_t *)&test_font_external_raw, core, "A\nBB", 1, 1, &width, &height);

    test_font_std_external_restore_platform(old_ops);

    EGUI_TEST_ASSERT_EQUAL_INT(0, ret);
    EGUI_TEST_ASSERT_EQUAL_INT((int)expected_width, (int)width);
    EGUI_TEST_ASSERT_EQUAL_INT((int)expected_height, (int)height);
    EGUI_TEST_ASSERT_TRUE(height > 2);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.code_count > 0);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.char_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_font_std_external_load_record.pixel_count);
    test_font_std_external_assert_core_and_lock(core);
}

static void test_font_std_external_measure_width_limit_with_core_uses_ext_load(void)
{
    egui_core_t *core;
    const egui_platform_ops_t *old_ops;
    egui_dim_t expected_width = 4;
    egui_dim_t expected_height = 0;
    egui_dim_t width = 4;
    egui_dim_t height = 0;
    int ret;

    test_font_setup_canvas();
    core = test_font_std_get_core();
    ret = egui_font_get_str_size_with_core((const egui_font_t *)&test_font_raw, core, "ABB", 0, 0, &expected_width, &expected_height);
    EGUI_TEST_ASSERT_EQUAL_INT(0, ret);

    old_ops = test_font_std_external_install_platform();

    ret = egui_font_get_str_size_with_core((const egui_font_t *)&test_font_external_raw, core, "ABB", 0, 0, &width, &height);

    test_font_std_external_restore_platform(old_ops);

    EGUI_TEST_ASSERT_EQUAL_INT(0, ret);
    EGUI_TEST_ASSERT_EQUAL_INT((int)expected_width, (int)width);
    EGUI_TEST_ASSERT_EQUAL_INT((int)expected_height, (int)height);
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)width);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.code_count > 0);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.char_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_font_std_external_load_record.pixel_count);
    test_font_std_external_assert_core_and_lock(core);
}

static void test_font_std_external_measure_without_core_loads_unlocked(void)
{
    const egui_platform_ops_t *old_ops;
    egui_dim_t width = 0;
    egui_dim_t height = 0;
    int ret;

    test_font_setup_canvas();
    old_ops = test_font_std_external_install_platform();

    ret = egui_font_get_str_size_with_core((const egui_font_t *)&test_font_external_raw, NULL, "AB", 0, 0, &width, &height);

    test_font_std_external_restore_platform(old_ops);

    EGUI_TEST_ASSERT_EQUAL_INT(0, ret);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)width);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)height);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.code_count > 0);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.char_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_font_std_external_load_record.pixel_count);
    EGUI_TEST_ASSERT_NULL(s_font_std_external_load_record.last_core);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_font_std_external_load_record.locked_count);
    EGUI_TEST_ASSERT_EQUAL_INT(s_font_std_external_load_record.total_count, s_font_std_external_load_record.unlocked_count);
}

static void test_font_std_external_compact_measure_with_core_uses_ext_load(void)
{
    egui_core_t *core;
    const egui_platform_ops_t *old_ops;
    egui_dim_t width = 0;
    egui_dim_t height = 0;
    uint8_t ret;

    test_font_setup_canvas();
    core = test_font_std_get_core();
    old_ops = test_font_std_external_install_platform();

    ret = egui_canvas_compact_text_measure_with_font_core(core, (const egui_font_t *)&test_font_external_raw, "AB", 16, 16, &width, &height);

    test_font_std_external_restore_platform(old_ops);

    EGUI_TEST_ASSERT_TRUE(ret);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)width);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)height);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.code_count > 0);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.char_count > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_font_std_external_load_record.pixel_count);
    test_font_std_external_assert_core_and_lock(core);
}

static void test_font_std_external_draw_uses_ext_load(void)
{
    egui_core_t *core;
    const egui_platform_ops_t *old_ops;

    test_font_draw_and_capture_char(&test_font_canvas, (const egui_font_t *)&test_font_raw, "A");
    memcpy(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb));

    core = test_font_std_get_core();
    old_ops = test_font_std_external_install_platform();
    test_font_draw_and_capture_char(&test_font_canvas, (const egui_font_t *)&test_font_external_raw, "A");
    test_font_std_external_restore_platform(old_ops);

    EGUI_TEST_ASSERT_TRUE(memcmp(expected_font_pfb, test_font_pfb, sizeof(test_font_pfb)) == 0);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.code_count > 0);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.char_count > 0);
    EGUI_TEST_ASSERT_TRUE(s_font_std_external_load_record.pixel_count > 0);
    test_font_std_external_assert_core_and_lock(core);
}

static void test_font_std_external_draw_scratch_grows_without_overlapping_allocations(void)
{
    egui_port_alloc_stats_t stats;
    egui_region_t rect;
    const egui_platform_ops_t *old_ops;

    test_font_setup_canvas();
    egui_region_init(&rect, 4, 5, 6, 4);
    old_ops = test_font_std_external_install_platform();

    egui_port_reset_alloc_stats();
    egui_canvas_draw_text_in_rect(&test_font_canvas, (const egui_font_t *)&test_font_external_raw, "AB", &rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP,
                                  EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_font_std_release_frame_cache(test_font_std_get_core());
    egui_port_get_alloc_stats(&stats);

    test_font_std_external_restore_platform(old_ops);

    EGUI_TEST_ASSERT_TRUE(stats.alloc_count >= 2);
    EGUI_TEST_ASSERT_EQUAL_INT(stats.alloc_count, stats.free_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, stats.live_alloc_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)stats.live_bytes);
    EGUI_TEST_ASSERT_TRUE(stats.peak_live_bytes < stats.alloc_bytes);
}

void test_font_std_run(void)
{
    EGUI_TEST_SUITE_BEGIN(font_std);
    EGUI_TEST_RUN(test_font_std_compressed_draw_matches_raw);
    EGUI_TEST_RUN(test_font_std_odd_width_compressed_draw_matches_raw);
    EGUI_TEST_RUN(test_font_std_compressed_size_matches_raw);
    EGUI_TEST_RUN(test_font_std_offscreen_compressed_glyph_skips_decode);
    EGUI_TEST_RUN(test_font_std_repeated_compressed_draw_in_rect_matches_raw);
    EGUI_TEST_RUN(test_font_std_fast_4_draws_fallback_box_for_unsupported_char);
    EGUI_TEST_RUN(test_font_std_external_code_desc_lookup_uses_ext_load);
    EGUI_TEST_RUN(test_font_std_external_measure_with_canvas_uses_ext_load);
    EGUI_TEST_RUN(test_font_std_external_measure_with_core_uses_ext_load);
    EGUI_TEST_RUN(test_font_std_external_measure_multiline_with_core_uses_ext_load);
    EGUI_TEST_RUN(test_font_std_external_measure_width_limit_with_core_uses_ext_load);
    EGUI_TEST_RUN(test_font_std_external_measure_without_core_loads_unlocked);
    EGUI_TEST_RUN(test_font_std_external_compact_measure_with_core_uses_ext_load);
    EGUI_TEST_RUN(test_font_std_external_draw_uses_ext_load);
    EGUI_TEST_RUN(test_font_std_external_draw_scratch_grows_without_overlapping_allocations);
    EGUI_TEST_SUITE_END();
}
