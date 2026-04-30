#ifndef _EGUI_KEY_EVENT_H_
#define _EGUI_KEY_EVENT_H_

/**
 * @file egui_key_event.h
 * @brief Keyboard-style input events and the fixed key-code mapping used by the core.
 */

#include "egui_common.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** High-level key-event phases queued by the input layer. */
enum egui_key_event_type
{
    EGUI_KEY_EVENT_NONE = 0,          // unused/default event slot
    EGUI_KEY_EVENT_ACTION_DOWN,       // key became pressed
    EGUI_KEY_EVENT_ACTION_UP,         // key was released
    EGUI_KEY_EVENT_ACTION_LONG_PRESS, // long-press state reported by the port or simulator
    EGUI_KEY_EVENT_ACTION_REPEAT,     // auto-repeat event while the key remains held
};

/** Stable logical key codes understood by widgets, focus traversal, and text input helpers. */
enum egui_key_code
{
    EGUI_KEY_CODE_NONE = 0,

    // Navigation keys
    EGUI_KEY_CODE_UP = 1,
    EGUI_KEY_CODE_DOWN = 2,
    EGUI_KEY_CODE_LEFT = 3,
    EGUI_KEY_CODE_RIGHT = 4,
    EGUI_KEY_CODE_TAB = 5,

    // Action keys
    EGUI_KEY_CODE_ENTER = 10,
    EGUI_KEY_CODE_ESCAPE = 11,
    EGUI_KEY_CODE_BACKSPACE = 12,
    EGUI_KEY_CODE_DELETE = 13,
    EGUI_KEY_CODE_SPACE = 14,
    EGUI_KEY_CODE_HOME = 15,
    EGUI_KEY_CODE_END = 16,

    // Digit keys 0-9
    EGUI_KEY_CODE_0 = 20,
    EGUI_KEY_CODE_1 = 21,
    EGUI_KEY_CODE_2 = 22,
    EGUI_KEY_CODE_3 = 23,
    EGUI_KEY_CODE_4 = 24,
    EGUI_KEY_CODE_5 = 25,
    EGUI_KEY_CODE_6 = 26,
    EGUI_KEY_CODE_7 = 27,
    EGUI_KEY_CODE_8 = 28,
    EGUI_KEY_CODE_9 = 29,

    // Letter keys A-Z
    EGUI_KEY_CODE_A = 30,
    EGUI_KEY_CODE_B = 31,
    EGUI_KEY_CODE_C = 32,
    EGUI_KEY_CODE_D = 33,
    EGUI_KEY_CODE_E = 34,
    EGUI_KEY_CODE_F = 35,
    EGUI_KEY_CODE_G = 36,
    EGUI_KEY_CODE_H = 37,
    EGUI_KEY_CODE_I = 38,
    EGUI_KEY_CODE_J = 39,
    EGUI_KEY_CODE_K = 40,
    EGUI_KEY_CODE_L = 41,
    EGUI_KEY_CODE_M = 42,
    EGUI_KEY_CODE_N = 43,
    EGUI_KEY_CODE_O = 44,
    EGUI_KEY_CODE_P = 45,
    EGUI_KEY_CODE_Q = 46,
    EGUI_KEY_CODE_R = 47,
    EGUI_KEY_CODE_S = 48,
    EGUI_KEY_CODE_T = 49,
    EGUI_KEY_CODE_U = 50,
    EGUI_KEY_CODE_V = 51,
    EGUI_KEY_CODE_W = 52,
    EGUI_KEY_CODE_X = 53,
    EGUI_KEY_CODE_Y = 54,
    EGUI_KEY_CODE_Z = 55,

    // Special characters
    EGUI_KEY_CODE_PERIOD = 60,
    EGUI_KEY_CODE_COMMA = 61,
    EGUI_KEY_CODE_MINUS = 62,
    EGUI_KEY_CODE_PLUS = 63,
    EGUI_KEY_CODE_SLASH = 64,
    EGUI_KEY_CODE_AT = 65,
    EGUI_KEY_CODE_UNDERSCORE = 66,
    EGUI_KEY_CODE_SEMICOLON = 67,
    EGUI_KEY_CODE_COLON = 68,
    EGUI_KEY_CODE_EXCLAMATION = 69,
    EGUI_KEY_CODE_QUESTION = 70,

    EGUI_KEY_CODE_MAX = 127, // highest code reserved by the current fixed mapping
};

/** One queued keyboard event stored in the per-core key FIFO. */
struct egui_key_event
{
    egui_snode_t node; // intrusive list node used by the core's queued key list

    uint8_t type;         // enum egui_key_event_type
    uint8_t key_code;     // enum egui_key_code
    uint8_t is_shift : 1; // non-zero when Shift was held for this event
    uint8_t is_ctrl : 1;  // non-zero when Ctrl was held for this event
    uint8_t reserved : 6; // reserved for future modifier bits
    uint32_t timestamp;   // event timestamp in milliseconds
};

/**
 * Convert one key event to printable ASCII when possible.
 * Digits, letters, space, and selected punctuation are supported; unsupported keys return `0`.
 */
char egui_key_event_to_char(const egui_key_event_t *event);

/** Return a short debug string such as `DOWN` or `REPEAT` for one key-event type. */
const char *egui_key_event_type_string(uint8_t type);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_KEY_EVENT_H_ */
