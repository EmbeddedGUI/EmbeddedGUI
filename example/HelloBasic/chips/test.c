#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

static egui_view_gridlayout_t grid;
static egui_view_chips_t chips_view;
static egui_view_label_t status_label;
static char status_text[32];
#if EGUI_CONFIG_RECORDING_TEST
static uint8_t clip_fail_reported = 0;
#endif

static const char *chip_labels[] = {
        "All", "Open", "Close", "Warn", "Error", "Mute",
};

EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 228, 180, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
EGUI_VIEW_CHIPS_PARAMS_INIT(chips_params, 0, 0, 220, 96, chip_labels, 6, 3, 8);
EGUI_VIEW_LABEL_PARAMS_INIT(status_label_params, 0, 0, 220, 28, "", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);

static void update_status_text(uint8_t index)
{
    if (index < 6)
    {
        snprintf(status_text, sizeof(status_text), "Selected: %s", chip_labels[index]);
    }
    else
    {
        snprintf(status_text, sizeof(status_text), "Selected: -");
    }
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
}

static void on_chip_selected(egui_view_t *self, uint8_t index)
{
    (void)self;
    EGUI_LOG_INF("chips: selected index=%d\r\n", index);
    update_status_text(index);
}

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    clip_fail_reported = 0;
#endif
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    egui_view_chips_init_with_params(EGUI_VIEW_OF(&chips_view), &chips_params);
    egui_view_chips_set_on_selected_listener(EGUI_VIEW_OF(&chips_view), on_chip_selected);
    egui_view_chips_set_corner_radius(EGUI_VIEW_OF(&chips_view), 12);
    egui_view_chips_set_bg_color(EGUI_VIEW_OF(&chips_view), EGUI_COLOR_DARK_GREY);
    egui_view_chips_set_selected_bg_color(EGUI_VIEW_OF(&chips_view), EGUI_COLOR_BLUE);
    egui_view_chips_set_border_color(EGUI_VIEW_OF(&chips_view), EGUI_COLOR_LIGHT_GREY);
    egui_view_chips_set_selected_index(EGUI_VIEW_OF(&chips_view), 0);
    egui_view_set_margin_all(EGUI_VIEW_OF(&chips_view), 6);

    egui_view_label_init_with_params(EGUI_VIEW_OF(&status_label), &status_label_params);
    egui_view_set_margin_all(EGUI_VIEW_OF(&status_label), 6);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
    update_status_text(0);

    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&chips_view));
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
    report_if_clipped("chips", EGUI_VIEW_OF(&chips_view));
    report_if_clipped("chips_status", EGUI_VIEW_OF(&status_label));
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    report_if_layout_clipped();
    switch (action_index)
    {
    case 0:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        egui_sim_get_view_pos(&chips_view, 0.18f, 0.25f, &p_action->x1, &p_action->y1);
        p_action->interval_ms = 450;
        return true;
    case 1:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        egui_sim_get_view_pos(&chips_view, 0.52f, 0.75f, &p_action->x1, &p_action->y1);
        p_action->interval_ms = 450;
        return true;
    case 2:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        egui_sim_get_view_pos(&chips_view, 0.84f, 0.75f, &p_action->x1, &p_action->y1);
        p_action->interval_ms = 600;
        return true;
    case 3:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    default:
        return false;
    }
}
#endif
