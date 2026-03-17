#include "egui.h"
#include "egui_view_split_button.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define SPLIT_BUTTON_ROOT_WIDTH          224
#define SPLIT_BUTTON_ROOT_HEIGHT         292
#define SPLIT_BUTTON_PRIMARY_WIDTH       194
#define SPLIT_BUTTON_PRIMARY_HEIGHT      74
#define SPLIT_BUTTON_COMPACT_WIDTH       106
#define SPLIT_BUTTON_COMPACT_HEIGHT      66
#define SPLIT_BUTTON_BOTTOM_ROW_WIDTH    218
#define SPLIT_BUTTON_BOTTOM_ROW_HEIGHT   82
#define SPLIT_BUTTON_STATUS_MIX          26
#define SPLIT_BUTTON_GUIDE_COLOR         0x7A8896
#define SPLIT_BUTTON_LABEL_COLOR         0x74808D
#define SPLIT_BUTTON_STATUS_COLOR        0x4E677E
#define SPLIT_BUTTON_COMPACT_LABEL_COLOR 0x0E776E
#define SPLIT_BUTTON_DISABLED_LABEL      0x8994A0

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_split_button_t button_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_split_button_t button_compact;
static egui_view_linearlayout_t disabled_column;
static egui_view_label_t disabled_label;
static egui_view_split_button_t button_disabled;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Split Button";
static const char *guide_text = "Tap segments, tap guide";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[64] = "Save draft: Save";

static const egui_view_split_button_snapshot_t primary_snapshots[] = {
        {"Save draft", "SV", "Save", "Run save or open more", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 1, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
        {"Share handoff", "SH", "Share", "Send fast or choose route", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_MENU},
        {"Export file", "EX", "Export", "Export PDF or pick format", EGUI_VIEW_SPLIT_BUTTON_TONE_WARNING, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_MENU},
        {"Archive page", "AR", "Archive", "Archive now or choose policy", EGUI_VIEW_SPLIT_BUTTON_TONE_DANGER, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
};

static const egui_view_split_button_snapshot_t compact_snapshots[] = {
        {"Quick", "SV", "Save", "Tight split action", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 1, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
        {"Review", "RV", "Review", "Compact action menu", EGUI_VIEW_SPLIT_BUTTON_TONE_NEUTRAL, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_MENU},
};

static const egui_view_split_button_snapshot_t disabled_snapshots[] = {
        {"Locked", "PB", "Publish", "Visible but inactive", EGUI_VIEW_SPLIT_BUTTON_TONE_NEUTRAL, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
};

static egui_color_t split_button_status_color(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_SPLIT_BUTTON_TONE_SUCCESS:
        return EGUI_COLOR_HEX(0x178454);
    case EGUI_VIEW_SPLIT_BUTTON_TONE_WARNING:
        return EGUI_COLOR_HEX(0xA86F12);
    case EGUI_VIEW_SPLIT_BUTTON_TONE_DANGER:
        return EGUI_COLOR_HEX(0xB13A35);
    case EGUI_VIEW_SPLIT_BUTTON_TONE_NEUTRAL:
        return EGUI_COLOR_HEX(0x738291);
    default:
        return EGUI_COLOR_HEX(0x2563EB);
    }
}

static void set_status(const egui_view_split_button_snapshot_t *snapshot, uint8_t part, const char *prefix)
{
    int pos;
    const char *part_text = part == EGUI_VIEW_SPLIT_BUTTON_PART_MENU ? "Menu" : "Primary";
    const char *label_text = part == EGUI_VIEW_SPLIT_BUTTON_PART_MENU ? "More" : snapshot->label;
    egui_color_t color = split_button_status_color(snapshot->tone);

    status_text[0] = '\0';
    pos = egui_sprintf_str(status_text, sizeof(status_text), prefix);
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, ": ");
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, label_text);
    }
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " / ");
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, part_text);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x56687A), SPLIT_BUTTON_STATUS_MIX), EGUI_ALPHA_100);
}

static void update_primary_status(void)
{
    uint8_t snapshot_index = egui_view_split_button_get_current_snapshot(EGUI_VIEW_OF(&button_primary));
    uint8_t part = egui_view_split_button_get_current_part(EGUI_VIEW_OF(&button_primary));

    set_status(&primary_snapshots[snapshot_index], part, primary_snapshots[snapshot_index].title);
}

static void apply_primary_snapshot(uint8_t index)
{
    egui_view_split_button_set_current_snapshot(EGUI_VIEW_OF(&button_primary), index);
    update_primary_status();
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    egui_view_split_button_set_current_snapshot(EGUI_VIEW_OF(&button_compact), index);
    if (update_status)
    {
        set_status(&compact_snapshots[index], egui_view_split_button_get_current_part(EGUI_VIEW_OF(&button_compact)), "Compact");
    }
}

static void on_primary_part_changed(egui_view_t *self, uint8_t part)
{
    uint8_t snapshot_index = egui_view_split_button_get_current_snapshot(self);

    set_status(&primary_snapshots[snapshot_index], part, primary_snapshots[snapshot_index].title);
}

static void on_compact_part_changed(egui_view_t *self, uint8_t part)
{
    uint8_t snapshot_index = egui_view_split_button_get_current_snapshot(self);

    set_status(&compact_snapshots[snapshot_index], part, "Compact");
}

static void on_guide_click(egui_view_t *self)
{
    uint8_t next = (egui_view_split_button_get_current_snapshot(EGUI_VIEW_OF(&button_primary)) + 1) % 4;

    EGUI_UNUSED(self);
    apply_primary_snapshot(next);
}

static void on_compact_label_click(egui_view_t *self)
{
    uint8_t next = (egui_view_split_button_get_current_snapshot(EGUI_VIEW_OF(&button_compact)) + 1) % 2;

    EGUI_UNUSED(self);
    apply_compact_snapshot(next, 1);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), SPLIT_BUTTON_ROOT_WIDTH, SPLIT_BUTTON_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), SPLIT_BUTTON_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 9, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), SPLIT_BUTTON_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(SPLIT_BUTTON_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), SPLIT_BUTTON_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(SPLIT_BUTTON_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_split_button_init(EGUI_VIEW_OF(&button_primary));
    egui_view_set_size(EGUI_VIEW_OF(&button_primary), SPLIT_BUTTON_PRIMARY_WIDTH, SPLIT_BUTTON_PRIMARY_HEIGHT);
    egui_view_split_button_set_font(EGUI_VIEW_OF(&button_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_split_button_set_meta_font(EGUI_VIEW_OF(&button_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_button_set_snapshots(EGUI_VIEW_OF(&button_primary), primary_snapshots, 4);
    egui_view_split_button_set_on_part_changed_listener(EGUI_VIEW_OF(&button_primary), on_primary_part_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&button_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&button_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), SPLIT_BUTTON_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(SPLIT_BUTTON_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), SPLIT_BUTTON_BOTTOM_ROW_WIDTH, SPLIT_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), SPLIT_BUTTON_COMPACT_WIDTH, SPLIT_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), SPLIT_BUTTON_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(SPLIT_BUTTON_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 2);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_split_button_init(EGUI_VIEW_OF(&button_compact));
    egui_view_set_size(EGUI_VIEW_OF(&button_compact), SPLIT_BUTTON_COMPACT_WIDTH, SPLIT_BUTTON_COMPACT_HEIGHT);
    egui_view_split_button_set_font(EGUI_VIEW_OF(&button_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_button_set_meta_font(EGUI_VIEW_OF(&button_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_button_set_snapshots(EGUI_VIEW_OF(&button_compact), compact_snapshots, 2);
    egui_view_split_button_set_compact_mode(EGUI_VIEW_OF(&button_compact), 1);
    egui_view_split_button_set_on_part_changed_listener(EGUI_VIEW_OF(&button_compact), on_compact_part_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&button_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&disabled_column));
    egui_view_set_size(EGUI_VIEW_OF(&disabled_column), SPLIT_BUTTON_COMPACT_WIDTH, SPLIT_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&disabled_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&disabled_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&disabled_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&disabled_column));

    egui_view_label_init(EGUI_VIEW_OF(&disabled_label));
    egui_view_set_size(EGUI_VIEW_OF(&disabled_label), SPLIT_BUTTON_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&disabled_label), "Disabled");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&disabled_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&disabled_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&disabled_label), EGUI_COLOR_HEX(SPLIT_BUTTON_DISABLED_LABEL), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&disabled_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&disabled_column), EGUI_VIEW_OF(&disabled_label));

    egui_view_split_button_init(EGUI_VIEW_OF(&button_disabled));
    egui_view_set_size(EGUI_VIEW_OF(&button_disabled), SPLIT_BUTTON_COMPACT_WIDTH, SPLIT_BUTTON_COMPACT_HEIGHT);
    egui_view_split_button_set_font(EGUI_VIEW_OF(&button_disabled), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_button_set_meta_font(EGUI_VIEW_OF(&button_disabled), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_button_set_snapshots(EGUI_VIEW_OF(&button_disabled), disabled_snapshots, 1);
    egui_view_split_button_set_compact_mode(EGUI_VIEW_OF(&button_disabled), 1);
    egui_view_split_button_set_disabled_mode(EGUI_VIEW_OF(&button_disabled), 1);
    egui_view_split_button_set_palette(EGUI_VIEW_OF(&button_disabled), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xD7DEE5), EGUI_COLOR_HEX(0x53606C),
                                       EGUI_COLOR_HEX(0x8E98A3), EGUI_COLOR_HEX(0x97A4B2), EGUI_COLOR_HEX(0x93A594), EGUI_COLOR_HEX(0xB29A67),
                                       EGUI_COLOR_HEX(0xA58A86), EGUI_COLOR_HEX(0x98A3AE));
    egui_view_group_add_child(EGUI_VIEW_OF(&disabled_column), EGUI_VIEW_OF(&button_disabled));

    apply_primary_snapshot(0);
    apply_compact_snapshot(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&disabled_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
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
            apply_primary_snapshot(0);
            apply_compact_snapshot(0, 0);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 170);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_snapshot(1);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 170);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_snapshot(2);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 5:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 170);
        return true;
    case 6:
        if (first_call)
        {
            apply_compact_snapshot(1, 0);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 7:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 170);
        return true;
    case 8:
        if (first_call)
        {
            apply_primary_snapshot(3);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 520);
        return true;
    default:
        return false;
    }
}
#endif
