#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_SCEEN_WIDTH  240
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

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW 1
#define EGUI_CONFIG_FUNCTION_SUPPORT_MASK   1

// HelloPerformance only drives single-pointer recorded interactions and its
// widget tree is shallow, so the default motion queue / capture-path budgets
// are larger than needed for this app.
#define EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT 3
#define EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX   12

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
// cost here is about PFB_HEIGHT * SCREEN_WIDTH * (2 + 1) = 11.5KB.
#define EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE 1

// HelloPerformance compressed image resources are RGB565-only,
// so decode scratch/cache buffers only need 2 bytes per pixel.
#ifndef EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE
#define EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE 2
#endif

// HelloPerformance draws one compressed-image workload at a time, so masked
// opaque fallback rows can borrow the codec row-cache alpha backing store
// instead of reserving a dedicated 240B decode-row alpha buffer in BSS.
#define EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE 1

// HelloPerformance's QOI scenes currently run acceptably from the row-band
// cache alone, so disable decoder checkpoints to remove their persistent heap.
#define EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT 0

// HelloPerformance's QOI decode stays on one active image stream per scene, so
// the RGBA index table can reconstruct RGB565 on demand without keeping a
// duplicate 64-entry RGB565 index array in static RAM.
#define EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE 0

// HelloPerformance exercises external raw-image draw/resize and external
// raw-image transform in separate benchmark scenes, so they can reuse one
// shared row-cache backing store without changing hot-path behavior.
#define EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS 1

// HelloPerformance never mixes external raw-image row-cache scenes with
// QOI/RLE row-cache scenes in one frame, so the external shared row caches can
// borrow the codec row-cache backing store instead of reserving separate BSS.
#define EGUI_CONFIG_IMAGE_EXTERNAL_SHARED_CACHE_USE_CODEC_ROW_CACHE 1

// HelloPerformance's recorded runtime/perf flow no longer reaches any
// allocator-backed paths after the app-side heap trims above, so keeping the
// QEMU platform malloc hooks disabled avoids linking newlib malloc state into
// the benchmark image.
#define EGUI_CONFIG_QEMU_PLATFORM_MALLOC_ENABLE 0

// HelloPerformance's external RLE scenes only stream 120px/240px RGB565 rows,
// and a 128B external read window still keeps the control stream hot while
// literal row copies above the window size fall back to direct loads anyway,
// trimming another 128B of static RAM versus the current 256B window.
#define EGUI_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE 128

// HelloPerformance's dominant external image paths use 240px/120px RGB565
// rows (480B/240B) plus matching alpha rows (240B/120B). Keeping the shared
// external caches at 1920B/960B preserves the same 4-row/8-row chunking for
// those hot scenes while trimming the unused tail from the default 2048B/1024B.
#define EGUI_CONFIG_IMAGE_EXTERNAL_DATA_CACHE_MAX_BYTES  1920
#define EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES 960

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
// screen-width clipping, and 16 cached glyphs still cover the visible hot
// prefix on HelloPerformance while trimming static RAM a bit further.
#ifndef EGUI_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS
#define EGUI_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS 16
#endif

// HelloPerformance only keeps one active perf font/string prefix hot at a
// time, so a single prefix-cache slot preserves reuse while trimming another
// cache entry from static RAM.
#ifndef EGUI_FONT_STD_DRAW_PREFIX_CACHE_SLOTS
#define EGUI_FONT_STD_DRAW_PREFIX_CACHE_SLOTS 1
#endif

// HelloPerformance image benchmarks keep one source image hot per scene, so a
// single "alpha row is fully opaque" cache slot preserves the fast-path hit
// without keeping extra global entries alive.
#define EGUI_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS 1

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
