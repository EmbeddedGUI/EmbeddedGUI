#include "egui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uicode_disp0.h"

static egui_core_t *app_core;
static egui_view_group_t root;
static egui_view_label_t title_label;
static egui_view_label_t name_label;
static egui_view_label_t mode_label;
static egui_view_label_t skip_label;
static egui_view_label_t status_label;
static egui_view_button_t start_button;
static egui_view_button_t disabled_button;
static egui_view_button_t apply_button;
static egui_view_combobox_t mode_combo;
static egui_view_textinput_t name_input;
static egui_view_checkbox_t ready_checkbox;
static egui_view_checkbox_t hidden_checkbox;
static egui_view_switch_t lock_switch;
static egui_view_switch_t gone_switch;
static egui_view_slider_t level_slider;
static egui_view_keyboard_t keyboard;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static egui_view_api_t level_slider_focus_frame_api;
static egui_view_api_t name_input_focus_api;
#endif

static const char *mode_items[] = {"Auto", "Manual", "Service"};
static const char *mode_icons[] = {EGUI_ICON_MS_SYNC, EGUI_ICON_MS_SETTINGS, EGUI_ICON_MS_EDIT};

static char status_text[64] = "Status: idle";

EGUI_VIEW_GROUP_PARAMS_INIT(root_params, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
EGUI_VIEW_LABEL_PARAMS_INIT(title_params, 12, 8, 216, 24, "Focus Navigation", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(name_params, 16, 48, 72, 26, "Name", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(mode_params, 148, 48, 82, 26, "Mode", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);
EGUI_VIEW_BUTTON_PARAMS_INIT(start_button_params, 12, 82, 82, 32, NULL, NULL, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_BUTTON_PARAMS_INIT(disabled_button_params, 100, 82, 40, 32, NULL, NULL, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_COMBOBOX_PARAMS_INIT(mode_combo_params, 148, 82, 80, 32, mode_items, 3, 0);
EGUI_VIEW_GROUP_PARAMS_INIT(keyboard_params, 0, EGUI_CONFIG_SCREEN_HEIGHT - EGUI_KEYBOARD_DEFAULT_HEIGHT, EGUI_KEYBOARD_DEFAULT_WIDTH, EGUI_KEYBOARD_DEFAULT_HEIGHT);
EGUI_VIEW_CHECKBOX_PARAMS_INIT_WITH_TEXT(ready_checkbox_params, 16, 152, 104, 30, 0, "Ready");
EGUI_VIEW_CHECKBOX_PARAMS_INIT_WITH_TEXT(hidden_checkbox_params, 144, 136, 84, 28, 0, "Hidden");
EGUI_VIEW_SWITCH_PARAMS_INIT(lock_switch_params, 144, 152, 64, 28, 0);
EGUI_VIEW_SWITCH_PARAMS_INIT(gone_switch_params, 22, 184, 64, 22, 0);
EGUI_VIEW_SLIDER_PARAMS_INIT(level_slider_params, 22, 206, 188, 22, 35);
EGUI_VIEW_LABEL_PARAMS_INIT(skip_params, 10, 232, 220, 18, "Skip: disabled hidden gone", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);
EGUI_VIEW_BUTTON_PARAMS_INIT(apply_button_params, 66, 256, 108, 30, NULL, NULL, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(status_params, 10, 294, 220, 20, status_text, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);

static void update_status_text(const char *source)
{
    snprintf(status_text, sizeof(status_text), "Status: %s", source);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
}

static void start_button_click_cb(egui_view_t *self)
{
    EGUI_UNUSED(self);
    update_status_text("start");
}

static void apply_button_click_cb(egui_view_t *self)
{
    EGUI_UNUSED(self);
    update_status_text("apply");
}

static void mode_combo_selected_cb(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    snprintf(status_text, sizeof(status_text), "Status: %s", mode_items[index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
}

static void name_input_submit_cb(egui_view_t *self, const char *text)
{
    EGUI_UNUSED(self);
    snprintf(status_text, sizeof(status_text), "Name: %s", text);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
}

static void ready_checkbox_checked_cb(egui_view_t *self, int is_checked)
{
    EGUI_UNUSED(self);
    update_status_text(is_checked ? "ready" : "not ready");
}

static void lock_switch_checked_cb(egui_view_t *self, int is_checked)
{
    EGUI_UNUSED(self);
    update_status_text(is_checked ? "locked" : "unlocked");
}

static void level_slider_changed_cb(egui_view_t *self, uint8_t value)
{
    EGUI_UNUSED(self);
    snprintf(status_text, sizeof(status_text), "Status: level %u", value);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void name_input_show_keyboard(egui_view_t *self)
{
    if (!EGUI_VIEW_OF(&keyboard)->is_visible || keyboard.target != self)
    {
        egui_view_keyboard_show(EGUI_VIEW_OF(&keyboard), self);
    }
}

static void name_input_focus_changed_cb(egui_view_t *self, int is_focused)
{
    egui_view_textinput_t *textinput = (egui_view_textinput_t *)self;
    egui_view_t *focused = egui_view_get_focused_view(self);

    if (is_focused)
    {
        textinput->cursor_visible = 1;
        egui_view_start_timer(self, &textinput->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);
    }
    else if (!egui_view_is_self_or_descendant_of(focused, EGUI_VIEW_OF(&keyboard)))
    {
        textinput->cursor_visible = 0;
        egui_view_stop_timer(self, &textinput->cursor_timer);
        egui_view_keyboard_hide(EGUI_VIEW_OF(&keyboard));
    }

    egui_view_invalidate(self);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int name_input_key_cb(egui_view_t *self, egui_key_event_t *event)
{
    if (event == NULL)
    {
        return 0;
    }

    if (event->key_code == EGUI_KEY_CODE_ENTER)
    {
        if (event->type == EGUI_KEY_EVENT_ACTION_UP)
        {
            name_input_show_keyboard(self);
        }
        return 1;
    }

    return 0;
}
#endif

static void level_slider_focus_frame_draw_cb(egui_view_t *self, const egui_region_t *frame_region)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);

    if (canvas == NULL || frame_region == NULL)
    {
        return;
    }

    egui_canvas_draw_round_rectangle(canvas, 0, 0, frame_region->size.width, frame_region->size.height, 6, 2, EGUI_COLOR_MAGENTA, EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle(canvas, 3, 3, frame_region->size.width - 6, frame_region->size.height - 6, 4, 1, EGUI_COLOR_ORANGE, EGUI_ALPHA_90);
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS && EGUI_CONFIG_FUNCTION_RECORDING_TEST
static void dispatch_key(uint8_t key_code)
{
    egui_key_event_t key_event;

    memset(&key_event, 0, sizeof(key_event));
    key_event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    key_event.key_code = key_code;
    egui_core_process_input_key(app_core, &key_event);

    key_event.type = EGUI_KEY_EVENT_ACTION_UP;
    egui_core_process_input_key(app_core, &key_event);
}

static uint8_t focused_is(egui_view_t *view)
{
    return egui_focus_manager_get_focused_view(app_core) == view;
}

static uint8_t keyboard_focused_key_is(uint8_t index)
{
    if (index >= EGUI_KEYBOARD_TOTAL_KEYS)
    {
        return 0;
    }

    return focused_is(EGUI_VIEW_OF(&keyboard.keys[index]));
}

static uint8_t keyboard_is_hidden(void)
{
    return !EGUI_VIEW_OF(&keyboard)->is_visible && EGUI_VIEW_OF(&keyboard)->is_gone && keyboard.target == NULL;
}
#endif

void test_init_ui(egui_core_t *core)
{
    app_core = core;
    snprintf(status_text, sizeof(status_text), "Status: idle");

    egui_view_group_init_with_params(EGUI_VIEW_OF(&root), core, &root_params);

    egui_view_label_init_with_params(EGUI_VIEW_OF(&title_label), core, &title_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&name_label), core, &name_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&mode_label), core, &mode_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&skip_label), core, &skip_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&status_label), core, &status_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&name_label), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mode_label), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&skip_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);

    egui_view_button_init_with_params(EGUI_VIEW_OF(&start_button), core, &start_button_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&start_button), "Start");
    egui_view_button_set_icon(EGUI_VIEW_OF(&start_button), EGUI_ICON_MS_PLAY_ARROW);
    egui_view_button_set_icon_font(EGUI_VIEW_OF(&start_button), EGUI_FONT_ICON_MS_20);
    egui_view_button_set_icon_text_gap(EGUI_VIEW_OF(&start_button), 4);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focus_frame_style(EGUI_VIEW_OF(&start_button), 1, 3, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
#endif
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&start_button), start_button_click_cb);

    egui_view_button_init_with_params(EGUI_VIEW_OF(&disabled_button), core, &disabled_button_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&disabled_button), "Off");
    egui_view_set_enable(EGUI_VIEW_OF(&disabled_button), false);

    egui_view_textinput_init(EGUI_VIEW_OF(&name_input), core);
    egui_view_set_position(EGUI_VIEW_OF(&name_input), 16, 118);
    egui_view_set_size(EGUI_VIEW_OF(&name_input), 78, 28);
    egui_view_set_padding(EGUI_VIEW_OF(&name_input), 6, 6, 4, 4);
    egui_view_textinput_set_font(EGUI_VIEW_OF(&name_input), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_textinput_set_placeholder(EGUI_VIEW_OF(&name_input), "TextBox");
    egui_view_textinput_set_text_color(EGUI_VIEW_OF(&name_input), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_textinput_set_placeholder_color(EGUI_VIEW_OF(&name_input), EGUI_COLOR_LIGHT_GREY, EGUI_ALPHA_100);
    egui_view_textinput_set_cursor_color(EGUI_VIEW_OF(&name_input), EGUI_COLOR_YELLOW);
    egui_view_textinput_set_max_length(EGUI_VIEW_OF(&name_input), 12);
    egui_view_textinput_set_on_submit(EGUI_VIEW_OF(&name_input), name_input_submit_cb);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focus_frame_style(EGUI_VIEW_OF(&name_input), 2, 2, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
    egui_view_override_api_on_focus_changed(EGUI_VIEW_OF(&name_input), &name_input_focus_api, name_input_focus_changed_cb);
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    name_input_focus_api.on_key = name_input_key_cb;
#endif
#endif

    egui_view_combobox_init_with_params(EGUI_VIEW_OF(&mode_combo), core, &mode_combo_params);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&mode_combo), mode_combo_selected_cb);
    egui_view_combobox_set_item_icons(EGUI_VIEW_OF(&mode_combo), mode_icons);
    egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&mode_combo), EGUI_FONT_ICON_MS_16);
    egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&mode_combo), EGUI_ICON_MS_KEYBOARD_ARROW_DOWN, EGUI_ICON_MS_KEYBOARD_ARROW_UP);
    egui_view_combobox_set_icon_text_gap(EGUI_VIEW_OF(&mode_combo), 4);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&mode_combo), 3);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focus_frame_style(EGUI_VIEW_OF(&mode_combo), 3, 2, EGUI_COLOR_ORANGE, EGUI_ALPHA_100);
#endif

    egui_view_checkbox_init_with_params(EGUI_VIEW_OF(&ready_checkbox), core, &ready_checkbox_params);
    egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(&ready_checkbox), ready_checkbox_checked_cb);
    egui_view_checkbox_set_icon_text_gap(EGUI_VIEW_OF(&ready_checkbox), 8);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focus_frame_style(EGUI_VIEW_OF(&ready_checkbox), 2, 2, EGUI_COLOR_CYAN, EGUI_ALPHA_100);
#endif

    egui_view_checkbox_init_with_params(EGUI_VIEW_OF(&hidden_checkbox), core, &hidden_checkbox_params);
    egui_view_set_visible(EGUI_VIEW_OF(&hidden_checkbox), false);

    egui_view_switch_init_with_params(EGUI_VIEW_OF(&lock_switch), core, &lock_switch_params);
    egui_view_switch_set_state_icons(EGUI_VIEW_OF(&lock_switch), EGUI_ICON_MS_LOCK, EGUI_ICON_MS_VISIBILITY_OFF);
    egui_view_switch_set_icon_font(EGUI_VIEW_OF(&lock_switch), EGUI_FONT_ICON_MS_16);
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&lock_switch), lock_switch_checked_cb);

    egui_view_switch_init_with_params(EGUI_VIEW_OF(&gone_switch), core, &gone_switch_params);
    egui_view_set_gone(EGUI_VIEW_OF(&gone_switch), true);

    egui_view_slider_init_with_params(EGUI_VIEW_OF(&level_slider), core, &level_slider_params);
    egui_view_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&level_slider), level_slider_changed_cb);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focus_frame_style(EGUI_VIEW_OF(&level_slider), 4, 2, EGUI_COLOR_MAGENTA, EGUI_ALPHA_100);
    egui_view_override_api_on_draw_focus_frame(EGUI_VIEW_OF(&level_slider), &level_slider_focus_frame_api, level_slider_focus_frame_draw_cb);
#endif

    egui_view_button_init_with_params(EGUI_VIEW_OF(&apply_button), core, &apply_button_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&apply_button), "Apply");
    egui_view_button_set_icon(EGUI_VIEW_OF(&apply_button), EGUI_ICON_MS_DONE);
    egui_view_button_set_icon_font(EGUI_VIEW_OF(&apply_button), EGUI_FONT_ICON_MS_20);
    egui_view_button_set_icon_text_gap(EGUI_VIEW_OF(&apply_button), 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&apply_button), apply_button_click_cb);

    egui_view_keyboard_init(EGUI_VIEW_OF(&keyboard), core);
    egui_view_keyboard_set_font(EGUI_VIEW_OF(&keyboard), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_keyboard_set_icon_font(EGUI_VIEW_OF(&keyboard), EGUI_FONT_ICON_MS_20);
    egui_view_group_apply_params(EGUI_VIEW_OF(&keyboard), &keyboard_params);

    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&title_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&name_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&mode_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&start_button));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&disabled_button));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&name_input));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&mode_combo));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&ready_checkbox));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&hidden_checkbox));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&lock_switch));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&gone_switch));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&level_slider));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&skip_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&apply_button));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&status_label));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&keyboard));
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static uint8_t runtime_fail_reported;

static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            runtime_fail_reported = 0;
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 1:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_RIGHT);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 2:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !focused_is(EGUI_VIEW_OF(&start_button)))
        {
            report_runtime_failure("direction key did not enter first focusable view");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_ENTER);
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
#else
        EGUI_SIM_SET_CLICK_VIEW(p_action, &start_button, 220);
        return true;
#endif
    case 3:
        if (first_call && strcmp(status_text, "Status: start") != 0)
        {
            report_runtime_failure("start button click did not update status");
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_DOWN);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 4:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !focused_is(EGUI_VIEW_OF(&name_input)))
        {
            report_runtime_failure("down key did not move from start to textinput");
        }
        if (first_call)
        {
            if (EGUI_VIEW_OF(&keyboard)->is_visible || keyboard.target != NULL)
            {
                report_runtime_failure("textinput focus showed keyboard before enter");
            }
            recording_request_snapshot();
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 5:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_ENTER);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 6:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call)
        {
            if (!EGUI_VIEW_OF(&keyboard)->is_visible || keyboard.target != EGUI_VIEW_OF(&name_input) || !keyboard_focused_key_is(0))
            {
                report_runtime_failure("textinput enter did not show keyboard");
            }
            recording_request_snapshot();
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 7:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_DOWN);
            dispatch_key(EGUI_KEY_CODE_RIGHT);
            dispatch_key(EGUI_KEY_CODE_RIGHT);
            dispatch_key(EGUI_KEY_CODE_RIGHT);
            dispatch_key(EGUI_KEY_CODE_RIGHT);
            dispatch_key(EGUI_KEY_CODE_RIGHT);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 8:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !keyboard_focused_key_is(15))
        {
            report_runtime_failure("keyboard direction keys did not reach h");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_ENTER);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 9:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && strcmp(egui_view_textinput_get_text(EGUI_VIEW_OF(&name_input)), "h") != 0)
        {
            report_runtime_failure("keyboard enter did not insert h");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_UP);
            dispatch_key(EGUI_KEY_CODE_RIGHT);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 10:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !keyboard_focused_key_is(7))
        {
            report_runtime_failure("keyboard direction keys did not reach i");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_ENTER);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 11:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && strcmp(egui_view_textinput_get_text(EGUI_VIEW_OF(&name_input)), "hi") != 0)
        {
            report_runtime_failure("keyboard enter did not insert i");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_DOWN);
            dispatch_key(EGUI_KEY_CODE_DOWN);
            dispatch_key(EGUI_KEY_CODE_DOWN);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 12:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !keyboard_focused_key_is(EGUI_KEYBOARD_KEY_IDX_ENTER))
        {
            report_runtime_failure("keyboard direction keys did not reach done");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_ENTER);
            recording_request_snapshot();
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 13:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !keyboard_is_hidden())
        {
            report_runtime_failure("keyboard done key did not hide keyboard");
        }
        if (first_call && strcmp(status_text, "Name: hi") != 0)
        {
            report_runtime_failure("textinput submit did not update status");
        }
        if (first_call && !focused_is(EGUI_VIEW_OF(&name_input)))
        {
            report_runtime_failure("keyboard hide did not restore textinput focus");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_DOWN);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 14:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !focused_is(EGUI_VIEW_OF(&ready_checkbox)))
        {
            report_runtime_failure("down key did not move from textinput to ready checkbox");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_UP);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 15:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !focused_is(EGUI_VIEW_OF(&name_input)))
        {
            report_runtime_failure("up key did not return to textinput");
        }
        if (first_call)
        {
            if (EGUI_VIEW_OF(&keyboard)->is_visible || keyboard.target != NULL)
            {
                report_runtime_failure("returning to textinput showed keyboard before enter");
            }
            dispatch_key(EGUI_KEY_CODE_ENTER);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 16:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call)
        {
            if (!EGUI_VIEW_OF(&keyboard)->is_visible || keyboard.target != EGUI_VIEW_OF(&name_input) || !keyboard_focused_key_is(0))
            {
                report_runtime_failure("textinput enter did not show keyboard after returning");
            }
            dispatch_key(EGUI_KEY_CODE_ESCAPE);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 17:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !keyboard_is_hidden())
        {
            report_runtime_failure("keyboard escape key did not hide keyboard");
        }
        if (first_call && !focused_is(EGUI_VIEW_OF(&name_input)))
        {
            report_runtime_failure("keyboard escape did not restore textinput focus");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_UP);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 18:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !focused_is(EGUI_VIEW_OF(&start_button)))
        {
            report_runtime_failure("up key did not move from textinput to start button");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_RIGHT);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 19:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !focused_is(EGUI_VIEW_OF(&mode_combo)))
        {
            report_runtime_failure("right key did not skip disabled button and reach combobox");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_ENTER);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 20:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !egui_view_combobox_is_expanded(EGUI_VIEW_OF(&mode_combo)))
        {
            report_runtime_failure("combobox did not expand on enter key");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_DOWN);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 21:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && egui_view_combobox_get_current_index(EGUI_VIEW_OF(&mode_combo)) != 1)
        {
            report_runtime_failure("combobox key selection did not advance");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_ENTER);
            recording_request_snapshot();
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 22:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && egui_view_combobox_is_expanded(EGUI_VIEW_OF(&mode_combo)))
        {
            report_runtime_failure("combobox enter did not collapse dropdown");
        }
        if (first_call && strcmp(status_text, "Status: Manual") != 0)
        {
            report_runtime_failure("combobox enter did not notify selection");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_DOWN);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 23:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !focused_is(EGUI_VIEW_OF(&lock_switch)))
        {
            report_runtime_failure("down key did not skip hidden checkbox");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_LEFT);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 24:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !focused_is(EGUI_VIEW_OF(&ready_checkbox)))
        {
            report_runtime_failure("left key did not move from switch to checkbox");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_ENTER);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 25:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !ready_checkbox.is_checked)
        {
            report_runtime_failure("checkbox enter did not toggle state");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_DOWN);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 26:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !focused_is(EGUI_VIEW_OF(&level_slider)))
        {
            report_runtime_failure("down key did not skip gone switch");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_RIGHT);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 27:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && egui_view_slider_get_value(EGUI_VIEW_OF(&level_slider)) != 36)
        {
            report_runtime_failure("slider did not consume right key");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_DOWN);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 28:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !focused_is(EGUI_VIEW_OF(&apply_button)))
        {
            report_runtime_failure("down key did not move from slider to apply button");
        }
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_ENTER);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 29:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && strcmp(status_text, "Status: apply") != 0)
        {
            report_runtime_failure("apply button enter did not update status");
        }
#endif
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    default:
        return false;
    }
}
#endif
