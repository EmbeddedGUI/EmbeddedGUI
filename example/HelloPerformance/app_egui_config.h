#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_SCEEN_WIDTH  240
#define EGUI_CONFIG_SCEEN_HEIGHT 240

#define EGUI_CONFIG_MAX_FPS 1

// HelloPerformance runs on PC/QEMU with synchronous tile flush, so extra PFB
// buffers do not create useful overlap and only increase static RAM.
#ifndef EGUI_CONFIG_PFB_BUFFER_COUNT
#define EGUI_CONFIG_PFB_BUFFER_COUNT 1
#endif

// HelloPerformance's largest built-in round/circle benchmark radius is 240,
// and the canvas lookup uses a strict `< range` check, so 241 is the minimum
// value that still preserves current coverage while trimming circle LUT/cache RAM.
#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 241

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW 1
#define EGUI_CONFIG_FUNCTION_SUPPORT_MASK   1

#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8  1

// Enable large image tests (480px/240px direct-draw, resize, rotate)
// Set to 0 to save flash on constrained devices; 40x40 tiled tests still run.
#define EGUI_TEST_CONFIG_IMAGE_LARGE 1

// Enable external resource support.
// QEMU uses semihosting file I/O for resource loading.
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 1

// Double-size (480x480) image tests - always enabled (was 960x960 before, now small enough for QEMU too)
#define EGUI_TEST_CONFIG_IMAGE_DOUBLE 1

// Enable image compression codecs for performance testing
#define EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE 1
#define EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE 1

// Enable row-band decode cache: first PFB tile decodes to cache,
// horizontal tile neighbors blend from cache without re-decoding.
// HelloPerformance uses RGB565-only compressed images, so actual RAM
// cost here is about PFB_HEIGHT * SCREEN_WIDTH * (2 + 1) = 21.6KB.
#define EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE 1

// HelloPerformance compressed image resources are RGB565-only,
// so decode scratch/cache buffers only need 2 bytes per pixel.
#ifndef EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE
#define EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE 2
#endif

// HelloPerformance's QOI scenes currently run acceptably from the row-band
// cache alone, so disable decoder checkpoints to remove their persistent heap.
#define EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT 0

// HelloPerformance exercises external raw-image draw/resize and external
// raw-image transform in separate benchmark scenes, so they can reuse one
// shared row-cache backing store without changing hot-path behavior.
#define EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS 1

// HelloPerformance's recorded runtime/perf flow no longer reaches any
// allocator-backed paths after the app-side heap trims above, so keeping the
// QEMU platform malloc hooks disabled avoids linking newlib malloc state into
// the benchmark image.
#define EGUI_CONFIG_QEMU_PLATFORM_MALLOC_ENABLE 0

// HelloPerformance's external RLE scenes only stream 120px/240px RGB565 rows,
// so a 768B external read window still covers the active per-row decode chunks
// while trimming static RAM versus the 1024B default.
#define EGUI_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE 768

// HelloPerformance text transform only uses short benchmark strings and the
// 26pt perf font whose bitmap offsets stay well below 64KB, so the cached
// text-transform layout can use 16-bit offsets/indices to trim heap.
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT 1
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT  1

// HelloPerformance's rotated-text benchmarks only use a fixed 7-line string
// (~70 glyphs), so building the per-draw layout on stack avoids the small
// persistent heap layout cache without affecting current content coverage.
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_GLYPHS 80
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_LINES  8

// The rotated-text visible alpha8 tile peaks a little above 4KB in
// HelloPerformance. Keeping tiles up to 6KB on stack preserves the fast
// alpha8 path while avoiding the transient heap spike in those scenes.
#define EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_STACK_MAX_BYTES 6144

// HelloPerformance only uses small ASCII subsets (88/93 glyphs), so the
// frame-local ASCII lookup cache can use 8-bit indices and the multi-line
// text cache only needs one small slot for the 7-line benchmark string.
#define EGUI_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE 0
#define EGUI_FONT_STD_LINE_CACHE_ENABLE         0
#define EGUI_FONT_STD_ASCII_LOOKUP_INDEX_8BIT 1
#define EGUI_FONT_STD_LINE_CACHE_MAX_LINES    8
#define EGUI_FONT_STD_LINE_CACHE_SLOTS        1

// The long single-line perf string still exits the cached prefix early on
// screen-width clipping, so 32 cached glyphs keeps the hot prefix covered
// while trimming HelloPerformance static RAM.
#ifndef EGUI_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS
#define EGUI_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS 32
#endif

// HelloPerformance only keeps one active perf font/string prefix hot at a
// time, so a single prefix-cache slot preserves reuse while trimming another
// cache entry from static RAM.
#ifndef EGUI_FONT_STD_DRAW_PREFIX_CACHE_SLOTS
#define EGUI_FONT_STD_DRAW_PREFIX_CACHE_SLOTS 1
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
