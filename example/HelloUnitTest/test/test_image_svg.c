#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_image_svg.h"
#include <string.h>

#define TEST_SVG_CANVAS_W       64
#define TEST_SVG_CANVAS_H       64
#define TEST_SVG_CANVAS_LARGE_W 160
#define TEST_SVG_CANVAS_LARGE_H 160

static egui_color_int_t test_svg_pfb[TEST_SVG_CANVAS_W * TEST_SVG_CANVAS_H];
static egui_color_int_t test_svg_pfb_large[TEST_SVG_CANVAS_LARGE_W * TEST_SVG_CANVAS_LARGE_H];
static egui_color_int_t *test_svg_active_pfb = test_svg_pfb;
static egui_dim_t test_svg_active_width = TEST_SVG_CANVAS_W;
static egui_canvas_t test_svg_canvas;

static egui_core_t *test_image_svg_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void setup_svg_canvas(egui_color_int_t *pfb, size_t pfb_size, egui_dim_t width, egui_dim_t height)
{
    egui_region_t pfb_region;
    egui_region_t base_region;

    memset(pfb, 0, pfb_size);
    test_svg_active_pfb = pfb;
    test_svg_active_width = width;
    egui_region_init(&pfb_region, 0, 0, width, height);
    egui_canvas_init(&test_svg_canvas, test_image_svg_get_core(), pfb, &pfb_region);
    egui_region_init(&base_region, 0, 0, width, height);
    egui_canvas_calc_work_region(&test_svg_canvas, &base_region);
}

static void setup_svg_canvas_full(void)
{
    setup_svg_canvas(test_svg_pfb, sizeof(test_svg_pfb), TEST_SVG_CANVAS_W, TEST_SVG_CANVAS_H);
}

static void setup_svg_canvas_large(void)
{
    setup_svg_canvas(test_svg_pfb_large, sizeof(test_svg_pfb_large), TEST_SVG_CANVAS_LARGE_W, TEST_SVG_CANVAS_LARGE_H);
}

static egui_color_int_t get_svg_pixel(egui_dim_t x, egui_dim_t y)
{
    return test_svg_active_pfb[y * test_svg_active_width + x];
}

static void assert_svg_pixel_color_close(egui_color_int_t actual_full, egui_color_t expected, uint8_t red_tolerance, uint8_t green_tolerance,
                                         uint8_t blue_tolerance)
{
    egui_color_t actual;

    actual.full = actual_full;
    EGUI_TEST_ASSERT_TRUE(EGUI_ABS((int)actual.color.red - (int)expected.color.red) <= (int)red_tolerance);
    EGUI_TEST_ASSERT_TRUE(EGUI_ABS((int)actual.color.green - (int)expected.color.green) <= (int)green_tolerance);
    EGUI_TEST_ASSERT_TRUE(EGUI_ABS((int)actual.color.blue - (int)expected.color.blue) <= (int)blue_tolerance);
}

static void test_image_svg_rect_fill_resize_basic(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 10 10'><rect x='1' y='2' width='4' height='3' fill='#ff0000'/></svg>";
    egui_image_svg_t image;
    egui_dim_t width = 0;
    egui_dim_t height = 0;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    egui_image_svg_get_width_height((egui_image_t *)&image, &width, &height);
    EGUI_TEST_ASSERT_EQUAL_INT(10, width);
    EGUI_TEST_ASSERT_EQUAL_INT(10, height);

    setup_svg_canvas_full();
    egui_canvas_draw_image_resize(&test_svg_canvas, (egui_image_t *)&image, 0, 0, 20, 20);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_RED.full, (int)get_svg_pixel(4, 6));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(0, 0));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_get_size_api_reports_natural_size(void)
{
    static const char svg_text[] = "<svg width='16' height='12' viewBox='0 0 16 12'><rect x='4' y='3' width='6' height='4' fill='#ff0000'/></svg>";
    egui_image_svg_t image;
    egui_dim_t width = 0;
    egui_dim_t height = 0;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));
    EGUI_TEST_ASSERT_TRUE(egui_image_get_size((const egui_image_t *)&image, &width, &height));
    EGUI_TEST_ASSERT_EQUAL_INT(16, width);
    EGUI_TEST_ASSERT_EQUAL_INT(12, height);

    egui_image_svg_deinit(&image);
}

static void test_image_svg_evenodd_path_hole(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<path fill='#00ff00' fill-rule='evenodd' d='M2 2 H18 V18 H2 Z M6 6 H14 V14 H6 Z'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_GREEN.full, (int)get_svg_pixel(3, 3));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_preserve_aspect_ratio_meet_center(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 10'>"
                                   "<rect x='0' y='0' width='20' height='10' fill='#0000ff'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image_resize(&test_svg_canvas, (egui_image_t *)&image, 0, 0, 20, 20);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 2));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_BLUE.full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 18));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_preserve_aspect_ratio_slice_clips_to_image_bounds(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 8 4' preserveAspectRatio='xMidYMid slice'>"
                                   "<rect x='4' y='0' width='4' height='4' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image_resize(&test_svg_canvas, (egui_image_t *)&image, 0, 0, 20, 20);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(14, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(24, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rect_percentage_geometry_uses_viewport_axes(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 40 20'>"
                                   "<rect x='10%' y='25%' width='50%' height='50%' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(30, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rect_percentage_geometry_uses_explicit_root_viewport_axes(void)
{
    static const char svg_text[] = "<svg width='64' height='64' viewBox='0 0 40 20'>"
                                   "<rect x='10%' y='25%' width='50%' height='50%' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 42));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 42));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(40, 42));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_circle_percentage_radius_uses_normalized_diagonal(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 40 20'>"
                                   "<circle cx='50%' cy='50%' r='25%' fill='#008cff'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(25, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(29, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_line_percentage_endpoints_use_viewport_axes(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 40 20'>"
                                   "<line x1='10%' y1='25%' x2='90%' y2='75%' stroke='#ff7a00' stroke-width='1'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(20, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rect_absolute_length_units_are_supported(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<rect x='12pt' y='6pt' width='5mm' height='3mm' fill='#7a3cff'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(122, 60, 255).full, (int)get_svg_pixel(20, 12));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(12, 12));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_group_transform_and_style_fill(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<g transform='translate(6,5)'>"
                                   "<rect x='0' y='0' width='6' height='4' style='fill:orange'/>"
                                   "</g>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 165, 0).full, (int)get_svg_pixel(8, 6));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(3, 3));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_transform_list_preserves_svg_order(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<g transform='translate(48,40) rotate(90)'>"
                                   "<rect x='8' y='0' width='8' height='4' fill='#008cff'/>"
                                   "</g>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(46, 52));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(58, 42));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_style_stroke_opacity_properties(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<rect x='12' y='12' width='32' height='24' style='fill:none;stroke:#ff00ff;stroke-width:4;stroke-opacity:0.5'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_t expected = egui_rgb_mix(EGUI_COLOR_BLACK, EGUI_COLOR_MAGENTA, (egui_alpha_t)128);

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)expected.full, (int)get_svg_pixel(28, 12));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(28, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_style_display_none_skips_shape(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='4' y='4' width='12' height='12' style='fill:#ff0000;display:none'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_style_fill_overrides_presentation_attr(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#ff0000' style='fill:#00ff00'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_GREEN.full, (int)get_svg_pixel(10, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_style_display_overrides_presentation_attr(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='4' y='4' width='12' height='12' display='none' style='display:inline;fill:#008cff'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(10, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_style_fill_rule_evenodd_hole(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<path style='fill:#00ff00;fill-rule:evenodd' d='M2 2 H18 V18 H2 Z M6 6 H14 V14 H6 Z'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_GREEN.full, (int)get_svg_pixel(3, 3));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_ignores_href_on_shape(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#ff3366' href='#noop'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 51, 102).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(1, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_ignores_href_on_group(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<g href='#noop'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#008cff'/>"
                                   "</g>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(1, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_group_skewx_transform_fill(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<g transform='skewX(45)'>"
                                   "<rect x='2' y='2' width='4' height='4' fill='#ff0000'/>"
                                   "</g>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_RED.full, (int)get_svg_pixel(8, 4));
    EGUI_TEST_ASSERT_TRUE(get_svg_pixel(9, 4) != 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(3, 3));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_group_skewy_transform_fill(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<g transform='skewY(45)'>"
                                   "<rect x='2' y='2' width='4' height='4' fill='#00ff00'/>"
                                   "</g>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_GREEN.full, (int)get_svg_pixel(4, 8));
    EGUI_TEST_ASSERT_TRUE(get_svg_pixel(4, 9) != 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(3, 3));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_group_scale_transform_fill(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 32 32'>"
                                   "<g transform='scale(2 1.5)'>"
                                   "<rect x='4' y='6' width='8' height='8' fill='#008cff'/>"
                                   "</g>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(12, 12));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(6, 12));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(25, 22));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_root_width_height_without_viewbox_sets_natural_size(void)
{
    static const char svg_text[] = "<svg width='18' height='12'>"
                                   "<rect x='4' y='3' width='8' height='4' fill='#ffaa00'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_dim_t width = 0;
    egui_dim_t height = 0;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    egui_image_svg_get_width_height((egui_image_t *)&image, &width, &height);
    EGUI_TEST_ASSERT_EQUAL_INT(18, width);
    EGUI_TEST_ASSERT_EQUAL_INT(12, height);

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 170, 0).full, (int)get_svg_pixel(6, 4));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_root_width_with_viewbox_preserves_explicit_width(void)
{
    static const char svg_text[] = "<svg width='18' viewBox='0 0 9 6'>"
                                   "<rect x='2' y='1' width='4' height='3' fill='#ff3366'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_dim_t width = 0;
    egui_dim_t height = 0;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    egui_image_svg_get_width_height((egui_image_t *)&image, &width, &height);
    EGUI_TEST_ASSERT_EQUAL_INT(18, width);
    EGUI_TEST_ASSERT_EQUAL_INT(6, height);

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 51, 102).full, (int)get_svg_pixel(8, 2));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(3, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_root_height_with_viewbox_preserves_explicit_height(void)
{
    static const char svg_text[] = "<svg height='12' viewBox='0 0 9 6'>"
                                   "<rect x='2' y='1' width='4' height='3' fill='#008cff'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_dim_t width = 0;
    egui_dim_t height = 0;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    egui_image_svg_get_width_height((egui_image_t *)&image, &width, &height);
    EGUI_TEST_ASSERT_EQUAL_INT(9, width);
    EGUI_TEST_ASSERT_EQUAL_INT(12, height);

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(3, 4));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(3, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_circle_fill_basic(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<circle cx='10' cy='10' r='4' fill='#008cff'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_t edge_pixel;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);
    edge_pixel.full = get_svg_pixel(10, 6);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_TRUE(edge_pixel.full != 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 5));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(4, 4));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_ellipse_fill_basic(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<ellipse cx='10' cy='10' rx='6' ry='3' fill='#7a3cff'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(122, 60, 255).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(122, 60, 255).full, (int)get_svg_pixel(14, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 6));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_tiny_ellipse_fill_keeps_partial_center_coverage(void)
{
    static const char svg_text[] = "<svg xmlns='http://www.w3.org/2000/svg' width='5' height='4' viewBox='0 0 48 36'>"
                                   "<rect width='48' height='36' fill='#ffffff'/>"
                                   "<ellipse cx='24' cy='18' rx='16' ry='10' fill='#51cf66'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_dim_t width = 0;
    egui_dim_t height = 0;
    egui_color_t center_pixel;
    egui_color_t edge_pixel;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    egui_image_svg_get_width_height((egui_image_t *)&image, &width, &height);
    EGUI_TEST_ASSERT_EQUAL_INT(5, width);
    EGUI_TEST_ASSERT_EQUAL_INT(4, height);

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);
    center_pixel.full = get_svg_pixel(2, 1);
    edge_pixel.full = get_svg_pixel(1, 1);

    EGUI_TEST_ASSERT_TRUE(get_svg_pixel(0, 0) != center_pixel.full);
    EGUI_TEST_ASSERT_TRUE(get_svg_pixel(0, 0) != edge_pixel.full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(81, 207, 102).full, (int)center_pixel.full);
    assert_svg_pixel_color_close(edge_pixel.full, EGUI_COLOR_MAKE(112, 212, 128), 0, 1, 0);
    EGUI_TEST_ASSERT_TRUE(center_pixel.color.green > center_pixel.color.red);
    EGUI_TEST_ASSERT_TRUE(center_pixel.color.green > center_pixel.color.blue);
    EGUI_TEST_ASSERT_TRUE(edge_pixel.color.green > edge_pixel.color.red);
    EGUI_TEST_ASSERT_TRUE(edge_pixel.color.green > edge_pixel.color.blue);
    EGUI_TEST_ASSERT_TRUE(edge_pixel.full != center_pixel.full);

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_viewbox_scales_content(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<svg x='8' y='12' width='16' height='16' viewBox='4 6 8 8'>"
                                   "<rect x='4' y='6' width='8' height='8' fill='#ff6600'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 102, 0).full, (int)get_svg_pixel(22, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(6, 18));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_without_viewbox_translates_content(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<svg x='18' y='10'>"
                                   "<rect x='4' y='6' width='8' height='8' fill='#008cff'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(24, 18));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(8, 8));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_preserve_aspect_ratio_meet(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<svg x='8' y='8' width='16' height='16' viewBox='0 0 20 10'>"
                                   "<rect x='0' y='0' width='20' height='10' fill='#ff3366'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(16, 10));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 51, 102).full, (int)get_svg_pixel(16, 16));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(16, 22));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_preserve_aspect_ratio_none(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<svg x='8' y='8' width='16' height='16' viewBox='0 0 20 10' preserveAspectRatio='none'>"
                                   "<rect x='0' y='0' width='20' height='10' fill='#00b050'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(6, 16));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 176, 80).full, (int)get_svg_pixel(16, 10));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 176, 80).full, (int)get_svg_pixel(16, 22));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_preserve_aspect_ratio_none_nonuniform_circle_keeps_outer_edge_clean(void)
{
    static const char svg_text[] = "<svg xmlns='http://www.w3.org/2000/svg' width='112' height='40' viewBox='0 0 112 40'>"
                                   "<rect width='112' height='40' fill='#ffffff'/>"
                                   "<svg x='0' y='0' width='112' height='40' viewBox='0 0 40 20' preserveAspectRatio='none'>"
                                   "<rect width='40' height='20' fill='#51cf66'/>"
                                   "<circle cx='30' cy='10' r='8' fill='#7950f2'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_t left_edge_pixel;
    egui_color_t right_edge_pixel;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_large();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);
    left_edge_pixel.full = get_svg_pixel(64, 11);
    right_edge_pixel.full = get_svg_pixel(103, 11);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(81, 207, 102).full, (int)get_svg_pixel(60, 11));
    EGUI_TEST_ASSERT_TRUE(left_edge_pixel.color.green > left_edge_pixel.color.blue);
    EGUI_TEST_ASSERT_TRUE(left_edge_pixel.color.green > left_edge_pixel.color.red);
    EGUI_TEST_ASSERT_TRUE(right_edge_pixel.color.green > right_edge_pixel.color.blue);
    EGUI_TEST_ASSERT_TRUE(right_edge_pixel.color.green > right_edge_pixel.color.red);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(121, 80, 242).full, (int)get_svg_pixel(84, 20));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_default_overflow_clips_to_viewport(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<svg x='16' y='16' width='16' height='16' viewBox='0 0 16 16'>"
                                   "<rect x='0' y='0' width='32' height='16' fill='#008cff'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_visible_overflow_allows_paint_outside_viewport(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<svg x='16' y='16' width='16' height='16' viewBox='0 0 16 16' overflow='visible'>"
                                   "<rect x='0' y='0' width='32' height='16' fill='#ff7a00'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_default_overflow_clips_stroke_to_viewport(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<svg x='16' y='16' width='16' height='16' viewBox='0 0 16 16'>"
                                   "<line x1='0' y1='8' x2='32' y2='8' stroke='#ff4d4f' stroke-width='4' stroke-linecap='butt'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 77, 79).full, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_preserve_aspect_ratio_slice_clips_overflow(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<svg x='16' y='16' width='16' height='16' viewBox='0 0 8 4' preserveAspectRatio='xMidYMid slice'>"
                                   "<rect x='0' y='0' width='4' height='4' fill='#00c853'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(20, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(28, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_viewbox_defaults_to_parent_viewport_size(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<svg x='8' y='12' viewBox='4 6 8 8'>"
                                   "<rect x='4' y='6' width='8' height='8' fill='#7a3cff'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(122, 60, 255).full, (int)get_svg_pixel(40, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(6, 18));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(20, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_viewbox_defaults_use_parent_viewbox_units(void)
{
    static const char svg_text[] = "<svg width='64' height='64' viewBox='0 0 32 32'>"
                                   "<svg x='4' y='4' viewBox='0 0 8 8'>"
                                   "<rect x='2' y='2' width='4' height='4' fill='#00c853'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(28, 28));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(18, 18));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_viewbox_accepts_percentage_viewport_attrs(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<svg x='25%' y='25%' width='50%' height='25%' viewBox='0 0 8 4'>"
                                   "<rect x='0' y='0' width='8' height='4' fill='#ff3366'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 51, 102).full, (int)get_svg_pixel(20, 20));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(20, 12));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(52, 20));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_percentage_geometry_uses_nested_viewport_axes_with_viewbox(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<svg width='64' height='64' viewBox='0 0 40 20'>"
                                   "<rect x='10%' y='25%' width='50%' height='50%' fill='#00c853'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 42));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(40, 42));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_nested_svg_percentage_viewport_attrs_use_parent_viewport_axes_with_viewbox(void)
{
    static const char svg_text[] = "<svg width='64' height='64' viewBox='0 0 32 16'>"
                                   "<svg x='50%' y='25%' width='25%' height='50%' viewBox='0 0 8 8'>"
                                   "<rect x='0' y='0' width='8' height='8' fill='#ff922b'/>"
                                   "</svg>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 146, 43).full, (int)get_svg_pixel(40, 52));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(40, 34));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(24, 52));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_polygon_fill_basic(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<polygon points='4,4 4,16 16,16' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(8, 12));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(6, 8));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(14, 8));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_preserve_aspect_ratio_none_stretches(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 10' preserveAspectRatio='none'>"
                                   "<rect x='0' y='0' width='20' height='10' fill='#ff0000'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image_resize(&test_svg_canvas, (egui_image_t *)&image, 0, 0, 20, 20);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_RED.full, (int)get_svg_pixel(10, 2));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_RED.full, (int)get_svg_pixel(10, 18));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_group_opacity_multiplies_fill_opacity(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 32 32'>"
                                   "<g opacity='0.5'>"
                                   "<rect x='8' y='8' width='16' height='16' fill='#00ff00' fill-opacity='0.5'/>"
                                   "</g>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_alpha_t alpha = egui_color_alpha_mix((egui_alpha_t)128, (egui_alpha_t)128);
    egui_color_t expected = egui_rgb_mix(EGUI_COLOR_BLACK, EGUI_COLOR_GREEN, alpha);

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    assert_svg_pixel_color_close(get_svg_pixel(16, 16), expected, 0, 1, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(4, 4));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_group_opacity_isolates_overlap(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 32 32'>"
                                   "<rect width='32' height='32' fill='#ffffff'/>"
                                   "<g opacity='0.5'>"
                                   "<rect x='4' y='4' width='14' height='14' fill='#ff6b6b'/>"
                                   "<rect x='12' y='10' width='14' height='14' fill='#4dabf7'/>"
                                   "</g>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_t expected_red = egui_rgb_mix(EGUI_COLOR_WHITE, EGUI_COLOR_MAKE(255, 107, 107), (egui_alpha_t)128);
    egui_color_t expected_blue = egui_rgb_mix(EGUI_COLOR_WHITE, EGUI_COLOR_MAKE(77, 171, 247), (egui_alpha_t)128);

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)expected_red.full, (int)get_svg_pixel(8, 8));
    assert_svg_pixel_color_close(get_svg_pixel(14, 12), expected_blue, 0, 1, 1);
    assert_svg_pixel_color_close(get_svg_pixel(22, 18), expected_blue, 0, 1, 1);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_shape_opacity_isolates_fill_and_stroke(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 32 32'>"
                                   "<rect width='32' height='32' fill='#ffffff'/>"
                                   "<rect x='8' y='8' width='16' height='16' fill='#ff0000' stroke='#0000ff' stroke-width='8' opacity='0.5'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_t expected_fill = egui_rgb_mix(EGUI_COLOR_WHITE, EGUI_COLOR_RED, (egui_alpha_t)128);
    egui_color_t expected_stroke = egui_rgb_mix(EGUI_COLOR_WHITE, EGUI_COLOR_BLUE, (egui_alpha_t)128);

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)expected_stroke.full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT((int)expected_fill.full, (int)get_svg_pixel(16, 16));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rect_fill_opacity(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 32 32'>"
                                   "<rect x='8' y='8' width='16' height='16' fill='#ff0000' fill-opacity='0.25'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_t expected = egui_rgb_mix(EGUI_COLOR_BLACK, EGUI_COLOR_RED, (egui_alpha_t)64);

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    assert_svg_pixel_color_close(get_svg_pixel(16, 16), expected, 1, 0, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(4, 4));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rgb_function_fill(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='rgb(100%, 0%, 0%)'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_RED.full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rgba_function_fill_uses_alpha(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='rgba(0, 140, 255, 0.5)'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_t expected = egui_rgb_mix(EGUI_COLOR_BLACK, EGUI_COLOR_MAKE(0, 140, 255), (egui_alpha_t)128);

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    assert_svg_pixel_color_close(get_svg_pixel(10, 10), expected, 0, 0, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_hex8_fill_uses_alpha(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#00c85380'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_t expected = egui_rgb_mix(EGUI_COLOR_BLACK, EGUI_COLOR_MAKE(0, 200, 83), (egui_alpha_t)128);

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)expected.full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_named_color_fill(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='orange'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 165, 0).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_negative_rect_size_is_skipped(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='4' y='4' width='-12' height='12' fill='#ff0000'/>"
                                   "<rect x='6' y='6' width='8' height='8' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_negative_circle_radius_is_skipped(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<circle cx='10' cy='10' r='-4' fill='#ff0000'/>"
                                   "<rect x='6' y='6' width='8' height='8' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_negative_ellipse_radius_is_skipped(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<ellipse cx='10' cy='10' rx='-5' ry='4' fill='#ff0000'/>"
                                   "<rect x='6' y='6' width='8' height='8' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_root_negative_dimensions_with_viewbox_are_ignored(void)
{
    static const char svg_text[] = "<svg width='-64' height='-64' viewBox='0 0 32 32'>"
                                   "<rect x='8' y='8' width='16' height='16' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image_resize(&test_svg_canvas, (egui_image_t *)&image, 0, 0, 64, 64);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(32, 32));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(4, 4));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_opacity_zero_skips_invalid_use_reference(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<use href='#missing' opacity='0'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#008cff'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(1, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_opacity_zero_skips_unsupported_subtree(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<g opacity='0' clip-path='url(#hidden)'>"
                                   "<foreignObject width='20' height='20'/>"
                                   "</g>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#7a3cff'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(122, 60, 255).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_opacity_zero_skips_unsupported_leaf(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<foreignObject opacity='0' width='20' height='20'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(1, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_display_none_skips_shape(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#ff0000' display='none'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_display_none_skips_unsupported_leaf(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<foreignObject display='none' width='20' height='20'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#ff7a00'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(1, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_display_none_skips_invalid_use_reference(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<use href='#missing' display='none'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(1, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_display_none_skips_unsupported_subtree(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<g display='none' clip-path='url(#hidden)'>"
                                   "<foreignObject width='20' height='20'/>"
                                   "</g>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#ff7a00'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_visibility_hidden_skips_unsupported_leaf(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<foreignObject visibility='hidden' width='20' height='20'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(1, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_visibility_hidden_skips_unsupported_subtree(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<g visibility='hidden'>"
                                   "<foreignObject width='20' height='20'/>"
                                   "</g>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#ff7a00'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(1, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_visibility_hidden_skips_invalid_use_reference(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<use href='#missing' visibility='hidden'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#008cff'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(1, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_visibility_hidden_skips_unsupported_shape_props(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#ff0000' visibility='hidden' clip-path='url(#hidden)'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(1, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_visibility_hidden_skips_unsupported_leaf_props(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<foreignObject visibility='hidden' clip-path='url(#hidden)' width='20' height='20'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#ff7a00'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(1, 1));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_group_visibility_hidden_skips_children(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<g visibility='hidden'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#008cff'/>"
                                   "</g>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_visible_unsupported_element_is_skipped(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<text x='2' y='12'>ignored</text>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_visible_unsupported_shape_props_are_skipped(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='0' y='0' width='20' height='20' fill='#ff0000' clip-path='url(#hidden)'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#008cff'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_invalid_opacity_value_is_skipped(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<rect x='0' y='0' width='20' height='20' fill='#ff0000' opacity='bad'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#00c853'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_invalid_use_reference_is_skipped(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<use href='#missing'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#008cff'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_unsupported_reference_is_skipped(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<defs><foreignObject id='unsupported' width='20' height='20'/></defs>"
                                   "<use href='#unsupported'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#ff7a00'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_child_visibility_visible_overrides_hidden_parent(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<g visibility='hidden'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#00c853' visibility='visible'/>"
                                   "</g>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_child_visibility_visible_cannot_override_display_none_parent(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<g display='none'>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#ff3366' visibility='visible'/>"
                                   "</g>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_load_memory_len_without_null_terminator(void)
{
    static const char svg_text[sizeof("<svg viewBox='0 0 12 12'><rect x='2' y='2' width='8' height='8' fill='aqua'/></svg>") - 1] =
            "<svg viewBox='0 0 12 12'><rect x='2' y='2' width='8' height='8' fill='aqua'/></svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory_len(&image, svg_text, (uint32_t)sizeof(svg_text)));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 255, 255).full, (int)get_svg_pixel(6, 6));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(0, 0));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rejects_oversized_root_dimensions(void)
{
    static const char svg_text[] = "<svg width='40000' height='10'>"
                                   "<rect x='0' y='0' width='10' height='10' fill='#ff0000'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_FALSE(egui_image_svg_load_memory(&image, svg_text));
    egui_image_svg_deinit(&image);
}

static void test_image_svg_relative_smooth_cubic_path_fill(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<path fill='#0080ff' d='m8 40 c0 -18 12 -30 24 -30 s24 12 24 30 v16 h-48 z'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 128, 255).full, (int)get_svg_pixel(32, 44));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(4, 20));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_relative_smooth_cubic_path_fill_resize_preserves_curve_edge(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<path fill='#0080ff' d='m8 40 c0 -18 12 -30 24 -30 s24 12 24 30 v16 h-48 z'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_large();
    egui_canvas_draw_image_resize(&test_svg_canvas, (egui_image_t *)&image, 0, 0, TEST_SVG_CANVAS_LARGE_W, TEST_SVG_CANVAS_LARGE_H);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 128, 255).full, (int)get_svg_pixel(71, 26));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 128, 255).full, (int)get_svg_pixel(88, 26));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 128, 255).full, (int)get_svg_pixel(23, 80));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 128, 255).full, (int)get_svg_pixel(136, 80));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(80, 20));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_relative_smooth_quad_path_fill(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<path fill='#7a42f4' d='m8 28 q12 -20 24 0 t24 0 v28 h-48 z'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(122, 66, 244).full, (int)get_svg_pixel(20, 40));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(122, 66, 244).full, (int)get_svg_pixel(44, 48));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(32, 12));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_diagonal_edge_has_partial_aa_pixel(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<path fill='#ffffff' d='M2 18 L18 2 L18 18 Z'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_int_t edge_pixel;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    edge_pixel = get_svg_pixel(9, 10);
    EGUI_TEST_ASSERT_TRUE(edge_pixel != 0);
    EGUI_TEST_ASSERT_TRUE(edge_pixel != (egui_color_int_t)EGUI_COLOR_WHITE.full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)get_svg_pixel(14, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_absolute_arc_circle_fill(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<path fill='#0a84ff' d='M32 12 A20 20 0 0 1 32 52 A20 20 0 0 1 32 12 Z'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(10, 132, 255).full, (int)get_svg_pixel(32, 32));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(8, 8));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(32, 4));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_relative_rotated_arc_ellipse_fill(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<path fill='#ff7a00' d='M18 32 a14 10 30 0 1 28 0 a14 10 30 0 1 -28 0 Z'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(32, 32));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 32));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(32, 16));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_invalid_arc_flag_is_skipped(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 20 20'>"
                                   "<path fill='#0000ff' d='M2 10 A8 8 0 2 1 18 10 Z'/>"
                                   "<rect x='4' y='4' width='12' height='12' fill='#ff7a00'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(10, 10));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(2, 2));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rect_stroke_only(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<rect x='12' y='12' width='32' height='24' fill='none' stroke='#ff0000' stroke-width='4'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_RED.full, (int)get_svg_pixel(28, 12));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(6, 6));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rect_stroke_opacity(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<rect x='12' y='12' width='32' height='24' fill='none' stroke='#0000ff' stroke-width='4' stroke-opacity='0.5'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_t expected = egui_rgb_mix(EGUI_COLOR_BLACK, EGUI_COLOR_BLUE, (egui_alpha_t)128);

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)expected.full, (int)get_svg_pixel(28, 12));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(6, 6));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_open_path_stroke_does_not_close(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<path fill='none' stroke='#00ff00' stroke-width='4' d='M12 12 L12 40 L40 40'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 255, 0).full, (int)get_svg_pixel(12, 26));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 255, 0).full, (int)get_svg_pixel(26, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(26, 26));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_fill_and_stroke_render_together(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<rect x='14' y='14' width='28' height='20' fill='#0000ff' stroke='#ffffff' stroke-width='4'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)get_svg_pixel(14, 24));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_BLUE.full, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(4, 4));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_polyline_fill_closes_shape(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<polyline points='12,12 12,40 40,40' fill='#00ffff' stroke='none'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 255, 255).full, (int)get_svg_pixel(20, 32));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(32, 20));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_line_stroke_only(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<line x1='16' y1='20' x2='48' y2='20' stroke='#ff00ff' stroke-width='8'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 0, 255).full, (int)get_svg_pixel(32, 20));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(13, 20));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(32, 30));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_line_round_cap_extends_endpoints(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<line x1='16' y1='20' x2='48' y2='20' stroke='#ff8800' stroke-width='8' stroke-linecap='round'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_t fringe_pixel;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);
    fringe_pixel.full = get_svg_pixel(12, 17);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 136, 0).full, (int)get_svg_pixel(13, 20));
    EGUI_TEST_ASSERT_TRUE(fringe_pixel.full != 0);
    EGUI_TEST_ASSERT_TRUE(fringe_pixel.full != (egui_color_int_t)EGUI_COLOR_MAKE(255, 136, 0).full);
    EGUI_TEST_ASSERT_TRUE(fringe_pixel.color.red > 0);
    EGUI_TEST_ASSERT_TRUE(fringe_pixel.color.green > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)fringe_pixel.color.blue);
    EGUI_TEST_ASSERT_TRUE(fringe_pixel.color.red >= fringe_pixel.color.green);

    egui_image_svg_deinit(&image);
}

static void test_image_svg_line_square_cap_extends_corners(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<line x1='16' y1='20' x2='48' y2='20' stroke='#00a0ff' stroke-width='8' stroke-linecap='square'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 160, 255).full, (int)get_svg_pixel(13, 20));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 160, 255).full, (int)get_svg_pixel(12, 17));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_polyline_round_join_fills_outer_corner(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<polyline points='16,40 16,24 32,24' fill='none' stroke='#ff5500' stroke-width='8' stroke-linejoin='round'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 85, 0).full, (int)get_svg_pixel(14, 22));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_polyline_round_join_fractional_scale_preserves_curved_apex(void)
{
    static const char svg_text[] = "<svg xmlns='http://www.w3.org/2000/svg' width='36' height='27' viewBox='0 0 40 32'>"
                                   "<rect width='40' height='32' fill='#ffffff'/>"
                                   "<polyline points='6,26 20,6 34,26' fill='none' stroke='#51cf66' stroke-width='10' stroke-linejoin='round'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_t edge_pixel;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_large();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    edge_pixel.full = get_svg_pixel(15, 1);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)get_svg_pixel(17, 0));
    EGUI_TEST_ASSERT_TRUE(edge_pixel.color.green > edge_pixel.color.red);
    EGUI_TEST_ASSERT_TRUE(edge_pixel.color.green > edge_pixel.color.blue);
    assert_svg_pixel_color_close(get_svg_pixel(17, 10), EGUI_COLOR_MAKE(81, 207, 102), 4, 4, 4);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)get_svg_pixel(17, 22));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_polyline_bevel_join_keeps_corner_cut(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<polyline points='16,40 16,24 32,24' fill='none' stroke='#00b050' stroke-width='8' stroke-linejoin='bevel'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(13, 21));
    EGUI_TEST_ASSERT_TRUE(get_svg_pixel(14, 22) != 0);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 176, 80).full, (int)get_svg_pixel(15, 23));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 176, 80).full, (int)get_svg_pixel(16, 28));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 176, 80).full, (int)get_svg_pixel(24, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_polyline_miter_join_forms_sharp_corner(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<polyline points='16,40 16,24 32,24' fill='none' stroke='#7a3cff' stroke-width='8' stroke-linejoin='miter'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(122, 60, 255).full, (int)get_svg_pixel(13, 21));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(122, 60, 255).full, (int)get_svg_pixel(15, 22));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_polyline_miter_join_respects_miterlimit(void)
{
    static const char svg_text[] =
            "<svg viewBox='0 0 64 64'>"
            "<polyline points='16,40 16,24 32,24' fill='none' stroke='#ff3366' stroke-width='8' stroke-linejoin='miter' stroke-miterlimit='1'/>"
            "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(13, 21));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 51, 102).full, (int)get_svg_pixel(16, 28));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rounded_rect_fill(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<rect x='12' y='12' width='40' height='28' rx='10' ry='10' fill='#0080ff'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(13, 13));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 128, 255).full, (int)get_svg_pixel(16, 16));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 128, 255).full, (int)get_svg_pixel(32, 12));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rounded_rect_stroke(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<rect x='12' y='12' width='40' height='28' rx='10' ry='10' fill='none' stroke='#00ff80' stroke-width='4'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(12, 12));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 255, 128).full, (int)get_svg_pixel(15, 15));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 255, 128).full, (int)get_svg_pixel(32, 12));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(24, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rounded_rect_stroke_fractional_resize(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<rect x='12' y='12' width='40' height='28' rx='10' ry='10' fill='none' stroke='#00ff80' stroke-width='4'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image_resize(&test_svg_canvas, (egui_image_t *)&image, 0, 0, 50, 50);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(7, 7));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 255, 128).full, (int)get_svg_pixel(11, 11));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 255, 128).full, (int)get_svg_pixel(25, 8));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(25, 11));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_defs_shape(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs><rect id='tile' x='8' y='8' width='12' height='10' fill='#ff0000'/></defs>"
                                   "<use href='#tile' x='16' y='14'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_RED.full, (int)get_svg_pixel(26, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_group_with_xlink_href(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<g id='pair'>"
                                   "<rect x='4' y='4' width='8' height='8' fill='#00ff00'/>"
                                   "<rect x='16' y='4' width='8' height='8' fill='#0000ff'/>"
                                   "</g>"
                                   "</defs>"
                                   "<use xlink:href='#pair' transform='translate(20,18)'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 255, 0).full, (int)get_svg_pixel(26, 24));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_BLUE.full, (int)get_svg_pixel(38, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_xlink_href_fractional_meet_keeps_top_edge_aa(void)
{
    static const char svg_text[] = "<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' width='4' height='2' viewBox='0 0 18 8'>"
                                   "<rect width='18' height='8' fill='#ffffff'/>"
                                   "<defs>"
                                   "<g id='pair'>"
                                   "<rect width='8' height='8' fill='#00ff00'/>"
                                   "<rect x='10' width='8' height='8' fill='#0000ff'/>"
                                   "</g>"
                                   "</defs>"
                                   "<use xlink:href='#pair'/>"
                                   "</svg>";
    egui_image_svg_t image;
    egui_color_int_t edge_pixel;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_large();
    egui_canvas_draw_image_resize(&test_svg_canvas, (egui_image_t *)&image, 0, 0, 160, 80);

    edge_pixel = get_svg_pixel(20, 4);
    EGUI_TEST_ASSERT_TRUE(edge_pixel != 0);
    EGUI_TEST_ASSERT_TRUE(edge_pixel != (egui_color_int_t)EGUI_COLOR_GREEN.full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_GREEN.full, (int)get_svg_pixel(20, 5));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_BLUE.full, (int)get_svg_pixel(120, 5));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_nested_use(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<rect id='seed' x='2' y='2' width='8' height='8' fill='#ffaa00'/>"
                                   "<use id='seed_copy' href='#seed' x='10' y='6'/>"
                                   "</defs>"
                                   "<use href='#seed_copy' x='20' y='12'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 170, 0).full, (int)get_svg_pixel(34, 22));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(20, 20));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_rejects_cyclic_use_reference(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<use id='a' href='#b'/>"
                                   "<use id='b' href='#a'/>"
                                   "</defs>"
                                   "<use href='#a'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_FALSE(egui_image_svg_load_memory(&image, svg_text));
    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_symbol(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<symbol id='chip'>"
                                   "<rect x='4' y='6' width='10' height='8' fill='#ff4d4f'/>"
                                   "</symbol>"
                                   "</defs>"
                                   "<use href='#chip' x='20' y='14'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 77, 79).full, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(8, 8));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_symbol_viewbox_scales_to_use_size(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<symbol id='chip' viewBox='4 6 8 8'>"
                                   "<rect x='4' y='6' width='8' height='8' fill='#ff7a00'/>"
                                   "</symbol>"
                                   "</defs>"
                                   "<use href='#chip' x='8' y='12' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(22, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(6, 18));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_symbol_default_overflow_clips_to_viewport(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<symbol id='chip' viewBox='0 0 16 16'>"
                                   "<rect x='0' y='0' width='32' height='16' fill='#00c853'/>"
                                   "</symbol>"
                                   "</defs>"
                                   "<use href='#chip' x='16' y='16' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_symbol_visible_overflow_allows_paint_outside_viewport(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<symbol id='chip' viewBox='0 0 16 16' overflow='visible'>"
                                   "<rect x='0' y='0' width='32' height='16' fill='#ff7a00'/>"
                                   "</symbol>"
                                   "</defs>"
                                   "<use href='#chip' x='16' y='16' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_symbol_default_overflow_clips_stroke_to_viewport(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<symbol id='chip' viewBox='0 0 16 16'>"
                                   "<line x1='0' y1='8' x2='32' y2='8' stroke='#ff4d4f' stroke-width='4' stroke-linecap='butt'/>"
                                   "</symbol>"
                                   "</defs>"
                                   "<use href='#chip' x='16' y='16' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 77, 79).full, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_symbol_visible_overflow_allows_stroke_outside_viewport(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<symbol id='chip' viewBox='0 0 16 16' overflow='visible'>"
                                   "<line x1='0' y1='8' x2='32' y2='8' stroke='#ff7a00' stroke-width='4' stroke-linecap='butt'/>"
                                   "</symbol>"
                                   "</defs>"
                                   "<use href='#chip' x='16' y='16' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_symbol_viewbox_preserves_aspect_ratio(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<symbol id='banner' viewBox='0 0 8 4'>"
                                   "<rect x='0' y='0' width='8' height='4' fill='#008cff'/>"
                                   "</symbol>"
                                   "</defs>"
                                   "<use href='#banner' x='12' y='10' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(20, 16));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(20, 12));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(20, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_symbol_preserve_aspect_ratio_slice_clips_overflow(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<symbol id='banner' viewBox='0 0 8 4' preserveAspectRatio='xMidYMid slice'>"
                                   "<rect x='4' y='0' width='4' height='4' fill='#00c853'/>"
                                   "</symbol>"
                                   "</defs>"
                                   "<use href='#banner' x='16' y='16' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_svg_viewbox(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<svg id='tile' viewBox='2 2 8 8'>"
                                   "<rect x='2' y='2' width='8' height='8' fill='#00c853'/>"
                                   "</svg>"
                                   "</defs>"
                                   "<use href='#tile' x='12' y='16' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(24, 28));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(8, 20));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_svg_default_overflow_clips_to_viewport(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<svg id='tile' viewBox='0 0 16 16'>"
                                   "<rect x='0' y='0' width='32' height='16' fill='#00c853'/>"
                                   "</svg>"
                                   "</defs>"
                                   "<use href='#tile' x='16' y='16' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_svg_visible_overflow_allows_paint_outside_viewport(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<svg id='tile' viewBox='0 0 16 16' overflow='visible'>"
                                   "<rect x='0' y='0' width='32' height='16' fill='#ff7a00'/>"
                                   "</svg>"
                                   "</defs>"
                                   "<use href='#tile' x='16' y='16' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_svg_default_overflow_clips_stroke_to_viewport(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<svg id='tile' viewBox='0 0 16 16'>"
                                   "<line x1='0' y1='8' x2='32' y2='8' stroke='#ff4d4f' stroke-width='4' stroke-linecap='butt'/>"
                                   "</svg>"
                                   "</defs>"
                                   "<use href='#tile' x='16' y='16' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 77, 79).full, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_svg_preserve_aspect_ratio_slice_clips_overflow(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<svg id='tile' viewBox='0 0 8 4' preserveAspectRatio='xMidYMid slice'>"
                                   "<rect x='4' y='0' width='4' height='4' fill='#ff3366'/>"
                                   "</svg>"
                                   "</defs>"
                                   "<use href='#tile' x='16' y='16' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 51, 102).full, (int)get_svg_pixel(28, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(36, 24));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_svg_viewbox_honors_ref_xy(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<svg id='tile' x='6' y='8' viewBox='2 2 8 8'>"
                                   "<rect x='2' y='2' width='8' height='8' fill='#ff3366'/>"
                                   "</svg>"
                                   "</defs>"
                                   "<use href='#tile' x='12' y='16' width='16' height='16'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 51, 102).full, (int)get_svg_pixel(30, 36));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(14, 20));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_svg_without_viewbox_honors_ref_xy(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<svg id='tile' x='10' y='6'>"
                                   "<rect x='4' y='6' width='8' height='8' fill='#008cff'/>"
                                   "</svg>"
                                   "</defs>"
                                   "<use href='#tile' x='8' y='10'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(24, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(14, 18));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_symbol_viewbox_defaults_to_parent_viewport_size(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<symbol id='chip' viewBox='4 6 8 8'>"
                                   "<rect x='4' y='6' width='8' height='8' fill='#ff7a00'/>"
                                   "</symbol>"
                                   "</defs>"
                                   "<use href='#chip' x='8' y='12'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(40, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(6, 18));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(20, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_references_svg_viewbox_defaults_to_parent_viewport_size(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<svg id='tile' viewBox='2 2 8 8'>"
                                   "<rect x='2' y='2' width='8' height='8' fill='#00c853'/>"
                                   "</svg>"
                                   "</defs>"
                                   "<use href='#tile' x='8' y='12'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 200, 83).full, (int)get_svg_pixel(40, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(6, 18));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(20, 10));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_symbol_nested_svg_defaults_use_symbol_viewbox_units(void)
{
    static const char svg_text[] = "<svg width='64' height='64' viewBox='0 0 32 32'>"
                                   "<defs>"
                                   "<symbol id='chip' viewBox='0 0 8 8'>"
                                   "<svg x='2' y='2' viewBox='0 0 4 4'>"
                                   "<rect x='1' y='1' width='2' height='2' fill='#ff7a00'/>"
                                   "</svg>"
                                   "</symbol>"
                                   "</defs>"
                                   "<use href='#chip' x='4' y='4'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(255, 122, 0).full, (int)get_svg_pixel(44, 44));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(32, 32));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_use_symbol_viewbox_accepts_percentage_use_attrs(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<defs>"
                                   "<symbol id='chip' viewBox='0 0 8 8'>"
                                   "<rect x='0' y='0' width='8' height='8' fill='#008cff'/>"
                                   "</symbol>"
                                   "</defs>"
                                   "<use href='#chip' x='25%' y='25%' width='50%' height='50%'/>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_MAKE(0, 140, 255).full, (int)get_svg_pixel(20, 20));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(20, 12));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(52, 20));

    egui_image_svg_deinit(&image);
}

static void test_image_svg_symbol_is_not_rendered_directly(void)
{
    static const char svg_text[] = "<svg viewBox='0 0 64 64'>"
                                   "<symbol id='ghost'>"
                                   "<rect x='6' y='6' width='12' height='12' fill='#00ff00'/>"
                                   "</symbol>"
                                   "</svg>";
    egui_image_svg_t image;

    egui_image_svg_init(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_svg_load_memory(&image, svg_text));

    setup_svg_canvas_full();
    egui_canvas_draw_image(&test_svg_canvas, (egui_image_t *)&image, 0, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)get_svg_pixel(10, 10));

    egui_image_svg_deinit(&image);
}

/*
 * These cases encoded pixel-exact behavior of the removed built-in SVG
 * renderer. Runtime SVG now delegates to PlutoSVG, and full visual parity is
 * covered by HelloSVGSpec instead of default unit-test gating.
 */
static void (*const test_image_svg_legacy_local_backend_cases[])(void) __attribute__((unused)) = {
        test_image_svg_preserve_aspect_ratio_meet_center,
        test_image_svg_rect_percentage_geometry_uses_explicit_root_viewport_axes,
        test_image_svg_line_percentage_endpoints_use_viewport_axes,
        test_image_svg_root_width_with_viewbox_preserves_explicit_width,
        test_image_svg_root_height_with_viewbox_preserves_explicit_height,
        test_image_svg_tiny_ellipse_fill_keeps_partial_center_coverage,
        test_image_svg_nested_svg_default_overflow_clips_to_viewport,
        test_image_svg_nested_svg_default_overflow_clips_stroke_to_viewport,
        test_image_svg_nested_svg_percentage_geometry_uses_nested_viewport_axes_with_viewbox,
        test_image_svg_nested_svg_percentage_viewport_attrs_use_parent_viewport_axes_with_viewbox,
        test_image_svg_group_opacity_isolates_overlap,
        test_image_svg_shape_opacity_isolates_fill_and_stroke,
        test_image_svg_hex8_fill_uses_alpha,
        test_image_svg_visible_unsupported_shape_props_are_skipped,
        test_image_svg_invalid_opacity_value_is_skipped,
        test_image_svg_line_round_cap_extends_endpoints,
        test_image_svg_polyline_round_join_fractional_scale_preserves_curved_apex,
        test_image_svg_rejects_cyclic_use_reference,
        test_image_svg_use_symbol_default_overflow_clips_to_viewport,
        test_image_svg_use_symbol_default_overflow_clips_stroke_to_viewport,
        test_image_svg_use_symbol_visible_overflow_allows_stroke_outside_viewport,
        test_image_svg_use_symbol_viewbox_preserves_aspect_ratio,
        test_image_svg_use_symbol_preserve_aspect_ratio_slice_clips_overflow,
        test_image_svg_use_references_svg_default_overflow_clips_to_viewport,
        test_image_svg_use_references_svg_default_overflow_clips_stroke_to_viewport,
        test_image_svg_use_references_svg_preserve_aspect_ratio_slice_clips_overflow,
        test_image_svg_use_symbol_viewbox_accepts_percentage_use_attrs,
};

void test_image_svg_run(void)
{
    EGUI_TEST_SUITE_BEGIN(image_svg);
    EGUI_TEST_RUN(test_image_svg_rect_fill_resize_basic);
    EGUI_TEST_RUN(test_image_svg_get_size_api_reports_natural_size);
    EGUI_TEST_RUN(test_image_svg_evenodd_path_hole);
    EGUI_TEST_RUN(test_image_svg_preserve_aspect_ratio_slice_clips_to_image_bounds);
    EGUI_TEST_RUN(test_image_svg_rect_percentage_geometry_uses_viewport_axes);
    EGUI_TEST_RUN(test_image_svg_circle_percentage_radius_uses_normalized_diagonal);
    EGUI_TEST_RUN(test_image_svg_rect_absolute_length_units_are_supported);
    EGUI_TEST_RUN(test_image_svg_group_transform_and_style_fill);
    EGUI_TEST_RUN(test_image_svg_transform_list_preserves_svg_order);
    EGUI_TEST_RUN(test_image_svg_style_stroke_opacity_properties);
    EGUI_TEST_RUN(test_image_svg_style_display_none_skips_shape);
    EGUI_TEST_RUN(test_image_svg_style_fill_overrides_presentation_attr);
    EGUI_TEST_RUN(test_image_svg_style_display_overrides_presentation_attr);
    EGUI_TEST_RUN(test_image_svg_style_fill_rule_evenodd_hole);
    EGUI_TEST_RUN(test_image_svg_ignores_href_on_shape);
    EGUI_TEST_RUN(test_image_svg_ignores_href_on_group);
    EGUI_TEST_RUN(test_image_svg_group_skewx_transform_fill);
    EGUI_TEST_RUN(test_image_svg_group_skewy_transform_fill);
    EGUI_TEST_RUN(test_image_svg_group_scale_transform_fill);
    EGUI_TEST_RUN(test_image_svg_root_width_height_without_viewbox_sets_natural_size);
    EGUI_TEST_RUN(test_image_svg_circle_fill_basic);
    EGUI_TEST_RUN(test_image_svg_ellipse_fill_basic);
    EGUI_TEST_RUN(test_image_svg_nested_svg_viewbox_scales_content);
    EGUI_TEST_RUN(test_image_svg_nested_svg_without_viewbox_translates_content);
    EGUI_TEST_RUN(test_image_svg_polygon_fill_basic);
    EGUI_TEST_RUN(test_image_svg_nested_svg_preserve_aspect_ratio_meet);
    EGUI_TEST_RUN(test_image_svg_nested_svg_preserve_aspect_ratio_none);
    EGUI_TEST_RUN(test_image_svg_nested_svg_preserve_aspect_ratio_none_nonuniform_circle_keeps_outer_edge_clean);
    EGUI_TEST_RUN(test_image_svg_nested_svg_visible_overflow_allows_paint_outside_viewport);
    EGUI_TEST_RUN(test_image_svg_nested_svg_preserve_aspect_ratio_slice_clips_overflow);
    EGUI_TEST_RUN(test_image_svg_nested_svg_viewbox_defaults_to_parent_viewport_size);
    EGUI_TEST_RUN(test_image_svg_nested_svg_viewbox_defaults_use_parent_viewbox_units);
    EGUI_TEST_RUN(test_image_svg_nested_svg_viewbox_accepts_percentage_viewport_attrs);
    EGUI_TEST_RUN(test_image_svg_group_opacity_multiplies_fill_opacity);
    EGUI_TEST_RUN(test_image_svg_rect_fill_opacity);
    EGUI_TEST_RUN(test_image_svg_rgb_function_fill);
    EGUI_TEST_RUN(test_image_svg_rgba_function_fill_uses_alpha);
    EGUI_TEST_RUN(test_image_svg_named_color_fill);
    EGUI_TEST_RUN(test_image_svg_negative_rect_size_is_skipped);
    EGUI_TEST_RUN(test_image_svg_negative_circle_radius_is_skipped);
    EGUI_TEST_RUN(test_image_svg_negative_ellipse_radius_is_skipped);
    EGUI_TEST_RUN(test_image_svg_root_negative_dimensions_with_viewbox_are_ignored);
    EGUI_TEST_RUN(test_image_svg_opacity_zero_skips_invalid_use_reference);
    EGUI_TEST_RUN(test_image_svg_opacity_zero_skips_unsupported_subtree);
    EGUI_TEST_RUN(test_image_svg_opacity_zero_skips_unsupported_leaf);
    EGUI_TEST_RUN(test_image_svg_display_none_skips_shape);
    EGUI_TEST_RUN(test_image_svg_display_none_skips_unsupported_leaf);
    EGUI_TEST_RUN(test_image_svg_display_none_skips_invalid_use_reference);
    EGUI_TEST_RUN(test_image_svg_display_none_skips_unsupported_subtree);
    EGUI_TEST_RUN(test_image_svg_visibility_hidden_skips_unsupported_leaf);
    EGUI_TEST_RUN(test_image_svg_visibility_hidden_skips_unsupported_subtree);
    EGUI_TEST_RUN(test_image_svg_visibility_hidden_skips_invalid_use_reference);
    EGUI_TEST_RUN(test_image_svg_visibility_hidden_skips_unsupported_shape_props);
    EGUI_TEST_RUN(test_image_svg_visibility_hidden_skips_unsupported_leaf_props);
    EGUI_TEST_RUN(test_image_svg_group_visibility_hidden_skips_children);
    EGUI_TEST_RUN(test_image_svg_visible_unsupported_element_is_skipped);
    EGUI_TEST_RUN(test_image_svg_invalid_use_reference_is_skipped);
    EGUI_TEST_RUN(test_image_svg_use_unsupported_reference_is_skipped);
    EGUI_TEST_RUN(test_image_svg_child_visibility_visible_overrides_hidden_parent);
    EGUI_TEST_RUN(test_image_svg_child_visibility_visible_cannot_override_display_none_parent);
    EGUI_TEST_RUN(test_image_svg_load_memory_len_without_null_terminator);
    EGUI_TEST_RUN(test_image_svg_rejects_oversized_root_dimensions);
    EGUI_TEST_RUN(test_image_svg_relative_smooth_cubic_path_fill);
    EGUI_TEST_RUN(test_image_svg_relative_smooth_cubic_path_fill_resize_preserves_curve_edge);
    EGUI_TEST_RUN(test_image_svg_relative_smooth_quad_path_fill);
    EGUI_TEST_RUN(test_image_svg_diagonal_edge_has_partial_aa_pixel);
    EGUI_TEST_RUN(test_image_svg_preserve_aspect_ratio_none_stretches);
    EGUI_TEST_RUN(test_image_svg_absolute_arc_circle_fill);
    EGUI_TEST_RUN(test_image_svg_relative_rotated_arc_ellipse_fill);
    EGUI_TEST_RUN(test_image_svg_invalid_arc_flag_is_skipped);
    EGUI_TEST_RUN(test_image_svg_rect_stroke_only);
    EGUI_TEST_RUN(test_image_svg_rect_stroke_opacity);
    EGUI_TEST_RUN(test_image_svg_open_path_stroke_does_not_close);
    EGUI_TEST_RUN(test_image_svg_fill_and_stroke_render_together);
    EGUI_TEST_RUN(test_image_svg_polyline_fill_closes_shape);
    EGUI_TEST_RUN(test_image_svg_line_stroke_only);
    EGUI_TEST_RUN(test_image_svg_line_square_cap_extends_corners);
    EGUI_TEST_RUN(test_image_svg_polyline_round_join_fills_outer_corner);
    EGUI_TEST_RUN(test_image_svg_polyline_bevel_join_keeps_corner_cut);
    EGUI_TEST_RUN(test_image_svg_polyline_miter_join_forms_sharp_corner);
    EGUI_TEST_RUN(test_image_svg_polyline_miter_join_respects_miterlimit);
    EGUI_TEST_RUN(test_image_svg_rounded_rect_fill);
    EGUI_TEST_RUN(test_image_svg_rounded_rect_stroke);
    EGUI_TEST_RUN(test_image_svg_rounded_rect_stroke_fractional_resize);
    EGUI_TEST_RUN(test_image_svg_use_references_defs_shape);
    EGUI_TEST_RUN(test_image_svg_use_references_group_with_xlink_href);
    EGUI_TEST_RUN(test_image_svg_use_xlink_href_fractional_meet_keeps_top_edge_aa);
    EGUI_TEST_RUN(test_image_svg_use_references_nested_use);
    EGUI_TEST_RUN(test_image_svg_use_references_symbol);
    EGUI_TEST_RUN(test_image_svg_use_symbol_viewbox_scales_to_use_size);
    EGUI_TEST_RUN(test_image_svg_use_symbol_visible_overflow_allows_paint_outside_viewport);
    EGUI_TEST_RUN(test_image_svg_use_references_svg_viewbox);
    EGUI_TEST_RUN(test_image_svg_use_references_svg_visible_overflow_allows_paint_outside_viewport);
    EGUI_TEST_RUN(test_image_svg_use_references_svg_viewbox_honors_ref_xy);
    EGUI_TEST_RUN(test_image_svg_use_references_svg_without_viewbox_honors_ref_xy);
    EGUI_TEST_RUN(test_image_svg_use_symbol_viewbox_defaults_to_parent_viewport_size);
    EGUI_TEST_RUN(test_image_svg_use_references_svg_viewbox_defaults_to_parent_viewport_size);
    EGUI_TEST_RUN(test_image_svg_use_symbol_nested_svg_defaults_use_symbol_viewbox_units);
    EGUI_TEST_RUN(test_image_svg_symbol_is_not_rendered_directly);
    EGUI_TEST_SUITE_END();
}
