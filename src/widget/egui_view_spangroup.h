#ifndef _EGUI_VIEW_SPANGROUP_H_
#define _EGUI_VIEW_SPANGROUP_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of spans stored directly inside one span-group widget. */
#define EGUI_VIEW_SPANGROUP_MAX_SPANS 8

/**
 * @brief One text fragment with its own font and color.
 */
typedef struct egui_view_span
{
    const char *text;
    const egui_font_t *font;
    egui_color_t color;
} egui_view_span_t;

typedef struct egui_view_spangroup egui_view_spangroup_t;
/**
 * @brief Small fixed-capacity rich-text container.
 *
 * Each span can use a different font and color. The built-in renderer places
 * spans left-to-right and wraps whole spans to the next line when needed.
 */
struct egui_view_spangroup
{
    egui_view_t base;

    egui_view_span_t spans[EGUI_VIEW_SPANGROUP_MAX_SPANS];
    uint8_t span_count;
    uint8_t line_spacing;
    uint8_t align;
};

// ============== Spangroup Params ==============
typedef struct egui_view_spangroup_params egui_view_spangroup_params_t;
/**
 * @brief Construction-time parameter block for one span-group container.
 */
struct egui_view_spangroup_params
{
    egui_region_t region;
};

/** Build a span-group parameter block with region only. */
#define EGUI_VIEW_SPANGROUP_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_spangroup_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

/** Append one span entry. Returns the new index, or -1 when the fixed span array is full. */
int egui_view_spangroup_add_span(egui_view_t *self, const char *text, const egui_font_t *font, egui_color_t color);
/** Return the number of stored spans. Returns 0 when self is NULL. */
uint8_t egui_view_spangroup_get_span_count(egui_view_t *self);
/** Return one stored span by index, or NULL when out of range or self is NULL. */
const egui_view_span_t *egui_view_spangroup_get_span(egui_view_t *self, uint8_t index);
/** Remove every stored span. */
void egui_view_spangroup_clear(egui_view_t *self);
/** Store an alignment hint for the span group. The current built-in renderer still draws from the left edge. */
void egui_view_spangroup_set_align(egui_view_t *self, uint8_t align);
/** Return the stored alignment hint. Returns 0 when self is NULL. */
uint8_t egui_view_spangroup_get_align(egui_view_t *self);
/** Set the extra vertical spacing inserted when text wraps onto a new line. */
void egui_view_spangroup_set_line_spacing(egui_view_t *self, uint8_t spacing);
/** Return the extra vertical spacing inserted between wrapped rows. Returns 0 when self is NULL. */
uint8_t egui_view_spangroup_get_line_spacing(egui_view_t *self);

/** Apply a span-group parameter block after initialization. */
void egui_view_spangroup_apply_params(egui_view_t *self, const egui_view_spangroup_params_t *params);
/** Initialize a span group and immediately apply its parameter block. */
void egui_view_spangroup_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_spangroup_params_t *params);
/** Initialize a small rich-text container backed by a fixed span array. */
void egui_view_spangroup_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SPANGROUP_H_ */
