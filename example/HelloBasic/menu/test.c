#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

static egui_view_menu_t menu_1;

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
    egui_view_menu_init_with_params(EGUI_VIEW_OF(&menu_1), &menu_1_params);
    egui_view_menu_set_pages(EGUI_VIEW_OF(&menu_1), pages, 3);
    egui_view_menu_set_icon_font(EGUI_VIEW_OF(&menu_1), EGUI_FONT_ICON_MS_20);
    egui_view_menu_set_navigation_icons(EGUI_VIEW_OF(&menu_1), EGUI_ICON_MS_ARROW_BACK, EGUI_ICON_MS_ARROW_FORWARD);
    egui_view_menu_set_icon_text_gap(EGUI_VIEW_OF(&menu_1), 10);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&menu_1));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0: // Click "Settings" (item 0, y = 30 + 1 + 0*30 + 15 = 46)
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = 46;
        p_action->interval_ms = 1500;
        return true;
    case 1: // Click back arrow (header area, left side)
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = 15;
        p_action->y1 = 15;
        p_action->interval_ms = 1500;
        return true;
    case 2: // Click "About" (item 1, y = 30 + 1 + 1*30 + 15 = 76)
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = 76;
        p_action->interval_ms = 1500;
        return true;
    case 3: // Click back
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = 15;
        p_action->y1 = 15;
        p_action->interval_ms = 1500;
        return true;
    default:
        return false;
    }
}
#endif
