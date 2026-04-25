#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

struct egui_font_std;
extern const struct egui_font_std egui_res_font_hello_simple_14_4;

#define EGUI_CONFIG_FONT_DEFAULT                              (&egui_res_font_hello_simple_14_4)
#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 20
#define EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW                   0
#define EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW             0
#define EGUI_CONFIG_FUNCTION_CANVAS_COMPACT_NUMBER                0
#define EGUI_CONFIG_FUNCTION_CANVAS_EXTRA_CLIP                    0
#define EGUI_CONFIG_FUNCTION_CANVAS_SPEC_CIRCLE_INFO              0
#define EGUI_CONFIG_FUNCTION_IMAGE_STD_FRAME_CACHE_RELEASE        0
#define EGUI_CONFIG_FUNCTION_IMAGE_STD_RGB565_ALPHA_OPAQUE_CACHE  0
#define EGUI_CONFIG_FUNCTION_CANVAS_TRANSFORM_FRAME_CACHE_RELEASE 0
#define EGUI_CONFIG_FUNCTION_FONT_STD_FRAME_CACHE_RELEASE         0
#define EGUI_CONFIG_FUNCTION_FONT_STD_CODE_LOOKUP_CACHE           0
#define EGUI_CONFIG_FUNCTION_TEXT_TRANSFORM_SIZE_CACHE            0
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4                0
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH                        1
#define EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY                     0
#define EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG                       0
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOAST                        0
#define EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC_LIBC_FALLBACK         0
#define EGUI_CONFIG_QEMU_PLATFORM_PRINTF_ENABLE                  0
#define EGUI_CONFIG_FUNCTION_VIEW_BUTTON_ICON                     0
#define EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_ONLY              0
#define EGUI_CONFIG_FUNCTION_VIEW_LABEL_COMPACT_FALLBACK          0
#define EGUI_CONFIG_FUNCTION_VIEW_GROUP_TOUCH_CAPTURE_PATH        0
#define EGUI_CONFIG_FUNCTION_CORE_PRE_COMPUTE_SCROLL              0
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE             0
#define EGUI_CONFIG_PFB_BUFFER_COUNT                              1
#define EGUI_CONFIG_DIRTY_AREA_COUNT                              1
#define EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT                      1
#define EGUI_CONFIG_IMAGE_FILE_DECODER_MAX_COUNT                  1

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
