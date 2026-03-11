#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_patch_bay.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_patch_bay_t bay_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_patch_bay_t bay_preview;
static egui_view_patch_bay_t bay_locked;

static const char *title_text = "Patch Bay";
static const char *guide_text = "Tap panels to rotate routes";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {147, 0}};
static const egui_color_t live_status_color = EGUI_COLOR_HEX(0x53C9FF);
static const egui_color_t preview_status_color = EGUI_COLOR_HEX(0x54D39A);
static const egui_color_t warn_status_color = EGUI_COLOR_HEX(0xF5B24B);
static const egui_color_t lock_status_color = EGUI_COLOR_HEX(0xE0B173);

static const egui_view_patch_bay_snapshot_t primary_snapshots[] = {
        {"CORE PATCH", "LIVE", "Mic A -> Bus 2", "mix live", 0, 1, 0},
        {"CORE PATCH", "WATCH", "Synth -> Bus 3", "latency +2", 1, 2, 1},
        {"CORE PATCH", "LIVE", "FX -> Bus 1", "bus clear", 2, 0, 0},
};

static const egui_view_patch_bay_snapshot_t preview_snapshots[] = {
        {"PRE", "ARM", "Cue 1", "armed", 0, 0, 0},
        {"PRE", "SCAN", "Cue 2", "trace", 1, 2, 1},
        {"PRE", "ARM", "Cue 3", "armed", 2, 1, 0},
};

static const egui_view_patch_bay_snapshot_t locked_snapshots[] = {
        {"LOCK", "HOLD", "Aux 4", "sealed", 1, 1, 0},
        {"LOCK", "WARN", "Aux 5", "guard", 0, 2, 1},
        {"LOCK", "HOLD", "Aux 6", "sealed", 2, 0, 0},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_patch_bay_get_current_snapshot(self) + 1) % 3;
    egui_view_patch_bay_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Core route watch", warn_status_color);
    }
    else
    {
        set_status("Core route live", live_status_color);
    }
}

static void on_preview_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_patch_bay_get_current_snapshot(self) + 1) % 3;
    egui_view_patch_bay_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Preview scanning", warn_status_color);
    }
    else
    {
        set_status("Preview armed", preview_status_color);
    }
}

static void on_locked_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_patch_bay_get_current_snapshot(self) + 1) % 3;
    egui_view_patch_bay_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Locked route guard", warn_status_color);
    }
    else
    {
        set_status("Locked route hold", lock_status_color);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), live_status_color, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 220, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x6D8097), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_patch_bay_init(EGUI_VIEW_OF(&bay_primary));
    egui_view_set_size(EGUI_VIEW_OF(&bay_primary), 192, 118);
    egui_view_patch_bay_set_snapshots(EGUI_VIEW_OF(&bay_primary), primary_snapshots, 3);
    egui_view_patch_bay_set_font(EGUI_VIEW_OF(&bay_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&bay_primary), 0, 0, 0, 6);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&bay_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bay_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 220, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Core route live");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), live_status_color, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 142, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0x30475C));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 212, 86);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_patch_bay_init(EGUI_VIEW_OF(&bay_preview));
    egui_view_set_size(EGUI_VIEW_OF(&bay_preview), 108, 80);
    egui_view_patch_bay_set_snapshots(EGUI_VIEW_OF(&bay_preview), preview_snapshots, 3);
    egui_view_patch_bay_set_font(EGUI_VIEW_OF(&bay_preview), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_patch_bay_set_compact_mode(EGUI_VIEW_OF(&bay_preview), 1);
    egui_view_patch_bay_set_palette(EGUI_VIEW_OF(&bay_preview), EGUI_COLOR_HEX(0x0F1F20), EGUI_COLOR_HEX(0x173032), EGUI_COLOR_HEX(0x2F6869),
                                    EGUI_COLOR_HEX(0xE0FBF6), EGUI_COLOR_HEX(0x83B2AC), EGUI_COLOR_HEX(0x56D0A6), EGUI_COLOR_HEX(0xE2A852),
                                    EGUI_COLOR_HEX(0xD1A06E), EGUI_COLOR_HEX(0x091415));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&bay_preview), on_preview_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&bay_preview));

    egui_view_patch_bay_init(EGUI_VIEW_OF(&bay_locked));
    egui_view_set_size(EGUI_VIEW_OF(&bay_locked), 100, 80);
    egui_view_patch_bay_set_snapshots(EGUI_VIEW_OF(&bay_locked), locked_snapshots, 3);
    egui_view_patch_bay_set_font(EGUI_VIEW_OF(&bay_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_patch_bay_set_compact_mode(EGUI_VIEW_OF(&bay_locked), 1);
    egui_view_patch_bay_set_locked_mode(EGUI_VIEW_OF(&bay_locked), 1);
    egui_view_patch_bay_set_palette(EGUI_VIEW_OF(&bay_locked), EGUI_COLOR_HEX(0x241A13), EGUI_COLOR_HEX(0x31251C), EGUI_COLOR_HEX(0x6B513A),
                                    EGUI_COLOR_HEX(0xFFF1DE), EGUI_COLOR_HEX(0xC7A891), EGUI_COLOR_HEX(0xE9B069), EGUI_COLOR_HEX(0xF0B75E),
                                    EGUI_COLOR_HEX(0xE4B478), EGUI_COLOR_HEX(0x140E0A));
    egui_view_set_margin(EGUI_VIEW_OF(&bay_locked), 4, 0, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&bay_locked), on_locked_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&bay_locked));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&bay_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&bay_preview), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&bay_locked), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
