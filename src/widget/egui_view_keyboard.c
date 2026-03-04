#include <stdio.h>
#include <string.h>

#include "egui_view_keyboard.h"
#include "font/egui_font.h"
#include "core/egui_api.h"
#include "core/egui_core.h"
#include "background/egui_background_color.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

// ============== Key label tables for each mode ==============

// Index order: Row0[0..9], Row1[10..18], Row2[19..27], Row3[28..30]
// Row2: [19]=shift, [20..26]=letters, [27]=backspace
// Row3: [28]=mode, [29]=space, [30]=enter

static const char *keyboard_labels_lowercase[EGUI_KEYBOARD_TOTAL_KEYS] = {
        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "a", "s", "d",    "f", "g",  "h",
        "j", "k", "l", "^", "z", "x", "c", "v", "b", "n", "m", "<", "?123", " ", "OK",
};

static const char *keyboard_labels_uppercase[EGUI_KEYBOARD_TOTAL_KEYS] = {
        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "A", "S", "D",    "F", "G",  "H",
        "J", "K", "L", "^", "Z", "X", "C", "V", "B", "N", "M", "<", "?123", " ", "OK",
};

static const char *keyboard_labels_symbols[EGUI_KEYBOARD_TOTAL_KEYS] = {
        "1", "2", "3", "4",    "5", "6", "7", "8", "9", "0", "@", "#", "$",   "%", "&",  "-",
        "+", "(", ")", "?123", "!", "?", ",", ".", ";", ":", "'", "<", "ABC", " ", "OK",
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

    // Handle Shift key
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

    // Handle Backspace key
    if (key_idx == EGUI_KEYBOARD_KEY_IDX_BACKSPACE)
    {
        egui_view_textinput_delete_char(keyboard->target);
        return;
    }

    // Handle Mode key (?123 / ABC)
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

    // Handle Enter key
    if (key_idx == EGUI_KEYBOARD_KEY_IDX_ENTER)
    {
        egui_view_textinput_t *ti = (egui_view_textinput_t *)keyboard->target;
        if (ti->on_submit)
        {
            ti->on_submit(keyboard->target, ti->text);
        }
        return;
    }

    // Handle regular character keys
    const char *char_table = egui_view_keyboard_get_char_table(keyboard->mode);
    char c = char_table[key_idx];
    if (c != 0)
    {
        egui_view_textinput_insert_char(keyboard->target, c);

        // Auto-return to lowercase after typing one uppercase letter
        if (keyboard->mode == EGUI_KEYBOARD_MODE_UPPERCASE)
        {
            egui_view_keyboard_set_mode(EGUI_VIEW_OF(keyboard), EGUI_KEYBOARD_MODE_LOWERCASE);
        }
    }
}

// ============== Public API ==============

void egui_view_keyboard_set_mode(egui_view_t *self, uint8_t mode)
{
    EGUI_LOCAL_INIT(egui_view_keyboard_t);

    if (mode > EGUI_KEYBOARD_MODE_SYMBOLS)
    {
        mode = EGUI_KEYBOARD_MODE_LOWERCASE;
    }

    local->mode = mode;

    const char **labels;
    switch (mode)
    {
    case EGUI_KEYBOARD_MODE_UPPERCASE:
        labels = keyboard_labels_uppercase;
        break;
    case EGUI_KEYBOARD_MODE_SYMBOLS:
        labels = keyboard_labels_symbols;
        break;
    default:
        labels = keyboard_labels_lowercase;
        break;
    }

    for (int i = 0; i < EGUI_KEYBOARD_TOTAL_KEYS; i++)
    {
        egui_view_label_set_text(EGUI_VIEW_OF(&local->keys[i]), labels[i]);
    }
}

void egui_view_keyboard_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    local->font = font;

    for (int i = 0; i < EGUI_KEYBOARD_TOTAL_KEYS; i++)
    {
        egui_view_label_set_font(EGUI_VIEW_OF(&local->keys[i]), font);
    }
}

void egui_view_keyboard_show(egui_view_t *self, egui_view_t *target_textinput)
{
    EGUI_LOCAL_INIT(egui_view_keyboard_t);

    // Restore any previous position adjustment first (e.g. switching between textinputs)
    if (local->adjusted_view != NULL)
    {
        egui_core_update_region_dirty(&local->adjusted_view->region_screen);
        egui_view_set_position(local->adjusted_view, local->adjusted_view->region.location.x, local->saved_y);
        local->adjusted_view = NULL;
    }

    local->target = target_textinput;
    self->is_visible = true;
    self->is_gone = false;

    // Keyboard avoidance: check if target textinput is obscured by keyboard
    egui_dim_t kbd_top = self->region.location.y;
    egui_dim_t target_bottom = target_textinput->region_screen.location.y + target_textinput->region_screen.size.height;

    if (target_bottom + 4 > kbd_top)
    {
        // Find the root-level ancestor of the target (walk up until parent is NULL)
        egui_view_t *root_view = target_textinput;
        while (EGUI_VIEW_PARENT(root_view) != NULL)
        {
            root_view = EGUI_VIEW_PARENT(root_view);
        }

        // Don't adjust the keyboard itself
        if (root_view != self)
        {
            // Calculate shift: move target just above keyboard with 4px gap
            egui_dim_t shift = target_bottom - kbd_top + 4;

            // Save original position
            local->adjusted_view = root_view;
            local->saved_y = root_view->region.location.y;

            // Mark old position as dirty before moving
            egui_core_update_region_dirty(&root_view->region_screen);

            // Move root view up
            egui_view_set_position(root_view, root_view->region.location.x, local->saved_y - shift);
        }
    }

    egui_view_invalidate(self);
}

void egui_view_keyboard_hide(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_keyboard_t);
    local->target = NULL;

    // Restore position adjustment before hiding
    if (local->adjusted_view != NULL)
    {
        egui_core_update_region_dirty(&local->adjusted_view->region_screen);
        egui_view_set_position(local->adjusted_view, local->adjusted_view->region.location.x, local->saved_y);
        local->adjusted_view = NULL;
    }

    // Invalidate BEFORE hiding, so the framework marks the keyboard area as dirty
    // (egui_view_invalidate skips invisible views)
    egui_view_invalidate(self);
    self->is_visible = false;
    self->is_gone = true;
}

// ============== Helper to init a single key button ==============

static void egui_view_keyboard_init_key(egui_view_keyboard_t *keyboard, int key_idx, egui_dim_t width, const char *label, int is_special,
                                        egui_view_linearlayout_t *row)
{
    egui_view_button_t *key = &keyboard->keys[key_idx];
    egui_view_t *key_view = EGUI_VIEW_OF(key);

    egui_view_button_init(key_view);
    egui_view_set_size(key_view, width, EGUI_KEYBOARD_KEY_HEIGHT);
    egui_view_label_set_text(key_view, label);
    egui_view_label_set_font_color(key_view, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_set_on_click_listener(key_view, egui_view_keyboard_key_click_cb);
    egui_view_set_background(key_view, is_special ? EGUI_BG_OF(&bg_kb_special) : EGUI_BG_OF(&bg_kb_key));
    egui_view_set_margin(key_view, 1, 1, 1, 1);
    // Keyboard keys must not trigger focus-clear on touch, or they would dismiss the keyboard
    // before the click handler fires (ACTION_DOWN clears focus, ACTION_UP fires click).
    key_view->is_no_focus_clear = 1;
    egui_view_group_add_child(EGUI_VIEW_OF(row), key_view);
}

// ============== Init ==============

void egui_view_keyboard_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_keyboard_t);

    // Init base group
    egui_view_group_init(self);

    // Set keyboard background
    egui_view_set_background(self, EGUI_BG_OF(&bg_kb));

    // Make keyboard clickable to consume touch events on gaps between keys
    self->is_clickable = true;
    // Do not let touches on the keyboard root (gap areas) clear the textinput focus
    self->is_no_focus_clear = 1;

    local->mode = EGUI_KEYBOARD_MODE_LOWERCASE;
    local->target = NULL;
    local->font = NULL;
    local->adjusted_view = NULL;
    local->saved_y = 0;

    const char **labels = keyboard_labels_lowercase;
    int key_idx = 0;

    // Initialize 4 row LinearLayouts
    for (int r = 0; r < EGUI_KEYBOARD_ROW_COUNT; r++)
    {
        egui_view_linearlayout_init(EGUI_VIEW_OF(&local->rows[r]));
        egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&local->rows[r]), 1); // horizontal
        egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&local->rows[r]), EGUI_ALIGN_HCENTER);
        egui_view_set_size(EGUI_VIEW_OF(&local->rows[r]), EGUI_KEYBOARD_DEFAULT_WIDTH, EGUI_KEYBOARD_KEY_HEIGHT + 2);
    }

    // --- Row 0: 10 normal keys ---
    for (int i = 0; i < EGUI_KEYBOARD_ROW0_KEY_COUNT; i++, key_idx++)
    {
        egui_view_keyboard_init_key(local, key_idx, EGUI_KEYBOARD_KEY_WIDTH_NORMAL, labels[key_idx], 0, &local->rows[0]);
    }

    // --- Row 1: 9 normal keys ---
    for (int i = 0; i < EGUI_KEYBOARD_ROW1_KEY_COUNT; i++, key_idx++)
    {
        egui_view_keyboard_init_key(local, key_idx, EGUI_KEYBOARD_KEY_WIDTH_NORMAL, labels[key_idx], 0, &local->rows[1]);
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

        egui_view_keyboard_init_key(local, key_idx, w, labels[key_idx], is_special, &local->rows[2]);
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

        egui_view_keyboard_init_key(local, key_idx, w, labels[key_idx], is_special, &local->rows[3]);
    }

    // Layout each row's children horizontally, then add row to keyboard
    for (int r = 0; r < EGUI_KEYBOARD_ROW_COUNT; r++)
    {
        egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&local->rows[r]));
        egui_view_group_add_child(self, EGUI_VIEW_OF(&local->rows[r]));
    }

    // Set keyboard total size
    egui_view_set_size(self, EGUI_KEYBOARD_DEFAULT_WIDTH, EGUI_KEYBOARD_DEFAULT_HEIGHT);

    // Layout rows vertically within keyboard
    egui_view_group_layout_childs(self, 0, 0, 0, EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP);

    // Start hidden
    self->is_visible = false;
    self->is_gone = true;

    egui_view_set_view_name(self, "egui_view_keyboard");
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
