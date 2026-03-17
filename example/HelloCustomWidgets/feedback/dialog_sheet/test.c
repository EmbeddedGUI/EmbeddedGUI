#include "egui.h"
#include "egui_view_dialog_sheet.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define DIALOG_SHEET_ROOT_WIDTH          224
#define DIALOG_SHEET_ROOT_HEIGHT         300
#define DIALOG_SHEET_PRIMARY_WIDTH       196
#define DIALOG_SHEET_PRIMARY_HEIGHT      132
#define DIALOG_SHEET_COMPACT_WIDTH       106
#define DIALOG_SHEET_COMPACT_HEIGHT      86
#define DIALOG_SHEET_BOTTOM_ROW_WIDTH    218
#define DIALOG_SHEET_BOTTOM_ROW_HEIGHT   96
#define DIALOG_SHEET_GUIDE_COLOR         0x80909D
#define DIALOG_SHEET_PRIMARY_LABEL_COLOR 0x7B8795
#define DIALOG_SHEET_STATUS_COLOR        0x4E6980
#define DIALOG_SHEET_COMPACT_LABEL_COLOR 0x0B756C
#define DIALOG_SHEET_LOCKED_LABEL_COLOR  0x7F8C99
#define DIALOG_SHEET_STATUS_MIX          32

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_dialog_sheet_t sheet_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_dialog_sheet_t sheet_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_dialog_sheet_t sheet_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Dialog Sheet";
static const char *guide_text = "Tap actions to shift focus";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {128, 0}};
static char status_text[48] = "Focus Reconnect";

static const egui_view_dialog_sheet_snapshot_t primary_snapshots[] = {
        {"Sync issue", "Reconnect account?", "Resume sync for review.", "Reconnect", "Later", "Sync", "Queue paused", EGUI_VIEW_DIALOG_SHEET_TONE_WARNING, 1, 1,
         EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
        {"Delete draft", "Delete unfinished layout?", "Remove local draft.", "Delete", "Cancel", "Draft", "Cannot undo", EGUI_VIEW_DIALOG_SHEET_TONE_ERROR, 1,
         1, EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY},
        {"Template", "Apply starter scene?", "Load base panels.", "Apply", NULL, "Template", "Saved", EGUI_VIEW_DIALOG_SHEET_TONE_ACCENT, 0, 0,
         EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
        {"Publishing", "Send build to review?", "Share build now.", "Send", NULL, "Review", "Ready", EGUI_VIEW_DIALOG_SHEET_TONE_SUCCESS, 0, 1,
         EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
};

static const egui_view_dialog_sheet_snapshot_t compact_snapshots[] = {
        {"Network", "Reconnect?", "Resume sync.", "Retry", NULL, "Sync", "", EGUI_VIEW_DIALOG_SHEET_TONE_WARNING, 0, 0, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
        {"Review", "Send?", "Share build.", "Send", NULL, "Build", "", EGUI_VIEW_DIALOG_SHEET_TONE_ACCENT, 0, 0, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
};

static const egui_view_dialog_sheet_snapshot_t locked_snapshots[] = {
        {"Read only", "Review", "State fixed.", NULL, NULL, "Locked", "", EGUI_VIEW_DIALOG_SHEET_TONE_NEUTRAL, 0, 0, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
};

static egui_color_t dialog_sheet_status_color(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_DIALOG_SHEET_TONE_SUCCESS:
        return EGUI_COLOR_HEX(0x178454);
    case EGUI_VIEW_DIALOG_SHEET_TONE_WARNING:
        return EGUI_COLOR_HEX(0xA46F1D);
    case EGUI_VIEW_DIALOG_SHEET_TONE_ERROR:
        return EGUI_COLOR_HEX(0xB13B35);
    case EGUI_VIEW_DIALOG_SHEET_TONE_NEUTRAL:
        return EGUI_COLOR_HEX(0x708090);
    default:
        return EGUI_COLOR_HEX(0x2563EB);
    }
}

static const char *dialog_sheet_action_text(const egui_view_dialog_sheet_snapshot_t *snapshot, uint8_t action)
{
    if (snapshot == NULL)
    {
        return "Idle";
    }
    if (action == EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY && snapshot->secondary_action != NULL && snapshot->secondary_action[0] != '\0')
    {
        return snapshot->secondary_action;
    }
    if (snapshot->primary_action != NULL && snapshot->primary_action[0] != '\0')
    {
        return snapshot->primary_action;
    }
    return "Idle";
}

static void set_status(const char *prefix, const egui_view_dialog_sheet_snapshot_t *snapshot, uint8_t action)
{
    int pos;

    status_text[0] = '\0';
    pos = egui_sprintf_str(status_text, sizeof(status_text), prefix);
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ':');
    }
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, dialog_sheet_action_text(snapshot, action));
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label),
                                   egui_rgb_mix(dialog_sheet_status_color(snapshot == NULL ? EGUI_VIEW_DIALOG_SHEET_TONE_ACCENT : snapshot->tone),
                                                EGUI_COLOR_HEX(0x536677), DIALOG_SHEET_STATUS_MIX),
                                   EGUI_ALPHA_100);
}

static void update_primary_status(void)
{
    const egui_view_dialog_sheet_snapshot_t *snapshot = &primary_snapshots[egui_view_dialog_sheet_get_current_snapshot(EGUI_VIEW_OF(&sheet_primary))];
    uint8_t action = egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&sheet_primary));

    set_status("Focus", snapshot, action);
}

static void apply_primary_snapshot(uint8_t index)
{
    egui_view_dialog_sheet_set_current_snapshot(EGUI_VIEW_OF(&sheet_primary), index);
    update_primary_status();
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    const egui_view_dialog_sheet_snapshot_t *snapshot = &compact_snapshots[index];
    uint8_t action;

    egui_view_dialog_sheet_set_current_snapshot(EGUI_VIEW_OF(&sheet_compact), index);
    action = egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&sheet_compact));
    if (update_status)
    {
        set_status("Compact", snapshot, action);
    }
}

static void on_primary_action_changed(egui_view_t *self, uint8_t action_index)
{
    const egui_view_dialog_sheet_snapshot_t *snapshot = &primary_snapshots[egui_view_dialog_sheet_get_current_snapshot(self)];

    set_status("Focus", snapshot, action_index);
}

static void on_compact_action_changed(egui_view_t *self, uint8_t action_index)
{
    const egui_view_dialog_sheet_snapshot_t *snapshot = &compact_snapshots[egui_view_dialog_sheet_get_current_snapshot(self)];

    set_status("Compact", snapshot, action_index);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), DIALOG_SHEET_ROOT_WIDTH, DIALOG_SHEET_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), DIALOG_SHEET_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 7, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), DIALOG_SHEET_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(DIALOG_SHEET_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), DIALOG_SHEET_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard sheet");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(DIALOG_SHEET_PRIMARY_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_dialog_sheet_init(EGUI_VIEW_OF(&sheet_primary));
    egui_view_set_size(EGUI_VIEW_OF(&sheet_primary), DIALOG_SHEET_PRIMARY_WIDTH, DIALOG_SHEET_PRIMARY_HEIGHT);
    egui_view_dialog_sheet_set_font(EGUI_VIEW_OF(&sheet_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_dialog_sheet_set_meta_font(EGUI_VIEW_OF(&sheet_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_dialog_sheet_set_snapshots(EGUI_VIEW_OF(&sheet_primary), primary_snapshots, 4);
    egui_view_dialog_sheet_set_on_action_changed_listener(EGUI_VIEW_OF(&sheet_primary), on_primary_action_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&sheet_primary), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&sheet_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), DIALOG_SHEET_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(DIALOG_SHEET_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 129, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xE5EBF1));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), DIALOG_SHEET_BOTTOM_ROW_WIDTH, DIALOG_SHEET_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), DIALOG_SHEET_COMPACT_WIDTH, DIALOG_SHEET_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), DIALOG_SHEET_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(DIALOG_SHEET_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_dialog_sheet_init(EGUI_VIEW_OF(&sheet_compact));
    egui_view_set_size(EGUI_VIEW_OF(&sheet_compact), DIALOG_SHEET_COMPACT_WIDTH, DIALOG_SHEET_COMPACT_HEIGHT);
    egui_view_dialog_sheet_set_font(EGUI_VIEW_OF(&sheet_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_dialog_sheet_set_meta_font(EGUI_VIEW_OF(&sheet_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_dialog_sheet_set_snapshots(EGUI_VIEW_OF(&sheet_compact), compact_snapshots, 2);
    egui_view_dialog_sheet_set_compact_mode(EGUI_VIEW_OF(&sheet_compact), 1);
    egui_view_dialog_sheet_set_palette(EGUI_VIEW_OF(&sheet_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xF2F7F7), EGUI_COLOR_HEX(0xD3DDDA),
                                       EGUI_COLOR_HEX(0x21323B), EGUI_COLOR_HEX(0x72828F), EGUI_COLOR_HEX(0x0D8076), EGUI_COLOR_HEX(0x178454),
                                       EGUI_COLOR_HEX(0xB77719), EGUI_COLOR_HEX(0xB13B35), EGUI_COLOR_HEX(0x7A8796));
    egui_view_dialog_sheet_set_on_action_changed_listener(EGUI_VIEW_OF(&sheet_compact), on_compact_action_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&sheet_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), DIALOG_SHEET_COMPACT_WIDTH, DIALOG_SHEET_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), DIALOG_SHEET_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read-only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(DIALOG_SHEET_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_dialog_sheet_init(EGUI_VIEW_OF(&sheet_locked));
    egui_view_set_size(EGUI_VIEW_OF(&sheet_locked), DIALOG_SHEET_COMPACT_WIDTH, DIALOG_SHEET_COMPACT_HEIGHT);
    egui_view_dialog_sheet_set_font(EGUI_VIEW_OF(&sheet_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_dialog_sheet_set_meta_font(EGUI_VIEW_OF(&sheet_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_dialog_sheet_set_snapshots(EGUI_VIEW_OF(&sheet_locked), locked_snapshots, 1);
    egui_view_dialog_sheet_set_compact_mode(EGUI_VIEW_OF(&sheet_locked), 1);
    egui_view_dialog_sheet_set_locked_mode(EGUI_VIEW_OF(&sheet_locked), 1);
    egui_view_dialog_sheet_set_palette(EGUI_VIEW_OF(&sheet_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xF4F6F8), EGUI_COLOR_HEX(0xD3DCE4),
                                       EGUI_COLOR_HEX(0x566473), EGUI_COLOR_HEX(0x8C98A6), EGUI_COLOR_HEX(0x94A2AF), EGUI_COLOR_HEX(0x93A39A),
                                       EGUI_COLOR_HEX(0xB59A68), EGUI_COLOR_HEX(0xA0A8B2), EGUI_COLOR_HEX(0x9AA6B4));
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&sheet_locked));

    apply_primary_snapshot(0);
    apply_compact_snapshot(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&locked_column));
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
            apply_compact_snapshot(1, 1);
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
