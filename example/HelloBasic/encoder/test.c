/*
 * Encoder Input Driver Demo
 *
 * Demonstrates egui_encoder_driver_t:
 *   - CW  rotation  -> counter increments
 *   - CCW rotation  -> counter decrements
 *   - Button press  -> counter resets to zero
 *
 * A mock encoder driver is registered at startup. In recording-test mode
 * the driver plays a scripted sequence so the screenshot captures visible
 * state changes.
 *
 * Layout:
 *   Top:    Title label  ("Encoder Demo")
 *   Center: Large counter label (starts at "0")
 *   Bottom: Hint label   ("CW: +1   CCW: -1   Press: Reset")
 */

#include "egui.h"
#include <stdio.h>
#include <string.h>
#include "uicode_disp0.h"
#include "core/egui_encoder_driver.h"
#include "core/egui_key_event.h"

#define SCREEN_W EGUI_CONFIG_SCREEN_WIDTH
#define SCREEN_H EGUI_CONFIG_SCREEN_HEIGHT

/* ------------------------------------------------------------------ */
/* View objects                                                        */
/* ------------------------------------------------------------------ */

static egui_view_label_t s_title_label;
static egui_view_label_t s_counter_label;
static egui_view_label_t s_hint_label;

/* Focusable proxy view that receives key events. */
static egui_view_t s_focus_view;
static egui_view_api_t s_focus_api;

/* ------------------------------------------------------------------ */
/* Counter state                                                       */
/* ------------------------------------------------------------------ */

static int s_counter;
static char s_counter_buf[16];

/* ------------------------------------------------------------------ */
/* Mock encoder driver                                                 */
/* ------------------------------------------------------------------ */

static int s_poll_count;

static int mock_encoder_read(void *user_data, int16_t *out_delta, uint8_t *out_btn)
{
    (void)user_data;

    s_poll_count++;
    *out_delta = 0;
    *out_btn = 0;

    /* Poll 20-24: five CW ticks -> counter 0->5 */
    if (s_poll_count >= 20 && s_poll_count <= 24)
    {
        *out_delta = 1;
        return 0;
    }

    /* Poll 35-36: button held (press edge on 35, release on 37) */
    if (s_poll_count == 35 || s_poll_count == 36)
    {
        *out_btn = 1;
        return 0;
    }

    /* Poll 46-50: five CCW ticks -> counter goes down */
    if (s_poll_count >= 46 && s_poll_count <= 50)
    {
        *out_delta = -1;
        return 0;
    }

    return 0;
}

static const egui_encoder_driver_ops_t s_mock_ops = {mock_encoder_read};
static egui_encoder_driver_t s_mock_driver;

/* ------------------------------------------------------------------ */
/* Key handler on focus view                                           */
/* ------------------------------------------------------------------ */

static void update_counter_display(void)
{
    snprintf(s_counter_buf, sizeof(s_counter_buf), "%d", s_counter);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_counter_label), s_counter_buf);
    egui_view_invalidate(EGUI_VIEW_OF(&s_counter_label));
}

static int on_key(egui_view_t *self, egui_key_event_t *event)
{
    (void)self;

    if (event->type != EGUI_KEY_EVENT_ACTION_DOWN)
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_RIGHT:
        s_counter++;
        update_counter_display();
        return 1;

    case EGUI_KEY_CODE_LEFT:
        s_counter--;
        update_counter_display();
        return 1;

    case EGUI_KEY_CODE_ENTER:
        s_counter = 0;
        update_counter_display();
        return 1;

    default:
        break;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Entry point                                                         */
/* ------------------------------------------------------------------ */

void test_init_ui(egui_core_t *core)
{
    int title_h = 28;
    int hint_h = 24;
    int center_y = title_h + (SCREEN_H - title_h - hint_h) / 2 - 20;

    /* Title */
    egui_view_label_init(EGUI_VIEW_OF(&s_title_label), core);
    egui_view_set_position(EGUI_VIEW_OF(&s_title_label), 0, 4);
    egui_view_set_size(EGUI_VIEW_OF(&s_title_label), SCREEN_W, title_h);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_title_label), "Encoder Demo");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_title_label), EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_title_label), EGUI_THEME_PRIMARY, EGUI_ALPHA_100);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&s_title_label));

    /* Counter label */
    snprintf(s_counter_buf, sizeof(s_counter_buf), "%d", s_counter);
    egui_view_label_init(EGUI_VIEW_OF(&s_counter_label), core);
    egui_view_set_position(EGUI_VIEW_OF(&s_counter_label), 0, center_y);
    egui_view_set_size(EGUI_VIEW_OF(&s_counter_label), SCREEN_W, 48);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_counter_label), s_counter_buf);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_counter_label), EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_counter_label), EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&s_counter_label));

    /* Hint label */
    egui_view_label_init(EGUI_VIEW_OF(&s_hint_label), core);
    egui_view_set_position(EGUI_VIEW_OF(&s_hint_label), 0, SCREEN_H - hint_h - 4);
    egui_view_set_size(EGUI_VIEW_OF(&s_hint_label), SCREEN_W, hint_h);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_hint_label), "CW: +1   CCW: -1   Press: Reset");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_hint_label), EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_hint_label), EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&s_hint_label));

    /* Focusable proxy view (invisible, full-screen; captures key events) */
    egui_view_init(EGUI_VIEW_OF(&s_focus_view), core);
    egui_view_set_position(EGUI_VIEW_OF(&s_focus_view), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&s_focus_view), SCREEN_W, SCREEN_H);
    egui_view_set_focusable(EGUI_VIEW_OF(&s_focus_view), 1);
    egui_view_override_api_on_key(EGUI_VIEW_OF(&s_focus_view), &s_focus_api, on_key);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&s_focus_view));
    egui_view_request_focus(EGUI_VIEW_OF(&s_focus_view));

    /* Register mock encoder driver */
    s_poll_count = 0;
    s_mock_driver.ops = &s_mock_ops;
    s_mock_driver.user_data = NULL;
    s_mock_driver._last_button = 0;
    s_mock_driver._long_press_sent = 0;
    s_mock_driver._press_tick = 0;
    egui_encoder_driver_register(core, &s_mock_driver);
}

/* ------------------------------------------------------------------ */
/* Recording actions (capture 3 frames at distinct counter values)    */
/* ------------------------------------------------------------------ */

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);

    last_action = action_index;

    /* 4 snapshots: counter=0, counter=5, counter=0 (reset), counter=-5 */
    if (action_index >= 4)
    {
        return false;
    }
    if (first_call)
    {
        recording_request_snapshot();
    }
    EGUI_SIM_SET_WAIT(p_action, 400);
    return true;
}
#endif
