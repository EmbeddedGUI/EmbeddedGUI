#include "egui.h"
#include "egui_view_settings_panel.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define SETTINGS_PANEL_PRIMARY_WIDTH  196
#define SETTINGS_PANEL_PRIMARY_HEIGHT 132
#define SETTINGS_PANEL_BOTTOM_WIDTH   212
#define SETTINGS_PANEL_BOTTOM_HEIGHT  98
#define SETTINGS_PANEL_COLUMN_WIDTH   104
#define SETTINGS_PANEL_CARD_HEIGHT    84

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_settings_panel_t panel_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_settings_panel_t panel_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_settings_panel_t panel_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Settings Panel";
static const char *guide_text = "Tap panels to review states";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {147, 0}};

static const egui_view_settings_panel_item_t primary_items_0[] = {
        {"TH", "Theme", "Light", 0, 1, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
        {"TX", "Text size", "100%", 3, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
        {"SL", "Sleep", "5 min", 3, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
};

static const egui_view_settings_panel_item_t primary_items_1[] = {
        {"BK", "Backup sync", NULL, 1, 1, EGUI_VIEW_SETTINGS_PANEL_TRAILING_SWITCH_ON},
        {"AL", "Alerts", NULL, 0, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_SWITCH_ON},
        {"CH", "Channel", "Stable", 3, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
};

static const egui_view_settings_panel_item_t primary_items_2[] = {
        {"UP", "Updates", "Pause", 2, 1, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
        {"SV", "Saver", NULL, 3, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_SWITCH_OFF},
        {"NW", "Network", "Metered", 3, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
};

static const egui_view_settings_panel_item_t primary_items_3[] = {
        {"PR", "Privacy", NULL, 3, 1, EGUI_VIEW_SETTINGS_PANEL_TRAILING_CHEVRON},
        {"LG", "Log files", "30d", 3, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
        {"AC", "Account", "Managed", 3, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
};

static const egui_view_settings_panel_item_t compact_items_0[] = {
        {"TH", "Mode", "Day", 0, 1, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
        {"BK", "Sync", NULL, 1, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_SWITCH_ON},
};

static const egui_view_settings_panel_item_t compact_items_1[] = {
        {"UP", "Patch", "Hold", 2, 1, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
        {"NW", "Data", NULL, 3, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_SWITCH_OFF},
};

static const egui_view_settings_panel_item_t locked_items_0[] = {
        {"PR", "Privacy", NULL, 3, 1, EGUI_VIEW_SETTINGS_PANEL_TRAILING_CHEVRON},
        {"AC", "Account", "Managed", 3, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
};

static const egui_view_settings_panel_item_t locked_items_1[] = {
        {"LG", "Logs", "30d", 3, 1, EGUI_VIEW_SETTINGS_PANEL_TRAILING_VALUE},
        {"SV", "Saver", NULL, 3, 0, EGUI_VIEW_SETTINGS_PANEL_TRAILING_SWITCH_OFF},
};

static const egui_view_settings_panel_snapshot_t primary_snapshots[] = {
        {"PERSONAL", "Workspace settings", "Fluent setting rows stay aligned.", "Theme ready.", primary_items_0, 3, 0},
        {"SYNC", "Backup and alerts", "Switch rows keep the same rhythm.", "Backup stays on.", primary_items_1, 3, 0},
        {"UPDATE", "Release controls", "Warning focus keeps risk visible.", "Manual review on.", primary_items_2, 3, 0},
        {"PRIVACY", "Account review", "Muted rows stay calm in review.", "Privacy stays calm.", primary_items_3, 3, 0},
};

static const egui_view_settings_panel_snapshot_t compact_snapshots[] = {
        {"SET", "Compact", "", "Theme ready.", compact_items_0, 2, 0},
        {"WARN", "Compact", "", "Pause held.", compact_items_1, 2, 0},
};

static const egui_view_settings_panel_snapshot_t locked_snapshots[] = {
        {"LOCK", "Read only", "", "Privacy calm.", locked_items_0, 2, 0},
        {"LOCK", "Read only", "", "Logs archived.", locked_items_1, 2, 0},
};

static egui_color_t settings_panel_status_color(uint8_t tone)
{
    switch (tone)
    {
    case 1:
        return EGUI_COLOR_HEX(0x177A43);
    case 2:
        return EGUI_COLOR_HEX(0xA96E0F);
    case 3:
        return EGUI_COLOR_HEX(0x6A7480);
    default:
        return EGUI_COLOR_HEX(0x265FC8);
    }
}

static const char *settings_panel_status_text(uint8_t tone)
{
    switch (tone)
    {
    case 1:
        return "Success settings row active";
    case 2:
        return "Warning settings row active";
    case 3:
        return "Neutral settings row active";
    default:
        return "Accent settings row active";
    }
}

static uint8_t settings_panel_focus_tone(const egui_view_settings_panel_snapshot_t *snapshot)
{
    uint8_t index;

    if (snapshot == NULL || snapshot->items == NULL || snapshot->item_count == 0)
    {
        return 0;
    }

    index = snapshot->focus_index;
    if (index >= snapshot->item_count)
    {
        index = 0;
    }
    return snapshot->items[index].tone;
}

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void apply_primary_snapshot(uint8_t index)
{
    uint8_t tone = settings_panel_focus_tone(&primary_snapshots[index]);

    egui_view_settings_panel_set_current_snapshot(EGUI_VIEW_OF(&panel_primary), index);
    set_status(settings_panel_status_text(tone), settings_panel_status_color(tone));
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    uint8_t tone = settings_panel_focus_tone(&compact_snapshots[index]);

    egui_view_settings_panel_set_current_snapshot(EGUI_VIEW_OF(&panel_compact), index);
    if (!update_status)
    {
        return;
    }

    if (tone == 2)
    {
        set_status("Compact warning preview", settings_panel_status_color(tone));
    }
    else
    {
        set_status("Compact accent preview", settings_panel_status_color(tone));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_settings_panel_get_current_snapshot(self) + 1) % 4;

    apply_primary_snapshot(next);
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_settings_panel_get_current_snapshot(self) + 1) % 2;

    EGUI_UNUSED(self);
    apply_compact_snapshot(next, 1);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 300);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x667584), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_settings_panel_init(EGUI_VIEW_OF(&panel_primary));
    egui_view_set_size(EGUI_VIEW_OF(&panel_primary), SETTINGS_PANEL_PRIMARY_WIDTH, SETTINGS_PANEL_PRIMARY_HEIGHT);
    egui_view_settings_panel_set_snapshots(EGUI_VIEW_OF(&panel_primary), primary_snapshots, 4);
    egui_view_settings_panel_set_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_settings_panel_set_meta_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_settings_panel_set_palette(EGUI_VIEW_OF(&panel_primary), EGUI_COLOR_HEX(0xFEFEFF), EGUI_COLOR_HEX(0xF8FAFC), EGUI_COLOR_HEX(0xD2DBE6),
                                         EGUI_COLOR_HEX(0x17212B), EGUI_COLOR_HEX(0x667586), EGUI_COLOR_HEX(0x2E63DA), EGUI_COLOR_HEX(0x178454),
                                         EGUI_COLOR_HEX(0xB67619), EGUI_COLOR_HEX(0x768392));
    egui_view_set_margin(EGUI_VIEW_OF(&panel_primary), 0, 0, 0, 1);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&panel_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&panel_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Accent settings row active");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), settings_panel_status_color(0), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 148, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDEE6EC));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), SETTINGS_PANEL_BOTTOM_WIDTH, SETTINGS_PANEL_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), SETTINGS_PANEL_COLUMN_WIDTH, SETTINGS_PANEL_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), SETTINGS_PANEL_COLUMN_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0x156860), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_settings_panel_init(EGUI_VIEW_OF(&panel_compact));
    egui_view_set_size(EGUI_VIEW_OF(&panel_compact), SETTINGS_PANEL_COLUMN_WIDTH, SETTINGS_PANEL_CARD_HEIGHT);
    egui_view_settings_panel_set_snapshots(EGUI_VIEW_OF(&panel_compact), compact_snapshots, 2);
    egui_view_settings_panel_set_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_settings_panel_set_meta_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_settings_panel_set_compact_mode(EGUI_VIEW_OF(&panel_compact), 1);
    egui_view_settings_panel_set_palette(EGUI_VIEW_OF(&panel_compact), EGUI_COLOR_HEX(0xFCFDFF), EGUI_COLOR_HEX(0xF8FAFC), EGUI_COLOR_HEX(0xD1DBE6),
                                         EGUI_COLOR_HEX(0x213141), EGUI_COLOR_HEX(0x68778A), EGUI_COLOR_HEX(0x2E63DA), EGUI_COLOR_HEX(0x178454),
                                         EGUI_COLOR_HEX(0xB67619), EGUI_COLOR_HEX(0x768392));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&panel_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&panel_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), SETTINGS_PANEL_COLUMN_WIDTH, SETTINGS_PANEL_BOTTOM_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), SETTINGS_PANEL_COLUMN_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(0x7A8794), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_settings_panel_init(EGUI_VIEW_OF(&panel_locked));
    egui_view_set_size(EGUI_VIEW_OF(&panel_locked), SETTINGS_PANEL_COLUMN_WIDTH, SETTINGS_PANEL_CARD_HEIGHT);
    egui_view_settings_panel_set_snapshots(EGUI_VIEW_OF(&panel_locked), locked_snapshots, 2);
    egui_view_settings_panel_set_current_snapshot(EGUI_VIEW_OF(&panel_locked), 1);
    egui_view_settings_panel_set_font(EGUI_VIEW_OF(&panel_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_settings_panel_set_meta_font(EGUI_VIEW_OF(&panel_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_settings_panel_set_compact_mode(EGUI_VIEW_OF(&panel_locked), 1);
    egui_view_settings_panel_set_locked_mode(EGUI_VIEW_OF(&panel_locked), 1);
    egui_view_settings_panel_set_palette(EGUI_VIEW_OF(&panel_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xF7F8FA), EGUI_COLOR_HEX(0xD3DCE2),
                                         EGUI_COLOR_HEX(0x5E6C79), EGUI_COLOR_HEX(0x8E99A5), EGUI_COLOR_HEX(0x94A3B1), EGUI_COLOR_HEX(0x7DA488),
                                         EGUI_COLOR_HEX(0xB18E4B), EGUI_COLOR_HEX(0x96A1AC));
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&panel_locked));

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
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 1:
        if (first_call)
        {
            apply_primary_snapshot(1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_snapshot(2);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 3:
        if (first_call)
        {
            apply_compact_snapshot(1, 1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_snapshot(3);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 640);
        return true;
    default:
        return false;
    }
}
#endif
