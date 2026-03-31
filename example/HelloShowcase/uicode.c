#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"
#include "app_egui_resource_generate.h"

#ifndef EGUI_SHOWCASE_PARITY_RECORDING
#define EGUI_SHOWCASE_PARITY_RECORDING 0
#endif

#define SHOWCASE_CANVAS_WIDTH      HELLO_SHOWCASE_CANVAS_WIDTH
#define SHOWCASE_CANVAS_HEIGHT     HELLO_SHOWCASE_CANVAS_HEIGHT
#define SHOWCASE_KEYBOARD_HEIGHT   128
#define SHOWCASE_KEYBOARD_Y        ((EGUI_CONFIG_SCEEN_HEIGHT > SHOWCASE_KEYBOARD_HEIGHT) ? (EGUI_CONFIG_SCEEN_HEIGHT - SHOWCASE_KEYBOARD_HEIGHT) : 0)
#define SHOWCASE_KEYBOARD_HIDDEN_Y (EGUI_CONFIG_SCEEN_HEIGHT + SHOWCASE_KEYBOARD_HEIGHT)

// ============================================================================
// Widget Showcase
// Displays all visual widgets on 1280x1024 black canvas for promotion.
// ============================================================================

// ---- Root container ----
// Slate Ocean palette: Dark=#0D1117 Light=#F6F8FA Accent=#58A6FF/#0969DA
static egui_view_canvas_panner_t root;
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_root_p_dark_new, EGUI_COLOR_MAKE(13, 17, 23), EGUI_ALPHA_100); // #0D1117
EGUI_BACKGROUND_PARAM_INIT(bg_root_params_dark_new, &bg_root_p_dark_new, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_root_dark_new, &bg_root_params_dark_new);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_root_p_light_new, EGUI_COLOR_MAKE(246, 248, 250), EGUI_ALPHA_100); // #F6F8FA
EGUI_BACKGROUND_PARAM_INIT(bg_root_params_light_new, &bg_root_p_light_new, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_root_light_new, &bg_root_params_light_new);

// ---- Category title labels (9 total) ----
static egui_view_label_t t_basic, t_toggle, t_progress, t_canvas;
static egui_view_label_t t_slider, t_chart, t_time, t_special, t_data;
static egui_view_t panel_basic, panel_toggle, panel_progress, panel_canvas;
static egui_view_t panel_slider, panel_chart, panel_time, panel_special, panel_data;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_panel_dark_p, EGUI_COLOR_MAKE(22, 27, 34), 210, 12, 1, // #161B22
                                                        EGUI_COLOR_MAKE(48, 54, 61), 200);                        // #30363D
EGUI_BACKGROUND_PARAM_INIT(bg_panel_dark_params, &bg_panel_dark_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_panel_dark, &bg_panel_dark_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_panel_light_p, EGUI_COLOR_MAKE(255, 255, 255), 240, 12, 1, // #FFFFFF
                                                        EGUI_COLOR_MAKE(208, 215, 222), 200);                         // #D0D7DE
EGUI_BACKGROUND_PARAM_INIT(bg_panel_light_params, &bg_panel_light_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_panel_light, &bg_panel_light_params);

// ---- Basic Controls ----
static egui_view_button_t wg_button;
static egui_view_label_t wg_label;
static egui_view_textblock_t wg_textblock;
static egui_view_dynamic_label_t wg_dynlabel;
static egui_view_card_t wg_card;
static egui_view_label_t wg_card_child;
static egui_view_textinput_t wg_textinput;

// ---- Toggle & Selection ----
static egui_view_switch_t wg_switch;
static egui_view_checkbox_t wg_checkbox;
static egui_view_radio_button_t wg_radio;
static egui_view_radio_button_t wg_radio2;
static egui_view_radio_group_t wg_radio_grp;
static egui_view_toggle_button_t wg_togbtn;
static egui_view_led_t wg_led;
static egui_view_notification_badge_t wg_badge;

// ---- Progress & Status ----
static egui_view_progress_bar_t wg_pbar;
static egui_view_circular_progress_bar_t wg_cpbar;
static egui_view_gauge_t wg_gauge;
static egui_view_activity_ring_t wg_actring;
static egui_view_page_indicator_t wg_pagind;

// ---- Slider & Picker ----
static egui_view_slider_t wg_slider;
static egui_view_arc_slider_t wg_arcslider;
static egui_view_number_picker_t wg_numpick;
static egui_view_spinner_t wg_spinner;
static egui_view_roller_t wg_roller;
static egui_view_combobox_t wg_combobox;
static egui_view_scale_t wg_scale;

// ---- Chart ----
static egui_view_chart_line_t wg_chart_line;
static egui_view_chart_bar_t wg_chart_bar;
static egui_view_chart_pie_t wg_chart_pie;
static egui_view_chart_scatter_t wg_chart_scatter;

// ---- Time & Date ----
static egui_view_analog_clock_t wg_aclock;
static egui_view_digital_clock_t wg_dclock;
static egui_view_stopwatch_t wg_stopwatch;
static egui_view_mini_calendar_t wg_calendar;

// ---- Specialized ----
static egui_view_compass_t wg_compass;
static egui_view_heart_rate_t wg_heartrate;
static egui_view_line_t wg_line;
static egui_view_divider_t wg_divider;

// ---- Data & Container ----
static egui_view_table_t wg_table;
static egui_view_list_t wg_list;
static egui_view_button_matrix_t wg_btnmatrix;
static egui_view_spangroup_t wg_spangrp;
static egui_view_tab_bar_t wg_tabbar;
static egui_view_window_t wg_window;
static egui_view_label_t wg_window_lbl;
static egui_view_menu_t wg_menu;

// ---- Keyboard ----
static egui_view_keyboard_t wg_keyboard;

// ---- Canvas Demo views ----
static egui_view_t wg_cv_gradient;
static egui_view_t wg_cv_shadow;
static egui_view_t wg_cv_border1;
static egui_view_t wg_cv_border2;
static egui_view_t wg_cv_alpha1;
static egui_view_t wg_cv_alpha2;
static egui_view_line_t wg_cv_hqline;

// ---- Caption labels (widget type annotations) ----
static egui_view_label_t cap_switch, cap_led, cap_badge;
static egui_view_label_t cap_cpbar, cap_gauge, cap_actring, cap_pagind;
static egui_view_label_t cap_gradient, cap_shadow, cap_border1, cap_border2;
static egui_view_label_t cap_alpha1_lbl, cap_alpha2_lbl, cap_hqline_lbl;
static egui_view_label_t cap_arcslider, cap_spinner;
static egui_view_label_t cap_line_wg, cap_divider_wg;

// ---- Basic panel type caption labels ----
static egui_view_label_t cap_label_wg, cap_textblock_wg;
static egui_view_label_t cap_dynlabel_wg, cap_textinput_wg;

// Cap array for batch color update
static egui_view_label_t *s_captions[] = {
        &cap_switch,
        &cap_led,
        &cap_badge,
        &cap_cpbar,
        &cap_gauge,
        &cap_actring,
        &cap_pagind,
        &cap_gradient,
        &cap_shadow,
        &cap_border1,
        &cap_border2,
        &cap_alpha1_lbl,
        &cap_alpha2_lbl,
        &cap_hqline_lbl,
        &cap_arcslider,
        &cap_spinner,
        &cap_line_wg,
        &cap_divider_wg,
        // Basic panel type captions
        &cap_label_wg,
        &cap_textblock_wg,
        &cap_dynlabel_wg,
        &cap_textinput_wg,
};

// ---- Theme & Layer ----
static egui_view_button_t btn_theme;
static int is_dark_theme = 1;
static uint8_t active_layer = 2;

// ---- Language ----
static egui_view_button_t btn_lang;
static uint8_t is_chinese = 0;

static void update_theme(void);
static void update_language(void);
static void update_layer_visual(void);
static void on_theme_click(egui_view_t *view);
static void on_lang_click(egui_view_t *view);
static void on_layer_click(egui_view_t *view);
static void on_textinput_focus_changed(egui_view_t *self, int is_focused);

// ---- Animation ----
static egui_timer_t anim_timer;
static uint16_t anim_tick = 0;

// ============================================================================
// Static data for widgets
// ============================================================================

// Roller items
static const char *roller_items[] = {"Mon", "Tue", "Wed", "Thu", "Fri"};
static const char *roller_items_cn[] = {"周一", "周二", "周三", "周四", "周五"};
// Combobox items
static const char *combo_items[] = {"Sky", "Mint", "Steel"};
static const char *combo_items_cn[] = {"天空", "薄荷", "钢铁"};
// Tab bar texts
static const char *tab_texts[] = {"Home", "Set", "Info"};
static const char *tab_texts_cn[] = {"首页", "设置", "信息"};
// Mini calendar weekday labels
static const char *calendar_weekdays_cn[] = {"日", "一", "二", "三", "四", "五", "六"};
// Button matrix labels
static const char *bm_labels[] = {"1", "2", "3", "4", "5", "6"};
// Chinese menu data
static const egui_view_menu_item_t menu_items_cn[] = {
        {.text = "设置", .sub_page_index = 0xFF},
        {.text = "关于", .sub_page_index = 0xFF},
        {.text = "帮助", .sub_page_index = 0xFF},
};
static const egui_view_menu_page_t menu_pages_cn[] = {
        {.title = "菜单", .items = menu_items_cn, .item_count = 3},
};

// Chart line data
static const egui_chart_point_t cl_pts[] = {{0, 10}, {20, 45}, {40, 25}, {60, 55}, {80, 35}, {100, 60}};
static const egui_chart_series_t cl_ser[] = {{.points = cl_pts, .point_count = 6, .color = EGUI_COLOR_MAKE(88, 166, 255), .name = "S1"}}; // #58A6FF
// Chart bar data
static const egui_chart_point_t cb_pts[] = {{0, 30}, {1, 55}, {2, 20}, {3, 45}, {4, 38}};
static const egui_chart_series_t cb_ser[] = {{.points = cb_pts, .point_count = 5, .color = EGUI_COLOR_MAKE(63, 185, 80), .name = "S1"}}; // #3FB950
// Chart scatter data
static const egui_chart_point_t cs_pts[] = {{10, 20}, {25, 48}, {40, 30}, {55, 60}, {70, 40}, {85, 52}};
static const egui_chart_series_t cs_ser[] = {{.points = cs_pts, .point_count = 6, .color = EGUI_COLOR_MAKE(255, 123, 114), .name = "S1"}}; // #FF7B72
// Chart pie data
static const egui_chart_pie_slice_t pie_sl[] = {
        {.value = 40, .color = EGUI_COLOR_MAKE(88, 166, 255), .name = "A"},  // #58A6FF
        {.value = 30, .color = EGUI_COLOR_MAKE(63, 185, 80), .name = "B"},   // #3FB950
        {.value = 20, .color = EGUI_COLOR_MAKE(255, 123, 114), .name = "C"}, // #FF7B72
        {.value = 10, .color = EGUI_COLOR_MAKE(203, 166, 247), .name = "D"}, // #CBA6F7
};

// Line widget points (HQ)
static const egui_view_line_point_t hq_pts[] = {{0, 110}, {40, 10}, {80, 80}, {120, 20}, {160, 90}, {200, 40}};

// Menu data
static const egui_view_menu_item_t menu_items[] = {
        {.text = "Settings", .sub_page_index = 0xFF},
        {.text = "About", .sub_page_index = 0xFF},
        {.text = "Help", .sub_page_index = 0xFF},
};
static const egui_view_menu_page_t menu_pages[] = {
        {.title = "Menu", .items = menu_items, .item_count = 3},
};

// ============================================================================
// Backgrounds for canvas demo
// ============================================================================

// Gradient demo: dark=#0D1117->#1B2A3B  light=#FFFFFF->#DCE8F5
EGUI_BACKGROUND_GRADIENT_PARAM_INIT(bg_grad_dark_p, EGUI_BACKGROUND_GRADIENT_DIR_HORIZONTAL, EGUI_COLOR_MAKE(13, 17, 23), EGUI_COLOR_MAKE(27, 42, 59),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_grad_dark_params, &bg_grad_dark_p, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(bg_grad_dark, &bg_grad_dark_params);

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(bg_grad_light_p, EGUI_BACKGROUND_GRADIENT_DIR_HORIZONTAL, EGUI_COLOR_MAKE(255, 255, 255), EGUI_COLOR_MAKE(220, 232, 245),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_grad_light_params, &bg_grad_light_p, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(bg_grad_light, &bg_grad_light_params);

// Shadow demo: dark card #1F2D3D  light card #FFFFFF
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_shadow_dark_p, EGUI_COLOR_MAKE(31, 45, 61), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(bg_shadow_dark_params, &bg_shadow_dark_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_shadow_dark, &bg_shadow_dark_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_shadow_light_p, EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(bg_shadow_light_params, &bg_shadow_light_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_shadow_light, &bg_shadow_light_params);

EGUI_SHADOW_PARAM_INIT_ROUND(shadow_demo_dark, 10, 5, 5, EGUI_COLOR_MAKE(0, 0, 0), 160, 10);
EGUI_SHADOW_PARAM_INIT_ROUND(shadow_demo_light, 10, 4, 4, EGUI_COLOR_MAKE(128, 128, 128), 130, 8);

// TextInput focus-aware background (normal: grey border; focused: accent blue border)
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_ti_normal_p, EGUI_COLOR_MAKE(22, 27, 34), EGUI_ALPHA_100, 4, 1, EGUI_COLOR_MAKE(72, 80, 96),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_ti_focused_p, EGUI_COLOR_MAKE(22, 27, 34), EGUI_ALPHA_100, 4, 2, EGUI_COLOR_MAKE(88, 166, 255),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT_WITH_FOCUS(bg_ti_params, &bg_ti_normal_p, NULL, NULL, &bg_ti_focused_p);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_textinput_showcase, &bg_ti_params);

// TextInput light-mode background (white fill, blue accent border on focus)
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_ti_light_normal_p, EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100, 4, 1,
                                                        EGUI_COLOR_MAKE(168, 177, 186), EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_ti_light_focused_p, EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100, 4, 2,
                                                        EGUI_COLOR_MAKE(9, 105, 218), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT_WITH_FOCUS(bg_ti_light_params, &bg_ti_light_normal_p, NULL, NULL, &bg_ti_light_focused_p);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_textinput_showcase_light, &bg_ti_light_params);

// Textblock border: transparent fill, 1px neutral stroke
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_tb_p, EGUI_COLOR_MAKE(0, 0, 0), 0, 4, 1, EGUI_COLOR_MAKE(88, 96, 112), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_tb_params, &bg_tb_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_textblock_showcase, &bg_tb_params);

// Border demo 1: solid fill + 1px accent stroke
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE(bg_border1_dark_p, EGUI_COLOR_MAKE(22, 27, 34), EGUI_ALPHA_100, 1, EGUI_COLOR_MAKE(88, 166, 255),
                                              EGUI_ALPHA_100); // #58A6FF
EGUI_BACKGROUND_PARAM_INIT(bg_border1_dark_params, &bg_border1_dark_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_border1_dark, &bg_border1_dark_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE(bg_border1_light_p, EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100, 1, EGUI_COLOR_MAKE(9, 105, 218),
                                              EGUI_ALPHA_100); // #0969DA
EGUI_BACKGROUND_PARAM_INIT(bg_border1_light_params, &bg_border1_light_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_border1_light, &bg_border1_light_params);

// Border demo 2: rounded + 2px green stroke
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_border2_dark_p, EGUI_COLOR_MAKE(22, 27, 34), EGUI_ALPHA_100, 8, 2, EGUI_COLOR_MAKE(63, 185, 80),
                                                        EGUI_ALPHA_100); // #3FB950
EGUI_BACKGROUND_PARAM_INIT(bg_border2_dark_params, &bg_border2_dark_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_border2_dark, &bg_border2_dark_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_border2_light_p, EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100, 8, 2, EGUI_COLOR_MAKE(63, 185, 80),
                                                        EGUI_ALPHA_100); // #3FB950
EGUI_BACKGROUND_PARAM_INIT(bg_border2_light_params, &bg_border2_light_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_border2_light, &bg_border2_light_params);

// Layer demo: active=blue-tinted  inactive=dim card
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha1_dark_active_p, EGUI_COLOR_MAKE(14, 50, 95), 245, 10, 3, // deep blue #0E3260
                                                        EGUI_COLOR_MAKE(88, 166, 255), EGUI_ALPHA_100);                   // #58A6FF
EGUI_BACKGROUND_PARAM_INIT(bg_alpha1_dark_active_params, &bg_alpha1_dark_active_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha1_dark_active, &bg_alpha1_dark_active_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha1_dark_inactive_p, EGUI_COLOR_MAKE(13, 17, 23), 130, 10, 1, // near-black #0D1117
                                                        EGUI_COLOR_MAKE(33, 38, 45), EGUI_ALPHA_100);                       // #21262D
EGUI_BACKGROUND_PARAM_INIT(bg_alpha1_dark_inactive_params, &bg_alpha1_dark_inactive_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha1_dark_inactive, &bg_alpha1_dark_inactive_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha2_dark_active_p, EGUI_COLOR_MAKE(14, 50, 95), 245, 10, 3, // deep blue #0E3260
                                                        EGUI_COLOR_MAKE(88, 166, 255), EGUI_ALPHA_100);                   // #58A6FF
EGUI_BACKGROUND_PARAM_INIT(bg_alpha2_dark_active_params, &bg_alpha2_dark_active_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha2_dark_active, &bg_alpha2_dark_active_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha2_dark_inactive_p, EGUI_COLOR_MAKE(13, 17, 23), 130, 10, 1, // near-black #0D1117
                                                        EGUI_COLOR_MAKE(33, 38, 45), EGUI_ALPHA_100);                       // #21262D
EGUI_BACKGROUND_PARAM_INIT(bg_alpha2_dark_inactive_params, &bg_alpha2_dark_inactive_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha2_dark_inactive, &bg_alpha2_dark_inactive_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha1_light_active_p, EGUI_COLOR_MAKE(178, 213, 255), 240, 10, 3, // sky blue #B2D5FF
                                                        EGUI_COLOR_MAKE(9, 105, 218), EGUI_ALPHA_100);                        // #0969DA
EGUI_BACKGROUND_PARAM_INIT(bg_alpha1_light_active_params, &bg_alpha1_light_active_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha1_light_active, &bg_alpha1_light_active_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha1_light_inactive_p, EGUI_COLOR_MAKE(246, 248, 250), 150, 10, 1, // pale gray #F6F8FA
                                                        EGUI_COLOR_MAKE(208, 215, 222), EGUI_ALPHA_100);                        // #D0D7DE
EGUI_BACKGROUND_PARAM_INIT(bg_alpha1_light_inactive_params, &bg_alpha1_light_inactive_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha1_light_inactive, &bg_alpha1_light_inactive_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha2_light_active_p, EGUI_COLOR_MAKE(178, 213, 255), 240, 10, 3, // sky blue #B2D5FF
                                                        EGUI_COLOR_MAKE(9, 105, 218), EGUI_ALPHA_100);                        // #0969DA
EGUI_BACKGROUND_PARAM_INIT(bg_alpha2_light_active_params, &bg_alpha2_light_active_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha2_light_active, &bg_alpha2_light_active_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha2_light_inactive_p, EGUI_COLOR_MAKE(246, 248, 250), 150, 10, 1, // pale gray #F6F8FA
                                                        EGUI_COLOR_MAKE(208, 215, 222), EGUI_ALPHA_100);                        // #D0D7DE
EGUI_BACKGROUND_PARAM_INIT(bg_alpha2_light_inactive_params, &bg_alpha2_light_inactive_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha2_light_inactive, &bg_alpha2_light_inactive_params);

// NumberPicker backgrounds
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_numpick_dark_p, EGUI_COLOR_MAKE(22, 27, 34), 210, 8, 1, EGUI_COLOR_MAKE(48, 54, 61), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_numpick_dark_params, &bg_numpick_dark_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_numpick_dark, &bg_numpick_dark_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_numpick_light_p, EGUI_COLOR_MAKE(255, 255, 255), 220, 8, 1, EGUI_COLOR_MAKE(208, 215, 222),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_numpick_light_params, &bg_numpick_light_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_numpick_light, &bg_numpick_light_params);

// ============================================================================
// TextInput focus: show/hide keyboard
// ============================================================================
static void on_textinput_focus_changed(egui_view_t *self, int is_focused)
{
    egui_view_textinput_t *ti = (egui_view_textinput_t *)self;
    if (is_focused)
    {
        ti->cursor_visible = 1;
        egui_timer_start_timer(&ti->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);
        egui_view_set_position(EGUI_VIEW_OF(&wg_keyboard), 0, SHOWCASE_KEYBOARD_Y);
        egui_view_keyboard_show(EGUI_VIEW_OF(&wg_keyboard), self);
    }
    else
    {
        ti->cursor_visible = 0;
        egui_timer_stop_timer(&ti->cursor_timer);
        egui_view_keyboard_hide(EGUI_VIEW_OF(&wg_keyboard));
        egui_view_set_position(EGUI_VIEW_OF(&wg_keyboard), 0, SHOWCASE_KEYBOARD_HIDDEN_Y);
    }
    egui_view_invalidate(self);
}

// ============================================================================
// Theme & Layer Callbacks
// ============================================================================
static void update_theme(void)
{
    if (is_dark_theme)
    {
        egui_view_set_background(EGUI_VIEW_OF(&root), EGUI_BG_OF(&bg_root_dark_new));
        egui_color_t text_color = EGUI_COLOR_MAKE(201, 209, 217); // #C9D1D9
        egui_color_t title_color = EGUI_COLOR_MAKE(88, 166, 255); // #58A6FF

        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_label), text_color, EGUI_ALPHA_100);
        egui_view_textblock_set_font_color(EGUI_VIEW_OF(&wg_textblock), text_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_dynlabel), text_color, EGUI_ALPHA_100);
        egui_view_checkbox_set_text_color(EGUI_VIEW_OF(&wg_checkbox), text_color);
        egui_view_radio_button_set_text_color(EGUI_VIEW_OF(&wg_radio), text_color);
        egui_view_radio_button_set_text_color(EGUI_VIEW_OF(&wg_radio2), text_color);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_dclock), text_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_stopwatch), text_color, EGUI_ALPHA_100);
        // Titles
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_basic), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_toggle), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_progress), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_canvas), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_slider), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_chart), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_time), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_special), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_data), title_color, EGUI_ALPHA_100);

        egui_view_label_set_font_color(EGUI_VIEW_OF(&btn_theme), text_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&btn_lang), text_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_button), text_color, EGUI_ALPHA_100);
        egui_view_toggle_button_set_text_color(EGUI_VIEW_OF(&wg_togbtn), text_color);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_card_child), text_color, EGUI_ALPHA_100);
        wg_table.header_text_color = text_color;
        wg_table.cell_text_color = text_color;
        wg_tabbar.text_color = text_color;
        wg_tabbar.active_text_color = title_color;
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_window.title_label), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_window_lbl), text_color, EGUI_ALPHA_100);
        wg_menu.text_color = text_color;
        egui_view_button_matrix_set_text_color(EGUI_VIEW_OF(&wg_btnmatrix), text_color);
        for (int i = 0; i < wg_list.item_count; i++)
            egui_view_label_set_font_color((egui_view_t *)&wg_list.items[i], text_color, EGUI_ALPHA_100);

        wg_calendar.text_color = text_color;
        wg_calendar.header_color = title_color;
        wg_calendar.weekend_color = text_color;
        wg_calendar.bg_color = EGUI_COLOR_MAKE(22, 27, 34);
        wg_roller.text_color = text_color;
        wg_combobox.text_color = EGUI_COLOR_MAKE(201, 209, 217);
        wg_combobox.bg_color = EGUI_COLOR_MAKE(22, 27, 34);
        wg_scale.label_color = text_color;
        wg_aclock.dial_color = text_color;
        wg_aclock.tick_color = text_color;
        wg_aclock.hour_color = text_color;
        wg_aclock.minute_color = text_color;
        wg_numpick.text_color = text_color;
        wg_numpick.button_color = EGUI_COLOR_MAKE(88, 166, 255);
        wg_compass.dial_color = text_color;
        egui_view_set_background(EGUI_VIEW_OF(&wg_numpick), EGUI_BG_OF(&bg_numpick_dark));
        wg_window.header_color = EGUI_COLOR_MAKE(31, 35, 42);
        wg_window.content_bg_color = EGUI_COLOR_MAKE(22, 27, 34);
        wg_menu.header_color = EGUI_COLOR_MAKE(31, 35, 42);
        wg_menu.item_color = EGUI_COLOR_MAKE(22, 27, 34);
        wg_menu.highlight_color = EGUI_COLOR_MAKE(48, 54, 61);
        egui_view_card_set_bg_color(EGUI_VIEW_OF(&wg_card), EGUI_COLOR_MAKE(30, 40, 55), EGUI_ALPHA_100);

        // Chart theme: (bg, axis, grid, text) -- dark palette
        egui_view_chart_line_set_colors(EGUI_VIEW_OF(&wg_chart_line), EGUI_COLOR_MAKE(18, 22, 30), EGUI_COLOR_MAKE(72, 88, 110), EGUI_COLOR_MAKE(36, 44, 58),
                                        EGUI_COLOR_MAKE(145, 160, 180));
        egui_view_chart_bar_set_colors(EGUI_VIEW_OF(&wg_chart_bar), EGUI_COLOR_MAKE(18, 22, 30), EGUI_COLOR_MAKE(72, 88, 110), EGUI_COLOR_MAKE(36, 44, 58),
                                       EGUI_COLOR_MAKE(145, 160, 180));
        egui_view_chart_pie_set_colors(EGUI_VIEW_OF(&wg_chart_pie), EGUI_COLOR_MAKE(18, 22, 30), EGUI_COLOR_MAKE(145, 160, 180));
        egui_view_chart_scatter_set_colors(EGUI_VIEW_OF(&wg_chart_scatter), EGUI_COLOR_MAKE(18, 22, 30), EGUI_COLOR_MAKE(72, 88, 110),
                                           EGUI_COLOR_MAKE(36, 44, 58), EGUI_COLOR_MAKE(145, 160, 180));

        egui_view_set_background(&wg_cv_gradient, EGUI_BG_OF(&bg_grad_dark));
        egui_view_set_background(&wg_cv_shadow, EGUI_BG_OF(&bg_shadow_dark));
        egui_view_set_shadow(&wg_cv_shadow, &shadow_demo_dark);
        egui_view_set_background(&wg_cv_border1, EGUI_BG_OF(&bg_border1_dark));
        egui_view_set_background(&wg_cv_border2, EGUI_BG_OF(&bg_border2_dark));
        egui_view_line_set_line_color(EGUI_VIEW_OF(&wg_cv_hqline), EGUI_COLOR_MAKE(63, 185, 80)); // #3FB950

        // Panels
        egui_view_set_background(&panel_basic, EGUI_BG_OF(&bg_panel_dark));
        egui_view_set_background(&panel_toggle, EGUI_BG_OF(&bg_panel_dark));
        egui_view_set_background(&panel_progress, EGUI_BG_OF(&bg_panel_dark));
        egui_view_set_background(&panel_canvas, EGUI_BG_OF(&bg_panel_dark));
        egui_view_set_background(&panel_slider, EGUI_BG_OF(&bg_panel_dark));
        egui_view_set_background(&panel_chart, EGUI_BG_OF(&bg_panel_dark));
        egui_view_set_background(&panel_time, EGUI_BG_OF(&bg_panel_dark));
        egui_view_set_background(&panel_special, EGUI_BG_OF(&bg_panel_dark));
        egui_view_set_background(&panel_data, EGUI_BG_OF(&bg_panel_dark));
        // TextInput dark background
        egui_view_set_background(EGUI_VIEW_OF(&wg_textinput), EGUI_BG_OF(&bg_textinput_showcase));
        egui_view_textinput_set_text_color(EGUI_VIEW_OF(&wg_textinput), EGUI_COLOR_MAKE(201, 209, 217), EGUI_ALPHA_100);
        // ActivityRing dark bg colors
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wg_actring), 0, EGUI_COLOR_MAKE(0x3B, 0x15, 0x15));
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wg_actring), 1, EGUI_COLOR_MAKE(0x0A, 0x2E, 0x20));
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wg_actring), 2, EGUI_COLOR_MAKE(0x0E, 0x2F, 0x3E));
        // CircularProgressBar dark bk color
        egui_view_circular_progress_bar_set_bk_color(EGUI_VIEW_OF(&wg_cpbar), EGUI_COLOR_MAKE(33, 38, 45));
        wg_cpbar.progress_color = EGUI_COLOR_MAKE(88, 166, 255); // #58A6FF
        wg_cpbar.text_color = EGUI_COLOR_MAKE(201, 209, 217);    // #C9D1D9
        // ArcSlider dark colors
        wg_arcslider.track_color = EGUI_COLOR_MAKE(33, 38, 45);    // #21262D
        wg_arcslider.active_color = EGUI_COLOR_MAKE(88, 166, 255); // #58A6FF
        wg_arcslider.thumb_color = EGUI_COLOR_WHITE;
        // Gauge dark colors
        wg_gauge.bk_color = EGUI_COLOR_MAKE(33, 38, 45);         // #21262D
        wg_gauge.progress_color = EGUI_COLOR_MAKE(88, 166, 255); // #58A6FF
        wg_gauge.needle_color = EGUI_COLOR_MAKE(255, 123, 114);  // #FF7B72 warm red
        wg_gauge.text_color = EGUI_COLOR_MAKE(201, 209, 217);    // #C9D1D9
        // Table header dark bg
        egui_view_table_set_header_bg_color(EGUI_VIEW_OF(&wg_table), EGUI_COLOR_MAKE(31, 35, 42));
        wg_table.header_text_color = EGUI_COLOR_MAKE(201, 209, 217);
    }
    else
    {
        egui_view_set_background(EGUI_VIEW_OF(&root), EGUI_BG_OF(&bg_root_light_new));
        egui_color_t text_color = EGUI_COLOR_MAKE(36, 41, 47);   // #24292F
        egui_color_t title_color = EGUI_COLOR_MAKE(9, 105, 218); // #0969DA

        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_label), text_color, EGUI_ALPHA_100);
        egui_view_textblock_set_font_color(EGUI_VIEW_OF(&wg_textblock), text_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_dynlabel), text_color, EGUI_ALPHA_100);
        egui_view_checkbox_set_text_color(EGUI_VIEW_OF(&wg_checkbox), text_color);
        egui_view_radio_button_set_text_color(EGUI_VIEW_OF(&wg_radio), text_color);
        egui_view_radio_button_set_text_color(EGUI_VIEW_OF(&wg_radio2), text_color);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_dclock), text_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_stopwatch), text_color, EGUI_ALPHA_100);
        // Titles
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_basic), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_toggle), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_progress), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_canvas), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_slider), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_chart), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_time), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_special), title_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&t_data), title_color, EGUI_ALPHA_100);

        egui_view_label_set_font_color(EGUI_VIEW_OF(&btn_theme), text_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&btn_lang), text_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_button), text_color, EGUI_ALPHA_100);
        egui_view_toggle_button_set_text_color(EGUI_VIEW_OF(&wg_togbtn), text_color);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_card_child), text_color, EGUI_ALPHA_100);
        wg_table.header_text_color = title_color;
        wg_table.cell_text_color = text_color;
        wg_tabbar.text_color = text_color;
        wg_tabbar.active_text_color = title_color;
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_window.title_label), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_window_lbl), text_color, EGUI_ALPHA_100);
        wg_menu.text_color = text_color;
        egui_view_button_matrix_set_text_color(EGUI_VIEW_OF(&wg_btnmatrix), text_color);
        for (int i = 0; i < wg_list.item_count; i++)
            egui_view_label_set_font_color((egui_view_t *)&wg_list.items[i], text_color, EGUI_ALPHA_100);

        wg_calendar.text_color = text_color;
        wg_calendar.header_color = title_color;
        wg_calendar.weekend_color = title_color;
        wg_calendar.bg_color = EGUI_THEME_SURFACE;
        wg_roller.text_color = text_color;
        wg_combobox.text_color = text_color;
        wg_combobox.bg_color = EGUI_THEME_SURFACE;
        wg_scale.label_color = text_color;
        wg_aclock.dial_color = text_color;
        wg_aclock.tick_color = text_color;
        wg_aclock.hour_color = text_color;
        wg_aclock.minute_color = text_color;
        wg_numpick.text_color = text_color;
        wg_numpick.button_color = EGUI_COLOR_MAKE(9, 105, 218);
        wg_compass.dial_color = text_color;
        egui_view_set_background(EGUI_VIEW_OF(&wg_numpick), EGUI_BG_OF(&bg_numpick_light));
        wg_window.header_color = EGUI_COLOR_MAKE(9, 105, 218);
        wg_window.content_bg_color = EGUI_COLOR_MAKE(255, 255, 255);
        wg_menu.header_color = EGUI_COLOR_MAKE(9, 105, 218);
        wg_menu.item_color = EGUI_COLOR_MAKE(255, 255, 255);
        wg_menu.highlight_color = EGUI_COLOR_MAKE(208, 215, 222);
        egui_view_card_set_bg_color(EGUI_VIEW_OF(&wg_card), EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100);

        // Chart theme: (bg, axis, grid, text) -- light palette
        egui_view_chart_line_set_colors(EGUI_VIEW_OF(&wg_chart_line), EGUI_COLOR_MAKE(250, 251, 253), EGUI_COLOR_MAKE(60, 75, 95),
                                        EGUI_COLOR_MAKE(215, 222, 232), EGUI_COLOR_MAKE(45, 55, 70));
        egui_view_chart_bar_set_colors(EGUI_VIEW_OF(&wg_chart_bar), EGUI_COLOR_MAKE(250, 251, 253), EGUI_COLOR_MAKE(60, 75, 95), EGUI_COLOR_MAKE(215, 222, 232),
                                       EGUI_COLOR_MAKE(45, 55, 70));
        egui_view_chart_pie_set_colors(EGUI_VIEW_OF(&wg_chart_pie), EGUI_COLOR_MAKE(250, 251, 253), EGUI_COLOR_MAKE(45, 55, 70));
        egui_view_chart_scatter_set_colors(EGUI_VIEW_OF(&wg_chart_scatter), EGUI_COLOR_MAKE(250, 251, 253), EGUI_COLOR_MAKE(60, 75, 95),
                                           EGUI_COLOR_MAKE(215, 222, 232), EGUI_COLOR_MAKE(45, 55, 70));

        egui_view_set_background(&wg_cv_gradient, EGUI_BG_OF(&bg_grad_light));
        egui_view_set_background(&wg_cv_shadow, EGUI_BG_OF(&bg_shadow_light));
        egui_view_set_shadow(&wg_cv_shadow, &shadow_demo_light);
        egui_view_set_background(&wg_cv_border1, EGUI_BG_OF(&bg_border1_light));
        egui_view_set_background(&wg_cv_border2, EGUI_BG_OF(&bg_border2_light));
        egui_view_line_set_line_color(EGUI_VIEW_OF(&wg_cv_hqline), EGUI_COLOR_MAKE(9, 105, 218)); // #0969DA

        // Panels
        egui_view_set_background(&panel_basic, EGUI_BG_OF(&bg_panel_light));

        egui_view_set_background(&panel_toggle, EGUI_BG_OF(&bg_panel_light));
        egui_view_set_background(&panel_progress, EGUI_BG_OF(&bg_panel_light));
        egui_view_set_background(&panel_canvas, EGUI_BG_OF(&bg_panel_light));
        egui_view_set_background(&panel_slider, EGUI_BG_OF(&bg_panel_light));
        egui_view_set_background(&panel_chart, EGUI_BG_OF(&bg_panel_light));
        egui_view_set_background(&panel_time, EGUI_BG_OF(&bg_panel_light));
        egui_view_set_background(&panel_special, EGUI_BG_OF(&bg_panel_light));
        egui_view_set_background(&panel_data, EGUI_BG_OF(&bg_panel_light));
        // TextInput light background
        egui_view_set_background(EGUI_VIEW_OF(&wg_textinput), EGUI_BG_OF(&bg_textinput_showcase_light));
        egui_view_textinput_set_text_color(EGUI_VIEW_OF(&wg_textinput), EGUI_COLOR_MAKE(36, 41, 47), EGUI_ALPHA_100);
        // ActivityRing light bg colors (desaturated light tint matching each ring fg color)
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wg_actring), 0, EGUI_COLOR_MAKE(0xFD, 0xD5, 0xD5));
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wg_actring), 1, EGUI_COLOR_MAKE(0xC5, 0xF2, 0xDF));
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wg_actring), 2, EGUI_COLOR_MAKE(0xC8, 0xE9, 0xF9));
        // CircularProgressBar light bk color
        egui_view_circular_progress_bar_set_bk_color(EGUI_VIEW_OF(&wg_cpbar), EGUI_COLOR_MAKE(208, 215, 222));
        wg_cpbar.progress_color = EGUI_COLOR_MAKE(9, 105, 218); // #0969DA
        wg_cpbar.text_color = EGUI_COLOR_MAKE(36, 41, 47);      // #24292F
        // ArcSlider light colors
        wg_arcslider.track_color = EGUI_COLOR_MAKE(208, 215, 222); // #D0D7DE
        wg_arcslider.active_color = EGUI_COLOR_MAKE(9, 105, 218);  // #0969DA
        wg_arcslider.thumb_color = EGUI_COLOR_WHITE;
        // Gauge light colors
        wg_gauge.bk_color = EGUI_COLOR_MAKE(208, 215, 222);     // #D0D7DE
        wg_gauge.progress_color = EGUI_COLOR_MAKE(9, 105, 218); // #0969DA
        wg_gauge.needle_color = EGUI_COLOR_MAKE(207, 34, 46);   // #CF2230 danger red
        wg_gauge.text_color = EGUI_COLOR_MAKE(36, 41, 47);      // #24292F
        // Table header light bg: strong blue bg with white text for clear distinction
        egui_view_table_set_header_bg_color(EGUI_VIEW_OF(&wg_table), EGUI_COLOR_MAKE(9, 105, 218));
        wg_table.header_text_color = EGUI_COLOR_WHITE;
    }

    // Update all caption label colors
    {
        egui_color_t cap_color = is_dark_theme ? EGUI_COLOR_MAKE(200, 215, 235) : EGUI_COLOR_MAKE(55, 70, 95);
        for (int i = 0; i < (int)(sizeof(s_captions) / sizeof(s_captions[0])); i++)
        {
            egui_view_label_set_font_color(EGUI_VIEW_OF(s_captions[i]), cap_color, EGUI_ALPHA_100);
        }
    }
    update_layer_visual();
    egui_view_invalidate(EGUI_VIEW_OF(&root));
}

static void update_layer_visual(void)
{
    if (is_dark_theme)
    {
        egui_view_set_background(&wg_cv_alpha1, EGUI_BG_OF((active_layer == 1) ? &bg_alpha1_dark_active : &bg_alpha1_dark_inactive));
        egui_view_set_background(&wg_cv_alpha2, EGUI_BG_OF((active_layer == 2) ? &bg_alpha2_dark_active : &bg_alpha2_dark_inactive));
    }
    else
    {
        egui_view_set_background(&wg_cv_alpha1, EGUI_BG_OF((active_layer == 1) ? &bg_alpha1_light_active : &bg_alpha1_light_inactive));
        egui_view_set_background(&wg_cv_alpha2, EGUI_BG_OF((active_layer == 2) ? &bg_alpha2_light_active : &bg_alpha2_light_inactive));
    }

    if (active_layer == 1)
    {
        egui_view_set_layer(&wg_cv_alpha1, EGUI_VIEW_LAYER_TOP);
        egui_view_set_layer(&wg_cv_alpha2, EGUI_VIEW_LAYER_DEFAULT);
    }
    else
    {
        egui_view_set_layer(&wg_cv_alpha1, EGUI_VIEW_LAYER_DEFAULT);
        egui_view_set_layer(&wg_cv_alpha2, EGUI_VIEW_LAYER_TOP);
    }
    // Force cap labels to the very end of LAYER_TOP so they render above the active card.
    // Toggling DEFAULT → TOP forces re-insertion at the tail of the layer list,
    // ensuring labels are drawn after (on top of) both cards.
    egui_view_set_layer(EGUI_VIEW_OF(&cap_alpha1_lbl), EGUI_VIEW_LAYER_DEFAULT);
    egui_view_set_layer(EGUI_VIEW_OF(&cap_alpha2_lbl), EGUI_VIEW_LAYER_DEFAULT);
    egui_view_set_layer(EGUI_VIEW_OF(&cap_alpha1_lbl), EGUI_VIEW_LAYER_TOP);
    egui_view_set_layer(EGUI_VIEW_OF(&cap_alpha2_lbl), EGUI_VIEW_LAYER_TOP);

    // Update layer caption colors: active = high-contrast on card bg, inactive = dim
    {
        // dark active bg = deep navy (#0E3260) -> white text best contrast
        // light active bg = sky blue (#B2D5FF) -> dark navy text for high contrast
        egui_color_t active_color = is_dark_theme ? EGUI_COLOR_WHITE : EGUI_COLOR_MAKE(14, 50, 95);
        egui_color_t inactive_color = is_dark_theme ? EGUI_COLOR_MAKE(160, 175, 195) : EGUI_COLOR_MAKE(65, 80, 100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&cap_alpha1_lbl), (active_layer == 1) ? active_color : inactive_color, EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&cap_alpha2_lbl), (active_layer == 2) ? active_color : inactive_color, EGUI_ALPHA_100);
    }
}

static void on_theme_click(egui_view_t *view)
{
    egui_focus_manager_clear_focus();
    is_dark_theme = !is_dark_theme;
    // light_mode (sun, U+E518) when dark active; dark_mode (moon, U+E51C) when light active
    egui_view_label_set_text(EGUI_VIEW_OF(&btn_theme), is_dark_theme ? "\xee\x94\x98" : "\xee\x94\x9c");
    update_theme();
    update_language();
}

// i18n helper: select string by is_chinese flag
#define S(en, cn) (is_chinese ? (cn) : (en))

static void update_language(void)
{
    const egui_font_t *tf = is_chinese ? (const egui_font_t *)&egui_res_font_notosanssc_14_4 : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    // Section title fonts + texts
    egui_view_label_set_font(EGUI_VIEW_OF(&t_basic), tf);
    egui_view_label_set_font(EGUI_VIEW_OF(&t_toggle), tf);
    egui_view_label_set_font(EGUI_VIEW_OF(&t_progress), tf);
    egui_view_label_set_font(EGUI_VIEW_OF(&t_canvas), tf);
    egui_view_label_set_font(EGUI_VIEW_OF(&t_slider), tf);
    egui_view_label_set_font(EGUI_VIEW_OF(&t_chart), tf);
    egui_view_label_set_font(EGUI_VIEW_OF(&t_time), tf);
    egui_view_label_set_font(EGUI_VIEW_OF(&t_special), tf);
    egui_view_label_set_font(EGUI_VIEW_OF(&t_data), tf);

    egui_view_label_set_text(EGUI_VIEW_OF(&t_basic), S("Basic", "基础"));
    egui_view_label_set_text(EGUI_VIEW_OF(&t_toggle), S("Toggle", "切换"));
    egui_view_label_set_text(EGUI_VIEW_OF(&t_progress), S("Progress", "进度"));
    egui_view_label_set_text(EGUI_VIEW_OF(&t_canvas), S("Canvas", "画布"));
    egui_view_label_set_text(EGUI_VIEW_OF(&t_slider), S("Slider/Picker", "滑块/选择"));
    egui_view_label_set_text(EGUI_VIEW_OF(&t_chart), S("Chart", "图表"));
    egui_view_label_set_text(EGUI_VIEW_OF(&t_time), S("Time/Date", "时间/日期"));
    egui_view_label_set_text(EGUI_VIEW_OF(&t_special), S("Specialized", "特殊"));
    egui_view_label_set_text(EGUI_VIEW_OF(&t_data), S("Data/Container", "数据/容器"));

    // Basic widget fonts + texts
    egui_view_label_set_font(EGUI_VIEW_OF(&wg_button), tf);
    egui_view_label_set_font(EGUI_VIEW_OF(&wg_label), tf);
    egui_view_label_set_font(EGUI_VIEW_OF(&wg_dynlabel), tf);
    egui_view_label_set_font(EGUI_VIEW_OF(&wg_card_child), tf);
    egui_view_toggle_button_set_font(EGUI_VIEW_OF(&wg_togbtn), tf);

    egui_view_label_set_text(EGUI_VIEW_OF(&wg_button), S("Button", "按钮"));
    egui_view_label_set_text(EGUI_VIEW_OF(&wg_label), S("Label Text", "标签文本"));
    egui_view_dynamic_label_set_text(EGUI_VIEW_OF(&wg_dynlabel), S("Dynamic 42", "动态 42"));
    egui_view_label_set_text(EGUI_VIEW_OF(&wg_card_child), S("Card Widget", "卡片控件"));
    egui_view_toggle_button_set_text(EGUI_VIEW_OF(&wg_togbtn), S("ON", "开"));
    egui_view_textblock_set_font(EGUI_VIEW_OF(&wg_textblock), tf);
    egui_view_textblock_set_text(EGUI_VIEW_OF(&wg_textblock), S("Multi-line\ntext block", "多行\n文本块"));
    egui_view_textinput_set_font(EGUI_VIEW_OF(&wg_textinput), tf);
    egui_view_textinput_set_text(EGUI_VIEW_OF(&wg_textinput), S("Hello", "你好"));
    egui_view_textinput_set_placeholder(EGUI_VIEW_OF(&wg_textinput), S("Type here...", "请输入..."));

    // Checkbox / radio button
    egui_view_checkbox_set_font(EGUI_VIEW_OF(&wg_checkbox), tf);
    egui_view_checkbox_set_text(EGUI_VIEW_OF(&wg_checkbox), S("Checked", "已选中"));
    egui_view_radio_button_set_font(EGUI_VIEW_OF(&wg_radio), tf);
    egui_view_radio_button_set_text(EGUI_VIEW_OF(&wg_radio), S("Option 1", "选项 1"));
    egui_view_radio_button_set_font(EGUI_VIEW_OF(&wg_radio2), tf);
    egui_view_radio_button_set_text(EGUI_VIEW_OF(&wg_radio2), S("Option 2", "选项 2"));

    // Language button: shows target language name
    egui_view_label_set_text(EGUI_VIEW_OF(&btn_lang), is_chinese ? "EN" : "中文");

    // Caption font updates
    for (int i = 0; i < (int)(sizeof(s_captions) / sizeof(s_captions[0])); i++)
    {
        egui_view_label_set_font(EGUI_VIEW_OF(s_captions[i]), tf);
    }
    // Caption text updates
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_label_wg), S("Label", "标签"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_textblock_wg), S("Textblock", "文本块"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_dynlabel_wg), S("DynLabel", "动态标"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_textinput_wg), S("TextInput", "输入框"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_switch), S("Switch", "开关"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_led), S("LED", "LED"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_badge), S("Badge", "徽章"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_cpbar), S("CircProgressBar", "圆进度条"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_gauge), S("Gauge", "仪表盘"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_actring), S("ActivityRing", "活动环"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_pagind), S("PageIndicator", "页码指示"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_gradient), S("Gradient", "渐变"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_shadow), S("Shadow", "阴影"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_border1), S("Stroke 1px", "边框1"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_border2), S("Round 2px", "边框2"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_alpha1_lbl), S("Layer 1", "图层1"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_alpha2_lbl), S("Layer 2", "图层2"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_hqline_lbl), S("HQ Line", "HQ线"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_arcslider), S("ArcSlider", "弧滑块"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_spinner), S("Spinner", "旋转框"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_line_wg), S("Line", "线条"));
    egui_view_label_set_text(EGUI_VIEW_OF(&cap_divider_wg), S("Divider", "分割线"));

    // Dynamic items: roller, combobox, tabbar, menu — switch both data AND font
    wg_roller.font = tf;
    egui_view_combobox_set_font(EGUI_VIEW_OF(&wg_combobox), tf);
    wg_tabbar.font = tf;
    wg_menu.font = tf;
    wg_calendar.font = tf;
    egui_view_mini_calendar_set_weekday_labels(EGUI_VIEW_OF(&wg_calendar), is_chinese ? calendar_weekdays_cn : NULL);

    egui_view_roller_set_items(EGUI_VIEW_OF(&wg_roller), is_chinese ? roller_items_cn : roller_items, 5);
    egui_view_combobox_set_items(EGUI_VIEW_OF(&wg_combobox), is_chinese ? combo_items_cn : combo_items, 3);
    egui_view_tab_bar_set_tabs(EGUI_VIEW_OF(&wg_tabbar), is_chinese ? tab_texts_cn : tab_texts, 3);
    egui_view_menu_set_pages(EGUI_VIEW_OF(&wg_menu), is_chinese ? menu_pages_cn : menu_pages, 1);

    // printf("[lang] is_chinese=%d  font=%s  menu_title=%s  tabbar[0]=%s  roller[0]=%s\n",
    //        is_chinese,
    //        is_chinese ? "notosanssc_14" : "default",
    //        is_chinese ? menu_pages_cn[0].title : menu_pages[0].title,
    //        is_chinese ? tab_texts_cn[0]         : tab_texts[0],
    //        is_chinese ? roller_items_cn[0]       : roller_items[0]);
    fflush(stdout);
}

static void on_lang_click(egui_view_t *view)
{
    egui_focus_manager_clear_focus();
    is_chinese = !is_chinese;
    update_language();
}

static void on_layer_click(egui_view_t *view)
{
    if (view == &wg_cv_alpha1)
    {
        active_layer = 1;
    }
    else if (view == &wg_cv_alpha2)
    {
        active_layer = 2;
    }

    update_layer_visual();
    egui_view_invalidate(EGUI_VIEW_OF(&root));
}

// ============================================================================
// Animation timer callback
// ============================================================================
static void anim_cb(egui_timer_t *timer)
{
    anim_tick++;

    // ProgressBar: 0..90 cycle
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&wg_pbar), (uint8_t)((anim_tick * 2) % 100));

    // CircularProgressBar: 0..100 cycle
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&wg_cpbar), (uint8_t)((anim_tick * 3) % 100));

    // ActivityRing: 3 rings grow
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wg_actring), 0, (uint8_t)((anim_tick * 2) % 100));
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wg_actring), 1, (uint8_t)((anim_tick * 3 + 30) % 100));
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wg_actring), 2, (uint8_t)((anim_tick * 4 + 60) % 100));

    // AnalogClock: tick seconds
    uint8_t sec = anim_tick % 60;
    uint8_t min = (anim_tick / 60) % 60;
    uint8_t hr = (anim_tick / 3600) % 12;
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&wg_aclock), hr, min, sec);

    // HeartRate: animate
    egui_view_heart_rate_set_animate(EGUI_VIEW_OF(&wg_heartrate), 1);

    // Stopwatch: tick
    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&wg_stopwatch), anim_tick * 100);
}

// ============================================================================
// Helper: init a category title label
// ============================================================================
static void init_title(egui_view_label_t *lbl, int x, int y, int w, const char *text)
{
    egui_view_label_init(EGUI_VIEW_OF(lbl));
    egui_view_set_position(EGUI_VIEW_OF(lbl), x, y);
    egui_view_set_size(EGUI_VIEW_OF(lbl), w, 20);
    egui_view_label_set_font(EGUI_VIEW_OF(lbl), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_text(EGUI_VIEW_OF(lbl), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(lbl), EGUI_COLOR_MAKE(120, 124, 120), EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(lbl), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(lbl));
}

// ============================================================================
// Helper: init a small caption label for a widget
// ============================================================================
static void init_caption(egui_view_label_t *lbl, int x, int y, int w, const char *text)
{
    egui_view_label_init(EGUI_VIEW_OF(lbl));
    egui_view_set_position(EGUI_VIEW_OF(lbl), x, y);
    egui_view_set_size(EGUI_VIEW_OF(lbl), w, 20);
    egui_view_label_set_font(EGUI_VIEW_OF(lbl), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_text(EGUI_VIEW_OF(lbl), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(lbl), EGUI_COLOR_MAKE(200, 215, 235), EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(lbl), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(lbl));
}

// ============================================================================
// Main init
// ============================================================================
static void uicode_init_ui(void)
{
    // ---- Root group ----
    egui_view_canvas_panner_init(EGUI_VIEW_OF(&root));
    egui_view_set_position(EGUI_VIEW_OF(&root), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&root), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_canvas_panner_set_canvas_size(EGUI_VIEW_OF(&root), SHOWCASE_CANVAS_WIDTH, SHOWCASE_CANVAS_HEIGHT);
    // Theme background will be set in update_theme

    // ---- Language toggle button (CN font; shows target language name) ----
    egui_view_button_init(EGUI_VIEW_OF(&btn_lang));
    egui_view_set_position(EGUI_VIEW_OF(&btn_lang), 1118, 12);
    egui_view_set_size(EGUI_VIEW_OF(&btn_lang), 60, 44);
    egui_view_label_set_font(EGUI_VIEW_OF(&btn_lang), (const egui_font_t *)&egui_res_font_notosanssc_14_4);
    egui_view_label_set_text(EGUI_VIEW_OF(&btn_lang), "中文");
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_lang), on_lang_click);

    // ---- Theme switcher button (icon-only: sun/moon from Material Symbols) ----
    egui_view_button_init(EGUI_VIEW_OF(&btn_theme));
    egui_view_set_position(EGUI_VIEW_OF(&btn_theme), 1188, 12);
    egui_view_set_size(EGUI_VIEW_OF(&btn_theme), 60, 44);
    egui_view_label_set_font(EGUI_VIEW_OF(&btn_theme), (const egui_font_t *)&egui_res_font_materialicon_20_4);
    egui_view_label_set_text(EGUI_VIEW_OF(&btn_theme), "\xee\x94\x98"); // light_mode icon (U+E518)
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_theme), on_theme_click);
    // NOTE: btn_lang and btn_theme added to root LAST so they render on top

    // ---- Section panels for visual grouping ----
    egui_view_init(&panel_basic);
    egui_view_set_position(&panel_basic, 10, 4);
    egui_view_set_size(&panel_basic, 214, 296);
    egui_view_set_background(&panel_basic, EGUI_BG_OF(&bg_panel_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &panel_basic);

    egui_view_init(&panel_toggle);
    egui_view_set_position(&panel_toggle, 234, 4);
    egui_view_set_size(&panel_toggle, 178, 260);
    egui_view_set_background(&panel_toggle, EGUI_BG_OF(&bg_panel_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &panel_toggle);

    egui_view_init(&panel_progress);
    egui_view_set_position(&panel_progress, 424, 4);
    egui_view_set_size(&panel_progress, 244, 306);
    egui_view_set_background(&panel_progress, EGUI_BG_OF(&bg_panel_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &panel_progress);

    egui_view_init(&panel_canvas);
    egui_view_set_position(&panel_canvas, 674, 4);
    egui_view_set_size(&panel_canvas, 590, 322);
    egui_view_set_background(&panel_canvas, EGUI_BG_OF(&bg_panel_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &panel_canvas);

    egui_view_init(&panel_slider);
    egui_view_set_position(&panel_slider, 10, 336);
    egui_view_set_size(&panel_slider, 388, 240);
    egui_view_set_background(&panel_slider, EGUI_BG_OF(&bg_panel_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &panel_slider);

    egui_view_init(&panel_chart);
    egui_view_set_position(&panel_chart, 424, 336);
    egui_view_set_size(&panel_chart, 840, 280);
    egui_view_set_background(&panel_chart, EGUI_BG_OF(&bg_panel_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &panel_chart);

    egui_view_init(&panel_time);
    egui_view_set_position(&panel_time, 10, 614);
    egui_view_set_size(&panel_time, 350, 292);
    egui_view_set_background(&panel_time, EGUI_BG_OF(&bg_panel_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &panel_time);

    egui_view_init(&panel_special);
    egui_view_set_position(&panel_special, 394, 614);
    egui_view_set_size(&panel_special, 236, 226);
    egui_view_set_background(&panel_special, EGUI_BG_OF(&bg_panel_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &panel_special);

    egui_view_init(&panel_data);
    egui_view_set_position(&panel_data, 634, 614);
    egui_view_set_size(&panel_data, 630, 300);
    egui_view_set_background(&panel_data, EGUI_BG_OF(&bg_panel_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &panel_data);

    // ---- Category titles ----
    init_title(&t_basic, 16, 8, 180, "Basic");
    init_title(&t_toggle, 240, 8, 180, "Toggle");
    init_title(&t_progress, 430, 8, 230, "Progress");
    init_title(&t_canvas, 680, 8, 580, "Canvas");
    init_title(&t_slider, 16, 340, 380, "Slider / Picker");
    init_title(&t_chart, 430, 340, 838, "Chart");
    init_title(&t_time, 16, 618, 340, "Time / Date");
    init_title(&t_special, 400, 618, 220, "Specialized");
    init_title(&t_data, 640, 618, 628, "Data / Container");

    // ==================================================================
    // BASIC CONTROLS
    // ==================================================================

    egui_view_button_init(EGUI_VIEW_OF(&wg_button));
    egui_view_set_position(EGUI_VIEW_OF(&wg_button), 16, 30);
    egui_view_set_size(EGUI_VIEW_OF(&wg_button), 110, 36);
    egui_view_label_set_font(EGUI_VIEW_OF(&wg_button), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_text(EGUI_VIEW_OF(&wg_button), "Button");
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_button));

    egui_view_label_init(EGUI_VIEW_OF(&wg_label));
    egui_view_set_position(EGUI_VIEW_OF(&wg_label), 16, 74);
    egui_view_set_size(EGUI_VIEW_OF(&wg_label), 110, 20);
    egui_view_label_set_font(EGUI_VIEW_OF(&wg_label), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_text(EGUI_VIEW_OF(&wg_label), "Label Text");
    egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_label), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_label));

    egui_view_textblock_init(EGUI_VIEW_OF(&wg_textblock));
    egui_view_set_position(EGUI_VIEW_OF(&wg_textblock), 16, 104);
    egui_view_set_size(EGUI_VIEW_OF(&wg_textblock), 110, 48);
    egui_view_set_padding(EGUI_VIEW_OF(&wg_textblock), 6, 6, 4, 4);
    egui_view_textblock_set_font(EGUI_VIEW_OF(&wg_textblock), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_textblock_set_text(EGUI_VIEW_OF(&wg_textblock), "Multi-line\ntext block");
    egui_view_textblock_set_font_color(EGUI_VIEW_OF(&wg_textblock), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&wg_textblock), EGUI_BG_OF(&bg_textblock_showcase));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_textblock));

    egui_view_dynamic_label_init(EGUI_VIEW_OF(&wg_dynlabel));
    egui_view_set_position(EGUI_VIEW_OF(&wg_dynlabel), 16, 160);
    egui_view_set_size(EGUI_VIEW_OF(&wg_dynlabel), 110, 20);
    egui_view_label_set_font(EGUI_VIEW_OF(&wg_dynlabel), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_dynamic_label_set_text(EGUI_VIEW_OF(&wg_dynlabel), "Dynamic 42");
    egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_dynlabel), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_dynlabel));

    egui_view_card_init(EGUI_VIEW_OF(&wg_card));
    egui_view_set_position(EGUI_VIEW_OF(&wg_card), 16, 192);
    egui_view_set_size(EGUI_VIEW_OF(&wg_card), 130, 52);
    egui_view_label_init(EGUI_VIEW_OF(&wg_card_child));
    egui_view_set_size(EGUI_VIEW_OF(&wg_card_child), 110, 30);
    egui_view_set_padding(EGUI_VIEW_OF(&wg_card_child), 6, 6, 4, 4);
    egui_view_label_set_font(EGUI_VIEW_OF(&wg_card_child), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_text(EGUI_VIEW_OF(&wg_card_child), "Card Widget");
    egui_view_card_add_child(EGUI_VIEW_OF(&wg_card), EGUI_VIEW_OF(&wg_card_child));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_card));

    egui_view_textinput_init(EGUI_VIEW_OF(&wg_textinput));
    egui_view_set_position(EGUI_VIEW_OF(&wg_textinput), 16, 252);
    egui_view_set_size(EGUI_VIEW_OF(&wg_textinput), 110, 30);
    egui_view_set_padding(EGUI_VIEW_OF(&wg_textinput), 6, 6, 4, 4);
    egui_view_set_background(EGUI_VIEW_OF(&wg_textinput), EGUI_BG_OF(&bg_textinput_showcase));
    egui_view_textinput_set_font(EGUI_VIEW_OF(&wg_textinput), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_textinput_set_placeholder(EGUI_VIEW_OF(&wg_textinput), "Type here...");
    egui_view_textinput_set_text(EGUI_VIEW_OF(&wg_textinput), "Hello");
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_textinput));
    // Type annotations for basic panel widgets (right-side labels)
    init_caption(&cap_label_wg, 130, 74, 74, "Label");
    init_caption(&cap_textblock_wg, 130, 120, 74, "Textblock");
    init_caption(&cap_dynlabel_wg, 130, 160, 74, "DynLabel");
    init_caption(&cap_textinput_wg, 130, 257, 74, "TextInput");

    // ==================================================================
    // TOGGLE & SELECTION
    // ==================================================================

    egui_view_switch_init(EGUI_VIEW_OF(&wg_switch));
    egui_view_set_position(EGUI_VIEW_OF(&wg_switch), 240, 30);
    egui_view_set_size(EGUI_VIEW_OF(&wg_switch), 54, 28);
    egui_view_switch_set_checked(EGUI_VIEW_OF(&wg_switch), 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_switch));
    init_caption(&cap_switch, 300, 36, 70, "Switch");

    egui_view_checkbox_init(EGUI_VIEW_OF(&wg_checkbox));
    egui_view_set_position(EGUI_VIEW_OF(&wg_checkbox), 240, 66);
    egui_view_set_size(EGUI_VIEW_OF(&wg_checkbox), 160, 24);
    egui_view_checkbox_set_font(EGUI_VIEW_OF(&wg_checkbox), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_checkbox_set_text(EGUI_VIEW_OF(&wg_checkbox), "Checked");
    egui_view_checkbox_set_checked(EGUI_VIEW_OF(&wg_checkbox), 1);
    egui_view_checkbox_set_text_color(EGUI_VIEW_OF(&wg_checkbox), EGUI_COLOR_WHITE);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_checkbox));

    egui_view_radio_group_init(&wg_radio_grp);
    egui_view_radio_button_init(EGUI_VIEW_OF(&wg_radio));
    egui_view_set_position(EGUI_VIEW_OF(&wg_radio), 240, 98);
    egui_view_set_size(EGUI_VIEW_OF(&wg_radio), 110, 24);
    egui_view_radio_button_set_font(EGUI_VIEW_OF(&wg_radio), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_radio_button_set_text(EGUI_VIEW_OF(&wg_radio), "Option 1");
    egui_view_radio_button_set_checked(EGUI_VIEW_OF(&wg_radio), 1);
    egui_view_radio_button_set_text_color(EGUI_VIEW_OF(&wg_radio), EGUI_COLOR_WHITE);
    egui_view_radio_group_add(&wg_radio_grp, EGUI_VIEW_OF(&wg_radio));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_radio));

    egui_view_radio_button_init(EGUI_VIEW_OF(&wg_radio2));
    egui_view_set_position(EGUI_VIEW_OF(&wg_radio2), 240, 128);
    egui_view_set_size(EGUI_VIEW_OF(&wg_radio2), 110, 24);
    egui_view_radio_button_set_font(EGUI_VIEW_OF(&wg_radio2), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_radio_button_set_text(EGUI_VIEW_OF(&wg_radio2), "Option 2");
    egui_view_radio_button_set_checked(EGUI_VIEW_OF(&wg_radio2), 0);
    egui_view_radio_button_set_text_color(EGUI_VIEW_OF(&wg_radio2), EGUI_COLOR_WHITE);
    egui_view_radio_group_add(&wg_radio_grp, EGUI_VIEW_OF(&wg_radio2));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_radio2));

    egui_view_toggle_button_init(EGUI_VIEW_OF(&wg_togbtn));
    egui_view_set_position(EGUI_VIEW_OF(&wg_togbtn), 240, 160);
    egui_view_set_size(EGUI_VIEW_OF(&wg_togbtn), 110, 34);
    egui_view_toggle_button_set_font(EGUI_VIEW_OF(&wg_togbtn), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_toggle_button_set_text(EGUI_VIEW_OF(&wg_togbtn), "ON");
    egui_view_toggle_button_set_toggled(EGUI_VIEW_OF(&wg_togbtn), 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_togbtn));

    egui_view_led_init(EGUI_VIEW_OF(&wg_led));
    egui_view_set_position(EGUI_VIEW_OF(&wg_led), 240, 206);
    egui_view_set_size(EGUI_VIEW_OF(&wg_led), 26, 26);
    egui_view_led_set_on(EGUI_VIEW_OF(&wg_led));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_led));

    egui_view_notification_badge_init(EGUI_VIEW_OF(&wg_badge));
    egui_view_set_position(EGUI_VIEW_OF(&wg_badge), 298, 210);
    egui_view_set_size(EGUI_VIEW_OF(&wg_badge), 26, 22);
    egui_view_notification_badge_set_count(EGUI_VIEW_OF(&wg_badge), 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_badge));
    init_caption(&cap_led, 240, 234, 36, "LED");
    init_caption(&cap_badge, 294, 234, 50, "Badge");

    // ==================================================================
    // PROGRESS & STATUS
    // ==================================================================

    egui_view_progress_bar_init(EGUI_VIEW_OF(&wg_pbar));
    egui_view_set_position(EGUI_VIEW_OF(&wg_pbar), 430, 30);
    egui_view_set_size(EGUI_VIEW_OF(&wg_pbar), 210, 16);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&wg_pbar), 30);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_pbar));

    egui_view_circular_progress_bar_init(EGUI_VIEW_OF(&wg_cpbar));
    egui_view_set_position(EGUI_VIEW_OF(&wg_cpbar), 430, 56);
    egui_view_set_size(EGUI_VIEW_OF(&wg_cpbar), 90, 90);
    egui_view_circular_progress_bar_set_stroke_width(EGUI_VIEW_OF(&wg_cpbar), 8);
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&wg_cpbar), 50);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_cpbar));

    egui_view_gauge_init(EGUI_VIEW_OF(&wg_gauge));
    egui_view_set_position(EGUI_VIEW_OF(&wg_gauge), 540, 50);
    egui_view_set_size(EGUI_VIEW_OF(&wg_gauge), 120, 120);
    wg_gauge.stroke_width = 12;
    egui_view_gauge_set_value(EGUI_VIEW_OF(&wg_gauge), 65);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_gauge));
    init_caption(&cap_cpbar, 430, 148, 120, "CircProgressBar");
    init_caption(&cap_gauge, 570, 150, 60, "Gauge");

    egui_view_activity_ring_init(EGUI_VIEW_OF(&wg_actring));
    egui_view_set_position(EGUI_VIEW_OF(&wg_actring), 430, 162);
    egui_view_set_size(EGUI_VIEW_OF(&wg_actring), 110, 110);
    egui_view_activity_ring_set_ring_count(EGUI_VIEW_OF(&wg_actring), 3);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wg_actring), 0, 70);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wg_actring), 1, 50);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wg_actring), 2, 30);
    egui_view_activity_ring_set_stroke_width(EGUI_VIEW_OF(&wg_actring), 12);
    egui_view_activity_ring_set_ring_gap(EGUI_VIEW_OF(&wg_actring), 3);
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&wg_actring), 0, EGUI_COLOR_MAKE(0xEF, 0x44, 0x44));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&wg_actring), 1, EGUI_COLOR_MAKE(0x10, 0xB9, 0x81));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&wg_actring), 2, EGUI_COLOR_MAKE(0x38, 0xBD, 0xF8));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wg_actring), 0, EGUI_COLOR_MAKE(0x3B, 0x15, 0x15));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wg_actring), 1, EGUI_COLOR_MAKE(0x0A, 0x2E, 0x20));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wg_actring), 2, EGUI_COLOR_MAKE(0x0E, 0x2F, 0x3E));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_actring));

    egui_view_page_indicator_init(EGUI_VIEW_OF(&wg_pagind));
    egui_view_set_position(EGUI_VIEW_OF(&wg_pagind), 530, 240);
    egui_view_set_size(EGUI_VIEW_OF(&wg_pagind), 130, 18);
    egui_view_page_indicator_set_total_count(EGUI_VIEW_OF(&wg_pagind), 5);
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&wg_pagind), 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_pagind));
    init_caption(&cap_actring, 430, 270, 104, "ActivityRing");
    init_caption(&cap_pagind, 546, 270, 116, "PageIndicator");

    // ==================================================================
    // CANVAS DEMO
    // ==================================================================

    egui_view_init(&wg_cv_gradient);
    egui_view_set_position(&wg_cv_gradient, 680, 30);
    egui_view_set_size(&wg_cv_gradient, 240, 60);
    egui_view_set_background(&wg_cv_gradient, EGUI_BG_OF(&bg_grad_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &wg_cv_gradient);

    egui_view_init(&wg_cv_shadow);
    egui_view_set_position(&wg_cv_shadow, 940, 30);
    egui_view_set_size(&wg_cv_shadow, 120, 80);
    egui_view_set_background(&wg_cv_shadow, EGUI_BG_OF(&bg_shadow_dark));
    egui_view_set_shadow(&wg_cv_shadow, &shadow_demo_dark);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &wg_cv_shadow);

    egui_view_init(&wg_cv_border1);
    egui_view_set_position(&wg_cv_border1, 680, 102);
    egui_view_set_size(&wg_cv_border1, 110, 60);
    egui_view_set_background(&wg_cv_border1, EGUI_BG_OF(&bg_border1_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &wg_cv_border1);

    egui_view_init(&wg_cv_border2);
    egui_view_set_position(&wg_cv_border2, 806, 102);
    egui_view_set_size(&wg_cv_border2, 110, 60);
    egui_view_set_background(&wg_cv_border2, EGUI_BG_OF(&bg_border2_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &wg_cv_border2);

    egui_view_init(&wg_cv_alpha1);
    egui_view_set_position(&wg_cv_alpha1, 680, 174);
    egui_view_set_size(&wg_cv_alpha1, 150, 100);
    egui_view_set_background(&wg_cv_alpha1, EGUI_BG_OF(&bg_alpha1_dark_inactive));
    egui_view_set_clickable(&wg_cv_alpha1, 1);
    egui_view_set_on_click_listener(&wg_cv_alpha1, on_layer_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &wg_cv_alpha1);

    egui_view_init(&wg_cv_alpha2);
    egui_view_set_position(&wg_cv_alpha2, 760, 210);
    egui_view_set_size(&wg_cv_alpha2, 150, 100);
    egui_view_set_background(&wg_cv_alpha2, EGUI_BG_OF(&bg_alpha2_dark_active));
    egui_view_set_clickable(&wg_cv_alpha2, 1);
    egui_view_set_on_click_listener(&wg_cv_alpha2, on_layer_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), &wg_cv_alpha2);

    egui_view_line_init(EGUI_VIEW_OF(&wg_cv_hqline));
    egui_view_set_position(EGUI_VIEW_OF(&wg_cv_hqline), 940, 140);
    egui_view_set_size(EGUI_VIEW_OF(&wg_cv_hqline), 240, 140);
    egui_view_line_set_points(EGUI_VIEW_OF(&wg_cv_hqline), hq_pts, 6);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&wg_cv_hqline), 2);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&wg_cv_hqline), EGUI_COLOR_MAKE(88, 164, 208));
    egui_view_line_set_use_round_cap(EGUI_VIEW_OF(&wg_cv_hqline), 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_cv_hqline));
    // Canvas demo captions (overlay, inside each demo box top-left)
    init_caption(&cap_gradient, 684, 33, 80, "Gradient");
    init_caption(&cap_shadow, 944, 33, 70, "Shadow");
    init_caption(&cap_border1, 685, 105, 84, "Stroke 1px");
    init_caption(&cap_border2, 811, 105, 84, "Round 2px");
    init_caption(&cap_alpha1_lbl, 686, 177, 64, "Layer 1");
    init_caption(&cap_alpha2_lbl, 766, 213, 64, "Layer 2");
    init_caption(&cap_hqline_lbl, 942, 127, 70, "HQ Line");

    // ==================================================================
    // SLIDER & PICKER
    // ==================================================================

    egui_view_slider_init(EGUI_VIEW_OF(&wg_slider));
    egui_view_set_position(EGUI_VIEW_OF(&wg_slider), 16, 362);
    egui_view_set_size(EGUI_VIEW_OF(&wg_slider), 180, 28);
    egui_view_slider_set_value(EGUI_VIEW_OF(&wg_slider), 60);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_slider));

    egui_view_arc_slider_init(EGUI_VIEW_OF(&wg_arcslider));
    egui_view_set_position(EGUI_VIEW_OF(&wg_arcslider), 11, 393);
    egui_view_set_size(EGUI_VIEW_OF(&wg_arcslider), 120, 120);
    wg_arcslider.stroke_width = 12;
    wg_arcslider.thumb_radius = 9;
    egui_view_arc_slider_set_value(EGUI_VIEW_OF(&wg_arcslider), 70);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_arcslider));

    egui_view_number_picker_init(EGUI_VIEW_OF(&wg_numpick));
    egui_view_set_position(EGUI_VIEW_OF(&wg_numpick), 132, 390);
    egui_view_set_size(EGUI_VIEW_OF(&wg_numpick), 80, 120);
    egui_view_number_picker_set_range(EGUI_VIEW_OF(&wg_numpick), 0, 99);
    egui_view_number_picker_set_value(EGUI_VIEW_OF(&wg_numpick), 42);
    egui_view_set_background(EGUI_VIEW_OF(&wg_numpick), EGUI_BG_OF(&bg_numpick_dark));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_numpick));

    egui_view_spinner_init(EGUI_VIEW_OF(&wg_spinner));
    egui_view_set_position(EGUI_VIEW_OF(&wg_spinner), 216, 410);
    egui_view_set_size(EGUI_VIEW_OF(&wg_spinner), 40, 40);
#if !EGUI_SHOWCASE_PARITY_RECORDING
    egui_view_spinner_start(EGUI_VIEW_OF(&wg_spinner));
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_spinner));

    {
        EGUI_VIEW_ROLLER_PARAMS_INIT(rp, 276, 362, 90, 110, roller_items, 5, 1);
        egui_view_roller_init_with_params(EGUI_VIEW_OF(&wg_roller), &rp);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_roller));

    {
        EGUI_VIEW_COMBOBOX_PARAMS_INIT(cp, 16, 520, 150, 30, combo_items, 3, 0);
        egui_view_combobox_init_with_params(EGUI_VIEW_OF(&wg_combobox), &cp);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_combobox));

    {
        EGUI_VIEW_SCALE_PARAMS_INIT(sp, 186, 520, 200, 48, 0, 100, 5);
        egui_view_scale_init_with_params(EGUI_VIEW_OF(&wg_scale), &sp);
        egui_view_scale_set_value(EGUI_VIEW_OF(&wg_scale), 60);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_scale));
    init_caption(&cap_arcslider, 35, 490, 80, "ArcSlider");
    init_caption(&cap_spinner, 216, 452, 60, "Spinner");

    // ==================================================================
    // CHART (black background, subtle grid)
    // ==================================================================

    {
        EGUI_VIEW_CHART_LINE_PARAMS_INIT(p, 430, 362, 400, 120);
        egui_view_chart_line_init_with_params(EGUI_VIEW_OF(&wg_chart_line), &p);
        egui_view_chart_line_set_axis_x(EGUI_VIEW_OF(&wg_chart_line), 0, 100, 20);
        egui_view_chart_line_set_axis_y(EGUI_VIEW_OF(&wg_chart_line), 0, 80, 20);
        egui_view_chart_line_set_series(EGUI_VIEW_OF(&wg_chart_line), cl_ser, 1);
        egui_view_chart_line_set_colors(EGUI_VIEW_OF(&wg_chart_line), EGUI_COLOR_MAKE(18, 22, 30), EGUI_COLOR_MAKE(72, 88, 110), EGUI_COLOR_MAKE(36, 44, 58),
                                        EGUI_COLOR_MAKE(145, 160, 180));
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_chart_line));

    {
        EGUI_VIEW_CHART_BAR_PARAMS_INIT(p, 850, 362, 400, 120);
        egui_view_chart_bar_init_with_params(EGUI_VIEW_OF(&wg_chart_bar), &p);
        egui_view_chart_bar_set_axis_x(EGUI_VIEW_OF(&wg_chart_bar), 0, 4, 1);
        egui_view_chart_bar_set_axis_y(EGUI_VIEW_OF(&wg_chart_bar), 0, 60, 20);
        egui_view_chart_bar_set_series(EGUI_VIEW_OF(&wg_chart_bar), cb_ser, 1);
        egui_view_chart_bar_set_colors(EGUI_VIEW_OF(&wg_chart_bar), EGUI_COLOR_MAKE(18, 22, 30), EGUI_COLOR_MAKE(72, 88, 110), EGUI_COLOR_MAKE(36, 44, 58),
                                       EGUI_COLOR_MAKE(145, 160, 180));
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_chart_bar));

    {
        EGUI_VIEW_CHART_PIE_PARAMS_INIT(p, 430, 492, 260, 120);
        egui_view_chart_pie_init_with_params(EGUI_VIEW_OF(&wg_chart_pie), &p);
        egui_view_chart_pie_set_slices(EGUI_VIEW_OF(&wg_chart_pie), pie_sl, 4);
        egui_view_chart_pie_set_colors(EGUI_VIEW_OF(&wg_chart_pie), EGUI_COLOR_MAKE(18, 22, 30), EGUI_COLOR_MAKE(145, 160, 180));
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_chart_pie));

    {
        EGUI_VIEW_CHART_SCATTER_PARAMS_INIT(p, 710, 492, 540, 120);
        egui_view_chart_scatter_init_with_params(EGUI_VIEW_OF(&wg_chart_scatter), &p);
        egui_view_chart_scatter_set_axis_x(EGUI_VIEW_OF(&wg_chart_scatter), 0, 100, 20);
        egui_view_chart_scatter_set_axis_y(EGUI_VIEW_OF(&wg_chart_scatter), 0, 80, 20);
        egui_view_chart_scatter_set_series(EGUI_VIEW_OF(&wg_chart_scatter), cs_ser, 1);
        egui_view_chart_scatter_set_colors(EGUI_VIEW_OF(&wg_chart_scatter), EGUI_COLOR_MAKE(18, 22, 30), EGUI_COLOR_MAKE(72, 88, 110),
                                           EGUI_COLOR_MAKE(36, 44, 58), EGUI_COLOR_MAKE(145, 160, 180));
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_chart_scatter));
    // Chart captions removed to avoid overlap

    // ==================================================================
    // TIME & DATE
    // ==================================================================

    {
        EGUI_VIEW_ANALOG_CLOCK_PARAMS_INIT(p, 16, 640, 130, 130, 10, 10, 30);
        egui_view_analog_clock_init_with_params(EGUI_VIEW_OF(&wg_aclock), &p);
        egui_view_analog_clock_show_second(EGUI_VIEW_OF(&wg_aclock), 1);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_aclock));

    egui_view_digital_clock_init(EGUI_VIEW_OF(&wg_dclock));
    egui_view_set_position(EGUI_VIEW_OF(&wg_dclock), 160, 640);
    egui_view_set_size(EGUI_VIEW_OF(&wg_dclock), 170, 30);
    egui_view_digital_clock_set_time(EGUI_VIEW_OF(&wg_dclock), 14, 30, 0);
    egui_view_digital_clock_set_colon_blink(EGUI_VIEW_OF(&wg_dclock), 1);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_dclock), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_dclock));

    egui_view_stopwatch_init(EGUI_VIEW_OF(&wg_stopwatch));
    egui_view_set_position(EGUI_VIEW_OF(&wg_stopwatch), 160, 680);
    egui_view_set_size(EGUI_VIEW_OF(&wg_stopwatch), 170, 30);
    egui_view_stopwatch_set_state(EGUI_VIEW_OF(&wg_stopwatch), 1);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&wg_stopwatch), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_stopwatch));

    {
        EGUI_VIEW_MINI_CALENDAR_PARAMS_INIT(p, 160, 720, 180, 150, 2026, 3, 2);
        egui_view_mini_calendar_init_with_params(EGUI_VIEW_OF(&wg_calendar), &p);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_calendar));

    // ==================================================================
    // SPECIALIZED
    // ==================================================================

    {
        EGUI_VIEW_COMPASS_PARAMS_INIT(p, 400, 640, 110, 110, 45);
        egui_view_compass_init_with_params(EGUI_VIEW_OF(&wg_compass), &p);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_compass));

    {
        EGUI_VIEW_HEART_RATE_PARAMS_INIT(p, 400, 762, 110, 68, 72);
        egui_view_heart_rate_init_with_params(EGUI_VIEW_OF(&wg_heartrate), &p);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_heartrate));

    {
        EGUI_VIEW_LINE_PARAMS_INIT(lp, 526, 660, 100, 6, 2, EGUI_COLOR_MAKE(72, 148, 184));
        static const egui_view_line_point_t simple_pts[] = {{0, 3}, {100, 3}};
        egui_view_line_init_with_params(EGUI_VIEW_OF(&wg_line), &lp);
        egui_view_line_set_points(EGUI_VIEW_OF(&wg_line), simple_pts, 2);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_line));

    {
        EGUI_VIEW_DIVIDER_PARAMS_INIT(dp, 556, 672, 2, 80, EGUI_COLOR_MAKE(136, 176, 176));
        egui_view_divider_init_with_params(EGUI_VIEW_OF(&wg_divider), &dp);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_divider));
    init_caption(&cap_line_wg, 526, 668, 60, "Line");
    init_caption(&cap_divider_wg, 562, 678, 60, "Divider");

    // ==================================================================
    // DATA & CONTAINER
    // ==================================================================

    {
        EGUI_VIEW_TABLE_PARAMS_INIT(tp, 640, 640, 200, 96, 3, 3);
        egui_view_table_init_with_params(EGUI_VIEW_OF(&wg_table), &tp);
        egui_view_table_set_header_rows(EGUI_VIEW_OF(&wg_table), 1);
        egui_view_table_set_show_grid(EGUI_VIEW_OF(&wg_table), 1);
        egui_view_table_set_cell(EGUI_VIEW_OF(&wg_table), 0, 0, "Name");
        egui_view_table_set_cell(EGUI_VIEW_OF(&wg_table), 0, 1, "Val");
        egui_view_table_set_cell(EGUI_VIEW_OF(&wg_table), 0, 2, "Unit");
        egui_view_table_set_cell(EGUI_VIEW_OF(&wg_table), 1, 0, "Temp");
        egui_view_table_set_cell(EGUI_VIEW_OF(&wg_table), 1, 1, "25");
        egui_view_table_set_cell(EGUI_VIEW_OF(&wg_table), 1, 2, "C");
        egui_view_table_set_cell(EGUI_VIEW_OF(&wg_table), 2, 0, "Hum");
        egui_view_table_set_cell(EGUI_VIEW_OF(&wg_table), 2, 1, "60");
        egui_view_table_set_cell(EGUI_VIEW_OF(&wg_table), 2, 2, "%");
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_table));

    {
        EGUI_VIEW_BUTTON_MATRIX_PARAMS_INIT(bp, 860, 640, 170, 84, 3, 2);
        egui_view_button_matrix_init_with_params(EGUI_VIEW_OF(&wg_btnmatrix), &bp);
        egui_view_button_matrix_set_labels(EGUI_VIEW_OF(&wg_btnmatrix), bm_labels, 6, 3);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_btnmatrix));

    {
        EGUI_VIEW_TAB_BAR_PARAMS_INIT(tbp, 640, 748, 160, 34, tab_texts, 3);
        egui_view_tab_bar_init_with_params(EGUI_VIEW_OF(&wg_tabbar), &tbp);
        egui_view_tab_bar_set_current_index(EGUI_VIEW_OF(&wg_tabbar), 0);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_tabbar));

    {
        EGUI_VIEW_LIST_PARAMS_INIT(lp, 640, 794, 140, 110, 30);
        egui_view_list_init_with_params(EGUI_VIEW_OF(&wg_list), &lp);
        egui_view_scroll_set_scrollbar_enabled(EGUI_VIEW_OF(&wg_list), 1);
        egui_view_list_add_item(EGUI_VIEW_OF(&wg_list), "Item A");
        egui_view_list_add_item(EGUI_VIEW_OF(&wg_list), "Item B");
        egui_view_list_add_item(EGUI_VIEW_OF(&wg_list), "Item C");
        egui_view_list_add_item(EGUI_VIEW_OF(&wg_list), "Item D");
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_list));

    egui_view_spangroup_init(EGUI_VIEW_OF(&wg_spangrp));
    egui_view_set_position(EGUI_VIEW_OF(&wg_spangrp), 820, 748);
    egui_view_set_size(EGUI_VIEW_OF(&wg_spangrp), 180, 40);
    egui_view_spangroup_add_span(EGUI_VIEW_OF(&wg_spangrp), "Primary ", (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_MAKE(72, 144, 168));
    egui_view_spangroup_add_span(EGUI_VIEW_OF(&wg_spangrp), "Secondary ", (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_MAKE(104, 168, 176));
    egui_view_spangroup_add_span(EGUI_VIEW_OF(&wg_spangrp), "Tertiary", (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_MAKE(120, 184, 192));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_spangrp));

    {
        EGUI_VIEW_WINDOW_PARAMS_INIT(wp, 1060, 640, 150, 96, 20, "Window");
        egui_view_window_init_with_params(EGUI_VIEW_OF(&wg_window), &wp);
        egui_view_label_init(EGUI_VIEW_OF(&wg_window_lbl));
        egui_view_set_size(EGUI_VIEW_OF(&wg_window_lbl), 120, 22);
        egui_view_label_set_font(EGUI_VIEW_OF(&wg_window_lbl), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
        egui_view_label_set_text(EGUI_VIEW_OF(&wg_window_lbl), "Content");
        egui_view_window_add_content(EGUI_VIEW_OF(&wg_window), EGUI_VIEW_OF(&wg_window_lbl));
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_window));

    {
        EGUI_VIEW_MENU_PARAMS_INIT(mp, 1060, 752, 150, 86, 20, 20);
        egui_view_menu_init_with_params(EGUI_VIEW_OF(&wg_menu), &mp);
        egui_view_menu_set_pages(EGUI_VIEW_OF(&wg_menu), menu_pages, 1);
    }
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&wg_menu));
    // Language button and theme button added last so they render on top of all panels
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&btn_lang));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&btn_theme));

    // ==================================================================
    // Start animation timer (100ms period)
    // ==================================================================
#if !EGUI_SHOWCASE_PARITY_RECORDING
    egui_timer_init_timer(&anim_timer, NULL, anim_cb);
    egui_timer_start_timer(&anim_timer, 100, 100);
#endif

    // ==================================================================
    // Attach root to core
    // ==================================================================
    egui_core_add_user_root_view(EGUI_VIEW_OF(&root));

    // Keyboard widget - displayed at bottom of screen, hidden by default
    egui_view_keyboard_init(EGUI_VIEW_OF(&wg_keyboard));
    egui_view_set_position(EGUI_VIEW_OF(&wg_keyboard), 0, SHOWCASE_KEYBOARD_HIDDEN_Y);
    egui_view_set_size(EGUI_VIEW_OF(&wg_keyboard), EGUI_CONFIG_SCEEN_WIDTH, SHOWCASE_KEYBOARD_HEIGHT);
    egui_view_keyboard_set_font(EGUI_VIEW_OF(&wg_keyboard), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&wg_keyboard));

    // Register focus listener on textinput to show/hide keyboard
    static egui_view_api_t wg_textinput_focus_api;
    egui_view_override_api_on_focus_changed(EGUI_VIEW_OF(&wg_textinput), &wg_textinput_focus_api, on_textinput_focus_changed);

    // Default theme setup
    is_dark_theme = 1;
    update_theme();
    update_language();
}

void uicode_create_ui(void)
{
    uicode_init_ui();
}

// ============================================================================
// Recording actions for runtime verification
// ============================================================================
#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
#if EGUI_SHOWCASE_PARITY_RECORDING
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 1000);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &btn_theme, 500);
        return true;
    case 2:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 1000);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &btn_lang, 500);
        return true;
    case 4:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 1000);
        return true;
    default:
        return false;
    }
#else
#if (SHOWCASE_CANVAS_WIDTH > EGUI_CONFIG_SCEEN_WIDTH) || (SHOWCASE_CANVAS_HEIGHT > EGUI_CONFIG_SCEEN_HEIGHT)
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 600);
        return true;
    case 1:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH - 20;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT - 20;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT - 20;
        p_action->steps = 6;
        p_action->interval_ms = 220;
        return true;
    case 2:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 3:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT - 20;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 6;
        p_action->interval_ms = 220;
        return true;
    case 4:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    default:
        return false;
    }
#else
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 1000);
        return true;
    case 1:
        // Click theme toggle
        EGUI_SIM_SET_CLICK_VIEW(p_action, &btn_theme, 500);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 3:
        // Click blue layer
        EGUI_SIM_SET_CLICK_VIEW(p_action, &wg_cv_alpha2, 500);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 5:
        // Click red layer
        EGUI_SIM_SET_CLICK_VIEW(p_action, &wg_cv_alpha1, 500);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 7:
        // Click language button -> switch to Chinese
        EGUI_SIM_SET_CLICK_VIEW(p_action, &btn_lang, 500);
        return true;
    case 8:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    case 9:
        // Swipe UP (finger from y=870 to y=800): scroll list content toward bottom boundary
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = 710;
        p_action->y1 = 870;
        p_action->x2 = 710;
        p_action->y2 = 800;
        p_action->steps = 5;
        p_action->interval_ms = 400;
        return true;
    case 10:
        EGUI_SIM_SET_WAIT(p_action, 400);
        return true;
    case 11:
        // Swipe DOWN fast (finger from y=800 to y=880): fling back toward top boundary - tests clamp
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = 710;
        p_action->y1 = 800;
        p_action->x2 = 710;
        p_action->y2 = 880;
        p_action->steps = 5;
        p_action->interval_ms = 400;
        return true;
    case 12:
        EGUI_SIM_SET_WAIT(p_action, 600);
        return true;
    default:
        return false;
    }
#endif
#endif
}
#endif
