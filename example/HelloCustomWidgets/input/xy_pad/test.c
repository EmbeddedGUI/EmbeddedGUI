#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_xy_pad.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_xy_pad_t xy_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_xy_pad_t xy_macro;
static egui_view_xy_pad_t xy_latch;

static const char *title_text = "XY Pad";
static const char *guide_text = "Tap panels to rotate motion scenes";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {141, 0}};
static const egui_color_t live_status_color = EGUI_COLOR_HEX(0x57D2F5);
static const egui_color_t macro_status_color = EGUI_COLOR_HEX(0x58D5A8);
static const egui_color_t warn_status_color = EGUI_COLOR_HEX(0xF2B05E);
static const egui_color_t latch_status_color = EGUI_COLOR_HEX(0xD8AB78);

static const egui_view_xy_pad_snapshot_t primary_snapshots[] = {
        {"XY FIELD", "LIVE", "Filter A", "tilt north", 64, 71, 38, 0},
        {"XY FIELD", "DRAG", "Filter B", "sweep east", 82, 43, 56, 1},
        {"XY FIELD", "LIVE", "Filter C", "glide low", 34, 28, 44, 0},
};

static const egui_view_xy_pad_snapshot_t macro_snapshots[] = {
        {"MAC", "ARM", "Macro 1", "queued", 26, 74, 28, 0},
        {"MAC", "SCAN", "Macro 2", "trace", 53, 51, 34, 1},
        {"MAC", "ARM", "Macro 3", "queued", 73, 29, 26, 0},
};

static const egui_view_xy_pad_snapshot_t latch_snapshots[] = {
        {"LATCH", "SAFE", "Latch A", "sealed", 69, 66, 26, 2},
        {"LATCH", "WARN", "Latch B", "guard", 41, 35, 38, 1},
        {"LATCH", "SAFE", "Latch C", "sealed", 21, 57, 30, 2},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_xy_pad_get_current_snapshot(self) + 1) % 3;
    egui_view_xy_pad_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Primary sweep drag", warn_status_color);
    }
    else
    {
        set_status("Primary field live", live_status_color);
    }
}

static void on_macro_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_xy_pad_get_current_snapshot(self) + 1) % 3;
    egui_view_xy_pad_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Macro lane scanning", warn_status_color);
    }
    else
    {
        set_status("Macro lane armed", macro_status_color);
    }
}

static void on_latch_click(egui_view_t *self)
{
    uint8_t next;

    next = (egui_view_xy_pad_get_current_snapshot(self) + 1) % 3;
    egui_view_xy_pad_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Latch bank warn", warn_status_color);
    }
    else
    {
        set_status("Latch bank safe", latch_status_color);
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

    egui_view_xy_pad_init(EGUI_VIEW_OF(&xy_primary));
    egui_view_set_size(EGUI_VIEW_OF(&xy_primary), 192, 118);
    egui_view_xy_pad_set_snapshots(EGUI_VIEW_OF(&xy_primary), primary_snapshots, 3);
    egui_view_xy_pad_set_font(EGUI_VIEW_OF(&xy_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&xy_primary), 0, 0, 0, 6);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&xy_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&xy_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 220, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Primary field live");
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

    egui_view_xy_pad_init(EGUI_VIEW_OF(&xy_macro));
    egui_view_set_size(EGUI_VIEW_OF(&xy_macro), 108, 80);
    egui_view_xy_pad_set_snapshots(EGUI_VIEW_OF(&xy_macro), macro_snapshots, 3);
    egui_view_xy_pad_set_font(EGUI_VIEW_OF(&xy_macro), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_xy_pad_set_compact_mode(EGUI_VIEW_OF(&xy_macro), 1);
    egui_view_xy_pad_set_palette(
            EGUI_VIEW_OF(&xy_macro),
            EGUI_COLOR_HEX(0x0F201A),
            EGUI_COLOR_HEX(0x173127),
            EGUI_COLOR_HEX(0x34685A),
            EGUI_COLOR_HEX(0xE6FBF1),
            EGUI_COLOR_HEX(0x8FB5AB),
            EGUI_COLOR_HEX(0x58D5A8),
            EGUI_COLOR_HEX(0xE2AE57),
            EGUI_COLOR_HEX(0xD0A56F),
            EGUI_COLOR_HEX(0x88F1C9));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&xy_macro), on_macro_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&xy_macro));

    egui_view_xy_pad_init(EGUI_VIEW_OF(&xy_latch));
    egui_view_set_size(EGUI_VIEW_OF(&xy_latch), 100, 80);
    egui_view_xy_pad_set_snapshots(EGUI_VIEW_OF(&xy_latch), latch_snapshots, 3);
    egui_view_xy_pad_set_font(EGUI_VIEW_OF(&xy_latch), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_xy_pad_set_compact_mode(EGUI_VIEW_OF(&xy_latch), 1);
    egui_view_xy_pad_set_locked_mode(EGUI_VIEW_OF(&xy_latch), 1);
    egui_view_xy_pad_set_palette(
            EGUI_VIEW_OF(&xy_latch),
            EGUI_COLOR_HEX(0x241A14),
            EGUI_COLOR_HEX(0x33261D),
            EGUI_COLOR_HEX(0x6D5542),
            EGUI_COLOR_HEX(0xFFF1E2),
            EGUI_COLOR_HEX(0xD1AB92),
            EGUI_COLOR_HEX(0xF0BF79),
            EGUI_COLOR_HEX(0xF2B05E),
            EGUI_COLOR_HEX(0xD8AB78),
            EGUI_COLOR_HEX(0xFFD4A0));
    egui_view_set_margin(EGUI_VIEW_OF(&xy_latch), 4, 0, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&xy_latch), on_latch_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&xy_latch));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&xy_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&xy_macro), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&xy_latch), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
