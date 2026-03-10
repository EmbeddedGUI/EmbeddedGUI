#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "egui_view_sample_chart.h"

static egui_view_sample_chart_t widget;

void test_init_ui(void)
{
    egui_view_sample_chart_init(EGUI_VIEW_OF(&widget));
    egui_view_set_size(EGUI_VIEW_OF(&widget), 280, 180);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&widget));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    return false;
}
#endif
