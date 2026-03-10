#include <stdlib.h>

#include "egui.h"
#include "egui_view_waveform_strip.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_waveform_strip_t waveform_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_waveform_strip_t waveform_compact;
static egui_view_label_t compact_label;
static egui_view_linearlayout_t locked_column;
static egui_view_waveform_strip_t waveform_locked;
static egui_view_label_t locked_label;

static const char *title_text = "Waveform Strip";
static const char *guide_text = "Tap strips to rotate cue";
static const egui_view_line_point_t section_divider_points[] = {{0, 0}, {147, 0}};

static const uint8_t primary_bars_a[] = {12, 18, 26, 20, 14, 10, 24, 30, 22, 16, 12, 20, 26, 18, 10, 8};
static const uint8_t primary_bars_b[] = {8, 12, 20, 28, 18, 14, 24, 16, 10, 18, 30, 24, 14, 10, 16, 22};
static const egui_view_waveform_strip_snapshot_t primary_snapshots[] = {
        {"Track A", primary_bars_a, 16, 7, 12},
        {"Track B", primary_bars_b, 16, 10, 3},
};

static const uint8_t compact_bars_a[] = {8, 14, 20, 12, 18, 10, 16, 22, 14, 8};
static const uint8_t compact_bars_b[] = {10, 18, 12, 20, 16, 8, 14, 22, 18, 10};
static const egui_view_waveform_strip_snapshot_t compact_snapshots[] = {
        {"Compact A", compact_bars_a, 10, 4, 7},
        {"Compact B", compact_bars_b, 10, 7, 2},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void apply_compact_state(uint8_t index, uint8_t is_active)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), (index % 2) == 0 ? "Compact A" : "Compact B");
    egui_view_waveform_strip_set_focus_bar(EGUI_VIEW_OF(&waveform_compact), (index % 2) == 0 ? 7 : 2);
    if (is_active)
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0xF59E0B), EGUI_ALPHA_100);
        egui_view_waveform_strip_set_palette(
                EGUI_VIEW_OF(&waveform_compact),
                EGUI_COLOR_HEX(0x23160A),
                EGUI_COLOR_HEX(0x8B5E1A),
                EGUI_COLOR_HEX(0xFDE68A),
                EGUI_COLOR_HEX(0xD6A15B),
                EGUI_COLOR_HEX(0xF59E0B));
    }
    else
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0x8B5E1A), EGUI_ALPHA_100);
        egui_view_waveform_strip_set_palette(
                EGUI_VIEW_OF(&waveform_compact),
                EGUI_COLOR_HEX(0x121A28),
                EGUI_COLOR_HEX(0x3D5068),
                EGUI_COLOR_HEX(0xCBD5E1),
                EGUI_COLOR_HEX(0x7F92A7),
                EGUI_COLOR_HEX(0x60A5FA));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_waveform_strip_get_current_snapshot(self) + 1) % 2;
    egui_view_waveform_strip_set_current_snapshot(self, next);
    egui_view_waveform_strip_set_focus_bar(self, next == 0 ? 12 : 3);
    apply_compact_state(egui_view_waveform_strip_get_current_snapshot(EGUI_VIEW_OF(&waveform_compact)), 0);
    set_status(next == 0 ? "Track A cue" : "Track B cue", EGUI_COLOR_HEX(0x38BDF8));
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_waveform_strip_get_current_snapshot(self) + 1) % 2;
    egui_view_waveform_strip_set_current_snapshot(self, next);
    apply_compact_state(next, 1);
    set_status(next == 0 ? "Compact A wave" : "Compact B wave", EGUI_COLOR_HEX(0xF59E0B));
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 220, 304);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 220, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x38BDF8), EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 220, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x64748B), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_waveform_strip_init(EGUI_VIEW_OF(&waveform_primary));
    egui_view_set_size(EGUI_VIEW_OF(&waveform_primary), 176, 132);
    egui_view_waveform_strip_set_snapshots(EGUI_VIEW_OF(&waveform_primary), primary_snapshots, 2);
    egui_view_waveform_strip_set_current_snapshot(EGUI_VIEW_OF(&waveform_primary), 0);
    egui_view_waveform_strip_set_focus_bar(EGUI_VIEW_OF(&waveform_primary), 12);
    egui_view_waveform_strip_set_palette(
            EGUI_VIEW_OF(&waveform_primary),
            EGUI_COLOR_HEX(0x0F1728),
            EGUI_COLOR_HEX(0x536379),
            EGUI_COLOR_HEX(0xE2E8F0),
            EGUI_COLOR_HEX(0x93A5BC),
            EGUI_COLOR_HEX(0x38BDF8));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&waveform_primary), on_primary_click);
    egui_view_set_margin(EGUI_VIEW_OF(&waveform_primary), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&waveform_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 220, 14);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Track A cue");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(0x38BDF8), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 148, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), section_divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0x2C425E));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 220, 104);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), 106, 108);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, 4, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), 106, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact A");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_waveform_strip_init(EGUI_VIEW_OF(&waveform_compact));
    egui_view_set_size(EGUI_VIEW_OF(&waveform_compact), 106, 92);
    egui_view_waveform_strip_set_snapshots(EGUI_VIEW_OF(&waveform_compact), compact_snapshots, 2);
    egui_view_waveform_strip_set_current_snapshot(EGUI_VIEW_OF(&waveform_compact), 0);
    egui_view_waveform_strip_set_font(EGUI_VIEW_OF(&waveform_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_waveform_strip_set_show_header(EGUI_VIEW_OF(&waveform_compact), 0);
    egui_view_waveform_strip_set_compact_mode(EGUI_VIEW_OF(&waveform_compact), 1);
    apply_compact_state(0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&waveform_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&waveform_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), 106, 108);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), 106, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Locked");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(0xB8C3D2), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_waveform_strip_init(EGUI_VIEW_OF(&waveform_locked));
    egui_view_set_size(EGUI_VIEW_OF(&waveform_locked), 106, 92);
    egui_view_waveform_strip_set_snapshots(EGUI_VIEW_OF(&waveform_locked), compact_snapshots, 2);
    egui_view_waveform_strip_set_current_snapshot(EGUI_VIEW_OF(&waveform_locked), 0);
    egui_view_waveform_strip_set_focus_bar(EGUI_VIEW_OF(&waveform_locked), 7);
    egui_view_waveform_strip_set_show_header(EGUI_VIEW_OF(&waveform_locked), 0);
    egui_view_waveform_strip_set_compact_mode(EGUI_VIEW_OF(&waveform_locked), 1);
    egui_view_waveform_strip_set_palette(
            EGUI_VIEW_OF(&waveform_locked),
            EGUI_COLOR_HEX(0x0E1522),
            EGUI_COLOR_HEX(0x46566B),
            EGUI_COLOR_HEX(0xCBD5E1),
            EGUI_COLOR_HEX(0x8695A9),
            EGUI_COLOR_HEX(0x758E83));
    egui_view_set_enable(EGUI_VIEW_OF(&waveform_locked), 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&waveform_locked));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&waveform_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&waveform_compact), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&waveform_locked), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
