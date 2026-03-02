#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

static egui_view_table_t table;

EGUI_VIEW_TABLE_PARAMS_INIT(table_params, 0, 0, 220, 80, 4, 3);

void test_init_ui(void)
{
    egui_view_table_init_with_params(EGUI_VIEW_OF(&table), &table_params);

    // Header row
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 0, 0, "Name");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 0, 1, "Age");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 0, 2, "City");

    // Data rows
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 1, 0, "Alice");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 1, 1, "30");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 1, 2, "NYC");

    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 2, 0, "Bob");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 2, 1, "25");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 2, 2, "LA");

    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 3, 0, "Carol");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 3, 1, "28");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 3, 2, "SF");

    egui_core_add_user_root_view(EGUI_VIEW_OF(&table));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
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
