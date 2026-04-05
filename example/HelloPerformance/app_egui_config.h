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

// Keep the larger basic-circle fill fast path only in HelloPerformance.
// Other apps keep the smaller legacy path by default to reduce code size.
#define EGUI_CONFIG_CIRCLE_FILL_BASIC_PERF_OPT_ENABLE 1

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW 1
#define EGUI_CONFIG_FUNCTION_SUPPORT_MASK   1

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

// Enable row-band decode cache: first PFB tile decodes to cache,
// horizontal tile neighbors blend from cache without re-decoding.
#ifndef EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE 1
#endif

/*
 * Optional size-first experiments.
 *
 * Keep this block disabled in the shipped HelloPerformance profile.
 * Disabling masked std-font fast draw saves about 2.6KB text in this app and
 * leaves the current basic TEXT scenes unchanged, but masked text falls back
 * to the generic glyph path. Gradient masked-text hotspots also regress
 * sharply once the dedicated row-blend glyph path is gone.
 *
 * Disabling only the masked std-font row-blend glyph fast path saves about
 * 1.6KB text in this app, keeps the current basic TEXT / EXTERN_TEXT scenes
 * unchanged, and only trades gradient masked-text row blending back to the
 * generic glyph path. Current QEMU samples show about 3.3x slower
 * `TEXT_GRADIENT` and about 5.7x slower `TEXT_RECT_GRADIENT`, while
 * `TEXT_ROTATE_GRADIENT` stays unchanged.
 *
 * Disabling std-image fast draw saves about 38KB text in this app, but the
 * internal RGB565 direct-draw path regresses by roughly 3x~11x in QEMU.
 *
 * Disabling std-image alpha direct draw saves about 8KB text in this app and
 * keeps raw RGB565 direct-draw scenes unchanged, but alpha direct draw falls
 * back to the generic path.
 *
 * Disabling std-image alpha8 direct draw saves about 1.7KB text in this app,
 * removes the specialized RGB565_8 direct-draw path, and keeps raw RGB565 /
 * RGB565_1/2/4 direct-draw fast paths unchanged. RGB565_8 scenes fall back to
 * the generic get_point loop.
 *
 * Disabling std-image packed-alpha direct draw saves about 5.5KB text in this
 * app, removes the specialized RGB565_1/2/4 direct-draw path, and keeps raw
 * RGB565 / RGB565_8 direct-draw fast paths unchanged. RGB565_1/2/4 scenes
 * fall back to the generic get_point loop.
 *
 * Disabling std-image alpha-color fast paths saves about 1.2KB text in this
 * app, removes the specialized alpha-only tint draw/resize path, and keeps
 * raw IMAGE_565 / EXTERN_IMAGE_565 fast paths unchanged. IMAGE_COLOR /
 * IMAGE_RESIZE_COLOR fall back to generic get_point loops. A later draw/resize
 * child split only recovered about 52B / 184B here, so the finer-grained
 * child macros were rejected and only the umbrella experiment remains.
 *
 * Disabling std-image fast resize saves about 19KB text in this app and keeps
 * the current basic draw scenes unchanged, but raw RGB565 resize regresses by
 * roughly 3x~6x in QEMU.
 *
 * Disabling std-image alpha resize fast draw saves about 11KB text in this
 * app and keeps raw RGB565 resize scenes unchanged, but alpha resize falls
 * back to the generic path.
 *
 * Disabling std-image alpha8 resize saves about 3.8KB text in this app,
 * removes the specialized RGB565_8 resize path, and keeps raw RGB565 /
 * RGB565_1/2/4 resize fast paths unchanged. RGB565_8 resize scenes fall back
 * to the generic get_point loop.
 *
 * Disabling std-image packed-alpha resize saves about 7.5KB text in this app,
 * removes the specialized RGB565_1/2/4 resize path, and keeps raw RGB565 /
 * RGB565_8 resize fast paths unchanged. RGB565_1/2/4 resize scenes fall back
 * to the generic get_point loop.
 *
 * Disabling std-image external alpha fast paths saves about 4.1KB text in
 * this app and keeps the current raw EXTERN_IMAGE_565 / EXTERN_IMAGE_RESIZE_565
 * scenes unchanged, but external RGB565_1/2/4/8 draw/resize falls back to the
 * generic path.
 *
 * Disabling only std-image external packed-alpha fast paths saves about 2.8KB
 * text in this app, keeps the current raw EXTERN_IMAGE_565 /
 * EXTERN_IMAGE_RESIZE_565 and external RGB565_8 draw/resize scenes unchanged,
 * and only trades away the specialized external RGB565_1/2/4 draw/resize
 * helpers.
 *
 * Disabling only std-image external packed-alpha resize fast paths also saves
 * about 2.8KB text in this app, keeps the current raw EXTERN_IMAGE_565 /
 * EXTERN_IMAGE_RESIZE_565 and external RGB565_1/2/4 direct-draw scenes
 * unchanged, and only trades away the specialized external RGB565_1/2/4
 * resize helpers. The complementary draw-only split is only about 180B, so
 * this resize child macro is the worthwhile finer-grained experiment.
 *
 * Disabling std-image mask-shape fast paths saves about 13.0KB text in this
 * app and keeps the current raw IMAGE_565 / EXTERN_IMAGE_565 /
 * IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565 scenes unchanged, but
 * circle/round-rectangle masked image scenes fall back to the generic masked
 * path.
 *
 * Disabling only the circle masked-image fast path saves about 7.2KB text,
 * while disabling only the round-rectangle masked-image fast path saves about
 * 9.3KB text in this app. Both switches keep the current basic draw/resize
 * scenes unchanged and only trade away the corresponding masked-image hotspot.
 *
 * Disabling only the round-rectangle masked-image resize fast path saves about
 * 3.8KB text in this app, keeps the current basic draw/resize scenes and
 * current round-rect QOI / RLE / external masked-image samples unchanged, and
 * only trades away the internal round-rect masked resize helpers used by
 * MASK_IMAGE_ROUND_RECT / MASK_IMAGE_ROUND_RECT_QUARTER /
 * MASK_IMAGE_ROUND_RECT_DOUBLE. Current samples are
 * MASK_IMAGE_ROUND_RECT +10.8%, MASK_IMAGE_ROUND_RECT_QUARTER +7.7%,
 * MASK_IMAGE_ROUND_RECT_DOUBLE +10.4%, and
 * MASK_IMAGE_TEST_PERF_ROUND_RECT +9.7%.
 *
 * Disabling only the round-rectangle masked-image direct-draw fast path saves
 * about 6.1KB text in this app, keeps the current basic draw/resize scenes and
 * current internal round-rect masked-image resize samples unchanged, and only
 * trades away the round-rect masked-image draw helpers used by QOI / RLE /
 * external resource paths. Current samples are
 * MASK_IMAGE_QOI_ROUND_RECT +10.4%, MASK_IMAGE_RLE_ROUND_RECT +24.7%,
 * EXTERN_MASK_IMAGE_QOI_ROUND_RECT +4.0%, and
 * EXTERN_MASK_IMAGE_RLE_ROUND_RECT +12.8%.
 *
 * Disabling only the circle masked-image resize fast path saves about 2.0KB
 * text in this app, keeps the current basic draw/resize scenes and current
 * circle QOI / RLE / external masked-image samples unchanged, and only trades
 * away the internal circle masked resize helpers used by
 * MASK_IMAGE_CIRCLE / MASK_IMAGE_CIRCLE_QUARTER /
 * MASK_IMAGE_CIRCLE_DOUBLE. Current samples are
 * MASK_IMAGE_CIRCLE +12.4%, MASK_IMAGE_CIRCLE_QUARTER +9.9%,
 * MASK_IMAGE_CIRCLE_DOUBLE +9.9%, and
 * MASK_IMAGE_TEST_PERF_CIRCLE +36.2%.
 *
 * Disabling only the circle masked-image direct-draw fast path saves about
 * 3.0KB text in this app, keeps the current basic draw/resize scenes and
 * current internal circle masked-image resize samples unchanged, and only
 * trades away the circle masked-image draw helpers used by QOI / RLE /
 * external resource paths. Current samples are
 * MASK_IMAGE_QOI_CIRCLE +90.3%, MASK_IMAGE_RLE_CIRCLE +208.7%,
 * EXTERN_MASK_IMAGE_QOI_CIRCLE +35.5%, and
 * EXTERN_MASK_IMAGE_RLE_CIRCLE +111.2%.
 *
 * Disabling only the circle masked-image alpha8 direct-draw fast path saves
 * about 1.9KB text in this app, keeps the current basic draw/resize scenes,
 * raw circle masked-image draw/resize samples, and non-alpha8 circle
 * QOI / RLE / external samples unchanged, and only trades away the circle
 * alpha8 masked-image draw helpers used by 8-bit codec / external resource
 * paths. Current samples are MASK_IMAGE_QOI_8_CIRCLE +232.5%,
 * MASK_IMAGE_RLE_8_CIRCLE +331.1%,
 * EXTERN_MASK_IMAGE_QOI_8_CIRCLE +189.5%, and
 * EXTERN_MASK_IMAGE_RLE_8_CIRCLE +226.6%.
 *
 * Disabling only the circle masked-image rgb565 direct-draw fast path saves
 * about 1.2KB text in this app, keeps the current basic draw/resize scenes,
 * raw circle masked-image draw/resize samples, and alpha8 circle draw samples
 * unchanged, and only trades away the non-alpha8 circle masked-image draw
 * helpers used by QOI / RLE / external resource paths. Current full `239`
 * scene samples show no regression above `10%`; representative non-alpha8
 * circle draw samples are MASK_IMAGE_QOI_CIRCLE +2.8%,
 * MASK_IMAGE_RLE_CIRCLE +6.5%, EXTERN_MASK_IMAGE_QOI_CIRCLE +1.1%, and
 * EXTERN_MASK_IMAGE_RLE_CIRCLE +3.5%.
 *
 * Disabling only the circle masked-image alpha8 resize fast path saves about
 * 0.5KB text in this app, keeps the current basic draw/resize scenes, raw
 * RGB565 circle resize samples, and circle draw / codec samples unchanged,
 * and only trades away the circle RGB565_8 masked-image resize helpers used
 * by MASK_IMAGE_CIRCLE / EXTERN_MASK_IMAGE_CIRCLE /
 * MASK_IMAGE_CIRCLE_QUARTER / MASK_IMAGE_CIRCLE_DOUBLE. Current samples are
 * MASK_IMAGE_CIRCLE +11.4%, EXTERN_MASK_IMAGE_CIRCLE +11.0%,
 * MASK_IMAGE_CIRCLE_QUARTER +7.1%, and MASK_IMAGE_CIRCLE_DOUBLE +9.3%.
 *
 * Disabling only the circle masked-image rgb565 resize fast path saves about
 * 1.5KB text in this app, keeps the current basic draw/resize scenes,
 * alpha8 circle resize samples, and current circle draw / codec / external
 * samples unchanged, but raw sampled circle resize hotspots still regress.
 * Current full `239`-scene perf has `MASK_IMAGE_TEST_PERF_CIRCLE +36.2%`,
 * and runtime on/off also shows a real pixel mismatch on `frame_0190.png`, so
 * this child is only suitable as an experiment record rather than a current
 * recommended split.
 *
 * Disabling only the round-rectangle masked-image rgb565 resize fast path
 * saves about 2.5KB text in this app, keeps the current basic draw/resize
 * scenes, alpha8 round-rect resize samples, and round-rect draw / codec /
 * external samples unchanged, and the current full `239`-scene perf run still
 * shows no regression above `10%`. Representative raw round-rect resize
 * samples are MASK_IMAGE_ROUND_RECT +0.0%,
 * MASK_IMAGE_ROUND_RECT_QUARTER +0.3%,
 * MASK_IMAGE_ROUND_RECT_DOUBLE +0.0%, and
 * MASK_IMAGE_TEST_PERF_ROUND_RECT +9.7%.
 *
 * Disabling std-image opaque-source promotion saves about 1.3KB text and 40B
 * BSS in this app, and leaves the current basic draw/resize scenes unchanged,
 * but RGB565 images with a fully-opaque alpha source stop promoting back to
 * the raw RGB565 fast path under masked draw/resize branches.
 *
 * Disabling std-image row-overlay fast paths saves about 2.7KB text in this
 * app and leaves the current basic draw/resize scenes unchanged, but image
 * gradient-overlay scenes fall back to the generic per-point mask path.
 *
 * Disabling only the non-raw std-image row-overlay fast paths saves about
 * 1.5KB text in this app, and current-mainline full perf still keeps the
 * current basic draw/resize scenes plus raw IMAGE_GRADIENT_OVERLAY within
 * noise (`IMAGE_GRADIENT_OVERLAY +0.1%`). It only trades away the generic
 * get_pixel / RGB565_8 row-overlay branches that run when a mask exposes
 * `mask_get_row_overlay()`.
 *
 * Disabling only the raw RGB565 std-image row-overlay fast paths saves about
 * 1.3KB text in this app, keeps the current basic draw/resize scenes and
 * current non-raw overlay samples unchanged, but IMAGE_GRADIENT_OVERLAY
 * regresses by about +165.6% when it falls back to the generic per-point mask
 * path.
 *
 * Disabling mask-image identity-scale fast paths saves about 3.3KB text in
 * this app and keeps the current basic RECTANGLE / CIRCLE / ROUND_RECTANGLE /
 * TEXT / IMAGE_565 / EXTERN_IMAGE_565 scenes unchanged, but identity-scale
 * image-mask solid-fill / masked-image hotspots fall back to the generic
 * scaled or mapped path. A later child split, rechecked on current mainline,
 * showed the solid-fill half is only about 660B here, while the masked-image
 * blend half still saves about 2.6KB text and leaves the current basic
 * RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 /
 * EXTERN_IMAGE_565 scenes unchanged. The remaining regressions stay on
 * identity-scale masked-image blend hotspots such as `MASK_IMAGE_IMAGE`
 * (+53.4%), `EXTERN_MASK_IMAGE_IMAGE` (+47.4%), and `MASK_IMAGE_RLE_IMAGE`
 * (+23.2%), so only the blend child remains worth keeping as a finer-grained
 * experiment.
 *
 * Disabling std-image mask visible-range fast paths saves about 1.4KB text in
 * this app and keeps the current basic RECTANGLE / CIRCLE / ROUND_RECTANGLE /
 * TEXT / IMAGE_565 / EXTERN_IMAGE_565 scenes unchanged, but partial-row
 * masked std-image draw/resize paths stop trimming fully invisible edge spans.
 * Current sampled masked-image hotspots stay in noise except
 * `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE`, which regresses by about 84.5%.
 *
 * Disabling std-image masked row-partial fast paths saves about 7.6KB text in
 * this app and keeps the current basic RECTANGLE / CIRCLE / ROUND_RECTANGLE /
 * TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 /
 * EXTERN_IMAGE_RESIZE_565 scenes unchanged, but masked-image hotspots that
 * rely on `EGUI_MASK_ROW_PARTIAL` regress sharply. Current samples include
 * `MASK_IMAGE_TEST_PERF_CIRCLE +331.9%`,
 * `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +206.2%`, and
 * `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +118.6%`.
 *
 * Disabling only the std-image masked row-partial draw fast paths saves about
 * 2.4KB text in this app, keeps the current basic RECTANGLE / CIRCLE /
 * ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 /
 * EXTERN_IMAGE_RESIZE_565 scenes unchanged, and the current full 239-scene
 * perf run shows no regression above 10%. The current suite still does not
 * isolate raw alpha masked direct-draw partial-row hotspots, so this child is
 * suitable as a conditional experiment macro rather than a default-off option.
 *
 * Disabling only the std-image masked row-partial alpha8 draw fast paths saves
 * about 1.1KB text in this app, keeps the current basic RECTANGLE / CIRCLE /
 * ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 /
 * EXTERN_IMAGE_RESIZE_565 scenes unchanged, and the current full 239-scene
 * perf run still shows no regression above 10%. Current `IMAGE_565_8` /
 * `EXTERN_IMAGE_565_8` and sampled alpha8 masked-image draw samples also stay
 * in noise, but the suite still does not isolate raw alpha8 masked direct-draw
 * partial-row hotspots, so this child is also only suitable as a conditional
 * experiment macro.
 *
 * Disabling only the std-image masked row-partial packed-alpha draw fast paths
 * saves about 1.4KB text in this app, keeps the current basic RECTANGLE /
 * CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 /
 * IMAGE_RESIZE_565 / EXTERN_IMAGE_RESIZE_565 scenes unchanged, and the
 * current full 239-scene perf run still shows no regression above 10%.
 * Current `IMAGE_565_1/2/4` / `EXTERN_IMAGE_565_1/2/4` and sampled packed
 * masked-image draw samples also stay in noise, but the suite still does not
 * isolate raw packed-alpha masked direct-draw partial-row hotspots, so this
 * child is likewise only suitable as a conditional experiment macro.
 *
 * Disabling only the std-image masked row-partial resize fast paths saves
 * about 6.3KB text in this app, keeps the current basic RECTANGLE / CIRCLE /
 * ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 /
 * EXTERN_IMAGE_RESIZE_565 scenes unchanged, and keeps masked-image draw plus
 * most no-mask / round-rect / image resize samples unchanged. The remaining
 * regressions concentrate in circle and external partial-row masked resize
 * hotspots, including `MASK_IMAGE_CIRCLE +148.0%`,
 * `EXTERN_MASK_IMAGE_CIRCLE +141.6%`,
 * `MASK_IMAGE_TEST_PERF_CIRCLE +331.9%`,
 * `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +206.2%`,
 * `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +118.6%`, and
 * `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE +89.7%`.
 *
 * Disabling std-image masked row-inside fast paths saves about 1.3KB text in
 * this app and keeps the current basic RECTANGLE / CIRCLE / ROUND_RECTANGLE /
 * TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 /
 * EXTERN_IMAGE_RESIZE_565 scenes unchanged, but masked-image hotspots that
 * rely on `EGUI_MASK_ROW_INSIDE` regress sharply. Current samples include
 * `MASK_IMAGE_TEST_PERF_CIRCLE +399.1%`,
 * `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +255.1%`, and
 * `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +813.6%`.
 *
 * Disabling only the std-image masked row-inside resize fast paths saves
 * about 0.9KB text in this app, keeps the current basic RECTANGLE / CIRCLE /
 * ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 / IMAGE_RESIZE_565 /
 * EXTERN_IMAGE_RESIZE_565 scenes unchanged, and keeps sampled no-mask /
 * round-rect / circle / image masked-image resize scenes unchanged. The
 * remaining regressions stay concentrated in inside-row masked-image resize
 * hotspots, including `MASK_IMAGE_TEST_PERF_CIRCLE +399.1%`,
 * `EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE +254.7%`,
 * `EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT +813.6%`, and
 * `EXTERN_MASK_IMAGE_TEST_PERF_IMAGE +14.5%`.
 *
 * Disabling canvas circle masked-fill fast paths saves about 3.8KB text in
 * this app and keeps the current basic RECTANGLE / CIRCLE / ROUND_RECTANGLE /
 * TEXT / IMAGE_565 / EXTERN_IMAGE_565 scenes unchanged, but circle masked
 * solid-fill hotspots fall back to the generic masked-fill path.
 *
 * Disabling only the canvas circle masked-fill segment fast path saves about
 * 3.3KB text in this app, keeps the current basic RECTANGLE / CIRCLE /
 * ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 scenes unchanged, and
 * the current full `239`-scene perf run still shows no regression above
 * `10%`. It only trades away the circle partial-row segment helper inside
 * `egui_canvas_fill_masked_row_segment()`, while the whole-row
 * `egui_mask_circle_fill_row_segment()` path stays enabled.
 *
 * Disabling canvas round-rectangle masked-fill fast paths saves about 1.0KB
 * text in this app and keeps the current basic RECTANGLE / CIRCLE /
 * ROUND_RECTANGLE / TEXT / IMAGE_565 / EXTERN_IMAGE_565 scenes unchanged, but
 * round-rectangle masked solid-fill hotspots fall back to the generic
 * masked-fill path.
 *
 * Disabling canvas row-blend masked-fill fast paths saves about 2.9KB text in
 * this app and keeps the current basic RECTANGLE / CIRCLE / ROUND_RECTANGLE /
 * TEXT / IMAGE_565 / EXTERN_IMAGE_565 scenes unchanged, but gradient-masked
 * solid fills fall back from row-wise color blend to per-pixel masked fill.
 *
 * Disabling canvas image masked-fill fast paths saves about 1.8KB text in this
 * app and keeps the current basic RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT /
 * IMAGE_565 / EXTERN_IMAGE_565 scenes unchanged, but image-masked solid fills
 * fall back to the generic masked-fill path.
 *
 * Disabling canvas row-range masked-fill fast paths saves about 8.0KB text in
 * this app, and current-mainline full perf still keeps the current basic
 * RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 /
 * EXTERN_IMAGE_565 scenes plus sampled canvas masked-fill hotspots within
 * noise (`MASK_GRADIENT_RECT_FILL +0.0%`, `MASK_RECT_FILL_IMAGE +0.0%`,
 * `MASK_RECT_FILL_ROUND_RECT +0.0%`, `MASK_RECT_FILL_CIRCLE +0.1%`,
 * `MASK_ROUND_RECT_FILL_WITH_MASK +0.3%`). Only custom solid-fill masks that
 * rely on `mask_get_row_range()` fall back to the generic per-pixel masked-fill
 * path.
 *
 * Disabling canvas row-partial masked-fill fast paths saves about 2.9KB text
 * in this app, and current-mainline full perf still keeps the current basic
 * RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 /
 * EXTERN_IMAGE_565 scenes plus sampled canvas masked-fill hotspots within
 * noise (`MASK_GRADIENT_RECT_FILL +0.0%`, `MASK_RECT_FILL_IMAGE +0.4%`,
 * `MASK_RECT_FILL_ROUND_RECT +0.3%`, `MASK_RECT_FILL_CIRCLE +0.1%`). It only
 * trades the `EGUI_MASK_ROW_PARTIAL` branch back to generic per-pixel masked
 * fill while keeping row-range skip/full-row acceleration for `OUTSIDE` /
 * `INSIDE`.
 *
 * Disabling canvas row-inside masked-fill fast paths saves about 1.7KB text in
 * this app, and current-mainline full perf still keeps the current basic
 * RECTANGLE / CIRCLE / ROUND_RECTANGLE / TEXT / IMAGE_565 /
 * EXTERN_IMAGE_565 scenes plus sampled canvas masked-fill hotspots within
 * noise (`MASK_GRADIENT_RECT_FILL +0.0%`, `MASK_RECT_FILL_IMAGE +0.0%`,
 * `MASK_RECT_FILL_ROUND_RECT -0.3%`, `MASK_RECT_FILL_CIRCLE +0.0%`). It only
 * trades the `EGUI_MASK_ROW_INSIDE` direct-fill branch back to generic masked
 * fill while keeping `OUTSIDE` skip and optional `PARTIAL` acceleration.
 */
#if 0
#define EGUI_CONFIG_FONT_STD_FAST_MASK_DRAW_ENABLE                                  0
#define EGUI_CONFIG_FONT_STD_MASK_ROW_BLEND_FAST_PATH_ENABLE                        0
#define EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ENABLE                                      0
#define EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA_ENABLE                                0
#define EGUI_CONFIG_IMAGE_STD_FAST_DRAW_ALPHA8_ENABLE                               0
#define EGUI_CONFIG_IMAGE_STD_FAST_DRAW_PACKED_ALPHA_ENABLE                         0
#define EGUI_CONFIG_IMAGE_STD_ALPHA_COLOR_FAST_PATH_ENABLE                          0
#define EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ENABLE                                    0
#define EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA_ENABLE                              0
#define EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_ALPHA8_ENABLE                             0
#define EGUI_CONFIG_IMAGE_STD_FAST_RESIZE_PACKED_ALPHA_ENABLE                       0
#define EGUI_CONFIG_IMAGE_CODEC_MASK_IMAGE_ROW_BLOCK_FAST_PATH_ENABLE               0
#define EGUI_CONFIG_IMAGE_STD_EXTERNAL_ALPHA_FAST_PATH_ENABLE                       0
#define EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_FAST_PATH_ENABLE                0
#define EGUI_CONFIG_IMAGE_STD_EXTERNAL_PACKED_ALPHA_RESIZE_FAST_PATH_ENABLE         0
#define EGUI_CONFIG_IMAGE_STD_MASK_SHAPE_FAST_PATH_ENABLE                           0
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_FAST_PATH_ENABLE                          0
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_FAST_PATH_ENABLE                   0
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_ALPHA8_FAST_PATH_ENABLE            0
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_RESIZE_RGB565_FAST_PATH_ENABLE            0
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_FAST_PATH_ENABLE                     0
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_RGB565_FAST_PATH_ENABLE              0
#define EGUI_CONFIG_IMAGE_STD_MASK_CIRCLE_DRAW_ALPHA8_FAST_PATH_ENABLE              0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_FAST_PATH_ENABLE                      0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_FAST_PATH_ENABLE               0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_ALPHA8_FAST_PATH_ENABLE        0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_RESIZE_RGB565_FAST_PATH_ENABLE        0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_FAST_PATH_ENABLE                 0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_RGB565_FAST_PATH_ENABLE          0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROUND_RECT_DRAW_ALPHA8_FAST_PATH_ENABLE          0
#define EGUI_CONFIG_IMAGE_STD_MASK_VISIBLE_RANGE_FAST_PATH_ENABLE                   0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_FAST_PATH_ENABLE                     0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_FAST_PATH_ENABLE                0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_ALPHA8_FAST_PATH_ENABLE         0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_DRAW_PACKED_ALPHA_FAST_PATH_ENABLE   0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_FAST_PATH_ENABLE              0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_ALPHA8_FAST_PATH_ENABLE       0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_PACKED_ALPHA_FAST_PATH_ENABLE 0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_PARTIAL_RESIZE_RGB565_FAST_PATH_ENABLE       0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_FAST_PATH_ENABLE                      0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_FAST_PATH_ENABLE               0
#define EGUI_CONFIG_IMAGE_STD_MASK_ROW_INSIDE_RESIZE_ALPHA8_FAST_PATH_ENABLE        0
#define EGUI_CONFIG_IMAGE_STD_RGB565_OPAQUE_SOURCE_CHECK_ENABLE                     0
#define EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_FAST_PATH_ENABLE                          0
#define EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_NON_RGB565_FAST_PATH_ENABLE               0
#define EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_ALPHA8_FAST_PATH_ENABLE                   0
#define EGUI_CONFIG_IMAGE_STD_ROW_OVERLAY_RGB565_FAST_PATH_ENABLE                   0
#define EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_FAST_PATH_ENABLE                      0
#define EGUI_CONFIG_MASK_IMAGE_IDENTITY_SCALE_BLEND_FAST_PATH_ENABLE                0
#define EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_FAST_PATH_ENABLE                        0
#define EGUI_CONFIG_CANVAS_MASK_FILL_CIRCLE_SEGMENT_FAST_PATH_ENABLE                0
#define EGUI_CONFIG_CANVAS_MASK_FILL_ROUND_RECT_FAST_PATH_ENABLE                    0
#define EGUI_CONFIG_CANVAS_MASK_FILL_ROW_BLEND_FAST_PATH_ENABLE                     0
#define EGUI_CONFIG_CANVAS_MASK_FILL_IMAGE_FAST_PATH_ENABLE                         0
#define EGUI_CONFIG_CANVAS_MASK_FILL_ROW_RANGE_FAST_PATH_ENABLE                     0
#define EGUI_CONFIG_CANVAS_MASK_FILL_ROW_PARTIAL_FAST_PATH_ENABLE                   0
#define EGUI_CONFIG_CANVAS_MASK_FILL_ROW_INSIDE_FAST_PATH_ENABLE                    0
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
#define EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE                          2

// Optional low-RAM experiment: masked opaque fallback rows can borrow the
// codec row-cache alpha backing store instead of reserving a dedicated
// decode-row alpha buffer. Current recheck says 1 vs default 0 saves about
// 132B text and 8B BSS with no measurable perf/runtime/unit regression.
#define EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE          1

// Historical low-RAM experiment only: dropping QOI checkpoints from the
// default 2 to 0 saves about 276B text and 8B BSS here, but current
// HelloPerformance recheck regresses 20 QOI scenes by about +110%~+911%.
#define EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT                           0

// Low-RAM QOI row-cache mode: only cache the horizontal tail instead of the
// full row-band. Saves about 1.5KB peak heap on 240px alpha-QOI scenes.
#define EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE                    1

// Historical low-RAM experiment: shrink the external RLE read window from the
// default 1024B to 64B. Current recheck saves about 1024B BSS for 96B extra
// text, but external RLE hotspots regress by about +17.5%~+21.8%.
#define EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE                 64

// Move the external RLE read window itself out of persistent BSS and reuse a
// transient frame-heap window instead. Current HelloPerformance A/B trades
// about 1.0KB BSS for about 0.8KB extra text and about +2.7%~+6.2% on tiled /
// external RLE scenes.
#define EGUI_CONFIG_IMAGE_RLE_EXTERNAL_WINDOW_PERSISTENT_CACHE_ENABLE    0

// Move std/external row-cache slot storage out of persistent globals and
// recreate it per source. Current HelloPerformance recheck keeps the full
// perf/runtime/unit set equivalent while trading about 20B text for 40B less
// BSS.
#define EGUI_CONFIG_IMAGE_STD_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE       0

// Move transform/external row-cache slot storage out of persistent globals and
// recreate it per source. Current HelloPerformance recheck keeps the full
// perf/runtime/unit set equivalent while saving about 24B text and 56B BSS.
#define EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE 0

// Disable persistent "source alpha is fully opaque" metadata cache slots.
#define EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS                   0

// Text-transform layout: 16-bit indices shrink transient heap for the current
// benchmark strings and font offsets.
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT              1
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT               1

// Rotated-text scratch: keep layout/tile scratch on transient heap and cap the
// visible alpha8 fast-path cache at the smaller low-RAM ceiling.
#define EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE                   1
#define EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES              2560
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
