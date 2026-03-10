#include <stdlib.h>

#include "egui.h"
#include "egui_view_level_meter.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_level_meter_t meter_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_level_meter_t meter_compact;
static egui_view_label_t compact_label;
static egui_view_linearlayout_t locked_column;
static egui_view_level_meter_t meter_locked;
static egui_view_label_t locked_label;

static const char *title_text = "Level Meter";
static const char *guide_text = "Tap cards to cycle";
static const egui_view_line_point_t section_divider_points[] = {{0, 0}, {171, 0}};

static const egui_view_level_meter_channel_t primary_channels_a[] = {
        {"K", 28, 34, 0}, {"S", 44, 58, 1}, {"VX", 72, 84, 2},
        {"BS", 56, 64, 1}, {"PD", 38, 46, 0}};
static const egui_view_level_meter_channel_t primary_channels_b[] = {
        {"K", 36, 42, 0}, {"S", 62, 74, 2}, {"VX", 48, 60, 1},
        {"BS", 78, 88, 2}, {"PD", 42, 50, 0}};
static const egui_view_level_meter_snapshot_t primary_snapshots[] = {
        {"Bus A", primary_channels_a, 5, 2},
        {"Bus B", primary_channels_b, 5, 3},
};

static const egui_view_level_meter_channel_t compact_channels_a[] = {
        {"L", 26, 32, 0}, {"M", 58, 68, 1}, {"H", 76, 84, 2}, {"FX", 44, 52, 0}};
static const egui_view_level_meter_channel_t compact_channels_b[] = {
        {"L", 42, 50, 1}, {"M", 72, 82, 2}, {"H", 34, 42, 0}, {"FX", 64, 74, 1}};
static const egui_view_level_meter_snapshot_t compact_snapshots[] = {
        {"Compact A", compact_channels_a, 4, 2},
        {"Compact B", compact_channels_b, 4, 1},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void apply_compact_state(uint8_t index, uint8_t is_active)
{
    egui_view_level_meter_set_focus_channel(EGUI_VIEW_OF(&meter_compact), (index % 2) == 0 ? 2 : 1);
    if (is_active)
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0xF7C15A), EGUI_ALPHA_100);
        egui_view_level_meter_set_palette(
                EGUI_VIEW_OF(&meter_compact),
                EGUI_COLOR_HEX(0x261A0D),
                EGUI_COLOR_HEX(0xB17831),
                EGUI_COLOR_HEX(0xFDE7A1),
                EGUI_COLOR_HEX(0xD8AF67),
                EGUI_COLOR_HEX(0xF59E0B));
    }
    else
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0xAFBCCC), EGUI_ALPHA_100);
        egui_view_level_meter_set_palette(
                EGUI_VIEW_OF(&meter_compact),
                EGUI_COLOR_HEX(0x111A28),
                EGUI_COLOR_HEX(0x5A7693),
                EGUI_COLOR_HEX(0xDEE7F1),
                EGUI_COLOR_HEX(0x97AABD),
                EGUI_COLOR_HEX(0x60A5FA));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_level_meter_get_current_snapshot(self) + 1) % 2;
    egui_view_level_meter_set_current_snapshot(self, next);
    egui_view_level_meter_set_focus_channel(self, next == 0 ? 2 : 3);
    apply_compact_state(egui_view_level_meter_get_current_snapshot(EGUI_VIEW_OF(&meter_compact)), 0);
    set_status(next == 0 ? "Bus A" : "Bus B", EGUI_COLOR_HEX(0x4FD1FF));
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_level_meter_get_current_snapshot(self) + 1) % 2;
    egui_view_level_meter_set_current_snapshot(self, next);
    apply_compact_state(next, 1);
    set_status(next == 0 ? "Compact A" : "Compact B", EGUI_COLOR_HEX(0xF59E0B));
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 230, 310);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 230, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x68DCFF), EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 230, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x8CA6C0), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_level_meter_init(EGUI_VIEW_OF(&meter_primary));
    egui_view_set_size(EGUI_VIEW_OF(&meter_primary), 190, 142);
    egui_view_level_meter_set_snapshots(EGUI_VIEW_OF(&meter_primary), primary_snapshots, 2);
    egui_view_level_meter_set_current_snapshot(EGUI_VIEW_OF(&meter_primary), 0);
    egui_view_level_meter_set_focus_channel(EGUI_VIEW_OF(&meter_primary), 2);
    egui_view_level_meter_set_palette(
            EGUI_VIEW_OF(&meter_primary),
            EGUI_COLOR_HEX(0x0F1728),
            EGUI_COLOR_HEX(0x597088),
            EGUI_COLOR_HEX(0xE2E8F0),
            EGUI_COLOR_HEX(0x93A5BC),
            EGUI_COLOR_HEX(0x4FD1FF));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&meter_primary), on_primary_click);
    egui_view_set_margin(EGUI_VIEW_OF(&meter_primary), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&meter_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 230, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Bus A");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(0x6AE5FF), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 172, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), section_divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0x6398C8));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 6);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 230, 110);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), 110, 112);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, 6, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), 110, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_level_meter_init(EGUI_VIEW_OF(&meter_compact));
    egui_view_set_size(EGUI_VIEW_OF(&meter_compact), 110, 96);
    egui_view_level_meter_set_snapshots(EGUI_VIEW_OF(&meter_compact), compact_snapshots, 2);
    egui_view_level_meter_set_current_snapshot(EGUI_VIEW_OF(&meter_compact), 0);
    egui_view_level_meter_set_font(EGUI_VIEW_OF(&meter_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_level_meter_set_show_header(EGUI_VIEW_OF(&meter_compact), 0);
    egui_view_level_meter_set_compact_mode(EGUI_VIEW_OF(&meter_compact), 1);
    apply_compact_state(0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&meter_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&meter_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), 110, 112);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), 110, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Locked");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(0xC8D2DE), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_level_meter_init(EGUI_VIEW_OF(&meter_locked));
    egui_view_set_size(EGUI_VIEW_OF(&meter_locked), 110, 96);
    egui_view_level_meter_set_snapshots(EGUI_VIEW_OF(&meter_locked), compact_snapshots, 2);
    egui_view_level_meter_set_current_snapshot(EGUI_VIEW_OF(&meter_locked), 0);
    egui_view_level_meter_set_focus_channel(EGUI_VIEW_OF(&meter_locked), 2);
    egui_view_level_meter_set_show_header(EGUI_VIEW_OF(&meter_locked), 0);
    egui_view_level_meter_set_compact_mode(EGUI_VIEW_OF(&meter_locked), 1);
    egui_view_level_meter_set_palette(
            EGUI_VIEW_OF(&meter_locked),
            EGUI_COLOR_HEX(0x0E1522),
            EGUI_COLOR_HEX(0x556477),
            EGUI_COLOR_HEX(0xD5E0EA),
            EGUI_COLOR_HEX(0x92A2B5),
            EGUI_COLOR_HEX(0x748680));
    egui_view_set_enable(EGUI_VIEW_OF(&meter_locked), 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&meter_locked));

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
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 400);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&meter_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&meter_compact), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&meter_locked), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
