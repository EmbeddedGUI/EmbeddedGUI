#ifndef _UICODE_DISP0_H_
#define _UICODE_DISP0_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

void uicode_disp0_init(egui_core_t *core);
egui_view_t *hello_multi_display_disp0_get_next_button(int slot_index);
egui_view_t *hello_multi_display_disp0_get_finish_button(int slot_index);
int hello_multi_display_disp0_get_activity_slot_count(void);

#ifdef __cplusplus
}
#endif

#endif /* _UICODE_DISP0_H_ */
