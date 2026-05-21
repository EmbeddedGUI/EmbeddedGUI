#ifndef _EGUI_VIEW_KEYBOARD_H_
#define _EGUI_VIEW_KEYBOARD_H_

#include "egui_view_group.h"
#include "egui_view_linearlayout.h"
#include "egui_view_button.h"
#include "egui_view_textinput.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

// Keyboard mode constants
#define EGUI_KEYBOARD_MODE_LOWERCASE 0
#define EGUI_KEYBOARD_MODE_UPPERCASE 1
#define EGUI_KEYBOARD_MODE_SYMBOLS   2

// Keyboard layout constants
#define EGUI_KEYBOARD_ROW_COUNT      4
#define EGUI_KEYBOARD_ROW0_KEY_COUNT 10
#define EGUI_KEYBOARD_ROW1_KEY_COUNT 9
#define EGUI_KEYBOARD_ROW2_KEY_COUNT 9
#define EGUI_KEYBOARD_ROW3_KEY_COUNT 3
#define EGUI_KEYBOARD_TOTAL_KEYS     (EGUI_KEYBOARD_ROW0_KEY_COUNT + EGUI_KEYBOARD_ROW1_KEY_COUNT + EGUI_KEYBOARD_ROW2_KEY_COUNT + EGUI_KEYBOARD_ROW3_KEY_COUNT)

// Default dimensions (for 240px wide screen)
#define EGUI_KEYBOARD_DEFAULT_WIDTH  EGUI_CONFIG_SCREEN_WIDTH
#define EGUI_KEYBOARD_DEFAULT_HEIGHT 128
#define EGUI_KEYBOARD_KEY_HEIGHT     28

// Key width definitions
#define EGUI_KEYBOARD_KEY_WIDTH_NORMAL 22
#define EGUI_KEYBOARD_KEY_WIDTH_SMALL  22
#define EGUI_KEYBOARD_KEY_WIDTH_SHIFT  33
#define EGUI_KEYBOARD_KEY_WIDTH_BACK   33
#define EGUI_KEYBOARD_KEY_WIDTH_MODE   50
#define EGUI_KEYBOARD_KEY_WIDTH_SPACE  120
#define EGUI_KEYBOARD_KEY_WIDTH_ENTER  50

// Special key indices
#define EGUI_KEYBOARD_KEY_IDX_SHIFT     19
#define EGUI_KEYBOARD_KEY_IDX_BACKSPACE 27
#define EGUI_KEYBOARD_KEY_IDX_MODE      28
#define EGUI_KEYBOARD_KEY_IDX_SPACE     29
#define EGUI_KEYBOARD_KEY_IDX_ENTER     30

typedef struct egui_view_keyboard egui_view_keyboard_t;
/** On-screen keyboard composed of four row layouts plus one button per logical key. */
struct egui_view_keyboard
{
    egui_view_group_t base;
    egui_view_api_t api;     /* Instance API copy used to override keyboard key handling. */
    egui_view_api_t key_api; /* Shared key-button API copy used for directional focus navigation. */

    egui_view_linearlayout_t rows[EGUI_KEYBOARD_ROW_COUNT]; /* One horizontal layout per keyboard row. */

    egui_view_button_t keys[EGUI_KEYBOARD_TOTAL_KEYS]; /* Flat button array indexed by the shared key tables. */

    uint8_t mode; /* Current label/output mode: lowercase, uppercase, or symbols. */

    egui_view_t *target;               /* Active textinput receiving characters, or NULL while hidden. */
    egui_view_t *suppress_show_target; /* Target whose immediate restore-focus show request should be ignored once. */

    const egui_font_t *font;      /* Font used by normal character labels. */
    const egui_font_t *icon_font; /* Optional icon font for Shift/Backspace/Enter glyphs. */
    const char *shift_icon;       /* Borrowed icon string for the Shift key. */
    const char *backspace_icon;   /* Borrowed icon string for the Backspace key. */
    const char *enter_icon;       /* Borrowed icon string for the Enter key. */

    egui_view_t *adjusted_view; /* Root view moved upward to avoid keyboard overlap, if any. */
    egui_dim_t saved_y;         /* Original Y position restored when the keyboard hides. */
};

/** Initialize the on-screen keyboard. It starts hidden and must be shown for a target text input. */
void egui_view_keyboard_init(egui_view_t *self, egui_core_t *core);
/** Set the font used by normal character keys and refresh all key labels. */
void egui_view_keyboard_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the font used by normal character keys. */
const egui_font_t *egui_view_keyboard_get_font(egui_view_t *self);
/** Set the font used by special-key icons and refresh icon-based keys. */
void egui_view_keyboard_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the explicit special-key icon font override, or NULL when automatic resolution is used. */
const egui_font_t *egui_view_keyboard_get_icon_font(egui_view_t *self);
/** Override the Shift, Backspace, and Enter glyph strings, then refresh labels for the current mode. */
void egui_view_keyboard_set_special_key_icons(egui_view_t *self, const char *shift_icon, const char *backspace_icon, const char *enter_icon);
/** Return the effective Shift glyph string. */
const char *egui_view_keyboard_get_shift_icon(egui_view_t *self);
/** Return the effective Backspace glyph string. */
const char *egui_view_keyboard_get_backspace_icon(egui_view_t *self);
/** Return the effective Enter glyph string. */
const char *egui_view_keyboard_get_enter_icon(egui_view_t *self);
/** Show the keyboard for one target text input and move the target's root view upward if the keyboard would cover it. */
void egui_view_keyboard_show(egui_view_t *self, egui_view_t *target_textinput);
/** Hide the keyboard, clear the target, and restore any root-view position adjusted by `show()`. */
void egui_view_keyboard_hide(egui_view_t *self);
/** Switch between lowercase, uppercase, and symbol layouts. Invalid modes fall back to lowercase. */
void egui_view_keyboard_set_mode(egui_view_t *self, uint8_t mode);
/** Return the current keyboard mode. */
uint8_t egui_view_keyboard_get_mode(egui_view_t *self);
/** Return the active textinput receiving keyboard input, or NULL while hidden. */
egui_view_t *egui_view_keyboard_get_target(egui_view_t *self);

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_KEYBOARD_H_ */
