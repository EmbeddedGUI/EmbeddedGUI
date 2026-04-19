#ifndef _UICODE_DISP1_H_
#define _UICODE_DISP1_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

void uicode_disp1_init(egui_core_t *core);
extern egui_color_int_t hello_multi_display_disp1_pfb[1][EGUI_CONFIG_PFB_1_WIDTH * EGUI_CONFIG_PFB_1_HEIGHT];
void hello_multi_display_hetero_sub_update_page_label(int page_index);
egui_view_t *hello_multi_display_hetero_sub_get_interaction_target(void);
uint32_t hello_multi_display_hetero_sub_get_tick_counter(void);
int hello_multi_display_hetero_sub_get_page_index(void);

#ifdef __cplusplus
}
#endif

#endif /* _UICODE_DISP1_H_ */
