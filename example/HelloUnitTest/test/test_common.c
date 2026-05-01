#include "egui.h"
#include "test/egui_test.h"
#include "test_common.h"

static void test_align_center(void)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_common_align_get_x_y(100, 100, 20, 20, EGUI_ALIGN_CENTER, &x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(40, x);
    EGUI_TEST_ASSERT_EQUAL_INT(40, y);
}

static void test_align_left_top(void)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_common_align_get_x_y(100, 100, 20, 20, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, &x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(0, x);
    EGUI_TEST_ASSERT_EQUAL_INT(0, y);
}

static void test_align_right_bottom(void)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_common_align_get_x_y(100, 100, 20, 20, EGUI_ALIGN_RIGHT | EGUI_ALIGN_BOTTOM, &x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(80, x);
    EGUI_TEST_ASSERT_EQUAL_INT(80, y);
}

static void test_align_hcenter_top(void)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_common_align_get_x_y(100, 100, 20, 20, EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP, &x, &y);
    EGUI_TEST_ASSERT_EQUAL_INT(40, x);
    EGUI_TEST_ASSERT_EQUAL_INT(0, y);
}

static void test_alpha_mix_full(void)
{
    // Full alpha * any = any
    egui_alpha_t result = egui_color_alpha_mix(EGUI_ALPHA_100, EGUI_ALPHA_50);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_50, result);

    result = egui_color_alpha_mix(EGUI_ALPHA_50, EGUI_ALPHA_100);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_50, result);
}

static void test_alpha_mix_zero(void)
{
    // Zero alpha * any = 0
    egui_alpha_t result = egui_color_alpha_mix(EGUI_ALPHA_0, EGUI_ALPHA_100);
    EGUI_TEST_ASSERT_EQUAL_INT(0, result);
}

static void test_alpha_mix_partial(void)
{
    // 50% * 50% ~= 25%
    egui_alpha_t result = egui_color_alpha_mix(EGUI_ALPHA_50, EGUI_ALPHA_50);
    // 127 * 127 >> 8 = 63
    EGUI_TEST_ASSERT_TRUE(result > 50 && result < 80);
}

static void test_alpha_make(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_0, EGUI_ALPHA_MAKE(0));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_10, EGUI_ALPHA_MAKE(10));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_50, EGUI_ALPHA_MAKE(50));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_90, EGUI_ALPHA_MAKE(90));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, EGUI_ALPHA_MAKE(100));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, EGUI_ALPHA_MAKE(135));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_0, EGUI_ALPHA_MAKE(-10));
    EGUI_TEST_ASSERT_EQUAL_INT(94, EGUI_ALPHA_MAKE(37));
}

#if EGUI_CONFIG_COLOR_DEPTH == 16
static void test_rgb565_mix_known_values(void)
{
    egui_color_t bg;
    egui_color_t fg;
    egui_color_t out;

    bg.full = 0xFFFF;
    fg.full = 0x0000;
    out = egui_rgb_mix(bg, fg, 128);
    EGUI_TEST_ASSERT_EQUAL_INT(0x7BEF, out.full);

    bg.full = 0x0000;
    fg.full = 0xFFFF;
    out = egui_rgb_mix(bg, fg, 128);
    EGUI_TEST_ASSERT_EQUAL_INT(0x7BEF, out.full);

    bg.full = 0xF800;
    fg.full = 0x001F;
    egui_rgb_mix_ptr(&bg, &fg, &out, 128);
    EGUI_TEST_ASSERT_EQUAL_INT(0x780F, out.full);

    bg.full = 0x1234;
    fg.full = 0xABCD;
    out = egui_rgb_mix(bg, fg, 85);
    EGUI_TEST_ASSERT_EQUAL_INT(0x3AB1, out.full);
}
#endif

static void test_utf8_code_explicit_widths(void)
{
    uint32_t code = 0;
    int bytes;

    bytes = egui_font_get_utf8_code("A", &code);
    EGUI_TEST_ASSERT_EQUAL_INT(1, bytes);
    EGUI_TEST_ASSERT_EQUAL_INT('A', code);

    bytes = egui_font_get_utf8_code("\xC2\xA2", &code);
    EGUI_TEST_ASSERT_EQUAL_INT(2, bytes);
    EGUI_TEST_ASSERT_EQUAL_INT(0xC2A2, code);

    bytes = egui_font_get_utf8_code("\xE4\xB8\xAD", &code);
    EGUI_TEST_ASSERT_EQUAL_INT(3, bytes);
    EGUI_TEST_ASSERT_EQUAL_INT(0xE4B8AD, code);

    bytes = egui_font_get_utf8_code("\xF0\x9F\x98\x80", &code);
    EGUI_TEST_ASSERT_EQUAL_INT(4, bytes);
    EGUI_TEST_ASSERT_TRUE(code == 0xF09F9880UL);
}

static void test_max_min_abs(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(10, EGUI_MAX(5, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(5, EGUI_MIN(5, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(5, EGUI_ABS(-5));
    EGUI_TEST_ASSERT_EQUAL_INT(5, EGUI_ABS(5));
    EGUI_TEST_ASSERT_EQUAL_INT(0, EGUI_ABS(0));
}

void test_common_run(void)
{
    EGUI_TEST_SUITE_BEGIN(common);

    EGUI_TEST_RUN(test_align_center);
    EGUI_TEST_RUN(test_align_left_top);
    EGUI_TEST_RUN(test_align_right_bottom);
    EGUI_TEST_RUN(test_align_hcenter_top);
    EGUI_TEST_RUN(test_alpha_mix_full);
    EGUI_TEST_RUN(test_alpha_mix_zero);
    EGUI_TEST_RUN(test_alpha_mix_partial);
    EGUI_TEST_RUN(test_alpha_make);
#if EGUI_CONFIG_COLOR_DEPTH == 16
    EGUI_TEST_RUN(test_rgb565_mix_known_values);
#endif
    EGUI_TEST_RUN(test_utf8_code_explicit_widths);
    EGUI_TEST_RUN(test_max_min_abs);

    EGUI_TEST_SUITE_END();
}
