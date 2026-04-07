#ifndef _SIZE_ANALYSIS_APP_CONFIG_OVERRIDE_H_
#define _SIZE_ANALYSIS_APP_CONFIG_OVERRIDE_H_

/*
 * Default placeholder values for size-analysis probes.
 * Individual report scripts override them via USER_CFLAGS (-D...),
 * so this header should stay stable in the repository.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUPPORT_MASK
#define EGUI_CONFIG_FUNCTION_SUPPORT_MASK  0
#endif

#ifndef EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE 1
#endif

#ifndef EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE 0
#endif

#endif /* _SIZE_ANALYSIS_APP_CONFIG_OVERRIDE_H_ */
