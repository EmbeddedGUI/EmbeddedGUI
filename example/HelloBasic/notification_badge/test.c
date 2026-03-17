#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

static egui_view_notification_badge_t badge_1;
static egui_view_notification_badge_t badge_2;
static egui_view_notification_badge_t badge_3;
static egui_view_notification_badge_t badge_4;

static egui_view_gridlayout_t grid;

EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 220, 220, 2, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(badge_1_params, 0, 0, 34, 28, 0);
EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(badge_2_params, 0, 0, 56, 30, 150);
EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(badge_3_params, 0, 0, 34, 30, 0);
EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(badge_4_params, 0, 0, 34, 30, 0);

void test_init_ui(void)
{
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    egui_view_notification_badge_init_with_params(EGUI_VIEW_OF(&badge_1), &badge_1_params);
    egui_view_notification_badge_set_count(EGUI_VIEW_OF(&badge_1), 5);
    egui_view_notification_badge_set_badge_color(EGUI_VIEW_OF(&badge_1), EGUI_THEME_PRIMARY);
    egui_view_notification_badge_set_text_color(EGUI_VIEW_OF(&badge_1), EGUI_COLOR_WHITE);

    egui_view_notification_badge_init_with_params(EGUI_VIEW_OF(&badge_2), &badge_2_params);
    egui_view_notification_badge_set_badge_color(EGUI_VIEW_OF(&badge_2), EGUI_THEME_SUCCESS);
    egui_view_notification_badge_set_text_color(EGUI_VIEW_OF(&badge_2), EGUI_COLOR_WHITE);

    egui_view_notification_badge_init_with_params(EGUI_VIEW_OF(&badge_3), &badge_3_params);
    egui_view_notification_badge_set_content_style(EGUI_VIEW_OF(&badge_3), EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_ICON);
    egui_view_notification_badge_set_icon(EGUI_VIEW_OF(&badge_3), EGUI_ICON_MS_NOTIFICATIONS);
    egui_view_notification_badge_set_icon_font(EGUI_VIEW_OF(&badge_3), EGUI_FONT_ICON_MS_20);
    egui_view_notification_badge_set_badge_color(EGUI_VIEW_OF(&badge_3), EGUI_THEME_WARNING);
    egui_view_notification_badge_set_text_color(EGUI_VIEW_OF(&badge_3), EGUI_COLOR_WHITE);

    egui_view_notification_badge_init_with_params(EGUI_VIEW_OF(&badge_4), &badge_4_params);
    egui_view_notification_badge_set_content_style(EGUI_VIEW_OF(&badge_4), EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_ICON);
    egui_view_notification_badge_set_icon(EGUI_VIEW_OF(&badge_4), EGUI_ICON_MS_HEART);
    egui_view_notification_badge_set_icon_font(EGUI_VIEW_OF(&badge_4), EGUI_FONT_ICON_MS_20);
    egui_view_notification_badge_set_badge_color(EGUI_VIEW_OF(&badge_4), EGUI_THEME_DANGER);
    egui_view_notification_badge_set_text_color(EGUI_VIEW_OF(&badge_4), EGUI_COLOR_WHITE);

    egui_view_set_margin_all(EGUI_VIEW_OF(&badge_1), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&badge_2), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&badge_3), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&badge_4), 6);

    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&badge_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&badge_2));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&badge_3));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&badge_4));

    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
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
