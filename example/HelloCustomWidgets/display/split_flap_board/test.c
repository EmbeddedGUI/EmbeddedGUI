#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_split_flap_board.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_split_flap_board_t board_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_split_flap_board_t board_compact;
static egui_view_split_flap_board_t board_standby;

static const char *title_text = "Split Flap Board";
static const char *guide_text = "Tap boards to cycle flights";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {149, 0}};
static const egui_color_t primary_status_color = EGUI_COLOR_HEX(0x48B6E6);
static const egui_color_t queue_status_color = EGUI_COLOR_HEX(0x2EC58C);
static const egui_color_t standby_status_color = EGUI_COLOR_HEX(0xD79318);

static const egui_view_split_flap_board_snapshot_t primary_snapshots[] = {
        {"DEPARTURE", "LIVE", "A17", "OSAKA", "18:40", 1, 0},
        {"DEPARTURE", "DELAY", "B22", "SEOUL", "19:05", 2, 1},
        {"DEPARTURE", "BOARD", "C08", "TOKYO", "19:30", 0, 0},
};

static const egui_view_split_flap_board_snapshot_t compact_snapshots[] = {
        {"QUEUE", "NEXT", "D14", "NAGOYA", "18:55", 1, 0},
        {"QUEUE", "HOLD", "E03", "TAIPEI", "19:12", 0, 1},
        {"QUEUE", "NEXT", "F19", "BUSAN", "19:48", 2, 0},
};

static const egui_view_split_flap_board_snapshot_t standby_snapshots[] = {
        {"STBY", "OPEN", "G5", "CREW", "20:05", 0, 0},
        {"STBY", "LOCK", "H2", "SERVICE", "20:20", 1, 1},
        {"STBY", "OPEN", "J7", "BAG", "20:35", 1, 0},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_split_flap_board_get_current_snapshot(self) + 1) % 3;
    egui_view_split_flap_board_set_current_snapshot(self, next);
    if (next == 0)
    {
        set_status("Primary A17 live", primary_status_color);
    }
    else if (next == 1)
    {
        set_status("Primary B22 delayed", standby_status_color);
    }
    else
    {
        set_status("Primary C08 boarding", primary_status_color);
    }
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_split_flap_board_get_current_snapshot(self) + 1) % 3;
    egui_view_split_flap_board_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Queue hold E03", queue_status_color);
    }
    else
    {
        set_status("Queue preview updated", queue_status_color);
    }
}

static void on_standby_click(egui_view_t *self)
{
    uint8_t next = (egui_view_split_flap_board_get_current_snapshot(self) + 1) % 3;
    egui_view_split_flap_board_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Standby lane locked", standby_status_color);
    }
    else
    {
        set_status("Standby lane open", standby_status_color);
    }
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 220, 308);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 220, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x93C5FD), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 220, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x586576), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_split_flap_board_init(EGUI_VIEW_OF(&board_primary));
    egui_view_set_size(EGUI_VIEW_OF(&board_primary), 192, 116);
    egui_view_split_flap_board_set_snapshots(EGUI_VIEW_OF(&board_primary), primary_snapshots, 3);
    egui_view_split_flap_board_set_font(EGUI_VIEW_OF(&board_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_split_flap_board_set_palette(EGUI_VIEW_OF(&board_primary), EGUI_COLOR_HEX(0x18212D), EGUI_COLOR_HEX(0x4B5D73), EGUI_COLOR_HEX(0xE2E8F0),
                                           EGUI_COLOR_HEX(0x94A3B8), EGUI_COLOR_HEX(0x38BDF8), EGUI_COLOR_HEX(0xF59E0B), EGUI_COLOR_HEX(0x0F172A));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&board_primary), on_primary_click);
    egui_view_set_margin(EGUI_VIEW_OF(&board_primary), 0, 0, 0, 6);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&board_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 220, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Primary A17 live");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), primary_status_color, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 150, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0x334155));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 211, 86);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_split_flap_board_init(EGUI_VIEW_OF(&board_compact));
    egui_view_set_size(EGUI_VIEW_OF(&board_compact), 122, 80);
    egui_view_split_flap_board_set_snapshots(EGUI_VIEW_OF(&board_compact), compact_snapshots, 3);
    egui_view_split_flap_board_set_font(EGUI_VIEW_OF(&board_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_flap_board_set_compact_mode(EGUI_VIEW_OF(&board_compact), 1);
    egui_view_split_flap_board_set_palette(EGUI_VIEW_OF(&board_compact), EGUI_COLOR_HEX(0x132019), EGUI_COLOR_HEX(0x34584B), EGUI_COLOR_HEX(0xD9FBE8),
                                           EGUI_COLOR_HEX(0x8AA99A), EGUI_COLOR_HEX(0x34D399), EGUI_COLOR_HEX(0xF59E0B), EGUI_COLOR_HEX(0x0A120E));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&board_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&board_compact));

    egui_view_split_flap_board_init(EGUI_VIEW_OF(&board_standby));
    egui_view_set_size(EGUI_VIEW_OF(&board_standby), 86, 80);
    egui_view_split_flap_board_set_snapshots(EGUI_VIEW_OF(&board_standby), standby_snapshots, 3);
    egui_view_split_flap_board_set_font(EGUI_VIEW_OF(&board_standby), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_split_flap_board_set_compact_mode(EGUI_VIEW_OF(&board_standby), 1);
    egui_view_split_flap_board_set_palette(EGUI_VIEW_OF(&board_standby), EGUI_COLOR_HEX(0x241B10), EGUI_COLOR_HEX(0x6B4F16), EGUI_COLOR_HEX(0xFDE68A),
                                           EGUI_COLOR_HEX(0xC3A85A), EGUI_COLOR_HEX(0xF59E0B), EGUI_COLOR_HEX(0xEF4444), EGUI_COLOR_HEX(0x130E08));
    egui_view_set_margin(EGUI_VIEW_OF(&board_standby), 3, 0, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&board_standby), on_standby_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&board_standby));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&board_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&board_compact), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&board_standby), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
