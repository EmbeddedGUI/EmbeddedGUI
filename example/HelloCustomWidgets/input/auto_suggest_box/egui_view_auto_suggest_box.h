#ifndef _HELLO_CUSTOM_WIDGETS_AUTO_SUGGEST_BOX_STYLE_H_
#define _HELLO_CUSTOM_WIDGETS_AUTO_SUGGEST_BOX_STYLE_H_

#include "../../../../src/widget/egui_view_autocomplete.h"

#ifdef __cplusplus
extern "C" {
#endif

void hcw_auto_suggest_box_apply_standard_style(egui_view_t *self);
void hcw_auto_suggest_box_apply_compact_style(egui_view_t *self);
void hcw_auto_suggest_box_apply_read_only_style(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif /* _HELLO_CUSTOM_WIDGETS_AUTO_SUGGEST_BOX_STYLE_H_ */
