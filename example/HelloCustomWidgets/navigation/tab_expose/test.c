#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_tab_expose.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_tab_expose_t expose_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_tab_expose_t expose_saved;
static egui_view_tab_expose_t expose_recent;

static const char *title_text = "Tab Expose";
static const char *guide_text = "Tap cards to cycle tab sets";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {153, 0}};
static const egui_color_t accent_text_color = EGUI_COLOR_HEX(0x67D6FF);
static const egui_color_t ready_text_color = EGUI_COLOR_HEX(0x72DEB8);
static const egui_color_t warn_text_color = EGUI_COLOR_HEX(0xF2B05E);
static const egui_color_t lock_text_color = EGUI_COLOR_HEX(0xD6A36E);

static const egui_view_tab_expose_snapshot_t primary_snapshots[] = {
        {"TABS", "GRID", "Three tabs ready", "focus tab open", 0, 0x03, 0},
        {"TABS", "LIVE", "Review tabs spread", "center tab live", 1, 0x06, 1},
        {"TABS", "SAFE", "Recall stack calm", "last tab sealed", 2, 0x05, 2},
};

static const egui_view_tab_expose_snapshot_t saved_snapshots[] = {
        {"SAVE", "PIN", "Pinned tabs", "saved group", 0, 0x03, 0},
        {"SAVE", "SCAN", "Review pack", "sort tabs", 1, 0x06, 1},
        {"SAVE", "PIN", "Pinned calm", "safe group", 2, 0x05, 2},
};

static const egui_view_tab_expose_snapshot_t recent_snapshots[] = {
        {"REC", "LAST", "Last tabs", "recent group", 0, 0x03, 2},
        {"REC", "WARN", "Hot queue", "guard set", 1, 0x06, 1},
        {"REC", "LAST", "Recall calm", "recent seal", 2, 0x05, 2},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_tab_expose_get_current_snapshot(self) + 1) % 3;
    egui_view_tab_expose_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Primary live tabs", warn_text_color);
    }
    else if (next == 2)
    {
        set_status("Primary recall safe", lock_text_color);
    }
    else
    {
        set_status("Primary grid ready", accent_text_color);
    }
}

static void on_saved_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_tab_expose_get_current_snapshot(self) + 1) % 3;
    egui_view_tab_expose_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Saved scan on", warn_text_color);
    }
    else
    {
        set_status("Saved tabs pinned", ready_text_color);
    }
}

static void on_recent_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_tab_expose_get_current_snapshot(self) + 1) % 3;
    egui_view_tab_expose_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Recent warn view", warn_text_color);
    }
    else
    {
        set_status("Recent tabs sealed", lock_text_color);
    }
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 220, 306);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 220, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), accent_text_color, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 220, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x70859E), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_tab_expose_init(EGUI_VIEW_OF(&expose_primary));
    egui_view_set_size(EGUI_VIEW_OF(&expose_primary), 194, 120);
    egui_view_tab_expose_set_snapshots(EGUI_VIEW_OF(&expose_primary), primary_snapshots, 3);
    egui_view_tab_expose_set_font(EGUI_VIEW_OF(&expose_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&expose_primary), 0, 0, 0, 6);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&expose_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&expose_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 220, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Primary grid ready");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), accent_text_color, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 154, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0x2B4254));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 216, 87);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_tab_expose_init(EGUI_VIEW_OF(&expose_saved));
    egui_view_set_size(EGUI_VIEW_OF(&expose_saved), 105, 81);
    egui_view_tab_expose_set_snapshots(EGUI_VIEW_OF(&expose_saved), saved_snapshots, 3);
    egui_view_tab_expose_set_font(EGUI_VIEW_OF(&expose_saved), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_tab_expose_set_compact_mode(EGUI_VIEW_OF(&expose_saved), 1);
    egui_view_tab_expose_set_palette(EGUI_VIEW_OF(&expose_saved), EGUI_COLOR_HEX(0x122117), EGUI_COLOR_HEX(0x1A3225), EGUI_COLOR_HEX(0x478066),
                                     EGUI_COLOR_HEX(0xECFFF5), EGUI_COLOR_HEX(0x90B7A5), EGUI_COLOR_HEX(0x66E2B1), EGUI_COLOR_HEX(0xE1B35C),
                                     EGUI_COLOR_HEX(0xCCA97A), EGUI_COLOR_HEX(0xA5F8D3));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&expose_saved), on_saved_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&expose_saved));

    egui_view_tab_expose_init(EGUI_VIEW_OF(&expose_recent));
    egui_view_set_size(EGUI_VIEW_OF(&expose_recent), 105, 81);
    egui_view_tab_expose_set_snapshots(EGUI_VIEW_OF(&expose_recent), recent_snapshots, 3);
    egui_view_tab_expose_set_font(EGUI_VIEW_OF(&expose_recent), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_tab_expose_set_compact_mode(EGUI_VIEW_OF(&expose_recent), 1);
    egui_view_tab_expose_set_locked_mode(EGUI_VIEW_OF(&expose_recent), 1);
    egui_view_tab_expose_set_palette(EGUI_VIEW_OF(&expose_recent), EGUI_COLOR_HEX(0x251913), EGUI_COLOR_HEX(0x37261D), EGUI_COLOR_HEX(0x755B45),
                                     EGUI_COLOR_HEX(0xFFF2E6), EGUI_COLOR_HEX(0xD2AF95), EGUI_COLOR_HEX(0xF6C688), EGUI_COLOR_HEX(0xF2B05E),
                                     EGUI_COLOR_HEX(0xD6A36E), EGUI_COLOR_HEX(0xFFDFB4));
    egui_view_set_margin(EGUI_VIEW_OF(&expose_recent), 6, 0, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&expose_recent), on_recent_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&expose_recent));

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 400);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&expose_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&expose_saved), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&expose_recent), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 7:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&expose_primary), 700);
        return true;
    case 8:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 9:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&expose_saved), 700);
        return true;
    case 10:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 11:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&expose_recent), 700);
        return true;
    case 12:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
