#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

static egui_view_segmented_control_t segmented_primary;
static egui_view_segmented_control_t segmented_compact;
static egui_view_segmented_control_t segmented_disabled;
static egui_view_gridlayout_t grid;

static const char *segments_primary[] = {"Home", "Search", "Alert", "Prefs"};
static const char *segment_icons_primary[] = {EGUI_ICON_MS_HOME, EGUI_ICON_MS_SEARCH, EGUI_ICON_MS_NOTIFICATIONS, EGUI_ICON_MS_SETTINGS};
static const char *segments_compact[] = {"View", "Warn", "Error"};
static const char *segment_icons_compact[] = {EGUI_ICON_MS_VISIBILITY, EGUI_ICON_MS_WARNING, EGUI_ICON_MS_ERROR};
static const char *segments_disabled[] = {"Heart", "Star", "Lock", "Done"};
static const char *segment_icons_disabled[] = {EGUI_ICON_MS_HEART, EGUI_ICON_MS_STAR, EGUI_ICON_MS_LOCK, EGUI_ICON_MS_DONE};

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 232, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
EGUI_VIEW_SEGMENTED_CONTROL_PARAMS_INIT(segmented_primary_params, 0, 0, 208, 46, segments_primary, 4);
EGUI_VIEW_SEGMENTED_CONTROL_PARAMS_INIT(segmented_compact_params, 0, 0, 208, 42, segments_compact, 3);
EGUI_VIEW_SEGMENTED_CONTROL_PARAMS_INIT(segmented_disabled_params, 0, 0, 220, 46, segments_disabled, 4);

static void on_segment_changed(egui_view_t *self, uint8_t index)
{
    (void)self;
    EGUI_LOG_INF("Segment changed: %d\r\n", index);
}

#if EGUI_CONFIG_RECORDING_TEST
static int resolve_segment_gap(int content_width, uint8_t count, int gap)
{
    if (count <= 1)
    {
        return 0;
    }
    while (gap > 0 && (content_width - gap * (count - 1)) < count)
    {
        gap--;
    }
    return gap;
}

static void get_segment_center(egui_view_t *view, uint8_t count, uint8_t index, uint8_t horizontal_padding, uint8_t segment_gap, int *x, int *y)
{
    if (count == 0)
    {
        *x = view->region_screen.location.x + view->region_screen.size.width / 2;
        *y = view->region_screen.location.y + view->region_screen.size.height / 2;
        return;
    }
    if (index >= count)
    {
        index = count - 1;
    }

    int width = view->region_screen.size.width;
    int height = view->region_screen.size.height;
    int padding = horizontal_padding;
    if (padding * 2 >= width || padding * 2 >= height)
    {
        padding = 0;
    }

    int content_width = width - padding * 2;
    int content_height = height - padding * 2;
    if (content_width <= 0 || content_height <= 0)
    {
        *x = view->region_screen.location.x + width / 2;
        *y = view->region_screen.location.y + height / 2;
        return;
    }

    int gap = resolve_segment_gap(content_width, count, segment_gap);
    int available_width = content_width - gap * (count - 1);
    if (available_width < count)
    {
        *x = view->region_screen.location.x + width / 2;
        *y = view->region_screen.location.y + height / 2;
        return;
    }

    int base_width = available_width / count;
    int remainder = available_width % count;
    int cursor_x = view->region_screen.location.x + padding;
    uint8_t i;
    for (i = 0; i < count; i++)
    {
        int segment_width = base_width;
        if (remainder > 0)
        {
            segment_width++;
            remainder--;
        }
        if (i == index)
        {
            *x = cursor_x + segment_width / 2;
            *y = view->region_screen.location.y + padding + content_height / 2;
            return;
        }
        cursor_x += segment_width + gap;
    }

    *x = view->region_screen.location.x + width / 2;
    *y = view->region_screen.location.y + height / 2;
}
#endif

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
#endif

    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    egui_view_segmented_control_init_with_params(EGUI_VIEW_OF(&segmented_primary), &segmented_primary_params);
    egui_view_segmented_control_set_on_segment_changed_listener(EGUI_VIEW_OF(&segmented_primary), on_segment_changed);
    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&segmented_primary), 1);
    egui_view_segmented_control_set_corner_radius(EGUI_VIEW_OF(&segmented_primary), 12);
    egui_view_segmented_control_set_segment_gap(EGUI_VIEW_OF(&segmented_primary), 3);
    egui_view_segmented_control_set_horizontal_padding(EGUI_VIEW_OF(&segmented_primary), 2);
    egui_view_segmented_control_set_bg_color(EGUI_VIEW_OF(&segmented_primary), EGUI_THEME_SURFACE_VARIANT);
    egui_view_segmented_control_set_selected_bg_color(EGUI_VIEW_OF(&segmented_primary), EGUI_THEME_PRIMARY);
    egui_view_segmented_control_set_selected_text_color(EGUI_VIEW_OF(&segmented_primary), EGUI_COLOR_WHITE);
    egui_view_segmented_control_set_segment_icons(EGUI_VIEW_OF(&segmented_primary), segment_icons_primary);
    egui_view_segmented_control_set_icon_font(EGUI_VIEW_OF(&segmented_primary), EGUI_FONT_ICON_MS_24);
    egui_view_segmented_control_set_icon_text_gap(EGUI_VIEW_OF(&segmented_primary), 1);

    egui_view_segmented_control_init_with_params(EGUI_VIEW_OF(&segmented_compact), &segmented_compact_params);
    egui_view_segmented_control_set_on_segment_changed_listener(EGUI_VIEW_OF(&segmented_compact), on_segment_changed);
    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&segmented_compact), 0);
    egui_view_segmented_control_set_corner_radius(EGUI_VIEW_OF(&segmented_compact), 8);
    egui_view_segmented_control_set_segment_gap(EGUI_VIEW_OF(&segmented_compact), 1);
    egui_view_segmented_control_set_horizontal_padding(EGUI_VIEW_OF(&segmented_compact), 1);
    egui_view_segmented_control_set_bg_color(EGUI_VIEW_OF(&segmented_compact), EGUI_THEME_TRACK_BG);
    egui_view_segmented_control_set_selected_bg_color(EGUI_VIEW_OF(&segmented_compact), EGUI_THEME_WARNING);
    egui_view_segmented_control_set_text_color(EGUI_VIEW_OF(&segmented_compact), EGUI_THEME_TEXT_PRIMARY);
    egui_view_segmented_control_set_selected_text_color(EGUI_VIEW_OF(&segmented_compact), EGUI_COLOR_BLACK);
    egui_view_segmented_control_set_border_color(EGUI_VIEW_OF(&segmented_compact), EGUI_THEME_TRACK_OFF);
    egui_view_segmented_control_set_segment_icons(EGUI_VIEW_OF(&segmented_compact), segment_icons_compact);
    egui_view_segmented_control_set_icon_font(EGUI_VIEW_OF(&segmented_compact), EGUI_FONT_ICON_MS_24);
    egui_view_segmented_control_set_icon_text_gap(EGUI_VIEW_OF(&segmented_compact), 1);

    egui_view_segmented_control_init_with_params(EGUI_VIEW_OF(&segmented_disabled), &segmented_disabled_params);
    egui_view_segmented_control_set_on_segment_changed_listener(EGUI_VIEW_OF(&segmented_disabled), on_segment_changed);
    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&segmented_disabled), 3);
    egui_view_segmented_control_set_corner_radius(EGUI_VIEW_OF(&segmented_disabled), 10);
    egui_view_segmented_control_set_segment_gap(EGUI_VIEW_OF(&segmented_disabled), 2);
    egui_view_segmented_control_set_horizontal_padding(EGUI_VIEW_OF(&segmented_disabled), 2);
    egui_view_segmented_control_set_bg_color(EGUI_VIEW_OF(&segmented_disabled), EGUI_THEME_SURFACE_VARIANT);
    egui_view_segmented_control_set_selected_bg_color(EGUI_VIEW_OF(&segmented_disabled), EGUI_THEME_SECONDARY);
    egui_view_segmented_control_set_text_color(EGUI_VIEW_OF(&segmented_disabled), EGUI_THEME_TEXT_SECONDARY);
    egui_view_segmented_control_set_segment_icons(EGUI_VIEW_OF(&segmented_disabled), segment_icons_disabled);
    egui_view_segmented_control_set_icon_font(EGUI_VIEW_OF(&segmented_disabled), EGUI_FONT_ICON_MS_24);
    egui_view_segmented_control_set_icon_text_gap(EGUI_VIEW_OF(&segmented_disabled), 1);
    egui_view_set_enable(EGUI_VIEW_OF(&segmented_disabled), false);

    // boundary input should be ignored
    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&segmented_primary), 9);

    egui_view_set_margin_all(EGUI_VIEW_OF(&segmented_primary), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&segmented_compact), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&segmented_disabled), 6);

    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&segmented_primary));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&segmented_compact));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&segmented_disabled));
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));
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

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;
    int x = 0;
    int y = 0;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call && egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&segmented_primary)) != 1)
        {
            report_runtime_failure("segmented_primary initial index mismatch");
        }
        get_segment_center(EGUI_VIEW_OF(&segmented_primary), 4, 2, 2, 3, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 800;
        return true;
    case 1:
        get_segment_center(EGUI_VIEW_OF(&segmented_primary), 4, 3, 2, 3, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 700;
        return true;
    case 2:
        get_segment_center(EGUI_VIEW_OF(&segmented_primary), 4, 0, 2, 3, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 320;
        return true;
    case 3:
        get_segment_center(EGUI_VIEW_OF(&segmented_primary), 4, 1, 2, 3, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 320;
        return true;
    case 4:
        get_segment_center(EGUI_VIEW_OF(&segmented_compact), 3, 2, 1, 1, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 300;
        return true;
    case 5:
        get_segment_center(EGUI_VIEW_OF(&segmented_compact), 3, 1, 1, 1, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 260;
        return true;
    case 6:
        get_segment_center(EGUI_VIEW_OF(&segmented_compact), 3, 0, 1, 1, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 260;
        return true;
    case 7:
        if (first_call && egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&segmented_compact)) != 0)
        {
            report_runtime_failure("segmented_compact final index mismatch");
        }
        get_segment_center(EGUI_VIEW_OF(&segmented_disabled), 4, 1, 2, 2, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 700;
        return true;
    case 8:
        if (first_call)
        {
            if (egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&segmented_primary)) != 1)
            {
                report_runtime_failure("segmented_primary final index mismatch");
            }
            if (egui_view_segmented_control_get_current_index(EGUI_VIEW_OF(&segmented_disabled)) != 3)
            {
                report_runtime_failure("segmented_disabled changed while disabled");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 520);
        return true;
    default:
        return false;
    }
}
#endif
