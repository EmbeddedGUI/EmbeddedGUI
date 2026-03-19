#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

static egui_view_mini_calendar_t calendar;

EGUI_VIEW_MINI_CALENDAR_PARAMS_INIT(calendar_params, 10, 10, 220, 220, 2026, 2, 22);

static uint8_t selected_count;
static uint8_t last_selected_day;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint8_t day_count = days[month - 1];

    if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
    {
        day_count = 29;
    }

    return day_count;
}

static uint8_t day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

    if (month < 3)
    {
        year--;
    }

    return (uint8_t)((year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7);
}

static void on_date_selected(egui_view_t *self, uint8_t day)
{
    EGUI_UNUSED(self);
    selected_count++;
    last_selected_day = day;
}

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
#endif
    selected_count = 0;
    last_selected_day = 0;

    egui_view_mini_calendar_init_with_params(EGUI_VIEW_OF(&calendar), &calendar_params);
    egui_view_mini_calendar_set_today(EGUI_VIEW_OF(&calendar), 22);
    egui_view_mini_calendar_set_first_day_of_week(EGUI_VIEW_OF(&calendar), 1);
    egui_view_mini_calendar_set_on_date_selected_listener(EGUI_VIEW_OF(&calendar), on_date_selected);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&calendar));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
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

static void set_click_day(egui_sim_action_t *p_action, uint8_t day, int interval_ms)
{
    egui_dim_t cell_w = EGUI_VIEW_OF(&calendar)->region.size.width / 7;
    egui_dim_t header_h = EGUI_VIEW_OF(&calendar)->region.size.height / 8;
    egui_dim_t cell_h = (EGUI_VIEW_OF(&calendar)->region.size.height - header_h * 2) / 6;
    uint8_t start_col = (day_of_week(calendar.year, calendar.month, 1) - calendar.first_day_of_week + 7) % 7;
    uint8_t total_days = days_in_month(calendar.year, calendar.month);
    uint8_t pos;
    uint8_t col;
    uint8_t row;

    if (day < 1 || day > total_days)
    {
        day = 1;
    }

    pos = (uint8_t)(start_col + day - 1);
    col = (uint8_t)(pos % 7);
    row = (uint8_t)(pos / 7);

    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = EGUI_VIEW_OF(&calendar)->region_screen.location.x + col * cell_w + cell_w / 2;
    p_action->y1 = EGUI_VIEW_OF(&calendar)->region_screen.location.y + header_h * 2 + row * cell_h + cell_h / 2;
    p_action->x2 = 0;
    p_action->y2 = 0;
    p_action->steps = 0;
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
        set_click_day(p_action, 24, 320);
        return true;
    case 1:
        if (first_call)
        {
            if (calendar.day != 24)
            {
                report_runtime_failure("mini_calendar did not update selected day");
            }
            if (selected_count == 0 || last_selected_day != 24)
            {
                report_runtime_failure("mini_calendar did not fire date selected callback");
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
