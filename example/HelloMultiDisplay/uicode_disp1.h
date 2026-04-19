#ifndef _UICODE_DISP1_H_
#define _UICODE_DISP1_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

void uicode_disp1_init(egui_core_t *core);
extern egui_color_int_t hello_multi_display_disp1_pfb[1][EGUI_CONFIG_PFB_1_WIDTH * EGUI_CONFIG_PFB_1_HEIGHT];
egui_view_t *hello_multi_display_disp1_get_next_button(int slot_index);
egui_view_t *hello_multi_display_disp1_get_finish_button(int slot_index);
int hello_multi_display_disp1_get_activity_slot_count(void);

#ifdef __cplusplus
}
#endif

#endif /* _UICODE_DISP1_H_ */
