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

// HelloPerformance drives one benchmark root view directly and never enters
// the activity/dialog/toast stack, so keeping those core-side state machines
// alive only increases persistent RAM.
#define EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY 0
#define EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG   0
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOAST    0

// HelloPerformance mounts its only benchmark view directly at the root level,
// so the extra user-root wrapper inside egui_core just adds one unused
// root-group object in persistent RAM.
#define EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE 0

// HelloPerformance only replays one pointer event at a time and consumes it
// immediately in the polling loop, so one cached motion slot is enough.
#define EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT 1

// HelloPerformance does not use fling/scroll velocity, so the input module
// can drop its persistent velocity tracker state.
#define EGUI_CONFIG_INPUT_VELOCITY_TRACKER_ENABLE 0

// The app keeps one root-group wrapper plus a single fullscreen test view, so
// a 2-entry touch capture path still preserves follow-up DOWN/MOVE/UP routing.
#define EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX 2

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

// HelloPerformance's generated QOI assets are 40/120/240px tall, so the
// decoder's current-row cursor can stay 8-bit and drop another 4B from qoi_state.
#define EGUI_CONFIG_IMAGE_QOI_ROW_INDEX_8BIT_ENABLE 1

// HelloPerformance's QOI decode stays on one active image stream per scene, so
// the RGBA index table can reconstruct RGB565 on demand without keeping a
// duplicate 64-entry RGB565 index array in static RAM.
#define EGUI_CONFIG_IMAGE_QOI_INDEX_RGB565_CACHE_ENABLE 0

// HelloPerformance's QOI assets are all RGB565 sources. RGB565+alpha images
// can rebuild RGB888 from canonical expansion plus cached alpha, and opaque
// RGB565 images only need one small variant byte for the encoder's speed-
// biased representative choice. This trims another 64B from qoi_state.
#define EGUI_CONFIG_IMAGE_QOI_COMPACT_RGB565_INDEX_ENABLE 1

// HelloPerformance exercises external raw-image draw/resize and external
// raw-image transform in separate benchmark scenes, so they can reuse one
// shared row-cache backing store without changing hot-path behavior.
#define EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS 1

// HelloPerformance never mixes external raw-image row-cache scenes with
// QOI/RLE row-cache scenes in one frame, so the external shared row caches can
// borrow the codec row-cache backing store instead of reserving separate BSS.
#define EGUI_CONFIG_IMAGE_EXTERNAL_SHARED_CACHE_USE_CODEC_ROW_CACHE 1

// HelloPerformance's rotated-text visible alpha8 tile buffer follows the
// actual transformed glyph bounds. Per the RAM rule for size-related buffers,
// keep the QEMU heap hooks enabled so this transient scratch can use frame
// heap instead of a large fixed stack array.
#define EGUI_CONFIG_QEMU_PLATFORM_MALLOC_ENABLE 1

// HelloPerformance's external RLE scenes only stream 120px/240px RGB565 rows,
// and a 64B external read window still keeps the control stream hot while
// literal row copies above the window size fall back to direct loads anyway,
// trimming another 64B of static RAM versus the current 128B window.
#define EGUI_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE 64

// HelloPerformance's RLE benchmarks keep one compressed image workload active
// per scene, and the row-band cache already handles the hot horizontal-tile
// reuse path. The extra decoder checkpoint only keeps a 16B fallback state in
// BSS, so disable it here and let rare backward seeks restart from row 0.
#ifndef EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE
#define EGUI_CONFIG_IMAGE_RLE_CHECKPOINT_ENABLE 0
#endif

// HelloPerformance's external RLE benchmarks only keep one decode stream
// alive per draw, so the window-cache metadata can live on stack instead of
// reserving a persistent global slot in BSS.
#define EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE 0

// HelloPerformance's dominant external image paths use 240px/120px RGB565
// rows (480B/240B) plus matching alpha rows (240B/120B). Keeping the shared
// external caches at 1920B/960B preserves the same 4-row/8-row chunking for
// those hot scenes while trimming the unused tail from the default 2048B/1024B.
#define EGUI_CONFIG_IMAGE_EXTERNAL_DATA_CACHE_MAX_BYTES  1920
#define EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES 960

// HelloPerformance's shadow benchmarks use width=20 with radius 0/30. The
// rounded-shadow path only needs about 51 d_sq buckets at the current shift,
// so a 64-entry cap preserves the same lookup precision while trimming stack.
#define EGUI_CONFIG_SHADOW_DSQ_LUT_MAX 64

// HelloPerformance text transform only uses short benchmark strings and the
// 26pt perf font whose bitmap offsets stay well below 64KB, so the cached
// text-transform layout can use 16-bit offsets/indices to trim heap.
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT 1
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT  1

// HelloPerformance refreshes every benchmark scene manually and never relies
// on the framework's periodic refresh loop, so the core auto-refresh timer can
// be removed entirely.
#define EGUI_CONFIG_CORE_AUTO_REFRESH_TIMER_ENABLE 0

// HelloPerformance only draws a single fullscreen benchmark view and never
// uses scroll/list/viewport style nested clip regions, so the canvas does not
// need to retain an extra clip pointer in persistent RAM.
#define EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE 0

// HelloPerformance's circle/round-rect workloads stay within the built-in
// radius LUT range configured above, so canvas never needs runtime-registered
// out-of-range circle metadata.
#define EGUI_CONFIG_CANVAS_SPEC_CIRCLE_INFO_ENABLE 0

// HelloPerformance keeps a single fullscreen benchmark view and every scene
// switch invalidates that whole surface, so one dirty-region slot is enough.
#define EGUI_CONFIG_DIRTY_AREA_COUNT 1

// HelloPerformance advances scenes through its timer/recording pipeline and
// does not rely on touch interaction, so the touch dispatcher and motion queue
// only consume persistent RAM in this app.
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH 0

// HelloPerformance only reaches one text-rotation prepare call per draw, so
// recomputing the affine bounds is cheaper than keeping a persistent 60B cache.
#define EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE 0

// HelloPerformance's rotated-text scenes stay on the stack layout path and
// only measure one short benchmark string per draw, so the remaining layout
// and dimension metadata caches do not justify permanent static RAM.
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE 0
#define EGUI_CONFIG_TEXT_TRANSFORM_DIM_CACHE_ENABLE    0

// HelloPerformance's rotated-text benchmarks only use a fixed 7-line string
// (70 glyphs), so the stack layout and tile collectors only need a small
// margin above that current content size.
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_GLYPHS 72
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_LINES  8
#define EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS         72
#define EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_LINES          8

// The rotated-text visible alpha8 tile size follows actual transformed glyph
// bounds, so do not keep a fixed large stack buffer here. Let the existing
// per-frame heap cache size itself dynamically and release at frame end.
#define EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_STACK_MAX_BYTES 0

// HelloPerformance only uses the external 26pt perf font whose current glyph
// descriptors peak at 29x25 pixels, so the external rotated-text path only
// needs 15 bytes per glyph row and 270 bytes for one full glyph bitmap.
#define EGUI_CONFIG_TEXT_TRANSFORM_EXTERNAL_GLYPH_ROW_CACHE_BYTES    16
#define EGUI_CONFIG_TEXT_TRANSFORM_EXTERNAL_ALPHA8_GLYPH_STACK_BYTES 288

// HelloPerformance only uses small ASCII subsets (88/93 glyphs), so the
// frame-local ASCII lookup cache can use 8-bit indices and the multi-line
// text cache only needs one small slot for the 7-line benchmark string.
#define EGUI_FONT_STD_CODE_LOOKUP_CACHE_ASCII_COMPACT 1
#define EGUI_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE 0
#define EGUI_FONT_STD_LINE_CACHE_ENABLE         0
#define EGUI_FONT_STD_ASCII_LOOKUP_INDEX_8BIT 1
#define EGUI_FONT_STD_LINE_CACHE_MAX_LINES    8
#define EGUI_FONT_STD_LINE_CACHE_SLOTS        1

// HelloPerformance only draws a few short benchmark strings per scene, so the
// draw-prefix cache's persistent BSS is not worth keeping for this app.
#ifndef EGUI_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS
#define EGUI_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS 0
#endif

// Disable prefix-cache slots together with the glyph budget above.
#ifndef EGUI_FONT_STD_DRAW_PREFIX_CACHE_SLOTS
#define EGUI_FONT_STD_DRAW_PREFIX_CACHE_SLOTS 0
#endif

// HelloPerformance's current image workloads do not justify keeping a
// persistent "source alpha is fully opaque" metadata slot alive in BSS.
#define EGUI_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS 0

// HelloPerformance already shares the external-image row buffers, and each
// draw only needs one row-cache metadata block at a time, so keep that
// metadata on stack instead of reserving a persistent global union in BSS.
#define EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE 0

// HelloPerformance's external image transform benchmarks also reuse the same
// shared row buffers and only keep one transform source alive per draw, so the
// transform-side row-cache metadata can move to stack as well.
#define EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE 0

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
