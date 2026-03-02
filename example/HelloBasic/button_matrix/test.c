#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

static egui_view_button_matrix_t btn_matrix;

static const char *btn_labels[] = {
        "7", "8", "9", "/", "4", "5", "6", "*", "1", "2", "3", "-", "C", "0", "=", "+",
};

EGUI_VIEW_BUTTON_MATRIX_PARAMS_INIT(btn_matrix_params, 10, 10, 220, 220, 4, 4);

static void on_btn_click(egui_view_t *self, uint8_t btn_index)
{
    EGUI_LOG_INF("ButtonMatrix clicked: index=%d, label=%s\n", btn_index, btn_labels[btn_index]);
}

void test_init_ui(void)
{
    egui_view_button_matrix_init_with_params(EGUI_VIEW_OF(&btn_matrix), &btn_matrix_params);
    egui_view_button_matrix_set_labels(EGUI_VIEW_OF(&btn_matrix), btn_labels, 16, 4);
    egui_view_button_matrix_set_on_click(EGUI_VIEW_OF(&btn_matrix), on_btn_click);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&btn_matrix));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &btn_matrix, 1000);
        return true;
    default:
        return false;
    }
}
#endif
