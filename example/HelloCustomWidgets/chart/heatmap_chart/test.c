#define HEATMAP_ROOT_WIDTH 230
#define HEATMAP_ROOT_HEIGHT 306
#define HEATMAP_TITLE_COLOR 0x5EDBFF
#define HEATMAP_GUIDE_TEXT "Tap cards to cycle"
#define HEATMAP_GUIDE_COLOR 0x829AB7
#define HEATMAP_GUIDE_MARGIN_BOTTOM 3
#define HEATMAP_PRIMARY_WIDTH 188
#define HEATMAP_PRIMARY_HEIGHT 138
#define HEATMAP_PRIMARY_MARGIN_BOTTOM 4
#define HEATMAP_STATUS_TEXT_A "Core A"
#define HEATMAP_STATUS_TEXT_B "Core B"
#define HEATMAP_STATUS_TEXT_COMPACT_A "Compact A"
#define HEATMAP_STATUS_TEXT_COMPACT_B "Compact B"
#define HEATMAP_STATUS_PRIMARY_COLOR 0x4FD3FF
#define HEATMAP_STATUS_COMPACT_COLOR 0xF59E0B
#define HEATMAP_STATUS_MARGIN_BOTTOM 4
#define HEATMAP_DIVIDER_WIDTH 170
#define HEATMAP_DIVIDER_POINTS 169
#define HEATMAP_DIVIDER_COLOR_PRIMARY 0x5D8CBC
#define HEATMAP_DIVIDER_COLOR_COMPACT 0x9B6A34
#define HEATMAP_DIVIDER_MARGIN_BOTTOM 5
#define HEATMAP_BOTTOM_ROW_WIDTH 230
#define HEATMAP_BOTTOM_ROW_HEIGHT 106
#define HEATMAP_COLUMN_WIDTH 108
#define HEATMAP_COLUMN_HEIGHT 108
#define HEATMAP_COLUMN_GAP 6
#define HEATMAP_CARD_HEIGHT 92
#define HEATMAP_CARD_LABEL_TEXT "Compact"
#define HEATMAP_LOCKED_LABEL_TEXT "Locked"
#define HEATMAP_COMPACT_LABEL_COLOR_IDLE 0xB8C5D4
#define HEATMAP_COMPACT_LABEL_COLOR_ACTIVE 0xF3C56B
#define HEATMAP_LOCKED_LABEL_COLOR 0xCCD6E1
#define HEATMAP_PRIMARY_BG 0x0C1422
#define HEATMAP_PRIMARY_BORDER 0x516178
#define HEATMAP_PRIMARY_COLD 0x2563EB
#define HEATMAP_PRIMARY_HOT 0xF97316
#define HEATMAP_PRIMARY_TEXT 0xE2E8F0
#define HEATMAP_PRIMARY_MUTED 0x94A3B8
#define HEATMAP_COMPACT_ACTIVE_BG 0x24150A
#define HEATMAP_COMPACT_ACTIVE_BORDER 0x8F4D1A
#define HEATMAP_COMPACT_ACTIVE_COLD 0xC2410C
#define HEATMAP_COMPACT_ACTIVE_HOT 0xF59E0B
#define HEATMAP_COMPACT_ACTIVE_TEXT 0xFDE68A
#define HEATMAP_COMPACT_ACTIVE_MUTED 0xC08A1B
#define HEATMAP_COMPACT_IDLE_BG 0x141B29
#define HEATMAP_COMPACT_IDLE_BORDER 0x35506E
#define HEATMAP_COMPACT_IDLE_COLD 0x2563EB
#define HEATMAP_COMPACT_IDLE_HOT 0xEA580C
#define HEATMAP_COMPACT_IDLE_TEXT 0xCBD5E1
#define HEATMAP_COMPACT_IDLE_MUTED 0x70839C
#define HEATMAP_LOCKED_BG 0x101725
#define HEATMAP_LOCKED_BORDER 0x4F5D72
#define HEATMAP_LOCKED_COLD 0x7C8CA0
#define HEATMAP_LOCKED_HOT 0xC8A87A
#define HEATMAP_LOCKED_TEXT 0xCBD5E1
#define HEATMAP_LOCKED_MUTED 0x94A3B8

#include <stdlib.h>

#include "egui.h"
#include "egui_view_heatmap_chart.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_heatmap_chart_t heatmap_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_heatmap_chart_t heatmap_compact;
static egui_view_label_t compact_label;
static egui_view_linearlayout_t locked_column;
static egui_view_heatmap_chart_t heatmap_locked;
static egui_view_label_t locked_label;

static const char *title_text = "Heatmap Chart";
static const char *guide_text = HEATMAP_GUIDE_TEXT;
static const egui_view_line_point_t section_divider_points[] = {{0, 0}, {HEATMAP_DIVIDER_POINTS, 0}};
static const char *primary_row_labels[] = {"C1", "C2", "C3", "C4"};
static const char *primary_col_labels[] = {"M1", "M2", "M3", "M4"};
static const uint8_t primary_values_a[] = {
        18, 42, 65, 72,
        24, 38, 58, 84,
        12, 34, 77, 91,
        8, 29, 51, 69};
static const uint8_t primary_values_b[] = {
        62, 28, 19, 44,
        75, 41, 33, 56,
        88, 63, 47, 31,
        93, 71, 52, 26};
static const uint8_t compact_values_a[] = {
        15, 44, 72,
        28, 58, 83,
        11, 37, 65};
static const uint8_t compact_values_b[] = {
        63, 35, 18,
        79, 52, 29,
        91, 68, 46};

static void set_status(const char *text, egui_color_t color, egui_color_t guide_color, egui_color_t divider_color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), guide_color, EGUI_ALPHA_100);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), divider_color);
}

static void apply_compact_state(uint8_t index, int is_active)
{
    (void)index;
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), HEATMAP_CARD_LABEL_TEXT);
    if (is_active)
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(HEATMAP_COMPACT_LABEL_COLOR_ACTIVE), EGUI_ALPHA_100);
        egui_view_heatmap_chart_set_palette(
                EGUI_VIEW_OF(&heatmap_compact),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_ACTIVE_BG),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_ACTIVE_BORDER),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_ACTIVE_COLD),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_ACTIVE_HOT),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_ACTIVE_TEXT),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_ACTIVE_MUTED));
        egui_view_heatmap_chart_set_show_values(EGUI_VIEW_OF(&heatmap_compact), 1);
    }
    else
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(HEATMAP_COMPACT_LABEL_COLOR_IDLE), EGUI_ALPHA_100);
        egui_view_heatmap_chart_set_palette(
                EGUI_VIEW_OF(&heatmap_compact),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_IDLE_BG),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_IDLE_BORDER),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_IDLE_COLD),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_IDLE_HOT),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_IDLE_TEXT),
                EGUI_COLOR_HEX(HEATMAP_COMPACT_IDLE_MUTED));
        egui_view_heatmap_chart_set_show_values(EGUI_VIEW_OF(&heatmap_compact), 0);
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_heatmap_chart_get_current_value_set(self) + 1) % 2;

    egui_view_heatmap_chart_set_current_value_set(self, next);
    apply_compact_state(egui_view_heatmap_chart_get_current_value_set(EGUI_VIEW_OF(&heatmap_compact)), 0);
    set_status(
            (next == 0) ? HEATMAP_STATUS_TEXT_A : HEATMAP_STATUS_TEXT_B,
            EGUI_COLOR_HEX(HEATMAP_STATUS_PRIMARY_COLOR),
            EGUI_COLOR_HEX(HEATMAP_GUIDE_COLOR),
            EGUI_COLOR_HEX(HEATMAP_DIVIDER_COLOR_PRIMARY));
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_heatmap_chart_get_current_value_set(self) + 1) % 2;

    egui_view_heatmap_chart_set_current_value_set(self, next);
    apply_compact_state(next, 1);
    set_status(
            (next == 0) ? HEATMAP_STATUS_TEXT_COMPACT_A : HEATMAP_STATUS_TEXT_COMPACT_B,
            EGUI_COLOR_HEX(HEATMAP_STATUS_COMPACT_COLOR),
            EGUI_COLOR_HEX(0xA07A42),
            EGUI_COLOR_HEX(HEATMAP_DIVIDER_COLOR_COMPACT));
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), HEATMAP_ROOT_WIDTH, HEATMAP_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), HEATMAP_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(HEATMAP_TITLE_COLOR), EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), HEATMAP_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(HEATMAP_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, HEATMAP_GUIDE_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_heatmap_chart_init(EGUI_VIEW_OF(&heatmap_primary));
    egui_view_set_size(EGUI_VIEW_OF(&heatmap_primary), HEATMAP_PRIMARY_WIDTH, HEATMAP_PRIMARY_HEIGHT);
    egui_view_heatmap_chart_set_shape(EGUI_VIEW_OF(&heatmap_primary), 4, 4);
    egui_view_heatmap_chart_set_axis_labels(EGUI_VIEW_OF(&heatmap_primary), primary_row_labels, primary_col_labels);
    egui_view_heatmap_chart_set_value_set(EGUI_VIEW_OF(&heatmap_primary), 0, primary_values_a);
    egui_view_heatmap_chart_set_value_set(EGUI_VIEW_OF(&heatmap_primary), 1, primary_values_b);
    egui_view_heatmap_chart_set_current_value_set(EGUI_VIEW_OF(&heatmap_primary), 0);
    egui_view_heatmap_chart_set_palette(
            EGUI_VIEW_OF(&heatmap_primary),
            EGUI_COLOR_HEX(HEATMAP_PRIMARY_BG),
            EGUI_COLOR_HEX(HEATMAP_PRIMARY_BORDER),
            EGUI_COLOR_HEX(HEATMAP_PRIMARY_COLD),
            EGUI_COLOR_HEX(HEATMAP_PRIMARY_HOT),
            EGUI_COLOR_HEX(HEATMAP_PRIMARY_TEXT),
            EGUI_COLOR_HEX(HEATMAP_PRIMARY_MUTED));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&heatmap_primary), on_primary_click);
    egui_view_set_margin(EGUI_VIEW_OF(&heatmap_primary), 0, 0, 0, HEATMAP_PRIMARY_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&heatmap_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), HEATMAP_ROOT_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), HEATMAP_STATUS_TEXT_A);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(HEATMAP_STATUS_PRIMARY_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, HEATMAP_STATUS_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), HEATMAP_DIVIDER_WIDTH, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), section_divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(HEATMAP_DIVIDER_COLOR_PRIMARY));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, HEATMAP_DIVIDER_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), HEATMAP_BOTTOM_ROW_WIDTH, HEATMAP_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), HEATMAP_COLUMN_WIDTH, HEATMAP_COLUMN_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, HEATMAP_COLUMN_GAP, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), HEATMAP_COLUMN_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), HEATMAP_CARD_LABEL_TEXT);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(HEATMAP_COMPACT_LABEL_COLOR_IDLE), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_heatmap_chart_init(EGUI_VIEW_OF(&heatmap_compact));
    egui_view_set_size(EGUI_VIEW_OF(&heatmap_compact), HEATMAP_COLUMN_WIDTH, HEATMAP_CARD_HEIGHT);
    egui_view_heatmap_chart_set_shape(EGUI_VIEW_OF(&heatmap_compact), 3, 3);
    egui_view_heatmap_chart_set_value_set(EGUI_VIEW_OF(&heatmap_compact), 0, compact_values_a);
    egui_view_heatmap_chart_set_value_set(EGUI_VIEW_OF(&heatmap_compact), 1, compact_values_b);
    egui_view_heatmap_chart_set_current_value_set(EGUI_VIEW_OF(&heatmap_compact), 0);
    egui_view_heatmap_chart_set_font(EGUI_VIEW_OF(&heatmap_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_heatmap_chart_set_show_axis_labels(EGUI_VIEW_OF(&heatmap_compact), 0);
    egui_view_heatmap_chart_set_show_header(EGUI_VIEW_OF(&heatmap_compact), 0);
    apply_compact_state(0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&heatmap_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&heatmap_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), HEATMAP_COLUMN_WIDTH, HEATMAP_COLUMN_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), HEATMAP_COLUMN_GAP, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), HEATMAP_COLUMN_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), HEATMAP_LOCKED_LABEL_TEXT);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(HEATMAP_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_heatmap_chart_init(EGUI_VIEW_OF(&heatmap_locked));
    egui_view_set_size(EGUI_VIEW_OF(&heatmap_locked), HEATMAP_COLUMN_WIDTH, HEATMAP_CARD_HEIGHT);
    egui_view_heatmap_chart_set_shape(EGUI_VIEW_OF(&heatmap_locked), 3, 3);
    egui_view_heatmap_chart_set_value_set(EGUI_VIEW_OF(&heatmap_locked), 0, compact_values_a);
    egui_view_heatmap_chart_set_current_value_set(EGUI_VIEW_OF(&heatmap_locked), 0);
    egui_view_heatmap_chart_set_font(EGUI_VIEW_OF(&heatmap_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_heatmap_chart_set_show_axis_labels(EGUI_VIEW_OF(&heatmap_locked), 0);
    egui_view_heatmap_chart_set_show_header(EGUI_VIEW_OF(&heatmap_locked), 0);
    egui_view_heatmap_chart_set_show_values(EGUI_VIEW_OF(&heatmap_locked), 0);
    egui_view_heatmap_chart_set_palette(
            EGUI_VIEW_OF(&heatmap_locked),
            EGUI_COLOR_HEX(HEATMAP_LOCKED_BG),
            EGUI_COLOR_HEX(HEATMAP_LOCKED_BORDER),
            EGUI_COLOR_HEX(HEATMAP_LOCKED_COLD),
            EGUI_COLOR_HEX(HEATMAP_LOCKED_HOT),
            EGUI_COLOR_HEX(HEATMAP_LOCKED_TEXT),
            EGUI_COLOR_HEX(HEATMAP_LOCKED_MUTED));
    egui_view_set_enable(EGUI_VIEW_OF(&heatmap_locked), 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&heatmap_locked));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&heatmap_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&heatmap_compact), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&heatmap_locked), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
