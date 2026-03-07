#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "egui_view_sample_widget.h"

static egui_view_sample_widget_t sample;

static char sample_str[] = "Sample Widget";

void test_init_ui(void)
{
    egui_view_sample_widget_init(EGUI_VIEW_OF(&sample));
    egui_view_set_size(EGUI_VIEW_OF(&sample), 160, 40);
    egui_view_label_set_text(EGUI_VIEW_OF(&sample), sample_str);
    egui_view_sample_widget_set_border(EGUI_VIEW_OF(&sample), EGUI_COLOR_GREEN, 2);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&sample));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    return false;
}
#endif
