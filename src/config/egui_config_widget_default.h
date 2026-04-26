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

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_WIDGET_DEFAULT_H_ */
