#ifndef _EGUI_FONT_H_
#define _EGUI_FONT_H_

#include "core/egui_canvas.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_font_api egui_font_api_t;
struct egui_font_api
{
    int (*draw_string)(const egui_font_t *self, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha);
    int (*get_str_size)(const egui_font_t *self, const void *string, uint8_t is_multi_line, egui_dim_t line_space, egui_dim_t *width, egui_dim_t *height);
};

struct egui_font
{
    const void *res; // font resource

    const egui_font_api_t *api; // virtual api
};

#define EGUI_FONT_DEFINE_STATIC(_type, _name, _res)                                                                                                            \
    extern const egui_font_api_t _type##_api_table;                                                                                                            \
    static _type _name = {.res = _res, .api = &_type##_api_table}

#define EGUI_FONT_SUB_DEFINE_STATIC(_type, _name, _res)                                                                                                        \
    extern const egui_font_api_t _type##_api_table;                                                                                                            \
    static _type _name = {.base = {.res = _res, .api = &_type##_api_table}}

#define EGUI_FONT_SUB_DEFINE_CONST(_type, _name, _res)                                                                                                               \
    extern const egui_font_api_t _type##_api_table;                                                                                                            \
    _type const _name = {.base = {.res = _res, .api = &_type##_api_table}}

#define EGUI_FONT_SUB_DEFINE(_type, _name, _res)                                                                                                               \
    extern const egui_font_api_t _type##_api_table;                                                                                                            \
    _type _name = {.base = {.res = _res, .api = &_type##_api_table}}

int egui_font_get_utf8_code(const char *s, uint32_t *output_utf8_code);
void egui_font_draw_string_in_rect(const egui_font_t *self, const void *string, egui_region_t *rect, uint8_t align_type, egui_dim_t line_space, egui_color_t color,
                                   egui_alpha_t alpha);
void egui_font_init(egui_font_t *self, const void *res);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_FONT_H_ */
