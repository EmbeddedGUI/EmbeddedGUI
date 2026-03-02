#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

static egui_view_mini_calendar_t calendar;

EGUI_VIEW_MINI_CALENDAR_PARAMS_INIT(calendar_params, 10, 10, 220, 220, 2026, 2, 22);

void test_init_ui(void)
{
    egui_view_mini_calendar_init_with_params(EGUI_VIEW_OF(&calendar), &calendar_params);
    egui_view_mini_calendar_set_today(EGUI_VIEW_OF(&calendar), 22);
    egui_view_mini_calendar_set_first_day_of_week(EGUI_VIEW_OF(&calendar), 1);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&calendar));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 1)
    {
        return false;
    }
    EGUI_SIM_SET_WAIT(p_action, 1500);
    return true;
}
#endif
