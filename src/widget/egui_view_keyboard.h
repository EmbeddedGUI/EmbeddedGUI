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
#define EGUI_KEYBOARD_DEFAULT_WIDTH  EGUI_CONFIG_SCEEN_WIDTH
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
struct egui_view_keyboard
{
    egui_view_group_t base;

    // Row layout containers
    egui_view_linearlayout_t rows[EGUI_KEYBOARD_ROW_COUNT];

    // All key buttons
    egui_view_button_t keys[EGUI_KEYBOARD_TOTAL_KEYS];

    // Current mode: 0=lowercase, 1=uppercase, 2=symbols
    uint8_t mode;

    // Target text input view (NULL when keyboard is not active)
    egui_view_t *target;

    // Font for key labels
    const egui_font_t *font;

    // Keyboard avoidance: saved state for restoring position when keyboard hides
    egui_view_t *adjusted_view; // view whose Y position was adjusted (NULL if none)
    egui_dim_t saved_y;         // original Y position before adjustment
};

void egui_view_keyboard_init(egui_view_t *self);
void egui_view_keyboard_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_keyboard_show(egui_view_t *self, egui_view_t *target_textinput);
void egui_view_keyboard_hide(egui_view_t *self);
void egui_view_keyboard_set_mode(egui_view_t *self, uint8_t mode);

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_KEYBOARD_H_ */
