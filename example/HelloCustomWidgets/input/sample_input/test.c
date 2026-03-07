#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "egui_view_sample_input.h"

static egui_view_sample_input_t sample_input;
static char label_str[] = "Sample Input";

void test_init_ui(void)
{
    egui_view_sample_input_init(EGUI_VIEW_OF(&sample_input), 0, 0, 200, 60);
    egui_view_label_set_text(EGUI_VIEW_OF(&sample_input.label), label_str);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&sample_input));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    return false;
}
#endif
