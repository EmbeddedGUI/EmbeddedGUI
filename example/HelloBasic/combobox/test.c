#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 comboboxes: XS / S / M / L
static egui_view_combobox_t combo_xs;
static egui_view_combobox_t combo_s;
static egui_view_combobox_t combo_m;
static egui_view_combobox_t combo_l;
static uint8_t runtime_fail_reported;

// Grid container
static egui_view_gridlayout_t grid;

// Items
static const char *items_xs[] = {"Home", "Cloud", "Lock"};
static const char *item_icons_xs[] = {EGUI_ICON_MS_HOME, EGUI_ICON_MS_CLOUD, EGUI_ICON_MS_LOCK};
static const char *items_s[] = {"Info", "Warn", "Error"};
static const char *item_icons_s[] = {EGUI_ICON_MS_INFO, EGUI_ICON_MS_WARNING, EGUI_ICON_MS_ERROR};
static const char *items_m[] = {"Play", "Pause", "Sync"};
static const char *item_icons_m[] = {EGUI_ICON_MS_PLAY_ARROW, EGUI_ICON_MS_PAUSE, EGUI_ICON_MS_SYNC};
static const char *items_l[] = {"Person", "Camera", "Music", "Settings"};
static const char *item_icons_l[] = {EGUI_ICON_MS_PERSON, EGUI_ICON_MS_CAMERA, EGUI_ICON_MS_MUSIC_NOTE, EGUI_ICON_MS_SETTINGS};

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

#if EGUI_CONFIG_RECORDING_TEST
static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static void get_dropdown_item_center(egui_view_t *view, uint8_t item_index, int *x, int *y)
{
    egui_view_combobox_t *local = (egui_view_combobox_t *)view;

    *x = view->region_screen.location.x + view->region_screen.size.width / 2;
    *y = view->region_screen.location.y + local->collapsed_height + item_index * local->item_height + local->item_height / 2;
}
#endif

void test_init_ui(void)
{
    runtime_fail_reported = 0;

    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all comboboxes
    egui_view_combobox_init_with_params(EGUI_VIEW_OF(&combo_xs), &combo_xs_params);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&combo_xs), on_combo_selected);
    egui_view_combobox_set_item_icons(EGUI_VIEW_OF(&combo_xs), item_icons_xs);
    egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&combo_xs), EGUI_FONT_ICON_MS_16);
    egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&combo_xs), EGUI_ICON_MS_KEYBOARD_ARROW_DOWN, EGUI_ICON_MS_KEYBOARD_ARROW_UP);
    egui_view_combobox_set_icon_text_gap(EGUI_VIEW_OF(&combo_xs), 4);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&combo_xs), 3);

    egui_view_combobox_init_with_params(EGUI_VIEW_OF(&combo_s), &combo_s_params);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&combo_s), on_combo_selected);
    egui_view_combobox_set_item_icons(EGUI_VIEW_OF(&combo_s), item_icons_s);
    egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&combo_s), EGUI_FONT_ICON_MS_20);
    egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&combo_s), EGUI_ICON_MS_KEYBOARD_ARROW_DOWN, EGUI_ICON_MS_KEYBOARD_ARROW_UP);
    egui_view_combobox_set_icon_text_gap(EGUI_VIEW_OF(&combo_s), 4);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&combo_s), 3);

    egui_view_combobox_init_with_params(EGUI_VIEW_OF(&combo_m), &combo_m_params);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&combo_m), on_combo_selected);
    egui_view_combobox_set_item_icons(EGUI_VIEW_OF(&combo_m), item_icons_m);
    egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&combo_m), EGUI_FONT_ICON_MS_20);
    egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&combo_m), EGUI_ICON_MS_EXPAND_MORE, EGUI_ICON_MS_EXPAND_LESS);
    egui_view_combobox_set_icon_text_gap(EGUI_VIEW_OF(&combo_m), 5);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&combo_m), 3);

    egui_view_combobox_init_with_params(EGUI_VIEW_OF(&combo_l), &combo_l_params);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&combo_l), on_combo_selected);
    egui_view_combobox_set_item_icons(EGUI_VIEW_OF(&combo_l), item_icons_l);
    egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&combo_l), EGUI_FONT_ICON_MS_24);
    egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&combo_l), EGUI_ICON_MS_EXPAND_MORE, EGUI_ICON_MS_EXPAND_LESS);
    egui_view_combobox_set_icon_text_gap(EGUI_VIEW_OF(&combo_l), 6);
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
    static int last_action = -1;
    int first_call = action_index != last_action;
    int x = 0;
    int y = 0;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call && egui_view_combobox_get_current_index(EGUI_VIEW_OF(&combo_xs)) != 0)
        {
            report_runtime_failure("combo_xs initial index mismatch");
        }
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &combo_xs, 280);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 4:
        get_dropdown_item_center(EGUI_VIEW_OF(&combo_xs), 1, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 280;
        return true;
    case 5:
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 6:
        if (first_call)
        {
            if (egui_view_combobox_get_current_index(EGUI_VIEW_OF(&combo_xs)) != 1)
            {
                report_runtime_failure("combo_xs selection did not commit");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 7:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &combo_m, 280);
        return true;
    case 8:
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 10:
        get_dropdown_item_center(EGUI_VIEW_OF(&combo_m), 2, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 280;
        return true;
    case 11:
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 12:
        if (first_call)
        {
            if (egui_view_combobox_get_current_index(EGUI_VIEW_OF(&combo_m)) != 2)
            {
                report_runtime_failure("combo_m selection did not commit");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 320);
        return true;
    default:
        return false;
    }
}
#endif
