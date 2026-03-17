#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// Layout container
static egui_view_linearlayout_t container;

// Label for title
static egui_view_label_t label_title;

// Text input field
static egui_view_textinput_t textinput_1;

// Submit button
static egui_view_button_t button_submit;

// Result label
static egui_view_label_t label_result;

// Keyboard
static egui_view_keyboard_t keyboard;

// Background for text input - normal
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_textinput_param_normal, EGUI_COLOR_MAKE(0x30, 0x30, 0x30), EGUI_ALPHA_100, 4, 1,
                                                        EGUI_COLOR_LIGHT_GREY, EGUI_ALPHA_100);
// Background for text input - focused
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_textinput_param_focused, EGUI_COLOR_MAKE(0x30, 0x30, 0x30), EGUI_ALPHA_100, 4, 2, EGUI_COLOR_GREEN,
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT_WITH_FOCUS(bg_textinput_params, &bg_textinput_param_normal, NULL, NULL, &bg_textinput_param_focused);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_textinput, &bg_textinput_params);

// Background for button
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_btn_param_normal, EGUI_COLOR_GREEN, EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_btn_param_pressed, EGUI_COLOR_DARK_GREEN, EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_PARAM_INIT(bg_btn_params, &bg_btn_param_normal, &bg_btn_param_pressed, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_button, &bg_btn_params);

#define TEXTINPUT_WIDTH  180
#define TEXTINPUT_HEIGHT 28
#define BUTTON_WIDTH     100
#define BUTTON_HEIGHT    30

static char result_str[64] = "";

static void on_submit(egui_view_t *self, const char *text)
{
    egui_api_sprintf(result_str, "Submitted: %s", text);
    egui_view_label_set_text(EGUI_VIEW_OF(&label_result), result_str);
}

static void button_click_cb(egui_view_t *self)
{
    const char *text = egui_view_textinput_get_text(EGUI_VIEW_OF(&textinput_1));
    on_submit(EGUI_VIEW_OF(&textinput_1), text);
}

// Click on background area clears focus, which dismisses the keyboard
static void container_click_cb(egui_view_t *self)
{
    egui_focus_manager_clear_focus();
}

// Custom focus change listener that handles both cursor blinking and keyboard show/hide
static void textinput_focus_changed(egui_view_t *self, int is_focused)
{
    egui_view_textinput_t *ti = (egui_view_textinput_t *)self;

    if (is_focused)
    {
        // Start cursor blinking
        ti->cursor_visible = 1;
        egui_timer_start_timer(&ti->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);

        // Show keyboard
        egui_view_keyboard_show(EGUI_VIEW_OF(&keyboard), self);
    }
    else
    {
        // Stop cursor blinking
        ti->cursor_visible = 0;
        egui_timer_stop_timer(&ti->cursor_timer);

        // Hide keyboard
        egui_view_keyboard_hide(EGUI_VIEW_OF(&keyboard));
    }
    egui_view_invalidate(self);
}

void test_init_ui(void)
{
    // Init container
    egui_view_linearlayout_init(EGUI_VIEW_OF(&container));
    egui_view_set_position(EGUI_VIEW_OF(&container), 0, 200);
    egui_view_set_size(EGUI_VIEW_OF(&container), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&container), 0); // vertical
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&container), EGUI_ALIGN_HCENTER);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&container), container_click_cb);

    // Title label
    egui_view_label_init(EGUI_VIEW_OF(&label_title));
    egui_view_set_position(EGUI_VIEW_OF(&label_title), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&label_title), EGUI_CONFIG_SCEEN_WIDTH, 90);
    egui_view_label_set_text(EGUI_VIEW_OF(&label_title), "TextInput Demo");
    egui_view_label_set_font(EGUI_VIEW_OF(&label_title), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&label_title), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_title), EGUI_ALIGN_CENTER);
    egui_view_set_margin(EGUI_VIEW_OF(&label_title), 0, 0, 10, 5);

    // Text input
    egui_view_textinput_init(EGUI_VIEW_OF(&textinput_1));
    egui_view_set_size(EGUI_VIEW_OF(&textinput_1), TEXTINPUT_WIDTH, TEXTINPUT_HEIGHT);
    egui_view_set_padding(EGUI_VIEW_OF(&textinput_1), 6, 6, 4, 4);
    egui_view_textinput_set_font(EGUI_VIEW_OF(&textinput_1), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_textinput_set_text_color(EGUI_VIEW_OF(&textinput_1), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_textinput_set_placeholder(EGUI_VIEW_OF(&textinput_1), "Type here...");
    egui_view_textinput_set_placeholder_color(EGUI_VIEW_OF(&textinput_1), EGUI_COLOR_LIGHT_GREY, EGUI_ALPHA_100);
    egui_view_textinput_set_cursor_color(EGUI_VIEW_OF(&textinput_1), EGUI_COLOR_GREEN);
    egui_view_textinput_set_on_submit(EGUI_VIEW_OF(&textinput_1), on_submit);
    egui_view_set_background(EGUI_VIEW_OF(&textinput_1), EGUI_BG_OF(&bg_textinput));
    egui_view_set_margin(EGUI_VIEW_OF(&textinput_1), 0, 0, 5, 5);

    // Submit button
    egui_view_button_init(EGUI_VIEW_OF(&button_submit));
    egui_view_set_size(EGUI_VIEW_OF(&button_submit), BUTTON_WIDTH, BUTTON_HEIGHT);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_submit), "Submit");
    egui_view_label_set_font(EGUI_VIEW_OF(&button_submit), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&button_submit), EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_submit), button_click_cb);
    egui_view_set_background(EGUI_VIEW_OF(&button_submit), EGUI_BG_OF(&bg_button));
    egui_view_set_margin(EGUI_VIEW_OF(&button_submit), 0, 0, 5, 5);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    EGUI_VIEW_OF(&button_submit)->is_focusable = true;
#endif

    // Result label
    egui_view_label_init(EGUI_VIEW_OF(&label_result));
    egui_view_set_position(EGUI_VIEW_OF(&label_result), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&label_result), EGUI_CONFIG_SCEEN_WIDTH, 20);
    egui_view_label_set_text(EGUI_VIEW_OF(&label_result), "");
    egui_view_label_set_font(EGUI_VIEW_OF(&label_result), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&label_result), EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_result), EGUI_ALIGN_CENTER);
    egui_view_set_margin(EGUI_VIEW_OF(&label_result), 0, 0, 5, 0);

    // Add children to container
    egui_view_group_add_child(EGUI_VIEW_OF(&container), EGUI_VIEW_OF(&label_title));
    egui_view_group_add_child(EGUI_VIEW_OF(&container), EGUI_VIEW_OF(&textinput_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&container), EGUI_VIEW_OF(&button_submit));
    egui_view_group_add_child(EGUI_VIEW_OF(&container), EGUI_VIEW_OF(&label_result));

    // Re-layout children inside the container
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&container));

    // Add container to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&container));

    // Initialize keyboard (positioned at bottom of screen, added as overlay)
    egui_view_keyboard_init(EGUI_VIEW_OF(&keyboard));
    egui_view_keyboard_set_font(EGUI_VIEW_OF(&keyboard), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_keyboard_set_icon_font(EGUI_VIEW_OF(&keyboard), EGUI_FONT_ICON_MS_20);
    egui_view_keyboard_set_special_key_icons(EGUI_VIEW_OF(&keyboard), EGUI_ICON_MS_KEYBOARD_ARROW_UP, EGUI_ICON_MS_BACKSPACE, EGUI_ICON_MS_DONE);
    egui_view_set_position(EGUI_VIEW_OF(&keyboard), 0, EGUI_CONFIG_SCEEN_HEIGHT - EGUI_KEYBOARD_DEFAULT_HEIGHT);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&keyboard));

    // Override textinput focus listener to handle keyboard show/hide
    egui_view_set_on_focus_change_listener(EGUI_VIEW_OF(&textinput_1), textinput_focus_changed);

    // Layout root children
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0: // click textinput to focus and show keyboard
        EGUI_SIM_SET_CLICK_VIEW(p_action, &textinput_1, 1500);
        return true;
    case 1: // wait to show keyboard state
        EGUI_SIM_SET_WAIT(p_action, 1500);
        return true;
    case 2: // click submit button
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_submit, 1000);
        return true;
    default:
        return false;
    }
}
#endif
