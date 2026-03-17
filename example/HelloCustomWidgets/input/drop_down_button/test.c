#include "egui.h"
#include "egui_view_drop_down_button.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define DROP_DOWN_BUTTON_ROOT_WIDTH          224
#define DROP_DOWN_BUTTON_ROOT_HEIGHT         292
#define DROP_DOWN_BUTTON_PRIMARY_WIDTH       194
#define DROP_DOWN_BUTTON_PRIMARY_HEIGHT      74
#define DROP_DOWN_BUTTON_COMPACT_WIDTH       104
#define DROP_DOWN_BUTTON_COMPACT_HEIGHT      66
#define DROP_DOWN_BUTTON_BOTTOM_ROW_WIDTH    216
#define DROP_DOWN_BUTTON_BOTTOM_ROW_HEIGHT   82
#define DROP_DOWN_BUTTON_STATUS_MIX          15
#define DROP_DOWN_BUTTON_GUIDE_COLOR         0x818D99
#define DROP_DOWN_BUTTON_LABEL_COLOR         0x6F7B87
#define DROP_DOWN_BUTTON_STATUS_COLOR        0x6B7884
#define DROP_DOWN_BUTTON_COMPACT_LABEL_COLOR 0x4A7076
#define DROP_DOWN_BUTTON_READONLY_LABEL      0x76828C

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_drop_down_button_t button_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_drop_down_button_t button_compact;
static egui_view_linearlayout_t readonly_column;
static egui_view_label_t readonly_label;
static egui_view_drop_down_button_t button_readonly;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Drop Down Button";
static const char *guide_text = "Tap guide or buttons to rotate";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {160, 0}};

static const egui_view_drop_down_button_snapshot_t primary_snapshots[] = {
        {"Standard", "SO", "Sort", "Sort by name, date or size", "v", EGUI_VIEW_DROP_DOWN_BUTTON_TONE_ACCENT, 1},
        {"Standard", "LY", "Layout", "Switch grid, list or board", "v", EGUI_VIEW_DROP_DOWN_BUTTON_TONE_NEUTRAL, 0},
        {"Standard", "TH", "Theme", "Choose light, dark or system", "v", EGUI_VIEW_DROP_DOWN_BUTTON_TONE_WARNING, 0},
};

static const egui_view_drop_down_button_snapshot_t compact_snapshots[] = {
        {"", "SO", "Sort", "", "v", EGUI_VIEW_DROP_DOWN_BUTTON_TONE_ACCENT, 1},
        {"", "FI", "Filter", "", "v", EGUI_VIEW_DROP_DOWN_BUTTON_TONE_NEUTRAL, 0},
};

static const egui_view_drop_down_button_snapshot_t readonly_snapshots[] = {
        {"", "LK", "Locked", "", "v", EGUI_VIEW_DROP_DOWN_BUTTON_TONE_NEUTRAL, 0},
};

static const char *primary_statuses[] = {
        "Sort menu ready",
        "Layout options ready",
        "Theme options ready",
};

static const char *compact_statuses[] = {
        "Compact sort menu ready",
        "Compact filter menu ready",
};

static egui_color_t drop_down_button_status_color(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_DROP_DOWN_BUTTON_TONE_SUCCESS:
        return EGUI_COLOR_HEX(0x178454);
    case EGUI_VIEW_DROP_DOWN_BUTTON_TONE_WARNING:
        return EGUI_COLOR_HEX(0xA86F12);
    case EGUI_VIEW_DROP_DOWN_BUTTON_TONE_DANGER:
        return EGUI_COLOR_HEX(0xB13A35);
    case EGUI_VIEW_DROP_DOWN_BUTTON_TONE_NEUTRAL:
        return EGUI_COLOR_HEX(0x738291);
    default:
        return EGUI_COLOR_HEX(0x2563EB);
    }
}

static void set_status_text(const char *text, uint8_t tone)
{
    egui_color_t color = drop_down_button_status_color(tone);

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(EGUI_COLOR_HEX(DROP_DOWN_BUTTON_STATUS_COLOR), color, DROP_DOWN_BUTTON_STATUS_MIX),
                                   EGUI_ALPHA_100);
}

static void apply_primary_snapshot(uint8_t index)
{
    egui_view_drop_down_button_set_current_snapshot(EGUI_VIEW_OF(&button_primary), index);
    set_status_text(primary_statuses[index], primary_snapshots[index].tone);
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    egui_view_drop_down_button_set_current_snapshot(EGUI_VIEW_OF(&button_compact), index);
    if (update_status)
    {
        set_status_text(compact_statuses[index], compact_snapshots[index].tone);
    }
}

#if EGUI_CONFIG_RECORDING_TEST
static void apply_view_pressed(egui_view_t *view, uint8_t pressed)
{
    egui_view_set_pressed(view, pressed ? 1 : 0);
}

static void apply_view_focus(egui_view_t *view, uint8_t focused)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (focused)
    {
        egui_view_request_focus(view);
    }
    else
    {
        egui_view_clear_focus(view);
    }
#else
    EGUI_UNUSED(view);
    EGUI_UNUSED(focused);
#endif
}

static void apply_primary_pressed(uint8_t pressed)
{
    apply_view_pressed(EGUI_VIEW_OF(&button_primary), pressed);
}

static void apply_primary_focus(uint8_t focused)
{
    apply_view_focus(EGUI_VIEW_OF(&button_primary), focused);
}

static void apply_compact_focus(uint8_t focused)
{
    apply_view_focus(EGUI_VIEW_OF(&button_compact), focused);
}
#endif

static void on_guide_click(egui_view_t *self)
{
    uint8_t next = (egui_view_drop_down_button_get_current_snapshot(EGUI_VIEW_OF(&button_primary)) + 1) % 3;

    EGUI_UNUSED(self);
    apply_primary_snapshot(next);
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_drop_down_button_get_current_snapshot(self) + 1) % 3;

    apply_primary_snapshot(next);
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_drop_down_button_get_current_snapshot(self) + 1) % 2;

    apply_compact_snapshot(next, 1);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    on_compact_click(EGUI_VIEW_OF(&button_compact));
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), DROP_DOWN_BUTTON_ROOT_WIDTH, DROP_DOWN_BUTTON_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), DROP_DOWN_BUTTON_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x273441), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), DROP_DOWN_BUTTON_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(DROP_DOWN_BUTTON_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), DROP_DOWN_BUTTON_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(DROP_DOWN_BUTTON_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_drop_down_button_init(EGUI_VIEW_OF(&button_primary));
    egui_view_set_size(EGUI_VIEW_OF(&button_primary), DROP_DOWN_BUTTON_PRIMARY_WIDTH, DROP_DOWN_BUTTON_PRIMARY_HEIGHT);
    egui_view_drop_down_button_set_font(EGUI_VIEW_OF(&button_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_drop_down_button_set_meta_font(EGUI_VIEW_OF(&button_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_drop_down_button_set_snapshots(EGUI_VIEW_OF(&button_primary), primary_snapshots, 3);
    egui_view_set_margin(EGUI_VIEW_OF(&button_primary), 0, 0, 0, 2);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&button_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), DROP_DOWN_BUTTON_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), primary_statuses[0]);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(DROP_DOWN_BUTTON_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 160, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDFE5EB));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), DROP_DOWN_BUTTON_BOTTOM_ROW_WIDTH, DROP_DOWN_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), DROP_DOWN_BUTTON_COMPACT_WIDTH, DROP_DOWN_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), DROP_DOWN_BUTTON_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(DROP_DOWN_BUTTON_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_drop_down_button_init(EGUI_VIEW_OF(&button_compact));
    egui_view_set_size(EGUI_VIEW_OF(&button_compact), DROP_DOWN_BUTTON_COMPACT_WIDTH, DROP_DOWN_BUTTON_COMPACT_HEIGHT);
    egui_view_drop_down_button_set_font(EGUI_VIEW_OF(&button_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_drop_down_button_set_meta_font(EGUI_VIEW_OF(&button_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_drop_down_button_set_snapshots(EGUI_VIEW_OF(&button_compact), compact_snapshots, 2);
    egui_view_drop_down_button_set_compact_mode(EGUI_VIEW_OF(&button_compact), 1);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&button_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&readonly_column));
    egui_view_set_size(EGUI_VIEW_OF(&readonly_column), DROP_DOWN_BUTTON_COMPACT_WIDTH, DROP_DOWN_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&readonly_column), 8, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&readonly_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&readonly_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&readonly_column));

    egui_view_label_init(EGUI_VIEW_OF(&readonly_label));
    egui_view_set_size(EGUI_VIEW_OF(&readonly_label), DROP_DOWN_BUTTON_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&readonly_label), "Read-only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&readonly_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&readonly_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&readonly_label), EGUI_COLOR_HEX(DROP_DOWN_BUTTON_READONLY_LABEL), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&readonly_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&readonly_column), EGUI_VIEW_OF(&readonly_label));

    egui_view_drop_down_button_init(EGUI_VIEW_OF(&button_readonly));
    egui_view_set_size(EGUI_VIEW_OF(&button_readonly), DROP_DOWN_BUTTON_COMPACT_WIDTH, DROP_DOWN_BUTTON_COMPACT_HEIGHT);
    egui_view_drop_down_button_set_font(EGUI_VIEW_OF(&button_readonly), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_drop_down_button_set_meta_font(EGUI_VIEW_OF(&button_readonly), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_drop_down_button_set_snapshots(EGUI_VIEW_OF(&button_readonly), readonly_snapshots, 1);
    egui_view_drop_down_button_set_compact_mode(EGUI_VIEW_OF(&button_readonly), 1);
    egui_view_drop_down_button_set_read_only_mode(EGUI_VIEW_OF(&button_readonly), 1);
    egui_view_drop_down_button_set_palette(EGUI_VIEW_OF(&button_readonly), EGUI_COLOR_HEX(0xF9FBFD), EGUI_COLOR_HEX(0xD6DFE7), EGUI_COLOR_HEX(0x5B6875),
                                           EGUI_COLOR_HEX(0x8F9CA8), EGUI_COLOR_HEX(0x93A4B2), EGUI_COLOR_HEX(0x93A1A5), EGUI_COLOR_HEX(0xA79B88),
                                           EGUI_COLOR_HEX(0x9E949B), EGUI_COLOR_HEX(0x98A8B4));
    egui_view_group_add_child(EGUI_VIEW_OF(&readonly_column), EGUI_VIEW_OF(&button_readonly));

    apply_primary_snapshot(0);
    apply_compact_snapshot(0, 0);

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
            apply_primary_focus(0);
            apply_compact_focus(0);
            apply_primary_pressed(0);
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
            apply_primary_focus(1);
            set_status_text("Sort focus state", primary_snapshots[0].tone);
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
            apply_primary_pressed(1);
            set_status_text("Sort pressed state", primary_snapshots[0].tone);
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
            apply_primary_pressed(0);
            apply_primary_focus(0);
            apply_primary_snapshot(1);
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
            apply_primary_snapshot(2);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 170);
        return true;
    case 10:
        if (first_call)
        {
            apply_compact_snapshot(0, 0);
            apply_compact_focus(1);
            set_status_text("Compact focus state", compact_snapshots[0].tone);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 11:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 170);
        return true;
    case 12:
        if (first_call)
        {
            apply_compact_focus(0);
            apply_compact_snapshot(1, 1);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 13:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 170);
        return true;
    case 14:
        if (first_call)
        {
            apply_primary_snapshot(0);
            apply_compact_snapshot(0, 0);
            apply_compact_focus(0);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 15:
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
