#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

#define EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC 1

#define EGUI_CONFIG_DIRTY_AREA_COUNT                        2
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY                    1
#define EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS                  1
#define EGUI_CONFIG_FUNCTION_IMAGE_FILE                     1
#define EGUI_CONFIG_FUNCTION_SUPPORT_LAYER                  1
#define EGUI_CONFIG_FUNCTION_SUPPORT_MASK                   1
#define EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4     1
#define EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4_XOR 1
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
