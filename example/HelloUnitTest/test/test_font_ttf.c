/**
 * Unit tests for egui_font_ttf_t (Runtime TTF font).
 *
 * Tests that can run without a real TTF file verify the API contract (NULL
 * safety, error return values, struct fields).
 * Tests that require a real TTF file load scripts/tools/build_in/DejaVuSans.ttf
 * from the project root directory.  These tests are skipped gracefully when
 * the file is not available.
 */

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_font_ttf.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if EGUI_CONFIG_FUNCTION_FONT_TTF

#include "font/egui_font_ttf.h"

/* ------------------------------------------------------------------ */
/* Tiny stub fallback font for testing the fallback chain              */
/* ------------------------------------------------------------------ */

static int s_stub_draw_called;
static int s_stub_measure_called;

static int stub_draw_string(const egui_font_t *self, egui_canvas_t *canvas, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color,
                            egui_alpha_t alpha)
{
    (void)self;
    (void)canvas;
    (void)string;
    (void)x;
    (void)y;
    (void)color;
    (void)alpha;
    s_stub_draw_called++;
    return 1;
}

static int stub_get_str_size(const egui_font_t *self, const void *string, uint8_t multi, egui_dim_t ls, egui_dim_t *w, egui_dim_t *h)
{
    (void)self;
    (void)string;
    (void)multi;
    (void)ls;
    if (w)
        *w = 8;
    if (h)
        *h = 12;
    s_stub_measure_called++;
    return 0;
}

static const egui_font_api_t s_stub_api = {
        .draw_string = stub_draw_string,
        .get_str_size = stub_get_str_size,
};
static egui_font_t s_stub_font = {.api = &s_stub_api, .res = NULL};

/* ------------------------------------------------------------------ */
/* TTF file loader helper (PC only)                                    */
/* ------------------------------------------------------------------ */

static uint8_t *s_ttf_buf = NULL;
static uint32_t s_ttf_size = 0;

/** Attempt to load the built-in DejaVuSans.ttf into a heap buffer. */
static int load_dejavu_font(void)
{
    if (s_ttf_buf != NULL)
    {
        return 1; /* Already loaded. */
    }
    const char *path = "scripts/tools/build_in/DejaVuSans.ttf";
    FILE *f = fopen(path, "rb");
    if (f == NULL)
    {
        return 0; /* File not available – skip file-dependent tests. */
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0)
    {
        fclose(f);
        return 0;
    }
    s_ttf_buf = (uint8_t *)malloc((size_t)sz);
    if (s_ttf_buf == NULL)
    {
        fclose(f);
        return 0;
    }
    s_ttf_size = (uint32_t)sz;
    if (fread(s_ttf_buf, 1, (size_t)sz, f) != (size_t)sz)
    {
        fclose(f);
        free(s_ttf_buf);
        s_ttf_buf = NULL;
        s_ttf_size = 0;
        return 0;
    }
    fclose(f);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Test helpers                                                        */
/* ------------------------------------------------------------------ */

static egui_color_int_t s_pfb[32 * 32];
static egui_canvas_t s_canvas;

static void setup_canvas(void)
{
    egui_core_t *core = uicode_get_core();
    memset(s_pfb, 0, sizeof(s_pfb));
    egui_region_t pfb_region = {{0, 0}, {32, 32}};
    egui_canvas_init(&s_canvas, core, s_pfb, &pfb_region);
}

/* ------------------------------------------------------------------ */
/* Tests: NULL / invalid argument safety                               */
/* ------------------------------------------------------------------ */

static void test_font_ttf_init_null_self(void)
{
    uint8_t dummy[4] = {0};
    int r = egui_font_ttf_init(NULL, dummy, sizeof(dummy), 16);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, r);
}

static void test_font_ttf_init_null_data(void)
{
    egui_font_ttf_t font;
    int r = egui_font_ttf_init(&font, NULL, 0, 16);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, r);
}

static void test_font_ttf_init_zero_size(void)
{
    egui_font_ttf_t font;
    uint8_t dummy[4] = {0};
    int r = egui_font_ttf_init(&font, dummy, 0, 16);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, r);
}

static void test_font_ttf_init_zero_height(void)
{
    egui_font_ttf_t font;
    uint8_t dummy[4] = {0x00};
    int r = egui_font_ttf_init(&font, dummy, sizeof(dummy), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, r);
}

static void test_font_ttf_init_garbage_data_fails(void)
{
    egui_font_ttf_t font;
    uint8_t garbage[64];
    memset(garbage, 0xAB, sizeof(garbage));
    int r = egui_font_ttf_init(&font, garbage, sizeof(garbage), 16);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, r);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)font.initialized);
}

static void test_font_ttf_set_fallback_null_self_is_safe(void)
{
    /* Must not crash. */
    egui_font_ttf_set_fallback(NULL, &s_stub_font);
    EGUI_TEST_ASSERT_TRUE(1);
}

/* ------------------------------------------------------------------ */
/* Tests: successful init from real TTF data                           */
/* ------------------------------------------------------------------ */

static void test_font_ttf_init_valid_succeeds(void)
{
    if (!load_dejavu_font())
    {
        /* Skip: font file not available in this build environment. */
        EGUI_TEST_ASSERT_TRUE(1);
        return;
    }
    egui_font_ttf_t font;
    int r = egui_font_ttf_init(&font, s_ttf_buf, s_ttf_size, 16);
    EGUI_TEST_ASSERT_EQUAL_INT(0, r);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)font.initialized);
    EGUI_TEST_ASSERT_TRUE(font.base.api != NULL);
    EGUI_TEST_ASSERT_TRUE(font.pixel_height == 16);
}

static void test_font_ttf_get_str_size_ascii(void)
{
    if (!load_dejavu_font())
    {
        EGUI_TEST_ASSERT_TRUE(1);
        return;
    }
    egui_font_ttf_t font;
    egui_font_ttf_init(&font, s_ttf_buf, s_ttf_size, 16);

    egui_dim_t w = 0, h = 0;
    font.base.api->get_str_size(&font.base, "Hi", 0, 0, &w, &h);

    EGUI_TEST_ASSERT_TRUE(w > 0);
    EGUI_TEST_ASSERT_TRUE(h > 0);
}

static void test_font_ttf_get_str_size_empty_string(void)
{
    if (!load_dejavu_font())
    {
        EGUI_TEST_ASSERT_TRUE(1);
        return;
    }
    egui_font_ttf_t font;
    egui_font_ttf_init(&font, s_ttf_buf, s_ttf_size, 16);

    egui_dim_t w = 99, h = 99;
    font.base.api->get_str_size(&font.base, "", 0, 0, &w, &h);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)w);
}

static void test_font_ttf_get_str_size_wider_for_longer_string(void)
{
    if (!load_dejavu_font())
    {
        EGUI_TEST_ASSERT_TRUE(1);
        return;
    }
    egui_font_ttf_t font;
    egui_font_ttf_init(&font, s_ttf_buf, s_ttf_size, 16);

    egui_dim_t w1 = 0, h1 = 0;
    egui_dim_t w2 = 0, h2 = 0;
    font.base.api->get_str_size(&font.base, "A", 0, 0, &w1, &h1);
    font.base.api->get_str_size(&font.base, "AAAA", 0, 0, &w2, &h2);

    EGUI_TEST_ASSERT_TRUE(w2 > w1);
}

static void test_font_ttf_draw_string_does_not_crash(void)
{
    if (!load_dejavu_font())
    {
        EGUI_TEST_ASSERT_TRUE(1);
        return;
    }
    egui_font_ttf_t font;
    egui_font_ttf_init(&font, s_ttf_buf, s_ttf_size, 16);
    setup_canvas();

    int r = font.base.api->draw_string(&font.base, &s_canvas, "Hi", 0, 0, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    EGUI_TEST_ASSERT_TRUE(r >= 0);
}

static void test_font_ttf_draw_null_canvas_returns_zero(void)
{
    if (!load_dejavu_font())
    {
        EGUI_TEST_ASSERT_TRUE(1);
        return;
    }
    egui_font_ttf_t font;
    egui_font_ttf_init(&font, s_ttf_buf, s_ttf_size, 16);

    int r = font.base.api->draw_string(&font.base, NULL, "Hi", 0, 0, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    EGUI_TEST_ASSERT_EQUAL_INT(0, r);
}

/* ------------------------------------------------------------------ */
/* Tests: fallback chain                                               */
/* ------------------------------------------------------------------ */

static void test_font_ttf_set_fallback_stored(void)
{
    if (!load_dejavu_font())
    {
        EGUI_TEST_ASSERT_TRUE(1);
        return;
    }
    egui_font_ttf_t font;
    egui_font_ttf_init(&font, s_ttf_buf, s_ttf_size, 16);

    EGUI_TEST_ASSERT_TRUE(font.fallback == NULL);
    egui_font_ttf_set_fallback(&font, &s_stub_font);
    EGUI_TEST_ASSERT_TRUE(font.fallback == &s_stub_font);
}

static void test_font_ttf_set_fallback_clear_with_null(void)
{
    if (!load_dejavu_font())
    {
        EGUI_TEST_ASSERT_TRUE(1);
        return;
    }
    egui_font_ttf_t font;
    egui_font_ttf_init(&font, s_ttf_buf, s_ttf_size, 16);
    egui_font_ttf_set_fallback(&font, &s_stub_font);
    egui_font_ttf_set_fallback(&font, NULL);
    EGUI_TEST_ASSERT_TRUE(font.fallback == NULL);
}

/* ------------------------------------------------------------------ */
/* Tests: uninitialised font safety                                    */
/* ------------------------------------------------------------------ */

static void test_font_ttf_draw_uninit_is_safe(void)
{
    egui_font_ttf_t font;
    memset(&font, 0, sizeof(font));
    font.base.api = &egui_font_ttf_t_api_table;

    setup_canvas();
    int r = egui_font_ttf_t_api_table.draw_string(&font.base, &s_canvas, "X", 0, 0, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    EGUI_TEST_ASSERT_EQUAL_INT(0, r);
}

static void test_font_ttf_measure_uninit_returns_zero_size(void)
{
    egui_font_ttf_t font;
    memset(&font, 0, sizeof(font));
    font.base.api = &egui_font_ttf_t_api_table;

    egui_dim_t w = 99, h = 99;
    egui_font_ttf_t_api_table.get_str_size(&font.base, "X", 0, 0, &w, &h);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)w);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)h);
}

/* ------------------------------------------------------------------ */
/* Test runner                                                         */
/* ------------------------------------------------------------------ */

void test_font_ttf_run(void)
{
    EGUI_TEST_SUITE_BEGIN(font_ttf);

    EGUI_TEST_CASE(test_font_ttf_init_null_self);
    EGUI_TEST_CASE(test_font_ttf_init_null_data);
    EGUI_TEST_CASE(test_font_ttf_init_zero_size);
    EGUI_TEST_CASE(test_font_ttf_init_zero_height);
    EGUI_TEST_CASE(test_font_ttf_init_garbage_data_fails);
    EGUI_TEST_CASE(test_font_ttf_set_fallback_null_self_is_safe);
    EGUI_TEST_CASE(test_font_ttf_init_valid_succeeds);
    EGUI_TEST_CASE(test_font_ttf_get_str_size_ascii);
    EGUI_TEST_CASE(test_font_ttf_get_str_size_empty_string);
    EGUI_TEST_CASE(test_font_ttf_get_str_size_wider_for_longer_string);
    EGUI_TEST_CASE(test_font_ttf_draw_string_does_not_crash);
    EGUI_TEST_CASE(test_font_ttf_draw_null_canvas_returns_zero);
    EGUI_TEST_CASE(test_font_ttf_set_fallback_stored);
    EGUI_TEST_CASE(test_font_ttf_set_fallback_clear_with_null);
    EGUI_TEST_CASE(test_font_ttf_draw_uninit_is_safe);
    EGUI_TEST_CASE(test_font_ttf_measure_uninit_returns_zero_size);

    EGUI_TEST_SUITE_END();

    if (s_ttf_buf != NULL)
    {
        free(s_ttf_buf);
        s_ttf_buf = NULL;
        s_ttf_size = 0;
    }
}

#else /* !EGUI_CONFIG_FUNCTION_FONT_TTF */

void test_font_ttf_run(void)
{
    /* Feature disabled – no tests. */
}

#endif /* EGUI_CONFIG_FUNCTION_FONT_TTF */
