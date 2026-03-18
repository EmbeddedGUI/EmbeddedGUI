#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

static egui_view_menu_t menu_1;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

// Define menu items for each page
static const egui_view_menu_item_t main_items[] = {
        {"Settings", 1, EGUI_ICON_MS_SETTINGS},
        {"About", 2, EGUI_ICON_MS_INFO},
        {"Exit", EGUI_VIEW_MENU_ITEM_LEAF, EGUI_ICON_MS_CLOSE},
};

static const egui_view_menu_item_t settings_items[] = {
        {"Display", EGUI_VIEW_MENU_ITEM_LEAF, EGUI_ICON_MS_VISIBILITY},
        {"Sound", EGUI_VIEW_MENU_ITEM_LEAF, EGUI_ICON_MS_MUSIC_NOTE},
        {"Network", EGUI_VIEW_MENU_ITEM_LEAF, EGUI_ICON_MS_WIFI},
};

static const egui_view_menu_item_t about_items[] = {
        {"Version", EGUI_VIEW_MENU_ITEM_LEAF, EGUI_ICON_MS_INFO},
        {"License", EGUI_VIEW_MENU_ITEM_LEAF, EGUI_ICON_MS_SECURITY},
};

// Define pages
static const egui_view_menu_page_t pages[] = {
        {"Main Menu", main_items, 3},
        {"Settings", settings_items, 3},
        {"About", about_items, 2},
};

EGUI_VIEW_MENU_PARAMS_INIT(menu_1_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, 30, 30);

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
#endif

    egui_view_menu_init_with_params(EGUI_VIEW_OF(&menu_1), &menu_1_params);
    egui_view_menu_set_pages(EGUI_VIEW_OF(&menu_1), pages, 3);
    egui_view_menu_set_icon_font(EGUI_VIEW_OF(&menu_1), EGUI_FONT_ICON_MS_20);
    egui_view_menu_set_navigation_icons(EGUI_VIEW_OF(&menu_1), EGUI_ICON_MS_ARROW_BACK, EGUI_ICON_MS_ARROW_FORWARD);
    egui_view_menu_set_icon_text_gap(EGUI_VIEW_OF(&menu_1), 10);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&menu_1));
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

static void set_menu_item_action(egui_sim_action_t *p_action, uint8_t item_index, uint32_t interval_ms)
{
    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
    p_action->y1 = menu_1.header_height + 1 + item_index * menu_1.item_height + menu_1.item_height / 2;
    p_action->interval_ms = interval_ms;
}

static void set_menu_back_action(egui_sim_action_t *p_action, uint32_t interval_ms)
{
    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = menu_1.header_height / 2;
    p_action->y1 = menu_1.header_height / 2;
    p_action->interval_ms = interval_ms;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call && (menu_1.current_page != 0 || menu_1.stack_depth != 0))
        {
            report_runtime_failure("menu initial state mismatch");
        }
        set_menu_item_action(p_action, 0, 320);
        return true;
    case 1:
        if (first_call && (menu_1.current_page != 1 || menu_1.stack_depth != 1))
        {
            report_runtime_failure("menu did not enter Settings page");
        }
        set_menu_back_action(p_action, 320);
        return true;
    case 2:
        if (first_call && (menu_1.current_page != 0 || menu_1.stack_depth != 0))
        {
            report_runtime_failure("menu did not return to Main page");
        }
        set_menu_item_action(p_action, 1, 320);
        return true;
    case 3:
        if (first_call && (menu_1.current_page != 2 || menu_1.stack_depth != 1))
        {
            report_runtime_failure("menu did not enter About page");
        }
        set_menu_back_action(p_action, 320);
        return true;
    case 4:
        if (first_call)
        {
            if (menu_1.current_page != 0 || menu_1.stack_depth != 0)
            {
                report_runtime_failure("menu final state mismatch after back navigation");
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
