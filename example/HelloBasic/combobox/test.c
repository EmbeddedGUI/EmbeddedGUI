#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 comboboxes: XS / S / M / L
static egui_view_combobox_t combo_xs;
static egui_view_combobox_t combo_s;
static egui_view_combobox_t combo_m;
static egui_view_combobox_t combo_l;

// Grid container
static egui_view_gridlayout_t grid;

// Items
static const char *items_xs[] = {"A", "B", "C"};
static const char *items_s[] = {"Red", "Green", "Blue"};
static const char *items_m[] = {"Small", "Medium", "Large"};
static const char *items_l[] = {"Option 1", "Option 2", "Option 3", "Option 4"};

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

// Size XS (120x28)
EGUI_VIEW_COMBOBOX_PARAMS_INIT(combo_xs_params, 0, 0, 120, 28, items_xs, 3, 0);
// Size S (140x32)
EGUI_VIEW_COMBOBOX_PARAMS_INIT(combo_s_params, 0, 0, 140, 32, items_s, 3, 0);
// Size M (170x38)
EGUI_VIEW_COMBOBOX_PARAMS_INIT(combo_m_params, 0, 0, 170, 38, items_m, 3, 0);
// Size L (200x44)
EGUI_VIEW_COMBOBOX_PARAMS_INIT(combo_l_params, 0, 0, 200, 44, items_l, 4, 0);

static void on_combo_selected(egui_view_t *self, uint8_t index)
{
    EGUI_LOG_INF("Combobox selected index: %d\r\n", index);
}

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all comboboxes
    egui_view_combobox_init_with_params(EGUI_VIEW_OF(&combo_xs), &combo_xs_params);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&combo_xs), on_combo_selected);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&combo_xs), 3);

    egui_view_combobox_init_with_params(EGUI_VIEW_OF(&combo_s), &combo_s_params);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&combo_s), on_combo_selected);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&combo_s), 3);

    egui_view_combobox_init_with_params(EGUI_VIEW_OF(&combo_m), &combo_m_params);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&combo_m), on_combo_selected);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&combo_m), 3);

    egui_view_combobox_init_with_params(EGUI_VIEW_OF(&combo_l), &combo_l_params);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&combo_l), on_combo_selected);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&combo_l), 4);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&combo_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&combo_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&combo_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&combo_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&combo_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&combo_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&combo_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&combo_l));

    // Layout grid children
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    // Add grid to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));

    // Center grid on screen
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
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
        // Click to expand combo_m
        p_action->type = EGUI_SIM_ACTION_CLICK;
        egui_sim_get_view_pos(&combo_m, 0.5f, 0.5f, &p_action->x1, &p_action->y1);
        p_action->interval_ms = 500;
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 1000);
        return true;
    default:
        return false;
    }
}
#endif
