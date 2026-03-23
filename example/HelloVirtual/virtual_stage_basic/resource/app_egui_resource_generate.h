

#ifndef _APP_EGUI_RESOURCE_GENERATE_H_
#define _APP_EGUI_RESOURCE_GENERATE_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#include "egui.h"

extern const egui_image_std_t egui_res_image_star_alpha_4;


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

