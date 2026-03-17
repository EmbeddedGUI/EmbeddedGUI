#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_clip_launcher_grid.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_clip_launcher_grid_t grid_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_clip_launcher_grid_t grid_compact;
static egui_view_clip_launcher_grid_t grid_locked;

static const char *title_text = "Clip Launcher";
static const char *guide_text = "Tap panels to rotate queued scenes";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {145, 0}};
static const egui_color_t live_status_color = EGUI_COLOR_HEX(0x63C8FF);
static const egui_color_t queue_status_color = EGUI_COLOR_HEX(0xF3B34D);
static const egui_color_t play_status_color = EGUI_COLOR_HEX(0x5CE2AF);
static const egui_color_t lock_status_color = EGUI_COLOR_HEX(0xD9A66F);

static const egui_view_clip_launcher_grid_snapshot_t primary_snapshots[] = {
        {"CLIPS", "LIVE", "Bass armed", "C queued", {0x0B, 0x0E, 0x07, 0x0D}, 1, 2, 1, 1, 0},
        {"CLIPS", "QUEUE", "Vox focus", "D next", {0x03, 0x0D, 0x0E, 0x06}, 2, 3, 2, 2, 1},
        {"CLIPS", "LIVE", "Drums armed", "A relaunch", {0x09, 0x07, 0x0D, 0x0B}, 0, 0, 0, 3, 0},
};

static const egui_view_clip_launcher_grid_snapshot_t compact_snapshots[] = {
        {"PRE", "ARM", "Scene C", "queue", {0x0B, 0x05, 0x06, 0x09}, 1, 2, 2, 0, 1},
        {"PRE", "SCAN", "Scene D", "trace", {0x03, 0x09, 0x0C, 0x06}, 2, 3, 3, 3, 0},
        {"PRE", "ARM", "Scene B", "ready", {0x06, 0x03, 0x0B, 0x0C}, 0, 1, 1, 2, 1},
};

static const egui_view_clip_launcher_grid_snapshot_t locked_snapshots[] = {
        {"LOCK", "HOLD", "Preview", "sealed", {0x09, 0x05, 0x03, 0x0B}, 0, 1, 0, 0, 2},
        {"LOCK", "WARN", "Recall", "guard", {0x06, 0x09, 0x05, 0x0C}, 2, 2, 1, 1, 2},
        {"LOCK", "HOLD", "Recall", "sealed", {0x03, 0x06, 0x09, 0x05}, 3, 0, 2, 2, 2},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_clip_launcher_grid_get_current_snapshot(self) + 1) % 3;
    egui_view_clip_launcher_grid_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Primary scene queued", queue_status_color);
    }
    else
    {
        set_status("Primary scene live", play_status_color);
    }
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_clip_launcher_grid_get_current_snapshot(self) + 1) % 3;
    egui_view_clip_launcher_grid_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Compact scan ready", queue_status_color);
    }
    else
    {
        set_status("Compact queue armed", live_status_color);
    }
}

static void on_locked_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_clip_launcher_grid_get_current_snapshot(self) + 1) % 3;
    egui_view_clip_launcher_grid_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Locked recall warn", queue_status_color);
    }
    else
    {
        set_status("Locked recall hold", lock_status_color);
    }
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 306);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 224, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), live_status_color, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 224, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x74879D), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_clip_launcher_grid_init(EGUI_VIEW_OF(&grid_primary));
    egui_view_set_size(EGUI_VIEW_OF(&grid_primary), 194, 126);
    egui_view_clip_launcher_grid_set_snapshots(EGUI_VIEW_OF(&grid_primary), primary_snapshots, 3);
    egui_view_clip_launcher_grid_set_font(EGUI_VIEW_OF(&grid_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&grid_primary), 0, 0, 0, 6);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&grid_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&grid_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Primary scene live");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), play_status_color, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 146, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0x30465C));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 212, 88);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_clip_launcher_grid_init(EGUI_VIEW_OF(&grid_compact));
    egui_view_set_size(EGUI_VIEW_OF(&grid_compact), 108, 84);
    egui_view_clip_launcher_grid_set_snapshots(EGUI_VIEW_OF(&grid_compact), compact_snapshots, 3);
    egui_view_clip_launcher_grid_set_font(EGUI_VIEW_OF(&grid_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_clip_launcher_grid_set_compact_mode(EGUI_VIEW_OF(&grid_compact), 1);
    egui_view_clip_launcher_grid_set_palette(EGUI_VIEW_OF(&grid_compact), EGUI_COLOR_HEX(0x0F1E22), EGUI_COLOR_HEX(0x173136), EGUI_COLOR_HEX(0x2B5F64),
                                             EGUI_COLOR_HEX(0xE1FBF5), EGUI_COLOR_HEX(0x86B8B0), EGUI_COLOR_HEX(0x5FC5FF), EGUI_COLOR_HEX(0xF3B34D),
                                             EGUI_COLOR_HEX(0xD9A66F), EGUI_COLOR_HEX(0x5CE2AF));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&grid_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&grid_compact));

    egui_view_clip_launcher_grid_init(EGUI_VIEW_OF(&grid_locked));
    egui_view_set_size(EGUI_VIEW_OF(&grid_locked), 100, 84);
    egui_view_clip_launcher_grid_set_snapshots(EGUI_VIEW_OF(&grid_locked), locked_snapshots, 3);
    egui_view_clip_launcher_grid_set_font(EGUI_VIEW_OF(&grid_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_clip_launcher_grid_set_compact_mode(EGUI_VIEW_OF(&grid_locked), 1);
    egui_view_clip_launcher_grid_set_locked_mode(EGUI_VIEW_OF(&grid_locked), 1);
    egui_view_clip_launcher_grid_set_palette(EGUI_VIEW_OF(&grid_locked), EGUI_COLOR_HEX(0x241A14), EGUI_COLOR_HEX(0x30241D), EGUI_COLOR_HEX(0x6B4E3A),
                                             EGUI_COLOR_HEX(0xFFF1DF), EGUI_COLOR_HEX(0xCFB197), EGUI_COLOR_HEX(0xE4B478), EGUI_COLOR_HEX(0xF0B75E),
                                             EGUI_COLOR_HEX(0xD9A66F), EGUI_COLOR_HEX(0xEFD49A));
    egui_view_set_margin(EGUI_VIEW_OF(&grid_locked), 4, 0, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&grid_locked), on_locked_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&grid_locked));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&grid_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&grid_compact), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&grid_locked), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
