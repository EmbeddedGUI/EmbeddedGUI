#ifndef _EGUI_CANVAS_COMPACT_H_
#define _EGUI_CANVAS_COMPACT_H_

#include "egui_canvas.h"
#include "font/egui_font.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_canvas_compact_text_layout egui_canvas_compact_text_layout_t;
struct egui_canvas_compact_text_layout
{
    egui_dim_t scale;
    egui_dim_t gap;
    egui_dim_t width;
    egui_dim_t height;
};

typedef struct egui_canvas_compact_number_layout egui_canvas_compact_number_layout_t;
struct egui_canvas_compact_number_layout
{
    egui_dim_t scale;
    egui_dim_t gap;
    egui_dim_t width;
    egui_dim_t height;
};

uint8_t egui_canvas_compact_text_is_supported(const char *text);
uint8_t egui_canvas_compact_text_measure(const char *text, egui_dim_t max_width, egui_dim_t max_height, egui_canvas_compact_text_layout_t *layout);
uint8_t egui_canvas_compact_text_measure_with_font(const egui_font_t *font, const char *text, egui_dim_t max_width, egui_dim_t max_height,
                                                   egui_dim_t *out_width, egui_dim_t *out_height);
uint8_t egui_canvas_compact_text_draw(const char *text, const egui_region_t *region, uint8_t align_type, egui_color_t color, egui_alpha_t alpha);
uint8_t egui_canvas_compact_text_draw_with_font(const egui_font_t *font, const char *text, const egui_region_t *region, uint8_t align_type,
                                                egui_dim_t line_space, uint8_t use_line_space, egui_color_t color, egui_alpha_t alpha);

uint8_t egui_canvas_compact_number_is_supported(const char *text);
uint8_t egui_canvas_compact_number_measure(const char *text, egui_dim_t max_width, egui_dim_t max_height, egui_canvas_compact_number_layout_t *layout);
uint8_t egui_canvas_compact_number_measure_with_font(const egui_font_t *font, const char *text, egui_dim_t max_width, egui_dim_t max_height,
                                                     egui_dim_t *out_width, egui_dim_t *out_height);
uint8_t egui_canvas_compact_number_draw(const char *text, const egui_region_t *region, egui_color_t color, egui_alpha_t alpha);
uint8_t egui_canvas_compact_number_draw_with_font(const egui_font_t *font, const char *text, const egui_region_t *region, egui_color_t color,
                                                  egui_alpha_t alpha);

void egui_canvas_draw_line_round_cap_compact(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color,
                                             egui_alpha_t alpha);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CANVAS_COMPACT_H_ */
