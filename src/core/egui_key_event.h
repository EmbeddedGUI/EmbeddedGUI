#ifndef _EGUI_KEY_EVENT_H_
#define _EGUI_KEY_EVENT_H_

#include "egui_common.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

enum egui_key_event_type
{
    EGUI_KEY_EVENT_NONE = 0,
    EGUI_KEY_EVENT_ACTION_DOWN,
    EGUI_KEY_EVENT_ACTION_UP,
    EGUI_KEY_EVENT_ACTION_LONG_PRESS,
    EGUI_KEY_EVENT_ACTION_REPEAT,
};

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

    EGUI_KEY_CODE_MAX = 127,
};

typedef struct egui_key_event egui_key_event_t;
struct egui_key_event
{
    egui_snode_t node;

    uint8_t type;     // enum egui_key_event_type
    uint8_t key_code; // enum egui_key_code
    uint8_t is_shift : 1;
    uint8_t is_ctrl : 1;
    uint8_t reserved : 6;
    uint32_t timestamp;
};

/**
 * Convert key event to printable ASCII character.
 * Returns 0 if the key is not printable.
 */
char egui_key_event_to_char(const egui_key_event_t *event);

const char *egui_key_event_type_string(uint8_t type);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_KEY_EVENT_H_ */
