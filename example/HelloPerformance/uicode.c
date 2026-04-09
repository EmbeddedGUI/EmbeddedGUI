#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

#include "egui_view_test_performance.h"
#include "image/egui_image_qoi.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#if EGUI_PORT == EGUI_PORT_TYPE_QEMU
#define TEST_REPEAT_COUNT 3
#else
#define TEST_REPEAT_COUNT 10
#endif

// Set to a specific EGUI_VIEW_TEST_PERFORMANCE_TYPE_* value to run only that test,
// or set to -1 to run all tests.
#ifndef EGUI_TEST_CONFIG_SINGLE_TEST
#define EGUI_TEST_CONFIG_SINGLE_TEST -1
#endif

// views in test_view_group_1
static egui_view_test_performance_t test_view;

static const char *egui_view_test_performance_type_string(int test_mode);
static void egui_view_test_performance_set_test_mode(int test_mode);
static int egui_view_test_performance_is_qoi_test_mode(int test_mode);
static int egui_view_test_performance_uses_logical_pfb_96_hint(int test_mode);
#if EGUI_CONFIG_RECORDING_TEST
static void egui_view_test_performance_show_test_mode(int test_mode);
static int egui_view_test_performance_get_recording_test_mode(int action_index);
#if EGUI_PORT == EGUI_PORT_TYPE_PC
static const char *g_recording_scene_name = NULL;
#endif
#endif

void uicode_init_ui(void)
{
    // Init all views
    // test_view
    egui_view_test_performance_init(EGUI_VIEW_OF(&test_view));
    egui_view_set_position(EGUI_VIEW_OF(&test_view), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_view));
}

static void user_manu_refresh_screen(void)
{
    // egui_screen_on() resumes the global refresh timer. Stop it again
    // before entering the manual performance refresh path.
    egui_core_stop_auto_refresh_screen();

    // Manual refresh screen three time to test performance.
    egui_core_force_refresh();
    egui_core_refresh_screen();

#if EGUI_PORT == EGUI_PORT_TYPE_PC
    // Brief delay for SDL event processing
    egui_api_delay(500);
#endif

#if EGUI_PORT == EGUI_PORT_TYPE_QEMU
    /* Use microsecond timer for high-resolution measurement in QEMU */
    extern uint32_t qemu_get_tick_us(void);
    uint32_t start_us = qemu_get_tick_us();
    for (int i = 0; i < TEST_REPEAT_COUNT; i++)
    {
        egui_core_force_refresh();
        egui_polling_refresh_display();
    }
    uint32_t total_us = qemu_get_tick_us() - start_us;
    uint32_t avg_us = total_us / TEST_REPEAT_COUNT;
    EGUI_LOG_INF("Refresh Screen Time: %d.%03dms\r\n", avg_us / 1000, avg_us % 1000);

    // Structured output for automated parsing
    EGUI_LOG_INF("PERF_RESULT:%s:%d.%03d\r\n", egui_view_test_performance_type_string(test_view.test_mode), avg_us / 1000, avg_us % 1000);
#else
    uint32_t start_time = egui_api_timer_get_current();
    for (int i = 0; i < TEST_REPEAT_COUNT; i++)
    {
        egui_core_force_refresh();
        egui_polling_refresh_display();
    }
    // get time.
    start_time = egui_api_timer_get_current() - start_time;
    EGUI_LOG_INF("Refresh Screen Time: %d.%dms\r\n", start_time / TEST_REPEAT_COUNT, start_time % TEST_REPEAT_COUNT);

    // Structured output for automated parsing
    EGUI_LOG_INF("PERF_RESULT:%s:%d.%d\r\n", egui_view_test_performance_type_string(test_view.test_mode), start_time / TEST_REPEAT_COUNT,
                 start_time % TEST_REPEAT_COUNT);
#endif

    // egui_api_refresh_display() in pc will use too much time, so we calc time.
    egui_api_refresh_display();
}

static const char *egui_view_test_performance_type_string(int test_mode)
{
    switch (test_mode)
    {
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_LINE:
        return "LINE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565:
        return "IMAGE_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_1:
        return "IMAGE_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_2:
        return "IMAGE_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_4:
        return "IMAGE_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_8:
        return "IMAGE_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565:
        return "IMAGE_RESIZE_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_1:
        return "IMAGE_RESIZE_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_2:
        return "IMAGE_RESIZE_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_4:
        return "IMAGE_RESIZE_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_8:
        return "IMAGE_RESIZE_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT:
        return "TEXT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_RECT:
        return "TEXT_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT:
        return "INTERNAL_TEXT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RECT:
        return "INTERNAL_TEXT_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RLE4:
        return "INTERNAL_TEXT_RLE4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RECT_RLE4:
        return "INTERNAL_TEXT_RECT_RLE4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RLE4_XOR:
        return "INTERNAL_TEXT_RLE4_XOR";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RECT_RLE4_XOR:
        return "INTERNAL_TEXT_RECT_RLE4_XOR";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT:
        return "EXTERN_TEXT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RECT:
        return "EXTERN_TEXT_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RLE4:
        return "EXTERN_TEXT_RLE4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RECT_RLE4:
        return "EXTERN_TEXT_RECT_RLE4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RLE4_XOR:
        return "EXTERN_TEXT_RLE4_XOR";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RECT_RLE4_XOR:
        return "EXTERN_TEXT_RECT_RLE4_XOR";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_RECTANGLE:
        return "RECTANGLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE:
        return "CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC:
        return "ARC";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE:
        return "ROUND_RECTANGLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_CORNERS:
        return "ROUND_RECTANGLE_CORNERS";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_RECTANGLE_FILL:
        return "RECTANGLE_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL:
        return "CIRCLE_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_FILL:
        return "ARC_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_FILL:
        return "ROUND_RECTANGLE_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_CORNERS_FILL:
        return "ROUND_RECTANGLE_CORNERS_FILL";

    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TRIANGLE:
        return "TRIANGLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TRIANGLE_FILL:
        return "TRIANGLE_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ELLIPSE:
        return "ELLIPSE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ELLIPSE_FILL:
        return "ELLIPSE_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON:
        return "POLYGON";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON_FILL:
        return "POLYGON_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_QUAD:
        return "BEZIER_QUAD";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_CUBIC:
        return "BEZIER_CUBIC";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_HQ:
        return "CIRCLE_HQ";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL_HQ:
        return "CIRCLE_FILL_HQ";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_HQ:
        return "ARC_HQ";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_FILL_HQ:
        return "ARC_FILL_HQ";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_LINE_HQ:
        return "LINE_HQ";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_RECT:
        return "GRADIENT_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT:
        return "GRADIENT_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_CIRCLE:
        return "GRADIENT_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_TRIANGLE:
        return "GRADIENT_TRIANGLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ARC_RING:
        return "GRADIENT_ARC_RING";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ARC_RING_ROUND_CAP:
        return "GRADIENT_ARC_RING_ROUND_CAP";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_RADIAL:
        return "GRADIENT_RADIAL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ANGULAR:
        return "GRADIENT_ANGULAR";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT_RING:
        return "GRADIENT_ROUND_RECT_RING";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_LINE_CAPSULE:
        return "GRADIENT_LINE_CAPSULE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_MULTI_STOP:
        return "GRADIENT_MULTI_STOP";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT_CORNERS:
        return "GRADIENT_ROUND_RECT_CORNERS";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_GRADIENT_OVERLAY:
        return "IMAGE_GRADIENT_OVERLAY";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_GRADIENT_RECT_FILL:
        return "MASK_GRADIENT_RECT_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_GRADIENT_IMAGE:
        return "MASK_GRADIENT_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_GRADIENT_IMAGE_ROTATE:
        return "MASK_GRADIENT_IMAGE_ROTATE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_COLOR:
        return "IMAGE_COLOR";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_COLOR:
        return "IMAGE_RESIZE_COLOR";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_SHADOW:
        return "SHADOW";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_SHADOW_ROUND:
        return "SHADOW_ROUND";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_GRADIENT:
        return "TEXT_GRADIENT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_RECT_GRADIENT:
        return "TEXT_RECT_GRADIENT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_GRADIENT:
        return "TEXT_ROTATE_GRADIENT";

    // Mask performance tests
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_NO_MASK:
        return "MASK_RECT_FILL_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_ROUND_RECT:
        return "MASK_RECT_FILL_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_CIRCLE:
        return "MASK_RECT_FILL_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_IMAGE:
        return "MASK_RECT_FILL_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_NO_MASK:
        return "MASK_IMAGE_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_ROUND_RECT:
        return "MASK_IMAGE_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_CIRCLE:
        return "MASK_IMAGE_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_IMAGE:
        return "MASK_IMAGE_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_NO_MASK:
        return "EXTERN_MASK_IMAGE_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_ROUND_RECT:
        return "EXTERN_MASK_IMAGE_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_CIRCLE:
        return "EXTERN_MASK_IMAGE_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_IMAGE:
        return "EXTERN_MASK_IMAGE_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_TEST_PERF_NO_MASK:
        return "MASK_IMAGE_TEST_PERF_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_TEST_PERF_ROUND_RECT:
        return "MASK_IMAGE_TEST_PERF_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_TEST_PERF_CIRCLE:
        return "MASK_IMAGE_TEST_PERF_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_TEST_PERF_IMAGE:
        return "MASK_IMAGE_TEST_PERF_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK:
        return "EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT:
        return "EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE:
        return "EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_IMAGE:
        return "EXTERN_MASK_IMAGE_TEST_PERF_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_ROUND_RECT_FILL_NO_MASK:
        return "MASK_ROUND_RECT_FILL_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_ROUND_RECT_FILL_WITH_MASK:
        return "MASK_ROUND_RECT_FILL_WITH_MASK";

    // Animation performance tests
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_TRANSLATE:
        return "ANIMATION_TRANSLATE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_ALPHA:
        return "ANIMATION_ALPHA";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_SCALE:
        return "ANIMATION_SCALE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_SET:
        return "ANIMATION_SET";

    // Image multi-size
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_QUARTER:
        return "IMAGE_565_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_DOUBLE:
        return "IMAGE_565_DOUBLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_8_QUARTER:
        return "IMAGE_565_8_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_8_DOUBLE:
        return "IMAGE_565_8_DOUBLE";

    // Extern image
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565:
        return "EXTERN_IMAGE_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_1:
        return "EXTERN_IMAGE_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_2:
        return "EXTERN_IMAGE_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_4:
        return "EXTERN_IMAGE_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_8:
        return "EXTERN_IMAGE_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565:
        return "EXTERN_IMAGE_RESIZE_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_1:
        return "EXTERN_IMAGE_RESIZE_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_2:
        return "EXTERN_IMAGE_RESIZE_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_4:
        return "EXTERN_IMAGE_RESIZE_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_8:
        return "EXTERN_IMAGE_RESIZE_565_8";

    // Mask multi-size: rect fill
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_NO_MASK_QUARTER:
        return "MASK_RECT_FILL_NO_MASK_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_NO_MASK_DOUBLE:
        return "MASK_RECT_FILL_NO_MASK_DOUBLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_ROUND_RECT_QUARTER:
        return "MASK_RECT_FILL_ROUND_RECT_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_ROUND_RECT_DOUBLE:
        return "MASK_RECT_FILL_ROUND_RECT_DOUBLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_CIRCLE_QUARTER:
        return "MASK_RECT_FILL_CIRCLE_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_CIRCLE_DOUBLE:
        return "MASK_RECT_FILL_CIRCLE_DOUBLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_IMAGE_QUARTER:
        return "MASK_RECT_FILL_IMAGE_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_IMAGE_DOUBLE:
        return "MASK_RECT_FILL_IMAGE_DOUBLE";

    // Mask multi-size: image
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_NO_MASK_QUARTER:
        return "MASK_IMAGE_NO_MASK_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_NO_MASK_DOUBLE:
        return "MASK_IMAGE_NO_MASK_DOUBLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_ROUND_RECT_QUARTER:
        return "MASK_IMAGE_ROUND_RECT_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_ROUND_RECT_DOUBLE:
        return "MASK_IMAGE_ROUND_RECT_DOUBLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_CIRCLE_QUARTER:
        return "MASK_IMAGE_CIRCLE_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_CIRCLE_DOUBLE:
        return "MASK_IMAGE_CIRCLE_DOUBLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_IMAGE_QUARTER:
        return "MASK_IMAGE_IMAGE_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_IMAGE_DOUBLE:
        return "MASK_IMAGE_IMAGE_DOUBLE";

    // Shape fill multi-size
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL_QUARTER:
        return "CIRCLE_FILL_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL_DOUBLE:
        return "CIRCLE_FILL_DOUBLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_FILL_QUARTER:
        return "ROUND_RECTANGLE_FILL_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_FILL_DOUBLE:
        return "ROUND_RECTANGLE_FILL_DOUBLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TRIANGLE_FILL_QUARTER:
        return "TRIANGLE_FILL_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TRIANGLE_FILL_DOUBLE:
        return "TRIANGLE_FILL_DOUBLE";

    // Image transform (rotation)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565:
        return "IMAGE_ROTATE_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_1:
        return "IMAGE_ROTATE_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_2:
        return "IMAGE_ROTATE_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_4:
        return "IMAGE_ROTATE_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_8:
        return "IMAGE_ROTATE_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_RESIZE:
        return "IMAGE_ROTATE_565_RESIZE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_QUARTER:
        return "IMAGE_ROTATE_565_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_DOUBLE:
        return "IMAGE_ROTATE_565_DOUBLE";

    // External image transform (rotation)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565:
        return "EXTERN_IMAGE_ROTATE_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_1:
        return "EXTERN_IMAGE_ROTATE_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_2:
        return "EXTERN_IMAGE_ROTATE_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_4:
        return "EXTERN_IMAGE_ROTATE_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_8:
        return "EXTERN_IMAGE_ROTATE_565_8";

    // Star image tests (real alpha)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_1:
        return "IMAGE_RESIZE_STAR_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_2:
        return "IMAGE_RESIZE_STAR_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_4:
        return "IMAGE_RESIZE_STAR_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_8:
        return "IMAGE_RESIZE_STAR_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_1:
        return "IMAGE_ROTATE_STAR_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_2:
        return "IMAGE_ROTATE_STAR_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_4:
        return "IMAGE_ROTATE_STAR_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_8:
        return "IMAGE_ROTATE_STAR_565_8";

    // Tiled draw (40x40)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_0:
        return "IMAGE_TILED_565_0";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_1:
        return "IMAGE_TILED_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_2:
        return "IMAGE_TILED_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_4:
        return "IMAGE_TILED_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_8:
        return "IMAGE_TILED_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_0:
        return "IMAGE_TILED_STAR_565_0";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_1:
        return "IMAGE_TILED_STAR_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_2:
        return "IMAGE_TILED_STAR_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_4:
        return "IMAGE_TILED_STAR_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_8:
        return "IMAGE_TILED_STAR_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_0:
        return "EXTERN_IMAGE_TILED_565_0";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_1:
        return "EXTERN_IMAGE_TILED_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_2:
        return "EXTERN_IMAGE_TILED_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_4:
        return "EXTERN_IMAGE_TILED_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_8:
        return "EXTERN_IMAGE_TILED_565_8";

    // Tiled resize (160x160)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_0:
        return "IMAGE_RESIZE_TILED_565_0";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_1:
        return "IMAGE_RESIZE_TILED_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_2:
        return "IMAGE_RESIZE_TILED_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_4:
        return "IMAGE_RESIZE_TILED_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_8:
        return "IMAGE_RESIZE_TILED_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_0:
        return "IMAGE_RESIZE_TILED_STAR_565_0";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_1:
        return "IMAGE_RESIZE_TILED_STAR_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_2:
        return "IMAGE_RESIZE_TILED_STAR_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_4:
        return "IMAGE_RESIZE_TILED_STAR_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_8:
        return "IMAGE_RESIZE_TILED_STAR_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_0:
        return "EXTERN_IMAGE_RESIZE_TILED_565_0";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_1:
        return "EXTERN_IMAGE_RESIZE_TILED_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_2:
        return "EXTERN_IMAGE_RESIZE_TILED_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_4:
        return "EXTERN_IMAGE_RESIZE_TILED_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_8:
        return "EXTERN_IMAGE_RESIZE_TILED_565_8";

    // Tiled rotate (80x80 slots 45deg)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_0:
        return "IMAGE_ROTATE_TILED_565_0";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_1:
        return "IMAGE_ROTATE_TILED_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_2:
        return "IMAGE_ROTATE_TILED_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_4:
        return "IMAGE_ROTATE_TILED_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_8:
        return "IMAGE_ROTATE_TILED_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_0:
        return "IMAGE_ROTATE_TILED_STAR_565_0";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_1:
        return "IMAGE_ROTATE_TILED_STAR_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_2:
        return "IMAGE_ROTATE_TILED_STAR_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_4:
        return "IMAGE_ROTATE_TILED_STAR_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_8:
        return "IMAGE_ROTATE_TILED_STAR_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_0:
        return "EXTERN_IMAGE_ROTATE_TILED_565_0";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_1:
        return "EXTERN_IMAGE_ROTATE_TILED_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_2:
        return "EXTERN_IMAGE_ROTATE_TILED_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_4:
        return "EXTERN_IMAGE_ROTATE_TILED_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_8:
        return "EXTERN_IMAGE_ROTATE_TILED_565_8";

    // Text transform (rotation)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_NONE:
        return "TEXT_ROTATE_NONE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE:
        return "TEXT_ROTATE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_RESIZE:
        return "TEXT_ROTATE_RESIZE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_QUARTER:
        return "TEXT_ROTATE_QUARTER";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_DOUBLE:
        return "TEXT_ROTATE_DOUBLE";

    // External text transform (rotation)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_ROTATE:
        return "EXTERN_TEXT_ROTATE";

    // Compressed image tests (QOI)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_QOI_565:
        return "IMAGE_QOI_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_QOI_565_8:
        return "IMAGE_QOI_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_QOI_565:
        return "EXTERN_IMAGE_QOI_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_QOI_565_8:
        return "EXTERN_IMAGE_QOI_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_QOI_565_0:
        return "IMAGE_TILED_QOI_565_0";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_QOI_565_8:
        return "IMAGE_TILED_QOI_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_NO_MASK:
        return "MASK_IMAGE_QOI_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_ROUND_RECT:
        return "MASK_IMAGE_QOI_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_CIRCLE:
        return "MASK_IMAGE_QOI_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_IMAGE:
        return "MASK_IMAGE_QOI_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_NO_MASK:
        return "MASK_IMAGE_QOI_8_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_ROUND_RECT:
        return "MASK_IMAGE_QOI_8_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_CIRCLE:
        return "MASK_IMAGE_QOI_8_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_IMAGE:
        return "MASK_IMAGE_QOI_8_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_NO_MASK:
        return "EXTERN_MASK_IMAGE_QOI_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_ROUND_RECT:
        return "EXTERN_MASK_IMAGE_QOI_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_CIRCLE:
        return "EXTERN_MASK_IMAGE_QOI_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_IMAGE:
        return "EXTERN_MASK_IMAGE_QOI_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_NO_MASK:
        return "EXTERN_MASK_IMAGE_QOI_8_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT:
        return "EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_CIRCLE:
        return "EXTERN_MASK_IMAGE_QOI_8_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_IMAGE:
        return "EXTERN_MASK_IMAGE_QOI_8_IMAGE";

    // Compressed image tests (RLE)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RLE_565:
        return "IMAGE_RLE_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RLE_565_8:
        return "IMAGE_RLE_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RLE_565:
        return "EXTERN_IMAGE_RLE_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RLE_565_8:
        return "EXTERN_IMAGE_RLE_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_RLE_565_0:
        return "IMAGE_TILED_RLE_565_0";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_RLE_565_8:
        return "IMAGE_TILED_RLE_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_NO_MASK:
        return "MASK_IMAGE_RLE_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_ROUND_RECT:
        return "MASK_IMAGE_RLE_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_CIRCLE:
        return "MASK_IMAGE_RLE_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_IMAGE:
        return "MASK_IMAGE_RLE_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_NO_MASK:
        return "MASK_IMAGE_RLE_8_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_ROUND_RECT:
        return "MASK_IMAGE_RLE_8_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_CIRCLE:
        return "MASK_IMAGE_RLE_8_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_IMAGE:
        return "MASK_IMAGE_RLE_8_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_NO_MASK:
        return "EXTERN_MASK_IMAGE_RLE_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_ROUND_RECT:
        return "EXTERN_MASK_IMAGE_RLE_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_CIRCLE:
        return "EXTERN_MASK_IMAGE_RLE_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_IMAGE:
        return "EXTERN_MASK_IMAGE_RLE_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_NO_MASK:
        return "EXTERN_MASK_IMAGE_RLE_8_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT:
        return "EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_CIRCLE:
        return "EXTERN_MASK_IMAGE_RLE_8_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_IMAGE:
        return "EXTERN_MASK_IMAGE_RLE_8_IMAGE";

    default:
        return "Unknown";
    }
}

static void egui_view_test_performance_set_test_mode(int test_mode)
{
    if (test_view.test_mode == test_mode)
    {
        return;
    }

#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
    if (egui_view_test_performance_is_qoi_test_mode(test_view.test_mode) && !egui_view_test_performance_is_qoi_test_mode(test_mode))
    {
        egui_image_qoi_release_checkpoints();
    }
#endif

    test_view.test_mode = test_mode;
    egui_view_invalidate(EGUI_VIEW_OF(&test_view));
}

static int egui_view_test_performance_is_qoi_test_mode(int test_mode)
{
    return test_mode >= EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_QOI_565 && test_mode <= EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_IMAGE;
}

static int egui_view_test_performance_uses_logical_pfb_96_hint(int test_mode)
{
    switch (test_mode)
    {
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_QOI_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_QOI_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_QOI_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_QOI_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_QOI_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_QOI_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RLE_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RLE_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RLE_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RLE_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_RLE_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_IMAGE:
        return 1;

    default:
        return 0;
    }
}

egui_dim_t egui_core_get_logical_pfb_target_width_hint(void)
{
    /* Keep the default 48x16 walk for most scenes.
     * The current high-heap codec hotspots can reuse a wider logical 96x8 walk,
     * which shrinks the tail-row cache without changing the physical PFB bytes. */
    return egui_view_test_performance_uses_logical_pfb_96_hint(test_view.test_mode) ? 96 : 0;
}

void egui_view_test_performance_with_test_mode(int test_mode)
{
    EGUI_LOG_INF("=========== Test Mode: %s ===========\r\n", egui_view_test_performance_type_string(test_mode));
    egui_view_test_performance_set_test_mode(test_mode);
    user_manu_refresh_screen();
}

void egui_view_test_performance_process(void)
{
#if EGUI_TEST_CONFIG_SINGLE_TEST >= 0
    // Single test mode
    if (egui_view_test_performance_is_enabled(EGUI_TEST_CONFIG_SINGLE_TEST))
    {
        egui_view_test_performance_with_test_mode(EGUI_TEST_CONFIG_SINGLE_TEST);
    }
    else
    {
        EGUI_LOG_INF("=========== Test Mode: %s [SKIP] ===========\r\n", egui_view_test_performance_type_string(EGUI_TEST_CONFIG_SINGLE_TEST));
    }
#else
    for (int i = 0; i < EGUI_VIEW_TEST_PERFORMANCE_TYPE_MAX; i++)
    {
        if (!egui_view_test_performance_is_enabled(i))
        {
            EGUI_LOG_INF("=========== Test Mode: %s [SKIP] ===========\r\n", egui_view_test_performance_type_string(i));
            EGUI_LOG_INF("PERF_SKIP:%s\r\n", egui_view_test_performance_type_string(i));
            continue;
        }
        egui_view_test_performance_with_test_mode(i);
    }
#endif

    EGUI_LOG_INF("PERF_COMPLETE\r\n");

#if EGUI_PORT == EGUI_PORT_TYPE_QEMU
    extern void qemu_notify_perf_complete(void);
    qemu_notify_perf_complete();
#endif

    // Test Single
    // egui_view_test_performance_with_test_mode(EGUI_VIEW_TEST_PERFORMANCE_TYPE_RECTANGLE_FILL);
}

static egui_timer_t ui_timer;
void egui_view_test_performance_timer_callback(egui_timer_t *timer)
{
    egui_view_test_performance_process();
}

void uicode_create_ui(void)
{
    uicode_init_ui();

#if EGUI_TEST_CONFIG_SINGLE_TEST >= 0
    if (egui_view_test_performance_is_enabled(EGUI_TEST_CONFIG_SINGLE_TEST))
    {
        egui_view_test_performance_set_test_mode(EGUI_TEST_CONFIG_SINGLE_TEST);
    }
#endif

    // For test performance, use manual refresh screen.
    egui_core_stop_auto_refresh_screen();

#if !EGUI_CONFIG_RECORDING_TEST
    ui_timer.callback = egui_view_test_performance_timer_callback;
    egui_timer_start_timer(&ui_timer, 100, 0);
#endif
}

#if EGUI_CONFIG_RECORDING_TEST
static void egui_view_test_performance_show_test_mode(int test_mode)
{
    egui_core_stop_auto_refresh_screen();
    egui_view_test_performance_set_test_mode(test_mode);
    egui_core_force_refresh();
    egui_core_refresh_screen();
}

#if EGUI_PORT == EGUI_PORT_TYPE_PC
const char *egui_port_get_recording_frame_label(void)
{
    return g_recording_scene_name;
}
#endif

static int egui_view_test_performance_get_recording_test_mode(int action_index)
{
#if EGUI_TEST_CONFIG_SINGLE_TEST >= 0
    if (action_index > 0)
    {
        return -1;
    }
    if (!egui_view_test_performance_is_enabled(EGUI_TEST_CONFIG_SINGLE_TEST))
    {
        return -1;
    }
    return EGUI_TEST_CONFIG_SINGLE_TEST;
#else
    int enabled_index = 0;
    for (int test_mode = 0; test_mode < EGUI_VIEW_TEST_PERFORMANCE_TYPE_MAX; test_mode++)
    {
        if (!egui_view_test_performance_is_enabled(test_mode))
        {
            continue;
        }
        if (enabled_index == action_index)
        {
            return test_mode;
        }
        enabled_index++;
    }
    return -1;
#endif
}

/**
 * @brief Recording actions for HelloPerformance.
 * Switch scenes one by one so each performance case gets a stable snapshot.
 */
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action_index = -1;
    int test_mode = egui_view_test_performance_get_recording_test_mode(action_index);
    int first_call = (action_index != last_action_index);

    if (test_mode < 0)
    {
        return false;
    }

    if (first_call)
    {
        const char *name = egui_view_test_performance_type_string(test_mode);
#if EGUI_PORT == EGUI_PORT_TYPE_PC
        g_recording_scene_name = name;
#endif
        EGUI_LOG_INF("PERF_SCENE:%s\r\n", name);
#if EGUI_PORT == EGUI_PORT_TYPE_PC
        // Request before forcing refresh so the snapshot waits for the new scene.
        recording_request_snapshot();
#endif
        egui_view_test_performance_show_test_mode(test_mode);
    }

    last_action_index = action_index;
    EGUI_SIM_SET_WAIT(p_action, 0);
    return true;
}
#endif
