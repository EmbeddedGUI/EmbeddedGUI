#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_command_palette.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_command_palette_t palette_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_command_palette_t palette_pinned;
static egui_view_command_palette_t palette_recent;

static const char *title_text = "Command Palette";
static const char *guide_text = "Tap cards to rotate command states";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {141, 0}};
static const egui_color_t accent_text_color = EGUI_COLOR_HEX(0x67D6FF);
static const egui_color_t ready_text_color = EGUI_COLOR_HEX(0x7CE0BE);
static const egui_color_t warn_text_color = EGUI_COLOR_HEX(0xF1B05F);
static const egui_color_t lock_text_color = EGUI_COLOR_HEX(0xD3B08A);

static const egui_view_command_palette_snapshot_t primary_snapshots[] = {
        {"CMD", "FIND", "jump to audio route", "Open mixer scene", "Toggle cue monitor", "Enter to open", 0, 0x03, 0},
        {"CMD", "LIVE", "resume capture lane", "Resume live stack", "Pin monitor bridge", "Tab to pivot", 1, 0x06, 1},
        {"CMD", "SAFE", "return calm layout", "Seal transport grid", "Recall quiet mix", "Esc to close", 2, 0x01, 2},
};

static const egui_view_command_palette_snapshot_t pinned_snapshots[] = {
        {"PIN", "SAVE", "open pin", "Pinned cues", "Hold quick route", "Pinned set", 1, 0x02, 0},
        {"PIN", "SCAN", "scan pins", "Scan recall", "Hold quick route", "Sorting pins", 0, 0x01, 1},
        {"PIN", "SAVE", "seal pin", "Saved cues", "Hold quick route", "Pinned ready", 1, 0x02, 2},
};

static const egui_view_command_palette_snapshot_t recent_snapshots[] = {
        {"REC", "LAST", "open last", "Return mix", "Hold safe recall", "Recent route", 1, 0x02, 2},
        {"REC", "WARN", "hot lane", "Guard route", "Hold safe recall", "Review warn", 0, 0x01, 1},
        {"REC", "LAST", "calm lane", "Recall cue", "Hold safe recall", "Recent sealed", 1, 0x02, 2},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_command_palette_get_current_snapshot(self) + 1) % 3;
    egui_view_command_palette_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Primary live route", warn_text_color);
    }
    else if (next == 2)
    {
        set_status("Primary safe route", lock_text_color);
    }
    else
    {
        set_status("Primary command find", accent_text_color);
    }
}

static void on_pinned_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_command_palette_get_current_snapshot(self) + 1) % 3;
    egui_view_command_palette_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Pinned scan active", warn_text_color);
    }
    else
    {
        set_status("Pinned stack saved", ready_text_color);
    }
}

static void on_recent_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_command_palette_get_current_snapshot(self) + 1) % 3;
    egui_view_command_palette_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Recent queue warn", warn_text_color);
    }
    else
    {
        set_status("Recent route sealed", lock_text_color);
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
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_command_palette_init(EGUI_VIEW_OF(&palette_primary));
    egui_view_set_size(EGUI_VIEW_OF(&palette_primary), 192, 118);
    egui_view_command_palette_set_snapshots(EGUI_VIEW_OF(&palette_primary), primary_snapshots, 3);
    egui_view_command_palette_set_font(EGUI_VIEW_OF(&palette_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&palette_primary), 0, 0, 0, 6);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&palette_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&palette_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 220, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Primary command find");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), accent_text_color, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 142, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0x30475C));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 215, 86);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_command_palette_init(EGUI_VIEW_OF(&palette_pinned));
    egui_view_set_size(EGUI_VIEW_OF(&palette_pinned), 108, 80);
    egui_view_command_palette_set_snapshots(EGUI_VIEW_OF(&palette_pinned), pinned_snapshots, 3);
    egui_view_command_palette_set_font(EGUI_VIEW_OF(&palette_pinned), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_command_palette_set_compact_mode(EGUI_VIEW_OF(&palette_pinned), 1);
    egui_view_command_palette_set_palette(EGUI_VIEW_OF(&palette_pinned), EGUI_COLOR_HEX(0x122117), EGUI_COLOR_HEX(0x3E6E59), EGUI_COLOR_HEX(0x18281F),
                                          EGUI_COLOR_HEX(0x203325), EGUI_COLOR_HEX(0xECFFF5), EGUI_COLOR_HEX(0x8DB6A2), EGUI_COLOR_HEX(0x61D9A9),
                                          EGUI_COLOR_HEX(0xE1B35C), EGUI_COLOR_HEX(0xCCAA78), EGUI_COLOR_HEX(0x98F2C7));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&palette_pinned), on_pinned_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&palette_pinned));

    egui_view_command_palette_init(EGUI_VIEW_OF(&palette_recent));
    egui_view_set_size(EGUI_VIEW_OF(&palette_recent), 102, 80);
    egui_view_command_palette_set_snapshots(EGUI_VIEW_OF(&palette_recent), recent_snapshots, 3);
    egui_view_command_palette_set_font(EGUI_VIEW_OF(&palette_recent), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_command_palette_set_compact_mode(EGUI_VIEW_OF(&palette_recent), 1);
    egui_view_command_palette_set_locked_mode(EGUI_VIEW_OF(&palette_recent), 1);
    egui_view_command_palette_set_palette(EGUI_VIEW_OF(&palette_recent), EGUI_COLOR_HEX(0x261B14), EGUI_COLOR_HEX(0x6B5644), EGUI_COLOR_HEX(0x31241D),
                                          EGUI_COLOR_HEX(0x3A2A21), EGUI_COLOR_HEX(0xFFF2E6), EGUI_COLOR_HEX(0xD2AE93), EGUI_COLOR_HEX(0xF0BC7E),
                                          EGUI_COLOR_HEX(0xF2B05E), EGUI_COLOR_HEX(0xD6A36E), EGUI_COLOR_HEX(0xFFD7AA));
    egui_view_set_margin(EGUI_VIEW_OF(&palette_recent), 5, 0, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&palette_recent), on_recent_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&palette_recent));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&palette_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&palette_pinned), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&palette_recent), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
