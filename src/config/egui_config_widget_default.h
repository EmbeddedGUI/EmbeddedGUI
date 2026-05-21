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

/**
 * Maximum number of flex lines a single FlexLayout pass can produce.
 * Each line occupies a small stack struct. 16 is enough for most embedded UIs.
 */
#ifndef EGUI_CONFIG_FLEXLAYOUT_MAX_LINES
#define EGUI_CONFIG_FLEXLAYOUT_MAX_LINES 16
#endif

/**
 * Maximum number of egui_style_t pointers that can be stacked on one view.
 * Styles are stored in a fixed array inside egui_view_t (no heap).
 * Overhead per view = EGUI_CONFIG_STYLE_MAX_PER_VIEW * sizeof(pointer) + 1 byte.
 */
#ifndef EGUI_CONFIG_STYLE_MAX_PER_VIEW
#define EGUI_CONFIG_STYLE_MAX_PER_VIEW 4
#endif

/**
 * Maximum number of egui_observer_t pointers that one egui_subject_t can hold.
 * Observers are stored in a fixed array inside egui_subject_t (no heap).
 * Overhead per subject = EGUI_CONFIG_SUBJECT_MAX_OBSERVERS * sizeof(pointer) + 1 byte.
 */
#ifndef EGUI_CONFIG_SUBJECT_MAX_OBSERVERS
#define EGUI_CONFIG_SUBJECT_MAX_OBSERVERS 4
#endif

/**
 * egui_font_ttf_t glyph cache configuration.
 *
 * EGUI_CONFIG_FONT_TTF_GLYPH_CACHE_SLOTS: number of rasterised glyphs to
 * keep in the ring-eviction cache embedded in egui_font_ttf_t.
 * Lower values reduce RAM but increase rasterisation frequency.
 *
 * EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_W / _MAX_H: maximum glyph bitmap
 * dimensions in pixels.  Glyphs larger than this are cached without a
 * bitmap (metrics only) and will not be rendered.  Set to values that
 * comfortably exceed the largest glyph produced by the chosen pixel_height.
 * Rule of thumb: MAX_W ≈ pixel_height, MAX_H ≈ pixel_height * 1.5.
 *
 * RAM per egui_font_ttf_t instance (excluding stbtt_fontinfo ~256B):
 *   SLOTS * (20 + MAX_W * MAX_H) bytes
 * With defaults (8, 20, 32): 8 * (20 + 640) = ~5.3 KB.
 */
#ifndef EGUI_CONFIG_FONT_TTF_GLYPH_CACHE_SLOTS
#define EGUI_CONFIG_FONT_TTF_GLYPH_CACHE_SLOTS 8
#endif

#ifndef EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_W
#define EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_W 20
#endif

#ifndef EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_H
#define EGUI_CONFIG_FONT_TTF_GLYPH_BITMAP_MAX_H 32
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_WIDGET_DEFAULT_H_ */
