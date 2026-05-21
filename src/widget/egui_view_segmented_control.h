#ifndef _EGUI_VIEW_SEGMENTED_CONTROL_H_
#define _EGUI_VIEW_SEGMENTED_CONTROL_H_

#include "egui_view.h"
#include "font/egui_font.h"

/**
 * @brief Compact single-choice control that splits one track into equal segments.
 *
 * The widget borrows label and icon arrays from the caller and keeps exactly
 * one active index. It supports touch and keyboard navigation while reusing one
 * shared background track instead of drawing fully independent buttons.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Fixed storage capacity for borrowed segment labels and icons. */
#define EGUI_VIEW_SEGMENTED_CONTROL_MAX_SEGMENTS 8
/* Sentinel stored in `pressed_index` when no pointer currently owns a segment. */
#define EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE 0xFF

/** Listener fired when the active segment changes. */
typedef void (*egui_view_on_segment_changed_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_segmented_control egui_view_segmented_control_t;
struct egui_view_segmented_control
{
    egui_view_t base;

    /* Borrowed label array for each segment. */
    const char **segment_texts;
    /* Optional borrowed icon array parallel to `segment_texts`. */
    const char **segment_icons;
    /* Number of active segments currently borrowed from the arrays. */
    uint8_t segment_count;
    /* Persistently selected segment index. */
    uint8_t current_index;
    /* Segment index captured by the current pointer gesture. */
    uint8_t pressed_index;
    /* Horizontal inset between the outer frame and segment track. */
    uint8_t horizontal_padding;
    /* Gap inserted between neighboring segments. */
    uint8_t segment_gap;
    /* Rounded corner radius shared by the outer frame and active segment. */
    uint8_t corner_radius;
    /* Shared alpha used for background, border, and content drawing. */
    egui_alpha_t alpha;
    /* Background color of the outer segmented-control track. */
    egui_color_t bg_color;
    /* Background color of the currently active segment. */
    egui_color_t selected_bg_color;
    /* Text and icon tint color for inactive segments. */
    egui_color_t text_color;
    /* Text and icon tint color for the active segment. */
    egui_color_t selected_text_color;
    /* Border color of the outer frame and optional separators. */
    egui_color_t border_color;
    /* Font used for segment labels. */
    const egui_font_t *font;
    /* Optional icon font override; auto-selected when NULL. */
    const egui_font_t *icon_font;
    /* Vertical spacing between icon and text in mixed segments. */
    egui_dim_t icon_text_gap;
    /* Callback fired after the active segment actually changes. */
    egui_view_on_segment_changed_listener_t on_segment_changed;
};

typedef struct egui_view_segmented_control_params egui_view_segmented_control_params_t;
/**
 * @brief Construction-time parameter block for one segmented control.
 */
struct egui_view_segmented_control_params
{
    /* Outer region occupied by the segmented control. */
    egui_region_t region;
    /* Borrowed label array for the initial segments. */
    const char **segment_texts;
    /* Optional borrowed icon array parallel to `segment_texts`. */
    const char **segment_icons;
    /* Number of valid entries in the borrowed arrays. */
    uint8_t segment_count;
};

/** Build a segmented-control parameter block with region and borrowed labels. */
#define EGUI_VIEW_SEGMENTED_CONTROL_PARAMS_INIT(_name, _x, _y, _w, _h, _texts, _count)                                                                         \
    static const egui_view_segmented_control_params_t _name = {                                                                                                \
            .region = {{(_x), (_y)}, {(_w), (_h)}}, .segment_texts = (_texts), .segment_icons = NULL, .segment_count = (_count)}

/** Apply a segmented-control parameter block after initialization. */
void egui_view_segmented_control_apply_params(egui_view_t *self, const egui_view_segmented_control_params_t *params);
/** Initialize a segmented control and immediately apply its parameter block. */
void egui_view_segmented_control_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_segmented_control_params_t *params);

/** Replace the segment text array. The segment count is clamped to the fixed maximum. */
void egui_view_segmented_control_set_segments(egui_view_t *self, const char **segment_texts, uint8_t segment_count);
/** Return the borrowed segment text array. */
const char **egui_view_segmented_control_get_segments(egui_view_t *self);
/** Return the active segment count after clamping to the fixed maximum. */
uint8_t egui_view_segmented_control_get_segment_count(egui_view_t *self);
/** Set the optional icon array that parallels the segment text array. */
void egui_view_segmented_control_set_segment_icons(egui_view_t *self, const char **segment_icons);
/** Return the optional borrowed icon array. */
const char **egui_view_segmented_control_get_segment_icons(egui_view_t *self);
/** Select the active segment and fire the change listener on real changes. */
void egui_view_segmented_control_set_current_index(egui_view_t *self, uint8_t index);
/** Return the current active segment index. */
uint8_t egui_view_segmented_control_get_current_index(egui_view_t *self);
/** Return the pointer-captured segment index, or `EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE`. */
uint8_t egui_view_segmented_control_get_pressed_index(egui_view_t *self);
/** Register the callback fired when the active segment changes. */
void egui_view_segmented_control_set_on_segment_changed_listener(egui_view_t *self, egui_view_on_segment_changed_listener_t listener);
/** Return the registered segment change callback. */
egui_view_on_segment_changed_listener_t egui_view_segmented_control_get_on_segment_changed_listener(egui_view_t *self);

/** Set the background color of the outer segmented-control track. */
void egui_view_segmented_control_set_bg_color(egui_view_t *self, egui_color_t color);
/** Return the background color of the outer segmented-control track. */
egui_color_t egui_view_segmented_control_get_bg_color(egui_view_t *self);
/** Set the background color of the selected segment. */
void egui_view_segmented_control_set_selected_bg_color(egui_view_t *self, egui_color_t color);
/** Return the background color of the selected segment. */
egui_color_t egui_view_segmented_control_get_selected_bg_color(egui_view_t *self);
/** Set the text and icon tint color used by unselected segments. */
void egui_view_segmented_control_set_text_color(egui_view_t *self, egui_color_t color);
/** Return the text and icon tint color used by unselected segments. */
egui_color_t egui_view_segmented_control_get_text_color(egui_view_t *self);
/** Set the text and icon tint color used by the selected segment. */
void egui_view_segmented_control_set_selected_text_color(egui_view_t *self, egui_color_t color);
/** Return the text and icon tint color used by the selected segment. */
egui_color_t egui_view_segmented_control_get_selected_text_color(egui_view_t *self);
/** Set the border color of the outer segmented-control frame. */
void egui_view_segmented_control_set_border_color(egui_view_t *self, egui_color_t color);
/** Return the border color of the outer segmented-control frame. */
egui_color_t egui_view_segmented_control_get_border_color(egui_view_t *self);
/** Set the corner radius of the outer frame and selected segment. */
void egui_view_segmented_control_set_corner_radius(egui_view_t *self, uint8_t radius);
/** Return the corner radius of the outer frame and selected segment. */
uint8_t egui_view_segmented_control_get_corner_radius(egui_view_t *self);
/** Set the gap inserted between segments. */
void egui_view_segmented_control_set_segment_gap(egui_view_t *self, uint8_t gap);
/** Return the gap inserted between segments. */
uint8_t egui_view_segmented_control_get_segment_gap(egui_view_t *self);
/** Set the horizontal padding between the outer frame and segment content area. */
void egui_view_segmented_control_set_horizontal_padding(egui_view_t *self, uint8_t padding);
/** Return the horizontal padding between the outer frame and segment content area. */
uint8_t egui_view_segmented_control_get_horizontal_padding(egui_view_t *self);
/** Override the font used for segment labels. */
void egui_view_segmented_control_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the font used for segment labels. */
const egui_font_t *egui_view_segmented_control_get_font(egui_view_t *self);
/** Override the icon font used when segments show icons. */
void egui_view_segmented_control_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the explicit icon font override, or NULL when auto-selection is used. */
const egui_font_t *egui_view_segmented_control_get_icon_font(egui_view_t *self);
/** Set the vertical gap between icon and text in mixed icon-text segments. */
void egui_view_segmented_control_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
/** Return the vertical gap between icon and text in mixed icon-text segments. */
egui_dim_t egui_view_segmented_control_get_icon_text_gap(egui_view_t *self);

/** Default draw hook used by the segmented-control API table. */
void egui_view_segmented_control_on_draw(egui_view_t *self);
/** Initialize the focusable segmented-control widget. */
void egui_view_segmented_control_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SEGMENTED_CONTROL_H_ */
