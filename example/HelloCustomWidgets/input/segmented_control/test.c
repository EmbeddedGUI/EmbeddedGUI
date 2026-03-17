#include "egui.h"
#include "egui_view_segmented_control.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define SEGMENTED_CONTROL_PRIMARY_WIDTH       196
#define SEGMENTED_CONTROL_PRIMARY_HEIGHT      38
#define SEGMENTED_CONTROL_COMPACT_WIDTH       104
#define SEGMENTED_CONTROL_COMPACT_HEIGHT      30
#define SEGMENTED_CONTROL_BOTTOM_ROW_WIDTH    216
#define SEGMENTED_CONTROL_BOTTOM_ROW_HEIGHT   72
#define SEGMENTED_CONTROL_GUIDE_COLOR         0x6E7B8B
#define SEGMENTED_CONTROL_PRIMARY_LABEL_COLOR 0x768392
#define SEGMENTED_CONTROL_STATUS_COLOR        0x4D6781
#define SEGMENTED_CONTROL_COMPACT_LABEL_COLOR 0x0C756C
#define SEGMENTED_CONTROL_LOCKED_LABEL_COLOR  0x8794A2

typedef struct segmented_demo_snapshot segmented_demo_snapshot_t;
struct segmented_demo_snapshot
{
    const char **segments;
    const char **statuses;
    uint8_t segment_count;
    uint8_t selected_index;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_segmented_control_t control_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_segmented_control_t control_compact;
static egui_view_linearlayout_t read_only_column;
static egui_view_label_t read_only_label;
static egui_view_segmented_control_t control_read_only;

static uint8_t g_primary_snapshot = 0;
static uint8_t g_compact_snapshot = 0;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Segmented Control";
static const char *guide_text = "Tap guide to cycle scenarios";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {143, 0}};

static const char *primary_segments_0[] = {"Overview", "Team", "Usage", "Access"};
static const char *primary_status_0[] = {
        "Workspace overview active",
        "Workspace team filter active",
        "Workspace usage filter active",
        "Workspace access filter active",
};

static const char *primary_segments_1[] = {"Live", "Pending", "History"};
static const char *primary_status_1[] = {
        "Live incidents active",
        "Pending incidents active",
        "History incidents active",
};

static const char *primary_segments_2[] = {"Day", "Week", "Month", "Year"};
static const char *primary_status_2[] = {
        "Day range active",
        "Week range active",
        "Month range active",
        "Year range active",
};

static const segmented_demo_snapshot_t primary_snapshots[] = {
        {primary_segments_0, primary_status_0, 4, 1},
        {primary_segments_1, primary_status_1, 3, 0},
        {primary_segments_2, primary_status_2, 4, 2},
};

static const char *compact_segments_0[] = {"Day", "Week"};
static const char *compact_status_0[] = {
        "Compact day view",
        "Compact week view",
};

static const char *compact_segments_1[] = {"Low", "Mid", "High"};
static const char *compact_status_1[] = {
        "Compact low filter",
        "Compact mid filter",
        "Compact high filter",
};

static const segmented_demo_snapshot_t compact_snapshots[] = {
        {compact_segments_0, compact_status_0, 2, 0},
        {compact_segments_1, compact_status_1, 3, 1},
};

static const char *read_only_segments[] = {"Off", "Auto", "Lock"};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x536677), 28), EGUI_ALPHA_100);
}

static void apply_primary_snapshot(uint8_t index, uint8_t update_status)
{
    const segmented_demo_snapshot_t *snapshot = &primary_snapshots[index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]))];

    g_primary_snapshot = index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]));
    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&control_primary), snapshot->segments, snapshot->segment_count);
    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&control_primary), snapshot->selected_index);

    if (update_status && snapshot->selected_index < snapshot->segment_count)
    {
        set_status(snapshot->statuses[snapshot->selected_index], EGUI_COLOR_HEX(0x2563EB));
    }
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    const segmented_demo_snapshot_t *snapshot = &compact_snapshots[index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]))];

    g_compact_snapshot = index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]));
    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&control_compact), snapshot->segments, snapshot->segment_count);
    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&control_compact), snapshot->selected_index);

    if (update_status && snapshot->selected_index < snapshot->segment_count)
    {
        set_status(snapshot->statuses[snapshot->selected_index], EGUI_COLOR_HEX(0x0F766E));
    }
}

static void on_primary_segment_changed(egui_view_t *self, uint8_t index)
{
    const segmented_demo_snapshot_t *snapshot = &primary_snapshots[g_primary_snapshot];

    EGUI_UNUSED(self);
    if (index < snapshot->segment_count)
    {
        set_status(snapshot->statuses[index], EGUI_COLOR_HEX(0x2563EB));
    }
}

static void on_compact_segment_changed(egui_view_t *self, uint8_t index)
{
    const segmented_demo_snapshot_t *snapshot = &compact_snapshots[g_compact_snapshot];

    EGUI_UNUSED(self);
    if (index < snapshot->segment_count)
    {
        set_status(snapshot->statuses[index], EGUI_COLOR_HEX(0x0F766E));
    }
}

static void on_guide_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_primary_snapshot((uint8_t)(g_primary_snapshot + 1), 1);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_compact_snapshot((uint8_t)(g_compact_snapshot + 1), 1);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 280);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 224, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 9, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(SEGMENTED_CONTROL_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), 224, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(SEGMENTED_CONTROL_PRIMARY_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_segmented_control_init(EGUI_VIEW_OF(&control_primary));
    egui_view_set_size(EGUI_VIEW_OF(&control_primary), SEGMENTED_CONTROL_PRIMARY_WIDTH, SEGMENTED_CONTROL_PRIMARY_HEIGHT);
    egui_view_segmented_control_set_font(EGUI_VIEW_OF(&control_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_segmented_control_set_on_segment_changed_listener(EGUI_VIEW_OF(&control_primary), on_primary_segment_changed);
    hcw_segmented_control_apply_standard_style(EGUI_VIEW_OF(&control_primary));
    egui_view_set_margin(EGUI_VIEW_OF(&control_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&control_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Workspace team filter active");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(SEGMENTED_CONTROL_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 144, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDEE4EB));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), SEGMENTED_CONTROL_BOTTOM_ROW_WIDTH, SEGMENTED_CONTROL_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), SEGMENTED_CONTROL_COMPACT_WIDTH, SEGMENTED_CONTROL_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), SEGMENTED_CONTROL_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(SEGMENTED_CONTROL_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_segmented_control_init(EGUI_VIEW_OF(&control_compact));
    egui_view_set_size(EGUI_VIEW_OF(&control_compact), SEGMENTED_CONTROL_COMPACT_WIDTH, SEGMENTED_CONTROL_COMPACT_HEIGHT);
    egui_view_segmented_control_set_font(EGUI_VIEW_OF(&control_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_segmented_control_set_on_segment_changed_listener(EGUI_VIEW_OF(&control_compact), on_compact_segment_changed);
    hcw_segmented_control_apply_compact_style(EGUI_VIEW_OF(&control_compact));
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&control_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&read_only_column));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_column), SEGMENTED_CONTROL_COMPACT_WIDTH, SEGMENTED_CONTROL_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&read_only_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&read_only_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&read_only_column));

    egui_view_label_init(EGUI_VIEW_OF(&read_only_label));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_label), SEGMENTED_CONTROL_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&read_only_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&read_only_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&read_only_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&read_only_label), EGUI_COLOR_HEX(SEGMENTED_CONTROL_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&read_only_label));

    egui_view_segmented_control_init(EGUI_VIEW_OF(&control_read_only));
    egui_view_set_size(EGUI_VIEW_OF(&control_read_only), SEGMENTED_CONTROL_COMPACT_WIDTH, SEGMENTED_CONTROL_COMPACT_HEIGHT);
    egui_view_segmented_control_set_font(EGUI_VIEW_OF(&control_read_only), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    hcw_segmented_control_apply_read_only_style(EGUI_VIEW_OF(&control_read_only));
    egui_view_segmented_control_set_segments(EGUI_VIEW_OF(&control_read_only), read_only_segments, 3);
    egui_view_segmented_control_set_current_index(EGUI_VIEW_OF(&control_read_only), 1);
    egui_view_set_enable(EGUI_VIEW_OF(&control_read_only), false);
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&control_read_only));

    apply_primary_snapshot(0, 1);
    apply_compact_snapshot(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&read_only_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&control_primary));
#endif
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
    int width;
    int height;
    int padding;
    int content_width;
    int content_height;
    int gap;
    int available_width;
    int base_width;
    int remainder;
    int cursor_x;
    uint8_t i;

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

    width = view->region_screen.size.width;
    height = view->region_screen.size.height;
    padding = horizontal_padding;
    if (padding * 2 >= width || padding * 2 >= height)
    {
        padding = 0;
    }

    content_width = width - padding * 2;
    content_height = height - padding * 2;
    if (content_width <= 0 || content_height <= 0)
    {
        *x = view->region_screen.location.x + width / 2;
        *y = view->region_screen.location.y + height / 2;
        return;
    }

    gap = resolve_segment_gap(content_width, count, segment_gap);
    available_width = content_width - gap * (count - 1);
    if (available_width < count)
    {
        *x = view->region_screen.location.x + width / 2;
        *y = view->region_screen.location.y + height / 2;
        return;
    }

    base_width = available_width / count;
    remainder = available_width % count;
    cursor_x = view->region_screen.location.x + padding;
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

static void get_view_center(egui_view_t *view, int *x, int *y)
{
    *x = view->region_screen.location.x + view->region_screen.size.width / 2;
    *y = view->region_screen.location.y + view->region_screen.size.height / 2;
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
        if (first_call)
        {
            apply_primary_snapshot(0, 1);
            apply_compact_snapshot(0, 0);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
            egui_view_request_focus(EGUI_VIEW_OF(&control_primary));
#endif
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 1:
        get_segment_center(EGUI_VIEW_OF(&control_primary), 4, 2, 2, 2, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 260;
        return true;
    case 2:
        get_view_center(EGUI_VIEW_OF(&guide_label), &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 260;
        return true;
    case 3:
        get_segment_center(EGUI_VIEW_OF(&control_primary), 3, 1, 2, 2, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 260;
        return true;
    case 4:
        get_view_center(EGUI_VIEW_OF(&compact_label), &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 260;
        return true;
    case 5:
        get_segment_center(EGUI_VIEW_OF(&control_compact), 3, 2, 1, 1, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 260;
        return true;
    case 6:
        if (first_call)
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
            egui_view_request_focus(EGUI_VIEW_OF(&control_primary));
#endif
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 640);
        return true;
    default:
        return false;
    }
}
#endif
