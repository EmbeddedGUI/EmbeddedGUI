/**
 * @file egui_diag.c
 * @brief Cross-platform diagnostic probes for compatibility-sensitive paths.
 *
 * Why: TC32 GCC 4.5.1 (Telink) sometimes renders fonts
 * incorrectly while rect
 * fill looks fine. This file lets the customer call a single function on the
 * board, prints "expected vs actual" lines for
 * compatibility risk points we
 * have already hit (struct layout, bitfield order, signed-shift in the fast
 * RGB565 blend, alpha tables, packed-glyph inner
 * loops, format-flag config),
 * and a final pass/fail summary.
 * NOTE about formatting: the customer's serial logger truncates lines at ~100 characters
 * (incl. "[file:line] [TAG] " prefix). Every probe line is therefore kept short and prints just enough to compare two boards.
 *
 * Reusable: drop-in for any future port. Just call egui_diag_run_all().
 */

#include <stdint.h>
#include <stddef.h>

#include "egui_diag.h"
#include "core/egui_api.h"
#include "core/egui_common.h"
#include "font/egui_font_std.h"

#define EGUI_DIAG_TAG      "DIAG"
#define DIAG_LOG(fmt, ...) egui_api_log("[" EGUI_DIAG_TAG "] " fmt "\n", ##__VA_ARGS__)

#define DIAG_CHECK_U(_label, _expected, _actual)                                                                                                               \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        unsigned long _e = (unsigned long)(_expected);                                                                                                         \
        unsigned long _a = (unsigned long)(_actual);                                                                                                           \
        if (_e == _a)                                                                                                                                          \
        {                                                                                                                                                      \
            DIAG_LOG("OK %s e=%lx a=%lx", _label, _e, _a);                                                                                                     \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            DIAG_LOG("KO %s e=%lx a=%lx", _label, _e, _a);                                                                                                     \
            fail_count++;                                                                                                                                      \
        }                                                                                                                                                      \
    } while (0)

#define DIAG_CHECK_I(_label, _expected, _actual)                                                                                                               \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        long _e = (long)(_expected);                                                                                                                           \
        long _a = (long)(_actual);                                                                                                                             \
        if (_e == _a)                                                                                                                                          \
        {                                                                                                                                                      \
            DIAG_LOG("OK %s e=%ld a=%ld", _label, _e, _a);                                                                                                     \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            DIAG_LOG("KO %s e=%ld a=%ld", _label, _e, _a);                                                                                                     \
            fail_count++;                                                                                                                                      \
        }                                                                                                                                                      \
    } while (0)

#if EGUI_CONFIG_COLOR_DEPTH == 16
static uint16_t egui_diag_rgb565_mix_fast(uint16_t bg, uint16_t fg, egui_alpha_t alpha)
{
    uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
    uint32_t fg_rb_g = (fg | ((uint32_t)fg << 16)) & 0x07E0F81FUL;
    uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)alpha >> 3) >> 5)) & 0x07E0F81FUL;

    return (uint16_t)(result | (result >> 16));
}

static int32_t egui_diag_rgb565_mix_div32(int32_t value)
{
    if (value >= 0)
    {
        return value / 32;
    }

    return -(((-value) + 31) / 32);
}

static uint16_t egui_diag_rgb565_mix_safe(uint16_t bg, uint16_t fg, egui_alpha_t alpha)
{
    int32_t alpha5 = (int32_t)((uint32_t)alpha >> 3);
    int32_t bg_r = (int32_t)((bg >> 11) & 0x1F);
    int32_t bg_g = (int32_t)((bg >> 5) & 0x3F);
    int32_t bg_b = (int32_t)(bg & 0x1F);
    int32_t fg_r = (int32_t)((fg >> 11) & 0x1F);
    int32_t fg_g = (int32_t)((fg >> 5) & 0x3F);
    int32_t fg_b = (int32_t)(fg & 0x1F);
    uint16_t r = (uint16_t)(bg_r + egui_diag_rgb565_mix_div32((fg_r - bg_r) * alpha5));
    uint16_t g = (uint16_t)(bg_g + egui_diag_rgb565_mix_div32((fg_g - bg_g) * alpha5));
    uint16_t b = (uint16_t)(bg_b + egui_diag_rgb565_mix_div32((fg_b - bg_b) * alpha5));

    return (uint16_t)((r << 11) | (g << 5) | b);
}
#endif

/* ------------------------------------------------------------------ *
 * Probe 1: platform basics (sizes, signedness, endianness)
 *
 * ------------------------------------------------------------------ */
int egui_diag_probe_platform(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_platform ==");

    DIAG_LOG("sz c=%u s=%u i=%u l=%u p=%u", (unsigned)sizeof(char), (unsigned)sizeof(short), (unsigned)sizeof(int), (unsigned)sizeof(long),
             (unsigned)sizeof(void *));
    DIAG_CHECK_U("sz_u8", 1u, sizeof(uint8_t));
    DIAG_CHECK_U("sz_u16", 2u, sizeof(uint16_t));
    DIAG_CHECK_U("sz_u32", 4u, sizeof(uint32_t));

    {
        char c = (char)0xFF;
        DIAG_LOG("char_signed=%d (0xFF=%d)", (c < 0) ? 1 : 0, (int)c);
    }

    /* Little-endian assumption (PFB / glyph buffers). */
    {
        uint16_t v = 0x1234;
        uint8_t first = ((const uint8_t *)&v)[0];
        DIAG_CHECK_U("LE_first", 0x34u, first);
    }

    /* Arithmetic right shift on signed int (rgb565_mix_div32 relies on it). */
    DIAG_CHECK_I("sshr", -16, ((int32_t)-32) >> 1);

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 2: bitfield layout for egui_color_rgb565_t
 * ------------------------------------------------------------------ */
int egui_diag_probe_bitfield(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_bitfield ==");

#if EGUI_CONFIG_COLOR_DEPTH == 16
    egui_color_t c;

    c.full = 0;
    c.color.red = 0x1F;
    DIAG_CHECK_U("R31", 0xF800u, c.full);

    c.full = 0;
    c.color.green = 0x3F;
    DIAG_CHECK_U("G63", 0x07E0u, c.full);

    c.full = 0;
    c.color.blue = 0x1F;
    DIAG_CHECK_U("B31", 0x001Fu, c.full);

    c.full = 0xF800;
    DIAG_CHECK_U("F800.r", 0x1Fu, c.color.red);
    DIAG_CHECK_U("F800.g", 0x00u, c.color.green);
    DIAG_CHECK_U("F800.b", 0x00u, c.color.blue);
#else
    DIAG_LOG("skip (CD!=16)");
#endif

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 3: font descriptor struct sizes
 * ------------------------------------------------------------------ */
int egui_diag_probe_struct_layout(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_struct ==");

    DIAG_CHECK_U("sz_code_desc", 4u, sizeof(egui_font_std_code_descriptor_t));
    DIAG_CHECK_U("sz_char_desc", 12u, sizeof(egui_font_std_char_descriptor_t));

    DIAG_LOG("offs idx=%u sz=%u w=%u h=%u av=%u ox=%u oy=%u", (unsigned)offsetof(egui_font_std_char_descriptor_t, idx),
             (unsigned)offsetof(egui_font_std_char_descriptor_t, size), (unsigned)offsetof(egui_font_std_char_descriptor_t, box_w),
             (unsigned)offsetof(egui_font_std_char_descriptor_t, box_h), (unsigned)offsetof(egui_font_std_char_descriptor_t, adv),
             (unsigned)offsetof(egui_font_std_char_descriptor_t, off_x), (unsigned)offsetof(egui_font_std_char_descriptor_t, off_y));

    /* Cross-check via a small const test instance laid out in flash. */
    {
        static const egui_font_std_char_descriptor_t s_probe = {
                .idx = 0xDEADBEEFu,
                .size = 0xC0DEu,
                .box_w = 0x12u,
                .box_h = 0x34u,
                .adv = 0x56u,
                .off_x = -2,
                .off_y = -3,
        };
        DIAG_CHECK_U("p.idx", 0xDEADBEEFu, s_probe.idx);
        DIAG_CHECK_U("p.size", 0xC0DEu, s_probe.size);
        DIAG_CHECK_U("p.bw", 0x12u, s_probe.box_w);
        DIAG_CHECK_U("p.bh", 0x34u, s_probe.box_h);
        DIAG_CHECK_U("p.adv", 0x56u, s_probe.adv);
        DIAG_CHECK_I("p.ox", -2, s_probe.off_x);
        DIAG_CHECK_I("p.oy", -3, s_probe.off_y);

        /* Read each byte to detect packing/padding mismatch with the host. */
        {
            const uint8_t *p = (const uint8_t *)&s_probe;
            DIAG_LOG("raw=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10],
                     p[11]);
        }
    }

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 4: alpha lookup tables. Sample first/mid/last to keep log short.
 * ------------------------------------------------------------------ */
int egui_diag_probe_alpha_tables(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_alpha_tables ==");

    DIAG_CHECK_U("a4[0]", 0u, egui_alpha_change_table_4[0]);
    DIAG_CHECK_U("a4[1]", 17u, egui_alpha_change_table_4[1]);
    DIAG_CHECK_U("a4[8]", 136u, egui_alpha_change_table_4[8]);
    DIAG_CHECK_U("a4[15]", 255u, egui_alpha_change_table_4[15]);
    DIAG_CHECK_U("a2[0]", 0u, egui_alpha_change_table_2[0]);
    DIAG_CHECK_U("a2[1]", 85u, egui_alpha_change_table_2[1]);
    DIAG_CHECK_U("a2[3]", 255u, egui_alpha_change_table_2[3]);

    {
        const uint8_t *p = egui_alpha_change_table_4;
        DIAG_LOG("a4 raw=%02x %02x %02x .. %02x %02x", p[0], p[1], p[2], p[14], p[15]);
    }

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 5: alpha mix scalar
 *
 * Formula: (a*b + 128) >> 8, with shortcut when either side is 255.
 * ------------------------------------------------------------------ */
int egui_diag_probe_alpha_mix(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_alpha_mix ==");

    DIAG_CHECK_U("am(255,255)", 255u, egui_color_alpha_mix(255, 255));
    DIAG_CHECK_U("am(255,128)", 128u, egui_color_alpha_mix(255, 128));
    DIAG_CHECK_U("am(128,255)", 128u, egui_color_alpha_mix(128, 255));
    DIAG_CHECK_U("am(128,128)", 64u, egui_color_alpha_mix(128, 128));
    DIAG_CHECK_U("am(0,128)", 0u, egui_color_alpha_mix(0, 128));
    DIAG_CHECK_U("am(17,255)", 17u, egui_color_alpha_mix(17, 255));
    DIAG_CHECK_U("am(17,128)", 9u, egui_color_alpha_mix(17, 128));
    DIAG_CHECK_U("am(238,238)", 221u, egui_color_alpha_mix(238, 238));
    DIAG_CHECK_U("am(136,255)", 136u, egui_color_alpha_mix(136, 255));
    DIAG_CHECK_U("am(136,136)", 72u, egui_color_alpha_mix(136, 136));

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 6: rgb565 blend.
 *
 * IMPORTANT: egui_diag_rgb565_mix_safe / egui_diag_rgb565_mix_fast do NOT short-circuit
 * at alpha == 255; the early-out lives only in the public
 * egui_rgb_mix() and the font path's blend_pixel_ctx wrappers. So at alpha=255 these helpers compute "31/32 of the way" toward fg, never quite reaching fg. We
 * assert against that very behaviour so a regression is detectable.
 * ------------------------------------------------------------------ */
int egui_diag_probe_rgb565_blend(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_rgb565_blend ==");

#if EGUI_CONFIG_COLOR_DEPTH == 16
    /* Cases that are well-defined for both safe and fast paths. */
    {
        DIAG_CHECK_U("WoB128_safe", 0x7BEFu, egui_diag_rgb565_mix_safe(0x0000u, 0xFFFFu, 128));
        DIAG_CHECK_U("WoB128_fast", 0x7BEFu, egui_diag_rgb565_mix_fast(0x0000u, 0xFFFFu, 128));
    }
    {
        DIAG_CHECK_U("BoR128_safe", 0x780Fu, egui_diag_rgb565_mix_safe(0xF800u, 0x001Fu, 128));
        DIAG_CHECK_U("BoR128_fast", 0x780Fu, egui_diag_rgb565_mix_fast(0xF800u, 0x001Fu, 128));
    }
    {
        DIAG_CHECK_U("rnd85_safe", 0x3AB1u, egui_diag_rgb565_mix_safe(0x1234u, 0xABCDu, 85));
        DIAG_LOG("rnd85_fast=%x", (unsigned)egui_diag_rgb565_mix_fast(0x1234u, 0xABCDu, 85));
    }

    /* Public API early-outs at alpha<4 / alpha>251 -> direct pass-through. */
    {
        egui_color_t bg, fg;
        bg.full = 0x1234u;
        fg.full = 0xABCDu;
        DIAG_CHECK_U("api_a0", 0x1234u, egui_rgb_mix(bg, fg, 0).full);
        DIAG_CHECK_U("api_a3", 0x1234u, egui_rgb_mix(bg, fg, 3).full);
        DIAG_CHECK_U("api_a252", 0xABCDu, egui_rgb_mix(bg, fg, 252).full);
        DIAG_CHECK_U("api_a255", 0xABCDu, egui_rgb_mix(bg, fg, 255).full);
    }

    /* The safe/fast helpers themselves at alpha=255 - golden value below is
     * what the algorithm SHOULD produce on every ISA when there is no early-out. */
    {
        uint16_t safe = egui_diag_rgb565_mix_safe(0x1234u, 0xABCDu, 255);
        uint16_t fast = egui_diag_rgb565_mix_fast(0x1234u, 0xABCDu, 255);
        DIAG_CHECK_U("safe_a255", 0xA3ADu, safe);
        DIAG_LOG("fast_a255=%x", (unsigned)fast);
    }

    /* Sweep table_4 alphas: FG=red over BG=black. Result red channel must
     * monotonically increase. */
    {
        uint16_t prev = 0;
        for (int i = 0; i < 16; i++)
        {
            uint16_t v = egui_diag_rgb565_mix_safe(0x0000u, 0xF800u, egui_alpha_change_table_4[i]);
            DIAG_LOG("sw[%2d] a=%3u v=%04x", i, (unsigned)egui_alpha_change_table_4[i], (unsigned)v);
            if (v < prev)
            {
                DIAG_LOG("KO sw_mono i=%d", i);
                fail_count++;
            }
            prev = v;
        }
    }
#else
    DIAG_LOG("skip (CD!=16)");
#endif

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 7: 4bpp packed-glyph blend, mirrors egui_font_std_draw_fast_4_ctx.
 * ------------------------------------------------------------------ */
int egui_diag_probe_glyph_4bpp(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_glyph_4bpp ==");

#if EGUI_CONFIG_COLOR_DEPTH == 16
    /* 4-pixel row, packed 4bpp, low nibble first.
     * pixels: [0]=0x0F, [1]=0x08, [2]=0x0F, [3]=0x00. */
    const uint8_t glyph_row[2] = {(uint8_t)((0x08 << 4) | 0x0F), (uint8_t)((0x00 << 4) | 0x0F)};

    egui_color_int_t dst[4];
    dst[0] = 0x0000u;
    dst[1] = 0x0000u;
    dst[2] = 0xFFFFu;
    dst[3] = 0x07E0u;

    egui_color_t fg;
    fg.full = 0xF800u;

    {
        const uint8_t *src = glyph_row;
        uint8_t packed;
        uint8_t a0, a1;

        packed = src[0];
        a0 = (uint8_t)(packed & 0x0F);
        a1 = (uint8_t)((packed >> 4) & 0x0F);
        if (a0 == 0x0F)
            dst[0] = fg.full;
        else if (a0)
        {
            egui_color_t b;
            b.full = dst[0];
            dst[0] = egui_rgb_mix(b, fg, egui_alpha_change_table_4[a0]).full;
        }
        if (a1 == 0x0F)
            dst[1] = fg.full;
        else if (a1)
        {
            egui_color_t b;
            b.full = dst[1];
            dst[1] = egui_rgb_mix(b, fg, egui_alpha_change_table_4[a1]).full;
        }

        packed = src[1];
        a0 = (uint8_t)(packed & 0x0F);
        a1 = (uint8_t)((packed >> 4) & 0x0F);
        if (a0 == 0x0F)
            dst[2] = fg.full;
        else if (a0)
        {
            egui_color_t b;
            b.full = dst[2];
            dst[2] = egui_rgb_mix(b, fg, egui_alpha_change_table_4[a0]).full;
        }
        if (a1 == 0x0F)
            dst[3] = fg.full;
        else if (a1)
        {
            egui_color_t b;
            b.full = dst[3];
            dst[3] = egui_rgb_mix(b, fg, egui_alpha_change_table_4[a1]).full;
        }
    }

    DIAG_LOG("dst=%04x %04x %04x %04x", (unsigned)dst[0], (unsigned)dst[1], (unsigned)dst[2], (unsigned)dst[3]);
    DIAG_CHECK_U("d0", 0xF800u, dst[0]);
    {
        uint16_t expected = egui_diag_rgb565_mix_safe(0x0000u, 0xF800u, egui_alpha_change_table_4[0x08]);
        DIAG_CHECK_U("d1", expected, dst[1]);
    }
    DIAG_CHECK_U("d2", 0xF800u, dst[2]);
    DIAG_CHECK_U("d3", 0x07E0u, dst[3]);
#else
    DIAG_LOG("skip (CD!=16)");
#endif

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 8: font format / config flags (informational).
 *
 * If a build accidentally drops the right format flag, descriptors look fine
 * but glyphs are never drawn at all.
 * ------------------------------------------------------------------ */
int egui_diag_probe_format_flags(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_format_flags ==");

#ifdef EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    DIAG_LOG("FMT1=%d", (int)EGUI_CONFIG_FUNCTION_FONT_FORMAT_1);
#else
    DIAG_LOG("FMT1=undef");
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    DIAG_LOG("FMT2=%d", (int)EGUI_CONFIG_FUNCTION_FONT_FORMAT_2);
#else
    DIAG_LOG("FMT2=undef");
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    DIAG_LOG("FMT4=%d", (int)EGUI_CONFIG_FUNCTION_FONT_FORMAT_4);
#else
    DIAG_LOG("FMT4=undef");
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    DIAG_LOG("FMT8=%d", (int)EGUI_CONFIG_FUNCTION_FONT_FORMAT_8);
#else
    DIAG_LOG("FMT8=undef");
#endif

#ifdef EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW
    DIAG_LOG("FAST_DRAW=%d", (int)EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW);
#endif

    DIAG_LOG("sz_font_info=%u", (unsigned)sizeof(egui_font_std_info_t));

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 9: descriptor table indexing / binary search.
 *
 * If TC32 mis-aligns const arrays in flash, indexing code_array[i].code or
 * char_array[i].idx returns garbage and the bitmap pointer is wrong -> font
 * draws random noise even though the per-pixel math is fine.
 * ------------------------------------------------------------------ */
int egui_diag_probe_descriptor_lookup(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_desc_lookup ==");

    static const egui_font_std_code_descriptor_t s_codes[6] = {
            {0x00000020u}, {0x00000041u}, {0x00000061u}, {0x000000ABu}, {0x0000ABCDu}, {0xDEADBEEFu},
    };

    static const egui_font_std_char_descriptor_t s_chars[6] = {
            {.idx = 0x00010000u, .size = 16, .box_w = 8, .box_h = 16, .adv = 8, .off_x = 0, .off_y = 0},
            {.idx = 0x00020000u, .size = 32, .box_w = 16, .box_h = 16, .adv = 16, .off_x = 1, .off_y = 1},
            {.idx = 0x00030000u, .size = 64, .box_w = 16, .box_h = 32, .adv = 16, .off_x = -1, .off_y = -1},
            {.idx = 0x00040000u, .size = 128, .box_w = 24, .box_h = 24, .adv = 24, .off_x = 2, .off_y = -3},
            {.idx = 0xCAFEBABEu, .size = 256, .box_w = 32, .box_h = 32, .adv = 32, .off_x = 4, .off_y = -5},
            {.idx = 0xDEAD0000u, .size = 512, .box_w = 48, .box_h = 48, .adv = 48, .off_x = -7, .off_y = 9},
    };

    DIAG_CHECK_U("c[0]", 0x20u, s_codes[0].code);
    DIAG_CHECK_U("c[4]", 0xABCDu, s_codes[4].code);
    DIAG_CHECK_U("c[5]", 0xDEADBEEFu, s_codes[5].code);

    DIAG_CHECK_U("d[0].idx", 0x00010000u, s_chars[0].idx);
    DIAG_CHECK_U("d[4].idx", 0xCAFEBABEu, s_chars[4].idx);
    DIAG_CHECK_U("d[5].idx", 0xDEAD0000u, s_chars[5].idx);
    DIAG_CHECK_U("d[3].sz", 128u, s_chars[3].size);
    DIAG_CHECK_U("d[3].adv", 24u, s_chars[3].adv);
    DIAG_CHECK_I("d[3].ox", 2, s_chars[3].off_x);
    DIAG_CHECK_I("d[3].oy", -3, s_chars[3].off_y);

    /* Pointer stride must equal sizeof(struct). */
    {
        unsigned long stride = (unsigned long)((uintptr_t)&s_chars[1] - (uintptr_t)&s_chars[0]);
        DIAG_CHECK_U("ch_stride", (unsigned long)sizeof(egui_font_std_char_descriptor_t), stride);
    }
    {
        unsigned long stride = (unsigned long)((uintptr_t)&s_codes[1] - (uintptr_t)&s_codes[0]);
        DIAG_CHECK_U("cd_stride", (unsigned long)sizeof(egui_font_std_code_descriptor_t), stride);
    }

    /* Binary search for 0xABCD - matches the runtime path. */
    {
        uint32_t target = 0x0000ABCDu;
        int lo = 0, hi = (int)(sizeof(s_codes) / sizeof(s_codes[0])) - 1;
        int found = -1;
        while (lo <= hi)
        {
            int mid = (lo + hi) >> 1;
            uint32_t k = s_codes[mid].code;
            if (k == target)
            {
                found = mid;
                break;
            }
            if (k < target)
                lo = mid + 1;
            else
                hi = mid - 1;
        }
        DIAG_CHECK_I("bsearch", 4, found);
    }

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 10: 1bpp glyph row blend.
 * ------------------------------------------------------------------ */
int egui_diag_probe_glyph_1bpp(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_glyph_1bpp ==");

#if EGUI_CONFIG_COLOR_DEPTH == 16
    const uint8_t row = 0x55u; /* bit0=1, bit1=0, ... */
    egui_color_int_t dst[8];
    for (int i = 0; i < 8; i++)
        dst[i] = 0x0000u;

    egui_color_t fg;
    fg.full = 0xF800u;

    /* Replicate egui_font_std_draw_fast_1 inner test. */
    for (int col = 0; col < 8; col++)
    {
        if ((row >> (col & 7)) & 0x01)
        {
            dst[col] = fg.full;
        }
    }

    DIAG_LOG("d=%04x %04x %04x %04x %04x %04x %04x %04x", (unsigned)dst[0], (unsigned)dst[1], (unsigned)dst[2], (unsigned)dst[3], (unsigned)dst[4],
             (unsigned)dst[5], (unsigned)dst[6], (unsigned)dst[7]);
    DIAG_CHECK_U("d0", 0xF800u, dst[0]);
    DIAG_CHECK_U("d1", 0x0000u, dst[1]);
    DIAG_CHECK_U("d2", 0xF800u, dst[2]);
    DIAG_CHECK_U("d3", 0x0000u, dst[3]);
    DIAG_CHECK_U("d4", 0xF800u, dst[4]);
    DIAG_CHECK_U("d5", 0x0000u, dst[5]);
    DIAG_CHECK_U("d6", 0xF800u, dst[6]);
    DIAG_CHECK_U("d7", 0x0000u, dst[7]);
#else
    DIAG_LOG("skip (CD!=16)");
#endif

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 11: bitmap pointer arithmetic.
 *   const uint8_t *bitmap = font->bitmap_data + char_desc->idx;
 * If `idx` (uint32) is read with wrong endianness/alignment, or if pointer
 * + uint32 wraps differently on TC32, every bitmap is wrong.
 * ------------------------------------------------------------------ */
int egui_diag_probe_bitmap_ptr(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_bitmap_ptr ==");

    /* 256-byte synthetic bitmap "rom": each byte = its index. */
    static const uint8_t s_rom[256] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
            0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
            0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
            0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
            0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
            0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
            0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
            0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
            0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
            0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
    };

    struct
    {
        uint32_t idx;
    } s_desc = {0u};

    s_desc.idx = 0u;
    DIAG_CHECK_U("rom[0]", 0x00u, s_rom[s_desc.idx]);
    s_desc.idx = 1u;
    DIAG_CHECK_U("rom[1]", 0x01u, s_rom[s_desc.idx]);
    s_desc.idx = 0x55u;
    DIAG_CHECK_U("rom[55]", 0x55u, s_rom[s_desc.idx]);
    s_desc.idx = 0xFFu;
    DIAG_CHECK_U("rom[ff]", 0xFFu, s_rom[s_desc.idx]);

    {
        const uint8_t *base = s_rom;
        uint32_t off = 0xAAu;
        const uint8_t *p = base + off;
        DIAG_CHECK_U("ptr_add", 0xAAu, *p);
    }

    /* Read uint32 little-endian from rom (mirrors how flash idx is laid out). */
    {
        const uint8_t *p = s_rom + 4;
        uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
        DIAG_CHECK_U("rom_u32_LE", 0x07060504u, v);
    }

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 12: full sweep of egui_alpha_change_table_4.
 *
 * Existing probe only spot-checks 4 entries. Real glyph bytes use any of
 * 0..15 - this catches a partially-corrupted table (e.g. flash read-back
 * issue or wrong section linkage on TC32).
 * ------------------------------------------------------------------ */
int egui_diag_probe_alpha_table_full(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_alpha_table_full ==");

    for (int i = 0; i < 16; i++)
    {
        unsigned exp = (unsigned)(i * 17);
        unsigned act = (unsigned)egui_alpha_change_table_4[i];
        if (exp != act)
        {
            DIAG_LOG("KO a4[%d] e=%x a=%x", i, exp, act);
            fail_count++;
        }
    }
    if (fail_count == 0)
    {
        DIAG_LOG("OK a4_full 16x");
    }

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 13: split a real glyph byte into two 4-bit indices.
 *
 * 'H' first row in HelloSimple2's font: 0xe8 -> low=8, high=14. Forces the
 * shift / mask path that draw_4 uses, with a non-symmetrical byte (different
 * from the all-0x0F earlier probe).
 * ------------------------------------------------------------------ */
int egui_diag_probe_real_glyph_byte(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_real_glyph_byte ==");

    /* Force the value through a volatile to defeat any constant folding. */
    volatile uint8_t b = (uint8_t)0xE8u;
    DIAG_CHECK_U("nib_lo", 0x08u, (unsigned)(b & 0x0F));
    DIAG_CHECK_U("nib_hi", 0x0Eu, (unsigned)((b >> 4) & 0x0F));
    DIAG_CHECK_U("a4[8]", 136u, (unsigned)egui_alpha_change_table_4[8]);
    DIAG_CHECK_U("a4[14]", 238u, (unsigned)egui_alpha_change_table_4[14]);

    /* Same nibble extraction in the exact form used by egui_font_std_draw_4
     * (sel_data >> bit_pos) & 0x0F with bit_pos either 0 or 4. */
    {
        uint8_t sel_data = b;
        uint8_t bit_pos = 0;
        DIAG_CHECK_U("draw4_lo", 0x08u, (unsigned)((sel_data >> bit_pos) & 0x0F));
        bit_pos = 4;
        DIAG_CHECK_U("draw4_hi", 0x0Eu, (unsigned)((sel_data >> bit_pos) & 0x0F));
    }

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 14: end-to-end inner loop on the real first row of letter 'H'.
 *
 * This replicates egui_font_std_draw_4 exactly (FAST_DRAW=0 path that the
 * customer build is using) and writes into a stack PFB buffer. Expected
 * values are derived from the same egui_rgb_mix() the real renderer calls,
 * so the probe self-consistency cannot drift.
 *
 * 'H' glyph (HelloSimple2, 14px / 4bpp, box_w=9, box_h=10):
 *   first row bytes: 0xe8 0x00 0x00 0x80 0x0e
 *   nibbles  : 8 14   0 0   0 0   0 8   14
 *   pixels   : [a=8] [a=14] [0] [0] [0] [0] [0] [a=8] [a=14]
 * ------------------------------------------------------------------ */
int egui_diag_probe_real_glyph_row(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_real_glyph_row ==");

#if EGUI_CONFIG_COLOR_DEPTH == 16
    static const uint8_t s_h_row0[5] = {0xE8u, 0x00u, 0x00u, 0x80u, 0x0Eu};
    egui_color_int_t pfb[9];
    for (int i = 0; i < 9; i++)
    {
        pfb[i] = 0x0000u; /* black background */
    }

    egui_color_t fg;
    fg.full = 0xF800u; /* red foreground */

    /* Mirror of egui_font_std_draw_4 inner body. */
    {
        const uint8_t *p_data = s_h_row0;
        uint8_t bit_pos = 0;
        uint8_t sel_data = 0;
        uint8_t sel_value;
        for (int x_ = 0; x_ < 9; x_++)
        {
            if (bit_pos == 0)
            {
                sel_data = *p_data++;
            }
            sel_value = (uint8_t)((sel_data >> bit_pos) & 0x0F);
            if (sel_value)
            {
                /* Same public code path as egui_canvas_draw_point's alpha<255 branch. */
                egui_color_t b;
                b.full = pfb[x_];
                pfb[x_] = egui_rgb_mix(b, fg, egui_alpha_change_table_4[sel_value]).full;
            }
            bit_pos = (uint8_t)(bit_pos + 4);
            if (bit_pos == 8)
            {
                bit_pos = 0;
            }
        }
    }

    DIAG_LOG("h_row=%04x %04x %04x %04x %04x", (unsigned)pfb[0], (unsigned)pfb[1], (unsigned)pfb[2], (unsigned)pfb[3], (unsigned)pfb[4]);
    DIAG_LOG("h_row=%04x %04x %04x %04x", (unsigned)pfb[5], (unsigned)pfb[6], (unsigned)pfb[7], (unsigned)pfb[8]);

    /* Expected values: derive from the same library helpers so the probe
     * stays consistent with whatever the build configures (safe vs fast). */
    {
        egui_color_t bg, fgc;
        bg.full = 0x0000u;
        fgc.full = 0xF800u;
        egui_color_t e_a8 = egui_rgb_mix(bg, fgc, egui_alpha_change_table_4[8]);
        egui_color_t e_a14 = egui_rgb_mix(bg, fgc, egui_alpha_change_table_4[14]);

        DIAG_CHECK_U("h0", e_a8.full, pfb[0]);
        DIAG_CHECK_U("h1", e_a14.full, pfb[1]);
        DIAG_CHECK_U("h2", 0x0000u, pfb[2]);
        DIAG_CHECK_U("h3", 0x0000u, pfb[3]);
        DIAG_CHECK_U("h4", 0x0000u, pfb[4]);
        DIAG_CHECK_U("h5", 0x0000u, pfb[5]);
        DIAG_CHECK_U("h6", 0x0000u, pfb[6]);
        DIAG_CHECK_U("h7", e_a8.full, pfb[7]);
        DIAG_CHECK_U("h8", e_a14.full, pfb[8]);
    }
#else
    DIAG_LOG("skip (CD!=16)");
#endif

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 15: egui_rgb_mix_ptr (the actual hot-path function used in
 * egui_canvas_draw_point and rect-fill alpha blending).
 *
 * The earlier rgb565 probe only covered the value-returning egui_rgb_mix.
 * The pointer variant has a separate body and is what runs in the inner
 * loops on the customer's TC32 build.
 * ------------------------------------------------------------------ */
int egui_diag_probe_rgb_mix_ptr(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_rgb_mix_ptr ==");

#if EGUI_CONFIG_COLOR_DEPTH == 16
    egui_color_t bg, fg, out;

    /* alpha == 255: must early-out to fg (the ptr variant has its own check). */
    bg.full = 0x1234u;
    fg.full = 0xABCDu;
    out.full = 0xDEADu;
    egui_rgb_mix_ptr(&bg, &fg, &out, 255);
    DIAG_CHECK_U("ptr_a255", 0xABCDu, out.full);

    /* alpha == 0: must early-out to bg. */
    bg.full = 0x1234u;
    fg.full = 0xABCDu;
    out.full = 0xDEADu;
    egui_rgb_mix_ptr(&bg, &fg, &out, 0);
    DIAG_CHECK_U("ptr_a0", 0x1234u, out.full);

    /* Mid alpha: must equal egui_rgb_mix() value-returning sibling. */
    bg.full = 0x0000u;
    fg.full = 0xF800u;
    out.full = 0xDEADu;
    egui_rgb_mix_ptr(&bg, &fg, &out, 128);
    {
        egui_color_t ref = egui_rgb_mix(bg, fg, 128);
        DIAG_CHECK_U("ptr_a128", ref.full, out.full);
    }

    /* In-place: out aliases bg (the canvas hot path uses this exact form). */
    {
        egui_color_t inout;
        egui_color_t fgL;
        inout.full = 0x0000u;
        fgL.full = 0xF800u;
        egui_rgb_mix_ptr(&inout, &fgL, &inout, egui_alpha_change_table_4[8]);
        {
            egui_color_t bgZ, ref;
            bgZ.full = 0x0000u;
            ref = egui_rgb_mix(bgZ, fgL, egui_alpha_change_table_4[8]);
            DIAG_CHECK_U("ptr_inplace", ref.full, inout.full);
        }
    }
#else
    DIAG_LOG("skip (CD!=16)");
#endif

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Probe 16: simulated PFB pixel write.
 *
 * Mirrors what egui_canvas_draw_point does at the bottom of the call chain
 * (offset arithmetic + uint16 store). We don't have a real canvas at probe
 * time, so we run the formula on a stack pfb[] of width=10. This catches:
 *   - mis-aligned uint16 store on TC32 (would shift result by 1 byte)
 *   - signed/unsigned issue with int16_t pos_y * width
 *   - alpha=255 fast path (direct assign vs rgb_mix_ptr)
 * ------------------------------------------------------------------ */
int egui_diag_probe_canvas_pfb_write(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_pfb_write ==");

#if EGUI_CONFIG_COLOR_DEPTH == 16
    enum
    {
        W = 10,
        H = 4
    };
    egui_color_int_t pfb[W * H];
    for (int i = 0; i < W * H; i++)
    {
        pfb[i] = 0x0000u;
    }

    /* Direct assign at alpha=255 (mirrors `*back_color = color` branch). */
    {
        int16_t pos_x = 3;
        int16_t pos_y = 2;
        egui_color_t color;
        color.full = 0xF800u;
        egui_color_t *back = (egui_color_t *)&pfb[(int)pos_y * W + pos_x];
        *back = color;

        DIAG_CHECK_U("idx_calc", (unsigned)(2 * W + 3), (unsigned)((int)pos_y * W + pos_x));
        DIAG_CHECK_U("pfb[23]", 0xF800u, pfb[23]);
        DIAG_CHECK_U("pfb[22]", 0x0000u, pfb[22]);
        DIAG_CHECK_U("pfb[24]", 0x0000u, pfb[24]);
    }

    /* Alpha-blend branch at the same address. */
    {
        int16_t pos_x = 5;
        int16_t pos_y = 1;
        egui_color_t color;
        color.full = 0xF800u;
        egui_color_t *back = (egui_color_t *)&pfb[(int)pos_y * W + pos_x];
        egui_rgb_mix_ptr(back, &color, back, 128);

        egui_color_t bg, ref;
        bg.full = 0x0000u;
        ref = egui_rgb_mix(bg, color, 128);
        DIAG_CHECK_U("pfb[15]", ref.full, pfb[15]);
    }

    /* Negative pos_y on int16_t: force signed math through the index. The
     * real canvas guards this with an in-rect check, but if TC32 ever
     * promotes int16 differently it would manifest here. We compute the
     * index and only assert the math, not store. */
    {
        int16_t pos_x = 7;
        int16_t pos_y = -3;
        long idx = (long)pos_y * W + pos_x;
        DIAG_CHECK_I("neg_idx", -23L, idx);
    }
#else
    DIAG_LOG("skip (CD!=16)");
#endif

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * ROUND 3: compiler/TC32-suspect probes (17..50)
 *
 * The first 16 probes already passed on the customer's TC32, so the
 * font/blending math is correct at the lowest level. The probes below
 * widen coverage to:
 *   - integer promotion / signed-vs-unsigned semantics
 *   - mis-aligned 16/32-bit memory access (very common TC32 pitfall)
 *   - struct field access via byte cast (LE assembly)
 *   - libc routines used implicitly (memcpy/memmove)
 *   - function-pointer indirection through a struct (used by render API)
 *   - exhaustive alpha sweeps of egui_color_alpha_mix and egui_rgb_mix
 *   - an entire 16-nibble glyph row with every alpha 0..15
 *   - pixel-overdraw / canvas+glyph alpha composition
 *   - signed int8 off_y arithmetic
 *   - advance-pixel summation for the actual "Hello World!" target string
 *   - array-of-struct iteration with 19 descriptors
 *   - pixel_buffer offset progression (idx[i+1]-idx[i] == size[i])
 *   - reading a struct field through a raw byte cast
 *   - alpha=0 hot-path early-out
 *   - egui_color_t size, egui_dim_t signedness
 *   - both alpha lookup tables full sweep
 *   - dump of every EGUI_CONFIG_* macro that influences font rendering
 * ------------------------------------------------------------------ */

/* probe 17: uint8 * uint8 must widen to int (>=16 bit) before multiply. */
int egui_diag_probe_int_promotion(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_int_promo ==");
    {
        volatile uint8_t a = 200u;
        volatile uint8_t b = 200u;
        unsigned p = (unsigned)(a * b);
        DIAG_CHECK_U("u8mul", 40000u, p);
    }
    {
        volatile uint16_t a = 1000u;
        volatile uint16_t b = 100u;
        unsigned long p = (unsigned long)((unsigned)a * (unsigned)b);
        DIAG_CHECK_U("u16mul", 100000ul, p);
    }
    {
        /* (uint8*uint8 + uint8) */
        volatile uint8_t a = 17u;
        volatile uint8_t b = 8u;
        volatile uint8_t c = 128u;
        unsigned r = (unsigned)((a * b + c) >> 8);
        DIAG_CHECK_U("u8mul_acc", (17u * 8u + 128u) >> 8, r);
    }
    return fail_count;
}

/* probe 18: signed-vs-unsigned compare. (-1) < 1u must hold *as signed*
 * if the unsigned operand is narrower than int, but on TC32 with int=4
 * this evaluates with promotion: -1 (int) compared to 1u (int after
 * promotion of uint8) -> still true. (signed int) -1 < (unsigned int) 1
 * -> false on most platforms (the -1 becomes 0xFFFFFFFF). */
int egui_diag_probe_signed_unsigned_cmp(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_su_cmp ==");
    {
        volatile int8_t s = (int8_t)-1;
        volatile uint8_t u = 1u;
        /* both promote to int -> -1 < 1 == 1 */
        DIAG_CHECK_I("s8_lt_u8", 1, (s < u) ? 1 : 0);
    }
    {
        volatile int s = -1;
        volatile unsigned u = 1u;
        /* -1 reinterpreted as unsigned -> very large -> 0 */
        DIAG_CHECK_I("sint_lt_uint", 0, (((unsigned)s) < u) ? 1 : 0);
    }
    {
        /* int16 negative compared to uint16 small */
        volatile int16_t s = (int16_t)-1;
        volatile uint16_t u = 1u;
        DIAG_CHECK_I("s16_lt_u16", 1, (s < u) ? 1 : 0);
    }
    return fail_count;
}

/* probe 19: int16 * int16 must widen to int32 before storing in long. */
int egui_diag_probe_int16_widen_mul(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_i16_mul ==");
    {
        volatile int16_t a = 300;
        volatile int16_t b = 300;
        long r = (long)((int32_t)a * (int32_t)b);
        DIAG_CHECK_I("pos*pos", 90000L, r);
    }
    {
        volatile int16_t a = -240;
        volatile int16_t b = 240;
        long r = (long)((int32_t)a * (int32_t)b);
        DIAG_CHECK_I("neg*pos", -57600L, r);
    }
    {
        /* simulates (pos_y * width) used in PFB index arithmetic */
        volatile int16_t y = -3;
        volatile int16_t w = 240;
        long r = (long)y * (long)w + 7;
        DIAG_CHECK_I("idx_calc", -713L, r);
    }
    return fail_count;
}

/* probe 20: signed division/modulo truncation toward zero (C99). */
int egui_diag_probe_div_signed(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_div_sgn ==");
    {
        volatile int x = -257;
        DIAG_CHECK_I("div", -1, x / 256);
        DIAG_CHECK_I("mod", -1, x % 256);
    }
    {
        volatile int x = 257;
        DIAG_CHECK_I("div_p", 1, x / 256);
        DIAG_CHECK_I("mod_p", 1, x % 256);
    }
    {
        volatile int x = -16;
        DIAG_CHECK_I("div_e", -1, x / 16);
    }
    return fail_count;
}

/* probe 21: unsigned wraparound at uint8/16 boundaries. */
int egui_diag_probe_uint_wrap(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_u_wrap ==");
    {
        volatile uint8_t a = 250u;
        volatile uint8_t b = 10u;
        uint8_t r = (uint8_t)(a + b);
        DIAG_CHECK_U("u8_wrap", 4u, r);
    }
    {
        volatile uint16_t a = 65000u;
        volatile uint16_t b = 1000u;
        uint16_t r = (uint16_t)(a + b);
        DIAG_CHECK_U("u16_wrap", 464u, r);
    }
    {
        volatile uint8_t a = 5u;
        volatile uint8_t b = 10u;
        uint8_t r = (uint8_t)(a - b);
        DIAG_CHECK_U("u8_neg", 251u, r);
    }
    return fail_count;
}

/* probe 22: (uint32_t)-1 >> 1 must be 0x7FFFFFFF (logical shift on unsigned). */
int egui_diag_probe_shift_unsigned_neg(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_shft_un ==");
    {
        volatile uint32_t x = (uint32_t)-1;
        DIAG_CHECK_U("u32m1_sr1", 0x7FFFFFFFul, x >> 1);
    }
    {
        volatile uint16_t x = (uint16_t)-1;
        /* x promotes to int (signed) before shift -> still 0x7FFF */
        DIAG_CHECK_U("u16m1_sr1", 0x7FFFu, (unsigned)(x >> 1));
    }
    {
        /* arithmetic right shift on signed int32 */
        volatile int32_t x = -1;
        DIAG_CHECK_I("s32m1_sr1", -1L, (long)(x >> 1));
    }
    return fail_count;
}

/* probe 23: high-count shifts. */
int egui_diag_probe_shift_high_count(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_shft_hi ==");
    {
        volatile unsigned x = 1u;
        DIAG_CHECK_U("u1_sl31", 0x80000000ul, (unsigned long)(x << 31));
    }
    {
        volatile unsigned x = 0xFFu;
        DIAG_CHECK_U("u8_sl24", 0xFF000000ul, (unsigned long)(x << 24));
    }
    {
        /* byte-pos in 4bpp glyph: bit_pos can be 0,4 */
        volatile uint8_t b = 0xABu;
        DIAG_CHECK_U("nib_p0", 0xBu, (unsigned)((b >> 0) & 0xFu));
        DIAG_CHECK_U("nib_p4", 0xAu, (unsigned)((b >> 4) & 0xFu));
    }
    return fail_count;
}

/* probe 24: misaligned uint16 read. TC32 may trap or read wrong bytes. */
int egui_diag_probe_misaligned_u16(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_misa_u16 ==");
    {
        /* Bytes intentionally placed at odd offset within a buffer. */
        static const uint8_t buf[6] = {0x00u, 0x12u, 0x34u, 0x56u, 0x78u, 0x9Au};
        uint16_t v = 0u;
        /* Use memcpy: this is the *safe* path; it must always work. */
        egui_api_memcpy(&v, &buf[1], 2u);
        DIAG_CHECK_U("memcpy_u16_at1", 0x3412u, (unsigned)v);
        egui_api_memcpy(&v, &buf[3], 2u);
        DIAG_CHECK_U("memcpy_u16_at3", 0x7856u, (unsigned)v);
    }
    {
        /* Manual byte assembly (always correct, regardless of alignment). */
        static const uint8_t buf[4] = {0xAAu, 0xBBu, 0xCCu, 0xDDu};
        uint16_t v = (uint16_t)((unsigned)buf[1] | ((unsigned)buf[2] << 8));
        DIAG_CHECK_U("manual_u16_at1", 0xCCBBu, (unsigned)v);
    }
    return fail_count;
}

/* probe 25: misaligned uint32 read. */
int egui_diag_probe_misaligned_u32(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_misa_u32 ==");
    {
        static const uint8_t buf[8] = {0x00u, 0x11u, 0x22u, 0x33u, 0x44u, 0x55u, 0x66u, 0x77u};
        uint32_t v = 0u;
        egui_api_memcpy(&v, &buf[1], 4u);
        DIAG_CHECK_U("memcpy_u32_at1", 0x44332211ul, (unsigned long)v);
        egui_api_memcpy(&v, &buf[3], 4u);
        DIAG_CHECK_U("memcpy_u32_at3", 0x66554433ul, (unsigned long)v);
    }
    {
        /* idx field of egui_font_std_char_descriptor_t is uint32 at offset 0
         * inside a 12-byte packed struct. Read it via memcpy. */
        static const uint8_t buf[12] = {0x78u, 0x56u, 0x34u, 0x12u, 0x80u, 0x00u, 0x09u, 0x0Au, 0x0Bu, 0x01u, 0xFCu, 0x04u};
        uint32_t idx = 0u;
        egui_api_memcpy(&idx, &buf[0], 4u);
        DIAG_CHECK_U("desc_idx", 0x12345678ul, (unsigned long)idx);
    }
    return fail_count;
}

/* probe 26: manual little-endian 4-byte assembly. */
int egui_diag_probe_byte_assemble_le(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_le_asm ==");
    {
        volatile uint8_t b0 = 0x78u, b1 = 0x56u, b2 = 0x34u, b3 = 0x12u;
        uint32_t v = (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
        DIAG_CHECK_U("le4", 0x12345678ul, (unsigned long)v);
    }
    {
        /* big-endian variant for sanity (must NOT be used in production). */
        volatile uint8_t b0 = 0x12u, b1 = 0x34u, b2 = 0x56u, b3 = 0x78u;
        uint32_t v = ((uint32_t)b0 << 24) | ((uint32_t)b1 << 16) | ((uint32_t)b2 << 8) | (uint32_t)b3;
        DIAG_CHECK_U("be4", 0x12345678ul, (unsigned long)v);
    }
    return fail_count;
}

/* probe 27: memcpy(4) must produce identical result to *(uint32_t*). */
int egui_diag_probe_memcpy_4(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_memcpy4 ==");
    {
        uint32_t src = 0xDEADBEEFul;
        uint32_t dst = 0u;
        egui_api_memcpy(&dst, &src, 4u);
        DIAG_CHECK_U("u32_copy", 0xDEADBEEFul, (unsigned long)dst);
    }
    {
        uint8_t buf[4];
        uint32_t src = 0xCAFEBABEul;
        egui_api_memcpy(buf, &src, 4u);
        /* LE check */
        DIAG_CHECK_U("byte0", 0xBEu, (unsigned)buf[0]);
        DIAG_CHECK_U("byte1", 0xBAu, (unsigned)buf[1]);
        DIAG_CHECK_U("byte2", 0xFEu, (unsigned)buf[2]);
        DIAG_CHECK_U("byte3", 0xCAu, (unsigned)buf[3]);
    }
    return fail_count;
}

/* probe 28: hand-rolled memmove forward and backward overlap.
 * Avoids depending on libc memmove on the customer's TC32. */
static void egui_diag_memmove(void *dst, const void *src, unsigned n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    if (d == s || n == 0u)
        return;
    if (d < s)
    {
        for (unsigned i = 0u; i < n; i++)
            d[i] = s[i];
    }
    else
    {
        for (unsigned i = n; i > 0u; i--)
            d[i - 1u] = s[i - 1u];
    }
}

int egui_diag_probe_memmove_overlap(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_memmove ==");
    {
        uint8_t buf[6] = {1u, 2u, 3u, 4u, 5u, 6u};
        /* shift right by 1 within same buffer (overlap) */
        egui_diag_memmove(&buf[1], &buf[0], 5u);
        DIAG_CHECK_U("mm_b0", 1u, buf[0]);
        DIAG_CHECK_U("mm_b1", 1u, buf[1]);
        DIAG_CHECK_U("mm_b5", 5u, buf[5]);
    }
    {
        uint8_t buf[6] = {1u, 2u, 3u, 4u, 5u, 6u};
        /* shift left by 1 */
        egui_diag_memmove(&buf[0], &buf[1], 5u);
        DIAG_CHECK_U("mm_b0_l", 2u, buf[0]);
        DIAG_CHECK_U("mm_b4_l", 6u, buf[4]);
        DIAG_CHECK_U("mm_b5_l", 6u, buf[5]);
    }
    return fail_count;
}

/* probe 29: function-pointer indirection through a struct.
 * Simulates the dispatch pattern egui_view_t.api->on_draw used widely
 * in the rendering pipeline. */
typedef struct egui_diag_dispatch_s
{
    int (*op)(int a, int b);
    int magic;
} egui_diag_dispatch_t;

static int egui_diag_op_add(int a, int b)
{
    return a + b;
}
static int egui_diag_op_sub(int a, int b)
{
    return a - b;
}

int egui_diag_probe_func_ptr_call(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_fn_ptr ==");
    {
        egui_diag_dispatch_t d_add = {egui_diag_op_add, 0x1111};
        egui_diag_dispatch_t d_sub = {egui_diag_op_sub, 0x2222};
        const egui_diag_dispatch_t *p;

        p = &d_add;
        DIAG_CHECK_I("add", 30, p->op(10, 20));
        DIAG_CHECK_U("magic_a", 0x1111u, (unsigned)p->magic);

        p = &d_sub;
        DIAG_CHECK_I("sub", -10, p->op(10, 20));
        DIAG_CHECK_U("magic_s", 0x2222u, (unsigned)p->magic);
    }
    return fail_count;
}

/* probe 30: switch on font_bit_mode (1/2/4/8), verify jump-table semantics. */
static int egui_diag_bit_mode_route(int mode)
{
    switch (mode)
    {
    case 1:
        return 0xA1;
    case 2:
        return 0xA2;
    case 4:
        return 0xA4;
    case 8:
        return 0xA8;
    default:
        return 0xFF;
    }
}

int egui_diag_probe_switch_sparse(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_switch ==");
    DIAG_CHECK_U("m1", 0xA1u, (unsigned)egui_diag_bit_mode_route(1));
    DIAG_CHECK_U("m2", 0xA2u, (unsigned)egui_diag_bit_mode_route(2));
    DIAG_CHECK_U("m4", 0xA4u, (unsigned)egui_diag_bit_mode_route(4));
    DIAG_CHECK_U("m8", 0xA8u, (unsigned)egui_diag_bit_mode_route(8));
    DIAG_CHECK_U("m_def", 0xFFu, (unsigned)egui_diag_bit_mode_route(3));
    return fail_count;
}

/* probe 31: const data pointer identity. Detects linker-script issues
 * where the same const symbol is duplicated across sections. */
static const uint16_t egui_diag_const_blob[4] = {0x1111u, 0x2222u, 0x3333u, 0x4444u};

int egui_diag_probe_const_addr_stable(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_const ==");
    {
        const uint16_t *p1 = egui_diag_const_blob;
        const uint16_t *p2 = egui_diag_const_blob;
        DIAG_CHECK_U("addr_eq", 1u, (p1 == p2) ? 1u : 0u);
        DIAG_CHECK_U("blob0", 0x1111u, (unsigned)p1[0]);
        DIAG_CHECK_U("blob3", 0x4444u, (unsigned)p1[3]);
    }
    return fail_count;
}

/* probe 32: manual byte-wise strcmp on string literals. */
int egui_diag_probe_strcmp_basic(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_strcmp ==");
    {
        const char *a = "Hello";
        const char *b = "Hello";
        const char *c = "Hellp";
        int eq_ab = 1, eq_ac = 1;
        for (unsigned i = 0u; i < 6u; i++)
        {
            if ((unsigned char)a[i] != (unsigned char)b[i])
            {
                eq_ab = 0;
            }
            if ((unsigned char)a[i] != (unsigned char)c[i])
            {
                eq_ac = 0;
            }
        }
        DIAG_CHECK_I("eq_ab", 1, eq_ab);
        DIAG_CHECK_I("eq_ac", 0, eq_ac);
        unsigned n = 0u;
        while (a[n] != '\0')
            n++;
        DIAG_CHECK_U("len_a", 5u, n);
    }
    return fail_count;
}

/* probe 33: alpha_mix exhaustive sweep of representative pairs. */
int egui_diag_probe_alpha_mix_sweep(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_am_sweep ==");
    {
        /* a*b is computed (a*b + 128) >> 8 in egui_color_alpha_mix.
         * Spot-check 8 representative samples across the range. */
        static const struct
        {
            uint8_t a;
            uint8_t b;
        } cases[8] = {
                {0u, 0u}, {255u, 1u}, {1u, 255u}, {64u, 64u}, {127u, 127u}, {200u, 100u}, {15u, 240u}, {17u, 17u},
        };
        for (unsigned i = 0u; i < 8u; i++)
        {
            uint8_t a = cases[i].a;
            uint8_t b = cases[i].b;
            unsigned exp = (unsigned)((a * b + 128u) >> 8);
            unsigned act = (unsigned)egui_color_alpha_mix(a, b);
            char lbl[8];
            lbl[0] = 'a';
            lbl[1] = 'm';
            lbl[2] = '_';
            lbl[3] = (char)('0' + i);
            lbl[4] = '\0';
            DIAG_CHECK_U(lbl, exp, act);
        }
    }
    return fail_count;
}

/* probe 34: rgb_mix monotonicity in alpha (red channel must rise as alpha grows
 * when mixing black-bg with red-fg). */
int egui_diag_probe_rgb_mix_alpha_sweep(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_rm_sweep ==");
#if EGUI_CONFIG_COLOR_DEPTH == 16
    {
        egui_color_t bg, fg;
        bg.full = 0x0000u;
        fg.full = 0xF800u;
        unsigned prev_r = 0u;
        int monotonic = 1;
        for (unsigned a = 0u; a <= 255u; a += 17u)
        {
            egui_color_t r = egui_rgb_mix(bg, fg, (egui_alpha_t)a);
            unsigned cur_r = ((unsigned)r.full >> 11) & 0x1Fu;
            if (cur_r < prev_r)
            {
                monotonic = 0;
            }
            prev_r = cur_r;
        }
        DIAG_CHECK_I("monotonic", 1, monotonic);

        /* Endpoints. */
        {
            egui_color_t r0 = egui_rgb_mix(bg, fg, 0u);
            egui_color_t r255 = egui_rgb_mix(bg, fg, 255u);
            DIAG_CHECK_U("alpha0", 0x0000u, (unsigned)r0.full);
            DIAG_CHECK_U("alpha255", 0xF800u, (unsigned)r255.full);
        }
    }
#else
    DIAG_LOG("skip (CD!=16)");
#endif
    return fail_count;
}

/* probe 35: 16-nibble row covering every alpha 0..15. */
int egui_diag_probe_glyph_4bpp_grid(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_4bpp_grid ==");
#if EGUI_CONFIG_COLOR_DEPTH == 16
    {
        /* Bytes: low nibble (drawn first) = 0,2,4,6,8,10,12,14
         *        high nibble = 1,3,5,7,9,11,13,15 */
        static const uint8_t bytes[8] = {
                (uint8_t)((1u << 4) | 0u), (uint8_t)((3u << 4) | 2u),   (uint8_t)((5u << 4) | 4u),   (uint8_t)((7u << 4) | 6u),
                (uint8_t)((9u << 4) | 8u), (uint8_t)((11u << 4) | 10u), (uint8_t)((13u << 4) | 12u), (uint8_t)((15u << 4) | 14u),
        };
        egui_color_int_t pfb[16];
        for (unsigned i = 0u; i < 16u; i++)
            pfb[i] = 0x0000u;

        egui_color_t bg, fg;
        bg.full = 0x0000u;
        fg.full = 0xF800u;

        unsigned bit_pos = 0u;
        unsigned byte_idx = 0u;
        uint8_t sel_data = bytes[0];
        for (unsigned x = 0u; x < 16u; x++)
        {
            unsigned nib = (sel_data >> bit_pos) & 0x0Fu;
            uint8_t alpha = egui_alpha_change_table_4[nib];
            if (alpha != 0u)
            {
                egui_color_t out;
                if (alpha == 255u)
                {
                    out = fg;
                }
                else
                {
                    out = egui_rgb_mix(bg, fg, alpha);
                }
                pfb[x] = out.full;
            }
            bit_pos += 4u;
            if (bit_pos == 8u)
            {
                bit_pos = 0u;
                byte_idx++;
                sel_data = bytes[byte_idx & 7u];
            }
        }

        /* Verify each cell against egui_rgb_mix called fresh. */
        for (unsigned x = 0u; x < 16u; x++)
        {
            uint8_t a = egui_alpha_change_table_4[x];
            egui_color_t exp;
            if (a == 0u)
            {
                exp.full = 0x0000u;
            }
            else if (a == 255u)
            {
                exp = fg;
            }
            else
            {
                exp = egui_rgb_mix(bg, fg, a);
            }
            char lbl[8];
            lbl[0] = 'g';
            if (x < 10u)
            {
                lbl[1] = (char)('0' + x);
                lbl[2] = '\0';
            }
            else
            {
                lbl[1] = '1';
                lbl[2] = (char)('0' + (x - 10u));
                lbl[3] = '\0';
            }
            DIAG_CHECK_U(lbl, (unsigned)exp.full, (unsigned)pfb[x]);
        }
    }
#else
    DIAG_LOG("skip (CD!=16)");
#endif
    return fail_count;
}

/* probe 36: a row of zero bytes must produce zero writes (alpha=0 skip). */
int egui_diag_probe_glyph_4bpp_zero(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_4bpp_zero ==");
#if EGUI_CONFIG_COLOR_DEPTH == 16
    {
        static const uint8_t bytes[4] = {0u, 0u, 0u, 0u};
        egui_color_int_t pfb[8];
        for (unsigned i = 0u; i < 8u; i++)
            pfb[i] = 0xBEEFu;

        unsigned bit_pos = 0u;
        unsigned byte_idx = 0u;
        uint8_t sel_data = bytes[0];
        for (unsigned x = 0u; x < 8u; x++)
        {
            unsigned nib = (sel_data >> bit_pos) & 0x0Fu;
            uint8_t alpha = egui_alpha_change_table_4[nib];
            if (alpha != 0u)
            {
                pfb[x] = 0xDEADu; /* must NOT be reached */
            }
            bit_pos += 4u;
            if (bit_pos == 8u)
            {
                bit_pos = 0u;
                byte_idx++;
                sel_data = bytes[byte_idx & 3u];
            }
        }
        for (unsigned x = 0u; x < 8u; x++)
        {
            DIAG_CHECK_U("zero", 0xBEEFu, (unsigned)pfb[x]);
        }
    }
#else
    DIAG_LOG("skip (CD!=16)");
#endif
    return fail_count;
}

/* probe 37: same pixel drawn twice (overdraw), second blend operates
 * on the already-blended back color. */
int egui_diag_probe_glyph_overdraw(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_overdraw ==");
#if EGUI_CONFIG_COLOR_DEPTH == 16
    {
        egui_color_t bg, fg, mid, final_color;
        bg.full = 0x0000u;
        fg.full = 0xF800u;
        mid = egui_rgb_mix(bg, fg, 128u);
        final_color = egui_rgb_mix(mid, fg, 128u);

        /* Path under test: rgb_mix_ptr aliased twice. */
        egui_color_t pixel;
        pixel.full = 0x0000u;
        egui_rgb_mix_ptr(&pixel, &fg, &pixel, 128u);
        DIAG_CHECK_U("step1", (unsigned)mid.full, (unsigned)pixel.full);
        egui_rgb_mix_ptr(&pixel, &fg, &pixel, 128u);
        DIAG_CHECK_U("step2", (unsigned)final_color.full, (unsigned)pixel.full);
    }
#else
    DIAG_LOG("skip (CD!=16)");
#endif
    return fail_count;
}

/* probe 38: canvas_alpha * glyph_alpha composition (alpha_mix chain).
 * This is what egui_canvas_draw_point does at the very top before
 * dispatching to rgb_mix. */
int egui_diag_probe_canvas_alpha_compose(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_a_comp ==");
    {
        /* canvas alpha (e.g. dialog overlay) */
        uint8_t canvas_alpha = 200u;
        /* glyph alpha (4-bit nibble mapped) */
        uint8_t glyph_alpha = egui_alpha_change_table_4[8u]; /* 136 */
        uint8_t eff = egui_color_alpha_mix(canvas_alpha, glyph_alpha);
        unsigned exp = (200u * 136u + 128u) >> 8;
        DIAG_CHECK_U("eff", exp, (unsigned)eff);
    }
    {
        uint8_t canvas_alpha = 255u;
        uint8_t glyph_alpha = 100u;
        uint8_t eff = egui_color_alpha_mix(canvas_alpha, glyph_alpha);
        DIAG_CHECK_U("c255", 100u, (unsigned)eff);
    }
    {
        uint8_t canvas_alpha = 100u;
        uint8_t glyph_alpha = 0u;
        uint8_t eff = egui_color_alpha_mix(canvas_alpha, glyph_alpha);
        DIAG_CHECK_U("g0", 0u, (unsigned)eff);
    }
    return fail_count;
}

/* probe 39: int8 off_y (-128..127) sign-extension when added to int dim. */
int egui_diag_probe_negative_off_y(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_offy ==");
    {
        volatile int8_t off_y = (int8_t)-3;
        volatile int16_t base_y = 100;
        int16_t y = (int16_t)(base_y + off_y);
        DIAG_CHECK_I("y_minus3", 97, (int)y);
    }
    {
        volatile int8_t off_y = (int8_t)-128;
        volatile int16_t base_y = 200;
        int16_t y = (int16_t)(base_y + off_y);
        DIAG_CHECK_I("y_min", 72, (int)y);
    }
    {
        /* off_y stored as uint8 in raw bytes -> read as int8 and sign-extend. */
        volatile uint8_t raw = 0xFCu; /* -4 as int8 */
        int8_t signed_val = (int8_t)raw;
        DIAG_CHECK_I("sx", -4, (int)signed_val);
    }
    return fail_count;
}

/* probe 40: sum of advances for "Hello World!" (12 chars).
 * Uses synthetic per-char advance values; the *sum* is what matters as
 * an integer-arithmetic sanity check. */
int egui_diag_probe_advance_sum(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_adv_sum ==");
    {
        /* 'H' 'e' 'l' 'l' 'o' ' ' 'W' 'o' 'r' 'l' 'd' '!' */
        static const uint8_t adv[12] = {11u, 8u, 4u, 4u, 8u, 4u, 14u, 8u, 6u, 4u, 8u, 4u};
        unsigned long sum = 0u;
        for (unsigned i = 0u; i < 12u; i++)
        {
            sum += adv[i];
        }
        DIAG_CHECK_U("sum", 83ul, sum);
    }
    return fail_count;
}

/* probe 41: array-of-struct iteration (19 entries, like the real font). */
int egui_diag_probe_array_iter_19(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_arr19 ==");
    {
        static const egui_font_std_char_descriptor_t arr[19] = {
                {0ul, 4u, 9u, 10u, 11u, 1, 4},  {4ul, 20u, 7u, 7u, 8u, 0, 5},   {24ul, 20u, 2u, 11u, 4u, 1, 3},   {44ul, 40u, 7u, 7u, 8u, 0, 5},
                {84ul, 40u, 8u, 7u, 9u, 0, 5},  {124ul, 50u, 4u, 9u, 4u, 0, 4}, {174ul, 50u, 13u, 7u, 14u, 0, 5}, {224ul, 80u, 9u, 10u, 11u, 1, 4},
                {304ul, 30u, 5u, 7u, 6u, 0, 5}, {334ul, 60u, 7u, 7u, 8u, 0, 5}, {394ul, 30u, 1u, 1u, 4u, 1, 11},  {424ul, 30u, 7u, 7u, 8u, 0, 5},
                {454ul, 10u, 1u, 9u, 4u, 1, 3}, {464ul, 40u, 7u, 7u, 8u, 0, 5}, {504ul, 10u, 7u, 7u, 8u, 0, 5},   {514ul, 50u, 6u, 9u, 8u, 1, 3},
                {564ul, 30u, 6u, 7u, 7u, 0, 5}, {594ul, 40u, 7u, 7u, 8u, 0, 5}, {634ul, 40u, 8u, 7u, 9u, 0, 5},
        };
        unsigned long sum_idx = 0u;
        unsigned long sum_size = 0u;
        unsigned sum_w = 0u;
        for (unsigned i = 0u; i < 19u; i++)
        {
            sum_idx += arr[i].idx;
            sum_size += arr[i].size;
            sum_w += arr[i].box_w;
        }
        DIAG_CHECK_U("ct", 19u, (unsigned)(sizeof(arr) / sizeof(arr[0])));
        DIAG_CHECK_U("sum_idx", 5862ul, sum_idx);
        DIAG_CHECK_U("sum_sz", 674ul, sum_size);
        DIAG_CHECK_U("sum_w", 121u, sum_w);
        /* Spot-check a middle entry that requires correct stride (12B). */
        DIAG_CHECK_U("a7_idx", 224ul, (unsigned long)arr[7].idx);
        DIAG_CHECK_U("a7_w", 9u, (unsigned)arr[7].box_w);
        DIAG_CHECK_I("a7_oy", 4, (int)arr[7].off_y);
    }
    return fail_count;
}

/* probe 42: idx[i+1] - idx[i] must equal size[i] for every entry. */
int egui_diag_probe_pixel_idx_progression(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_idx_prg ==");
    {
        static const uint32_t idx[6] = {0ul, 4ul, 24ul, 44ul, 84ul, 124ul};
        static const uint16_t sz[5] = {4u, 20u, 20u, 40u, 40u};
        int all_ok = 1;
        for (unsigned i = 0u; i < 5u; i++)
        {
            if ((idx[i + 1u] - idx[i]) != (uint32_t)sz[i])
            {
                all_ok = 0;
            }
        }
        DIAG_CHECK_I("prg", 1, all_ok);
    }
    return fail_count;
}

/* probe 43: read a struct field through a raw byte cast (LE). Verifies
 * the compiler's struct member offset matches the byte layout we use
 * for the resource generator. */
int egui_diag_probe_struct_field_via_cast(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_field_cast ==");
    {
        egui_font_std_char_descriptor_t d;
        d.idx = 0xCAFEBABEul;
        d.size = 0x1234u;
        d.box_w = 0x56u;
        d.box_h = 0x78u;
        d.adv = 0x9Au;
        d.off_x = (int8_t)-1;
        d.off_y = (int8_t)-2;

        const uint8_t *raw = (const uint8_t *)&d;
        /* idx LE first 4 bytes */
        uint32_t idx = (uint32_t)raw[0] | ((uint32_t)raw[1] << 8) | ((uint32_t)raw[2] << 16) | ((uint32_t)raw[3] << 24);
        DIAG_CHECK_U("cast_idx", 0xCAFEBABEul, (unsigned long)idx);
        /* size at offset 4 (LE) */
        uint16_t sz = (uint16_t)((unsigned)raw[4] | ((unsigned)raw[5] << 8));
        DIAG_CHECK_U("cast_sz", 0x1234u, (unsigned)sz);
        /* box_w at offset 6 */
        DIAG_CHECK_U("cast_bw", 0x56u, (unsigned)raw[6]);
        /* off_y at offset 10 (signed read). Byte 11 is padding to align
         * the 12-byte struct. */
        DIAG_CHECK_I("cast_oy", -2, (int)(int8_t)raw[10]);
    }
    return fail_count;
}

/* probe 44: static local must default-init to 0. */
int egui_diag_probe_local_static_zero(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_st_zero ==");
    {
        static int s_int;
        static uint8_t s_arr[8];
        DIAG_CHECK_I("s_int", 0, s_int);
        unsigned long sum = 0u;
        for (unsigned i = 0u; i < 8u; i++)
            sum += s_arr[i];
        DIAG_CHECK_U("s_arr", 0u, sum);
    }
    return fail_count;
}

/* probe 45: PFB zero-init via simple loop. */
int egui_diag_probe_zero_pfb_init(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_pfb_init ==");
    {
        egui_color_int_t pfb[32];
        for (unsigned i = 0u; i < 32u; i++)
            pfb[i] = (egui_color_int_t)0xFFFFu;
        for (unsigned i = 0u; i < 32u; i++)
            pfb[i] = 0u;
        unsigned long sum = 0u;
        for (unsigned i = 0u; i < 32u; i++)
            sum += pfb[i];
        DIAG_CHECK_U("pfb_sum", 0u, sum);
    }
    return fail_count;
}

/* probe 46: simulate canvas_draw_point alpha=0 early-out (no PFB write). */
int egui_diag_probe_alpha_zero_skip(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_a0_skip ==");
#if EGUI_CONFIG_COLOR_DEPTH == 16
    {
        egui_color_int_t pfb[4] = {0x1234u, 0x5678u, 0x9ABCu, 0xDEF0u};
        egui_color_t fg;
        fg.full = 0xF800u;
        for (unsigned i = 0u; i < 4u; i++)
        {
            uint8_t a = 0u;
            if (a != 0u)
            {
                /* Would write here, but alpha=0 skipped at draw_point top. */
                pfb[i] = fg.full;
            }
        }
        DIAG_CHECK_U("p0", 0x1234u, (unsigned)pfb[0]);
        DIAG_CHECK_U("p1", 0x5678u, (unsigned)pfb[1]);
        DIAG_CHECK_U("p2", 0x9ABCu, (unsigned)pfb[2]);
        DIAG_CHECK_U("p3", 0xDEF0u, (unsigned)pfb[3]);
    }
#else
    DIAG_LOG("skip (CD!=16)");
#endif
    return fail_count;
}

/* probe 47: sizeof(egui_color_t). Must be 2 bytes for RGB565. */
int egui_diag_probe_color_struct_size(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_color_sz ==");
#if EGUI_CONFIG_COLOR_DEPTH == 16
    DIAG_CHECK_U("sz_color", 2u, (unsigned)sizeof(egui_color_t));
    DIAG_CHECK_U("sz_color_int", 2u, (unsigned)sizeof(egui_color_int_t));
    {
        egui_color_t c;
        c.full = 0xF800u;
        const uint8_t *raw = (const uint8_t *)&c;
        DIAG_CHECK_U("byte0", 0x00u, (unsigned)raw[0]);
        DIAG_CHECK_U("byte1", 0xF8u, (unsigned)raw[1]);
    }
#else
    DIAG_LOG("skip (CD!=16)");
#endif
    return fail_count;
}

/* probe 48: egui_dim_t signedness/size. Many text-layout calculations use
 * negative coordinates (off_x/off_y, scroll offsets). */
int egui_diag_probe_dim_signed(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_dim ==");
    {
        DIAG_CHECK_U("sz_dim", 2u, (unsigned)sizeof(egui_dim_t));
        egui_dim_t d = (egui_dim_t)-1;
        DIAG_CHECK_I("neg", -1, (int)d);
        d = (egui_dim_t)32000;
        DIAG_CHECK_I("max_pos", 32000, (int)d);
    }
    return fail_count;
}

/* probe 49: alpha_change_table_4 inverse: table[i] / 17 == i. */
int egui_diag_probe_alpha_table_inverse(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_a4_inv ==");
    int all_ok = 1;
    for (unsigned i = 0u; i < 16u; i++)
    {
        unsigned v = (unsigned)egui_alpha_change_table_4[i];
        if ((v / 17u) != i || (v % 17u) != 0u)
        {
            all_ok = 0;
        }
    }
    DIAG_CHECK_I("inv", 1, all_ok);
    /* Spot-check first/last to make failures concrete. */
    DIAG_CHECK_U("t0", 0u, (unsigned)egui_alpha_change_table_4[0]);
    DIAG_CHECK_U("t15", 255u, (unsigned)egui_alpha_change_table_4[15]);
    return fail_count;
}

/* probe 50: alpha_change_table_2 full sweep. */
int egui_diag_probe_alpha_table_2_full(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_a2_full ==");
    int all_ok = 1;
    for (unsigned i = 0u; i < 4u; i++)
    {
        unsigned v = (unsigned)egui_alpha_change_table_2[i];
        if (v != i * 85u)
        {
            all_ok = 0;
        }
    }
    DIAG_CHECK_I("a2_full", 1, all_ok);
    DIAG_CHECK_U("a2_0", 0u, (unsigned)egui_alpha_change_table_2[0]);
    DIAG_CHECK_U("a2_3", 255u, (unsigned)egui_alpha_change_table_2[3]);
    return fail_count;
}

/* probe 51 (bonus): dump every EGUI_CONFIG_* macro that influences font
 * rendering. This is the single most useful probe for diagnosing
 * platform-divergent behaviour: if the customer's TC32 build has a
 * different value for any flag listed below, the failure mode is
 * *configuration*, not compiler. */
int egui_diag_probe_compile_config(void)
{
    int fail_count = 0;
    DIAG_LOG("== probe_cfg_dump ==");

#ifdef EGUI_CONFIG_COLOR_DEPTH
    DIAG_LOG("CFG COLOR_DEPTH=%d", (int)EGUI_CONFIG_COLOR_DEPTH);
#else
    DIAG_LOG("CFG COLOR_DEPTH=undef");
#endif
#ifdef EGUI_CONFIG_SCREEN_WIDTH
    DIAG_LOG("CFG SCR_W=%d", (int)EGUI_CONFIG_SCREEN_WIDTH);
#endif
#ifdef EGUI_CONFIG_SCREEN_HEIGHT
    DIAG_LOG("CFG SCR_H=%d", (int)EGUI_CONFIG_SCREEN_HEIGHT);
#endif
#ifdef EGUI_CONFIG_PFB_WIDTH
    DIAG_LOG("CFG PFB_W=%d", (int)EGUI_CONFIG_PFB_WIDTH);
#endif
#ifdef EGUI_CONFIG_PFB_HEIGHT
    DIAG_LOG("CFG PFB_H=%d", (int)EGUI_CONFIG_PFB_HEIGHT);
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    DIAG_LOG("CFG FMT1=%d", (int)EGUI_CONFIG_FUNCTION_FONT_FORMAT_1);
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    DIAG_LOG("CFG FMT2=%d", (int)EGUI_CONFIG_FUNCTION_FONT_FORMAT_2);
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    DIAG_LOG("CFG FMT4=%d", (int)EGUI_CONFIG_FUNCTION_FONT_FORMAT_4);
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    DIAG_LOG("CFG FMT8=%d", (int)EGUI_CONFIG_FUNCTION_FONT_FORMAT_8);
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW
    DIAG_LOG("CFG FAST_DRAW=%d", (int)EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW);
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4
    DIAG_LOG("CFG RLE4=%d", (int)EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4);
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4_XOR
    DIAG_LOG("CFG RLE4X=%d", (int)EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4_XOR);
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_BUILTIN
    DIAG_LOG("CFG BUILTIN=%d", (int)EGUI_CONFIG_FUNCTION_FONT_BUILTIN);
#endif
#ifdef EGUI_CONFIG_FUNCTION_RGB565_BLEND_FAST
    DIAG_LOG("CFG RGB565_FAST=%d", (int)EGUI_CONFIG_FUNCTION_RGB565_BLEND_FAST);
#endif
#ifdef EGUI_CONFIG_FUNCTION_FONT_LINE_CACHE
    DIAG_LOG("CFG LINE_CACHE=%d", (int)EGUI_CONFIG_FUNCTION_FONT_LINE_CACHE);
#endif
#ifdef EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS
    DIAG_LOG("CFG PREFIX_SLOTS=%d", (int)EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS);
#endif
#ifdef EGUI_CONFIG_FUNCTION_RECORDING_TEST
    DIAG_LOG("CFG RECORDING=%d", (int)EGUI_CONFIG_FUNCTION_RECORDING_TEST);
#endif
#ifdef EGUI_CONFIG_DEBUG_LOG_LEVEL
    DIAG_LOG("CFG LOG_LEVEL=%d", (int)EGUI_CONFIG_DEBUG_LOG_LEVEL);
#endif
#ifdef EGUI_CONFIG_FUNCTION_HEAP
    DIAG_LOG("CFG HEAP=%d", (int)EGUI_CONFIG_FUNCTION_HEAP);
#endif
#ifdef EGUI_CONFIG_HEAP_SIZE
    DIAG_LOG("CFG HEAP_SZ=%lu", (unsigned long)EGUI_CONFIG_HEAP_SIZE);
#endif
#ifdef EGUI_CONFIG_FUNCTION_VIEW_TEXTBLOCK
    DIAG_LOG("CFG TEXTBLOCK=%d", (int)EGUI_CONFIG_FUNCTION_VIEW_TEXTBLOCK);
#endif
#ifdef EGUI_CONFIG_FUNCTION_TEXT_INDEX
    DIAG_LOG("CFG TXT_IDX=%d", (int)EGUI_CONFIG_FUNCTION_TEXT_INDEX);
#endif
#ifdef EGUI_CONFIG_FUNCTION_VIEW_LABEL
    DIAG_LOG("CFG LABEL=%d", (int)EGUI_CONFIG_FUNCTION_VIEW_LABEL);
#endif
#ifdef EGUI_CONFIG_FUNCTION_RESOURCE
    DIAG_LOG("CFG RESOURCE=%d", (int)EGUI_CONFIG_FUNCTION_RESOURCE);
#endif

    /* sizeof of every relevant struct (helps catch packing/padding diff). */
    DIAG_LOG("SZ font_info=%u", (unsigned)sizeof(egui_font_std_info_t));
    DIAG_LOG("SZ char_desc=%u", (unsigned)sizeof(egui_font_std_char_descriptor_t));
    DIAG_LOG("SZ code_desc=%u", (unsigned)sizeof(egui_font_std_code_descriptor_t));
    DIAG_LOG("SZ color=%u", (unsigned)sizeof(egui_color_t));
    DIAG_LOG("SZ dim=%u", (unsigned)sizeof(egui_dim_t));
    DIAG_LOG("SZ ptr=%u", (unsigned)sizeof(void *));

    return fail_count;
}

/* ------------------------------------------------------------------ *
 * Aggregate runner
 * ------------------------------------------------------------------ */
int egui_diag_run_all(void)
{
    int fail = 0;

    DIAG_LOG("==== egui_diag begin ====");
#if EGUI_TARGET_TC32
    DIAG_LOG("target=TC32");
#else
    DIAG_LOG("target=other");
#endif
    DIAG_LOG("CD=%d", (int)EGUI_CONFIG_COLOR_DEPTH);
#ifdef EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW
    DIAG_LOG("FAST_DRAW=%d", (int)EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW);
#endif

    fail += egui_diag_probe_platform();
    fail += egui_diag_probe_bitfield();
    fail += egui_diag_probe_struct_layout();
    fail += egui_diag_probe_alpha_tables();
    fail += egui_diag_probe_alpha_mix();
    fail += egui_diag_probe_rgb565_blend();
    fail += egui_diag_probe_glyph_4bpp();
    fail += egui_diag_probe_format_flags();
    fail += egui_diag_probe_descriptor_lookup();
    fail += egui_diag_probe_glyph_1bpp();
    fail += egui_diag_probe_bitmap_ptr();
    fail += egui_diag_probe_alpha_table_full();
    fail += egui_diag_probe_real_glyph_byte();
    fail += egui_diag_probe_real_glyph_row();
    fail += egui_diag_probe_rgb_mix_ptr();
    fail += egui_diag_probe_canvas_pfb_write();

    fail += egui_diag_probe_int_promotion();
    fail += egui_diag_probe_signed_unsigned_cmp();
    fail += egui_diag_probe_int16_widen_mul();
    fail += egui_diag_probe_div_signed();
    fail += egui_diag_probe_uint_wrap();
    fail += egui_diag_probe_shift_unsigned_neg();
    fail += egui_diag_probe_shift_high_count();
    fail += egui_diag_probe_misaligned_u16();
    fail += egui_diag_probe_misaligned_u32();
    fail += egui_diag_probe_byte_assemble_le();
    fail += egui_diag_probe_memcpy_4();
    fail += egui_diag_probe_memmove_overlap();
    fail += egui_diag_probe_func_ptr_call();
    fail += egui_diag_probe_switch_sparse();
    fail += egui_diag_probe_const_addr_stable();
    fail += egui_diag_probe_strcmp_basic();
    fail += egui_diag_probe_alpha_mix_sweep();
    fail += egui_diag_probe_rgb_mix_alpha_sweep();
    fail += egui_diag_probe_glyph_4bpp_grid();
    fail += egui_diag_probe_glyph_4bpp_zero();
    fail += egui_diag_probe_glyph_overdraw();
    fail += egui_diag_probe_canvas_alpha_compose();
    fail += egui_diag_probe_negative_off_y();
    fail += egui_diag_probe_advance_sum();
    fail += egui_diag_probe_array_iter_19();
    fail += egui_diag_probe_pixel_idx_progression();
    fail += egui_diag_probe_struct_field_via_cast();
    fail += egui_diag_probe_local_static_zero();
    fail += egui_diag_probe_zero_pfb_init();
    fail += egui_diag_probe_alpha_zero_skip();
    fail += egui_diag_probe_color_struct_size();
    fail += egui_diag_probe_dim_signed();
    fail += egui_diag_probe_alpha_table_inverse();
    fail += egui_diag_probe_alpha_table_2_full();
    fail += egui_diag_probe_compile_config();

    DIAG_LOG("==== end %s fail=%d ====", fail == 0 ? "PASS" : "FAIL", fail);
    return fail;
}
