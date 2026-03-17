#include "egui.h"
#include "egui_view_master_detail.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define MASTER_DETAIL_ROOT_WIDTH            224
#define MASTER_DETAIL_ROOT_HEIGHT           292
#define MASTER_DETAIL_PRIMARY_WIDTH         194
#define MASTER_DETAIL_PRIMARY_HEIGHT        96
#define MASTER_DETAIL_PREVIEW_WIDTH         108
#define MASTER_DETAIL_PREVIEW_HEIGHT        72
#define MASTER_DETAIL_BOTTOM_ROW_WIDTH      222
#define MASTER_DETAIL_BOTTOM_ROW_HEIGHT     88
#define MASTER_DETAIL_GUIDE_COLOR           0x7A8896
#define MASTER_DETAIL_LABEL_COLOR           0x74808D
#define MASTER_DETAIL_STATUS_COLOR          0x4C657C
#define MASTER_DETAIL_COMPACT_LABEL_COLOR   0x0E776E
#define MASTER_DETAIL_READ_ONLY_LABEL_COLOR 0x8994A0
#define MASTER_DETAIL_STATUS_MIX            24

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t standard_label;
static egui_view_master_detail_t panel_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_master_detail_t panel_compact;
static egui_view_linearlayout_t readonly_column;
static egui_view_label_t readonly_label;
static egui_view_master_detail_t panel_readonly;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Master Detail";
static const char *guide_text = "Tap rows, tap guide";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[72] = "Files library: Open";

static const egui_view_master_detail_item_t primary_items[] = {
        {"FI", "Files", "12 docs", "Workspace", "Files library", "Updated 3m ago", "Pinned drafts stay ready", "Shared notes stay close", "Open",
         EGUI_VIEW_MASTER_DETAIL_TONE_ACCENT, 1},
        {"RV", "Review", "4 tasks", "Review", "Review queue", "Awaiting handoff", "Approve copy and visuals", "Escalate blockers fast", "Assign",
         EGUI_VIEW_MASTER_DETAIL_TONE_WARNING, 0},
        {"MB", "Members", "7 people", "People", "Member roster", "2 active now", "Leads and notes stay aligned", "Roles remain easy to scan", "Invite",
         EGUI_VIEW_MASTER_DETAIL_TONE_SUCCESS, 0},
        {"AR", "Archive", "1 lock", "Archive", "Archive shelf", "Retention 30 days", "Past work stays searchable", "Restore before final purge", "Archive",
         EGUI_VIEW_MASTER_DETAIL_TONE_NEUTRAL, 0},
};

static const egui_view_master_detail_item_t compact_items[] = {
        {"FI", "Files", "12", "Docs", "Docs", "3m ago", "Pinned set", "Shared notes", "Open", EGUI_VIEW_MASTER_DETAIL_TONE_ACCENT, 1},
        {"RV", "Review", "4", "Review", "Queue", "Waiting", "Approve", "Escalate fast", "Assign", EGUI_VIEW_MASTER_DETAIL_TONE_WARNING, 0},
        {"AR", "Archive", "1", "Archive", "Shelf", "Locked", "Past work", "Restore later", "Store", EGUI_VIEW_MASTER_DETAIL_TONE_NEUTRAL, 0},
};

static const egui_view_master_detail_item_t readonly_items[] = {
        {"MB", "Members", "7", "People", "Roster", "Locked", "Names ready", "Fixed preview", "View", EGUI_VIEW_MASTER_DETAIL_TONE_SUCCESS, 0},
        {"AR", "Archive", "1", "Archive", "Shelf", "Locked", "Past work", "Restore later", "View", EGUI_VIEW_MASTER_DETAIL_TONE_NEUTRAL, 0},
        {"FI", "Files", "12", "Docs", "Files", "Locked", "Pinned set", "Shared notes", "Open", EGUI_VIEW_MASTER_DETAIL_TONE_ACCENT, 0},
};

static egui_color_t master_detail_status_color(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_MASTER_DETAIL_TONE_SUCCESS:
        return EGUI_COLOR_HEX(0x178454);
    case EGUI_VIEW_MASTER_DETAIL_TONE_WARNING:
        return EGUI_COLOR_HEX(0xA86F12);
    case EGUI_VIEW_MASTER_DETAIL_TONE_NEUTRAL:
        return EGUI_COLOR_HEX(0x738291);
    default:
        return EGUI_COLOR_HEX(0x2563EB);
    }
}

static void set_status(const egui_view_master_detail_item_t *item, const char *prefix)
{
    int pos;
    egui_color_t color = master_detail_status_color(item->tone);

    status_text[0] = '\0';
    pos = egui_sprintf_str(status_text, sizeof(status_text), prefix);
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, ": ");
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, item->detail_footer);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x56687A), MASTER_DETAIL_STATUS_MIX), EGUI_ALPHA_100);
}

static void apply_primary_item(uint8_t index)
{
    egui_view_master_detail_set_current_index(EGUI_VIEW_OF(&panel_primary), index);
    set_status(&primary_items[index], primary_items[index].detail_title);
}

static void apply_compact_item(uint8_t index, uint8_t update_status)
{
    egui_view_master_detail_set_current_index(EGUI_VIEW_OF(&panel_compact), index);
    if (update_status)
    {
        set_status(&compact_items[index], "Compact");
    }
}

static void on_primary_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    set_status(&primary_items[index], primary_items[index].detail_title);
}

static void on_compact_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    set_status(&compact_items[index], "Compact");
}

static void on_guide_click(egui_view_t *self)
{
    uint8_t next = (egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&panel_primary)) + 1) % 4;

    EGUI_UNUSED(self);
    apply_primary_item(next);
}

static void on_compact_label_click(egui_view_t *self)
{
    uint8_t next = (egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&panel_compact)) + 1) % 3;

    EGUI_UNUSED(self);
    apply_compact_item(next, 1);
}

#if EGUI_CONFIG_RECORDING_TEST
static void get_view_center(egui_view_t *view, int *x, int *y)
{
    *x = view->region_screen.location.x + view->region_screen.size.width / 2;
    *y = view->region_screen.location.y + view->region_screen.size.height / 2;
}

static void get_master_row_center(egui_view_t *view, uint8_t count, uint8_t index, uint8_t compact_mode, int *x, int *y)
{
    int pad_y = compact_mode ? 5 : 7;
    int row_height = compact_mode ? 14 : 18;
    int row_gap = compact_mode ? 3 : 4;
    int content_height = view->region_screen.size.height - pad_y * 2;
    int total_height = count > 0 ? (count * row_height + (count - 1) * row_gap) : 0;
    int start_y = view->region_screen.location.y + pad_y + (content_height - total_height) / 2;
    int x_offset = compact_mode ? 18 : 38;

    if (index >= count)
    {
        index = count - 1;
    }

    *x = view->region_screen.location.x + x_offset;
    *y = start_y + index * (row_height + row_gap) + row_height / 2;
}
#endif

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), MASTER_DETAIL_ROOT_WIDTH, MASTER_DETAIL_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), MASTER_DETAIL_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 9, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), MASTER_DETAIL_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(MASTER_DETAIL_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&standard_label));
    egui_view_set_size(EGUI_VIEW_OF(&standard_label), MASTER_DETAIL_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&standard_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&standard_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&standard_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&standard_label), EGUI_COLOR_HEX(MASTER_DETAIL_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&standard_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&standard_label));

    egui_view_master_detail_init(EGUI_VIEW_OF(&panel_primary));
    egui_view_set_size(EGUI_VIEW_OF(&panel_primary), MASTER_DETAIL_PRIMARY_WIDTH, MASTER_DETAIL_PRIMARY_HEIGHT);
    egui_view_master_detail_set_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_master_detail_set_meta_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_master_detail_set_items(EGUI_VIEW_OF(&panel_primary), primary_items, 4);
    egui_view_master_detail_set_on_selection_changed_listener(EGUI_VIEW_OF(&panel_primary), on_primary_selection_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&panel_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&panel_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), MASTER_DETAIL_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(MASTER_DETAIL_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), MASTER_DETAIL_BOTTOM_ROW_WIDTH, MASTER_DETAIL_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_HCENTER);
    egui_view_set_margin(EGUI_VIEW_OF(&bottom_row), 0, 0, 0, 0);

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), MASTER_DETAIL_PREVIEW_WIDTH, MASTER_DETAIL_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, 0, 6, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), MASTER_DETAIL_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(MASTER_DETAIL_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_master_detail_init(EGUI_VIEW_OF(&panel_compact));
    egui_view_set_size(EGUI_VIEW_OF(&panel_compact), MASTER_DETAIL_PREVIEW_WIDTH, MASTER_DETAIL_PREVIEW_HEIGHT);
    egui_view_master_detail_set_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_master_detail_set_meta_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_master_detail_set_items(EGUI_VIEW_OF(&panel_compact), compact_items, 3);
    egui_view_master_detail_set_compact_mode(EGUI_VIEW_OF(&panel_compact), 1);
    egui_view_master_detail_set_on_selection_changed_listener(EGUI_VIEW_OF(&panel_compact), on_compact_selection_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&panel_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&readonly_column));
    egui_view_set_size(EGUI_VIEW_OF(&readonly_column), MASTER_DETAIL_PREVIEW_WIDTH, MASTER_DETAIL_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&readonly_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&readonly_column), EGUI_ALIGN_HCENTER);
    egui_view_set_margin(EGUI_VIEW_OF(&readonly_column), 6, 0, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&readonly_column));

    egui_view_label_init(EGUI_VIEW_OF(&readonly_label));
    egui_view_set_size(EGUI_VIEW_OF(&readonly_label), MASTER_DETAIL_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&readonly_label), "Read Only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&readonly_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&readonly_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&readonly_label), EGUI_COLOR_HEX(MASTER_DETAIL_READ_ONLY_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&readonly_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&readonly_column), EGUI_VIEW_OF(&readonly_label));

    egui_view_master_detail_init(EGUI_VIEW_OF(&panel_readonly));
    egui_view_set_size(EGUI_VIEW_OF(&panel_readonly), MASTER_DETAIL_PREVIEW_WIDTH, MASTER_DETAIL_PREVIEW_HEIGHT);
    egui_view_master_detail_set_font(EGUI_VIEW_OF(&panel_readonly), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_master_detail_set_meta_font(EGUI_VIEW_OF(&panel_readonly), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_master_detail_set_items(EGUI_VIEW_OF(&panel_readonly), readonly_items, 3);
    egui_view_master_detail_set_compact_mode(EGUI_VIEW_OF(&panel_readonly), 1);
    egui_view_master_detail_set_read_only_mode(EGUI_VIEW_OF(&panel_readonly), 1);
    egui_view_master_detail_set_current_index(EGUI_VIEW_OF(&panel_readonly), 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&readonly_column), EGUI_VIEW_OF(&panel_readonly));

    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    apply_primary_item(0);
    apply_compact_item(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&readonly_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    int x = 0;
    int y = 0;

    switch (action_index)
    {
    case 0:
        get_master_row_center(EGUI_VIEW_OF(&panel_primary), 4, 1, 0, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 800;
        return true;
    case 1:
        get_master_row_center(EGUI_VIEW_OF(&panel_primary), 4, 2, 0, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 700;
        return true;
    case 2:
        get_view_center(EGUI_VIEW_OF(&guide_label), &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 360;
        return true;
    case 3:
        get_view_center(EGUI_VIEW_OF(&compact_label), &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 360;
        return true;
    case 4:
        get_master_row_center(EGUI_VIEW_OF(&panel_compact), 3, 1, 1, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 320;
        return true;
    case 5:
        get_master_row_center(EGUI_VIEW_OF(&panel_readonly), 3, 2, 1, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 520;
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 520);
        return true;
    default:
        return false;
    }
}
#endif
