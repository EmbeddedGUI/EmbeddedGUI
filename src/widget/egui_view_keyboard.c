#include <stdio.h>
#include <string.h>

#include "egui_view_keyboard.h"
#include "egui_view_icon_font.h"
#include "font/egui_font.h"
#include "core/egui_api.h"
#include "core/egui_core.h"
#include "background/egui_background_color.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

/*
 * The keyboard widget is built from a flat key index space shared by:
 * 1. visible label tables,
 * 2. emitted character tables,
 * 3. the button array
 * created at init time.
 * This keeps mode switching cheap because only labels/fonts need to be refreshed.
 */

// ============== Key label tables for each mode ==============

// Index order: Row0[0..9], Row1[10..18], Row2[19..27], Row3[28..30]
// Row2: [19]=shift, [20..26]=letters, [27]=backspace
// Row3: [28]=mode, [29]=space, [30]=enter

static const char *keyboard_labels_lowercase[EGUI_KEYBOARD_TOTAL_KEYS] = {
        "q",
        "w",
        "e",
        "r",
        "t",
        "y",
        "u",
        "i",
        "o",
        "p",
        "a",
        "s",
        "d",
        "f",
        "g",
        "h",
        "j",
        "k",
        "l",
        EGUI_ICON_MS_KEYBOARD_ARROW_UP,
        "z",
        "x",
        "c",
        "v",
        "b",
        "n",
        "m",
        EGUI_ICON_MS_BACKSPACE,
        "?123",
        " ",
        EGUI_ICON_MS_DONE,
};

static const char *keyboard_labels_uppercase[EGUI_KEYBOARD_TOTAL_KEYS] = {
        "Q",
        "W",
        "E",
        "R",
        "T",
        "Y",
        "U",
        "I",
        "O",
        "P",
        "A",
        "S",
        "D",
        "F",
        "G",
        "H",
        "J",
        "K",
        "L",
        EGUI_ICON_MS_KEYBOARD_ARROW_UP,
        "Z",
        "X",
        "C",
        "V",
        "B",
        "N",
        "M",
        EGUI_ICON_MS_BACKSPACE,
        "?123",
        " ",
        EGUI_ICON_MS_DONE,
};

static const char *keyboard_labels_symbols[EGUI_KEYBOARD_TOTAL_KEYS] = {
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        "9",
        "0",
        "@",
        "#",
        "$",
        "%",
        "&",
        "-",
        "+",
        "(",
        ")",
        "ABC",
        "!",
        "?",
        ",",
        ".",
        ";",
        ":",
        "'",
        EGUI_ICON_MS_BACKSPACE,
        "ABC",
        " ",
        EGUI_ICON_MS_DONE,
};

// ============== Character output tables ==============
// '\0' means special key (handled separately)

static const char keyboard_chars_lowercase[EGUI_KEYBOARD_TOTAL_KEYS] = {
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0, 'z', 'x', 'c', 'v', 'b', 'n', 'm', 0, 0, ' ', 0,
};

static const char keyboard_chars_uppercase[EGUI_KEYBOARD_TOTAL_KEYS] = {
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 0, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', 0, 0, ' ', 0,
};

static const char keyboard_chars_symbols[EGUI_KEYBOARD_TOTAL_KEYS] = {
        '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '@', '#', '$', '%', '&', '-', '+', '(', ')', 0, '!', '?', ',', '.', ';', ':', '\'', 0, 0, ' ', 0,
};

// ============== Background styles ==============

// Normal key background
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_kb_key_param_normal, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_100, 3);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_kb_key_param_pressed, EGUI_COLOR_LIGHT_GREY, EGUI_ALPHA_100, 3);
EGUI_BACKGROUND_PARAM_INIT(bg_kb_key_params, &bg_kb_key_param_normal, &bg_kb_key_param_pressed, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_kb_key, &bg_kb_key_params);

// Special key background
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_kb_special_param_normal, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_100, 3);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_kb_special_param_pressed, EGUI_COLOR_LIGHT_GREY, EGUI_ALPHA_100, 3);
EGUI_BACKGROUND_PARAM_INIT(bg_kb_special_params, &bg_kb_special_param_normal, &bg_kb_special_param_pressed, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_kb_special, &bg_kb_special_params);

// Keyboard background
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_kb_param_normal, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_kb_params, &bg_kb_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_kb, &bg_kb_params);

// ============== Internal helpers ==============

/**
 * Find the keyboard instance from a key button by walking up the parent chain.
 * button -> row_layout -> keyboard
 */
static egui_view_keyboard_t *egui_view_keyboard_find_from_key(egui_view_t *key_view)
{
    egui_view_t *row = EGUI_VIEW_PARENT(key_view);
    if (row == NULL)
    {
        return NULL;
    }
    egui_view_t *kbd = EGUI_VIEW_PARENT(row);
    if (kbd == NULL)
    {
        return NULL;
    }
    return (egui_view_keyboard_t *)kbd;
}

/**
 * Get the key index from a button pointer using pointer arithmetic.
 */
static int egui_view_keyboard_get_key_index(egui_view_keyboard_t *keyboard, egui_view_t *key_view)
{
    egui_view_button_t *btn = (egui_view_button_t *)key_view;
    int index = (int)(btn - keyboard->keys);
    if (index >= 0 && index < EGUI_KEYBOARD_TOTAL_KEYS)
    {
        return index;
    }
    return -1;
}

/**
 * Get the character table for the current mode.
 */
static const char *egui_view_keyboard_get_char_table(uint8_t mode)
{
    switch (mode)
    {
    case EGUI_KEYBOARD_MODE_UPPERCASE:
        return keyboard_chars_uppercase;
    case EGUI_KEYBOARD_MODE_SYMBOLS:
        return keyboard_chars_symbols;
    default:
        return keyboard_chars_lowercase;
    }
}

static const egui_font_t *egui_view_keyboard_resolve_icon_font(const egui_view_keyboard_t *keyboard)
{
    if (keyboard->icon_font != NULL)
    {
        return keyboard->icon_font;
    }

    return egui_view_icon_font_get_auto_fixed(EGUI_FONT_ICON_MS_20);
}

static const char *egui_view_keyboard_get_mode_label_table(uint8_t mode, int key_idx)
{
    // Labels may differ from emitted characters because some positions are mode/action keys.
    switch (mode)
    {
    case EGUI_KEYBOARD_MODE_UPPERCASE:
        return keyboard_labels_uppercase[key_idx];
    case EGUI_KEYBOARD_MODE_SYMBOLS:
        return keyboard_labels_symbols[key_idx];
    default:
        return keyboard_labels_lowercase[key_idx];
    }
}

static const char *egui_view_keyboard_get_key_label_for_mode(const egui_view_keyboard_t *keyboard, uint8_t mode, int key_idx)
{
    // Special keys can override the generic mode table with configurable icon glyphs.
    if (key_idx == EGUI_KEYBOARD_KEY_IDX_SHIFT && mode != EGUI_KEYBOARD_MODE_SYMBOLS)
    {
        return keyboard->shift_icon;
    }

    if (key_idx == EGUI_KEYBOARD_KEY_IDX_BACKSPACE)
    {
        return keyboard->backspace_icon;
    }

    if (key_idx == EGUI_KEYBOARD_KEY_IDX_ENTER)
    {
        return keyboard->enter_icon;
    }

    return egui_view_keyboard_get_mode_label_table(mode, key_idx);
}

static int egui_view_keyboard_key_uses_icon(const egui_view_keyboard_t *keyboard, int key_idx)
{
    // Shift only uses the icon in alphabetic modes; in symbols mode that slot becomes an "ABC" label.
    if (key_idx == EGUI_KEYBOARD_KEY_IDX_BACKSPACE || key_idx == EGUI_KEYBOARD_KEY_IDX_ENTER)
    {
        return 1;
    }

    if (key_idx == EGUI_KEYBOARD_KEY_IDX_SHIFT && keyboard->mode != EGUI_KEYBOARD_MODE_SYMBOLS)
    {
        return 1;
    }

    return 0;
}

static const egui_font_t *egui_view_keyboard_get_label_font(const egui_view_keyboard_t *keyboard, int key_idx)
{
    // Icon-bearing keys can use a separate font from normal text keys.
    if (egui_view_keyboard_key_uses_icon(keyboard, key_idx))
    {
        return egui_view_keyboard_resolve_icon_font(keyboard);
    }

    if (keyboard->font != NULL)
    {
        return keyboard->font;
    }

    return (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
}

static void egui_view_keyboard_apply_key_label(egui_view_keyboard_t *keyboard, int key_idx, const char *label)
{
    egui_view_t *key_view = EGUI_VIEW_OF(&keyboard->keys[key_idx]);

    // Each key is implemented as a button with an embedded label child, so text/font updates reuse button APIs.
    egui_view_label_set_text(key_view, label);
    egui_view_label_set_font(key_view, egui_view_keyboard_get_label_font(keyboard, key_idx));
}

static egui_view_t *egui_view_keyboard_get_key_view(egui_view_keyboard_t *keyboard, int key_idx)
{
    if (keyboard == NULL || key_idx < 0 || key_idx >= EGUI_KEYBOARD_TOTAL_KEYS)
    {
        return NULL;
    }

    return EGUI_VIEW_OF(&keyboard->keys[key_idx]);
}

static int egui_view_keyboard_is_keyboard_key(egui_view_keyboard_t *keyboard, egui_view_t *view)
{
    uint8_t i;

    if (keyboard == NULL || view == NULL)
    {
        return 0;
    }

    for (i = 0; i < EGUI_KEYBOARD_TOTAL_KEYS; i++)
    {
        if (view == EGUI_VIEW_OF(&keyboard->keys[i]))
        {
            return 1;
        }
    }

    return 0;
}

static void egui_view_keyboard_request_key_focus(egui_view_keyboard_t *keyboard, int key_idx)
{
    egui_view_t *key_view = egui_view_keyboard_get_key_view(keyboard, key_idx);

    if (key_view != NULL)
    {
        egui_view_request_focus(key_view);
    }
}

static int egui_view_keyboard_find_focused_key_index(egui_view_keyboard_t *keyboard)
{
    egui_view_t *focused;

    if (keyboard == NULL)
    {
        return -1;
    }

    focused = egui_view_get_focused_view(EGUI_VIEW_OF(keyboard));
    if (!egui_view_keyboard_is_keyboard_key(keyboard, focused))
    {
        return -1;
    }

    return egui_view_keyboard_get_key_index(keyboard, focused);
}

static int egui_view_keyboard_get_row_start(int row)
{
    switch (row)
    {
    case 0:
        return 0;
    case 1:
        return EGUI_KEYBOARD_ROW0_KEY_COUNT;
    case 2:
        return EGUI_KEYBOARD_ROW0_KEY_COUNT + EGUI_KEYBOARD_ROW1_KEY_COUNT;
    case 3:
        return EGUI_KEYBOARD_ROW0_KEY_COUNT + EGUI_KEYBOARD_ROW1_KEY_COUNT + EGUI_KEYBOARD_ROW2_KEY_COUNT;
    default:
        return 0;
    }
}

static int egui_view_keyboard_get_row_count(int row)
{
    switch (row)
    {
    case 0:
        return EGUI_KEYBOARD_ROW0_KEY_COUNT;
    case 1:
        return EGUI_KEYBOARD_ROW1_KEY_COUNT;
    case 2:
        return EGUI_KEYBOARD_ROW2_KEY_COUNT;
    case 3:
        return EGUI_KEYBOARD_ROW3_KEY_COUNT;
    default:
        return 0;
    }
}

static int egui_view_keyboard_get_key_row(int key_idx)
{
    if (key_idx < EGUI_KEYBOARD_ROW0_KEY_COUNT)
    {
        return 0;
    }
    if (key_idx < EGUI_KEYBOARD_ROW0_KEY_COUNT + EGUI_KEYBOARD_ROW1_KEY_COUNT)
    {
        return 1;
    }
    if (key_idx < EGUI_KEYBOARD_ROW0_KEY_COUNT + EGUI_KEYBOARD_ROW1_KEY_COUNT + EGUI_KEYBOARD_ROW2_KEY_COUNT)
    {
        return 2;
    }

    return 3;
}

static int egui_view_keyboard_get_key_col(int key_idx)
{
    int row = egui_view_keyboard_get_key_row(key_idx);

    return key_idx - egui_view_keyboard_get_row_start(row);
}

static int egui_view_keyboard_get_closest_col_in_row(int from_row, int from_col, int to_row)
{
    int from_count = egui_view_keyboard_get_row_count(from_row);
    int to_count = egui_view_keyboard_get_row_count(to_row);
    int to_col;

    if (from_count <= 0 || to_count <= 0)
    {
        return 0;
    }

    to_col = ((from_col * 2 + 1) * to_count) / (from_count * 2);
    if (to_col < 0)
    {
        to_col = 0;
    }
    if (to_col >= to_count)
    {
        to_col = to_count - 1;
    }

    return to_col;
}

static int egui_view_keyboard_move_focus(egui_view_keyboard_t *keyboard, uint8_t key_code)
{
    int current_idx = egui_view_keyboard_find_focused_key_index(keyboard);
    int row;
    int col;
    int row_count;

    if (current_idx < 0)
    {
        egui_view_keyboard_request_key_focus(keyboard, 0);
        return 1;
    }

    row = egui_view_keyboard_get_key_row(current_idx);
    col = egui_view_keyboard_get_key_col(current_idx);
    row_count = egui_view_keyboard_get_row_count(row);

    switch (key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        col = (col > 0) ? (col - 1) : (row_count - 1);
        break;
    case EGUI_KEY_CODE_RIGHT:
        col = (col + 1) % row_count;
        break;
    case EGUI_KEY_CODE_UP:
        if (row > 0)
        {
            col = egui_view_keyboard_get_closest_col_in_row(row, col, row - 1);
            row--;
        }
        break;
    case EGUI_KEY_CODE_DOWN:
        if (row < EGUI_KEYBOARD_ROW_COUNT - 1)
        {
            col = egui_view_keyboard_get_closest_col_in_row(row, col, row + 1);
            row++;
        }
        break;
    default:
        return 0;
    }

    egui_view_keyboard_request_key_focus(keyboard, egui_view_keyboard_get_row_start(row) + col);
    return 1;
}

// ============== Key click callback ==============

static void egui_view_keyboard_key_click_cb(egui_view_t *self)
{
    egui_view_keyboard_t *keyboard = egui_view_keyboard_find_from_key(self);
    if (keyboard == NULL || keyboard->target == NULL)
    {
        return;
    }

    int key_idx = egui_view_keyboard_get_key_index(keyboard, self);
    if (key_idx < 0)
    {
        return;
    }

    // Shift toggles only between lowercase/uppercase; symbols mode uses this slot as a return-to-ABC shortcut.
    if (key_idx == EGUI_KEYBOARD_KEY_IDX_SHIFT)
    {
        if (keyboard->mode == EGUI_KEYBOARD_MODE_LOWERCASE)
        {
            egui_view_keyboard_set_mode(EGUI_VIEW_OF(keyboard), EGUI_KEYBOARD_MODE_UPPERCASE);
        }
        else if (keyboard->mode == EGUI_KEYBOARD_MODE_UPPERCASE)
        {
            egui_view_keyboard_set_mode(EGUI_VIEW_OF(keyboard), EGUI_KEYBOARD_MODE_LOWERCASE);
        }
        else
        {
            // In symbols mode, this position acts as a mode switch back to lowercase
            egui_view_keyboard_set_mode(EGUI_VIEW_OF(keyboard), EGUI_KEYBOARD_MODE_LOWERCASE);
        }
        return;
    }

    // Backspace delegates editing to the active textinput.
    if (key_idx == EGUI_KEYBOARD_KEY_IDX_BACKSPACE)
    {
        egui_view_textinput_delete_char(keyboard->target);
        return;
    }

    // The mode key switches between symbol layout and the normal alphabet layout.
    if (key_idx == EGUI_KEYBOARD_KEY_IDX_MODE)
    {
        if (keyboard->mode == EGUI_KEYBOARD_MODE_SYMBOLS)
        {
            egui_view_keyboard_set_mode(EGUI_VIEW_OF(keyboard), EGUI_KEYBOARD_MODE_LOWERCASE);
        }
        else
        {
            egui_view_keyboard_set_mode(EGUI_VIEW_OF(keyboard), EGUI_KEYBOARD_MODE_SYMBOLS);
        }
        return;
    }

    // Enter forwards the submit callback instead of inserting a newline.
    if (key_idx == EGUI_KEYBOARD_KEY_IDX_ENTER)
    {
        egui_view_textinput_t *ti = (egui_view_textinput_t *)keyboard->target;
        if (ti->on_submit)
        {
            ti->on_submit(keyboard->target, ti->text);
        }
        egui_view_keyboard_hide(EGUI_VIEW_OF(keyboard));
        return;
    }

    // Normal keys emit one byte from the current mode table into the target textinput.
    const char *char_table = egui_view_keyboard_get_char_table(keyboard->mode);
    char c = char_table[key_idx];
    if (c != 0)
    {
        egui_view_textinput_insert_char(keyboard->target, c);

        // Uppercase behaves like a temporary shift state rather than caps lock.
        if (keyboard->mode == EGUI_KEYBOARD_MODE_UPPERCASE)
        {
            egui_view_keyboard_set_mode(EGUI_VIEW_OF(keyboard), EGUI_KEYBOARD_MODE_LOWERCASE);
        }
    }
}

static int egui_view_keyboard_handle_key_event(egui_view_keyboard_t *keyboard, egui_key_event_t *event)
{
    if (keyboard == NULL || event == NULL)
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_LEFT:
    case EGUI_KEY_CODE_RIGHT:
    case EGUI_KEY_CODE_UP:
    case EGUI_KEY_CODE_DOWN:
        if (event->type == EGUI_KEY_EVENT_ACTION_UP)
        {
            return egui_view_keyboard_move_focus(keyboard, event->key_code);
        }
        return event->type == EGUI_KEY_EVENT_ACTION_DOWN;
    case EGUI_KEY_CODE_ESCAPE:
        if (event->type == EGUI_KEY_EVENT_ACTION_UP)
        {
            egui_view_keyboard_hide(EGUI_VIEW_OF(keyboard));
        }
        return 1;
    default:
        break;
    }

    return 0;
}

static int egui_view_keyboard_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_keyboard_t);

    if (egui_view_keyboard_handle_key_event(local, event))
    {
        return 1;
    }

    return egui_view_on_key_event(self, event);
}

static int egui_view_keyboard_key_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    egui_view_keyboard_t *keyboard = egui_view_keyboard_find_from_key(self);

    if (egui_view_keyboard_handle_key_event(keyboard, event))
    {
        return 1;
    }

    return egui_view_on_key_event(self, event);
}

static void egui_view_keyboard_key_on_focus_changed(egui_view_t *self, int is_focused)
{
    egui_view_keyboard_t *keyboard = egui_view_keyboard_find_from_key(self);
    egui_view_t *focused;

    if (keyboard == NULL || is_focused)
    {
        return;
    }

    focused = egui_view_get_focused_view(self);
    if (!egui_view_is_self_or_descendant_of(focused, EGUI_VIEW_OF(keyboard)) && focused != keyboard->target && keyboard->target != NULL)
    {
        egui_view_keyboard_hide(EGUI_VIEW_OF(keyboard));
    }
}

static void egui_view_keyboard_key_on_draw_focus_frame(egui_view_t *self, const egui_region_t *frame_region)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_dim_t x;
    egui_dim_t y;

    if (self == NULL || canvas == NULL || frame_region == NULL || self->focus_frame_stroke <= 0)
    {
        return;
    }

    x = self->region_screen.location.x - frame_region->location.x;
    y = self->region_screen.location.y - frame_region->location.y;
    egui_canvas_draw_rectangle(canvas, x, y, self->region_screen.size.width, self->region_screen.size.height, self->focus_frame_stroke, self->focus_frame_color,
                               self->focus_frame_alpha);
}

// ============== Public API ==============

void egui_view_keyboard_set_mode(egui_view_t *self, uint8_t mode)
{
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    uint8_t i;

    // Invalid inputs degrade to lowercase so callers can safely pass unchecked values.
    if (mode > EGUI_KEYBOARD_MODE_SYMBOLS)
    {
        mode = EGUI_KEYBOARD_MODE_LOWERCASE;
    }

    local->mode = mode;

    for (i = 0; i < EGUI_KEYBOARD_TOTAL_KEYS; i++)
    {
        egui_view_keyboard_apply_key_label(local, i, egui_view_keyboard_get_key_label_for_mode(local, mode, i));
    }
}

uint8_t egui_view_keyboard_get_mode(egui_view_t *self)
{
    if (self == NULL)
    {
        return EGUI_KEYBOARD_MODE_LOWERCASE;
    }
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    return local->mode;
}

void egui_view_keyboard_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    uint8_t i;

    // Keep a usable default so callers do not need to provide a font explicitly.
    local->font = font != NULL ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    for (i = 0; i < EGUI_KEYBOARD_TOTAL_KEYS; i++)
    {
        egui_view_label_set_font(EGUI_VIEW_OF(&local->keys[i]), egui_view_keyboard_get_label_font(local, i));
    }
}

const egui_font_t *egui_view_keyboard_get_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    return local->font;
}

void egui_view_keyboard_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    uint8_t i;

    if (local->icon_font == font)
    {
        return;
    }

    // Only icon-backed keys need to be refreshed when the icon font changes.
    local->icon_font = font;

    for (i = 0; i < EGUI_KEYBOARD_TOTAL_KEYS; i++)
    {
        if (!egui_view_keyboard_key_uses_icon(local, i))
        {
            continue;
        }
        egui_view_label_set_font(EGUI_VIEW_OF(&local->keys[i]), egui_view_keyboard_get_label_font(local, i));
    }
}

const egui_font_t *egui_view_keyboard_get_icon_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    return local->icon_font;
}

void egui_view_keyboard_set_special_key_icons(egui_view_t *self, const char *shift_icon, const char *backspace_icon, const char *enter_icon)
{
    EGUI_LOCAL_INIT(egui_view_keyboard_t);

    // Null inputs restore the built-in Material Symbols defaults.
    if (shift_icon == NULL)
    {
        shift_icon = EGUI_ICON_MS_KEYBOARD_ARROW_UP;
    }
    if (backspace_icon == NULL)
    {
        backspace_icon = EGUI_ICON_MS_BACKSPACE;
    }
    if (enter_icon == NULL)
    {
        enter_icon = EGUI_ICON_MS_DONE;
    }

    if (local->shift_icon == shift_icon && local->backspace_icon == backspace_icon && local->enter_icon == enter_icon)
    {
        return;
    }

    local->shift_icon = shift_icon;
    local->backspace_icon = backspace_icon;
    local->enter_icon = enter_icon;

    // Re-enter the current mode so any visible special-key labels are refreshed together.
    egui_view_keyboard_set_mode(self, local->mode);
}

const char *egui_view_keyboard_get_shift_icon(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    return local->shift_icon;
}

const char *egui_view_keyboard_get_backspace_icon(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    return local->backspace_icon;
}

const char *egui_view_keyboard_get_enter_icon(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    return local->enter_icon;
}

egui_view_t *egui_view_keyboard_get_target(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    return local->target;
}

void egui_view_keyboard_show(egui_view_t *self, egui_view_t *target_textinput)
{
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    egui_view_t *old_target = local->target;

    if (target_textinput != NULL && local->suppress_show_target == target_textinput)
    {
        local->suppress_show_target = NULL;
        return;
    }

    if (old_target != NULL && old_target != target_textinput)
    {
        egui_view_textinput_set_cursor_active(old_target, 0);
    }

    // Restore any previous root-view offset first so switching targets does not accumulate shifts.
    if (local->adjusted_view != NULL)
    {
        egui_view_update_region_dirty(self, &local->adjusted_view->region_screen);
        egui_view_set_position(local->adjusted_view, local->adjusted_view->region.location.x, local->saved_y);
        local->adjusted_view = NULL;
    }

    local->target = target_textinput;
    if (target_textinput != NULL)
    {
        egui_view_textinput_set_cursor_active(target_textinput, 1);
    }
    self->is_visible = true;
    self->is_gone = false;

    // If the target would be covered, move its root ancestor upward instead of moving only the textinput itself.
    egui_dim_t kbd_top = self->region.location.y;
    egui_dim_t target_bottom = target_textinput->region_screen.location.y + target_textinput->region_screen.size.height;

    if (target_bottom + 4 > kbd_top)
    {
        // Walk to the nearest user-root child so the page/dialog shifts without moving the keyboard overlay itself.
        egui_view_t *root_view = target_textinput;
        egui_core_t *core = egui_view_get_core(self);
        egui_view_t *user_root = core != NULL ? EGUI_VIEW_OF(egui_core_get_user_root_view(core)) : NULL;
        while (EGUI_VIEW_PARENT(root_view) != NULL && EGUI_VIEW_PARENT(root_view) != user_root)
        {
            root_view = EGUI_VIEW_PARENT(root_view);
        }

        // Skip adjustment if the discovered root is the keyboard itself.
        if (root_view != self)
        {
            // Move the root just enough to keep a small gap between the target and the keyboard top edge.
            egui_dim_t shift = target_bottom - kbd_top + 4;

            local->adjusted_view = root_view;
            local->saved_y = root_view->region.location.y;

            // Dirty the old screen region before moving so the renderer repaints the vacated area.
            egui_view_update_region_dirty(self, &root_view->region_screen);

            egui_view_set_position(root_view, root_view->region.location.x, local->saved_y - shift);
        }
    }

    egui_view_invalidate(self);
    egui_view_keyboard_request_key_focus(local, 0);
}

void egui_view_keyboard_hide(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    egui_view_t *target = local->target;
    egui_view_t *focused = egui_view_get_focused_view(self);
    uint8_t focus_inside_keyboard = egui_view_is_self_or_descendant_of(focused, self);

    if (target != NULL && focus_inside_keyboard)
    {
        local->suppress_show_target = target;
    }

    local->target = NULL;
    if (target != NULL)
    {
        egui_view_textinput_set_cursor_active(target, 0);
    }

    // Restore the root view before hiding so the page returns to its original layout.
    if (local->adjusted_view != NULL)
    {
        egui_view_update_region_dirty(self, &local->adjusted_view->region_screen);
        egui_view_set_position(local->adjusted_view, local->adjusted_view->region.location.x, local->saved_y);
        local->adjusted_view = NULL;
    }

    // Invalidate before toggling visibility because invisible views no longer contribute dirty regions.
    egui_view_invalidate(self);
    self->is_visible = false;
    self->is_gone = true;

    if (target != NULL && focus_inside_keyboard)
    {
        egui_view_request_focus(target);
        if (local->suppress_show_target == target)
        {
            local->suppress_show_target = NULL;
        }
    }

    if (focus_inside_keyboard && egui_view_is_self_or_descendant_of(egui_view_get_focused_view(self), self))
    {
        egui_view_clear_focus(self);
    }
}

// ============== Helper to init a single key button ==============

static void egui_view_keyboard_init_key(egui_view_keyboard_t *keyboard, int key_idx, egui_dim_t width, const char *label, int is_special,
                                        egui_view_linearlayout_t *row)
{
    egui_view_button_t *key = &keyboard->keys[key_idx];
    egui_view_t *key_view = EGUI_VIEW_OF(key);

    // Keys are ordinary buttons; appearance differences come from width/background/font choices.
    egui_view_button_init(key_view, EGUI_VIEW_OF(keyboard)->core);
    if (key_idx == 0)
    {
        egui_view_copy_api(key_view, &keyboard->key_api);
        keyboard->key_api.on_key_event = egui_view_keyboard_key_on_key_event;
        keyboard->key_api.on_focus_changed = egui_view_keyboard_key_on_focus_changed;
        keyboard->key_api.on_draw_focus_frame = egui_view_keyboard_key_on_draw_focus_frame;
    }
    key_view->api = &keyboard->key_api;
    egui_view_set_size(key_view, width, EGUI_KEYBOARD_KEY_HEIGHT);
    egui_view_label_set_text(key_view, label);
    egui_view_label_set_font_color(key_view, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_set_on_click_listener(key_view, egui_view_keyboard_key_click_cb);
    egui_view_set_background(key_view, is_special ? EGUI_BG_OF(&bg_kb_special) : EGUI_BG_OF(&bg_kb_key));
    egui_view_set_margin(key_view, 1, 1, 1, 1);
    // Prevent ACTION_DOWN on a key from clearing focus on the target textinput before ACTION_UP fires the click.
    key_view->is_no_focus_clear = 1;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    key_view->is_focusable = 1;
    egui_view_set_focus_frame_style(key_view, 0, 2, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(row), key_view);
}

// ============== Init ==============

void egui_view_keyboard_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_keyboard_t);

    // The keyboard itself is a group whose children are row layouts, which in turn own the key buttons.
    egui_view_group_init(self, core);

    egui_view_set_background(self, EGUI_BG_OF(&bg_kb));

    // Gap touches should be consumed by the keyboard surface instead of leaking to views underneath.
    self->is_clickable = true;
    // Gap touches also must not clear textinput focus, or the keyboard would dismiss itself while in use.
    self->is_no_focus_clear = 1;
    egui_view_copy_api(self, &local->api);
    local->api.on_key_event = egui_view_keyboard_on_key_event;
    self->api = &local->api;

    local->mode = EGUI_KEYBOARD_MODE_LOWERCASE;
    local->target = NULL;
    local->suppress_show_target = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->icon_font = NULL;
    local->shift_icon = EGUI_ICON_MS_KEYBOARD_ARROW_UP;
    local->backspace_icon = EGUI_ICON_MS_BACKSPACE;
    local->enter_icon = EGUI_ICON_MS_DONE;
    local->adjusted_view = NULL;
    local->saved_y = 0;
    int key_idx = 0;

    // Build four horizontal rows, then let the outer group stack those rows vertically.
    for (int r = 0; r < EGUI_KEYBOARD_ROW_COUNT; r++)
    {
        egui_view_linearlayout_init(EGUI_VIEW_OF(&local->rows[r]), core);
        egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&local->rows[r]), 1); // horizontal
        egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&local->rows[r]), EGUI_ALIGN_HCENTER);
        egui_view_set_size(EGUI_VIEW_OF(&local->rows[r]), EGUI_KEYBOARD_DEFAULT_WIDTH, EGUI_KEYBOARD_KEY_HEIGHT + 2);
    }

    // --- Row 0: 10 normal keys ---
    for (int i = 0; i < EGUI_KEYBOARD_ROW0_KEY_COUNT; i++, key_idx++)
    {
        egui_view_keyboard_init_key(local, key_idx, EGUI_KEYBOARD_KEY_WIDTH_NORMAL, egui_view_keyboard_get_key_label_for_mode(local, local->mode, key_idx), 0,
                                    &local->rows[0]);
    }

    // --- Row 1: 9 normal keys ---
    for (int i = 0; i < EGUI_KEYBOARD_ROW1_KEY_COUNT; i++, key_idx++)
    {
        egui_view_keyboard_init_key(local, key_idx, EGUI_KEYBOARD_KEY_WIDTH_NORMAL, egui_view_keyboard_get_key_label_for_mode(local, local->mode, key_idx), 0,
                                    &local->rows[1]);
    }

    // --- Row 2: shift + 7 letters + backspace ---
    for (int i = 0; i < EGUI_KEYBOARD_ROW2_KEY_COUNT; i++, key_idx++)
    {
        egui_dim_t w;
        int is_special = 0;

        if (i == 0) // shift
        {
            w = EGUI_KEYBOARD_KEY_WIDTH_SHIFT;
            is_special = 1;
        }
        else if (i == EGUI_KEYBOARD_ROW2_KEY_COUNT - 1) // backspace
        {
            w = EGUI_KEYBOARD_KEY_WIDTH_BACK;
            is_special = 1;
        }
        else // letter
        {
            w = EGUI_KEYBOARD_KEY_WIDTH_SMALL;
        }

        egui_view_keyboard_init_key(local, key_idx, w, egui_view_keyboard_get_key_label_for_mode(local, local->mode, key_idx), is_special, &local->rows[2]);
    }

    // --- Row 3: mode + space + enter ---
    for (int i = 0; i < EGUI_KEYBOARD_ROW3_KEY_COUNT; i++, key_idx++)
    {
        egui_dim_t w;
        int is_special;

        if (i == 0) // mode
        {
            w = EGUI_KEYBOARD_KEY_WIDTH_MODE;
            is_special = 1;
        }
        else if (i == 1) // space
        {
            w = EGUI_KEYBOARD_KEY_WIDTH_SPACE;
            is_special = 0;
        }
        else // enter
        {
            w = EGUI_KEYBOARD_KEY_WIDTH_ENTER;
            is_special = 1;
        }

        egui_view_keyboard_init_key(local, key_idx, w, egui_view_keyboard_get_key_label_for_mode(local, local->mode, key_idx), is_special, &local->rows[3]);
    }

    // First layout keys within each row, then register the row as a child of the keyboard group.
    for (int r = 0; r < EGUI_KEYBOARD_ROW_COUNT; r++)
    {
        egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&local->rows[r]));
        egui_view_group_add_child(self, EGUI_VIEW_OF(&local->rows[r]));
    }

    egui_view_set_size(self, EGUI_KEYBOARD_DEFAULT_WIDTH, EGUI_KEYBOARD_DEFAULT_HEIGHT);

    // The group helper stacks row layouts top-to-bottom inside the keyboard body.
    egui_view_group_layout_childs(self, 0, 0, 0, EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP);

    // The keyboard is created hidden and only becomes active through `show()`.
    self->is_visible = false;
    self->is_gone = true;

    egui_view_keyboard_set_mode(self, local->mode);

    egui_view_set_view_name(self, "egui_view_keyboard");
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
