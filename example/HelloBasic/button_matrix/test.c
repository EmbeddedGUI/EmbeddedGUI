#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

static egui_view_button_matrix_t btn_matrix;

static const char *btn_texts[] = {
        "Search", "Home", "Profile", "Favorite", "Alerts", "Settings", "Lock", "Help", "Delete",
};

static const char *btn_icons[] = {
        EGUI_ICON_MS_SEARCH,   EGUI_ICON_MS_HOME, EGUI_ICON_MS_PERSON, EGUI_ICON_MS_FAVORITE, EGUI_ICON_MS_NOTIFICATIONS,
        EGUI_ICON_MS_SETTINGS, EGUI_ICON_MS_LOCK, EGUI_ICON_MS_HELP,   EGUI_ICON_MS_DELETE,
};

EGUI_VIEW_BUTTON_MATRIX_PARAMS_INIT(btn_matrix_params, 10, 10, 220, 220, 3, 8);

static void on_btn_click(egui_view_t *self, uint8_t btn_index)
{
    const char *name = "unknown";
    (void)self;

    if (btn_index < (sizeof(btn_texts) / sizeof(btn_texts[0])))
    {
        name = btn_texts[btn_index];
    }

    (void)name;
    EGUI_LOG_INF("ButtonMatrix clicked: index=%d, label=%s\n", btn_index, name);
}

void test_init_ui(void)
{
    egui_view_button_matrix_init_with_params(EGUI_VIEW_OF(&btn_matrix), &btn_matrix_params);
    egui_view_button_matrix_set_labels(EGUI_VIEW_OF(&btn_matrix), btn_texts, 9, 3);
    egui_view_button_matrix_set_icons(EGUI_VIEW_OF(&btn_matrix), btn_icons);
    egui_view_button_matrix_set_icon_font(EGUI_VIEW_OF(&btn_matrix), EGUI_FONT_ICON_MS_24);
    egui_view_button_matrix_set_on_click(EGUI_VIEW_OF(&btn_matrix), on_btn_click);
    egui_view_button_matrix_set_selection_enabled(EGUI_VIEW_OF(&btn_matrix), 1);
    egui_view_button_matrix_set_selected_index(EGUI_VIEW_OF(&btn_matrix), 0);
    egui_view_button_matrix_set_corner_radius(EGUI_VIEW_OF(&btn_matrix), 12);
    egui_view_button_matrix_set_gap(EGUI_VIEW_OF(&btn_matrix), 8);
    egui_view_button_matrix_set_icon_text_gap(EGUI_VIEW_OF(&btn_matrix), 2);

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
