#include <stdlib.h>

#include "egui.h"
#include "egui_view_boxplot_chart.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_boxplot_chart_t boxplot_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_boxplot_chart_t boxplot_compact;
static egui_view_label_t compact_label;
static egui_view_linearlayout_t locked_column;
static egui_view_boxplot_chart_t boxplot_locked;
static egui_view_label_t locked_label;

static const char *title_text = "Boxplot Chart";
static const char *guide_text = "Tap cards to cycle";
static const egui_view_line_point_t section_divider_points[] = {{0, 0}, {169, 0}};

static const char *primary_labels[] = {"A", "B", "C", "D", "E"};
static const uint8_t primary_min_a[] = {18, 26, 20, 31, 24};
static const uint8_t primary_q1_a[] = {24, 33, 27, 37, 31};
static const uint8_t primary_median_a[] = {30, 39, 34, 44, 37};
static const uint8_t primary_q3_a[] = {36, 45, 40, 50, 44};
static const uint8_t primary_max_a[] = {44, 53, 48, 58, 51};
static const uint8_t primary_min_b[] = {28, 22, 34, 26, 38};
static const uint8_t primary_q1_b[] = {35, 29, 41, 33, 45};
static const uint8_t primary_median_b[] = {42, 36, 48, 40, 52};
static const uint8_t primary_q3_b[] = {49, 43, 55, 47, 59};
static const uint8_t primary_max_b[] = {57, 51, 63, 55, 67};

static const char *compact_labels[] = {"1", "2", "3", "4"};
static const uint8_t compact_min_a[] = {14, 18, 23, 16};
static const uint8_t compact_q1_a[] = {19, 24, 29, 22};
static const uint8_t compact_median_a[] = {25, 30, 35, 28};
static const uint8_t compact_q3_a[] = {31, 36, 41, 34};
static const uint8_t compact_max_a[] = {37, 43, 47, 41};
static const uint8_t compact_min_b[] = {21, 16, 26, 19};
static const uint8_t compact_q1_b[] = {27, 22, 32, 25};
static const uint8_t compact_median_b[] = {33, 28, 38, 31};
static const uint8_t compact_q3_b[] = {39, 34, 44, 37};
static const uint8_t compact_max_b[] = {45, 40, 50, 43};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void apply_compact_state(uint8_t index, int is_active)
{
    if (is_active)
    {
        egui_view_boxplot_chart_set_show_labels(EGUI_VIEW_OF(&boxplot_compact), 1);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0xF9C96A), EGUI_ALPHA_100);
        egui_view_boxplot_chart_set_palette(
                EGUI_VIEW_OF(&boxplot_compact),
                EGUI_COLOR_HEX(0x23170C),
                EGUI_COLOR_HEX(0x9E6D2D),
                EGUI_COLOR_HEX(0xE2BA78),
                EGUI_COLOR_HEX(0xC26D15),
                EGUI_COLOR_HEX(0xFCD34D),
                EGUI_COLOR_HEX(0xFDE7A1),
                EGUI_COLOR_HEX(0xD29A35));
    }
    else
    {
        egui_view_boxplot_chart_set_show_labels(EGUI_VIEW_OF(&boxplot_compact), 0);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0xB2C0D0), EGUI_ALPHA_100);
        egui_view_boxplot_chart_set_palette(
                EGUI_VIEW_OF(&boxplot_compact),
                EGUI_COLOR_HEX(0x121A28),
                EGUI_COLOR_HEX(0x4C6A89),
                EGUI_COLOR_HEX(0x9AAEC2),
                EGUI_COLOR_HEX(0x2563EB),
                EGUI_COLOR_HEX(0xF59E0B),
                EGUI_COLOR_HEX(0xDEE7F1),
                EGUI_COLOR_HEX(0x8FA2B6));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_boxplot_chart_get_current_value_set(self) + 1) % 2;
    egui_view_boxplot_chart_set_current_value_set(self, next);
    apply_compact_state(egui_view_boxplot_chart_get_current_value_set(EGUI_VIEW_OF(&boxplot_compact)), 0);
    set_status((next == 0) ? "Core A" : "Core B", EGUI_COLOR_HEX(0x4FD1FF));
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_boxplot_chart_get_current_value_set(self) + 1) % 2;
    egui_view_boxplot_chart_set_current_value_set(self, next);
    apply_compact_state(next, 1);
    set_status((next == 0) ? "Compact A" : "Compact B", EGUI_COLOR_HEX(0xF59E0B));
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 230, 308);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 230, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x68E0FF), EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 230, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x8EA6C0), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_boxplot_chart_init(EGUI_VIEW_OF(&boxplot_primary));
    egui_view_set_size(EGUI_VIEW_OF(&boxplot_primary), 188, 142);
    egui_view_boxplot_chart_set_item_labels(EGUI_VIEW_OF(&boxplot_primary), primary_labels, 5);
    egui_view_boxplot_chart_set_value_set(
            EGUI_VIEW_OF(&boxplot_primary), 0, primary_min_a, primary_q1_a, primary_median_a, primary_q3_a, primary_max_a);
    egui_view_boxplot_chart_set_value_set(
            EGUI_VIEW_OF(&boxplot_primary), 1, primary_min_b, primary_q1_b, primary_median_b, primary_q3_b, primary_max_b);
    egui_view_boxplot_chart_set_current_value_set(EGUI_VIEW_OF(&boxplot_primary), 0);
    egui_view_boxplot_chart_set_palette(
            EGUI_VIEW_OF(&boxplot_primary),
            EGUI_COLOR_HEX(0x0F1728),
            EGUI_COLOR_HEX(0x617A92),
            EGUI_COLOR_HEX(0xA6B9CC),
            EGUI_COLOR_HEX(0x2563EB),
            EGUI_COLOR_HEX(0xF59E0B),
            EGUI_COLOR_HEX(0xE2E8F0),
            EGUI_COLOR_HEX(0xA5B4C4));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&boxplot_primary), on_primary_click);
    egui_view_set_margin(EGUI_VIEW_OF(&boxplot_primary), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&boxplot_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 230, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Core A");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(0x64DAFF), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 170, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), section_divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0x5C8BBC));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 6);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 230, 108);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), 108, 110);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, 6, 0, 0);
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

    egui_view_boxplot_chart_init(EGUI_VIEW_OF(&boxplot_compact));
    egui_view_set_size(EGUI_VIEW_OF(&boxplot_compact), 108, 94);
    egui_view_boxplot_chart_set_item_labels(EGUI_VIEW_OF(&boxplot_compact), compact_labels, 4);
    egui_view_boxplot_chart_set_value_set(
            EGUI_VIEW_OF(&boxplot_compact), 0, compact_min_a, compact_q1_a, compact_median_a, compact_q3_a, compact_max_a);
    egui_view_boxplot_chart_set_value_set(
            EGUI_VIEW_OF(&boxplot_compact), 1, compact_min_b, compact_q1_b, compact_median_b, compact_q3_b, compact_max_b);
    egui_view_boxplot_chart_set_current_value_set(EGUI_VIEW_OF(&boxplot_compact), 0);
    egui_view_boxplot_chart_set_font(EGUI_VIEW_OF(&boxplot_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_boxplot_chart_set_show_header(EGUI_VIEW_OF(&boxplot_compact), 0);
    apply_compact_state(0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&boxplot_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&boxplot_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), 108, 110);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), 108, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Locked");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(0xCCD6E1), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_boxplot_chart_init(EGUI_VIEW_OF(&boxplot_locked));
    egui_view_set_size(EGUI_VIEW_OF(&boxplot_locked), 108, 94);
    egui_view_boxplot_chart_set_item_labels(EGUI_VIEW_OF(&boxplot_locked), compact_labels, 4);
    egui_view_boxplot_chart_set_value_set(
            EGUI_VIEW_OF(&boxplot_locked), 0, compact_min_a, compact_q1_a, compact_median_a, compact_q3_a, compact_max_a);
    egui_view_boxplot_chart_set_current_value_set(EGUI_VIEW_OF(&boxplot_locked), 0);
    egui_view_boxplot_chart_set_show_labels(EGUI_VIEW_OF(&boxplot_locked), 0);
    egui_view_boxplot_chart_set_show_header(EGUI_VIEW_OF(&boxplot_locked), 0);
    egui_view_boxplot_chart_set_palette(
            EGUI_VIEW_OF(&boxplot_locked),
            EGUI_COLOR_HEX(0x0E1522),
            EGUI_COLOR_HEX(0x5D6E82),
            EGUI_COLOR_HEX(0x8EA2B6),
            EGUI_COLOR_HEX(0x758E83),
            EGUI_COLOR_HEX(0xC89D67),
            EGUI_COLOR_HEX(0xD7E0E9),
            EGUI_COLOR_HEX(0x97A8BC));
    egui_view_set_enable(EGUI_VIEW_OF(&boxplot_locked), 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&boxplot_locked));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&boxplot_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&boxplot_compact), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&boxplot_locked), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
