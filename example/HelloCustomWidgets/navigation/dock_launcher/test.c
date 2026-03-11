#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_dock_launcher.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_dock_launcher_t dock_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_dock_launcher_t dock_favorites;
static egui_view_dock_launcher_t dock_recent;

static const char *title_text = "Dock Launcher";
static const char *guide_text = "Tap cards to rotate launcher states";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {141, 0}};
static const egui_color_t focus_status_color = EGUI_COLOR_HEX(0x67D6FF);
static const egui_color_t ready_status_color = EGUI_COLOR_HEX(0x61D9A9);
static const egui_color_t warn_status_color = EGUI_COLOR_HEX(0xF2B05E);
static const egui_color_t lock_status_color = EGUI_COLOR_HEX(0xD6A36E);

static const egui_view_dock_launcher_snapshot_t primary_snapshots[] = {
        {"DOCK", "FOCUS", "Creative lane armed", "center tools online", 2, 0x14, 2, 0},
        {"DOCK", "LIVE", "Capture route running", "stream queue active", 3, 0x1C, 3, 1},
        {"DOCK", "READY", "Playback lane standby", "recent launch pinned", 1, 0x06, 1, 0},
};

static const egui_view_dock_launcher_snapshot_t favorites_snapshots[] = {
        {"FAV", "STAR", "Pinned tools", "starred", 1, 0x03, 1, 0},
        {"FAV", "SCAN", "Review lane", "sorting", 2, 0x06, 2, 1},
        {"FAV", "STAR", "Pinned set", "saved", 0, 0x01, 1, 0},
};

static const egui_view_dock_launcher_snapshot_t recent_snapshots[] = {
        {"REC", "SAFE", "Last launch", "sealed", 1, 0x02, 1, 2},
        {"REC", "WARN", "Return queue", "guard", 2, 0x04, 2, 1},
        {"REC", "SAFE", "Recall dock", "sealed", 0, 0x01, 1, 2},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_dock_launcher_get_current_snapshot(self) + 1) % 3;
    egui_view_dock_launcher_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Primary dock live", warn_status_color);
    }
    else if (next == 2)
    {
        set_status("Primary dock ready", ready_status_color);
    }
    else
    {
        set_status("Primary dock focus", focus_status_color);
    }
}

static void on_favorites_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_dock_launcher_get_current_snapshot(self) + 1) % 3;
    egui_view_dock_launcher_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Favorites scanning", warn_status_color);
    }
    else
    {
        set_status("Favorites pinned", ready_status_color);
    }
}

static void on_recent_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_dock_launcher_get_current_snapshot(self) + 1) % 3;
    egui_view_dock_launcher_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Recent dock warn", warn_status_color);
    }
    else
    {
        set_status("Recent dock safe", lock_status_color);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), focus_status_color, EGUI_ALPHA_100);
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

    egui_view_dock_launcher_init(EGUI_VIEW_OF(&dock_primary));
    egui_view_set_size(EGUI_VIEW_OF(&dock_primary), 192, 118);
    egui_view_dock_launcher_set_snapshots(EGUI_VIEW_OF(&dock_primary), primary_snapshots, 3);
    egui_view_dock_launcher_set_font(EGUI_VIEW_OF(&dock_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&dock_primary), 0, 0, 0, 6);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&dock_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&dock_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 220, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Primary dock focus");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), focus_status_color, EGUI_ALPHA_100);
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

    egui_view_dock_launcher_init(EGUI_VIEW_OF(&dock_favorites));
    egui_view_set_size(EGUI_VIEW_OF(&dock_favorites), 108, 80);
    egui_view_dock_launcher_set_snapshots(EGUI_VIEW_OF(&dock_favorites), favorites_snapshots, 3);
    egui_view_dock_launcher_set_font(EGUI_VIEW_OF(&dock_favorites), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_dock_launcher_set_compact_mode(EGUI_VIEW_OF(&dock_favorites), 1);
    egui_view_dock_launcher_set_palette(EGUI_VIEW_OF(&dock_favorites), EGUI_COLOR_HEX(0x122117), EGUI_COLOR_HEX(0x1A3122), EGUI_COLOR_HEX(0x3D6D58),
                                        EGUI_COLOR_HEX(0xECFFF5), EGUI_COLOR_HEX(0x8DB6A2), EGUI_COLOR_HEX(0x61D9A9), EGUI_COLOR_HEX(0xE1B35C),
                                        EGUI_COLOR_HEX(0xCCAA78), EGUI_COLOR_HEX(0x98F2C7));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&dock_favorites), on_favorites_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&dock_favorites));

    egui_view_dock_launcher_init(EGUI_VIEW_OF(&dock_recent));
    egui_view_set_size(EGUI_VIEW_OF(&dock_recent), 100, 80);
    egui_view_dock_launcher_set_snapshots(EGUI_VIEW_OF(&dock_recent), recent_snapshots, 3);
    egui_view_dock_launcher_set_font(EGUI_VIEW_OF(&dock_recent), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_dock_launcher_set_compact_mode(EGUI_VIEW_OF(&dock_recent), 1);
    egui_view_dock_launcher_set_locked_mode(EGUI_VIEW_OF(&dock_recent), 1);
    egui_view_dock_launcher_set_palette(EGUI_VIEW_OF(&dock_recent), EGUI_COLOR_HEX(0x261B14), EGUI_COLOR_HEX(0x35261D), EGUI_COLOR_HEX(0x6B5644),
                                        EGUI_COLOR_HEX(0xFFF2E6), EGUI_COLOR_HEX(0xD2AE93), EGUI_COLOR_HEX(0xF0BC7E), EGUI_COLOR_HEX(0xF2B05E),
                                        EGUI_COLOR_HEX(0xD6A36E), EGUI_COLOR_HEX(0xFFD7AA));
    egui_view_set_margin(EGUI_VIEW_OF(&dock_recent), 4, 0, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&dock_recent), on_recent_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&dock_recent));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&dock_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&dock_favorites), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&dock_recent), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
