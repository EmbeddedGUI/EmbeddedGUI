#include "egui_key_event.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY

static const char *key_event_type_strings[] = {
        "NONE", "DOWN", "UP", "LONG_PRESS", "REPEAT",
};

const char *egui_key_event_type_string(uint8_t type)
{
    if (type < EGUI_ARRAY_SIZE(key_event_type_strings))
    {
        return key_event_type_strings[type];
    }
    return "UNKNOWN";
}

char egui_key_event_to_char(const egui_key_event_t *event)
{
    uint8_t code = event->key_code;

    // Space
    if (code == EGUI_KEY_CODE_SPACE)
    {
        return ' ';
    }

    // Digits 0-9
    if (code >= EGUI_KEY_CODE_0 && code <= EGUI_KEY_CODE_9)
    {
        return '0' + (code - EGUI_KEY_CODE_0);
    }

    // Letters A-Z
    if (code >= EGUI_KEY_CODE_A && code <= EGUI_KEY_CODE_Z)
    {
        if (event->is_shift)
        {
            return 'A' + (code - EGUI_KEY_CODE_A);
        }
        return 'a' + (code - EGUI_KEY_CODE_A);
    }

    // Special characters
    switch (code)
    {
    case EGUI_KEY_CODE_PERIOD:
        return event->is_shift ? '>' : '.';
    case EGUI_KEY_CODE_COMMA:
        return event->is_shift ? '<' : ',';
    case EGUI_KEY_CODE_MINUS:
        return event->is_shift ? '_' : '-';
    case EGUI_KEY_CODE_PLUS:
        return '+';
    case EGUI_KEY_CODE_SLASH:
        return event->is_shift ? '?' : '/';
    case EGUI_KEY_CODE_AT:
        return '@';
    case EGUI_KEY_CODE_UNDERSCORE:
        return '_';
    case EGUI_KEY_CODE_SEMICOLON:
        return event->is_shift ? ':' : ';';
    case EGUI_KEY_CODE_COLON:
        return ':';
    case EGUI_KEY_CODE_EXCLAMATION:
        return '!';
    case EGUI_KEY_CODE_QUESTION:
        return '?';
    default:
        break;
    }

    return 0; // not printable
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY
