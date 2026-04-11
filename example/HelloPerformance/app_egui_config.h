#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_SCEEN_HEIGHT 240

#define EGUI_CONFIG_MAX_FPS 1

// HelloPerformance is dominated by full-screen benchmark scenes, and a 48x16
// tile still divides the 240x240 canvas cleanly while cutting the PFB-height-
// proportional row caches much harder than the default 30x30 tile.
#ifndef EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_WIDTH 48
#endif

#ifndef EGUI_CONFIG_PFB_HEIGHT
#define EGUI_CONFIG_PFB_HEIGHT 16
#endif

// HelloPerformance runs on PC/QEMU with synchronous tile flush, so extra PFB
// buffers do not create useful overlap and only increase static RAM.
#ifndef EGUI_CONFIG_PFB_BUFFER_COUNT
#define EGUI_CONFIG_PFB_BUFFER_COUNT 1
#endif

// HelloPerformance's largest built-in round/circle benchmark radius is 240,
// and the canvas lookup uses a strict `< range` check, so 241 is the minimum
// value that still preserves current coverage while trimming circle LUT/cache RAM.
#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 241

// App-local perf override: keep the larger basic-circle fill fast path only in
// HelloPerformance. Other apps stay on the smaller legacy path by default and
// do not carry this switch in the shared fast-path default header anymore.
#define EGUI_CONFIG_CIRCLE_FILL_BASIC 1

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW 1
#define EGUI_CONFIG_FUNCTION_SUPPORT_MASK   1
#define EGUI_CONFIG_FUNCTION_IMAGE_FILE     1

#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8  1

// Enable external resource support.
// QEMU uses semihosting file I/O for resource loading.
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 1

// Enable image compression codecs for performance testing
#define EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE 1
#define EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE 1

// Keep font RLE benchmark coverage app-local. Shared defaults stay off.
#define EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4     1
#define EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4_XOR 1

// Enable row-band decode cache: first PFB tile decodes to cache,
// horizontal tile neighbors blend from cache without re-decoding.
#ifndef EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE 1
#endif

// Small, SRAM-bounded compressed-font cache for RLE4/RLE4_XOR scenes.
// This keeps resident heap capped at 256B plus one metadata slot
// (~32B BSS on 32-bit MCUs), and larger glyphs stay decode-on-use.
#ifndef EGUI_CONFIG_FONT_STD_COMPRESSED_GLYPH_CACHE_MAX_BYTES
#define EGUI_CONFIG_FONT_STD_COMPRESSED_GLYPH_CACHE_MAX_BYTES 256
#endif

#ifndef EGUI_CONFIG_FONT_STD_COMPRESSED_GLYPH_CACHE_SLOTS
#define EGUI_CONFIG_FONT_STD_COMPRESSED_GLYPH_CACHE_SLOTS 1
#endif

/*
 * Optional size-first compatibility overrides.
 *
 * Keep this block disabled in the shipped HelloPerformance profile.
 * As of `2026-04-06`, shared-default fast-path macros have been moved next to
 * the implementation units that consume them, so apps do not carry a large
 * shared public macro surface for duplicate switches.
 *
 * Keep the detailed evidence and historical A/B records in
 * `fast_path_retention.md`. The block below only lists the one size-profile
 * compatibility override that is still used by current in-repo configs.
 *
 * It remains supported for compatibility, but it is no longer part of the
 * shared default fast-path macro surface.
 */
#if 0
#define EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE 0
#endif

/*
 * Optional perf-first external image cache experiment.
 *
 * Keep this block disabled by default so HelloPerformance keeps the smaller
 * no-persistent-cache baseline.
 *
 * Re-enabling a `5000B` external whole-image persistent cache budget adds
 * about `948B` text and `40B` BSS on the current HelloPerformance build,
 * keeps the full `239`-scene perf run free of any `>=10%` regressions, and
 * mainly speeds up small external tiled scenes whose total raw data fits in
 * that budget, such as `EXTERN_IMAGE_TILED_565_0` and
 * `EXTERN_IMAGE_RESIZE_TILED_565_1/2/4`.
 */
#if 0
#define EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES 5000
#endif

// HelloPerformance advances scenes through its timer/recording pipeline and
// does not rely on touch interaction, so the touch dispatcher and motion queue
// only consume persistent RAM in this app by default. Keep this overridable so
// perf/runtime rechecks can measure the true 1 vs 0 delta via USER_CFLAGS.
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH 0
#endif

/*
 * Optional low-RAM image/codec policy overrides.
 *
 * Keep this block disabled by default so HelloPerformance uses the framework
 * defaults, which favor maximum rendering throughput over RAM savings.
 *
 * Turn this block on only when you want to measure the old low-RAM
 * HelloPerformance profile for SRAM-sensitive benchmarks.
 */
#if 0
// Compressed image resources are RGB565-only, so decode scratch/cache buffers
// only need 2 bytes per pixel instead of the default 4.
#define EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE             2

// Low-RAM QOI row-cache mode: only cache the horizontal tail instead of the
// full row-band. Saves about 1.5KB peak heap on 240px alpha-QOI scenes.
#define EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE       1

// Historical low-RAM experiment: shrink the external RLE read window from the
// default 1024B to 64B. Current recheck saves about 1024B BSS for 96B extra
// text, but external RLE hotspots regress by about +17.5%~+21.8%.
#define EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE    64

// Text-transform layout: 16-bit indices shrink transient heap for the current
// benchmark strings and font offsets.
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT 1
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT  1

// Rotated-text scratch: keep layout/tile scratch on transient heap and cap the
// visible alpha8 fast-path cache at the smaller low-RAM ceiling.
#define EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE      1
#define EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES 2560
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
