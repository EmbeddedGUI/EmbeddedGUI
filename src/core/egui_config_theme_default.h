#ifndef _EGUI_CONFIG_THEME_DEFAULT_H_
#define _EGUI_CONFIG_THEME_DEFAULT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Primary & semantic colors ---- */

#ifndef EGUI_THEME_PRIMARY
#define EGUI_THEME_PRIMARY EGUI_COLOR_MAKE(0x25, 0x63, 0xEB) /* Blue 600 */
#endif

#ifndef EGUI_THEME_PRIMARY_DARK
#define EGUI_THEME_PRIMARY_DARK EGUI_COLOR_MAKE(0x1D, 0x4E, 0xD8) /* Blue 700 (Pressed) */
#endif

#ifndef EGUI_THEME_SECONDARY
#define EGUI_THEME_SECONDARY EGUI_COLOR_MAKE(0x14, 0xB8, 0xA6) /* Teal 500 */
#endif

#ifndef EGUI_THEME_SUCCESS
#define EGUI_THEME_SUCCESS EGUI_COLOR_MAKE(0x16, 0xA3, 0x4A) /* Green 600 */
#endif

#ifndef EGUI_THEME_WARNING
#define EGUI_THEME_WARNING EGUI_COLOR_MAKE(0xF5, 0x9E, 0x0B) /* Amber 500 */
#endif

#ifndef EGUI_THEME_DANGER
#define EGUI_THEME_DANGER EGUI_COLOR_MAKE(0xDC, 0x26, 0x26) /* Red 600 */
#endif

/* ---- Track & state colors ---- */

#ifndef EGUI_THEME_TRACK_BG
#define EGUI_THEME_TRACK_BG EGUI_COLOR_MAKE(0xE2, 0xE8, 0xF0) /* Slate 200 */
#endif

#ifndef EGUI_THEME_TRACK_OFF
#define EGUI_THEME_TRACK_OFF EGUI_COLOR_MAKE(0xCB, 0xD5, 0xE1) /* Slate 300 */
#endif

#ifndef EGUI_THEME_DISABLED
#define EGUI_THEME_DISABLED EGUI_COLOR_MAKE(0xD1, 0xD5, 0xDB) /* Gray 300 */
#endif

#ifndef EGUI_THEME_THUMB
#define EGUI_THEME_THUMB EGUI_COLOR_WHITE /* Slider/Switch knobs */
#endif

/* ---- Text colors ---- */

#ifndef EGUI_THEME_TEXT
#define EGUI_THEME_TEXT EGUI_COLOR_WHITE /* White text on primary */
#endif

#ifndef EGUI_THEME_TEXT_PRIMARY
#define EGUI_THEME_TEXT_PRIMARY EGUI_COLOR_MAKE(0x11, 0x18, 0x27) /* Gray 900 */
#endif

#ifndef EGUI_THEME_TEXT_SECONDARY
#define EGUI_THEME_TEXT_SECONDARY EGUI_COLOR_MAKE(0x6B, 0x72, 0x80) /* Gray 500 */
#endif

/* ---- Surface & border ---- */

#ifndef EGUI_THEME_SURFACE
#define EGUI_THEME_SURFACE EGUI_COLOR_MAKE(0xFF, 0xFF, 0xFF) /* White surface */
#endif

#ifndef EGUI_THEME_SURFACE_VARIANT
#define EGUI_THEME_SURFACE_VARIANT EGUI_COLOR_MAKE(0xF8, 0xFA, 0xFC) /* Slate 50 */
#endif

#ifndef EGUI_THEME_BORDER
#define EGUI_THEME_BORDER EGUI_COLOR_MAKE(0xE2, 0xE8, 0xF0) /* Slate 200 */
#endif

#ifndef EGUI_THEME_FOCUS
#define EGUI_THEME_FOCUS EGUI_THEME_PRIMARY /* Focus ring color */
#endif

/* ---- Press overlay ---- */

#ifndef EGUI_THEME_PRESS_OVERLAY
#define EGUI_THEME_PRESS_OVERLAY EGUI_COLOR_BLACK /* Press overlay color */
#endif

#ifndef EGUI_THEME_PRESS_OVERLAY_ALPHA
#define EGUI_THEME_PRESS_OVERLAY_ALPHA EGUI_ALPHA_10 /* Press overlay alpha */
#endif

/* ---- Radius & stroke ---- */

#ifndef EGUI_THEME_RADIUS_SM
#define EGUI_THEME_RADIUS_SM 6
#endif

#ifndef EGUI_THEME_RADIUS_MD
#define EGUI_THEME_RADIUS_MD 10
#endif

#ifndef EGUI_THEME_RADIUS_LG
#define EGUI_THEME_RADIUS_LG 14
#endif

#ifndef EGUI_THEME_STROKE_WIDTH
#define EGUI_THEME_STROKE_WIDTH 1
#endif

#ifndef EGUI_THEME_TRACK_THICKNESS
#define EGUI_THEME_TRACK_THICKNESS 6
#endif

/* ---- Scrollbar theme ---- */

#ifndef EGUI_THEME_SCROLLBAR_COLOR
#define EGUI_THEME_SCROLLBAR_COLOR EGUI_COLOR_MAKE(0x94, 0xA3, 0xB8) /* Slate 400 */
#endif

#ifndef EGUI_THEME_SCROLLBAR_ALPHA
#define EGUI_THEME_SCROLLBAR_ALPHA EGUI_ALPHA_50
#endif

#ifndef EGUI_THEME_SCROLLBAR_THICKNESS
#define EGUI_THEME_SCROLLBAR_THICKNESS 4
#endif

#ifndef EGUI_THEME_SCROLLBAR_MIN_LENGTH
#define EGUI_THEME_SCROLLBAR_MIN_LENGTH 8
#endif

#ifndef EGUI_THEME_SCROLLBAR_MARGIN
#define EGUI_THEME_SCROLLBAR_MARGIN 2
#endif

#ifndef EGUI_THEME_SCROLLBAR_RADIUS
#define EGUI_THEME_SCROLLBAR_RADIUS 2
#endif

#ifndef EGUI_THEME_SCROLLBAR_TOUCH_WIDTH
#define EGUI_THEME_SCROLLBAR_TOUCH_WIDTH 15
#endif

/* ---- Enhanced draw theme ---- */

#ifndef EGUI_THEME_PRIMARY_LIGHT
#define EGUI_THEME_PRIMARY_LIGHT EGUI_COLOR_MAKE(0x3B, 0x82, 0xF6) /* Blue 500 */
#endif

#ifndef EGUI_THEME_TRACK_BG_DARK
#define EGUI_THEME_TRACK_BG_DARK EGUI_COLOR_MAKE(0xCB, 0xD5, 0xE1) /* Slate 300 */
#endif

/* ---- Enhanced draw shadow ---- */

#ifndef EGUI_THEME_SHADOW_WIDTH_SM
#define EGUI_THEME_SHADOW_WIDTH_SM 3
#endif

#ifndef EGUI_THEME_SHADOW_WIDTH_MD
#define EGUI_THEME_SHADOW_WIDTH_MD 4
#endif

#ifndef EGUI_THEME_SHADOW_WIDTH_LG
#define EGUI_THEME_SHADOW_WIDTH_LG 6
#endif

#ifndef EGUI_THEME_SHADOW_OFS_Y_SM
#define EGUI_THEME_SHADOW_OFS_Y_SM 1
#endif

#ifndef EGUI_THEME_SHADOW_OFS_Y_MD
#define EGUI_THEME_SHADOW_OFS_Y_MD 2
#endif

#ifndef EGUI_THEME_SHADOW_OFS_Y_LG
#define EGUI_THEME_SHADOW_OFS_Y_LG 3
#endif

#ifndef EGUI_THEME_SHADOW_OPA
#define EGUI_THEME_SHADOW_OPA 80
#endif

#ifndef EGUI_THEME_ENHANCED_BORDER_WIDTH
#define EGUI_THEME_ENHANCED_BORDER_WIDTH 1
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_THEME_DEFAULT_H_ */
