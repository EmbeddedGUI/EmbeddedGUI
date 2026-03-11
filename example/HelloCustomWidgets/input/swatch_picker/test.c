#include <stdlib.h>

#include "egui.h"
#include "egui_view_swatch_picker.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_swatch_picker_t picker_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_swatch_picker_t picker_compact;
static egui_view_label_t compact_label;
static egui_view_linearlayout_t locked_column;
static egui_view_swatch_picker_t picker_locked;
static egui_view_label_t locked_label;

static const char *title_text = "Swatch Scenes";
static const char *guide_text = "Tap cards to cycle";
static const egui_view_line_point_t section_divider_points[] = {{0, 0}, {169, 0}};

static const char *primary_labels_a[] = {"Sky", "Mint", "Rose", "Sand", "Ink", "Lime"};
static const egui_color_t primary_colors_a[] = {EGUI_COLOR_HEX(0x38BDF8), EGUI_COLOR_HEX(0x34D399), EGUI_COLOR_HEX(0xFB7185),
                                                EGUI_COLOR_HEX(0xF59E0B), EGUI_COLOR_HEX(0x475569), EGUI_COLOR_HEX(0x84CC16)};
static const char *primary_labels_b[] = {"Ice", "Sun", "Coral", "Plum", "Teal", "Ash"};
static const egui_color_t primary_colors_b[] = {EGUI_COLOR_HEX(0x67E8F9), EGUI_COLOR_HEX(0xFBBF24), EGUI_COLOR_HEX(0xF97316),
                                                EGUI_COLOR_HEX(0xA855F7), EGUI_COLOR_HEX(0x14B8A6), EGUI_COLOR_HEX(0x94A3B8)};
static const egui_view_swatch_picker_snapshot_t primary_snapshots[] = {
        {"Palette A", primary_labels_a, primary_colors_a, 6, 2},
        {"Palette B", primary_labels_b, primary_colors_b, 6, 3},
};

static const char *compact_labels_a[] = {"A", "B", "C", "D", "E", "F"};
static const egui_color_t compact_colors_a[] = {EGUI_COLOR_HEX(0x60A5FA), EGUI_COLOR_HEX(0xF59E0B), EGUI_COLOR_HEX(0xFB7185),
                                                EGUI_COLOR_HEX(0x34D399), EGUI_COLOR_HEX(0xA78BFA), EGUI_COLOR_HEX(0xCBD5E1)};
static const char *compact_labels_b[] = {"G", "H", "I", "J", "K", "L"};
static const egui_color_t compact_colors_b[] = {EGUI_COLOR_HEX(0x22D3EE), EGUI_COLOR_HEX(0xF97316), EGUI_COLOR_HEX(0xE879F9),
                                                EGUI_COLOR_HEX(0x84CC16), EGUI_COLOR_HEX(0xF87171), EGUI_COLOR_HEX(0x94A3B8)};
static const egui_view_swatch_picker_snapshot_t compact_snapshots[] = {
        {"Compact A", compact_labels_a, compact_colors_a, 6, 1},
        {"Compact B", compact_labels_b, compact_colors_b, 6, 4},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void apply_compact_state(uint8_t index, uint8_t is_active)
{
    EGUI_UNUSED(index);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_swatch_picker_set_focus_item(EGUI_VIEW_OF(&picker_compact), (index % 2) == 0 ? 1 : 4);
    if (is_active)
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0xF2B226), EGUI_ALPHA_100);
        egui_view_swatch_picker_set_palette(EGUI_VIEW_OF(&picker_compact), EGUI_COLOR_HEX(0x24170B), EGUI_COLOR_HEX(0x9A651B), EGUI_COLOR_HEX(0xFDE7A4),
                                            EGUI_COLOR_HEX(0xC89A57), EGUI_COLOR_HEX(0xF2B226));
    }
    else
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0x8EACC8), EGUI_ALPHA_100);
        egui_view_swatch_picker_set_palette(EGUI_VIEW_OF(&picker_compact), EGUI_COLOR_HEX(0x111A29), EGUI_COLOR_HEX(0x405A7A), EGUI_COLOR_HEX(0xD8E3F0),
                                            EGUI_COLOR_HEX(0x7E93AD), EGUI_COLOR_HEX(0x67B6FF));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_swatch_picker_get_current_snapshot(self) + 1) % 2;
    egui_view_swatch_picker_set_current_snapshot(self, next);
    egui_view_swatch_picker_set_focus_item(self, next == 0 ? 2 : 3);
    apply_compact_state(egui_view_swatch_picker_get_current_snapshot(EGUI_VIEW_OF(&picker_compact)), 0);
    set_status(next == 0 ? "Palette A" : "Palette B", EGUI_COLOR_HEX(0x38BDF8));
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_swatch_picker_get_current_snapshot(self) + 1) % 2;
    egui_view_swatch_picker_set_current_snapshot(self, next);
    apply_compact_state(next, 1);
    set_status(next == 0 ? "Compact A" : "Compact B", EGUI_COLOR_HEX(0xF59E0B));
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 226, 308);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 226, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x4CC7FF), EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 226, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x6F839A), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_swatch_picker_init(EGUI_VIEW_OF(&picker_primary));
    egui_view_set_size(EGUI_VIEW_OF(&picker_primary), 188, 140);
    egui_view_swatch_picker_set_snapshots(EGUI_VIEW_OF(&picker_primary), primary_snapshots, 2);
    egui_view_swatch_picker_set_current_snapshot(EGUI_VIEW_OF(&picker_primary), 0);
    egui_view_swatch_picker_set_focus_item(EGUI_VIEW_OF(&picker_primary), 2);
    egui_view_swatch_picker_set_palette(EGUI_VIEW_OF(&picker_primary), EGUI_COLOR_HEX(0x0F1728), EGUI_COLOR_HEX(0x536379), EGUI_COLOR_HEX(0xE2E8F0),
                                        EGUI_COLOR_HEX(0x93A5BC), EGUI_COLOR_HEX(0x38BDF8));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&picker_primary), on_primary_click);
    egui_view_set_margin(EGUI_VIEW_OF(&picker_primary), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&picker_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 226, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Palette A");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(0x4CC7FF), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 170, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), section_divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0x4C77A3));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 226, 108);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), 108, 110);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, 5, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), 108, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_swatch_picker_init(EGUI_VIEW_OF(&picker_compact));
    egui_view_set_size(EGUI_VIEW_OF(&picker_compact), 108, 94);
    egui_view_swatch_picker_set_snapshots(EGUI_VIEW_OF(&picker_compact), compact_snapshots, 2);
    egui_view_swatch_picker_set_current_snapshot(EGUI_VIEW_OF(&picker_compact), 0);
    egui_view_swatch_picker_set_font(EGUI_VIEW_OF(&picker_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_swatch_picker_set_show_header(EGUI_VIEW_OF(&picker_compact), 0);
    egui_view_swatch_picker_set_compact_mode(EGUI_VIEW_OF(&picker_compact), 1);
    apply_compact_state(0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&picker_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&picker_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), 108, 110);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 5, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), 108, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Locked");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(0xAAB6C6), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_swatch_picker_init(EGUI_VIEW_OF(&picker_locked));
    egui_view_set_size(EGUI_VIEW_OF(&picker_locked), 108, 94);
    egui_view_swatch_picker_set_snapshots(EGUI_VIEW_OF(&picker_locked), compact_snapshots, 2);
    egui_view_swatch_picker_set_current_snapshot(EGUI_VIEW_OF(&picker_locked), 0);
    egui_view_swatch_picker_set_focus_item(EGUI_VIEW_OF(&picker_locked), 1);
    egui_view_swatch_picker_set_show_header(EGUI_VIEW_OF(&picker_locked), 0);
    egui_view_swatch_picker_set_compact_mode(EGUI_VIEW_OF(&picker_locked), 1);
    egui_view_swatch_picker_set_palette(EGUI_VIEW_OF(&picker_locked), EGUI_COLOR_HEX(0x0E1522), EGUI_COLOR_HEX(0x4B5D71), EGUI_COLOR_HEX(0xC6CED8),
                                        EGUI_COLOR_HEX(0x8694A3), EGUI_COLOR_HEX(0x6B7787));
    egui_view_set_enable(EGUI_VIEW_OF(&picker_locked), 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&picker_locked));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&picker_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&picker_compact), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&picker_locked), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
