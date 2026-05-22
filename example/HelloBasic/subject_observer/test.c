/*
 * Subject-Observer Demo
 *
 * Demonstrates the egui_subject_t / egui_observer_t data-binding API.
 *
 * Layout:
 *   - Title bar at the top.
 *   - Large counter label in the centre, updated by an observer whenever the
 *     counter model changes.
 *   - Two buttons at the bottom: "+ INC" increments the counter; "RESET" sets
 *     it back to zero.
 *
 * Data flow:
 *   button click → mutate s_counter → egui_subject_notify(&s_counter_subj)
 *   → on_counter_changed() → egui_view_label_set_text() → UI redraws
 *
 * No business logic is inside the view — it only reacts to notifications.
 */

#include "egui.h"
#include <stdio.h>
#include "uicode_disp0.h"
#include "core/egui_subject.h"

#define SCREEN_W EGUI_CONFIG_SCREEN_WIDTH
#define SCREEN_H EGUI_CONFIG_SCREEN_HEIGHT

/* Button geometry */
#define BTN_W     96
#define BTN_H     44
#define BTN_GAP   12
#define BTN_Y     (SCREEN_H - BTN_H - 12)
#define BTN_INC_X (SCREEN_W / 2 - BTN_W - BTN_GAP / 2)
#define BTN_RST_X (SCREEN_W / 2 + BTN_GAP / 2)

/* Counter label geometry */
#define CNT_X 16
#define CNT_Y (SCREEN_H / 2 - 42)
#define CNT_W (SCREEN_W - 32)
#define CNT_H 84

/* ------------------------------------------------------------------ */
/* Model                                                               */
/* ------------------------------------------------------------------ */

static int32_t s_counter;
static egui_subject_t s_counter_subj;

/* ------------------------------------------------------------------ */
/* View objects                                                        */
/* ------------------------------------------------------------------ */

static egui_view_label_t s_title_label;
static egui_view_label_t s_count_label;
static egui_view_button_t s_btn_inc;
static egui_view_button_t s_btn_reset;
static egui_view_group_t s_root;

/* Dynamic text buffer for the counter display */
static char s_count_buf[24];

/* Observer (stored as a static variable — no heap) */
static egui_observer_t s_count_observer;

/* ------------------------------------------------------------------ */
/* Static param blocks (compile-time only)                             */
/* ------------------------------------------------------------------ */

EGUI_VIEW_LABEL_PARAMS_INIT(s_title_params, 0, 8, SCREEN_W, 36, "Subject-Observer Demo", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_PRIMARY_DARK, EGUI_ALPHA_100);

EGUI_VIEW_BUTTON_PARAMS_INIT_SIMPLE(s_btn_inc_params, BTN_INC_X, BTN_Y, BTN_W, BTN_H, "+ INC");
EGUI_VIEW_BUTTON_PARAMS_INIT_SIMPLE(s_btn_reset_params, BTN_RST_X, BTN_Y, BTN_W, BTN_H, "RESET");

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_root_bg_param, EGUI_THEME_SURFACE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_root_bp, &s_root_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_root_bg, &s_root_bp);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(s_count_bg_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100, 8, 1, EGUI_THEME_BORDER, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_count_bp, &s_count_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_count_bg, &s_count_bp);

/* ------------------------------------------------------------------ */
/* Observer callback                                                   */
/* ------------------------------------------------------------------ */

static void on_counter_changed(egui_subject_t *subject, const void *data, void *user_data)
{
    const int32_t *p = (const int32_t *)data;
    EGUI_UNUSED(subject);
    EGUI_UNUSED(user_data);
    snprintf(s_count_buf, sizeof(s_count_buf), "Count: %d", (int)*p);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_count_label), s_count_buf);
}

/* ------------------------------------------------------------------ */
/* Button click callbacks                                              */
/* ------------------------------------------------------------------ */

static void on_inc_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    s_counter++;
    egui_subject_notify(&s_counter_subj, &s_counter);
}

static void on_reset_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    s_counter = 0;
    egui_subject_notify(&s_counter_subj, &s_counter);
}

/* ------------------------------------------------------------------ */
/* UI initialisation                                                   */
/* ------------------------------------------------------------------ */

void test_init_ui(egui_core_t *core)
{
    /* Model setup */
    s_counter = 0;
    egui_subject_init(&s_counter_subj);
    egui_subject_subscribe(&s_counter_subj, &s_count_observer, on_counter_changed, NULL);

    /* Prepare initial counter text */
    snprintf(s_count_buf, sizeof(s_count_buf), "Count: 0");

    egui_view_group_init(EGUI_VIEW_OF(&s_root), core);
    egui_view_set_position(EGUI_VIEW_OF(&s_root), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&s_root), SCREEN_W, SCREEN_H);
    egui_view_set_background(EGUI_VIEW_OF(&s_root), EGUI_BG_OF(&s_root_bg));

    /* Title label */
    egui_view_label_init_with_params(EGUI_VIEW_OF(&s_title_label), core, &s_title_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_title_label), EGUI_ALIGN_CENTER);

    /* Counter display label — text is set at runtime and updated via observer */
    egui_view_label_init(EGUI_VIEW_OF(&s_count_label), core);
    egui_view_set_position(EGUI_VIEW_OF(&s_count_label), CNT_X, CNT_Y);
    egui_view_set_size(EGUI_VIEW_OF(&s_count_label), CNT_W, CNT_H);
    egui_view_label_set_font(EGUI_VIEW_OF(&s_count_label), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_count_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_count_label), EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_count_label), s_count_buf);
    egui_view_set_background(EGUI_VIEW_OF(&s_count_label), EGUI_BG_OF(&s_count_bg));

    /* Increment button */
    egui_view_button_init_with_params(EGUI_VIEW_OF(&s_btn_inc), core, &s_btn_inc_params);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&s_btn_inc), on_inc_click);

    /* Reset button */
    egui_view_button_init_with_params(EGUI_VIEW_OF(&s_btn_reset), core, &s_btn_reset_params);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&s_btn_reset), on_reset_click);

    egui_view_group_add_child(EGUI_VIEW_OF(&s_root), EGUI_VIEW_OF(&s_title_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&s_root), EGUI_VIEW_OF(&s_count_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&s_root), EGUI_VIEW_OF(&s_btn_inc));
    egui_view_group_add_child(EGUI_VIEW_OF(&s_root), EGUI_VIEW_OF(&s_btn_reset));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&s_root));
}

/* ------------------------------------------------------------------ */
/* Recording test                                                      */
/* ------------------------------------------------------------------ */

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        /* Capture initial state: Count: 0 */
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 1:
        /* Click INC once → Count: 1 */
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&s_btn_inc), 500);
        return true;
    case 2:
        /* Click INC again → Count: 2 */
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&s_btn_inc), 500);
        return true;
    case 3:
        /* Reset → Count: 0 */
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&s_btn_reset), 500);
        return true;
    default:
        return false;
    }
}
#endif
