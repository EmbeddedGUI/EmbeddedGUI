#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "size_analysis_app_config_override.h"

#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 20
#define EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE         0
#ifndef EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE            1
#endif
#ifndef EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE            1
#endif

#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
