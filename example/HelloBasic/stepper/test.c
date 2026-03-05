#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

static egui_view_gridlayout_t grid;
static egui_view_stepper_t stepper_view;
static egui_view_button_t next_button;
static egui_view_label_t status_label;
static char status_text[48];
#if EGUI_CONFIG_RECORDING_TEST
static uint8_t clip_fail_reported = 0;
#endif

EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 220, 180, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
EGUI_VIEW_STEPPER_PARAMS_INIT(stepper_params, 0, 0, 180, 28, 5, 0);
EGUI_VIEW_LABEL_PARAMS_INIT(next_button_params, 0, 0, 120, 36, "Next", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(status_label_params, 0, 0, 210, 24, "", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);

static void update_status(uint8_t current)
{
    snprintf(status_text, sizeof(status_text), "Stage %u / 5  (tap Next)", (unsigned int)(current + 1U));
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    if (current >= 4)
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    }
    else
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
    }
}

static void on_next_click(egui_view_t *self)
{
    (void)self;
    uint8_t current = egui_view_stepper_get_current_step(EGUI_VIEW_OF(&stepper_view));
    current++;
    if (current >= 5)
    {
        current = 0;
    }
    EGUI_LOG_INF("stepper: next -> %d\r\n", current);
    egui_view_stepper_set_current_step(EGUI_VIEW_OF(&stepper_view), current);
    update_status(current);
}

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    clip_fail_reported = 0;
#endif
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    egui_view_stepper_init_with_params(EGUI_VIEW_OF(&stepper_view), &stepper_params);
    egui_view_set_margin_all(EGUI_VIEW_OF(&stepper_view), 8);

    egui_view_button_init_with_params(EGUI_VIEW_OF(&next_button), &next_button_params);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&next_button), on_next_click);
    egui_view_set_margin_all(EGUI_VIEW_OF(&next_button), 8);

    egui_view_label_init_with_params(EGUI_VIEW_OF(&status_label), &status_label_params);
    egui_view_set_margin_all(EGUI_VIEW_OF(&status_label), 8);
    update_status(0);

    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&stepper_view));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&next_button));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&status_label));
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
static void report_if_clipped(const char *name, egui_view_t *view)
{
    int clipped_w = (view->region_screen.size.width < view->region.size.width);
    int clipped_h = (view->region_screen.size.height < view->region.size.height);
    if (!clipped_w && !clipped_h)
    {
        return;
    }

    clip_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s clipped: screen(%d,%d) logic(%d,%d)\n", name, (int)view->region_screen.size.width, (int)view->region_screen.size.height,
           (int)view->region.size.width, (int)view->region.size.height);
}

static void report_if_layout_clipped(void)
{
    if (clip_fail_reported)
    {
        return;
    }
    report_if_clipped("stepper", EGUI_VIEW_OF(&stepper_view));
    report_if_clipped("stepper_next", EGUI_VIEW_OF(&next_button));
    report_if_clipped("stepper_status", EGUI_VIEW_OF(&status_label));
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    report_if_layout_clipped();
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &next_button, 500);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &next_button, 500);
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &next_button, 500);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &next_button, 500);
        return true;
    case 4:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &next_button, 500);
        return true;
    case 5:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    default:
        return false;
    }
}
#endif
