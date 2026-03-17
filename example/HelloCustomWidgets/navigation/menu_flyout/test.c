#include "egui.h"
#include "egui_view_menu_flyout.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define MENU_FLYOUT_PRIMARY_WIDTH  188
#define MENU_FLYOUT_PRIMARY_HEIGHT 118
#define MENU_FLYOUT_BOTTOM_WIDTH   214
#define MENU_FLYOUT_BOTTOM_HEIGHT  92
#define MENU_FLYOUT_COLUMN_WIDTH   105
#define MENU_FLYOUT_CARD_WIDTH     104
#define MENU_FLYOUT_CARD_HEIGHT    78
#define MENU_FLYOUT_STATUS_MUTED   22

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_menu_flyout_t flyout_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_menu_flyout_t flyout_compact;
static egui_view_linearlayout_t disabled_column;
static egui_view_label_t disabled_label;
static egui_view_menu_flyout_t flyout_disabled;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Menu Flyout";
static const char *guide_text = "Tap flyouts to rotate states";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};

static const egui_view_menu_flyout_item_t primary_items_0[] = {
        {"NW", "New tab", "Ctrl+N", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"OP", "Open recent", "", 0, 1, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_SUBMENU},
        {"SH", "Share link", "Ctrl+L", 0, 0, 1, 1, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"RN", "Rename", "F2", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
};

static const egui_view_menu_flyout_item_t primary_items_1[] = {
        {"SO", "Sort by name", "", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"DT", "Sort by date", "", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"PN", "Pin to top", "On", 1, 1, 1, 1, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"PR", "Preferences", "", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_SUBMENU},
};

static const egui_view_menu_flyout_item_t primary_items_2[] = {
        {"EX", "Export PNG", "Ctrl+E", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"CP", "Copy path", "Ctrl+Shift+C", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"DP", "Duplicate", "Ctrl+D", 0, 0, 1, 1, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"DL", "Delete item", "Shift+Del", 3, 1, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
};

static const egui_view_menu_flyout_item_t primary_items_3[] = {
        {"LY", "Layout wide", "", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"DS", "Density compact", "On", 2, 1, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"MV", "Move to group", "", 0, 0, 1, 1, EGUI_VIEW_MENU_FLYOUT_TRAILING_SUBMENU},
        {"AR", "Archive", "", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
};

static const egui_view_menu_flyout_item_t compact_items_0[] = {
        {"OP", "Open", "", 0, 1, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"CL", "Clone", "", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"MO", "More", "", 0, 0, 1, 1, EGUI_VIEW_MENU_FLYOUT_TRAILING_SUBMENU},
};

static const egui_view_menu_flyout_item_t compact_items_1[] = {
        {"PI", "Pin", "On", 1, 1, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"DN", "Density", "On", 2, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"DL", "Delete", "", 3, 0, 1, 1, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
};

static const egui_view_menu_flyout_item_t disabled_items_0[] = {
        {"RN", "Rename", "F2", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"EX", "Export", "Ctrl+E", 0, 1, 0, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"DL", "Delete", "", 3, 0, 0, 1, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
};

static const egui_view_menu_flyout_snapshot_t primary_snapshots[] = {
        {primary_items_0, 4, 1},
        {primary_items_1, 4, 2},
        {primary_items_2, 4, 3},
        {primary_items_3, 4, 1},
};

static const egui_view_menu_flyout_snapshot_t compact_snapshots[] = {
        {compact_items_0, 3, 0},
        {compact_items_1, 3, 0},
};

static const egui_view_menu_flyout_snapshot_t disabled_snapshots[] = {
        {disabled_items_0, 3, 1},
};

static const char *primary_statuses[] = {
        "Submenu entry focused",
        "Pinned state focused",
        "Danger action focused",
        "Density option focused",
};

static egui_color_t menu_flyout_status_color(uint8_t tone)
{
    switch (tone)
    {
    case 1:
        return EGUI_COLOR_HEX(0x177A43);
    case 2:
        return EGUI_COLOR_HEX(0xA96E0F);
    case 3:
        return EGUI_COLOR_HEX(0xA63A35);
    default:
        return EGUI_COLOR_HEX(0x265FC8);
    }
}

static uint8_t menu_flyout_focus_tone(const egui_view_menu_flyout_snapshot_t *snapshot)
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x56687A), MENU_FLYOUT_STATUS_MUTED), EGUI_ALPHA_100);
}

static void apply_primary_snapshot(uint8_t index)
{
    uint8_t tone = menu_flyout_focus_tone(&primary_snapshots[index]);

    egui_view_menu_flyout_set_current_snapshot(EGUI_VIEW_OF(&flyout_primary), index);
    set_status(primary_statuses[index], menu_flyout_status_color(tone));
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    egui_view_menu_flyout_set_current_snapshot(EGUI_VIEW_OF(&flyout_compact), index);
    if (!update_status)
    {
        return;
    }

    if (index == 0)
    {
        set_status("Compact command preview", menu_flyout_status_color(0));
    }
    else
    {
        set_status("Compact state snapshot", menu_flyout_status_color(2));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_menu_flyout_get_current_snapshot(self) + 1) % 4;

    apply_primary_snapshot(next);
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_menu_flyout_get_current_snapshot(self) + 1) % 2;

    EGUI_UNUSED(self);
    apply_compact_snapshot(next, 1);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 292);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x667483), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), 224, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Flyout");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(0x72808F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_menu_flyout_init(EGUI_VIEW_OF(&flyout_primary));
    egui_view_set_size(EGUI_VIEW_OF(&flyout_primary), MENU_FLYOUT_PRIMARY_WIDTH, MENU_FLYOUT_PRIMARY_HEIGHT);
    egui_view_menu_flyout_set_snapshots(EGUI_VIEW_OF(&flyout_primary), primary_snapshots, 4);
    egui_view_menu_flyout_set_font(EGUI_VIEW_OF(&flyout_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_menu_flyout_set_meta_font(EGUI_VIEW_OF(&flyout_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_menu_flyout_set_palette(EGUI_VIEW_OF(&flyout_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD2DBE4), EGUI_COLOR_HEX(0x17212B),
                                      EGUI_COLOR_HEX(0x6B7787), EGUI_COLOR_HEX(0x2E63DA), EGUI_COLOR_HEX(0x178454), EGUI_COLOR_HEX(0xB77719),
                                      EGUI_COLOR_HEX(0xB13A32), EGUI_COLOR_HEX(0xCDD6DE));
    egui_view_set_margin(EGUI_VIEW_OF(&flyout_primary), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&flyout_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&flyout_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), primary_statuses[0]);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(0x4E6881), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 148, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDDE5EC));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), MENU_FLYOUT_BOTTOM_WIDTH, MENU_FLYOUT_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), MENU_FLYOUT_COLUMN_WIDTH, MENU_FLYOUT_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), MENU_FLYOUT_COLUMN_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0x0E726B), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_menu_flyout_init(EGUI_VIEW_OF(&flyout_compact));
    egui_view_set_size(EGUI_VIEW_OF(&flyout_compact), MENU_FLYOUT_CARD_WIDTH, MENU_FLYOUT_CARD_HEIGHT);
    egui_view_menu_flyout_set_snapshots(EGUI_VIEW_OF(&flyout_compact), compact_snapshots, 2);
    egui_view_menu_flyout_set_font(EGUI_VIEW_OF(&flyout_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_menu_flyout_set_meta_font(EGUI_VIEW_OF(&flyout_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_menu_flyout_set_compact_mode(EGUI_VIEW_OF(&flyout_compact), 1);
    egui_view_menu_flyout_set_palette(EGUI_VIEW_OF(&flyout_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD1DBE4), EGUI_COLOR_HEX(0x20303E),
                                      EGUI_COLOR_HEX(0x708090), EGUI_COLOR_HEX(0x2E63DA), EGUI_COLOR_HEX(0x178454), EGUI_COLOR_HEX(0xB77719),
                                      EGUI_COLOR_HEX(0xB13A32), EGUI_COLOR_HEX(0xD0D8E0));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&flyout_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&flyout_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&disabled_column));
    egui_view_set_size(EGUI_VIEW_OF(&disabled_column), MENU_FLYOUT_COLUMN_WIDTH, MENU_FLYOUT_BOTTOM_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&disabled_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&disabled_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&disabled_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&disabled_column));

    egui_view_label_init(EGUI_VIEW_OF(&disabled_label));
    egui_view_set_size(EGUI_VIEW_OF(&disabled_label), MENU_FLYOUT_COLUMN_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&disabled_label), "Disabled");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&disabled_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&disabled_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&disabled_label), EGUI_COLOR_HEX(0x8A96A2), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&disabled_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&disabled_column), EGUI_VIEW_OF(&disabled_label));

    egui_view_menu_flyout_init(EGUI_VIEW_OF(&flyout_disabled));
    egui_view_set_size(EGUI_VIEW_OF(&flyout_disabled), MENU_FLYOUT_CARD_WIDTH, MENU_FLYOUT_CARD_HEIGHT);
    egui_view_menu_flyout_set_snapshots(EGUI_VIEW_OF(&flyout_disabled), disabled_snapshots, 1);
    egui_view_menu_flyout_set_font(EGUI_VIEW_OF(&flyout_disabled), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_menu_flyout_set_meta_font(EGUI_VIEW_OF(&flyout_disabled), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_menu_flyout_set_compact_mode(EGUI_VIEW_OF(&flyout_disabled), 1);
    egui_view_menu_flyout_set_disabled_mode(EGUI_VIEW_OF(&flyout_disabled), 1);
    egui_view_menu_flyout_set_palette(EGUI_VIEW_OF(&flyout_disabled), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xD4DCE3), EGUI_COLOR_HEX(0x52606E),
                                      EGUI_COLOR_HEX(0x8E99A5), EGUI_COLOR_HEX(0x95A3B2), EGUI_COLOR_HEX(0x8AA592), EGUI_COLOR_HEX(0xB18F52),
                                      EGUI_COLOR_HEX(0xB28A84), EGUI_COLOR_HEX(0xD7DDE3));
    egui_view_group_add_child(EGUI_VIEW_OF(&disabled_column), EGUI_VIEW_OF(&flyout_disabled));

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
