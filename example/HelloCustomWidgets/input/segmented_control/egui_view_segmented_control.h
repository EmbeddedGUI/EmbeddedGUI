#ifndef _HELLO_CUSTOM_WIDGETS_SEGMENTED_CONTROL_STYLE_H_
#define _HELLO_CUSTOM_WIDGETS_SEGMENTED_CONTROL_STYLE_H_

#include "../../../../src/widget/egui_view_segmented_control.h"

#ifdef __cplusplus
extern "C" {
#endif

void hcw_segmented_control_apply_standard_style(egui_view_t *self);
void hcw_segmented_control_apply_compact_style(egui_view_t *self);
void hcw_segmented_control_apply_read_only_style(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif /* _HELLO_CUSTOM_WIDGETS_SEGMENTED_CONTROL_STYLE_H_ */
