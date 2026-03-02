

#ifndef _APP_EGUI_RESOURCE_GENERATE_H_
#define _APP_EGUI_RESOURCE_GENERATE_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#include "egui.h"

extern const egui_image_std_t egui_res_image_album_cover_rgb565_4;
extern const egui_font_std_t egui_res_font_materialsymbolsoutlined_regular_14_4;
extern const egui_font_std_t egui_res_font_materialsymbolsoutlined_regular_20_4;
extern const egui_font_std_t egui_res_font_montserrat_medium_14_4;
extern const egui_font_std_t egui_res_font_montserrat_medium_20_4;


enum {
EGUI_EXT_RES_ID_BASE = 0x00, // avoid conflict with NULL.

EGUI_EXT_RES_ID_MAX,
};

extern const uint32_t egui_ext_res_id_map[EGUI_EXT_RES_ID_MAX];

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_RESOURCE_GENERATE_H_ */

