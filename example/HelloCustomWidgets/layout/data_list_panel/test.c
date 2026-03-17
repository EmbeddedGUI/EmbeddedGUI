#include "egui.h"
#include "egui_view_data_list_panel.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define DATA_LIST_PANEL_ROOT_WIDTH            224
#define DATA_LIST_PANEL_ROOT_HEIGHT           292
#define DATA_LIST_PANEL_PRIMARY_WIDTH         194
#define DATA_LIST_PANEL_PRIMARY_HEIGHT        116
#define DATA_LIST_PANEL_PREVIEW_WIDTH         108
#define DATA_LIST_PANEL_PREVIEW_HEIGHT        80
#define DATA_LIST_PANEL_BOTTOM_ROW_WIDTH      222
#define DATA_LIST_PANEL_BOTTOM_ROW_HEIGHT     94
#define DATA_LIST_PANEL_GUIDE_COLOR           0x768595
#define DATA_LIST_PANEL_LABEL_COLOR           0x74808D
#define DATA_LIST_PANEL_STATUS_COLOR          0x4C657C
#define DATA_LIST_PANEL_STATUS_MIX            28
#define DATA_LIST_PANEL_COMPACT_LABEL_COLOR   0x0F776E
#define DATA_LIST_PANEL_READ_ONLY_LABEL_COLOR 0x8994A0

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t standard_label;
static egui_view_data_list_panel_t panel_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_data_list_panel_t panel_compact;
static egui_view_linearlayout_t readonly_column;
static egui_view_label_t readonly_label;
static egui_view_data_list_panel_t panel_readonly;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Data List";
static const char *guide_text = "Tap rows or guide";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[80] = "Sync queue: Nightly sync Blocked";

static const egui_view_data_list_panel_item_t primary_items_0[] = {
        {"NS", "Nightly sync", "Ops handoff", "18", "Blocked", EGUI_VIEW_DATA_LIST_PANEL_TONE_ACCENT, 1},
        {"EX", "Exports", "Awaiting review", "12", "Review", EGUI_VIEW_DATA_LIST_PANEL_TONE_WARNING, 0},
        {"AP", "Approvals", "Ready to send", "7", "Ready", EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS, 0},
        {"AR", "Archive", "Cold store", "3", "Muted", EGUI_VIEW_DATA_LIST_PANEL_TONE_NEUTRAL, 0},
};

static const egui_view_data_list_panel_item_t primary_items_1[] = {
        {"TH", "Thumb pack", "Fresh uploads", "24", "Fresh", EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS, 1},
        {"MD", "Metadata", "Needs owner", "9", "Review", EGUI_VIEW_DATA_LIST_PANEL_TONE_WARNING, 0},
        {"VC", "Version diff", "Build mismatch", "5", "Watch", EGUI_VIEW_DATA_LIST_PANEL_TONE_ACCENT, 0},
        {"BK", "Backup set", "Cold copy", "2", "Muted", EGUI_VIEW_DATA_LIST_PANEL_TONE_NEUTRAL, 0},
};

static const egui_view_data_list_panel_item_t primary_items_2[] = {
        {"RT", "Retention", "31 day hold", "31", "Watch", EGUI_VIEW_DATA_LIST_PANEL_TONE_WARNING, 1},
        {"CL", "Cleanup", "Queued purge", "8", "Ready", EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS, 0},
        {"RS", "Restore", "Pending owner", "4", "Review", EGUI_VIEW_DATA_LIST_PANEL_TONE_ACCENT, 0},
        {"LG", "Legacy", "Muted set", "2", "Muted", EGUI_VIEW_DATA_LIST_PANEL_TONE_NEUTRAL, 0},
};

static const egui_view_data_list_panel_snapshot_t primary_snapshots[] = {
        {"Queue lane", "Sync queue", "3 rows tracked", primary_items_0, 3, 0},
        {"Asset desk", "Asset review", "3 assets staged", primary_items_1, 3, 0},
        {"Archive", "Archive sweep", "Retention overview", primary_items_2, 3, 1},
};

static const egui_view_data_list_panel_item_t compact_items_0[] = {
        {"NS", "Nightly", "Ops", "18", "Blocked", EGUI_VIEW_DATA_LIST_PANEL_TONE_ACCENT, 1},
        {"EX", "Exports", "Wait", "12", "Review", EGUI_VIEW_DATA_LIST_PANEL_TONE_WARNING, 0},
        {"AP", "Approve", "Ready", "7", "Ready", EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS, 0},
};

static const egui_view_data_list_panel_item_t compact_items_1[] = {
        {"TH", "Thumbs", "Fresh", "24", "Fresh", EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS, 1},
        {"MD", "Meta", "Owner", "9", "Review", EGUI_VIEW_DATA_LIST_PANEL_TONE_WARNING, 0},
        {"VC", "Diff", "Build", "5", "Watch", EGUI_VIEW_DATA_LIST_PANEL_TONE_ACCENT, 0},
};

static const egui_view_data_list_panel_snapshot_t compact_snapshots[] = {
        {"", "Recent", "Rows staged", compact_items_0, 3, 0},
        {"", "Review", "Rows checked", compact_items_1, 3, 0},
};

static const egui_view_data_list_panel_item_t readonly_items_0[] = {
        {"AR", "Archive", "Cold store", "3", "Muted", EGUI_VIEW_DATA_LIST_PANEL_TONE_NEUTRAL, 1},
        {"RS", "Restore", "Pending", "4", "Review", EGUI_VIEW_DATA_LIST_PANEL_TONE_ACCENT, 0},
        {"CL", "Cleanup", "Queued", "8", "Ready", EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS, 0},
};

static const egui_view_data_list_panel_snapshot_t readonly_snapshots[] = {
        {"", "Locked", "Static preview", readonly_items_0, 3, 0},
};

static egui_color_t data_list_status_color(uint8_t tone)
{
    egui_color_t tone_color;

    switch (tone)
    {
    case EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS:
        tone_color = EGUI_COLOR_HEX(0x178454);
        break;
    case EGUI_VIEW_DATA_LIST_PANEL_TONE_WARNING:
        tone_color = EGUI_COLOR_HEX(0xA86F12);
        break;
    case EGUI_VIEW_DATA_LIST_PANEL_TONE_NEUTRAL:
        tone_color = EGUI_COLOR_HEX(0x738291);
        break;
    default:
        tone_color = EGUI_COLOR_HEX(0x2563EB);
        break;
    }
    return egui_rgb_mix(EGUI_COLOR_HEX(DATA_LIST_PANEL_STATUS_COLOR), tone_color, DATA_LIST_PANEL_STATUS_MIX);
}

static void set_status(const egui_view_data_list_panel_snapshot_t *snapshot, uint8_t item_index, const char *fallback)
{
    const egui_view_data_list_panel_item_t *item;
    int pos;

    if (snapshot == NULL || snapshot->items == NULL || snapshot->item_count == 0)
    {
        egui_view_label_set_text(EGUI_VIEW_OF(&status_label), fallback);
        return;
    }
    if (item_index >= snapshot->item_count)
    {
        item_index = 0;
    }
    item = &snapshot->items[item_index];

    status_text[0] = '\0';
    pos = egui_sprintf_str(status_text, sizeof(status_text), snapshot->title);
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, ": ");
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, item->title);
        if (pos < (int)sizeof(status_text) - 1)
        {
            pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " ");
            egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, item->footer);
        }
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), data_list_status_color(item->tone), EGUI_ALPHA_100);
}

static void apply_primary_state(uint8_t snapshot_index, uint8_t item_index)
{
    egui_view_data_list_panel_set_current_snapshot(EGUI_VIEW_OF(&panel_primary), snapshot_index);
    egui_view_data_list_panel_set_current_index(EGUI_VIEW_OF(&panel_primary), item_index);
    set_status(&primary_snapshots[snapshot_index], item_index, "Sync queue");
}

static void apply_compact_state(uint8_t snapshot_index, uint8_t item_index)
{
    egui_view_data_list_panel_set_current_snapshot(EGUI_VIEW_OF(&panel_compact), snapshot_index);
    egui_view_data_list_panel_set_current_index(EGUI_VIEW_OF(&panel_compact), item_index);
}

static void on_primary_selection_changed(egui_view_t *self, uint8_t snapshot_index, uint8_t item_index)
{
    EGUI_UNUSED(self);
    set_status(&primary_snapshots[snapshot_index], item_index, "Sync queue");
}

static void on_compact_selection_changed(egui_view_t *self, uint8_t snapshot_index, uint8_t item_index)
{
    EGUI_UNUSED(self);
    set_status(&compact_snapshots[snapshot_index], item_index, "Compact");
}

static void on_guide_click(egui_view_t *self)
{
    uint8_t next = (egui_view_data_list_panel_get_current_snapshot(EGUI_VIEW_OF(&panel_primary)) + 1) % 3;

    EGUI_UNUSED(self);
    apply_primary_state(next, primary_snapshots[next].focus_index);
}

static void on_compact_label_click(egui_view_t *self)
{
    uint8_t next = (egui_view_data_list_panel_get_current_snapshot(EGUI_VIEW_OF(&panel_compact)) + 1) % 2;

    EGUI_UNUSED(self);
    apply_compact_state(next, compact_snapshots[next].focus_index);
}

#if EGUI_CONFIG_RECORDING_TEST
static void get_view_center(egui_view_t *view, int *x, int *y)
{
    *x = view->region_screen.location.x + view->region_screen.size.width / 2;
    *y = view->region_screen.location.y + view->region_screen.size.height / 2;
}

static void request_page_snapshot(void)
{
    egui_view_invalidate(EGUI_VIEW_OF(&root_layout));
    recording_request_snapshot();
}
#endif

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), DATA_LIST_PANEL_ROOT_WIDTH, DATA_LIST_PANEL_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), DATA_LIST_PANEL_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 9, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), DATA_LIST_PANEL_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(DATA_LIST_PANEL_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&standard_label));
    egui_view_set_size(EGUI_VIEW_OF(&standard_label), DATA_LIST_PANEL_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&standard_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&standard_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&standard_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&standard_label), EGUI_COLOR_HEX(DATA_LIST_PANEL_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&standard_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&standard_label));

    egui_view_data_list_panel_init(EGUI_VIEW_OF(&panel_primary));
    egui_view_set_size(EGUI_VIEW_OF(&panel_primary), DATA_LIST_PANEL_PRIMARY_WIDTH, DATA_LIST_PANEL_PRIMARY_HEIGHT);
    egui_view_data_list_panel_set_snapshots(EGUI_VIEW_OF(&panel_primary), primary_snapshots, 3);
    egui_view_data_list_panel_set_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_data_list_panel_set_meta_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_data_list_panel_set_on_selection_changed_listener(EGUI_VIEW_OF(&panel_primary), on_primary_selection_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&panel_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&panel_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), DATA_LIST_PANEL_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(DATA_LIST_PANEL_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 148, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDFE7EE));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), DATA_LIST_PANEL_BOTTOM_ROW_WIDTH, DATA_LIST_PANEL_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_HCENTER);

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), DATA_LIST_PANEL_PREVIEW_WIDTH, DATA_LIST_PANEL_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, 0, 6, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), DATA_LIST_PANEL_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(DATA_LIST_PANEL_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_data_list_panel_init(EGUI_VIEW_OF(&panel_compact));
    egui_view_set_size(EGUI_VIEW_OF(&panel_compact), DATA_LIST_PANEL_PREVIEW_WIDTH, DATA_LIST_PANEL_PREVIEW_HEIGHT);
    egui_view_data_list_panel_set_snapshots(EGUI_VIEW_OF(&panel_compact), compact_snapshots, 2);
    egui_view_data_list_panel_set_compact_mode(EGUI_VIEW_OF(&panel_compact), 1);
    egui_view_data_list_panel_set_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_data_list_panel_set_meta_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_data_list_panel_set_on_selection_changed_listener(EGUI_VIEW_OF(&panel_compact), on_compact_selection_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&panel_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&readonly_column));
    egui_view_set_size(EGUI_VIEW_OF(&readonly_column), DATA_LIST_PANEL_PREVIEW_WIDTH, DATA_LIST_PANEL_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&readonly_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&readonly_column), EGUI_ALIGN_HCENTER);
    egui_view_set_margin(EGUI_VIEW_OF(&readonly_column), 6, 0, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&readonly_column));

    egui_view_label_init(EGUI_VIEW_OF(&readonly_label));
    egui_view_set_size(EGUI_VIEW_OF(&readonly_label), DATA_LIST_PANEL_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&readonly_label), "Read Only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&readonly_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&readonly_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&readonly_label), EGUI_COLOR_HEX(DATA_LIST_PANEL_READ_ONLY_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&readonly_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&readonly_column), EGUI_VIEW_OF(&readonly_label));

    egui_view_data_list_panel_init(EGUI_VIEW_OF(&panel_readonly));
    egui_view_set_size(EGUI_VIEW_OF(&panel_readonly), DATA_LIST_PANEL_PREVIEW_WIDTH, DATA_LIST_PANEL_PREVIEW_HEIGHT);
    egui_view_data_list_panel_set_snapshots(EGUI_VIEW_OF(&panel_readonly), readonly_snapshots, 1);
    egui_view_data_list_panel_set_compact_mode(EGUI_VIEW_OF(&panel_readonly), 1);
    egui_view_data_list_panel_set_read_only_mode(EGUI_VIEW_OF(&panel_readonly), 1);
    egui_view_data_list_panel_set_font(EGUI_VIEW_OF(&panel_readonly), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_data_list_panel_set_meta_font(EGUI_VIEW_OF(&panel_readonly), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_group_add_child(EGUI_VIEW_OF(&readonly_column), EGUI_VIEW_OF(&panel_readonly));

    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    apply_primary_state(0, 0);
    apply_compact_state(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&readonly_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_invalidate(EGUI_VIEW_OF(&root_layout));
#if EGUI_CONFIG_RECORDING_TEST
    request_page_snapshot();
#endif
}

#if EGUI_CONFIG_RECORDING_TEST
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
            apply_primary_state(0, 0);
            apply_compact_state(0, 0);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    case 1:
        if (first_call)
        {
            apply_primary_state(0, 2);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 420);
        return true;
    case 2:
        get_view_center(EGUI_VIEW_OF(&guide_label), &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 420;
        return true;
    case 3:
        get_view_center(EGUI_VIEW_OF(&compact_label), &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 420;
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_state(2, 1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 420);
        return true;
    case 5:
        EGUI_SIM_SET_WAIT(p_action, 540);
        return true;
    default:
        return false;
    }
}
#endif
