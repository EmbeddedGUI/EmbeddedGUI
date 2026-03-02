#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// ============================================================================
// Enhanced Widgets Demo
// Showcases gradient-enhanced rendering for all basic widgets.
// Requires EGUI_CONFIG_WIDGET_ENHANCED_DRAW=1 in app_egui_config.h
// ============================================================================

// Button
static egui_view_button_t btn;
static char btn_text[] = "Enhanced Btn";

// Switch (on + off)
static egui_view_switch_t sw_on;
static egui_view_switch_t sw_off;

// Slider
static egui_view_slider_t slider;

// ProgressBar
static egui_view_progress_bar_t pbar;

// Checkbox (checked + unchecked)
static egui_view_checkbox_t cb_on;
static egui_view_checkbox_t cb_off;

// Card with label
static egui_view_card_t card;
static egui_view_label_t card_label;
static char card_text[] = "Card Widget";

// Phase 2 widgets
static egui_view_radio_button_t rb_on;
static egui_view_radio_button_t rb_off;
static egui_view_radio_group_t rb_group;
static egui_view_toggle_button_t tb_on;
static egui_view_toggle_button_t tb_off;
static egui_view_led_t led_on;
static egui_view_led_t led_off;
static egui_view_notification_badge_t badge;
static egui_view_page_indicator_t indicator;

// Scroll + Grid layout
static egui_view_scroll_t scroll;
static egui_view_gridlayout_t grid;
EGUI_VIEW_SCROLL_PARAMS_INIT(scroll_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, 520, 1, EGUI_ALIGN_CENTER);

void test_init_ui(void)
{
    egui_view_scroll_init_with_params(EGUI_VIEW_OF(&scroll), &scroll_params);
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Button
    egui_view_button_init(EGUI_VIEW_OF(&btn));
    egui_view_set_size(EGUI_VIEW_OF(&btn), 160, 38);
    egui_view_label_set_text(EGUI_VIEW_OF(&btn), btn_text);
    egui_view_set_margin_all(EGUI_VIEW_OF(&btn), 3);

    // Switch ON
    egui_view_switch_init(EGUI_VIEW_OF(&sw_on));
    egui_view_set_size(EGUI_VIEW_OF(&sw_on), 50, 26);
    egui_view_switch_set_checked(EGUI_VIEW_OF(&sw_on), 1);
    egui_view_set_margin_all(EGUI_VIEW_OF(&sw_on), 3);

    // Switch OFF
    egui_view_switch_init(EGUI_VIEW_OF(&sw_off));
    egui_view_set_size(EGUI_VIEW_OF(&sw_off), 50, 26);
    egui_view_set_margin_all(EGUI_VIEW_OF(&sw_off), 3);

    // Slider
    egui_view_slider_init(EGUI_VIEW_OF(&slider));
    egui_view_set_size(EGUI_VIEW_OF(&slider), 160, 26);
    egui_view_slider_set_value(EGUI_VIEW_OF(&slider), 60);
    egui_view_set_margin_all(EGUI_VIEW_OF(&slider), 3);

    // ProgressBar
    egui_view_progress_bar_init(EGUI_VIEW_OF(&pbar));
    egui_view_set_size(EGUI_VIEW_OF(&pbar), 160, 12);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&pbar), 70);
    egui_view_set_margin_all(EGUI_VIEW_OF(&pbar), 3);

    // Checkbox checked
    egui_view_checkbox_init(EGUI_VIEW_OF(&cb_on));
    egui_view_set_size(EGUI_VIEW_OF(&cb_on), 120, 20);
    egui_view_checkbox_set_text(EGUI_VIEW_OF(&cb_on), "Checked");
    egui_view_checkbox_set_checked(EGUI_VIEW_OF(&cb_on), 1);
    egui_view_set_margin_all(EGUI_VIEW_OF(&cb_on), 3);

    // Checkbox unchecked
    egui_view_checkbox_init(EGUI_VIEW_OF(&cb_off));
    egui_view_set_size(EGUI_VIEW_OF(&cb_off), 120, 20);
    egui_view_checkbox_set_text(EGUI_VIEW_OF(&cb_off), "Unchecked");
    egui_view_set_margin_all(EGUI_VIEW_OF(&cb_off), 3);

    // Card
    egui_view_card_init(EGUI_VIEW_OF(&card));
    egui_view_set_size(EGUI_VIEW_OF(&card), 160, 50);
    egui_view_set_margin_all(EGUI_VIEW_OF(&card), 3);
    egui_view_label_init(EGUI_VIEW_OF(&card_label));
    egui_view_set_size(EGUI_VIEW_OF(&card_label), 140, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&card_label), card_text);
    egui_view_card_add_child(EGUI_VIEW_OF(&card), EGUI_VIEW_OF(&card_label));

    // Radio buttons
    egui_view_radio_group_init(&rb_group);
    egui_view_radio_button_init(EGUI_VIEW_OF(&rb_on));
    egui_view_set_size(EGUI_VIEW_OF(&rb_on), 120, 20);
    egui_view_radio_button_set_text(EGUI_VIEW_OF(&rb_on), "Selected");
    egui_view_radio_button_set_checked(EGUI_VIEW_OF(&rb_on), 1);
    egui_view_set_margin_all(EGUI_VIEW_OF(&rb_on), 3);
    egui_view_radio_group_add(&rb_group, EGUI_VIEW_OF(&rb_on));

    egui_view_radio_button_init(EGUI_VIEW_OF(&rb_off));
    egui_view_set_size(EGUI_VIEW_OF(&rb_off), 120, 20);
    egui_view_radio_button_set_text(EGUI_VIEW_OF(&rb_off), "Option B");
    egui_view_set_margin_all(EGUI_VIEW_OF(&rb_off), 3);
    egui_view_radio_group_add(&rb_group, EGUI_VIEW_OF(&rb_off));

    // Toggle buttons
    egui_view_toggle_button_init(EGUI_VIEW_OF(&tb_on));
    egui_view_set_size(EGUI_VIEW_OF(&tb_on), 100, 32);
    egui_view_toggle_button_set_text(EGUI_VIEW_OF(&tb_on), "ON");
    egui_view_toggle_button_set_toggled(EGUI_VIEW_OF(&tb_on), 1);
    egui_view_set_margin_all(EGUI_VIEW_OF(&tb_on), 3);

    egui_view_toggle_button_init(EGUI_VIEW_OF(&tb_off));
    egui_view_set_size(EGUI_VIEW_OF(&tb_off), 100, 32);
    egui_view_toggle_button_set_text(EGUI_VIEW_OF(&tb_off), "OFF");
    egui_view_set_margin_all(EGUI_VIEW_OF(&tb_off), 3);

    // LEDs
    egui_view_led_init(EGUI_VIEW_OF(&led_on));
    egui_view_set_size(EGUI_VIEW_OF(&led_on), 24, 24);
    egui_view_led_set_on(EGUI_VIEW_OF(&led_on));
    egui_view_set_margin_all(EGUI_VIEW_OF(&led_on), 3);

    egui_view_led_init(EGUI_VIEW_OF(&led_off));
    egui_view_set_size(EGUI_VIEW_OF(&led_off), 24, 24);
    egui_view_set_margin_all(EGUI_VIEW_OF(&led_off), 3);

    // Notification badge
    egui_view_notification_badge_init(EGUI_VIEW_OF(&badge));
    egui_view_set_size(EGUI_VIEW_OF(&badge), 24, 20);
    egui_view_notification_badge_set_count(EGUI_VIEW_OF(&badge), 5);
    egui_view_set_margin_all(EGUI_VIEW_OF(&badge), 3);

    // Page indicator
    egui_view_page_indicator_init(EGUI_VIEW_OF(&indicator));
    egui_view_set_size(EGUI_VIEW_OF(&indicator), 120, 16);
    egui_view_page_indicator_set_total_count(EGUI_VIEW_OF(&indicator), 5);
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&indicator), 2);
    egui_view_set_margin_all(EGUI_VIEW_OF(&indicator), 3);

    // Add all to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&btn));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&sw_on));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&sw_off));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&slider));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&pbar));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&cb_on));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&cb_off));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&card));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&rb_on));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&rb_off));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&tb_on));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&tb_off));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&led_on));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&led_off));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&badge));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&indicator));

    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));
    egui_view_scroll_add_child(EGUI_VIEW_OF(&scroll), EGUI_VIEW_OF(&grid));
    egui_view_scroll_layout_childs(EGUI_VIEW_OF(&scroll));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&scroll));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 1:
        // Drag to scroll down and reveal Phase 2 widgets
        p_action->type = EGUI_SIM_ACTION_DRAG;
        p_action->x1 = 120;
        p_action->y1 = 250;
        p_action->x2 = 120;
        p_action->y2 = 50;
        p_action->steps = 10;
        p_action->interval_ms = 300;
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    default:
        return false;
    }
}
#endif
