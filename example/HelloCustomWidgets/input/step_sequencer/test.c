#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_step_sequencer.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_step_sequencer_t sequencer_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_step_sequencer_t sequencer_preview;
static egui_view_step_sequencer_t sequencer_locked;

static const char *title_text = "Step Sequencer";
static const char *guide_text = "Tap panels to rotate groove scenes";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {141, 0}};
static const egui_color_t live_status_color = EGUI_COLOR_HEX(0x53C9FF);
static const egui_color_t preview_status_color = EGUI_COLOR_HEX(0x56D0A6);
static const egui_color_t warn_status_color = EGUI_COLOR_HEX(0xF5B24B);
static const egui_color_t lock_status_color = EGUI_COLOR_HEX(0xE0AE73);

static const egui_view_step_sequencer_snapshot_t primary_snapshots[] = {
        {"STEP GRID", "LIVE", "Groove A", "swing +4", {0x89, 0x24, 0x92}, 2, 0, 0},
        {"STEP GRID", "EDIT", "Groove B", "fill armed", {0xAA, 0x52, 0x24}, 5, 1, 1},
        {"STEP GRID", "LIVE", "Groove C", "ghost hat", {0x91, 0x11, 0xE4}, 6, 2, 0},
};

static const egui_view_step_sequencer_snapshot_t preview_snapshots[] = {
        {"PRE", "ARM", "Scene 1", "queued", {0x15, 0x2A, 0x00}, 1, 0, 0},
        {"PRE", "SCAN", "Scene 2", "trace", {0x0F, 0x30, 0x00}, 3, 1, 1},
        {"PRE", "ARM", "Scene 3", "queued", {0x21, 0x12, 0x00}, 5, 2, 0},
};

static const egui_view_step_sequencer_snapshot_t locked_snapshots[] = {
        {"LOCK", "HOLD", "Bank A", "sealed", {0x81, 0x18, 0x42}, 2, 1, 2},
        {"LOCK", "WARN", "Bank B", "guard", {0x48, 0x24, 0x81}, 5, 2, 1},
        {"LOCK", "HOLD", "Bank C", "sealed", {0x24, 0x42, 0x18}, 6, 0, 2},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_step_sequencer_get_current_snapshot(self) + 1) % 3;
    egui_view_step_sequencer_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Primary groove edit", warn_status_color);
    }
    else
    {
        set_status("Primary groove live", live_status_color);
    }
}

static void on_preview_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_step_sequencer_get_current_snapshot(self) + 1) % 3;
    egui_view_step_sequencer_set_current_snapshot(self, next);
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

    next = (egui_view_step_sequencer_get_current_snapshot(self) + 1) % 3;
    egui_view_step_sequencer_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Locked bank warn", warn_status_color);
    }
    else
    {
        set_status("Locked bank hold", lock_status_color);
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

    egui_view_step_sequencer_init(EGUI_VIEW_OF(&sequencer_primary));
    egui_view_set_size(EGUI_VIEW_OF(&sequencer_primary), 192, 118);
    egui_view_step_sequencer_set_snapshots(EGUI_VIEW_OF(&sequencer_primary), primary_snapshots, 3);
    egui_view_step_sequencer_set_font(EGUI_VIEW_OF(&sequencer_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&sequencer_primary), 0, 0, 0, 6);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&sequencer_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&sequencer_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 220, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Primary groove live");
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

    egui_view_step_sequencer_init(EGUI_VIEW_OF(&sequencer_preview));
    egui_view_set_size(EGUI_VIEW_OF(&sequencer_preview), 108, 80);
    egui_view_step_sequencer_set_snapshots(EGUI_VIEW_OF(&sequencer_preview), preview_snapshots, 3);
    egui_view_step_sequencer_set_font(EGUI_VIEW_OF(&sequencer_preview), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_step_sequencer_set_compact_mode(EGUI_VIEW_OF(&sequencer_preview), 1);
    egui_view_step_sequencer_set_palette(EGUI_VIEW_OF(&sequencer_preview), EGUI_COLOR_HEX(0x0F1F20), EGUI_COLOR_HEX(0x173032), EGUI_COLOR_HEX(0x2F6869),
                                         EGUI_COLOR_HEX(0xE0FBF6), EGUI_COLOR_HEX(0x83B2AC), EGUI_COLOR_HEX(0x56D0A6), EGUI_COLOR_HEX(0xE2A852),
                                         EGUI_COLOR_HEX(0xD1A06E), EGUI_COLOR_HEX(0x7EEFD0));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&sequencer_preview), on_preview_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&sequencer_preview));

    egui_view_step_sequencer_init(EGUI_VIEW_OF(&sequencer_locked));
    egui_view_set_size(EGUI_VIEW_OF(&sequencer_locked), 100, 80);
    egui_view_step_sequencer_set_snapshots(EGUI_VIEW_OF(&sequencer_locked), locked_snapshots, 3);
    egui_view_step_sequencer_set_font(EGUI_VIEW_OF(&sequencer_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_step_sequencer_set_compact_mode(EGUI_VIEW_OF(&sequencer_locked), 1);
    egui_view_step_sequencer_set_locked_mode(EGUI_VIEW_OF(&sequencer_locked), 1);
    egui_view_step_sequencer_set_palette(EGUI_VIEW_OF(&sequencer_locked), EGUI_COLOR_HEX(0x241A13), EGUI_COLOR_HEX(0x31251C), EGUI_COLOR_HEX(0x6B513A),
                                         EGUI_COLOR_HEX(0xFFF1DE), EGUI_COLOR_HEX(0xC7A891), EGUI_COLOR_HEX(0xE9B069), EGUI_COLOR_HEX(0xF0B75E),
                                         EGUI_COLOR_HEX(0xE4B478), EGUI_COLOR_HEX(0xF3CC86));
    egui_view_set_margin(EGUI_VIEW_OF(&sequencer_locked), 4, 0, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&sequencer_locked), on_locked_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&sequencer_locked));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&sequencer_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&sequencer_preview), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&sequencer_locked), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
