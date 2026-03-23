#include "egui.h"
#include <stdio.h>
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

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
static uint8_t clicked_count;
static uint8_t last_clicked_index;

static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static void set_matrix_cell_click(egui_sim_action_t *p_action, float x_ratio, float y_ratio, int interval_ms)
{
    p_action->type = EGUI_SIM_ACTION_CLICK;
    egui_sim_get_view_pos(&btn_matrix, x_ratio, y_ratio, &p_action->x1, &p_action->y1);
    p_action->interval_ms = interval_ms;
}
#endif

static void on_btn_click(egui_view_t *self, uint8_t btn_index)
{
    const char *name = "unknown";
    (void)self;

    if (btn_index < (sizeof(btn_texts) / sizeof(btn_texts[0])))
    {
        name = btn_texts[btn_index];
    }

    (void)name;
#if EGUI_CONFIG_RECORDING_TEST
    clicked_count++;
    last_clicked_index = btn_index;
#endif
    EGUI_LOG_INF("ButtonMatrix clicked: index=%d, label=%s\n", btn_index, name);
}

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
    clicked_count = 0;
    last_clicked_index = 0xFF;
#endif
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
    static int last_action = -1;
    int first_call = (action_index != last_action);

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call && egui_view_button_matrix_get_selected_index(EGUI_VIEW_OF(&btn_matrix)) != 0)
        {
            report_runtime_failure("button_matrix initial selected index mismatch");
        }
        set_matrix_cell_click(p_action, 0.50f, 0.17f, 1000);
        return true;
    case 1:
        if (first_call)
        {
            if (egui_view_button_matrix_get_selected_index(EGUI_VIEW_OF(&btn_matrix)) != 1 || last_clicked_index != 1)
            {
                report_runtime_failure("button_matrix home selection did not commit");
            }
        }
        set_matrix_cell_click(p_action, 0.50f, 0.50f, 1000);
        return true;
    case 2:
        if (first_call)
        {
            if (egui_view_button_matrix_get_selected_index(EGUI_VIEW_OF(&btn_matrix)) != 4 || last_clicked_index != 4)
            {
                report_runtime_failure("button_matrix alerts selection did not commit");
            }
        }
        set_matrix_cell_click(p_action, 0.83f, 0.83f, 1000);
        return true;
    case 3:
        if (first_call)
        {
            if (egui_view_button_matrix_get_selected_index(EGUI_VIEW_OF(&btn_matrix)) != 8 || last_clicked_index != 8)
            {
                report_runtime_failure("button_matrix delete selection did not commit");
            }
            if (clicked_count != 3)
            {
                report_runtime_failure("button_matrix click count mismatch");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    default:
        return false;
    }
}
#endif
