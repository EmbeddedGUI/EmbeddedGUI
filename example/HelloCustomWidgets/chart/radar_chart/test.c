#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_radar_chart.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_radar_chart_t radar_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compare_column;
static egui_view_label_t compare_label;
static egui_view_radar_chart_t radar_compare;
static egui_view_linearlayout_t mini_column;
static egui_view_label_t mini_label;
static egui_view_radar_chart_t radar_mini;

static const char *title_text = "Radar Chart";
static const char *guide_text = "Tap charts to cycle profiles";
static const egui_view_line_point_t section_divider_points[] = {{0, 0}, {149, 0}};
static const char *compare_titles[] = {"Compare A", "Compare B"};
static const char *mini_titles[] = {"Mini A", "Mini B"};
static const char *primary_labels[] = {"CPU", "MEM", "IO", "TEMP", "NET", "UI"};
static const uint8_t primary_set_a[] = {82, 64, 48, 55, 70, 90};
static const uint8_t primary_set_b[] = {60, 84, 72, 40, 88, 58};

static const char *compare_labels[] = {"SAFE", "SPD", "PWR", "QUIET", "COST", "LIFE"};
static const uint8_t compare_set_a[] = {72, 58, 82, 44, 66, 74};
static const uint8_t compare_set_b[] = {54, 78, 68, 60, 52, 86};

static const char *mini_labels[] = {"S", "P", "Q", "E", "R"};
static const uint8_t mini_set_a[] = {78, 52, 68, 74, 60};
static const uint8_t mini_set_b[] = {58, 80, 50, 66, 84};
static const egui_color_t primary_status_color = EGUI_COLOR_HEX(0x31AFE4);

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void apply_compare_state(uint8_t index, int is_active)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&compare_label), compare_titles[index % 2]);
    if (is_active)
    {
        egui_view_label_set_font_color(
                EGUI_VIEW_OF(&compare_label),
                ((index % 2) == 0) ? EGUI_COLOR_HEX(0x3ADEA0) : EGUI_COLOR_HEX(0x74E8BE),
                EGUI_ALPHA_100);
        egui_view_radar_chart_set_palette(
                EGUI_VIEW_OF(&radar_compare),
                EGUI_COLOR_HEX(0x0B1F1A),
                EGUI_COLOR_HEX(0x39D8A2),
                EGUI_COLOR_HEX(0x7FF0C8),
                EGUI_COLOR_HEX(0xE9A923),
                EGUI_COLOR_HEX(0xF8C94A),
                EGUI_COLOR_HEX(0x334155),
                EGUI_COLOR_HEX(0xCBD5E1),
                EGUI_COLOR_HEX(0x94A3B8));
    }
    else
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compare_label), EGUI_COLOR_HEX(0x3F6257), EGUI_ALPHA_100);
        egui_view_radar_chart_set_palette(
                EGUI_VIEW_OF(&radar_compare),
                EGUI_COLOR_HEX(0x0A1613),
                EGUI_COLOR_HEX(0x257B63),
                EGUI_COLOR_HEX(0x46A48D),
                EGUI_COLOR_HEX(0x8A5B0D),
                EGUI_COLOR_HEX(0xB7791F),
                EGUI_COLOR_HEX(0x2B3A45),
                EGUI_COLOR_HEX(0x9FB2C6),
                EGUI_COLOR_HEX(0x6F8194));
    }
}

static void apply_mini_state(uint8_t index, int is_active)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&mini_label), mini_titles[index % 2]);
    if (is_active)
    {
        egui_view_label_set_font_color(
                EGUI_VIEW_OF(&mini_label),
                ((index % 2) == 0) ? EGUI_COLOR_HEX(0xEDA51A) : EGUI_COLOR_HEX(0xF8C53A),
                EGUI_ALPHA_100);
        egui_view_radar_chart_set_palette(
                EGUI_VIEW_OF(&radar_mini),
                EGUI_COLOR_HEX(0x26170A),
                EGUI_COLOR_HEX(0xF0A21B),
                EGUI_COLOR_HEX(0xFFD04A),
                EGUI_COLOR_HEX(0x8444F0),
                EGUI_COLOR_HEX(0xB59AFF),
                EGUI_COLOR_HEX(0x3F2A1D),
                EGUI_COLOR_HEX(0xFDE68A),
                EGUI_COLOR_HEX(0xF59E0B));
    }
    else
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&mini_label), EGUI_COLOR_HEX(0x75581A), EGUI_ALPHA_100);
        egui_view_radar_chart_set_palette(
                EGUI_VIEW_OF(&radar_mini),
                EGUI_COLOR_HEX(0x151008),
                EGUI_COLOR_HEX(0x99640A),
                EGUI_COLOR_HEX(0xB97D18),
                EGUI_COLOR_HEX(0x5B21B6),
                EGUI_COLOR_HEX(0x7C3AED),
                EGUI_COLOR_HEX(0x2A1F14),
                EGUI_COLOR_HEX(0xCAA85A),
                EGUI_COLOR_HEX(0x99640A));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_radar_chart_get_current_value_set(self) + 1) % 2;
    egui_view_radar_chart_set_current_value_set(self, next);
    apply_compare_state(egui_view_radar_chart_get_current_value_set(EGUI_VIEW_OF(&radar_compare)), 0);
    apply_mini_state(egui_view_radar_chart_get_current_value_set(EGUI_VIEW_OF(&radar_mini)), 0);
    if (next == 0)
    {
        set_status("Primary profile A", primary_status_color);
    }
    else
    {
        set_status("Primary profile B", primary_status_color);
    }
}

static void on_compare_click(egui_view_t *self)
{
    uint8_t next = (egui_view_radar_chart_get_current_value_set(self) + 1) % 2;
    egui_view_radar_chart_set_current_value_set(self, next);
    apply_compare_state(next, 1);
    apply_mini_state(egui_view_radar_chart_get_current_value_set(EGUI_VIEW_OF(&radar_mini)), 0);
    if (next == 0)
    {
        egui_view_radar_chart_set_compare_values(self, compare_set_b);
        set_status("Compare focus A", EGUI_COLOR_HEX(0x2FBE89));
    }
    else
    {
        egui_view_radar_chart_set_compare_values(self, compare_set_a);
        set_status("Compare focus B", EGUI_COLOR_HEX(0x2FBE89));
    }
}

static void on_mini_click(egui_view_t *self)
{
    uint8_t next = (egui_view_radar_chart_get_current_value_set(self) + 1) % 2;
    egui_view_radar_chart_set_current_value_set(self, next);
    apply_compare_state(egui_view_radar_chart_get_current_value_set(EGUI_VIEW_OF(&radar_compare)), 0);
    apply_mini_state(next, 1);
    if (next == 0)
    {
        set_status("Mini profile A", EGUI_COLOR_HEX(0xD89112));
    }
    else
    {
        set_status("Mini profile B", EGUI_COLOR_HEX(0xD89112));
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x38BDF8), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 220, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x4B5666), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_radar_chart_init(EGUI_VIEW_OF(&radar_primary));
    egui_view_set_size(EGUI_VIEW_OF(&radar_primary), 158, 146);
    egui_view_radar_chart_set_axis_labels(EGUI_VIEW_OF(&radar_primary), primary_labels, 6);
    egui_view_radar_chart_set_value_set(EGUI_VIEW_OF(&radar_primary), 0, primary_set_a);
    egui_view_radar_chart_set_value_set(EGUI_VIEW_OF(&radar_primary), 1, primary_set_b);
    egui_view_radar_chart_set_current_value_set(EGUI_VIEW_OF(&radar_primary), 0);
    egui_view_radar_chart_set_grid_levels(EGUI_VIEW_OF(&radar_primary), 4);
    egui_view_radar_chart_set_label_padding(EGUI_VIEW_OF(&radar_primary), 17);
    egui_view_radar_chart_set_palette(
            EGUI_VIEW_OF(&radar_primary),
            EGUI_COLOR_HEX(0x0D1724),
            EGUI_COLOR_HEX(0x38BDF8),
            EGUI_COLOR_HEX(0x7DD3FC),
            EGUI_COLOR_HEX(0x1D4ED8),
            EGUI_COLOR_HEX(0x60A5FA),
            EGUI_COLOR_HEX(0x334155),
            EGUI_COLOR_HEX(0xCBD5E1),
            EGUI_COLOR_HEX(0x94A3B8));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&radar_primary), on_primary_click);
    egui_view_set_margin(EGUI_VIEW_OF(&radar_primary), 0, 0, 0, 6);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&radar_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 220, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Primary profile A");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), primary_status_color, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 150, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), section_divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0x334155));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 214, 108);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compare_column));
    egui_view_set_size(EGUI_VIEW_OF(&compare_column), 120, 112);
    egui_view_set_margin(EGUI_VIEW_OF(&compare_column), 2, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compare_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compare_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compare_column));

    egui_view_label_init(EGUI_VIEW_OF(&compare_label));
    egui_view_set_size(EGUI_VIEW_OF(&compare_label), 120, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&compare_label), compare_titles[0]);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compare_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compare_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&compare_label), 0, 1, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&compare_column), EGUI_VIEW_OF(&compare_label));

    egui_view_radar_chart_init(EGUI_VIEW_OF(&radar_compare));
    egui_view_set_size(EGUI_VIEW_OF(&radar_compare), 120, 94);
    egui_view_radar_chart_set_axis_labels(EGUI_VIEW_OF(&radar_compare), compare_labels, 6);
    egui_view_radar_chart_set_value_set(EGUI_VIEW_OF(&radar_compare), 0, compare_set_a);
    egui_view_radar_chart_set_value_set(EGUI_VIEW_OF(&radar_compare), 1, compare_set_b);
    egui_view_radar_chart_set_current_value_set(EGUI_VIEW_OF(&radar_compare), 0);
    egui_view_radar_chart_set_compare_values(EGUI_VIEW_OF(&radar_compare), compare_set_b);
    egui_view_radar_chart_set_show_compare(EGUI_VIEW_OF(&radar_compare), 1);
    egui_view_radar_chart_set_grid_levels(EGUI_VIEW_OF(&radar_compare), 3);
    egui_view_radar_chart_set_label_padding(EGUI_VIEW_OF(&radar_compare), 18);
    egui_view_radar_chart_set_font(EGUI_VIEW_OF(&radar_compare), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&radar_compare), on_compare_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compare_column), EGUI_VIEW_OF(&radar_compare));
    apply_compare_state(0, 0);

    egui_view_linearlayout_init(EGUI_VIEW_OF(&mini_column));
    egui_view_set_size(EGUI_VIEW_OF(&mini_column), 90, 112);
    egui_view_set_margin(EGUI_VIEW_OF(&mini_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&mini_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&mini_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&mini_column));

    egui_view_label_init(EGUI_VIEW_OF(&mini_label));
    egui_view_set_size(EGUI_VIEW_OF(&mini_label), 90, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&mini_label), mini_titles[0]);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mini_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&mini_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&mini_label), 0, 1, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&mini_column), EGUI_VIEW_OF(&mini_label));

    egui_view_radar_chart_init(EGUI_VIEW_OF(&radar_mini));
    egui_view_set_size(EGUI_VIEW_OF(&radar_mini), 90, 94);
    egui_view_radar_chart_set_axis_labels(EGUI_VIEW_OF(&radar_mini), mini_labels, 5);
    egui_view_radar_chart_set_value_set(EGUI_VIEW_OF(&radar_mini), 0, mini_set_a);
    egui_view_radar_chart_set_value_set(EGUI_VIEW_OF(&radar_mini), 1, mini_set_b);
    egui_view_radar_chart_set_current_value_set(EGUI_VIEW_OF(&radar_mini), 0);
    egui_view_radar_chart_set_grid_levels(EGUI_VIEW_OF(&radar_mini), 2);
    egui_view_radar_chart_set_show_axis_labels(EGUI_VIEW_OF(&radar_mini), 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&radar_mini), on_mini_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&mini_column), EGUI_VIEW_OF(&radar_mini));
    apply_mini_state(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compare_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&mini_column));
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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&radar_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&radar_compare), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&radar_mini), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
