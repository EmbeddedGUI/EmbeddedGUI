#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

static egui_view_list_t list_1;

EGUI_VIEW_LIST_PARAMS_INIT(list_1_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, 34);

static void list_item_click_cb(egui_view_t *self, uint8_t index)
{
    EGUI_LOG_INF("List item %d clicked\n", index);
}

void test_init_ui(void)
{
    // Init list
    egui_view_list_init_with_params(EGUI_VIEW_OF(&list_1), &list_1_params);
    egui_view_scroll_set_scrollbar_enabled(EGUI_VIEW_OF(&list_1), 1);
    egui_view_list_set_icon_font(EGUI_VIEW_OF(&list_1), EGUI_FONT_ICON_MS_20);
    egui_view_list_set_icon_color(EGUI_VIEW_OF(&list_1), EGUI_COLOR_WHITE);
    egui_view_list_set_icon_text_gap(EGUI_VIEW_OF(&list_1), 10);
    egui_view_list_set_on_item_click(EGUI_VIEW_OF(&list_1), list_item_click_cb);

    // Add items
    egui_view_list_add_item_with_icon(EGUI_VIEW_OF(&list_1), EGUI_ICON_MS_HOME, "Overview");
    egui_view_list_add_item_with_icon(EGUI_VIEW_OF(&list_1), EGUI_ICON_MS_NOTIFICATIONS, "Notifications");
    egui_view_list_add_item_with_icon(EGUI_VIEW_OF(&list_1), EGUI_ICON_MS_VISIBILITY, "Display");
    egui_view_list_add_item_with_icon(EGUI_VIEW_OF(&list_1), EGUI_ICON_MS_MUSIC_NOTE, "Sound");
    egui_view_list_add_item_with_icon(EGUI_VIEW_OF(&list_1), EGUI_ICON_MS_WIFI, "Connectivity");
    egui_view_list_add_item_with_icon(EGUI_VIEW_OF(&list_1), EGUI_ICON_MS_LOCK, "Privacy");
    egui_view_list_add_item_with_icon(EGUI_VIEW_OF(&list_1), EGUI_ICON_MS_CLOUD, "Storage");
    egui_view_list_add_item_with_icon(EGUI_VIEW_OF(&list_1), EGUI_ICON_MS_PERSON, "Accessibility");
    egui_view_list_add_item_with_icon(EGUI_VIEW_OF(&list_1), EGUI_ICON_MS_INFO, "About Device");
    egui_view_list_add_item_with_icon(EGUI_VIEW_OF(&list_1), EGUI_ICON_MS_HELP, "Support");

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&list_1));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0: // swipe up to scroll down
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 4;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 4;
        p_action->steps = 5;
        p_action->interval_ms = 1500;
        return true;
    case 1: // swipe down to scroll back
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 4;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 4;
        p_action->steps = 5;
        p_action->interval_ms = 1500;
        return true;
    default:
        return false;
    }
}
#endif
