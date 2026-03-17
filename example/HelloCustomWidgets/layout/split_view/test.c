#include "egui.h"
#include "egui_view_split_view.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define SPLIT_VIEW_ROOT_W          224
#define SPLIT_VIEW_ROOT_H          296
#define SPLIT_VIEW_PRIMARY_W       194
#define SPLIT_VIEW_PRIMARY_H       104
#define SPLIT_VIEW_PREVIEW_W       108
#define SPLIT_VIEW_PREVIEW_H       74
#define SPLIT_VIEW_BOTTOM_W        222
#define SPLIT_VIEW_BOTTOM_H        90
#define SPLIT_VIEW_GUIDE_COLOR     0x788491
#define SPLIT_VIEW_LABEL_COLOR     0x74808D
#define SPLIT_VIEW_STATUS_COLOR    0x4C657C
#define SPLIT_VIEW_COMPACT_COLOR   0x0E776E
#define SPLIT_VIEW_READ_ONLY_COLOR 0x8994A0
#define SPLIT_VIEW_STATUS_MIX      24

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t standard_label;
static egui_view_split_view_t panel_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_split_view_t panel_compact;
static egui_view_linearlayout_t read_only_column;
static egui_view_label_t read_only_label;
static egui_view_split_view_t panel_read_only;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Split View";
static const char *guide_text = "Tap pane toggle or rows";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[72] = "Overview board: Pane open";

static const egui_view_split_view_item_t primary_items[] = {
        {"OV", "Overview", "8", "Workspace", "Overview board", "Updated 2m ago", "Pinned modules stay visible", "Status cards stay aligned", "Open",
         EGUI_VIEW_SPLIT_VIEW_TONE_ACCENT, 1},
        {"FI", "Files", "12", "Library", "Files library", "Synced 5m ago", "Shared assets stay within reach", "Recent exports stay sorted", "Browse",
         EGUI_VIEW_SPLIT_VIEW_TONE_ACCENT, 0},
        {"RV", "Review", "4", "Review", "Review queue", "Awaiting signoff", "Approvals stay on one rail", "Escalations remain visible", "Assign",
         EGUI_VIEW_SPLIT_VIEW_TONE_WARNING, 0},
        {"AR", "Archive", "1", "Archive", "Archive shelf", "Retention 30 days", "Older work stays tucked away", "Restore before final purge", "Archive",
         EGUI_VIEW_SPLIT_VIEW_TONE_NEUTRAL, 0},
};

static const egui_view_split_view_item_t compact_items[] = {
        {"OV", "Overview", "8", "Focus", "Overview", "Pinned snapshot", "Pane opens in place", "Detail copy stays light", "Open",
         EGUI_VIEW_SPLIT_VIEW_TONE_ACCENT, 1},
        {"RV", "Review", "4", "Queue", "Review", "Compact queue", "Approvals stay visible", "Follow-up stays nearby", "Assign",
         EGUI_VIEW_SPLIT_VIEW_TONE_WARNING, 0},
        {"AR", "Archive", "1", "Store", "Archive", "Compact archive", "Older work stays parked", "Restore when needed", "Store",
         EGUI_VIEW_SPLIT_VIEW_TONE_NEUTRAL, 0},
};

static const egui_view_split_view_item_t read_only_items[] = {
        {"MB", "Members", "7", "People", "Member roster", "Read only", "Names stay visible", "No touch edits allowed", "View",
         EGUI_VIEW_SPLIT_VIEW_TONE_SUCCESS, 0},
        {"FI", "Files", "12", "Library", "Files library", "Read only", "Shared assets stay visible", "Selection remains fixed", "View",
         EGUI_VIEW_SPLIT_VIEW_TONE_ACCENT, 0},
        {"AR", "Archive", "1", "Archive", "Archive shelf", "Read only", "Older work stays tucked away", "Restore from another flow", "View",
         EGUI_VIEW_SPLIT_VIEW_TONE_NEUTRAL, 0},
};

static egui_color_t split_view_status_color(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_SPLIT_VIEW_TONE_SUCCESS:
        return EGUI_COLOR_HEX(0x178454);
    case EGUI_VIEW_SPLIT_VIEW_TONE_WARNING:
        return EGUI_COLOR_HEX(0xA86F12);
    case EGUI_VIEW_SPLIT_VIEW_TONE_NEUTRAL:
        return EGUI_COLOR_HEX(0x738291);
    default:
        return EGUI_COLOR_HEX(0x2563EB);
    }
}

static void set_status_message(const char *title, const char *suffix, egui_color_t color)
{
    int pos;

    status_text[0] = '\0';
    pos = egui_sprintf_str(status_text, sizeof(status_text), title);
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, ": ");
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, suffix);
    }
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x56687A), SPLIT_VIEW_STATUS_MIX), EGUI_ALPHA_100);
}

static void refresh_primary_status(void)
{
    uint8_t index = egui_view_split_view_get_current_index(EGUI_VIEW_OF(&panel_primary));
    const egui_view_split_view_item_t *item = &primary_items[index];

    set_status_message(item->detail_title, egui_view_split_view_get_pane_expanded(EGUI_VIEW_OF(&panel_primary)) ? "Pane open" : "Pane compact",
                       split_view_status_color(item->tone));
}

static void on_primary_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    set_status_message(primary_items[index].detail_title, egui_view_split_view_get_pane_expanded(EGUI_VIEW_OF(&panel_primary)) ? "Pane open" : "Pane compact",
                       split_view_status_color(primary_items[index].tone));
}

static void on_primary_pane_state_changed(egui_view_t *self, uint8_t expanded)
{
    uint8_t index;

    EGUI_UNUSED(self);
    index = egui_view_split_view_get_current_index(EGUI_VIEW_OF(&panel_primary));
    set_status_message(primary_items[index].detail_title, expanded ? "Pane open" : "Pane compact", split_view_status_color(primary_items[index].tone));
}

static void on_compact_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    set_status_message(compact_items[index].detail_title,
                       egui_view_split_view_get_pane_expanded(EGUI_VIEW_OF(&panel_compact)) ? "Compact open" : "Compact rail",
                       split_view_status_color(compact_items[index].tone));
}

static void on_compact_pane_state_changed(egui_view_t *self, uint8_t expanded)
{
    uint8_t index;

    EGUI_UNUSED(self);
    index = egui_view_split_view_get_current_index(EGUI_VIEW_OF(&panel_compact));
    set_status_message(compact_items[index].detail_title, expanded ? "Compact open" : "Compact rail", split_view_status_color(compact_items[index].tone));
}

static void on_guide_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    egui_view_split_view_toggle_pane(EGUI_VIEW_OF(&panel_primary));
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    egui_view_split_view_toggle_pane(EGUI_VIEW_OF(&panel_compact));
}
void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), SPLIT_VIEW_ROOT_W, SPLIT_VIEW_ROOT_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), SPLIT_VIEW_ROOT_W, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 9, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), SPLIT_VIEW_ROOT_W, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(SPLIT_VIEW_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&standard_label));
    egui_view_set_size(EGUI_VIEW_OF(&standard_label), SPLIT_VIEW_ROOT_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&standard_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&standard_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&standard_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&standard_label), EGUI_COLOR_HEX(SPLIT_VIEW_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&standard_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&standard_label));

    egui_view_split_view_init(EGUI_VIEW_OF(&panel_primary));
    egui_view_set_size(EGUI_VIEW_OF(&panel_primary), SPLIT_VIEW_PRIMARY_W, SPLIT_VIEW_PRIMARY_H);
    egui_view_split_view_set_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_split_view_set_meta_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_view_set_items(EGUI_VIEW_OF(&panel_primary), primary_items, 4);
    egui_view_split_view_set_on_selection_changed_listener(EGUI_VIEW_OF(&panel_primary), on_primary_selection_changed);
    egui_view_split_view_set_on_pane_state_changed_listener(EGUI_VIEW_OF(&panel_primary), on_primary_pane_state_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&panel_primary), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&panel_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), SPLIT_VIEW_ROOT_W, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(SPLIT_VIEW_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), SPLIT_VIEW_BOTTOM_W, SPLIT_VIEW_BOTTOM_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_HCENTER);

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), SPLIT_VIEW_PREVIEW_W, SPLIT_VIEW_BOTTOM_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, 0, 6, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), SPLIT_VIEW_PREVIEW_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(SPLIT_VIEW_COMPACT_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_split_view_init(EGUI_VIEW_OF(&panel_compact));
    egui_view_set_size(EGUI_VIEW_OF(&panel_compact), SPLIT_VIEW_PREVIEW_W, SPLIT_VIEW_PREVIEW_H);
    egui_view_split_view_set_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_view_set_meta_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_view_set_items(EGUI_VIEW_OF(&panel_compact), compact_items, 3);
    egui_view_split_view_set_compact_mode(EGUI_VIEW_OF(&panel_compact), 1);
    egui_view_split_view_set_pane_expanded(EGUI_VIEW_OF(&panel_compact), 0);
    egui_view_split_view_set_on_selection_changed_listener(EGUI_VIEW_OF(&panel_compact), on_compact_selection_changed);
    egui_view_split_view_set_on_pane_state_changed_listener(EGUI_VIEW_OF(&panel_compact), on_compact_pane_state_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&panel_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&read_only_column));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_column), SPLIT_VIEW_PREVIEW_W, SPLIT_VIEW_BOTTOM_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&read_only_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&read_only_column), EGUI_ALIGN_HCENTER);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_column), 6, 0, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&read_only_column));

    egui_view_label_init(EGUI_VIEW_OF(&read_only_label));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_label), SPLIT_VIEW_PREVIEW_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&read_only_label), "Read Only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&read_only_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&read_only_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&read_only_label), EGUI_COLOR_HEX(SPLIT_VIEW_READ_ONLY_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&read_only_label));

    egui_view_split_view_init(EGUI_VIEW_OF(&panel_read_only));
    egui_view_set_size(EGUI_VIEW_OF(&panel_read_only), SPLIT_VIEW_PREVIEW_W, SPLIT_VIEW_PREVIEW_H);
    egui_view_split_view_set_font(EGUI_VIEW_OF(&panel_read_only), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_view_set_meta_font(EGUI_VIEW_OF(&panel_read_only), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_view_set_items(EGUI_VIEW_OF(&panel_read_only), read_only_items, 3);
    egui_view_split_view_set_current_index(EGUI_VIEW_OF(&panel_read_only), 1);
    egui_view_split_view_set_read_only_mode(EGUI_VIEW_OF(&panel_read_only), 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&panel_read_only));

    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    refresh_primary_status();

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&read_only_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_invalidate(EGUI_VIEW_OF(&root_layout));
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&panel_primary));
#endif
#if EGUI_CONFIG_RECORDING_TEST
    recording_request_snapshot();
#endif
}

#if EGUI_CONFIG_RECORDING_TEST
static void request_page_snapshot(void)
{
    egui_view_invalidate(EGUI_VIEW_OF(&root_layout));
    recording_request_snapshot();
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            egui_view_split_view_set_current_index(EGUI_VIEW_OF(&panel_primary), 0);
            egui_view_split_view_set_pane_expanded(EGUI_VIEW_OF(&panel_primary), 1);
            egui_view_split_view_set_current_index(EGUI_VIEW_OF(&panel_compact), 0);
            egui_view_split_view_set_pane_expanded(EGUI_VIEW_OF(&panel_compact), 0);
            egui_view_split_view_set_current_index(EGUI_VIEW_OF(&panel_read_only), 1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 1:
        if (first_call)
        {
            egui_view_split_view_set_pane_expanded(EGUI_VIEW_OF(&panel_primary), 0);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    case 2:
        if (first_call)
        {
            egui_view_split_view_set_current_index(EGUI_VIEW_OF(&panel_primary), 1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 3:
        if (first_call)
        {
            egui_view_split_view_set_pane_expanded(EGUI_VIEW_OF(&panel_primary), 1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    case 4:
        if (first_call)
        {
            egui_view_split_view_set_current_index(EGUI_VIEW_OF(&panel_primary), 2);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 5:
        if (first_call)
        {
            egui_view_split_view_set_current_index(EGUI_VIEW_OF(&panel_compact), 1);
            egui_view_split_view_set_pane_expanded(EGUI_VIEW_OF(&panel_compact), 1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    case 6:
        if (first_call)
        {
            egui_view_split_view_set_current_index(EGUI_VIEW_OF(&panel_compact), 2);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 7:
        if (first_call)
        {
            egui_view_split_view_set_pane_expanded(EGUI_VIEW_OF(&panel_compact), 0);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    case 8:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 520);
        return true;
    default:
        return false;
    }
}
#endif
