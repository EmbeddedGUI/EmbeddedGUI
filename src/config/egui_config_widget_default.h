#ifndef _EGUI_CONFIG_WIDGET_DEFAULT_H_
#define _EGUI_CONFIG_WIDGET_DEFAULT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Text input widget ---- */

/**
 * Widget icon-font auto fallback.
 * When disabled, widgets only draw icon glyphs after the user explicitly sets
 * an icon font via the corresponding
 * `*_set_icon_font()` API.
 */
#ifndef EGUI_CONFIG_WIDGET_AUTO_ICON_FONT_FALLBACK
#define EGUI_CONFIG_WIDGET_AUTO_ICON_FONT_FALLBACK 0
#endif

/**
 * Text input fixed buffer capacity in bytes.
 * Runtime cursor/length bookkeeping uses uint8_t, so valid range is 1..255.
 */
#ifndef EGUI_CONFIG_TEXTINPUT_MAX_LENGTH
#define EGUI_CONFIG_TEXTINPUT_MAX_LENGTH 32
#endif

/**
 * Button optional icon support.
 * When disabled, text buttons skip icon-only and icon+text mixed layout code.
 * Disable only for apps that use buttons as
 * plain text buttons.
 */
#ifndef EGUI_CONFIG_FUNCTION_VIEW_BUTTON_ICON
#define EGUI_CONFIG_FUNCTION_VIEW_BUTTON_ICON 1
#endif

/**
 * Cursor blink interval in milliseconds.
 */
#ifndef EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS
#define EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS 500
#endif

/* ---- Textblock widget ---- */

/**
 * Textblock edit buffer capacity in bytes.
 * Larger values increase both per-instance RAM and temporary stack usage.
 */
#ifndef EGUI_CONFIG_TEXTBLOCK_EDIT_MAX_LENGTH
#define EGUI_CONFIG_TEXTBLOCK_EDIT_MAX_LENGTH 256
#endif

/**
 * Textblock cursor blink interval in milliseconds.
 */
#ifndef EGUI_CONFIG_TEXTBLOCK_CURSOR_BLINK_MS
#define EGUI_CONFIG_TEXTBLOCK_CURSOR_BLINK_MS 500
#endif

/* ---- View / style features ---- */

/**
 * FlexLayout container support.
 * When enabled, egui_view_t gains a uint8_t flex_grow field.
 */
#ifndef EGUI_CONFIG_FUNCTION_FLEXLAYOUT
#define EGUI_CONFIG_FUNCTION_FLEXLAYOUT 0
#endif

/**
 * Maximum number of flex lines a single FlexLayout pass can produce.
 */
#ifndef EGUI_CONFIG_FLEXLAYOUT_MAX_LINES
#define EGUI_CONFIG_FLEXLAYOUT_MAX_LINES 16
#endif

/**
 * Style Cascade support.
 * When enabled, each view gains a fixed-size shared-style stack.
 */
#ifndef EGUI_CONFIG_FUNCTION_STYLE_CASCADE
#define EGUI_CONFIG_FUNCTION_STYLE_CASCADE 0
#endif

/**
 * Maximum number of egui_style_t pointers that can be stacked on one view.
 */
#ifndef EGUI_CONFIG_STYLE_MAX_PER_VIEW
#define EGUI_CONFIG_STYLE_MAX_PER_VIEW 4
#endif

/**
 * View state styles. Requires EGUI_CONFIG_FUNCTION_STYLE_CASCADE=1.
 */
#ifndef EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
#define EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES 0
#endif

/**
 * Optional per-view user_data pointer.
 */
#ifndef EGUI_CONFIG_FUNCTION_VIEW_USER_DATA
#define EGUI_CONFIG_FUNCTION_VIEW_USER_DATA 0
#endif

/**
 * Generic property access helpers for tooling and small dynamic UIs.
 */
#ifndef EGUI_CONFIG_FUNCTION_PROPERTY_LITE
#define EGUI_CONFIG_FUNCTION_PROPERTY_LITE 0
#endif

/* ---- Touch interaction helpers ---- */

/**
 * Extended click area support.
 * When enabled, each view can expand its touch hit region beyond visual bounds.
 */
#ifndef EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA
#define EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA 0
#endif

/**
 * Long-press listener for touch views.
 */
#ifndef EGUI_CONFIG_FUNCTION_LONG_PRESS
#define EGUI_CONFIG_FUNCTION_LONG_PRESS 0
#endif

/**
 * Hold duration in milliseconds before a long-press callback fires.
 */
#ifndef EGUI_CONFIG_LONG_PRESS_DURATION_MS
#define EGUI_CONFIG_LONG_PRESS_DURATION_MS 500
#endif

/**
 * Swipe direction listener for touch views.
 */
#ifndef EGUI_CONFIG_FUNCTION_SWIPE_LISTENER
#define EGUI_CONFIG_FUNCTION_SWIPE_LISTENER 0
#endif

/**
 * Minimum finger travel distance in pixels required for a swipe event.
 */
#ifndef EGUI_CONFIG_SWIPE_MIN_DISPLACEMENT_PX
#define EGUI_CONFIG_SWIPE_MIN_DISPLACEMENT_PX 20
#endif

/**
 * Draggable View support.
 */
#ifndef EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW
#define EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW 0
#endif

/* ---- Label widget ---- */

/**
 * Label long mode support for text overflow behavior.
 */
#ifndef EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
#define EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE 0
#endif

#ifndef EGUI_CONFIG_LABEL_LONG_DOTS_BUF_SIZE
#define EGUI_CONFIG_LABEL_LONG_DOTS_BUF_SIZE 128
#endif

/**
 * Label word wrap support. Requires EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE=1.
 */
#ifndef EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP
#define EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP 0
#endif

/**
 * Label inline recolor support for #RRGGBB text# tags.
 */
#ifndef EGUI_CONFIG_FUNCTION_LABEL_RECOLOR
#define EGUI_CONFIG_FUNCTION_LABEL_RECOLOR 0
#endif

/**
 * Label letter spacing support.
 */
#ifndef EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE
#define EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE 0
#endif

/**
 * Label printf-style formatting support.
 */
#ifndef EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT
#define EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT 0
#endif

#ifndef EGUI_CONFIG_LABEL_FMT_BUF_SIZE
#define EGUI_CONFIG_LABEL_FMT_BUF_SIZE 64
#endif

/* ---- Scroll widget ---- */

/**
 * Horizontal scroll mode support.
 */
#ifndef EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
#define EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL 0
#endif

/**
 * Scroll snap support.
 */
#ifndef EGUI_CONFIG_FUNCTION_SCROLL_SNAP
#define EGUI_CONFIG_FUNCTION_SCROLL_SNAP 0
#endif

/**
 * Duration in milliseconds of the snap-to-position animation.
 */
#ifndef EGUI_CONFIG_SCROLL_SNAP_DURATION_MS
#define EGUI_CONFIG_SCROLL_SNAP_DURATION_MS 200
#endif

/**
 * Scroll event listener support.
 */
#ifndef EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
#define EGUI_CONFIG_FUNCTION_SCROLL_LISTENER 0
#endif

/* ---- Image widget ---- */

/**
 * Image widget lightweight axis-aligned scale.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE
#define EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE 0
#endif

/**
 * Image widget rotation and affine scale transform.
 */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
#define EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM 0
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_WIDGET_DEFAULT_H_ */
