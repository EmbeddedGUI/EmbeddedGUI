#define TREEMAP_ROOT_WIDTH                 230
#define TREEMAP_ROOT_HEIGHT                300
#define TREEMAP_TITLE_WIDTH                230
#define TREEMAP_TITLE_COLOR                0x54D8FF
#define TREEMAP_GUIDE_TEXT                 "Tap cards to cycle"
#define TREEMAP_GUIDE_COLOR                0x7E96B4
#define TREEMAP_GUIDE_MARGIN_BOTTOM        3
#define TREEMAP_PRIMARY_WIDTH              188
#define TREEMAP_PRIMARY_HEIGHT             138
#define TREEMAP_PRIMARY_MARGIN_BOTTOM      4
#define TREEMAP_STATUS_TEXT_A              "Core A"
#define TREEMAP_STATUS_TEXT_B              "Core B"
#define TREEMAP_COMPACT_STATUS_A           "Compact A"
#define TREEMAP_COMPACT_STATUS_B           "Compact B"
#define TREEMAP_STATUS_COLOR_PRIMARY       0x38BDF8
#define TREEMAP_STATUS_COLOR_COMPACT       0xF59E0B
#define TREEMAP_STATUS_MARGIN_BOTTOM       4
#define TREEMAP_DIVIDER_WIDTH              170
#define TREEMAP_DIVIDER_POINTS             169
#define TREEMAP_DIVIDER_COLOR              0x5C8BBC
#define TREEMAP_DIVIDER_MARGIN_BOTTOM      5
#define TREEMAP_BOTTOM_ROW_WIDTH           230
#define TREEMAP_BOTTOM_ROW_HEIGHT          104
#define TREEMAP_COLUMN_WIDTH               108
#define TREEMAP_COLUMN_HEIGHT              106
#define TREEMAP_COLUMN_GAP                 6
#define TREEMAP_COMPACT_LABEL_TEXT         "Compact"
#define TREEMAP_LOCKED_LABEL_TEXT          "Locked"
#define TREEMAP_COMPACT_LABEL_COLOR_IDLE   0xB6C4D3
#define TREEMAP_COMPACT_LABEL_COLOR_ACTIVE 0xF4C96D
#define TREEMAP_LOCKED_LABEL_COLOR         0xCBD5E1
#define TREEMAP_COMPACT_SHOW_LABELS_IDLE   0
#define TREEMAP_COMPACT_SHOW_LABELS_ACTIVE 1
#define TREEMAP_COMPACT_ACTIVE_BG          0x22160B
#define TREEMAP_COMPACT_ACTIVE_BORDER      0x9A6B2A
#define TREEMAP_COMPACT_ACTIVE_TEXT        0xF7D488
#define TREEMAP_COMPACT_ACTIVE_MUTED       0xCC8A2E
#define TREEMAP_COMPACT_IDLE_BG            0x101A27
#define TREEMAP_COMPACT_IDLE_BORDER        0x4B6B8B
#define TREEMAP_COMPACT_IDLE_TEXT          0xD7E4F2
#define TREEMAP_COMPACT_IDLE_MUTED         0x8EA3BA
#define TREEMAP_PRIMARY_BG                 0x0D1724
#define TREEMAP_PRIMARY_BORDER             0x5B6B82
#define TREEMAP_PRIMARY_TEXT               0xE2E8F0
#define TREEMAP_PRIMARY_MUTED              0x94A3B8
#define TREEMAP_LOCKED_BG                  0x101723
#define TREEMAP_LOCKED_BORDER              0x5E7184
#define TREEMAP_LOCKED_TEXT                0xC6D0DB
#define TREEMAP_LOCKED_MUTED               0x94A3B6
#include <stdlib.h>

#include "egui.h"
#include "egui_view_treemap_chart.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_treemap_chart_t treemap_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_treemap_chart_t treemap_compact;
static egui_view_label_t compact_label;
static egui_view_linearlayout_t disabled_column;
static egui_view_treemap_chart_t treemap_disabled;
static egui_view_label_t disabled_label;

static const char *title_text = "Treemap Chart";
static const char *guide_text = TREEMAP_GUIDE_TEXT;
static const egui_view_line_point_t section_divider_points[] = {{0, 0}, {TREEMAP_DIVIDER_POINTS, 0}};
static const char *primary_labels[] = {"CPU", "GPU", "RAM", "NET", "DSP"};
static const uint8_t primary_values_a[] = {32, 22, 18, 15, 13};
static const uint8_t primary_values_b[] = {20, 28, 24, 12, 16};
static const char *compact_labels[] = {"TMP", "FAN", "PWR", "IO"};
static const uint8_t compact_values_a[] = {34, 24, 22, 20};
static const uint8_t compact_values_b[] = {18, 32, 28, 22};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void apply_compact_state(uint8_t index, int is_active)
{
    (void)index;
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), TREEMAP_COMPACT_LABEL_TEXT);
    if (is_active)
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(TREEMAP_COMPACT_LABEL_COLOR_ACTIVE), EGUI_ALPHA_100);
        egui_view_treemap_chart_set_show_labels(EGUI_VIEW_OF(&treemap_compact), TREEMAP_COMPACT_SHOW_LABELS_ACTIVE);
        egui_view_treemap_chart_set_palette(EGUI_VIEW_OF(&treemap_compact), EGUI_COLOR_HEX(TREEMAP_COMPACT_ACTIVE_BG),
                                            EGUI_COLOR_HEX(TREEMAP_COMPACT_ACTIVE_BORDER), EGUI_COLOR_HEX(TREEMAP_COMPACT_ACTIVE_TEXT),
                                            EGUI_COLOR_HEX(TREEMAP_COMPACT_ACTIVE_MUTED));
    }
    else
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(TREEMAP_COMPACT_LABEL_COLOR_IDLE), EGUI_ALPHA_100);
        egui_view_treemap_chart_set_show_labels(EGUI_VIEW_OF(&treemap_compact), TREEMAP_COMPACT_SHOW_LABELS_IDLE);
        egui_view_treemap_chart_set_palette(EGUI_VIEW_OF(&treemap_compact), EGUI_COLOR_HEX(TREEMAP_COMPACT_IDLE_BG),
                                            EGUI_COLOR_HEX(TREEMAP_COMPACT_IDLE_BORDER), EGUI_COLOR_HEX(TREEMAP_COMPACT_IDLE_TEXT),
                                            EGUI_COLOR_HEX(TREEMAP_COMPACT_IDLE_MUTED));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_treemap_chart_get_current_value_set(self) + 1) % 2;
    egui_view_treemap_chart_set_current_value_set(self, next);
    apply_compact_state(egui_view_treemap_chart_get_current_value_set(EGUI_VIEW_OF(&treemap_compact)), 0);
    set_status((next == 0) ? TREEMAP_STATUS_TEXT_A : TREEMAP_STATUS_TEXT_B, EGUI_COLOR_HEX(TREEMAP_STATUS_COLOR_PRIMARY));
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_treemap_chart_get_current_value_set(self) + 1) % 2;
    egui_view_treemap_chart_set_current_value_set(self, next);
    apply_compact_state(next, 1);
    set_status((next == 0) ? TREEMAP_COMPACT_STATUS_A : TREEMAP_COMPACT_STATUS_B, EGUI_COLOR_HEX(TREEMAP_STATUS_COLOR_COMPACT));
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), TREEMAP_ROOT_WIDTH, TREEMAP_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), TREEMAP_TITLE_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(TREEMAP_TITLE_COLOR), EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), TREEMAP_TITLE_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(TREEMAP_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, TREEMAP_GUIDE_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_treemap_chart_init(EGUI_VIEW_OF(&treemap_primary));
    egui_view_set_size(EGUI_VIEW_OF(&treemap_primary), TREEMAP_PRIMARY_WIDTH, TREEMAP_PRIMARY_HEIGHT);
    egui_view_treemap_chart_set_item_labels(EGUI_VIEW_OF(&treemap_primary), primary_labels, 5);
    egui_view_treemap_chart_set_value_set(EGUI_VIEW_OF(&treemap_primary), 0, primary_values_a);
    egui_view_treemap_chart_set_value_set(EGUI_VIEW_OF(&treemap_primary), 1, primary_values_b);
    egui_view_treemap_chart_set_current_value_set(EGUI_VIEW_OF(&treemap_primary), 0);
    egui_view_treemap_chart_set_palette(EGUI_VIEW_OF(&treemap_primary), EGUI_COLOR_HEX(TREEMAP_PRIMARY_BG), EGUI_COLOR_HEX(TREEMAP_PRIMARY_BORDER),
                                        EGUI_COLOR_HEX(TREEMAP_PRIMARY_TEXT), EGUI_COLOR_HEX(TREEMAP_PRIMARY_MUTED));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&treemap_primary), on_primary_click);
    egui_view_set_margin(EGUI_VIEW_OF(&treemap_primary), 0, 0, 0, TREEMAP_PRIMARY_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&treemap_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), TREEMAP_TITLE_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), TREEMAP_STATUS_TEXT_A);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(TREEMAP_STATUS_COLOR_PRIMARY), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, TREEMAP_STATUS_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), TREEMAP_DIVIDER_WIDTH, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), section_divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(TREEMAP_DIVIDER_COLOR));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, TREEMAP_DIVIDER_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), TREEMAP_BOTTOM_ROW_WIDTH, TREEMAP_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), TREEMAP_COLUMN_WIDTH, TREEMAP_COLUMN_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, TREEMAP_COLUMN_GAP, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), TREEMAP_COLUMN_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), TREEMAP_COMPACT_LABEL_TEXT);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(TREEMAP_COMPACT_LABEL_COLOR_IDLE), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_treemap_chart_init(EGUI_VIEW_OF(&treemap_compact));
    egui_view_set_size(EGUI_VIEW_OF(&treemap_compact), TREEMAP_COLUMN_WIDTH, 90);
    egui_view_treemap_chart_set_item_labels(EGUI_VIEW_OF(&treemap_compact), compact_labels, 4);
    egui_view_treemap_chart_set_value_set(EGUI_VIEW_OF(&treemap_compact), 0, compact_values_a);
    egui_view_treemap_chart_set_value_set(EGUI_VIEW_OF(&treemap_compact), 1, compact_values_b);
    egui_view_treemap_chart_set_current_value_set(EGUI_VIEW_OF(&treemap_compact), 0);
    egui_view_treemap_chart_set_font(EGUI_VIEW_OF(&treemap_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_treemap_chart_set_show_header(EGUI_VIEW_OF(&treemap_compact), 0);
    apply_compact_state(0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&treemap_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&treemap_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&disabled_column));
    egui_view_set_size(EGUI_VIEW_OF(&disabled_column), TREEMAP_COLUMN_WIDTH, TREEMAP_COLUMN_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&disabled_column), TREEMAP_COLUMN_GAP, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&disabled_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&disabled_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&disabled_column));

    egui_view_label_init(EGUI_VIEW_OF(&disabled_label));
    egui_view_set_size(EGUI_VIEW_OF(&disabled_label), TREEMAP_COLUMN_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&disabled_label), TREEMAP_LOCKED_LABEL_TEXT);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&disabled_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&disabled_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&disabled_label), 0, 0, 0, 3);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&disabled_label), EGUI_COLOR_HEX(TREEMAP_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&disabled_column), EGUI_VIEW_OF(&disabled_label));

    egui_view_treemap_chart_init(EGUI_VIEW_OF(&treemap_disabled));
    egui_view_set_size(EGUI_VIEW_OF(&treemap_disabled), TREEMAP_COLUMN_WIDTH, 90);
    egui_view_treemap_chart_set_item_labels(EGUI_VIEW_OF(&treemap_disabled), compact_labels, 4);
    egui_view_treemap_chart_set_value_set(EGUI_VIEW_OF(&treemap_disabled), 0, compact_values_a);
    egui_view_treemap_chart_set_current_value_set(EGUI_VIEW_OF(&treemap_disabled), 0);
    egui_view_treemap_chart_set_show_labels(EGUI_VIEW_OF(&treemap_disabled), 0);
    egui_view_treemap_chart_set_palette(EGUI_VIEW_OF(&treemap_disabled), EGUI_COLOR_HEX(TREEMAP_LOCKED_BG), EGUI_COLOR_HEX(TREEMAP_LOCKED_BORDER),
                                        EGUI_COLOR_HEX(TREEMAP_LOCKED_TEXT), EGUI_COLOR_HEX(TREEMAP_LOCKED_MUTED));
    egui_view_treemap_chart_set_show_header(EGUI_VIEW_OF(&treemap_disabled), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&treemap_disabled), 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&disabled_column), EGUI_VIEW_OF(&treemap_disabled));

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&disabled_column));
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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&treemap_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&treemap_compact), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&treemap_disabled), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
