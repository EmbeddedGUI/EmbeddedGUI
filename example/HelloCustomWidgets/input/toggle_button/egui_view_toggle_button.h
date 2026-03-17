#ifndef _HELLO_CUSTOM_WIDGETS_TOGGLE_BUTTON_STYLE_H_
#define _HELLO_CUSTOM_WIDGETS_TOGGLE_BUTTON_STYLE_H_

#include "../../../../src/widget/egui_view_toggle_button.h"

#ifdef __cplusplus
extern "C" {
#endif

void hcw_toggle_button_apply_standard_style(egui_view_t *self);
void hcw_toggle_button_apply_compact_style(egui_view_t *self);
void hcw_toggle_button_apply_read_only_style(egui_view_t *self);
int hcw_toggle_button_on_key_event(egui_view_t *self, egui_key_event_t *event);

#ifdef __cplusplus
}
#endif

#endif /* _HELLO_CUSTOM_WIDGETS_TOGGLE_BUTTON_STYLE_H_ */
