#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

#define EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC 1

#define EGUI_CONFIG_MAX_DISPLAY_COUNT                       3
#define EGUI_CONFIG_DIRTY_AREA_COUNT                        2
#define EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE    0
#define EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY               1
#define EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG                 1
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY                    1
#define EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS                  1
#define EGUI_CONFIG_FUNCTION_IMAGE_FILE                     1
#define EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE       1
#define EGUI_CONFIG_FUNCTION_SUPPORT_LAYER                  1
#define EGUI_CONFIG_FUNCTION_SUPPORT_MASK                   1
#define EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH      1
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOAST                  1
#define EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH            1
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG              1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8          1
#define EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4     1
#define EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4_XOR 1
#define EGUI_CONFIG_FUNCTION_FLEXLAYOUT                     1
#define EGUI_CONFIG_FUNCTION_STYLE_CASCADE                  1
#define EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES              1
#define EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER               1
#define EGUI_CONFIG_FUNCTION_FONT_TTF                       1
#define EGUI_CONFIG_FUNCTION_ENCODER                        1
#define EGUI_CONFIG_FUNCTION_EVENT_LITE                     1
#define EGUI_CONFIG_FUNCTION_MSGBOX                         1
#define EGUI_CONFIG_FUNCTION_FOCUS_GROUP                    1
#define EGUI_CONFIG_FUNCTION_PROPERTY_LITE                  1
#define EGUI_CONFIG_FUNCTION_ANIM_DELAY                     1
#define EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB               1
#define EGUI_CONFIG_FUNCTION_ANIM_PAUSE_RESUME              1
#define EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL              1
#define EGUI_CONFIG_FUNCTION_LABEL_RECOLOR                  1
#define EGUI_CONFIG_FUNCTION_VIEW_USER_DATA                 1
#define EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE             1
#define EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP                1
#define EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT                 1
#define EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA                 1
#define EGUI_CONFIG_FUNCTION_LONG_PRESS                     1
#define EGUI_CONFIG_FUNCTION_SWIPE_LISTENER                 1
#define EGUI_CONFIG_FUNCTION_SCROLL_SNAP                    1
#define EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW                 1
#define EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM                1
#define EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE                1
#define EGUI_CONFIG_FUNCTION_SCROLL_LISTENER                1
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
