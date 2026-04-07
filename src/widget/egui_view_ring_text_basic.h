#ifndef _EGUI_VIEW_RING_TEXT_BASIC_H_
#define _EGUI_VIEW_RING_TEXT_BASIC_H_

#include "egui_view.h"

void egui_view_ring_text_format_value(uint8_t value, uint8_t append_percent, char *buf, uint32_t buf_size);
uint8_t egui_view_ring_text_get_region_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t inner_r, egui_dim_t top_offset, uint8_t center_box,
                                             uint8_t value, uint8_t append_percent, egui_region_t *text_region);
void egui_view_ring_text_draw_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t inner_r, egui_dim_t top_offset, uint8_t center_box, uint8_t value,
                                    uint8_t append_percent, egui_color_t color, egui_alpha_t alpha);

#endif /* _EGUI_VIEW_RING_TEXT_BASIC_H_ */
