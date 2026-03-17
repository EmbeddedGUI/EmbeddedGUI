#include <stdlib.h>

#include "egui.h"
#include "egui_view_skeleton.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_skeleton_t skeleton_primary;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t pulse_column;
static egui_view_label_t pulse_label;
static egui_view_skeleton_t skeleton_pulse;
static egui_view_linearlayout_t static_column;
static egui_view_label_t static_label;
static egui_view_skeleton_t skeleton_static;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF7F8FA), EGUI_ALPHA_100, 16);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Skeleton";
static const char *guide_text = "Tap cards to switch loading";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {131, 0}};

static const egui_view_skeleton_block_t primary_blocks_a[] = {
        {0, 1, 22, 22, 11}, {30, 2, 80, 8, 4},  {30, 14, 58, 7, 4}, {0, 34, 176, 20, 7},
        {0, 62, 164, 7, 3}, {0, 74, 170, 7, 3}, {0, 86, 142, 7, 3}, {148, 85, 20, 8, 4},
};
static const egui_view_skeleton_block_t primary_blocks_b[] = {
        {0, 0, 96, 8, 4},   {0, 12, 136, 7, 3}, {0, 28, 52, 18, 7}, {62, 30, 78, 7, 3},  {62, 42, 60, 7, 3},
        {0, 56, 52, 18, 7}, {62, 58, 76, 7, 3}, {62, 70, 58, 7, 3}, {146, 32, 14, 7, 4}, {146, 60, 14, 7, 4},
};
static const egui_view_skeleton_block_t primary_blocks_c[] = {
        {0, 0, 96, 8, 4},   {0, 18, 16, 16, 8}, {24, 20, 76, 6, 3},   {24, 30, 48, 6, 3}, {136, 19, 28, 10, 5}, {0, 42, 16, 16, 8},
        {24, 44, 68, 6, 3}, {24, 54, 50, 6, 3}, {136, 43, 26, 10, 5}, {0, 66, 16, 16, 8}, {24, 68, 82, 6, 3},   {24, 78, 52, 6, 3},
};
static const egui_view_skeleton_snapshot_t primary_snapshots[] = {
        {"Article", "Loading article", primary_blocks_a, 8, 3},
        {"Feed", "Loading feed", primary_blocks_b, 10, 2},
        {"Settings", "Loading settings", primary_blocks_c, 12, 1},
};

static const egui_view_skeleton_block_t pulse_blocks_a[] = {
        {0, 4, 11, 11, 6}, {17, 5, 46, 5, 3}, {17, 13, 32, 5, 3}, {0, 24, 78, 10, 4}, {0, 38, 44, 4, 2},
};
static const egui_view_skeleton_block_t pulse_blocks_b[] = {
        {0, 3, 30, 15, 5},
        {40, 3, 30, 15, 5},
        {0, 25, 74, 6, 3},
        {0, 35, 48, 5, 3},
};
static const egui_view_skeleton_snapshot_t pulse_snapshots[] = {
        {"Pulse row", NULL, pulse_blocks_a, 5, 3},
        {"Pulse tile", NULL, pulse_blocks_b, 4, 1},
};

static const egui_view_skeleton_block_t static_blocks[] = {
        {0, 2, 14, 14, 7}, {20, 4, 36, 5, 3}, {20, 12, 24, 5, 3}, {0, 24, 74, 10, 4}, {0, 36, 52, 4, 2},
};
static const egui_view_skeleton_snapshot_t static_snapshots[] = {
        {"Static", NULL, static_blocks, 5, 3},
};

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_skeleton_get_current_snapshot(self) + 1) % 3;
    uint8_t pulse_index = next == 0 ? 0 : 1;

    egui_view_skeleton_set_current_snapshot(self, next);
    egui_view_skeleton_set_emphasis_block(self, primary_snapshots[next].emphasis_block);
    egui_view_skeleton_set_current_snapshot(EGUI_VIEW_OF(&skeleton_pulse), pulse_index);
    egui_view_skeleton_set_emphasis_block(EGUI_VIEW_OF(&skeleton_pulse), pulse_snapshots[pulse_index].emphasis_block);
}

static void on_pulse_click(egui_view_t *self)
{
    uint8_t next = (egui_view_skeleton_get_current_snapshot(self) + 1) % 2;

    egui_view_skeleton_set_current_snapshot(self, next);
    egui_view_skeleton_set_emphasis_block(self, pulse_snapshots[next].emphasis_block);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 284);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 224, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x223041), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x7A8693), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), 224, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Wave");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(0x87929E), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_skeleton_init(EGUI_VIEW_OF(&skeleton_primary));
    egui_view_set_size(EGUI_VIEW_OF(&skeleton_primary), 196, 124);
    egui_view_skeleton_set_snapshots(EGUI_VIEW_OF(&skeleton_primary), primary_snapshots, 3);
    egui_view_skeleton_set_current_snapshot(EGUI_VIEW_OF(&skeleton_primary), 1);
    egui_view_skeleton_set_emphasis_block(EGUI_VIEW_OF(&skeleton_primary), primary_snapshots[1].emphasis_block);
    egui_view_skeleton_set_font(EGUI_VIEW_OF(&skeleton_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_skeleton_set_animation_mode(EGUI_VIEW_OF(&skeleton_primary), EGUI_VIEW_SKELETON_ANIM_WAVE);
    egui_view_skeleton_set_palette(EGUI_VIEW_OF(&skeleton_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD9E1E8), EGUI_COLOR_HEX(0xE6EDF3),
                                   EGUI_COLOR_HEX(0x5B6D7C), EGUI_COLOR_HEX(0x83909D), EGUI_COLOR_HEX(0x7AA8F0));
    egui_view_set_margin(EGUI_VIEW_OF(&skeleton_primary), 0, 0, 0, 6);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&skeleton_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&skeleton_primary));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 132, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xE6EBF0));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 214, 86);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&pulse_column));
    egui_view_set_size(EGUI_VIEW_OF(&pulse_column), 106, 86);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&pulse_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&pulse_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&pulse_column));

    egui_view_label_init(EGUI_VIEW_OF(&pulse_label));
    egui_view_set_size(EGUI_VIEW_OF(&pulse_label), 106, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&pulse_label), "Pulse");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&pulse_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&pulse_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&pulse_label), EGUI_COLOR_HEX(0x236E69), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&pulse_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&pulse_column), EGUI_VIEW_OF(&pulse_label));

    egui_view_skeleton_init(EGUI_VIEW_OF(&skeleton_pulse));
    egui_view_set_size(EGUI_VIEW_OF(&skeleton_pulse), 106, 60);
    egui_view_skeleton_set_snapshots(EGUI_VIEW_OF(&skeleton_pulse), pulse_snapshots, 2);
    egui_view_skeleton_set_current_snapshot(EGUI_VIEW_OF(&skeleton_pulse), 1);
    egui_view_skeleton_set_emphasis_block(EGUI_VIEW_OF(&skeleton_pulse), pulse_snapshots[1].emphasis_block);
    egui_view_skeleton_set_font(EGUI_VIEW_OF(&skeleton_pulse), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_skeleton_set_compact_mode(EGUI_VIEW_OF(&skeleton_pulse), 1);
    egui_view_skeleton_set_show_footer(EGUI_VIEW_OF(&skeleton_pulse), 0);
    egui_view_skeleton_set_animation_mode(EGUI_VIEW_OF(&skeleton_pulse), EGUI_VIEW_SKELETON_ANIM_PULSE);
    egui_view_skeleton_set_palette(EGUI_VIEW_OF(&skeleton_pulse), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xDAE8E5), EGUI_COLOR_HEX(0xEDF6F4),
                                   EGUI_COLOR_HEX(0x607382), EGUI_COLOR_HEX(0x7B8C99), EGUI_COLOR_HEX(0x2B928A));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&skeleton_pulse), on_pulse_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&pulse_column), EGUI_VIEW_OF(&skeleton_pulse));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&static_column));
    egui_view_set_size(EGUI_VIEW_OF(&static_column), 106, 86);
    egui_view_set_margin(EGUI_VIEW_OF(&static_column), 2, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&static_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&static_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&static_column));

    egui_view_label_init(EGUI_VIEW_OF(&static_label));
    egui_view_set_size(EGUI_VIEW_OF(&static_label), 106, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&static_label), "Static");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&static_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&static_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&static_label), EGUI_COLOR_HEX(0x99A1AA), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&static_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&static_column), EGUI_VIEW_OF(&static_label));

    egui_view_skeleton_init(EGUI_VIEW_OF(&skeleton_static));
    egui_view_set_size(EGUI_VIEW_OF(&skeleton_static), 106, 60);
    egui_view_skeleton_set_snapshots(EGUI_VIEW_OF(&skeleton_static), static_snapshots, 1);
    egui_view_skeleton_set_current_snapshot(EGUI_VIEW_OF(&skeleton_static), 0);
    egui_view_skeleton_set_emphasis_block(EGUI_VIEW_OF(&skeleton_static), static_snapshots[0].emphasis_block);
    egui_view_skeleton_set_font(EGUI_VIEW_OF(&skeleton_static), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_skeleton_set_compact_mode(EGUI_VIEW_OF(&skeleton_static), 1);
    egui_view_skeleton_set_show_footer(EGUI_VIEW_OF(&skeleton_static), 0);
    egui_view_skeleton_set_locked_mode(EGUI_VIEW_OF(&skeleton_static), 1);
    egui_view_skeleton_set_animation_mode(EGUI_VIEW_OF(&skeleton_static), EGUI_VIEW_SKELETON_ANIM_NONE);
    egui_view_skeleton_set_palette(EGUI_VIEW_OF(&skeleton_static), EGUI_COLOR_HEX(0xFDFDFE), EGUI_COLOR_HEX(0xE6EAED), EGUI_COLOR_HEX(0xF3F5F7),
                                   EGUI_COLOR_HEX(0x697781), EGUI_COLOR_HEX(0x96A0AA), EGUI_COLOR_HEX(0xBDC6CE));
    egui_view_group_add_child(EGUI_VIEW_OF(&static_column), EGUI_VIEW_OF(&skeleton_static));

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&pulse_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&static_column));
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
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 1:
        EGUI_SIM_SET_WAIT(p_action, 520);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 980);
        return true;
    default:
        return false;
    }
}
#endif
