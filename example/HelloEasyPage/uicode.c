#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

#include "egui_page_0.h"
#include "egui_page_1.h"
#include "egui_page_2.h"

static egui_toast_std_t toast;

static egui_page_base_t *current_page = NULL;

#define UICODE_MAX_PAGE_NUM 3

union page_array
{
    egui_page_0_t page_0;
    egui_page_1_t page_1;
    egui_page_2_t page_2;
};

static union page_array g_page_array;

static int index = 0;

static char toast_str[50];

void uicode_switch_page(int page_index)
{
    index = page_index;

    egui_api_sprintf(toast_str, "Start page %d", page_index);
    egui_core_toast_show_info(toast_str);

    if (current_page)
    {
        egui_page_base_close(current_page);
    }

    switch (page_index)
    {
    case 0:
        egui_page_0_init((egui_page_base_t *)&g_page_array.page_0);
        current_page = (egui_page_base_t *)&g_page_array.page_0;
        break;
    case 1:
        egui_page_1_init((egui_page_base_t *)&g_page_array.page_1);
        current_page = (egui_page_base_t *)&g_page_array.page_1;
        break;
    case 2:
        egui_page_2_init((egui_page_base_t *)&g_page_array.page_2);
        current_page = (egui_page_base_t *)&g_page_array.page_2;
        break;
    default:
        break;
    }

    egui_page_base_open(current_page);
}

int uicode_start_next_page(void)
{
    int page_index = index + 1;
    if (page_index >= UICODE_MAX_PAGE_NUM)
    {
        egui_core_toast_show_info("No more next page");
        return -1;
    }

    uicode_switch_page(page_index);
    return 0;
}

int uicode_start_prev_page(void)
{
    int page_index = index - 1;
    if (page_index < 0)
    {
        egui_core_toast_show_info("No more previous page");
        return -1;
    }

    uicode_switch_page(page_index);
    return 0;
}

void egui_port_hanlde_key_event(int key, int event)
{
    if (event == 0)
    {
        return;
    }
    EGUI_LOG_INF("key event: %d, %d\n", key, event);

    if (current_page)
    {
        egui_page_base_key_pressed(current_page, key);
    }
}

void uicode_init_ui(void)
{
    // Start page
    index = 0;
    uicode_switch_page(0);

    // Init toast
    egui_toast_std_init((egui_toast_t *)&toast);
    egui_core_toast_set((egui_toast_t *)&toast);
}

void uicode_create_ui(void)
{
    uicode_init_ui();
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
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2 - 10;
        p_action->interval_ms = 500;
        return true;
    case 2:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2 - 10;
        p_action->interval_ms = 500;
        return true;
    case 3:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2 + 30;
        p_action->interval_ms = 500;
        return true;
    case 4:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2 + 30;
        p_action->interval_ms = 500;
        return true;
    default:
        return false;
    }
}
#endif
