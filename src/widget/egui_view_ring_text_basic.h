#ifndef _EGUI_VIEW_RING_TEXT_BASIC_H_
#define _EGUI_VIEW_RING_TEXT_BASIC_H_

#include "egui_view.h"

/**
 * @brief Shared text helpers for circular widgets that show one numeric value.
 *
 * These routines centralize the default font choice, placement rules, and
 * dirty-region computation used by ring-like widgets such as gauges and
 * progress indicators.
 * They are intentionally minimal and optimized for the stock one-line numeric
 * label pattern used by many built-in circular widgets.
 */

/** Format a ring value into `buf`, optionally appending `%`. Callers must provide enough room for the trailing terminator. */
void egui_view_ring_text_format_value(uint8_t value, uint8_t append_percent, char *buf, uint32_t buf_size);
/** Compute the dirty region for the stock centered ring label so widgets can invalidate only the text area. */
uint8_t egui_view_ring_text_get_region_basic(egui_core_t *core, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t inner_r, egui_dim_t top_offset,
                                             uint8_t center_box, uint8_t value, uint8_t append_percent, egui_region_t *text_region);
/** Draw the stock ring label using the same placement assumptions as `egui_view_ring_text_get_region_basic`. */
void egui_view_ring_text_draw_basic(egui_canvas_t *canvas, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t inner_r, egui_dim_t top_offset,
                                    uint8_t center_box, uint8_t value, uint8_t append_percent, egui_color_t color, egui_alpha_t alpha);

#endif /* _EGUI_VIEW_RING_TEXT_BASIC_H_ */
