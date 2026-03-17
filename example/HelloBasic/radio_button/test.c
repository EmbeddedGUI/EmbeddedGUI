#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

static egui_view_radio_button_t radio_home;
static egui_view_radio_button_t radio_alerts;
static egui_view_radio_button_t radio_privacy;
static egui_view_radio_button_t radio_settings;
static egui_view_radio_group_t radio_group;

static egui_view_gridlayout_t grid;

EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(radio_home_params, 0, 0, 176, 30, 1, "Home");
EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(radio_alerts_params, 0, 0, 188, 36, 0, "Notifications");
EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(radio_privacy_params, 0, 0, 196, 42, 0, "Privacy");
EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(radio_settings_params, 0, 0, 204, 50, 0, "Settings");

static void radio_changed_cb(egui_view_t *self, int index)
{
    EGUI_LOG_INF("Radio selected index: %d\n", index);
}

void test_init_ui(void)
{
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    egui_view_radio_group_init(&radio_group);
    egui_view_radio_group_set_on_changed_listener(&radio_group, radio_changed_cb);

    egui_view_radio_button_init_with_params(EGUI_VIEW_OF(&radio_home), &radio_home_params);
    egui_view_radio_button_set_mark_style(EGUI_VIEW_OF(&radio_home), EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_ICON);
    egui_view_radio_button_set_mark_icon(EGUI_VIEW_OF(&radio_home), EGUI_ICON_MS_HOME);
    egui_view_radio_button_set_icon_font(EGUI_VIEW_OF(&radio_home), EGUI_FONT_ICON_MS_20);
    egui_view_radio_button_set_icon_text_gap(EGUI_VIEW_OF(&radio_home), 10);
    egui_view_radio_group_add(&radio_group, EGUI_VIEW_OF(&radio_home));

    egui_view_radio_button_init_with_params(EGUI_VIEW_OF(&radio_alerts), &radio_alerts_params);
    egui_view_radio_button_set_mark_style(EGUI_VIEW_OF(&radio_alerts), EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_ICON);
    egui_view_radio_button_set_mark_icon(EGUI_VIEW_OF(&radio_alerts), EGUI_ICON_MS_NOTIFICATIONS);
    egui_view_radio_button_set_icon_font(EGUI_VIEW_OF(&radio_alerts), EGUI_FONT_ICON_MS_24);
    egui_view_radio_button_set_icon_text_gap(EGUI_VIEW_OF(&radio_alerts), 10);
    egui_view_radio_group_add(&radio_group, EGUI_VIEW_OF(&radio_alerts));

    egui_view_radio_button_init_with_params(EGUI_VIEW_OF(&radio_privacy), &radio_privacy_params);
    egui_view_radio_button_set_mark_style(EGUI_VIEW_OF(&radio_privacy), EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_ICON);
    egui_view_radio_button_set_mark_icon(EGUI_VIEW_OF(&radio_privacy), EGUI_ICON_MS_VISIBILITY);
    egui_view_radio_button_set_icon_font(EGUI_VIEW_OF(&radio_privacy), EGUI_FONT_ICON_MS_24);
    egui_view_radio_button_set_icon_text_gap(EGUI_VIEW_OF(&radio_privacy), 10);
    egui_view_radio_group_add(&radio_group, EGUI_VIEW_OF(&radio_privacy));

    egui_view_radio_button_init_with_params(EGUI_VIEW_OF(&radio_settings), &radio_settings_params);
    egui_view_radio_button_set_mark_style(EGUI_VIEW_OF(&radio_settings), EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_ICON);
    egui_view_radio_button_set_mark_icon(EGUI_VIEW_OF(&radio_settings), EGUI_ICON_MS_SETTINGS);
    egui_view_radio_button_set_icon_font(EGUI_VIEW_OF(&radio_settings), EGUI_FONT_ICON_MS_24);
    egui_view_radio_button_set_icon_text_gap(EGUI_VIEW_OF(&radio_settings), 10);
    egui_view_radio_group_add(&radio_group, EGUI_VIEW_OF(&radio_settings));

    egui_view_set_margin_all(EGUI_VIEW_OF(&radio_home), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&radio_alerts), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&radio_privacy), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&radio_settings), 6);

    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&radio_home));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&radio_alerts));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&radio_privacy));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&radio_settings));

    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &radio_alerts, 1000);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &radio_privacy, 1000);
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &radio_settings, 1000);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &radio_home, 1000);
        return true;
    default:
        return false;
    }
}
#endif
