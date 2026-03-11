#define CANDLE_ROOT_WIDTH             230
#define CANDLE_ROOT_HEIGHT            306
#define CANDLE_TITLE_COLOR            0x63DDFF
#define CANDLE_GUIDE_TEXT             "Tap cards to cycle"
#define CANDLE_GUIDE_COLOR            0x839AB6
#define CANDLE_GUIDE_MARGIN_BOTTOM    3
#define CANDLE_PRIMARY_WIDTH          188
#define CANDLE_PRIMARY_HEIGHT         138
#define CANDLE_PRIMARY_MARGIN_BOTTOM  4
#define CANDLE_STATUS_TEXT_A          "Core A"
#define CANDLE_STATUS_TEXT_B          "Core B"
#define CANDLE_STATUS_TEXT_COMPACT_A  "Compact A"
#define CANDLE_STATUS_TEXT_COMPACT_B  "Compact B"
#define CANDLE_STATUS_PRIMARY_COLOR   0x56D3FF
#define CANDLE_STATUS_DOWN_COLOR      0xFB7185
#define CANDLE_STATUS_COMPACT_COLOR   0xF59E0B
#define CANDLE_STATUS_MARGIN_BOTTOM   4
#define CANDLE_DIVIDER_WIDTH          170
#define CANDLE_DIVIDER_POINTS         169
#define CANDLE_DIVIDER_PRIMARY        0x5C8BBC
#define CANDLE_DIVIDER_DOWN           0x7A5360
#define CANDLE_DIVIDER_COMPACT        0x9A6A32
#define CANDLE_DIVIDER_MARGIN_BOTTOM  5
#define CANDLE_BOTTOM_ROW_WIDTH       230
#define CANDLE_BOTTOM_ROW_HEIGHT      106
#define CANDLE_COLUMN_WIDTH           108
#define CANDLE_COLUMN_HEIGHT          108
#define CANDLE_COLUMN_GAP             6
#define CANDLE_CARD_HEIGHT            92
#define CANDLE_COMPACT_LABEL_TEXT     "Compact"
#define CANDLE_LOCKED_LABEL_TEXT      "Locked"
#define CANDLE_COMPACT_LABEL_IDLE     0xB7C5D4
#define CANDLE_COMPACT_LABEL_ACTIVE   0xF4C96D
#define CANDLE_LOCKED_LABEL_COLOR     0xCCD6E1
#define CANDLE_PRIMARY_BG             0x0E1726
#define CANDLE_PRIMARY_BORDER         0x506077
#define CANDLE_PRIMARY_RISE           0x22C55E
#define CANDLE_PRIMARY_FALL           0xF43F5E
#define CANDLE_PRIMARY_NEUTRAL        0x94A3B8
#define CANDLE_PRIMARY_TEXT           0xE2E8F0
#define CANDLE_PRIMARY_MUTED          0x94A3B8
#define CANDLE_COMPACT_ACTIVE_BG      0x22160A
#define CANDLE_COMPACT_ACTIVE_BORDER  0x82531A
#define CANDLE_COMPACT_ACTIVE_RISE    0x34D399
#define CANDLE_COMPACT_ACTIVE_FALL    0xFB7185
#define CANDLE_COMPACT_ACTIVE_NEUTRAL 0xA8B4C4
#define CANDLE_COMPACT_ACTIVE_TEXT    0xFDE68A
#define CANDLE_COMPACT_ACTIVE_MUTED   0xC08A1B
#define CANDLE_COMPACT_IDLE_BG        0x121A28
#define CANDLE_COMPACT_IDLE_BORDER    0x35506E
#define CANDLE_COMPACT_IDLE_RISE      0x22C55E
#define CANDLE_COMPACT_IDLE_FALL      0xF43F5E
#define CANDLE_COMPACT_IDLE_NEUTRAL   0x8EA2B5
#define CANDLE_COMPACT_IDLE_TEXT      0xCBD5E1
#define CANDLE_COMPACT_IDLE_MUTED     0x70839C
#define CANDLE_LOCKED_BG              0x0E1522
#define CANDLE_LOCKED_BORDER          0x46566B
#define CANDLE_LOCKED_RISE            0x758E83
#define CANDLE_LOCKED_FALL            0x9E7B84
#define CANDLE_LOCKED_NEUTRAL         0x7E8A98
#define CANDLE_LOCKED_TEXT            0xB8C3D2
#define CANDLE_LOCKED_MUTED           0x8695A9

#include <stdlib.h>

#include "egui.h"
#include "egui_view_candlestick_chart.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_candlestick_chart_t candle_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_candlestick_chart_t candle_compact;
static egui_view_label_t compact_label;
static egui_view_linearlayout_t locked_column;
static egui_view_candlestick_chart_t candle_locked;
static egui_view_label_t locked_label;

static const char *title_text = "Candlestick Chart";
static const char *guide_text = CANDLE_GUIDE_TEXT;
static const egui_view_line_point_t section_divider_points[] = {{0, 0}, {CANDLE_DIVIDER_POINTS, 0}};

static const char *primary_labels[] = {"M1", "M2", "M3", "M4", "M5", "M6", "M7", "M8"};
static const uint8_t primary_open_a[] = {32, 36, 34, 41, 45, 43, 47, 52};
static const uint8_t primary_high_a[] = {40, 39, 42, 48, 51, 50, 54, 58};
static const uint8_t primary_low_a[] = {28, 31, 30, 35, 39, 37, 42, 46};
static const uint8_t primary_close_a[] = {37, 33, 40, 46, 41, 49, 52, 50};
static const uint8_t primary_open_b[] = {55, 51, 53, 47, 49, 44, 42, 38};
static const uint8_t primary_high_b[] = {60, 56, 58, 53, 52, 49, 46, 43};
static const uint8_t primary_low_b[] = {50, 47, 48, 42, 45, 39, 36, 33};
static const uint8_t primary_close_b[] = {52, 54, 49, 51, 46, 41, 39, 35};

static const char *compact_labels[] = {"1", "2", "3", "4", "5", "6"};
static const uint8_t compact_open_a[] = {24, 29, 27, 33, 31, 36};
static const uint8_t compact_high_a[] = {29, 34, 33, 38, 37, 42};
static const uint8_t compact_low_a[] = {20, 24, 23, 28, 27, 31};
static const uint8_t compact_close_a[] = {28, 26, 31, 30, 35, 33};
static const uint8_t compact_open_b[] = {35, 33, 31, 28, 26, 24};
static const uint8_t compact_high_b[] = {40, 37, 35, 33, 30, 28};
static const uint8_t compact_low_b[] = {31, 29, 27, 24, 22, 20};
static const uint8_t compact_close_b[] = {32, 30, 28, 25, 23, 21};

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
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), CANDLE_COMPACT_LABEL_TEXT);
    if (is_active)
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(CANDLE_COMPACT_LABEL_ACTIVE), EGUI_ALPHA_100);
        egui_view_candlestick_chart_set_show_labels(EGUI_VIEW_OF(&candle_compact), 1);
        egui_view_candlestick_chart_set_palette(EGUI_VIEW_OF(&candle_compact), EGUI_COLOR_HEX(CANDLE_COMPACT_ACTIVE_BG),
                                                EGUI_COLOR_HEX(CANDLE_COMPACT_ACTIVE_BORDER), EGUI_COLOR_HEX(CANDLE_COMPACT_ACTIVE_RISE),
                                                EGUI_COLOR_HEX(CANDLE_COMPACT_ACTIVE_FALL), EGUI_COLOR_HEX(CANDLE_COMPACT_ACTIVE_NEUTRAL),
                                                EGUI_COLOR_HEX(CANDLE_COMPACT_ACTIVE_TEXT), EGUI_COLOR_HEX(CANDLE_COMPACT_ACTIVE_MUTED));
    }
    else
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(CANDLE_COMPACT_LABEL_IDLE), EGUI_ALPHA_100);
        egui_view_candlestick_chart_set_show_labels(EGUI_VIEW_OF(&candle_compact), 0);
        egui_view_candlestick_chart_set_palette(EGUI_VIEW_OF(&candle_compact), EGUI_COLOR_HEX(CANDLE_COMPACT_IDLE_BG),
                                                EGUI_COLOR_HEX(CANDLE_COMPACT_IDLE_BORDER), EGUI_COLOR_HEX(CANDLE_COMPACT_IDLE_RISE),
                                                EGUI_COLOR_HEX(CANDLE_COMPACT_IDLE_FALL), EGUI_COLOR_HEX(CANDLE_COMPACT_IDLE_NEUTRAL),
                                                EGUI_COLOR_HEX(CANDLE_COMPACT_IDLE_TEXT), EGUI_COLOR_HEX(CANDLE_COMPACT_IDLE_MUTED));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_candlestick_chart_get_current_value_set(self) + 1) % 2;

    egui_view_candlestick_chart_set_current_value_set(self, next);
    apply_compact_state(egui_view_candlestick_chart_get_current_value_set(EGUI_VIEW_OF(&candle_compact)), 0);
    if (next == 0)
    {
        set_status(CANDLE_STATUS_TEXT_A, EGUI_COLOR_HEX(CANDLE_STATUS_PRIMARY_COLOR), EGUI_COLOR_HEX(CANDLE_GUIDE_COLOR),
                   EGUI_COLOR_HEX(CANDLE_DIVIDER_PRIMARY));
    }
    else
    {
        set_status(CANDLE_STATUS_TEXT_B, EGUI_COLOR_HEX(CANDLE_STATUS_DOWN_COLOR), EGUI_COLOR_HEX(0xA27783), EGUI_COLOR_HEX(CANDLE_DIVIDER_DOWN));
    }
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_candlestick_chart_get_current_value_set(self) + 1) % 2;

    egui_view_candlestick_chart_set_current_value_set(self, next);
    apply_compact_state(next, 1);
    set_status((next == 0) ? CANDLE_STATUS_TEXT_COMPACT_A : CANDLE_STATUS_TEXT_COMPACT_B, EGUI_COLOR_HEX(CANDLE_STATUS_COMPACT_COLOR), EGUI_COLOR_HEX(0xA07A42),
               EGUI_COLOR_HEX(CANDLE_DIVIDER_COMPACT));
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), CANDLE_ROOT_WIDTH, CANDLE_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), CANDLE_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(CANDLE_TITLE_COLOR), EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), CANDLE_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(CANDLE_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, CANDLE_GUIDE_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_candlestick_chart_init(EGUI_VIEW_OF(&candle_primary));
    egui_view_set_size(EGUI_VIEW_OF(&candle_primary), CANDLE_PRIMARY_WIDTH, CANDLE_PRIMARY_HEIGHT);
    egui_view_candlestick_chart_set_item_labels(EGUI_VIEW_OF(&candle_primary), primary_labels, 8);
    egui_view_candlestick_chart_set_value_set(EGUI_VIEW_OF(&candle_primary), 0, primary_open_a, primary_high_a, primary_low_a, primary_close_a);
    egui_view_candlestick_chart_set_value_set(EGUI_VIEW_OF(&candle_primary), 1, primary_open_b, primary_high_b, primary_low_b, primary_close_b);
    egui_view_candlestick_chart_set_current_value_set(EGUI_VIEW_OF(&candle_primary), 0);
    egui_view_candlestick_chart_set_palette(EGUI_VIEW_OF(&candle_primary), EGUI_COLOR_HEX(CANDLE_PRIMARY_BG), EGUI_COLOR_HEX(CANDLE_PRIMARY_BORDER),
                                            EGUI_COLOR_HEX(CANDLE_PRIMARY_RISE), EGUI_COLOR_HEX(CANDLE_PRIMARY_FALL), EGUI_COLOR_HEX(CANDLE_PRIMARY_NEUTRAL),
                                            EGUI_COLOR_HEX(CANDLE_PRIMARY_TEXT), EGUI_COLOR_HEX(CANDLE_PRIMARY_MUTED));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&candle_primary), on_primary_click);
    egui_view_set_margin(EGUI_VIEW_OF(&candle_primary), 0, 0, 0, CANDLE_PRIMARY_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&candle_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), CANDLE_ROOT_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), CANDLE_STATUS_TEXT_A);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(CANDLE_STATUS_PRIMARY_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, CANDLE_STATUS_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), CANDLE_DIVIDER_WIDTH, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), section_divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(CANDLE_DIVIDER_PRIMARY));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, CANDLE_DIVIDER_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), CANDLE_BOTTOM_ROW_WIDTH, CANDLE_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), CANDLE_COLUMN_WIDTH, CANDLE_COLUMN_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, CANDLE_COLUMN_GAP, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), CANDLE_COLUMN_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), CANDLE_COMPACT_LABEL_TEXT);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(CANDLE_COMPACT_LABEL_IDLE), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_candlestick_chart_init(EGUI_VIEW_OF(&candle_compact));
    egui_view_set_size(EGUI_VIEW_OF(&candle_compact), CANDLE_COLUMN_WIDTH, CANDLE_CARD_HEIGHT);
    egui_view_candlestick_chart_set_item_labels(EGUI_VIEW_OF(&candle_compact), compact_labels, 6);
    egui_view_candlestick_chart_set_value_set(EGUI_VIEW_OF(&candle_compact), 0, compact_open_a, compact_high_a, compact_low_a, compact_close_a);
    egui_view_candlestick_chart_set_value_set(EGUI_VIEW_OF(&candle_compact), 1, compact_open_b, compact_high_b, compact_low_b, compact_close_b);
    egui_view_candlestick_chart_set_current_value_set(EGUI_VIEW_OF(&candle_compact), 0);
    egui_view_candlestick_chart_set_font(EGUI_VIEW_OF(&candle_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_candlestick_chart_set_show_header(EGUI_VIEW_OF(&candle_compact), 0);
    apply_compact_state(0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&candle_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&candle_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), CANDLE_COLUMN_WIDTH, CANDLE_COLUMN_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), CANDLE_COLUMN_GAP, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), CANDLE_COLUMN_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), CANDLE_LOCKED_LABEL_TEXT);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(CANDLE_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_candlestick_chart_init(EGUI_VIEW_OF(&candle_locked));
    egui_view_set_size(EGUI_VIEW_OF(&candle_locked), CANDLE_COLUMN_WIDTH, CANDLE_CARD_HEIGHT);
    egui_view_candlestick_chart_set_item_labels(EGUI_VIEW_OF(&candle_locked), compact_labels, 6);
    egui_view_candlestick_chart_set_value_set(EGUI_VIEW_OF(&candle_locked), 0, compact_open_a, compact_high_a, compact_low_a, compact_close_a);
    egui_view_candlestick_chart_set_current_value_set(EGUI_VIEW_OF(&candle_locked), 0);
    egui_view_candlestick_chart_set_show_labels(EGUI_VIEW_OF(&candle_locked), 0);
    egui_view_candlestick_chart_set_show_header(EGUI_VIEW_OF(&candle_locked), 0);
    egui_view_candlestick_chart_set_palette(EGUI_VIEW_OF(&candle_locked), EGUI_COLOR_HEX(CANDLE_LOCKED_BG), EGUI_COLOR_HEX(CANDLE_LOCKED_BORDER),
                                            EGUI_COLOR_HEX(CANDLE_LOCKED_RISE), EGUI_COLOR_HEX(CANDLE_LOCKED_FALL), EGUI_COLOR_HEX(CANDLE_LOCKED_NEUTRAL),
                                            EGUI_COLOR_HEX(CANDLE_LOCKED_TEXT), EGUI_COLOR_HEX(CANDLE_LOCKED_MUTED));
    egui_view_set_enable(EGUI_VIEW_OF(&candle_locked), 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&candle_locked));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&candle_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&candle_compact), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&candle_locked), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
