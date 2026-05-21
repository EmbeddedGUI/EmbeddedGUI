#ifndef _EGUI_VIEW_TEXTINPUT_H_
#define _EGUI_VIEW_TEXTINPUT_H_

#include "egui_view.h"
#include "core/egui_timer.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired after the text buffer changes. */
typedef void (*egui_view_textinput_on_text_changed_t)(egui_view_t *self, const char *text);
/** Listener fired when Enter submits the current text. */
typedef void (*egui_view_textinput_on_submit_t)(egui_view_t *self, const char *text);

typedef struct egui_view_textinput egui_view_textinput_t;
/** Focusable single-line editor with an internal fixed-size text buffer. */
struct egui_view_textinput
{
    egui_view_t base;

    char text[EGUI_CONFIG_TEXTINPUT_MAX_LENGTH + 1]; /* Owned ASCII/byte buffer edited in place. */
    uint8_t text_len;                                /* Current text length in bytes. */
    uint8_t cursor_pos;                              /* Caret position expressed as a byte index. */
    uint8_t max_length;                              /* Runtime cap clamped by the build-time buffer size. */

    uint8_t cursor_visible : 1; /* Blink state of the caret while focused or keyboard-active. */
    uint8_t cursor_active : 1;  /* Keeps the caret active while an external editor, such as the on-screen keyboard, owns focus. */
    uint8_t password_mode : 1;  /* When non-zero, '*' is displayed in place of every actual character. */
    uint8_t reserved : 5;

    egui_timer_t cursor_timer; /* Timer used to toggle the caret visibility. */

    const egui_font_t *font;        /* Font used for both normal text and placeholder drawing. */
    egui_color_t text_color;        /* Main text color. */
    egui_alpha_t text_alpha;        /* Main text alpha. */
    egui_color_t placeholder_color; /* Placeholder text color. */
    egui_alpha_t placeholder_alpha; /* Placeholder text alpha. */
    egui_color_t cursor_color;      /* Caret color. */
    uint8_t align_type;             /* Alignment used when the text fits without scrolling. */

    const char *placeholder;    /* Borrowed placeholder string shown when the buffer is empty. */
    egui_dim_t scroll_offset_x; /* Horizontal shift applied when the caret moves past the viewport. */

    egui_view_textinput_on_text_changed_t on_text_changed; /* Optional callback fired after edits. */
    egui_view_textinput_on_submit_t on_submit;             /* Optional callback fired on Enter. */
};

/** Replace the current text, clamp it to `max_length`, move the cursor to the end, and fire `on_text_changed`. */
void egui_view_textinput_set_text(egui_view_t *self, const char *text);
/** Return the internal text buffer. The pointer stays valid until the next text mutation. */
const char *egui_view_textinput_get_text(egui_view_t *self);
/** Clear the text buffer. Equivalent to calling `set_text(self, NULL)`. */
void egui_view_textinput_clear(egui_view_t *self);
/** Insert one single-byte character at the current cursor position. */
void egui_view_textinput_insert_char(egui_view_t *self, char c);
/** Delete the character before the cursor, like Backspace. */
void egui_view_textinput_delete_char(egui_view_t *self);
/** Delete the character at the cursor, like Delete. */
void egui_view_textinput_delete_forward(egui_view_t *self);
/** Set the cursor by byte index. Values past the end are clamped. */
void egui_view_textinput_set_cursor_pos(egui_view_t *self, uint8_t pos);
/** Return the current cursor byte index. Returns 0 when self is NULL. */
uint8_t egui_view_textinput_get_cursor_pos(egui_view_t *self);
/** Move the cursor one character to the left when possible. */
void egui_view_textinput_move_cursor_left(egui_view_t *self);
/** Move the cursor one character to the right when possible. */
void egui_view_textinput_move_cursor_right(egui_view_t *self);
/** Move the cursor to the start of the text buffer. */
void egui_view_textinput_move_cursor_home(egui_view_t *self);
/** Move the cursor to the end of the text buffer. */
void egui_view_textinput_move_cursor_end(egui_view_t *self);
/** Override the font used for both text and placeholder rendering. */
void egui_view_textinput_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the configured font pointer, or NULL when none is set or self is NULL. */
const egui_font_t *egui_view_textinput_get_font(egui_view_t *self);
/** Set the main text color and alpha. */
void egui_view_textinput_set_text_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
/** Return the main text color. Returns zero when self is NULL. */
egui_color_t egui_view_textinput_get_text_color(egui_view_t *self);
/** Return the main text alpha. Returns 0 when self is NULL. */
egui_alpha_t egui_view_textinput_get_text_alpha(egui_view_t *self);
/** Set placeholder text shown when the buffer is empty and the widget is not focused. */
void egui_view_textinput_set_placeholder(egui_view_t *self, const char *placeholder);
/** Return the borrowed placeholder pointer, or NULL when none is set or self is NULL. */
const char *egui_view_textinput_get_placeholder(egui_view_t *self);
/** Set the placeholder text color and alpha. */
void egui_view_textinput_set_placeholder_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
/** Return the placeholder text color. Returns zero when self is NULL. */
egui_color_t egui_view_textinput_get_placeholder_color(egui_view_t *self);
/** Return the placeholder text alpha. Returns 0 when self is NULL. */
egui_alpha_t egui_view_textinput_get_placeholder_alpha(egui_view_t *self);
/** Set the blinking caret color. */
void egui_view_textinput_set_cursor_color(egui_view_t *self, egui_color_t color);
/** Return the blinking caret color. Returns zero when self is NULL. */
egui_color_t egui_view_textinput_get_cursor_color(egui_view_t *self);
/** Keep the caret active while another focus owner edits this input. */
void egui_view_textinput_set_cursor_active(egui_view_t *self, int is_active);
/** Return non-zero when the external caret-active flag is set. Returns 0 when self is NULL. */
int egui_view_textinput_get_cursor_active(egui_view_t *self);
/** Limit the editable length. Values above the build-time cap are clamped, and existing text may be truncated. */
void egui_view_textinput_set_max_length(egui_view_t *self, uint8_t max_length);
/** Return the current runtime max-length cap. Returns 0 when self is NULL. */
uint8_t egui_view_textinput_get_max_length(egui_view_t *self);
/** Register the callback fired after text-buffer mutations. */
void egui_view_textinput_set_on_text_changed(egui_view_t *self, egui_view_textinput_on_text_changed_t listener);
/** Return the registered text-change callback, or NULL when unset or self is NULL. */
egui_view_textinput_on_text_changed_t egui_view_textinput_get_on_text_changed(egui_view_t *self);
/** Register the callback fired when Enter is submitted from the focused input. */
void egui_view_textinput_set_on_submit(egui_view_t *self, egui_view_textinput_on_submit_t listener);
/** Return the registered submit callback, or NULL when unset or self is NULL. */
egui_view_textinput_on_submit_t egui_view_textinput_get_on_submit(egui_view_t *self);
/** Enable or disable password mode: displayed characters are replaced by '*'. The text buffer is unaffected. */
void egui_view_textinput_set_password_mode(egui_view_t *self, int enabled);
/** Return non-zero when password mode is active. */
int egui_view_textinput_get_password_mode(egui_view_t *self);
/** Default draw hook used by the text-input API table. */
void egui_view_textinput_on_draw(egui_view_t *self);
/** Initialize the focusable text-input widget. */
void egui_view_textinput_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TEXTINPUT_H_ */
