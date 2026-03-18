#include "egui.h"
#include <stdio.h>
#include <stdlib.h>
#include "uicode.h"

static egui_view_list_t list_1;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
static uint8_t list_click_count;
static int last_clicked_index;

static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}
#endif

EGUI_VIEW_LIST_PARAMS_INIT(list_1_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, 34);

static void list_item_click_cb(egui_view_t *self, uint8_t index)
{
#if EGUI_CONFIG_RECORDING_TEST
    (void)self;
    list_click_count++;
    last_clicked_index = index;
#else
    (void)self;
#endif
    EGUI_LOG_INF("List item %d clicked\n", index);
}

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
    list_click_count = 0;
    last_clicked_index = -1;
#endif
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
    static int last_action = -1;
    int first_call = (action_index != last_action);

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &list_1.items[1], 320);
        return true;
    case 1:
        if (first_call && last_clicked_index != 1)
        {
            report_runtime_failure("list first visible item click did not commit");
        }
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 4;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 4;
        p_action->steps = 5;
        p_action->interval_ms = 900;
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &list_1.items[5], 320);
        return true;
    case 4: // swipe down to scroll back
        if (first_call)
        {
            if (last_clicked_index != 5)
            {
                report_runtime_failure("list scrolled item click did not commit");
            }
            if (list_click_count != 2)
            {
                report_runtime_failure("list click count mismatch");
            }
        }
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 4;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 4;
        p_action->steps = 5;
        p_action->interval_ms = 900;
        return true;
    case 5:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    default:
        return false;
    }
}
#endif
