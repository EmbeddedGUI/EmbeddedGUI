#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "app_egui_resource_generate.h"

#ifndef EGUI_SHOWCASE_PARITY_RECORDING
#define EGUI_SHOWCASE_PARITY_RECORDING 0
#endif

#ifndef EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK
#define EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK 0
#endif

#define SHOWCASE_CANVAS_WIDTH       HELLO_VIRTUAL_STAGE_SHOWCASE_CANVAS_WIDTH
#define SHOWCASE_CANVAS_HEIGHT      HELLO_VIRTUAL_STAGE_SHOWCASE_CANVAS_HEIGHT
#define SHOWCASE_KEYBOARD_HEIGHT    128
#define SHOWCASE_KEYBOARD_Y         ((EGUI_CONFIG_SCEEN_HEIGHT > SHOWCASE_KEYBOARD_HEIGHT) ? (EGUI_CONFIG_SCEEN_HEIGHT - SHOWCASE_KEYBOARD_HEIGHT) : 0)
#define SHOWCASE_DIRTY_ANIM_FOCUS_X 96
#define SHOWCASE_DIRTY_ANIM_FOCUS_Y 250

#define SHOWCASE_NODE_COUNT 37U
#if EGUI_SHOWCASE_PARITY_RECORDING
#define SHOWCASE_LIVE_SLOT_LIMIT    20U
#define SHOWCASE_PINNED_SLOT_BUDGET 20U
#else
#define SHOWCASE_LIVE_SLOT_LIMIT    2U
#define SHOWCASE_PINNED_SLOT_BUDGET 6U
#endif

enum
{
    SHOWCASE_VIEW_TYPE_BUTTON = 1,
    SHOWCASE_VIEW_TYPE_TEXTINPUT,
    SHOWCASE_VIEW_TYPE_SWITCH,
    SHOWCASE_VIEW_TYPE_CHECKBOX,
    SHOWCASE_VIEW_TYPE_RADIO_BUTTON,
    SHOWCASE_VIEW_TYPE_TOGGLE_BUTTON,
    SHOWCASE_VIEW_TYPE_SLIDER,
    SHOWCASE_VIEW_TYPE_ARC_SLIDER,
    SHOWCASE_VIEW_TYPE_NUMBER_PICKER,
    SHOWCASE_VIEW_TYPE_ROLLER,
    SHOWCASE_VIEW_TYPE_COMBOBOX,
    SHOWCASE_VIEW_TYPE_MINI_CALENDAR,
    SHOWCASE_VIEW_TYPE_BUTTON_MATRIX,
    SHOWCASE_VIEW_TYPE_TAB_BAR,
    SHOWCASE_VIEW_TYPE_LIST,
    SHOWCASE_VIEW_TYPE_LAYER_CARD,
};

enum
{
    SHOWCASE_NODE_BASIC_STATIC = 100,
    SHOWCASE_NODE_TOGGLE_STATIC,
    SHOWCASE_NODE_PROGRESS_STATIC,
    SHOWCASE_NODE_CANVAS_STATIC,
    SHOWCASE_NODE_SLIDER_STATIC,
    SHOWCASE_NODE_CHART_STATIC,
    SHOWCASE_NODE_TIME_STATIC,
    SHOWCASE_NODE_SPECIAL_STATIC,
    SHOWCASE_NODE_DATA_STATIC,
    SHOWCASE_NODE_ALPHA_LABELS,
    SHOWCASE_NODE_BUTTON_BASIC,
    SHOWCASE_NODE_TEXTINPUT,
    SHOWCASE_NODE_SWITCH,
    SHOWCASE_NODE_CHECKBOX,
    SHOWCASE_NODE_OPTION1,
    SHOWCASE_NODE_OPTION2,
    SHOWCASE_NODE_TOGGLE_BUTTON,
    SHOWCASE_NODE_SLIDER,
    SHOWCASE_NODE_ARC_SLIDER,
    SHOWCASE_NODE_NUMBER_PICKER,
    SHOWCASE_NODE_ROLLER,
    SHOWCASE_NODE_COMBOBOX,
    SHOWCASE_NODE_MINI_CALENDAR,
    SHOWCASE_NODE_BUTTON_MATRIX,
    SHOWCASE_NODE_TAB_BAR,
    SHOWCASE_NODE_LIST,
    SHOWCASE_NODE_ALPHA1,
    SHOWCASE_NODE_ALPHA2,
    SHOWCASE_NODE_LANG_BUTTON,
    SHOWCASE_NODE_THEME_BUTTON,
    SHOWCASE_NODE_PROGRESS_BAR_ANIM,
    SHOWCASE_NODE_CIRCULAR_PROGRESS_ANIM,
    SHOWCASE_NODE_ACTIVITY_RING_ANIM,
    SHOWCASE_NODE_SPINNER_ANIM,
    SHOWCASE_NODE_ANALOG_CLOCK_ANIM,
    SHOWCASE_NODE_DIGITAL_TIME_ANIM,
    SHOWCASE_NODE_HEART_RATE_ANIM,
};

enum
{
    SHOWCASE_NODE_INDEX_BASIC_STATIC = 0,
    SHOWCASE_NODE_INDEX_TOGGLE_STATIC,
    SHOWCASE_NODE_INDEX_PROGRESS_STATIC,
    SHOWCASE_NODE_INDEX_CANVAS_STATIC,
    SHOWCASE_NODE_INDEX_SLIDER_STATIC,
    SHOWCASE_NODE_INDEX_CHART_STATIC,
    SHOWCASE_NODE_INDEX_TIME_STATIC,
    SHOWCASE_NODE_INDEX_SPECIAL_STATIC,
    SHOWCASE_NODE_INDEX_DATA_STATIC,
    SHOWCASE_NODE_INDEX_ALPHA_LABELS,
    SHOWCASE_NODE_INDEX_BUTTON_BASIC,
    SHOWCASE_NODE_INDEX_TEXTINPUT,
    SHOWCASE_NODE_INDEX_SWITCH,
    SHOWCASE_NODE_INDEX_CHECKBOX,
    SHOWCASE_NODE_INDEX_OPTION1,
    SHOWCASE_NODE_INDEX_OPTION2,
    SHOWCASE_NODE_INDEX_TOGGLE_BUTTON,
    SHOWCASE_NODE_INDEX_SLIDER,
    SHOWCASE_NODE_INDEX_ARC_SLIDER,
    SHOWCASE_NODE_INDEX_NUMBER_PICKER,
    SHOWCASE_NODE_INDEX_ROLLER,
    SHOWCASE_NODE_INDEX_COMBOBOX,
    SHOWCASE_NODE_INDEX_MINI_CALENDAR,
    SHOWCASE_NODE_INDEX_BUTTON_MATRIX,
    SHOWCASE_NODE_INDEX_TAB_BAR,
    SHOWCASE_NODE_INDEX_LIST,
    SHOWCASE_NODE_INDEX_ALPHA1,
    SHOWCASE_NODE_INDEX_ALPHA2,
    SHOWCASE_NODE_INDEX_LANG_BUTTON,
    SHOWCASE_NODE_INDEX_THEME_BUTTON,
    SHOWCASE_NODE_INDEX_PROGRESS_BAR_ANIM,
    SHOWCASE_NODE_INDEX_CIRCULAR_PROGRESS_ANIM,
    SHOWCASE_NODE_INDEX_ACTIVITY_RING_ANIM,
    SHOWCASE_NODE_INDEX_SPINNER_ANIM,
    SHOWCASE_NODE_INDEX_ANALOG_CLOCK_ANIM,
    SHOWCASE_NODE_INDEX_DIGITAL_TIME_ANIM,
    SHOWCASE_NODE_INDEX_HEART_RATE_ANIM,
};

typedef struct showcase_stage_node
{
    egui_virtual_stage_node_desc_t desc;
} showcase_stage_node_t;

typedef struct showcase_live_textinput_view
{
    egui_view_textinput_t textinput;
    egui_view_api_t focus_api;
} showcase_live_textinput_view_t;

typedef struct showcase_scratch_card
{
    egui_view_card_t card;
    egui_view_label_t child;
} showcase_scratch_card_t;

typedef struct showcase_scratch_window
{
    egui_view_window_t window;
    egui_view_label_t content;
} showcase_scratch_window_t;

typedef union showcase_scratch_widget
{
    egui_view_t view;
    egui_view_label_t label;
    egui_view_button_t button;
    egui_view_textblock_t textblock;
    egui_view_dynamic_label_t dynamic_label;
    showcase_scratch_card_t card;
    egui_view_textinput_t textinput;
    egui_view_switch_t switch_view;
    egui_view_checkbox_t checkbox;
    egui_view_radio_button_t radio;
    egui_view_toggle_button_t toggle_button;
    egui_view_led_t led;
    egui_view_notification_badge_t badge;
    egui_view_progress_bar_t progress_bar;
    egui_view_circular_progress_bar_t circular_progress_bar;
    egui_view_gauge_t gauge;
    egui_view_activity_ring_t activity_ring;
    egui_view_page_indicator_t page_indicator;
    egui_view_slider_t slider;
    egui_view_arc_slider_t arc_slider;
    egui_view_number_picker_t number_picker;
    egui_view_spinner_t spinner;
    egui_view_roller_t roller;
    egui_view_combobox_t combobox;
    egui_view_scale_t scale;
    egui_view_chart_line_t chart_line;
    egui_view_chart_bar_t chart_bar;
    egui_view_chart_pie_t chart_pie;
    egui_view_chart_scatter_t chart_scatter;
    egui_view_analog_clock_t analog_clock;
    egui_view_digital_clock_t digital_clock;
    egui_view_stopwatch_t stopwatch;
    egui_view_mini_calendar_t mini_calendar;
    egui_view_compass_t compass;
    egui_view_heart_rate_t heart_rate;
    egui_view_line_t line;
    egui_view_divider_t divider;
    egui_view_table_t table;
    egui_view_list_t list;
    egui_view_button_matrix_t button_matrix;
    egui_view_spangroup_t spangroup;
    egui_view_tab_bar_t tab_bar;
    showcase_scratch_window_t window;
    egui_view_menu_t menu;
} showcase_scratch_widget_t;

typedef struct showcase_context
{
    showcase_stage_node_t nodes[SHOWCASE_NODE_COUNT];
    uint8_t is_dark_theme;
    uint8_t is_chinese;
    uint8_t active_layer;
    uint8_t switch_checked;
    uint8_t checkbox_checked;
    uint8_t option_index;
    uint8_t toggle_checked;
    uint8_t slider_value;
    uint8_t arc_slider_value;
    uint8_t roller_index;
    uint8_t combobox_index;
    uint8_t calendar_day;
    uint8_t button_matrix_index;
    uint8_t tab_index;
    int16_t numpick_value;
    egui_dim_t list_scroll_y;
    uint16_t anim_tick;
    char textinput_text[EGUI_CONFIG_TEXTINPUT_MAX_LENGTH + 1];
} showcase_context_t;

static showcase_context_t showcase_ctx;
static showcase_scratch_widget_t showcase_scratch;
static egui_view_canvas_panner_t showcase_root;
static egui_view_virtual_stage_t showcase_stage_view;
static egui_view_keyboard_t showcase_keyboard_view;
static egui_timer_t showcase_anim_timer;
static uint8_t showcase_force_english_text;
#if EGUI_SHOWCASE_PARITY_RECORDING
static egui_timer_t showcase_bootstrap_timer;
#endif
static egui_region_t showcase_scratch_dirty_backup[EGUI_CONFIG_DIRTY_AREA_COUNT];
static uint8_t showcase_scratch_dirty_backup_valid;
static egui_dim_t showcase_draw_origin_x;
static egui_dim_t showcase_draw_origin_y;
static egui_view_t *showcase_find_live_view(uint32_t stable_id);

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;

static uint8_t showcase_recording_uses_small_screen(void)
{
    return (EGUI_CONFIG_SCEEN_WIDTH < SHOWCASE_CANVAS_WIDTH) || (EGUI_CONFIG_SCEEN_HEIGHT < SHOWCASE_CANVAS_HEIGHT);
}

static void showcase_sim_canvas_to_screen(egui_dim_t canvas_x, egui_dim_t canvas_y, int *x, int *y)
{
    egui_dim_t offset_x = egui_view_canvas_panner_get_offset_x(EGUI_VIEW_OF(&showcase_root));
    egui_dim_t offset_y = egui_view_canvas_panner_get_offset_y(EGUI_VIEW_OF(&showcase_root));

    *x = canvas_x - offset_x;
    *y = canvas_y - offset_y;
}

static void showcase_runtime_focus_node(uint32_t node_index)
{
    const egui_region_t *region = &showcase_ctx.nodes[node_index].desc.region;
    egui_dim_t target_x = region->location.x + region->size.width / 2 - EGUI_CONFIG_SCEEN_WIDTH / 2;
    egui_dim_t target_y = region->location.y + region->size.height / 2 - EGUI_CONFIG_SCEEN_HEIGHT / 2;
    egui_dim_t max_offset_x = SHOWCASE_CANVAS_WIDTH > EGUI_CONFIG_SCEEN_WIDTH ? (SHOWCASE_CANVAS_WIDTH - EGUI_CONFIG_SCEEN_WIDTH) : 0;
    egui_dim_t max_offset_y = SHOWCASE_CANVAS_HEIGHT > EGUI_CONFIG_SCEEN_HEIGHT ? (SHOWCASE_CANVAS_HEIGHT - EGUI_CONFIG_SCEEN_HEIGHT) : 0;

    if (target_x < 0)
    {
        target_x = 0;
    }
    else if (target_x > max_offset_x)
    {
        target_x = max_offset_x;
    }

    if (target_y < 0)
    {
        target_y = 0;
    }
    else if (target_y > max_offset_y)
    {
        target_y = max_offset_y;
    }

    egui_view_canvas_panner_set_offset(EGUI_VIEW_OF(&showcase_root), target_x, target_y);
    EGUI_VIEW_OF(&showcase_root)->api->calculate_layout(EGUI_VIEW_OF(&showcase_root));
    egui_view_invalidate(EGUI_VIEW_OF(&showcase_root));
}

static void showcase_sim_get_node_point(uint32_t node_index, uint8_t x_percent, uint8_t y_percent, int *x, int *y)
{
    const egui_region_t *region = &showcase_ctx.nodes[node_index].desc.region;

    showcase_sim_canvas_to_screen(region->location.x + (region->size.width * x_percent) / 100, region->location.y + (region->size.height * y_percent) / 100, x,
                                  y);
}

static void showcase_sim_set_click_node(egui_sim_action_t *p_action, uint32_t node_index, uint8_t x_percent, uint8_t y_percent, int interval_ms)
{
    showcase_sim_get_node_point(node_index, x_percent, y_percent, &p_action->x1, &p_action->y1);
    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->interval_ms = interval_ms;
}

static void showcase_sim_set_drag_node(egui_sim_action_t *p_action, uint32_t node_index, uint8_t from_x_percent, uint8_t from_y_percent, uint8_t to_x_percent,
                                       uint8_t to_y_percent, int steps, int interval_ms)
{
    showcase_sim_get_node_point(node_index, from_x_percent, from_y_percent, &p_action->x1, &p_action->y1);
    showcase_sim_get_node_point(node_index, to_x_percent, to_y_percent, &p_action->x2, &p_action->y2);
    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->steps = steps;
    p_action->interval_ms = interval_ms;
}

static void showcase_sim_set_screen_drag(egui_sim_action_t *p_action, int x1, int y1, int x2, int y2, int steps, int interval_ms)
{
    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = x1;
    p_action->y1 = y1;
    p_action->x2 = x2;
    p_action->y2 = y2;
    p_action->steps = steps;
    p_action->interval_ms = interval_ms;
}

static uint8_t showcase_sim_set_combobox_item_click(egui_sim_action_t *p_action, uint8_t item_index, int interval_ms)
{
    egui_view_t *combobox = showcase_find_live_view(SHOWCASE_NODE_COMBOBOX);
    egui_view_combobox_t *local;

    if (combobox == NULL)
    {
        return 0U;
    }

    local = (egui_view_combobox_t *)combobox;
    if (!egui_view_combobox_is_expanded(combobox))
    {
        return 0U;
    }

    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = combobox->region_screen.location.x + combobox->region_screen.size.width / 2;
    p_action->y1 = combobox->region_screen.location.y + local->collapsed_height + local->item_height * item_index + local->item_height / 2;
    p_action->interval_ms = interval_ms;
    return 1U;
}

static uint8_t showcase_sim_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint8_t total_days;

    if (month < 1U || month > 12U)
    {
        return 30U;
    }

    total_days = days[month - 1U];
    if (month == 2U && (year % 4U == 0U && (year % 100U != 0U || year % 400U == 0U)))
    {
        total_days = 29U;
    }

    return total_days;
}

static uint8_t showcase_sim_day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint16_t y = year;

    if (month < 1U || month > 12U)
    {
        return 0U;
    }

    if (month < 3U)
    {
        y--;
    }

    return (uint8_t)((y + y / 4U - y / 100U + y / 400U + (uint16_t)t[month - 1U] + day) % 7U);
}

static void showcase_sim_get_arc_slider_point(uint8_t value, int *x, int *y)
{
    const egui_region_t *region = &showcase_ctx.nodes[SHOWCASE_NODE_INDEX_ARC_SLIDER].desc.region;
    egui_dim_t center_x = region->location.x + region->size.width / 2;
    egui_dim_t center_y = region->location.y + region->size.height / 2;
    egui_dim_t radius = EGUI_MIN(region->size.width, region->size.height) / 2 - 9 - 1 - 6;
    int16_t angle_deg = (int16_t)(150 + (240 * value) / 100);
    egui_float_t angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(angle_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));

    *x = center_x + (int)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(radius), EGUI_FLOAT_COS(angle_rad)));
    *y = center_y + (int)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(radius), EGUI_FLOAT_SIN(angle_rad)));
    showcase_sim_canvas_to_screen(*x, *y, x, y);
}

static void showcase_sim_set_arc_slider_drag(egui_sim_action_t *p_action, uint8_t from_value, uint8_t to_value, int steps, int interval_ms)
{
    showcase_sim_get_arc_slider_point(from_value, &p_action->x1, &p_action->y1);
    showcase_sim_get_arc_slider_point(to_value, &p_action->x2, &p_action->y2);
    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->steps = steps;
    p_action->interval_ms = interval_ms;
}

static uint8_t showcase_sim_set_calendar_day_click(egui_sim_action_t *p_action, uint8_t day, int interval_ms)
{
    const egui_region_t *region = &showcase_ctx.nodes[SHOWCASE_NODE_INDEX_MINI_CALENDAR].desc.region;
    egui_dim_t cell_w = region->size.width / 7;
    egui_dim_t header_h = region->size.height / 8;
    egui_dim_t cell_h = (region->size.height - header_h * 2) / 6;
    uint8_t total_days = showcase_sim_days_in_month(2026, 3);
    uint8_t start_col = showcase_sim_day_of_week(2026, 3, 1);
    uint8_t pos;
    uint8_t col;
    uint8_t row;

    if (day < 1U || day > total_days || cell_w <= 0 || cell_h <= 0)
    {
        return 0U;
    }

    pos = (uint8_t)(start_col + day - 1U);
    col = (uint8_t)(pos % 7U);
    row = (uint8_t)(pos / 7U);

    p_action->type = EGUI_SIM_ACTION_CLICK;
    showcase_sim_canvas_to_screen(region->location.x + col * cell_w + cell_w / 2, region->location.y + header_h * 2 + row * cell_h + cell_h / 2, &p_action->x1,
                                  &p_action->y1);
    p_action->interval_ms = interval_ms;
    return 1U;
}

static void showcase_sim_set_button_matrix_click(egui_sim_action_t *p_action, uint8_t index, int interval_ms)
{
    const egui_region_t *region = &showcase_ctx.nodes[SHOWCASE_NODE_INDEX_BUTTON_MATRIX].desc.region;
    uint8_t cols = 3;
    uint8_t rows = 2;
    uint8_t gap = 2;
    egui_dim_t btn_w = (region->size.width - gap * (cols - 1U)) / cols;
    egui_dim_t btn_h = (region->size.height - gap * (rows - 1U)) / rows;
    uint8_t col = (uint8_t)(index % cols);
    uint8_t row = (uint8_t)(index / cols);

    p_action->type = EGUI_SIM_ACTION_CLICK;
    showcase_sim_canvas_to_screen(region->location.x + col * (btn_w + gap) + btn_w / 2, region->location.y + row * (btn_h + gap) + btn_h / 2, &p_action->x1,
                                  &p_action->y1);
    p_action->interval_ms = interval_ms;
}

static void showcase_sim_set_tab_bar_click(egui_sim_action_t *p_action, uint8_t index, int interval_ms)
{
    const egui_region_t *region = &showcase_ctx.nodes[SHOWCASE_NODE_INDEX_TAB_BAR].desc.region;
    egui_dim_t tab_w = region->size.width / 3;

    p_action->type = EGUI_SIM_ACTION_CLICK;
    showcase_sim_canvas_to_screen(region->location.x + index * tab_w + tab_w / 2, region->location.y + region->size.height / 2, &p_action->x1, &p_action->y1);
    p_action->interval_ms = interval_ms;
}
#endif

static const char *showcase_roller_items[] = {"Mon", "Tue", "Wed", "Thu", "Fri"};
static const char *showcase_roller_items_cn[] = {"周一", "周二", "周三", "周四", "周五"};
static const char *showcase_combo_items[] = {"Sky", "Mint", "Steel"};
static const char *showcase_combo_items_cn[] = {"天空", "薄荷", "钢铁"};
static const char *showcase_tab_texts[] = {"Home", "Set", "Info"};
static const char *showcase_tab_texts_cn[] = {"首页", "设置", "信息"};
static const char *showcase_matrix_labels[] = {"1", "2", "3", "4", "5", "6"};
static const char *showcase_list_items[] = {"Item A", "Item B", "Item C", "Item D", "Item E", "Item F", "Item G", "Item H"};
static const char *showcase_list_items_cn[] = {"项目 A", "项目 B", "项目 C", "项目 D", "项目 E", "项目 F", "项目 G", "项目 H"};
static const char *showcase_calendar_weekdays_cn[] = {"日", "一", "二", "三", "四", "五", "六"};
static const egui_view_menu_item_t showcase_menu_items[] = {
        {.text = "Settings", .sub_page_index = 0xFF, .icon = NULL},
        {.text = "About", .sub_page_index = 0xFF, .icon = NULL},
        {.text = "Help", .sub_page_index = 0xFF, .icon = NULL},
};
static const egui_view_menu_page_t showcase_menu_pages[] = {
        {.title = "Menu", .items = showcase_menu_items, .item_count = 3},
};
static const egui_view_menu_item_t showcase_menu_items_cn[] = {
        {.text = "设置", .sub_page_index = 0xFF, .icon = NULL},
        {.text = "关于", .sub_page_index = 0xFF, .icon = NULL},
        {.text = "帮助", .sub_page_index = 0xFF, .icon = NULL},
};
static const egui_view_menu_page_t showcase_menu_pages_cn[] = {
        {.title = "菜单", .items = showcase_menu_items_cn, .item_count = 3},
};
static const egui_chart_point_t showcase_chart_line_points[] = {{0, 10}, {20, 45}, {40, 25}, {60, 55}, {80, 35}, {100, 60}};
static const egui_chart_series_t showcase_chart_line_series[] = {
        {.points = showcase_chart_line_points, .point_count = 6, .color = EGUI_COLOR_MAKE(88, 166, 255), .name = "S1"},
};
static const egui_chart_point_t showcase_chart_bar_points[] = {{0, 30}, {1, 55}, {2, 20}, {3, 45}, {4, 38}};
static const egui_chart_series_t showcase_chart_bar_series[] = {
        {.points = showcase_chart_bar_points, .point_count = 5, .color = EGUI_COLOR_MAKE(63, 185, 80), .name = "S1"},
};
static const egui_chart_point_t showcase_chart_scatter_points[] = {{10, 20}, {25, 48}, {40, 30}, {55, 60}, {70, 40}, {85, 52}};
static const egui_chart_series_t showcase_chart_scatter_series[] = {
        {.points = showcase_chart_scatter_points, .point_count = 6, .color = EGUI_COLOR_MAKE(255, 123, 114), .name = "S1"},
};
static const egui_chart_pie_slice_t showcase_chart_pie_slices[] = {
        {.value = 40, .color = EGUI_COLOR_MAKE(88, 166, 255), .name = "A"},
        {.value = 30, .color = EGUI_COLOR_MAKE(63, 185, 80), .name = "B"},
        {.value = 20, .color = EGUI_COLOR_MAKE(255, 123, 114), .name = "C"},
        {.value = 10, .color = EGUI_COLOR_MAKE(203, 166, 247), .name = "D"},
};
static const egui_view_line_point_t showcase_hq_line_points[] = {{0, 110}, {40, 10}, {80, 80}, {120, 20}, {160, 90}, {200, 40}};
static const egui_view_line_point_t showcase_simple_line_points[] = {{0, 3}, {100, 3}};

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_root_dark_new_p, EGUI_COLOR_MAKE(13, 17, 23), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_root_dark_new_params, &bg_root_dark_new_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_root_dark_new, &bg_root_dark_new_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_root_light_new_p, EGUI_COLOR_MAKE(246, 248, 250), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_root_light_new_params, &bg_root_light_new_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_root_light_new, &bg_root_light_new_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_panel_dark_p, EGUI_COLOR_MAKE(22, 27, 34), 210, 12, 1, EGUI_COLOR_MAKE(48, 54, 61), 200);
EGUI_BACKGROUND_PARAM_INIT(bg_panel_dark_params, &bg_panel_dark_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_panel_dark, &bg_panel_dark_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_panel_light_p, EGUI_COLOR_MAKE(255, 255, 255), 240, 12, 1, EGUI_COLOR_MAKE(208, 215, 222), 200);
EGUI_BACKGROUND_PARAM_INIT(bg_panel_light_params, &bg_panel_light_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_panel_light, &bg_panel_light_params);

/*
 * Animated render-only nodes clear only their own content area. Using the
 * panel's blended fill color avoids repainting the whole panel and wiping
 *
 * sibling static widgets on every frame.
 */
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_panel_fill_dark_p, EGUI_COLOR_MAKE(8, 20, 24), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_panel_fill_dark_params, &bg_panel_fill_dark_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_panel_fill_dark, &bg_panel_fill_dark_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_panel_fill_light_p, EGUI_COLOR_MAKE(240, 248, 248), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_panel_fill_light_params, &bg_panel_fill_light_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_panel_fill_light, &bg_panel_fill_light_params);

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(bg_grad_dark_p, EGUI_BACKGROUND_GRADIENT_DIR_HORIZONTAL, EGUI_COLOR_MAKE(13, 17, 23), EGUI_COLOR_MAKE(27, 42, 59),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_grad_dark_params, &bg_grad_dark_p, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(bg_grad_dark, &bg_grad_dark_params);

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(bg_grad_light_p, EGUI_BACKGROUND_GRADIENT_DIR_HORIZONTAL, EGUI_COLOR_MAKE(255, 255, 255), EGUI_COLOR_MAKE(220, 232, 245),
                                    EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_grad_light_params, &bg_grad_light_p, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(bg_grad_light, &bg_grad_light_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_shadow_dark_p, EGUI_COLOR_MAKE(31, 45, 61), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(bg_shadow_dark_params, &bg_shadow_dark_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_shadow_dark, &bg_shadow_dark_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_shadow_light_p, EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(bg_shadow_light_params, &bg_shadow_light_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_shadow_light, &bg_shadow_light_params);

EGUI_SHADOW_PARAM_INIT_ROUND(shadow_demo_dark, 10, 5, 5, EGUI_COLOR_MAKE(0, 0, 0), 160, 10);
EGUI_SHADOW_PARAM_INIT_ROUND(shadow_demo_light, 10, 4, 4, EGUI_COLOR_MAKE(128, 128, 128), 130, 8);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_ti_normal_p, EGUI_COLOR_MAKE(22, 27, 34), EGUI_ALPHA_100, 4, 1, EGUI_COLOR_MAKE(72, 80, 96),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_ti_focused_p, EGUI_COLOR_MAKE(22, 27, 34), EGUI_ALPHA_100, 4, 2, EGUI_COLOR_MAKE(88, 166, 255),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT_WITH_FOCUS(bg_ti_params, &bg_ti_normal_p, NULL, NULL, &bg_ti_focused_p);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_textinput_showcase, &bg_ti_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_ti_light_normal_p, EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100, 4, 1,
                                                        EGUI_COLOR_MAKE(168, 177, 186), EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_ti_light_focused_p, EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100, 4, 2,
                                                        EGUI_COLOR_MAKE(9, 105, 218), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT_WITH_FOCUS(bg_ti_light_params, &bg_ti_light_normal_p, NULL, NULL, &bg_ti_light_focused_p);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_textinput_showcase_light, &bg_ti_light_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_tb_p, EGUI_COLOR_MAKE(0, 0, 0), 0, 4, 1, EGUI_COLOR_MAKE(88, 96, 112), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_tb_params, &bg_tb_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_textblock_showcase, &bg_tb_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE(bg_border1_dark_p, EGUI_COLOR_MAKE(22, 27, 34), EGUI_ALPHA_100, 1, EGUI_COLOR_MAKE(88, 166, 255), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_border1_dark_params, &bg_border1_dark_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_border1_dark, &bg_border1_dark_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE(bg_border1_light_p, EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100, 1, EGUI_COLOR_MAKE(9, 105, 218),
                                              EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_border1_light_params, &bg_border1_light_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_border1_light, &bg_border1_light_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_border2_dark_p, EGUI_COLOR_MAKE(22, 27, 34), EGUI_ALPHA_100, 8, 2, EGUI_COLOR_MAKE(63, 185, 80),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_border2_dark_params, &bg_border2_dark_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_border2_dark, &bg_border2_dark_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_border2_light_p, EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100, 8, 2, EGUI_COLOR_MAKE(63, 185, 80),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_border2_light_params, &bg_border2_light_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_border2_light, &bg_border2_light_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha1_dark_active_p, EGUI_COLOR_MAKE(14, 50, 95), 245, 10, 3, EGUI_COLOR_MAKE(88, 166, 255),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_alpha1_dark_active_params, &bg_alpha1_dark_active_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha1_dark_active, &bg_alpha1_dark_active_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha1_dark_inactive_p, EGUI_COLOR_MAKE(13, 17, 23), 130, 10, 1, EGUI_COLOR_MAKE(33, 38, 45),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_alpha1_dark_inactive_params, &bg_alpha1_dark_inactive_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha1_dark_inactive, &bg_alpha1_dark_inactive_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha2_dark_active_p, EGUI_COLOR_MAKE(14, 50, 95), 245, 10, 3, EGUI_COLOR_MAKE(88, 166, 255),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_alpha2_dark_active_params, &bg_alpha2_dark_active_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha2_dark_active, &bg_alpha2_dark_active_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha2_dark_inactive_p, EGUI_COLOR_MAKE(13, 17, 23), 130, 10, 1, EGUI_COLOR_MAKE(33, 38, 45),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_alpha2_dark_inactive_params, &bg_alpha2_dark_inactive_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha2_dark_inactive, &bg_alpha2_dark_inactive_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha1_light_active_p, EGUI_COLOR_MAKE(178, 213, 255), 240, 10, 3, EGUI_COLOR_MAKE(9, 105, 218),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_alpha1_light_active_params, &bg_alpha1_light_active_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha1_light_active, &bg_alpha1_light_active_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha1_light_inactive_p, EGUI_COLOR_MAKE(246, 248, 250), 150, 10, 1, EGUI_COLOR_MAKE(208, 215, 222),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_alpha1_light_inactive_params, &bg_alpha1_light_inactive_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha1_light_inactive, &bg_alpha1_light_inactive_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha2_light_active_p, EGUI_COLOR_MAKE(178, 213, 255), 240, 10, 3, EGUI_COLOR_MAKE(9, 105, 218),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_alpha2_light_active_params, &bg_alpha2_light_active_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha2_light_active, &bg_alpha2_light_active_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_alpha2_light_inactive_p, EGUI_COLOR_MAKE(246, 248, 250), 150, 10, 1, EGUI_COLOR_MAKE(208, 215, 222),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_alpha2_light_inactive_params, &bg_alpha2_light_inactive_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_alpha2_light_inactive, &bg_alpha2_light_inactive_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_numpick_dark_p, EGUI_COLOR_MAKE(22, 27, 34), 210, 8, 1, EGUI_COLOR_MAKE(48, 54, 61), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_numpick_dark_params, &bg_numpick_dark_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_numpick_dark, &bg_numpick_dark_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_numpick_light_p, EGUI_COLOR_MAKE(255, 255, 255), 220, 8, 1, EGUI_COLOR_MAKE(208, 215, 222),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_numpick_light_params, &bg_numpick_light_p, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_numpick_light, &bg_numpick_light_params);

static void showcase_apply_stage_background(void);
static const egui_font_t *showcase_get_text_font(void);
static const char *showcase_text(const char *en, const char *cn);
static egui_color_t showcase_get_text_color(void);
static egui_color_t showcase_get_title_color(void);
static egui_color_t showcase_get_caption_color(void);
static const char **showcase_get_roller_items(void);
static const char **showcase_get_combobox_items(void);
static const char **showcase_get_tab_texts(void);
static const char **showcase_get_list_items(void);
static const egui_view_menu_page_t *showcase_get_menu_pages(void);
static const char *showcase_get_default_textinput_text(uint8_t is_chinese);
static const char *showcase_get_default_textinput_placeholder(uint8_t is_chinese);
static const char *showcase_get_theme_icon(void);
static void showcase_copy_text(char *dst, size_t dst_size, const char *src);
static void showcase_notify_nodes(const uint32_t *stable_ids, uint32_t count);
static void showcase_draw_view(egui_view_t *view);
static void showcase_reset_scratch(void);
static void showcase_prepare_panel_view(egui_view_t *view, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h);
static void showcase_prepare_panel_fill_view(egui_view_t *view, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h);
static void showcase_prepare_title_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t w, const char *text);
static void showcase_prepare_caption_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t w, const char *text, egui_color_t color);
static void showcase_bind_button_view(egui_view_t *view, uint32_t stable_id);
static void showcase_bind_textinput_view(egui_view_t *view);
static void showcase_bind_switch_view(egui_view_t *view);
static void showcase_bind_checkbox_view(egui_view_t *view);
static void showcase_bind_radio_view(egui_view_t *view, uint32_t stable_id);
static void showcase_bind_toggle_button_view(egui_view_t *view);
static void showcase_bind_slider_view(egui_view_t *view);
static void showcase_bind_arc_slider_view(egui_view_t *view);
static void showcase_bind_number_picker_view(egui_view_t *view);
static void showcase_bind_roller_view(egui_view_t *view);
static void showcase_bind_combobox_view(egui_view_t *view, const egui_virtual_stage_node_desc_t *desc);
static void showcase_bind_mini_calendar_view(egui_view_t *view);
static void showcase_bind_button_matrix_view(egui_view_t *view);
static void showcase_bind_tab_bar_view(egui_view_t *view);
static void showcase_bind_list_view(egui_view_t *view, const egui_virtual_stage_node_desc_t *desc);
static void showcase_bind_layer_card_view(egui_view_t *view, uint32_t stable_id);
static void showcase_refresh_theme_views(void);
static void showcase_refresh_language_views(void);
static void showcase_refresh_layer_views(void);
static void showcase_refresh_option_views(void);
static void showcase_pin_fidelity_nodes(void);
static uint8_t showcase_get_list_item_count(void);

static const uint32_t showcase_language_notify_ids[] = {
        SHOWCASE_NODE_BASIC_STATIC,  SHOWCASE_NODE_TOGGLE_STATIC, SHOWCASE_NODE_PROGRESS_STATIC, SHOWCASE_NODE_CANVAS_STATIC, SHOWCASE_NODE_SLIDER_STATIC,
        SHOWCASE_NODE_CHART_STATIC,  SHOWCASE_NODE_TIME_STATIC,   SHOWCASE_NODE_SPECIAL_STATIC,  SHOWCASE_NODE_DATA_STATIC,   SHOWCASE_NODE_ALPHA_LABELS,
        SHOWCASE_NODE_BUTTON_BASIC,  SHOWCASE_NODE_TEXTINPUT,     SHOWCASE_NODE_CHECKBOX,        SHOWCASE_NODE_OPTION1,       SHOWCASE_NODE_OPTION2,
        SHOWCASE_NODE_TOGGLE_BUTTON, SHOWCASE_NODE_ROLLER,        SHOWCASE_NODE_COMBOBOX,        SHOWCASE_NODE_TAB_BAR,       SHOWCASE_NODE_LIST,
        SHOWCASE_NODE_LANG_BUTTON,
};

static const uint32_t showcase_theme_notify_ids[] = {
        SHOWCASE_NODE_BASIC_STATIC,  SHOWCASE_NODE_TOGGLE_STATIC, SHOWCASE_NODE_PROGRESS_STATIC, SHOWCASE_NODE_CANVAS_STATIC, SHOWCASE_NODE_SLIDER_STATIC,
        SHOWCASE_NODE_CHART_STATIC,  SHOWCASE_NODE_TIME_STATIC,   SHOWCASE_NODE_SPECIAL_STATIC,  SHOWCASE_NODE_DATA_STATIC,   SHOWCASE_NODE_ALPHA_LABELS,
        SHOWCASE_NODE_BUTTON_BASIC,  SHOWCASE_NODE_TEXTINPUT,     SHOWCASE_NODE_CHECKBOX,        SHOWCASE_NODE_OPTION1,       SHOWCASE_NODE_OPTION2,
        SHOWCASE_NODE_TOGGLE_BUTTON, SHOWCASE_NODE_SLIDER,        SHOWCASE_NODE_ARC_SLIDER,      SHOWCASE_NODE_NUMBER_PICKER, SHOWCASE_NODE_ROLLER,
        SHOWCASE_NODE_COMBOBOX,      SHOWCASE_NODE_MINI_CALENDAR, SHOWCASE_NODE_BUTTON_MATRIX,   SHOWCASE_NODE_TAB_BAR,       SHOWCASE_NODE_LIST,
        SHOWCASE_NODE_ALPHA1,        SHOWCASE_NODE_ALPHA2,        SHOWCASE_NODE_LANG_BUTTON,     SHOWCASE_NODE_THEME_BUTTON,
};

static const uint32_t showcase_layer_notify_ids[] = {
        SHOWCASE_NODE_ALPHA_LABELS,
        SHOWCASE_NODE_ALPHA1,
        SHOWCASE_NODE_ALPHA2,
};

static const uint32_t showcase_option_notify_ids[] = {
        SHOWCASE_NODE_OPTION1,
        SHOWCASE_NODE_OPTION2,
};

static void showcase_apply_stage_background(void)
{
    EGUI_VIEW_VIRTUAL_STAGE_SET_BACKGROUND(&showcase_stage_view, showcase_ctx.is_dark_theme ? EGUI_BG_OF(&bg_root_dark_new) : EGUI_BG_OF(&bg_root_light_new));
}

static const egui_font_t *showcase_get_text_font(void)
{
    return showcase_ctx.is_chinese ? (const egui_font_t *)&egui_res_font_notosanssc_14_4 : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
}

static const egui_font_t *showcase_get_data_font(void)
{
#if EGUI_SHOWCASE_PARITY_RECORDING
    if (showcase_ctx.is_chinese)
    {
        return (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    }
#endif
    return showcase_get_text_font();
}

static const char *showcase_text(const char *en, const char *cn)
{
#if EGUI_SHOWCASE_PARITY_RECORDING
    if (showcase_ctx.is_chinese && showcase_force_english_text)
    {
        return en;
    }
#endif
    return showcase_ctx.is_chinese ? cn : en;
}

static egui_color_t showcase_get_text_color(void)
{
    return showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(201, 209, 217) : EGUI_COLOR_MAKE(36, 41, 47);
}

static egui_color_t showcase_get_title_color(void)
{
    return showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(88, 166, 255) : EGUI_COLOR_MAKE(9, 105, 218);
}

static egui_color_t showcase_get_caption_color(void)
{
    return showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(200, 215, 235) : EGUI_COLOR_MAKE(55, 70, 95);
}

static const char **showcase_get_roller_items(void)
{
    return (const char **)(showcase_ctx.is_chinese ? showcase_roller_items_cn : showcase_roller_items);
}

static const char **showcase_get_combobox_items(void)
{
    return (const char **)(showcase_ctx.is_chinese ? showcase_combo_items_cn : showcase_combo_items);
}

static const char **showcase_get_tab_texts(void)
{
    return (const char **)(showcase_ctx.is_chinese ? showcase_tab_texts_cn : showcase_tab_texts);
}

static const char **showcase_get_list_items(void)
{
#if EGUI_SHOWCASE_PARITY_RECORDING
    if (showcase_ctx.is_chinese)
    {
        return (const char **)showcase_list_items;
    }
#endif
    return (const char **)(showcase_ctx.is_chinese ? showcase_list_items_cn : showcase_list_items);
}

static uint8_t showcase_get_list_item_count(void)
{
#if EGUI_CONFIG_RECORDING_TEST
#if EGUI_SHOWCASE_PARITY_RECORDING
    return 4U;
#else
    return 8U;
#endif
#else
    return 4U;
#endif
}

static const egui_view_menu_page_t *showcase_get_menu_pages(void)
{
    return showcase_ctx.is_chinese ? showcase_menu_pages_cn : showcase_menu_pages;
}

static const char *showcase_get_default_textinput_text(uint8_t is_chinese)
{
    return is_chinese ? "你好" : "Hello";
}

static const char *showcase_get_default_textinput_placeholder(uint8_t is_chinese)
{
    return is_chinese ? "请输入..." : "Type here...";
}

static const char *showcase_get_theme_icon(void)
{
    return showcase_ctx.is_dark_theme ? "\xee\x94\x98" : "\xee\x94\x9c";
}

static void showcase_copy_text(char *dst, size_t dst_size, const char *src)
{
    if (dst_size == 0U)
    {
        return;
    }

    if (src == NULL)
    {
        dst[0] = '\0';
        return;
    }

    strncpy(dst, src, dst_size - 1U);
    dst[dst_size - 1U] = '\0';
}

static void showcase_notify_nodes(const uint32_t *stable_ids, uint32_t count)
{
    if (stable_ids == NULL)
    {
        return;
    }

    egui_view_virtual_stage_notify_nodes_changed(EGUI_VIEW_OF(&showcase_stage_view), stable_ids, count);
}

static void showcase_draw_view(egui_view_t *view)
{
    egui_region_t *dirty_regions = egui_core_get_region_dirty_arr();
    egui_location_t original_location = view->region.location;

    if (view->parent == NULL)
    {
        view->region.location.x = (egui_dim_t)(view->region.location.x + showcase_draw_origin_x);
        view->region.location.y = (egui_dim_t)(view->region.location.y + showcase_draw_origin_y);
        egui_view_request_layout(view);
    }
    view->api->calculate_layout(view);

    if (showcase_scratch_dirty_backup_valid)
    {
        memcpy(dirty_regions, showcase_scratch_dirty_backup, sizeof(showcase_scratch_dirty_backup));
        showcase_scratch_dirty_backup_valid = 0U;
    }

    view->api->draw(view);

    if (view->parent == NULL)
    {
        view->region.location = original_location;
    }
}

static void showcase_reset_scratch(void)
{
    memcpy(showcase_scratch_dirty_backup, egui_core_get_region_dirty_arr(), sizeof(showcase_scratch_dirty_backup));
    showcase_scratch_dirty_backup_valid = 1U;
    memset(&showcase_scratch, 0, sizeof(showcase_scratch));
}

static void showcase_prepare_panel_view(egui_view_t *view, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    egui_view_init(view);
    egui_view_set_position(view, x, y);
    egui_view_set_size(view, w, h);
    egui_view_set_background(view, showcase_ctx.is_dark_theme ? EGUI_BG_OF(&bg_panel_dark) : EGUI_BG_OF(&bg_panel_light));
}

static void showcase_prepare_panel_fill_view(egui_view_t *view, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    egui_view_init(view);
    egui_view_set_position(view, x, y);
    egui_view_set_size(view, w, h);
    egui_view_set_background(view, showcase_ctx.is_dark_theme ? EGUI_BG_OF(&bg_panel_fill_dark) : EGUI_BG_OF(&bg_panel_fill_light));
}

static void showcase_prepare_title_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t w, const char *text)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), w, 20);
    egui_view_label_set_font(EGUI_VIEW_OF(label), showcase_get_text_font());
    egui_view_label_set_text(EGUI_VIEW_OF(label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), showcase_get_title_color(), EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
}

static void showcase_prepare_caption_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t w, const char *text, egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), w, 20);
    egui_view_label_set_font(EGUI_VIEW_OF(label), showcase_get_text_font());
    egui_view_label_set_text(EGUI_VIEW_OF(label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
}

static void showcase_draw_panel(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    showcase_prepare_panel_view(&showcase_scratch.view, x, y, w, h);
    showcase_draw_view(&showcase_scratch.view);
}

static void showcase_clear_panel_region(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    showcase_prepare_panel_fill_view(&showcase_scratch.view, x, y, w, h);
    showcase_draw_view(&showcase_scratch.view);
}

static void showcase_draw_title(egui_dim_t x, egui_dim_t y, egui_dim_t w, const char *text)
{
    showcase_reset_scratch();
    showcase_prepare_title_label(&showcase_scratch.label, x, y, w, text);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.label));
}

static void showcase_draw_caption(egui_dim_t x, egui_dim_t y, egui_dim_t w, const char *text, egui_color_t color)
{
    showcase_reset_scratch();
    showcase_prepare_caption_label(&showcase_scratch.label, x, y, w, text, color);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.label));
}

static void showcase_bind_button_view(egui_view_t *view, uint32_t stable_id)
{
    egui_color_t text_color = showcase_get_text_color();

    if (stable_id == SHOWCASE_NODE_THEME_BUTTON)
    {
        egui_view_label_set_font(view, (const egui_font_t *)&egui_res_font_materialicon_20_4);
        egui_view_label_set_text(view, showcase_get_theme_icon());
    }
    else if (stable_id == SHOWCASE_NODE_LANG_BUTTON)
    {
        egui_view_label_set_font(view, (const egui_font_t *)&egui_res_font_notosanssc_14_4);
        egui_view_label_set_text(view, showcase_ctx.is_chinese ? "EN" : "中文");
    }
    else
    {
        egui_view_label_set_font(view, showcase_get_text_font());
        egui_view_label_set_text(view, showcase_text("Button", "按钮"));
    }

    egui_view_label_set_font_color(view, text_color, EGUI_ALPHA_100);
}

static void showcase_bind_textinput_view(egui_view_t *view)
{
    egui_view_textinput_set_font(view, showcase_get_text_font());
    egui_view_textinput_set_text(view, showcase_ctx.textinput_text);
    egui_view_textinput_set_placeholder(view, showcase_get_default_textinput_placeholder(showcase_ctx.is_chinese));
    egui_view_textinput_set_text_color(view, showcase_get_text_color(), EGUI_ALPHA_100);
    egui_view_set_padding(view, 6, 6, 4, 4);
    egui_view_set_background(view, showcase_ctx.is_dark_theme ? EGUI_BG_OF(&bg_textinput_showcase) : EGUI_BG_OF(&bg_textinput_showcase_light));
}

static void showcase_bind_switch_view(egui_view_t *view)
{
    egui_view_switch_set_checked(view, showcase_ctx.switch_checked);
}

static void showcase_bind_checkbox_view(egui_view_t *view)
{
    egui_view_checkbox_set_font(view, showcase_get_text_font());
    egui_view_checkbox_set_text(view, showcase_text("Checked", "已选中"));
    egui_view_checkbox_set_checked(view, showcase_ctx.checkbox_checked);
    egui_view_checkbox_set_text_color(view, showcase_get_text_color());
}

static void showcase_bind_radio_view(egui_view_t *view, uint32_t stable_id)
{
    uint8_t is_option1 = stable_id == SHOWCASE_NODE_OPTION1;

    egui_view_radio_button_set_font(view, showcase_get_text_font());
    egui_view_radio_button_set_text(view, is_option1 ? showcase_text("Option 1", "选项 1") : showcase_text("Option 2", "选项 2"));
    egui_view_radio_button_set_checked(view, showcase_ctx.option_index == (is_option1 ? 0U : 1U));
    egui_view_radio_button_set_text_color(view, showcase_get_text_color());
}

static void showcase_bind_toggle_button_view(egui_view_t *view)
{
    egui_view_toggle_button_set_font(view, showcase_get_text_font());
    egui_view_toggle_button_set_text(view, showcase_text("ON", "开"));
    egui_view_toggle_button_set_toggled(view, showcase_ctx.toggle_checked);
    egui_view_toggle_button_set_text_color(view, showcase_get_text_color());
}

static void showcase_bind_slider_view(egui_view_t *view)
{
    egui_view_slider_t *slider = (egui_view_slider_t *)view;

    egui_view_slider_set_value(view, showcase_ctx.slider_value);
    slider->track_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(33, 38, 45) : EGUI_COLOR_MAKE(208, 215, 222);
    slider->active_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(88, 166, 255) : EGUI_COLOR_MAKE(9, 105, 218);
    slider->thumb_color = EGUI_COLOR_WHITE;
}

static void showcase_bind_arc_slider_view(egui_view_t *view)
{
    egui_view_arc_slider_t *arc_slider = (egui_view_arc_slider_t *)view;

    egui_view_arc_slider_set_value(view, showcase_ctx.arc_slider_value);
    arc_slider->stroke_width = 12;
    arc_slider->thumb_radius = 9;
    arc_slider->track_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(33, 38, 45) : EGUI_COLOR_MAKE(208, 215, 222);
    arc_slider->active_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(88, 166, 255) : EGUI_COLOR_MAKE(9, 105, 218);
    arc_slider->thumb_color = EGUI_COLOR_WHITE;
}

static void showcase_bind_number_picker_view(egui_view_t *view)
{
    egui_view_number_picker_t *picker = (egui_view_number_picker_t *)view;

    egui_view_number_picker_set_range(view, 0, 99);
    egui_view_number_picker_set_value(view, showcase_ctx.numpick_value);
    picker->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    picker->text_color = showcase_get_text_color();
    picker->button_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(88, 166, 255) : EGUI_COLOR_MAKE(9, 105, 218);
    egui_view_set_background(view, showcase_ctx.is_dark_theme ? EGUI_BG_OF(&bg_numpick_dark) : EGUI_BG_OF(&bg_numpick_light));
}

static void showcase_bind_roller_view(egui_view_t *view)
{
    egui_view_roller_t *roller = (egui_view_roller_t *)view;

    egui_view_roller_set_items(view, showcase_get_roller_items(), 5);
    egui_view_roller_set_current_index(view, showcase_ctx.roller_index);
    roller->font = showcase_get_text_font();
    roller->text_color = showcase_get_text_color();
}

static void showcase_bind_combobox_view(egui_view_t *view, const egui_virtual_stage_node_desc_t *desc)
{
    egui_view_combobox_t *combobox = (egui_view_combobox_t *)view;

    EGUI_UNUSED(desc);
    combobox->collapsed_height = 30;
    egui_view_combobox_set_items(view, showcase_get_combobox_items(), 3);
    egui_view_combobox_set_current_index(view, showcase_ctx.combobox_index);
    egui_view_combobox_set_max_visible_items(view, 3);
    egui_view_combobox_set_font(view, showcase_get_text_font());
    combobox->text_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(201, 209, 217) : showcase_get_text_color();
    combobox->bg_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(22, 27, 34) : EGUI_THEME_SURFACE;
}

static void showcase_bind_mini_calendar_view(egui_view_t *view)
{
    egui_view_mini_calendar_t *calendar = (egui_view_mini_calendar_t *)view;

    egui_view_mini_calendar_set_date(view, 2026, 3, showcase_ctx.calendar_day);
    egui_view_mini_calendar_set_today(view, 0);
    egui_view_mini_calendar_set_weekday_labels(view, showcase_ctx.is_chinese ? showcase_calendar_weekdays_cn : NULL);
    calendar->font = showcase_get_text_font();
    calendar->text_color = showcase_get_text_color();
    calendar->header_color = showcase_get_title_color();
    calendar->weekend_color = showcase_ctx.is_dark_theme ? showcase_get_text_color() : showcase_get_title_color();
    calendar->bg_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(22, 27, 34) : EGUI_THEME_SURFACE;
}

static void showcase_bind_button_matrix_view(egui_view_t *view)
{
    egui_view_button_matrix_set_labels(view, showcase_matrix_labels, 6, 3);
    egui_view_button_matrix_set_font(view, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_button_matrix_set_text_color(view, showcase_get_text_color());
}

static void showcase_bind_tab_bar_view(egui_view_t *view)
{
    egui_view_tab_bar_t *tab_bar = (egui_view_tab_bar_t *)view;

    egui_view_tab_bar_set_tabs(view, showcase_get_tab_texts(), 3);
    egui_view_tab_bar_set_current_index(view, showcase_ctx.tab_index);
    egui_view_tab_bar_set_font(view, showcase_get_text_font());
    tab_bar->text_color = showcase_get_text_color();
    tab_bar->active_text_color = showcase_get_title_color();
}

static void showcase_bind_list_view(egui_view_t *view, const egui_virtual_stage_node_desc_t *desc)
{
    egui_view_list_t *list = (egui_view_list_t *)view;
    const char **items = showcase_get_list_items();
    const egui_font_t *font = showcase_get_data_font();
    egui_color_t text_color = showcase_get_text_color();
    uint8_t item_count = showcase_get_list_item_count();
    uint8_t i;

    egui_view_scroll_set_size(view, desc->region.size.width, desc->region.size.height);

    if (list->item_count == 0U)
    {
        for (i = 0; i < item_count; i++)
        {
            egui_view_list_add_item(view, items[i]);
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
        egui_view_scroll_set_scrollbar_enabled(view, 1);
#endif
    }

    for (i = 0; i < list->item_count && i < item_count; i++)
    {
        list->item_texts[i] = items[i];
        egui_view_label_set_font(EGUI_VIEW_OF(&list->items[i]), font);
        egui_view_label_set_font_color((egui_view_t *)&list->items[i], text_color, EGUI_ALPHA_100);
    }

    egui_view_scroll_layout_childs(view);
}

static void showcase_bind_layer_card_view(egui_view_t *view, uint32_t stable_id)
{
    uint8_t is_alpha1 = stable_id == SHOWCASE_NODE_ALPHA1;
    uint8_t is_active = showcase_ctx.active_layer == (is_alpha1 ? 1U : 2U);

    if (showcase_ctx.is_dark_theme)
    {
        egui_view_set_background(view, is_alpha1 ? EGUI_BG_OF(is_active ? &bg_alpha1_dark_active : &bg_alpha1_dark_inactive)
                                                 : EGUI_BG_OF(is_active ? &bg_alpha2_dark_active : &bg_alpha2_dark_inactive));
    }
    else
    {
        egui_view_set_background(view, is_alpha1 ? EGUI_BG_OF(is_active ? &bg_alpha1_light_active : &bg_alpha1_light_inactive)
                                                 : EGUI_BG_OF(is_active ? &bg_alpha2_light_active : &bg_alpha2_light_inactive));
    }
}

static void showcase_prepare_button_preview(uint32_t stable_id, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    egui_view_button_init(EGUI_VIEW_OF(&showcase_scratch.button));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.button), x, y);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.button), w, h);
    showcase_bind_button_view(EGUI_VIEW_OF(&showcase_scratch.button), stable_id);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.button));
}

static void showcase_prepare_textinput_preview(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    egui_view_textinput_init(EGUI_VIEW_OF(&showcase_scratch.textinput));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.textinput), x, y);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.textinput), w, h);
    showcase_bind_textinput_view(EGUI_VIEW_OF(&showcase_scratch.textinput));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.textinput));
}

static void showcase_prepare_switch_preview(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    egui_view_switch_init(EGUI_VIEW_OF(&showcase_scratch.switch_view));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.switch_view), x, y);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.switch_view), w, h);
    showcase_bind_switch_view(EGUI_VIEW_OF(&showcase_scratch.switch_view));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.switch_view));
}

static void showcase_prepare_checkbox_preview(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    egui_view_checkbox_init(EGUI_VIEW_OF(&showcase_scratch.checkbox));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.checkbox), x, y);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.checkbox), w, h);
    showcase_bind_checkbox_view(EGUI_VIEW_OF(&showcase_scratch.checkbox));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.checkbox));
}

static void showcase_prepare_radio_preview(uint32_t stable_id, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    egui_view_radio_button_init(EGUI_VIEW_OF(&showcase_scratch.radio));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.radio), x, y);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.radio), w, h);
    showcase_bind_radio_view(EGUI_VIEW_OF(&showcase_scratch.radio), stable_id);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.radio));
}

static void showcase_prepare_toggle_button_preview(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    egui_view_toggle_button_init(EGUI_VIEW_OF(&showcase_scratch.toggle_button));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.toggle_button), x, y);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.toggle_button), w, h);
    showcase_bind_toggle_button_view(EGUI_VIEW_OF(&showcase_scratch.toggle_button));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.toggle_button));
}

static void showcase_prepare_slider_preview(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    egui_view_slider_init(EGUI_VIEW_OF(&showcase_scratch.slider));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.slider), x, y);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.slider), w, h);
    showcase_bind_slider_view(EGUI_VIEW_OF(&showcase_scratch.slider));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.slider));
}

static void showcase_prepare_arc_slider_preview(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    egui_view_arc_slider_init(EGUI_VIEW_OF(&showcase_scratch.arc_slider));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.arc_slider), x, y);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.arc_slider), w, h);
    showcase_bind_arc_slider_view(EGUI_VIEW_OF(&showcase_scratch.arc_slider));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.arc_slider));
}

static void showcase_prepare_number_picker_preview(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    egui_view_number_picker_init(EGUI_VIEW_OF(&showcase_scratch.number_picker));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.number_picker), x, y);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.number_picker), w, h);
    showcase_bind_number_picker_view(EGUI_VIEW_OF(&showcase_scratch.number_picker));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.number_picker));
}

static void showcase_prepare_roller_preview(void)
{
    showcase_reset_scratch();
    {
        EGUI_VIEW_ROLLER_PARAMS_INIT(params, 276, 362, 90, 110, showcase_roller_items, 5, 1);
        egui_view_roller_init_with_params(EGUI_VIEW_OF(&showcase_scratch.roller), &params);
    }
    showcase_bind_roller_view(EGUI_VIEW_OF(&showcase_scratch.roller));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.roller));
}

static void showcase_prepare_combobox_preview(void)
{
    showcase_reset_scratch();
    {
        EGUI_VIEW_COMBOBOX_PARAMS_INIT(params, 16, 520, 150, 30, showcase_combo_items, 3, 0);
        egui_view_combobox_init_with_params(EGUI_VIEW_OF(&showcase_scratch.combobox), &params);
    }
    showcase_bind_combobox_view(EGUI_VIEW_OF(&showcase_scratch.combobox), &showcase_ctx.nodes[SHOWCASE_NODE_INDEX_COMBOBOX].desc);
    egui_view_combobox_collapse(EGUI_VIEW_OF(&showcase_scratch.combobox));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.combobox));
}

static void showcase_prepare_mini_calendar_preview(void)
{
    showcase_reset_scratch();
    {
        egui_view_mini_calendar_params_t params = {.region = {{160, 720}, {180, 150}}, .year = 2026, .month = 3, .day = showcase_ctx.calendar_day};
        egui_view_mini_calendar_init_with_params(EGUI_VIEW_OF(&showcase_scratch.mini_calendar), &params);
    }
    showcase_bind_mini_calendar_view(EGUI_VIEW_OF(&showcase_scratch.mini_calendar));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.mini_calendar));
}

static void showcase_prepare_button_matrix_preview(void)
{
    showcase_reset_scratch();
    {
        EGUI_VIEW_BUTTON_MATRIX_PARAMS_INIT(params, 860, 640, 170, 84, 3, 2);
        egui_view_button_matrix_init_with_params(EGUI_VIEW_OF(&showcase_scratch.button_matrix), &params);
    }
    showcase_bind_button_matrix_view(EGUI_VIEW_OF(&showcase_scratch.button_matrix));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.button_matrix));
}

static void showcase_prepare_tab_bar_preview(void)
{
    showcase_reset_scratch();
    {
        EGUI_VIEW_TAB_BAR_PARAMS_INIT(params, 640, 748, 160, 34, showcase_tab_texts, 3);
        egui_view_tab_bar_init_with_params(EGUI_VIEW_OF(&showcase_scratch.tab_bar), &params);
    }
    showcase_bind_tab_bar_view(EGUI_VIEW_OF(&showcase_scratch.tab_bar));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.tab_bar));
}

static void showcase_prepare_list_preview(void)
{
    showcase_reset_scratch();
    {
        EGUI_VIEW_LIST_PARAMS_INIT(params, 640, 794, 140, 110, 30);
        egui_view_list_init_with_params(EGUI_VIEW_OF(&showcase_scratch.list), &params);
    }
    showcase_bind_list_view(EGUI_VIEW_OF(&showcase_scratch.list), &showcase_ctx.nodes[SHOWCASE_NODE_INDEX_LIST].desc);
    egui_view_scroll_to(EGUI_VIEW_OF(&showcase_scratch.list.base.container), 0, showcase_ctx.list_scroll_y);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.list));
}

static void showcase_prepare_layer_card_preview(uint32_t stable_id, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    showcase_reset_scratch();
    egui_view_init(&showcase_scratch.view);
    egui_view_set_position(&showcase_scratch.view, x, y);
    egui_view_set_size(&showcase_scratch.view, w, h);
    showcase_bind_layer_card_view(&showcase_scratch.view, stable_id);
    showcase_draw_view(&showcase_scratch.view);
}

static void showcase_draw_basic_static(void)
{
    egui_color_t text_color = showcase_get_text_color();
    egui_color_t caption_color = showcase_get_caption_color();

    showcase_draw_panel(10, 4, 214, 296);
    showcase_draw_title(16, 8, 180, showcase_text("Basic", "基础"));

    showcase_reset_scratch();
    egui_view_label_init(EGUI_VIEW_OF(&showcase_scratch.label));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.label), 16, 74);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.label), 110, 20);
    egui_view_label_set_font(EGUI_VIEW_OF(&showcase_scratch.label), showcase_get_text_font());
    egui_view_label_set_text(EGUI_VIEW_OF(&showcase_scratch.label), showcase_text("Label Text", "标签文本"));
    egui_view_label_set_font_color(EGUI_VIEW_OF(&showcase_scratch.label), text_color, EGUI_ALPHA_100);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.label));

    showcase_reset_scratch();
    egui_view_textblock_init(EGUI_VIEW_OF(&showcase_scratch.textblock));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.textblock), 16, 104);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.textblock), 110, 48);
    egui_view_set_padding(EGUI_VIEW_OF(&showcase_scratch.textblock), 6, 6, 4, 4);
    egui_view_textblock_set_font(EGUI_VIEW_OF(&showcase_scratch.textblock), showcase_get_text_font());
    egui_view_textblock_set_text(EGUI_VIEW_OF(&showcase_scratch.textblock), showcase_text("Multi-line\ntext block", "多行\n文本块"));
    egui_view_textblock_set_font_color(EGUI_VIEW_OF(&showcase_scratch.textblock), text_color, EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&showcase_scratch.textblock), EGUI_BG_OF(&bg_textblock_showcase));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.textblock));

    showcase_reset_scratch();
    egui_view_dynamic_label_init(EGUI_VIEW_OF(&showcase_scratch.dynamic_label));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.dynamic_label), 16, 160);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.dynamic_label), 110, 20);
    egui_view_label_set_font(EGUI_VIEW_OF(&showcase_scratch.dynamic_label), showcase_get_text_font());
    egui_view_dynamic_label_set_text(EGUI_VIEW_OF(&showcase_scratch.dynamic_label), showcase_text("Dynamic 42", "动态 42"));
    egui_view_label_set_font_color(EGUI_VIEW_OF(&showcase_scratch.dynamic_label), text_color, EGUI_ALPHA_100);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.dynamic_label));

    showcase_reset_scratch();
    egui_view_card_init(EGUI_VIEW_OF(&showcase_scratch.card.card));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.card.card), 16, 192);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.card.card), 130, 52);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&showcase_scratch.card.card),
                                showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(30, 40, 55) : EGUI_COLOR_MAKE(255, 255, 255), EGUI_ALPHA_100);
    egui_view_label_init(EGUI_VIEW_OF(&showcase_scratch.card.child));
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.card.child), 110, 30);
    egui_view_set_padding(EGUI_VIEW_OF(&showcase_scratch.card.child), 6, 6, 4, 4);
    egui_view_label_set_font(EGUI_VIEW_OF(&showcase_scratch.card.child), showcase_get_text_font());
    egui_view_label_set_text(EGUI_VIEW_OF(&showcase_scratch.card.child), showcase_text("Card Widget", "卡片控件"));
    egui_view_label_set_font_color(EGUI_VIEW_OF(&showcase_scratch.card.child), text_color, EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&showcase_scratch.card.card), EGUI_VIEW_OF(&showcase_scratch.card.child));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.card.card));

    showcase_draw_caption(130, 74, 74, showcase_text("Label", "标签"), caption_color);
    showcase_draw_caption(130, 120, 74, showcase_text("Textblock", "文本块"), caption_color);
    showcase_draw_caption(130, 160, 74, showcase_text("DynLabel", "动态标"), caption_color);
    showcase_draw_caption(130, 257, 74, showcase_text("TextInput", "输入框"), caption_color);
}

static void showcase_draw_toggle_static(void)
{
    egui_color_t caption_color = showcase_get_caption_color();

    showcase_draw_panel(234, 4, 178, 260);
    showcase_draw_title(240, 8, 180, showcase_text("Toggle", "切换"));
    showcase_draw_caption(300, 36, 70, showcase_text("Switch", "开关"), caption_color);

    showcase_reset_scratch();
    egui_view_led_init(EGUI_VIEW_OF(&showcase_scratch.led));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.led), 240, 206);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.led), 26, 26);
    egui_view_led_set_on(EGUI_VIEW_OF(&showcase_scratch.led));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.led));

    showcase_reset_scratch();
    egui_view_notification_badge_init(EGUI_VIEW_OF(&showcase_scratch.badge));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.badge), 298, 210);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.badge), 26, 22);
    egui_view_notification_badge_set_count(EGUI_VIEW_OF(&showcase_scratch.badge), 5);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.badge));

    showcase_draw_caption(240, 234, 36, "LED", caption_color);
    showcase_draw_caption(294, 234, 50, showcase_text("Badge", "徽章"), caption_color);
}

static void showcase_draw_progress_static(void)
{
    egui_color_t caption_color = showcase_get_caption_color();

    showcase_draw_panel(424, 4, 244, 306);
    showcase_draw_title(430, 8, 230, showcase_text("Progress", "进度"));

    showcase_reset_scratch();
    egui_view_gauge_init(EGUI_VIEW_OF(&showcase_scratch.gauge));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.gauge), 540, 50);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.gauge), 120, 120);
    showcase_scratch.gauge.stroke_width = 12;
    showcase_scratch.gauge.bk_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(33, 38, 45) : EGUI_COLOR_MAKE(208, 215, 222);
    showcase_scratch.gauge.progress_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(88, 166, 255) : EGUI_COLOR_MAKE(9, 105, 218);
    showcase_scratch.gauge.needle_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(255, 123, 114) : EGUI_COLOR_MAKE(207, 34, 46);
    showcase_scratch.gauge.text_color = showcase_get_text_color();
    egui_view_gauge_set_value(EGUI_VIEW_OF(&showcase_scratch.gauge), 65);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.gauge));
    showcase_draw_caption(430, 148, 120, showcase_text("CircProgressBar", "圆进度条"), caption_color);
    showcase_draw_caption(570, 150, 60, showcase_text("Gauge", "仪表盘"), caption_color);

    showcase_reset_scratch();
    egui_view_page_indicator_init(EGUI_VIEW_OF(&showcase_scratch.page_indicator));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.page_indicator), 530, 240);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.page_indicator), 130, 18);
    egui_view_page_indicator_set_total_count(EGUI_VIEW_OF(&showcase_scratch.page_indicator), 5);
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&showcase_scratch.page_indicator), 2);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.page_indicator));
    showcase_draw_caption(430, 270, 104, showcase_text("ActivityRing", "活动环"), caption_color);
    showcase_draw_caption(546, 270, 116, showcase_text("PageIndicator", "页码指示"), caption_color);
}

static void showcase_draw_progress_bar_anim(void)
{
    uint16_t tick = showcase_ctx.anim_tick;
    uint8_t progress_value = tick == 0U ? 30U : (uint8_t)((tick * 2U) % 100U);

    showcase_clear_panel_region(430, 30, 210, 16);

    showcase_reset_scratch();
    egui_view_progress_bar_init(EGUI_VIEW_OF(&showcase_scratch.progress_bar));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.progress_bar), 430, 30);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.progress_bar), 210, 16);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&showcase_scratch.progress_bar), progress_value);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.progress_bar));
}

static void showcase_draw_circular_progress_anim(void)
{
    uint16_t tick = showcase_ctx.anim_tick;
    uint8_t circular_value = tick == 0U ? 50U : (uint8_t)((tick * 3U) % 100U);

    showcase_clear_panel_region(430, 56, 90, 90);

    showcase_reset_scratch();
    egui_view_circular_progress_bar_init(EGUI_VIEW_OF(&showcase_scratch.circular_progress_bar));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.circular_progress_bar), 430, 56);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.circular_progress_bar), 90, 90);
    egui_view_circular_progress_bar_set_stroke_width(EGUI_VIEW_OF(&showcase_scratch.circular_progress_bar), 8);
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&showcase_scratch.circular_progress_bar), circular_value);
    egui_view_circular_progress_bar_set_bk_color(EGUI_VIEW_OF(&showcase_scratch.circular_progress_bar),
                                                 showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(33, 38, 45) : EGUI_COLOR_MAKE(208, 215, 222));
    showcase_scratch.circular_progress_bar.progress_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(88, 166, 255) : EGUI_COLOR_MAKE(9, 105, 218);
    showcase_scratch.circular_progress_bar.text_color = showcase_get_text_color();
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.circular_progress_bar));
}

static void showcase_draw_activity_ring_anim(void)
{
    uint16_t tick = showcase_ctx.anim_tick;
    uint8_t ring0_value = tick == 0U ? 70U : (uint8_t)((tick * 2U) % 100U);
    uint8_t ring1_value = tick == 0U ? 50U : (uint8_t)((tick * 3U + 30U) % 100U);
    uint8_t ring2_value = tick == 0U ? 30U : (uint8_t)((tick * 4U + 60U) % 100U);

    showcase_clear_panel_region(430, 162, 110, 110);

    showcase_reset_scratch();
    egui_view_activity_ring_init(EGUI_VIEW_OF(&showcase_scratch.activity_ring));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 430, 162);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 110, 110);
    egui_view_activity_ring_set_ring_count(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 3);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 0, ring0_value);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 1, ring1_value);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 2, ring2_value);
    egui_view_activity_ring_set_stroke_width(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 12);
    egui_view_activity_ring_set_ring_gap(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 3);
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 0, EGUI_COLOR_MAKE(0xEF, 0x44, 0x44));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 1, EGUI_COLOR_MAKE(0x10, 0xB9, 0x81));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 2, EGUI_COLOR_MAKE(0x38, 0xBD, 0xF8));
    if (showcase_ctx.is_dark_theme)
    {
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 0, EGUI_COLOR_MAKE(0x3B, 0x15, 0x15));
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 1, EGUI_COLOR_MAKE(0x0A, 0x2E, 0x20));
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 2, EGUI_COLOR_MAKE(0x0E, 0x2F, 0x3E));
    }
    else
    {
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 0, EGUI_COLOR_MAKE(0xFD, 0xD5, 0xD5));
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 1, EGUI_COLOR_MAKE(0xC5, 0xF2, 0xDF));
        egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&showcase_scratch.activity_ring), 2, EGUI_COLOR_MAKE(0xC8, 0xE9, 0xF9));
    }
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.activity_ring));
}

static void showcase_draw_canvas_static(void)
{
    egui_color_t caption_color = showcase_get_caption_color();

    showcase_draw_panel(674, 4, 590, 322);
    showcase_draw_title(680, 8, 580, showcase_text("Canvas", "画布"));

    showcase_reset_scratch();
    egui_view_init(&showcase_scratch.view);
    egui_view_set_position(&showcase_scratch.view, 680, 30);
    egui_view_set_size(&showcase_scratch.view, 240, 60);
    egui_view_set_background(&showcase_scratch.view, showcase_ctx.is_dark_theme ? EGUI_BG_OF(&bg_grad_dark) : EGUI_BG_OF(&bg_grad_light));
    showcase_draw_view(&showcase_scratch.view);
    showcase_draw_caption(684, 33, 80, showcase_text("Gradient", "渐变"), caption_color);

    showcase_reset_scratch();
    egui_view_init(&showcase_scratch.view);
    egui_view_set_position(&showcase_scratch.view, 940, 30);
    egui_view_set_size(&showcase_scratch.view, 120, 80);
    egui_view_set_background(&showcase_scratch.view, showcase_ctx.is_dark_theme ? EGUI_BG_OF(&bg_shadow_dark) : EGUI_BG_OF(&bg_shadow_light));
    egui_view_set_shadow(&showcase_scratch.view, showcase_ctx.is_dark_theme ? &shadow_demo_dark : &shadow_demo_light);
    showcase_draw_view(&showcase_scratch.view);
    showcase_draw_caption(944, 33, 70, showcase_text("Shadow", "阴影"), caption_color);

    showcase_reset_scratch();
    egui_view_init(&showcase_scratch.view);
    egui_view_set_position(&showcase_scratch.view, 680, 102);
    egui_view_set_size(&showcase_scratch.view, 110, 60);
    egui_view_set_background(&showcase_scratch.view, showcase_ctx.is_dark_theme ? EGUI_BG_OF(&bg_border1_dark) : EGUI_BG_OF(&bg_border1_light));
    showcase_draw_view(&showcase_scratch.view);
    showcase_draw_caption(685, 105, 84, showcase_text("Stroke 1px", "边框1"), caption_color);

    showcase_reset_scratch();
    egui_view_init(&showcase_scratch.view);
    egui_view_set_position(&showcase_scratch.view, 806, 102);
    egui_view_set_size(&showcase_scratch.view, 110, 60);
    egui_view_set_background(&showcase_scratch.view, showcase_ctx.is_dark_theme ? EGUI_BG_OF(&bg_border2_dark) : EGUI_BG_OF(&bg_border2_light));
    showcase_draw_view(&showcase_scratch.view);
    showcase_draw_caption(811, 105, 84, showcase_text("Round 2px", "边框2"), caption_color);

    showcase_reset_scratch();
    egui_view_line_init(EGUI_VIEW_OF(&showcase_scratch.line));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.line), 940, 140);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.line), 240, 140);
    egui_view_line_set_points(EGUI_VIEW_OF(&showcase_scratch.line), showcase_hq_line_points, 6);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&showcase_scratch.line), 2);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&showcase_scratch.line),
                                  showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(63, 185, 80) : EGUI_COLOR_MAKE(9, 105, 218));
    egui_view_line_set_use_round_cap(EGUI_VIEW_OF(&showcase_scratch.line), 1);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.line));
    showcase_draw_caption(942, 127, 70, showcase_text("HQ Line", "HQ线"), caption_color);
}

static void showcase_draw_alpha_labels(void)
{
    egui_color_t active_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_WHITE : EGUI_COLOR_MAKE(14, 50, 95);
    egui_color_t inactive_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(160, 175, 195) : EGUI_COLOR_MAKE(65, 80, 100);

    showcase_draw_caption(686, 177, 64, showcase_text("Layer 1", "图层1"), showcase_ctx.active_layer == 1U ? active_color : inactive_color);
    showcase_draw_caption(766, 213, 64, showcase_text("Layer 2", "图层2"), showcase_ctx.active_layer == 2U ? active_color : inactive_color);
}

static void showcase_draw_slider_static(void)
{
    egui_color_t caption_color = showcase_get_caption_color();
    egui_color_t text_color = showcase_get_text_color();

    showcase_draw_panel(10, 336, 388, 240);
    showcase_draw_title(16, 340, 380, showcase_text("Slider/Picker", "滑块/选择"));

    showcase_draw_caption(35, 490, 80, showcase_text("ArcSlider", "弧滑块"), caption_color);

    showcase_draw_caption(216, 452, 60, showcase_text("Spinner", "旋转框"), caption_color);

    showcase_reset_scratch();
    {
        EGUI_VIEW_SCALE_PARAMS_INIT(params, 186, 520, 200, 48, 0, 100, 5);
        egui_view_scale_init_with_params(EGUI_VIEW_OF(&showcase_scratch.scale), &params);
    }
    egui_view_scale_set_value(EGUI_VIEW_OF(&showcase_scratch.scale), 60);
    showcase_scratch.scale.label_color = text_color;
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.scale));
}

static void showcase_draw_spinner_anim(void)
{
    uint16_t tick = showcase_ctx.anim_tick;

    showcase_clear_panel_region(216, 410, 40, 40);

    showcase_reset_scratch();
    egui_view_spinner_init(EGUI_VIEW_OF(&showcase_scratch.spinner));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.spinner), 216, 410);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.spinner), 40, 40);
    showcase_scratch.spinner.rotation_angle = (int16_t)((tick * 18U) % 360U);
    showcase_scratch.spinner.is_spinning = 1U;
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.spinner));
}

static void showcase_draw_chart_static(void)
{
    showcase_draw_panel(424, 336, 840, 280);
    showcase_draw_title(430, 340, 838, showcase_text("Chart", "图表"));

    showcase_reset_scratch();
    {
        EGUI_VIEW_CHART_LINE_PARAMS_INIT(params, 430, 362, 400, 120);
        egui_view_chart_line_init_with_params(EGUI_VIEW_OF(&showcase_scratch.chart_line), &params);
    }
    egui_view_chart_line_set_axis_x(EGUI_VIEW_OF(&showcase_scratch.chart_line), 0, 100, 20);
    egui_view_chart_line_set_axis_y(EGUI_VIEW_OF(&showcase_scratch.chart_line), 0, 80, 20);
    egui_view_chart_line_set_series(EGUI_VIEW_OF(&showcase_scratch.chart_line), showcase_chart_line_series, 1);
    if (showcase_ctx.is_dark_theme)
    {
        egui_view_chart_line_set_colors(EGUI_VIEW_OF(&showcase_scratch.chart_line), EGUI_COLOR_MAKE(18, 22, 30), EGUI_COLOR_MAKE(72, 88, 110),
                                        EGUI_COLOR_MAKE(36, 44, 58), EGUI_COLOR_MAKE(145, 160, 180));
    }
    else
    {
        egui_view_chart_line_set_colors(EGUI_VIEW_OF(&showcase_scratch.chart_line), EGUI_COLOR_MAKE(250, 251, 253), EGUI_COLOR_MAKE(60, 75, 95),
                                        EGUI_COLOR_MAKE(215, 222, 232), EGUI_COLOR_MAKE(45, 55, 70));
    }
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.chart_line));

    showcase_reset_scratch();
    {
        EGUI_VIEW_CHART_BAR_PARAMS_INIT(params, 850, 362, 400, 120);
        egui_view_chart_bar_init_with_params(EGUI_VIEW_OF(&showcase_scratch.chart_bar), &params);
    }
    egui_view_chart_bar_set_axis_x(EGUI_VIEW_OF(&showcase_scratch.chart_bar), 0, 4, 1);
    egui_view_chart_bar_set_axis_y(EGUI_VIEW_OF(&showcase_scratch.chart_bar), 0, 60, 20);
    egui_view_chart_bar_set_series(EGUI_VIEW_OF(&showcase_scratch.chart_bar), showcase_chart_bar_series, 1);
    if (showcase_ctx.is_dark_theme)
    {
        egui_view_chart_bar_set_colors(EGUI_VIEW_OF(&showcase_scratch.chart_bar), EGUI_COLOR_MAKE(18, 22, 30), EGUI_COLOR_MAKE(72, 88, 110),
                                       EGUI_COLOR_MAKE(36, 44, 58), EGUI_COLOR_MAKE(145, 160, 180));
    }
    else
    {
        egui_view_chart_bar_set_colors(EGUI_VIEW_OF(&showcase_scratch.chart_bar), EGUI_COLOR_MAKE(250, 251, 253), EGUI_COLOR_MAKE(60, 75, 95),
                                       EGUI_COLOR_MAKE(215, 222, 232), EGUI_COLOR_MAKE(45, 55, 70));
    }
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.chart_bar));

    showcase_reset_scratch();
    {
        EGUI_VIEW_CHART_PIE_PARAMS_INIT(params, 430, 492, 260, 120);
        egui_view_chart_pie_init_with_params(EGUI_VIEW_OF(&showcase_scratch.chart_pie), &params);
    }
    egui_view_chart_pie_set_slices(EGUI_VIEW_OF(&showcase_scratch.chart_pie), showcase_chart_pie_slices, 4);
    egui_view_chart_pie_set_colors(EGUI_VIEW_OF(&showcase_scratch.chart_pie),
                                   showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(18, 22, 30) : EGUI_COLOR_MAKE(250, 251, 253),
                                   showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(145, 160, 180) : EGUI_COLOR_MAKE(45, 55, 70));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.chart_pie));

    showcase_reset_scratch();
    {
        EGUI_VIEW_CHART_SCATTER_PARAMS_INIT(params, 710, 492, 540, 120);
        egui_view_chart_scatter_init_with_params(EGUI_VIEW_OF(&showcase_scratch.chart_scatter), &params);
    }
    egui_view_chart_scatter_set_axis_x(EGUI_VIEW_OF(&showcase_scratch.chart_scatter), 0, 100, 20);
    egui_view_chart_scatter_set_axis_y(EGUI_VIEW_OF(&showcase_scratch.chart_scatter), 0, 80, 20);
    egui_view_chart_scatter_set_series(EGUI_VIEW_OF(&showcase_scratch.chart_scatter), showcase_chart_scatter_series, 1);
    if (showcase_ctx.is_dark_theme)
    {
        egui_view_chart_scatter_set_colors(EGUI_VIEW_OF(&showcase_scratch.chart_scatter), EGUI_COLOR_MAKE(18, 22, 30), EGUI_COLOR_MAKE(72, 88, 110),
                                           EGUI_COLOR_MAKE(36, 44, 58), EGUI_COLOR_MAKE(145, 160, 180));
    }
    else
    {
        egui_view_chart_scatter_set_colors(EGUI_VIEW_OF(&showcase_scratch.chart_scatter), EGUI_COLOR_MAKE(250, 251, 253), EGUI_COLOR_MAKE(60, 75, 95),
                                           EGUI_COLOR_MAKE(215, 222, 232), EGUI_COLOR_MAKE(45, 55, 70));
    }
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.chart_scatter));
}

static void showcase_draw_time_static(void)
{
    showcase_draw_panel(10, 614, 350, 292);
    showcase_draw_title(16, 618, 340, showcase_text("Time/Date", "时间/日期"));
}

static void showcase_draw_analog_clock_anim(void)
{
    uint16_t tick = showcase_ctx.anim_tick;
    uint8_t sec = tick == 0U ? 30U : (uint8_t)(tick % 60U);
    uint8_t min = tick == 0U ? 10U : (uint8_t)((tick / 60U) % 60U);
    uint8_t hr = tick == 0U ? 10U : (uint8_t)((tick / 3600U) % 12U);
    egui_color_t text_color = showcase_get_text_color();

    showcase_clear_panel_region(16, 640, 130, 130);

    showcase_reset_scratch();
    {
        EGUI_VIEW_ANALOG_CLOCK_PARAMS_INIT(params, 16, 640, 130, 130, 10, 10, 30);
        egui_view_analog_clock_init_with_params(EGUI_VIEW_OF(&showcase_scratch.analog_clock), &params);
    }
    egui_view_analog_clock_show_second(EGUI_VIEW_OF(&showcase_scratch.analog_clock), 1);
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&showcase_scratch.analog_clock), hr, min, sec);
    showcase_scratch.analog_clock.dial_color = text_color;
    showcase_scratch.analog_clock.tick_color = text_color;
    showcase_scratch.analog_clock.hour_color = text_color;
    showcase_scratch.analog_clock.minute_color = text_color;
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.analog_clock));
}

static void showcase_draw_digital_time_anim(void)
{
    uint16_t tick = showcase_ctx.anim_tick;
    egui_color_t text_color = showcase_get_text_color();

    showcase_clear_panel_region(160, 640, 170, 70);

    showcase_reset_scratch();
    egui_view_digital_clock_init(EGUI_VIEW_OF(&showcase_scratch.digital_clock));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.digital_clock), 160, 640);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.digital_clock), 170, 30);
    egui_view_digital_clock_set_time(EGUI_VIEW_OF(&showcase_scratch.digital_clock), 14, 30, 0);
    egui_view_digital_clock_set_colon_blink(EGUI_VIEW_OF(&showcase_scratch.digital_clock), 1);
    egui_view_digital_clock_set_colon_visible(EGUI_VIEW_OF(&showcase_scratch.digital_clock), (uint8_t)(((tick / 5U) & 0x01U) == 0U));
    egui_view_label_set_font_color(EGUI_VIEW_OF(&showcase_scratch.digital_clock), text_color, EGUI_ALPHA_100);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.digital_clock));

    showcase_reset_scratch();
    egui_view_stopwatch_init(EGUI_VIEW_OF(&showcase_scratch.stopwatch));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.stopwatch), 160, 680);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.stopwatch), 170, 30);
    egui_view_stopwatch_set_state(EGUI_VIEW_OF(&showcase_scratch.stopwatch), 1);
    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&showcase_scratch.stopwatch), (uint32_t)tick * 100U);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&showcase_scratch.stopwatch), text_color, EGUI_ALPHA_100);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.stopwatch));
}

static void showcase_draw_special_static(void)
{
    egui_color_t text_color = showcase_get_text_color();
    egui_color_t caption_color = showcase_get_caption_color();

    showcase_draw_panel(394, 614, 236, 226);
    showcase_draw_title(400, 618, 220, showcase_text("Specialized", "特殊"));

    showcase_reset_scratch();
    {
        EGUI_VIEW_COMPASS_PARAMS_INIT(params, 400, 640, 110, 110, 45);
        egui_view_compass_init_with_params(EGUI_VIEW_OF(&showcase_scratch.compass), &params);
    }
    showcase_scratch.compass.dial_color = text_color;
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.compass));

    showcase_reset_scratch();
    {
        EGUI_VIEW_LINE_PARAMS_INIT(params, 526, 660, 100, 6, 2, EGUI_COLOR_MAKE(72, 148, 184));
        egui_view_line_init_with_params(EGUI_VIEW_OF(&showcase_scratch.line), &params);
    }
    egui_view_line_set_points(EGUI_VIEW_OF(&showcase_scratch.line), showcase_simple_line_points, 2);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.line));
    showcase_draw_caption(526, 668, 60, showcase_text("Line", "线条"), caption_color);

    showcase_reset_scratch();
    {
        EGUI_VIEW_DIVIDER_PARAMS_INIT(params, 556, 672, 2, 80, EGUI_COLOR_MAKE(136, 176, 176));
        egui_view_divider_init_with_params(EGUI_VIEW_OF(&showcase_scratch.divider), &params);
    }
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.divider));
    showcase_draw_caption(562, 678, 60, showcase_text("Divider", "分割线"), caption_color);
}

static void showcase_draw_heart_rate_anim(void)
{
    uint16_t tick = showcase_ctx.anim_tick;

    showcase_clear_panel_region(400, 762, 110, 68);

    showcase_reset_scratch();
    {
        EGUI_VIEW_HEART_RATE_PARAMS_INIT(params, 400, 762, 110, 68, 72);
        egui_view_heart_rate_init_with_params(EGUI_VIEW_OF(&showcase_scratch.heart_rate), &params);
    }
    showcase_scratch.heart_rate.ecg_offset = (uint8_t)(tick & 31U);
    showcase_scratch.heart_rate.beat_phase = showcase_scratch.heart_rate.ecg_offset == 10U ? 8U : 0U;
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.heart_rate));
}

static void showcase_draw_data_static(void)
{
    egui_color_t text_color = showcase_get_text_color();
    const egui_font_t *text_font = showcase_get_data_font();

    showcase_draw_panel(634, 614, 630, 300);
    showcase_force_english_text = 0U;
    showcase_draw_title(640, 618, 628, showcase_text("Data/Container", "数据/容器"));
    showcase_force_english_text = 1U;

    showcase_reset_scratch();
    {
        EGUI_VIEW_TABLE_PARAMS_INIT(params, 640, 640, 200, 96, 3, 3);
        egui_view_table_init_with_params(EGUI_VIEW_OF(&showcase_scratch.table), &params);
    }
    egui_view_table_set_header_rows(EGUI_VIEW_OF(&showcase_scratch.table), 1);
    egui_view_table_set_show_grid(EGUI_VIEW_OF(&showcase_scratch.table), 1);
    egui_view_table_set_cell(EGUI_VIEW_OF(&showcase_scratch.table), 0, 0, showcase_text("Name", "名称"));
    egui_view_table_set_cell(EGUI_VIEW_OF(&showcase_scratch.table), 0, 1, showcase_text("Val", "值"));
    egui_view_table_set_cell(EGUI_VIEW_OF(&showcase_scratch.table), 0, 2, showcase_text("Unit", "单位"));
    egui_view_table_set_cell(EGUI_VIEW_OF(&showcase_scratch.table), 1, 0, showcase_text("Temp", "温度"));
    egui_view_table_set_cell(EGUI_VIEW_OF(&showcase_scratch.table), 1, 1, "25");
    egui_view_table_set_cell(EGUI_VIEW_OF(&showcase_scratch.table), 1, 2, "C");
    egui_view_table_set_cell(EGUI_VIEW_OF(&showcase_scratch.table), 2, 0, showcase_text("Hum", "湿度"));
    egui_view_table_set_cell(EGUI_VIEW_OF(&showcase_scratch.table), 2, 1, "60");
    egui_view_table_set_cell(EGUI_VIEW_OF(&showcase_scratch.table), 2, 2, "%");
    showcase_scratch.table.font = text_font;
    showcase_scratch.table.header_text_color = showcase_ctx.is_dark_theme ? text_color : EGUI_COLOR_WHITE;
    showcase_scratch.table.cell_text_color = text_color;
    egui_view_table_set_header_bg_color(EGUI_VIEW_OF(&showcase_scratch.table),
                                        showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(31, 35, 42) : EGUI_COLOR_MAKE(9, 105, 218));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.table));

    showcase_reset_scratch();
    egui_view_spangroup_init(EGUI_VIEW_OF(&showcase_scratch.spangroup));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_scratch.spangroup), 820, 748);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.spangroup), 180, 40);
    egui_view_spangroup_add_span(EGUI_VIEW_OF(&showcase_scratch.spangroup), showcase_text("Primary ", "主要 "), text_font, EGUI_COLOR_MAKE(72, 144, 168));
    egui_view_spangroup_add_span(EGUI_VIEW_OF(&showcase_scratch.spangroup), showcase_text("Secondary ", "次要 "), text_font, EGUI_COLOR_MAKE(104, 168, 176));
    egui_view_spangroup_add_span(EGUI_VIEW_OF(&showcase_scratch.spangroup), showcase_text("Tertiary", "第三"), text_font, EGUI_COLOR_MAKE(120, 184, 192));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.spangroup));

    showcase_reset_scratch();
    {
        EGUI_VIEW_WINDOW_PARAMS_INIT(params, 1060, 640, 150, 96, 20, "Window");
        egui_view_window_init_with_params(EGUI_VIEW_OF(&showcase_scratch.window.window), &params);
    }
    showcase_scratch.window.window.header_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(31, 35, 42) : EGUI_COLOR_MAKE(9, 105, 218);
    showcase_scratch.window.window.content_bg_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(22, 27, 34) : EGUI_COLOR_MAKE(255, 255, 255);
    egui_view_label_set_font(EGUI_VIEW_OF(&showcase_scratch.window.window.title_label), text_font);
    egui_view_window_set_title(EGUI_VIEW_OF(&showcase_scratch.window.window), showcase_text("Window", "窗口"));
    egui_view_label_set_font_color(EGUI_VIEW_OF(&showcase_scratch.window.window.title_label), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_label_init(EGUI_VIEW_OF(&showcase_scratch.window.content));
    egui_view_set_size(EGUI_VIEW_OF(&showcase_scratch.window.content), 120, 22);
    egui_view_label_set_font(EGUI_VIEW_OF(&showcase_scratch.window.content), text_font);
    egui_view_label_set_text(EGUI_VIEW_OF(&showcase_scratch.window.content), showcase_text("Content", "内容"));
    egui_view_label_set_font_color(EGUI_VIEW_OF(&showcase_scratch.window.content), text_color, EGUI_ALPHA_100);
    egui_view_window_add_content(EGUI_VIEW_OF(&showcase_scratch.window.window), EGUI_VIEW_OF(&showcase_scratch.window.content));
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.window.window));
    showcase_force_english_text = 0U;

    showcase_reset_scratch();
    {
        EGUI_VIEW_MENU_PARAMS_INIT(params, 1060, 752, 150, 86, 20, 20);
        egui_view_menu_init_with_params(EGUI_VIEW_OF(&showcase_scratch.menu), &params);
    }
    egui_view_menu_set_pages(EGUI_VIEW_OF(&showcase_scratch.menu), showcase_get_menu_pages(), 1);
    showcase_scratch.menu.font = showcase_get_text_font();
    showcase_scratch.menu.text_color = text_color;
    showcase_scratch.menu.header_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(31, 35, 42) : EGUI_COLOR_MAKE(9, 105, 218);
    showcase_scratch.menu.item_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(22, 27, 34) : EGUI_COLOR_MAKE(255, 255, 255);
    showcase_scratch.menu.highlight_color = showcase_ctx.is_dark_theme ? EGUI_COLOR_MAKE(48, 54, 61) : EGUI_COLOR_MAKE(208, 215, 222);
    showcase_draw_view(EGUI_VIEW_OF(&showcase_scratch.menu));
}

static void showcase_textinput_focus_changed(egui_view_t *self, int is_focused)
{
    egui_view_textinput_t *textinput = (egui_view_textinput_t *)self;

    if (is_focused)
    {
        textinput->cursor_visible = 1;
        egui_timer_start_timer(&textinput->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);
        egui_view_keyboard_show(EGUI_VIEW_OF(&showcase_keyboard_view), self);
    }
    else
    {
        textinput->cursor_visible = 0;
        egui_timer_stop_timer(&textinput->cursor_timer);
        egui_view_keyboard_hide(EGUI_VIEW_OF(&showcase_keyboard_view));
    }

    egui_view_invalidate(self);
}

static void showcase_textinput_changed(egui_view_t *self, const char *text)
{
    EGUI_UNUSED(self);
    showcase_copy_text(showcase_ctx.textinput_text, sizeof(showcase_ctx.textinput_text), text);
}

static void showcase_switch_changed(egui_view_t *self, int is_checked)
{
    EGUI_UNUSED(self);
    showcase_ctx.switch_checked = (uint8_t)(is_checked ? 1U : 0U);
}

static void showcase_checkbox_changed(egui_view_t *self, int is_checked)
{
    EGUI_UNUSED(self);
    showcase_ctx.checkbox_checked = (uint8_t)(is_checked ? 1U : 0U);
}

static void showcase_radio_click(egui_view_t *view)
{
    uint32_t stable_id = 0U;

    if (!EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_ID_BY_VIEW(&showcase_stage_view, view, &stable_id))
    {
        return;
    }

    showcase_ctx.option_index = stable_id == SHOWCASE_NODE_OPTION2 ? 1U : 0U;
    showcase_refresh_option_views();
}

static void showcase_toggle_changed(egui_view_t *self, uint8_t is_toggled)
{
    EGUI_UNUSED(self);
    showcase_ctx.toggle_checked = is_toggled;
}

static void showcase_slider_changed(egui_view_t *self, uint8_t value)
{
    EGUI_UNUSED(self);
    showcase_ctx.slider_value = value;
}

static void showcase_arc_slider_changed(egui_view_t *self, uint8_t value)
{
    EGUI_UNUSED(self);
    showcase_ctx.arc_slider_value = value;
}

static void showcase_number_picker_changed(egui_view_t *self, int16_t value)
{
    EGUI_UNUSED(self);
    showcase_ctx.numpick_value = value;
}

static void showcase_roller_selected(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    showcase_ctx.roller_index = index;
}

static void showcase_combobox_selected(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    showcase_ctx.combobox_index = index;
}

static void showcase_calendar_day_selected(egui_view_t *self, uint8_t day)
{
    EGUI_UNUSED(self);
    showcase_ctx.calendar_day = day;
}

static void showcase_button_matrix_click(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    showcase_ctx.button_matrix_index = index;
}

static void showcase_tab_bar_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    showcase_ctx.tab_index = index;
}

static void showcase_update_dynamic_nodes(void)
{
    showcase_ctx.nodes[SHOWCASE_NODE_INDEX_ALPHA1].desc.z_order = showcase_ctx.active_layer == 1U ? 12 : 11;
    showcase_ctx.nodes[SHOWCASE_NODE_INDEX_ALPHA2].desc.z_order = showcase_ctx.active_layer == 2U ? 12 : 11;
    showcase_ctx.nodes[SHOWCASE_NODE_INDEX_ALPHA_LABELS].desc.z_order = 13;
}

static void showcase_notify_animated_nodes(void)
{
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(&showcase_stage_view, SHOWCASE_NODE_PROGRESS_BAR_ANIM, SHOWCASE_NODE_CIRCULAR_PROGRESS_ANIM,
                                       SHOWCASE_NODE_ACTIVITY_RING_ANIM, SHOWCASE_NODE_SPINNER_ANIM, SHOWCASE_NODE_ANALOG_CLOCK_ANIM,
                                       SHOWCASE_NODE_DIGITAL_TIME_ANIM, SHOWCASE_NODE_HEART_RATE_ANIM);
}

static void showcase_refresh_theme_views(void)
{
    showcase_apply_stage_background();
    showcase_notify_nodes(showcase_theme_notify_ids, EGUI_ARRAY_SIZE(showcase_theme_notify_ids));
    showcase_notify_animated_nodes();
}

static void showcase_refresh_language_views(void)
{
    showcase_notify_nodes(showcase_language_notify_ids, EGUI_ARRAY_SIZE(showcase_language_notify_ids));
}

static void showcase_refresh_layer_views(void)
{
    showcase_update_dynamic_nodes();
    showcase_notify_nodes(showcase_layer_notify_ids, EGUI_ARRAY_SIZE(showcase_layer_notify_ids));
}

static void showcase_refresh_option_views(void)
{
    showcase_notify_nodes(showcase_option_notify_ids, EGUI_ARRAY_SIZE(showcase_option_notify_ids));
}

static void showcase_pin_fidelity_nodes(void)
{
#if EGUI_SHOWCASE_PARITY_RECORDING
    EGUI_VIEW_VIRTUAL_STAGE_PIN_IDS(&showcase_stage_view, SHOWCASE_NODE_BUTTON_BASIC, SHOWCASE_NODE_TEXTINPUT, SHOWCASE_NODE_SWITCH, SHOWCASE_NODE_CHECKBOX,
                                    SHOWCASE_NODE_OPTION1, SHOWCASE_NODE_OPTION2, SHOWCASE_NODE_TOGGLE_BUTTON, SHOWCASE_NODE_SLIDER, SHOWCASE_NODE_ARC_SLIDER,
                                    SHOWCASE_NODE_NUMBER_PICKER, SHOWCASE_NODE_ROLLER, SHOWCASE_NODE_COMBOBOX, SHOWCASE_NODE_MINI_CALENDAR,
                                    SHOWCASE_NODE_BUTTON_MATRIX, SHOWCASE_NODE_TAB_BAR, SHOWCASE_NODE_LIST, SHOWCASE_NODE_ALPHA1, SHOWCASE_NODE_ALPHA2,
                                    SHOWCASE_NODE_LANG_BUTTON, SHOWCASE_NODE_THEME_BUTTON);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(&showcase_stage_view, SHOWCASE_NODE_BUTTON_BASIC, SHOWCASE_NODE_TEXTINPUT, SHOWCASE_NODE_SWITCH, SHOWCASE_NODE_CHECKBOX,
                                       SHOWCASE_NODE_OPTION1, SHOWCASE_NODE_OPTION2, SHOWCASE_NODE_TOGGLE_BUTTON, SHOWCASE_NODE_SLIDER,
                                       SHOWCASE_NODE_ARC_SLIDER, SHOWCASE_NODE_NUMBER_PICKER, SHOWCASE_NODE_ROLLER, SHOWCASE_NODE_COMBOBOX,
                                       SHOWCASE_NODE_MINI_CALENDAR, SHOWCASE_NODE_BUTTON_MATRIX, SHOWCASE_NODE_TAB_BAR, SHOWCASE_NODE_LIST,
                                       SHOWCASE_NODE_ALPHA1, SHOWCASE_NODE_ALPHA2, SHOWCASE_NODE_LANG_BUTTON, SHOWCASE_NODE_THEME_BUTTON);
#endif
}

static void showcase_theme_button_click(egui_view_t *view)
{
    EGUI_UNUSED(view);
    egui_focus_manager_clear_focus();
    showcase_ctx.is_dark_theme = showcase_ctx.is_dark_theme ? 0U : 1U;
    showcase_refresh_theme_views();
}

static void showcase_language_button_click(egui_view_t *view)
{
    EGUI_UNUSED(view);
    egui_focus_manager_clear_focus();
    showcase_ctx.is_chinese = showcase_ctx.is_chinese ? 0U : 1U;
    showcase_copy_text(showcase_ctx.textinput_text, sizeof(showcase_ctx.textinput_text), showcase_get_default_textinput_text(showcase_ctx.is_chinese));
    showcase_refresh_language_views();
}

static void showcase_layer_card_click(egui_view_t *view)
{
    uint32_t stable_id = 0U;

    if (!EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_ID_BY_VIEW(&showcase_stage_view, view, &stable_id))
    {
        return;
    }

    if (stable_id == SHOWCASE_NODE_ALPHA1)
    {
        showcase_ctx.active_layer = 1U;
    }
    else if (stable_id == SHOWCASE_NODE_ALPHA2)
    {
        showcase_ctx.active_layer = 2U;
    }
    else
    {
        return;
    }

    showcase_refresh_layer_views();
}

static void showcase_anim_cb(egui_timer_t *timer)
{
    EGUI_UNUSED(timer);
    showcase_ctx.anim_tick++;
    showcase_notify_animated_nodes();
}

#if EGUI_SHOWCASE_PARITY_RECORDING
static void showcase_bootstrap_cb(egui_timer_t *timer)
{
    EGUI_UNUSED(timer);
    showcase_pin_fidelity_nodes();
    showcase_refresh_theme_views();
    showcase_refresh_language_views();
    showcase_refresh_layer_views();
    EGUI_VIEW_VIRTUAL_STAGE_INVALIDATE(&showcase_stage_view);
}
#endif

static void showcase_apply_default_state(void)
{
    memset(&showcase_ctx, 0, sizeof(showcase_ctx));
    showcase_ctx.is_dark_theme = 1U;
    showcase_ctx.is_chinese = 0U;
    showcase_ctx.active_layer = 2U;
    showcase_ctx.switch_checked = 1U;
    showcase_ctx.checkbox_checked = 1U;
    showcase_ctx.option_index = 0U;
    showcase_ctx.toggle_checked = 1U;
    showcase_ctx.slider_value = 60U;
    showcase_ctx.arc_slider_value = 70U;
    showcase_ctx.roller_index = 1U;
    showcase_ctx.combobox_index = 0U;
    showcase_ctx.calendar_day = 2U;
    showcase_ctx.button_matrix_index = EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE;
    showcase_ctx.tab_index = 0U;
    showcase_ctx.numpick_value = 42;
    showcase_copy_text(showcase_ctx.textinput_text, sizeof(showcase_ctx.textinput_text), showcase_get_default_textinput_text(showcase_ctx.is_chinese));
}

static void showcase_set_node(uint32_t index, uint32_t stable_id, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, uint16_t view_type, int16_t z_order,
                              uint8_t flags)
{
    showcase_ctx.nodes[index].desc.region.location.x = x;
    showcase_ctx.nodes[index].desc.region.location.y = y;
    showcase_ctx.nodes[index].desc.region.size.width = w;
    showcase_ctx.nodes[index].desc.region.size.height = h;
    showcase_ctx.nodes[index].desc.stable_id = stable_id;
    showcase_ctx.nodes[index].desc.view_type = view_type;
    showcase_ctx.nodes[index].desc.z_order = z_order;
    showcase_ctx.nodes[index].desc.flags = flags;
}

static void showcase_init_nodes(void)
{
    memset(showcase_ctx.nodes, 0, sizeof(showcase_ctx.nodes));

    showcase_set_node(SHOWCASE_NODE_INDEX_BASIC_STATIC, SHOWCASE_NODE_BASIC_STATIC, 10, 4, 214, 296, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_TOGGLE_STATIC, SHOWCASE_NODE_TOGGLE_STATIC, 234, 4, 178, 260, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_PROGRESS_STATIC, SHOWCASE_NODE_PROGRESS_STATIC, 424, 4, 244, 306, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_CANVAS_STATIC, SHOWCASE_NODE_CANVAS_STATIC, 674, 4, 590, 322, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_SLIDER_STATIC, SHOWCASE_NODE_SLIDER_STATIC, 10, 336, 388, 240, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_CHART_STATIC, SHOWCASE_NODE_CHART_STATIC, 424, 336, 840, 280, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_TIME_STATIC, SHOWCASE_NODE_TIME_STATIC, 10, 614, 350, 292, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_SPECIAL_STATIC, SHOWCASE_NODE_SPECIAL_STATIC, 394, 614, 236, 226, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_DATA_STATIC, SHOWCASE_NODE_DATA_STATIC, 634, 614, 630, 300, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_ALPHA_LABELS, SHOWCASE_NODE_ALPHA_LABELS, 680, 174, 230, 136, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 13, 0);

    showcase_set_node(SHOWCASE_NODE_INDEX_BUTTON_BASIC, SHOWCASE_NODE_BUTTON_BASIC, 16, 30, 110, 36, SHOWCASE_VIEW_TYPE_BUTTON, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_TEXTINPUT, SHOWCASE_NODE_TEXTINPUT, 16, 252, 110, 30, SHOWCASE_VIEW_TYPE_TEXTINPUT, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_SWITCH, SHOWCASE_NODE_SWITCH, 240, 30, 54, 28, SHOWCASE_VIEW_TYPE_SWITCH, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_CHECKBOX, SHOWCASE_NODE_CHECKBOX, 240, 66, 160, 24, SHOWCASE_VIEW_TYPE_CHECKBOX, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_OPTION1, SHOWCASE_NODE_OPTION1, 240, 98, 110, 24, SHOWCASE_VIEW_TYPE_RADIO_BUTTON, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_OPTION2, SHOWCASE_NODE_OPTION2, 240, 128, 110, 24, SHOWCASE_VIEW_TYPE_RADIO_BUTTON, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_TOGGLE_BUTTON, SHOWCASE_NODE_TOGGLE_BUTTON, 240, 160, 110, 34, SHOWCASE_VIEW_TYPE_TOGGLE_BUTTON, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_SLIDER, SHOWCASE_NODE_SLIDER, 16, 362, 180, 28, SHOWCASE_VIEW_TYPE_SLIDER, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_ARC_SLIDER, SHOWCASE_NODE_ARC_SLIDER, 11, 393, 120, 120, SHOWCASE_VIEW_TYPE_ARC_SLIDER, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_NUMBER_PICKER, SHOWCASE_NODE_NUMBER_PICKER, 132, 390, 80, 120, SHOWCASE_VIEW_TYPE_NUMBER_PICKER, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_ROLLER, SHOWCASE_NODE_ROLLER, 276, 362, 90, 110, SHOWCASE_VIEW_TYPE_ROLLER, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_COMBOBOX, SHOWCASE_NODE_COMBOBOX, 16, 520, 150, 105, SHOWCASE_VIEW_TYPE_COMBOBOX, 20,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_MINI_CALENDAR, SHOWCASE_NODE_MINI_CALENDAR, 160, 720, 180, 150, SHOWCASE_VIEW_TYPE_MINI_CALENDAR, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_BUTTON_MATRIX, SHOWCASE_NODE_BUTTON_MATRIX, 860, 640, 170, 84, SHOWCASE_VIEW_TYPE_BUTTON_MATRIX, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_TAB_BAR, SHOWCASE_NODE_TAB_BAR, 640, 748, 160, 34, SHOWCASE_VIEW_TYPE_TAB_BAR, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_LIST, SHOWCASE_NODE_LIST, 640, 794, 140, 110, SHOWCASE_VIEW_TYPE_LIST, 5,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_ALPHA1, SHOWCASE_NODE_ALPHA1, 680, 174, 150, 100, SHOWCASE_VIEW_TYPE_LAYER_CARD, 11,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_ALPHA2, SHOWCASE_NODE_ALPHA2, 760, 210, 150, 100, SHOWCASE_VIEW_TYPE_LAYER_CARD, 12,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_LANG_BUTTON, SHOWCASE_NODE_LANG_BUTTON, 1118, 12, 60, 44, SHOWCASE_VIEW_TYPE_BUTTON, 30,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_THEME_BUTTON, SHOWCASE_NODE_THEME_BUTTON, 1188, 12, 60, 44, SHOWCASE_VIEW_TYPE_BUTTON, 30,
                      EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    showcase_set_node(SHOWCASE_NODE_INDEX_PROGRESS_BAR_ANIM, SHOWCASE_NODE_PROGRESS_BAR_ANIM, 430, 30, 210, 16, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 1, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_CIRCULAR_PROGRESS_ANIM, SHOWCASE_NODE_CIRCULAR_PROGRESS_ANIM, 430, 56, 90, 90, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE,
                      1, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_ACTIVITY_RING_ANIM, SHOWCASE_NODE_ACTIVITY_RING_ANIM, 430, 162, 110, 110, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 1,
                      0);
    showcase_set_node(SHOWCASE_NODE_INDEX_SPINNER_ANIM, SHOWCASE_NODE_SPINNER_ANIM, 216, 410, 40, 40, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 1, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_ANALOG_CLOCK_ANIM, SHOWCASE_NODE_ANALOG_CLOCK_ANIM, 16, 640, 130, 130, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 1, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_DIGITAL_TIME_ANIM, SHOWCASE_NODE_DIGITAL_TIME_ANIM, 160, 640, 170, 70, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 1, 0);
    showcase_set_node(SHOWCASE_NODE_INDEX_HEART_RATE_ANIM, SHOWCASE_NODE_HEART_RATE_ANIM, 400, 762, 110, 68, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 1, 0);

    showcase_update_dynamic_nodes();
}

static egui_view_t *showcase_find_live_view(uint32_t stable_id)
{
    return EGUI_VIEW_VIRTUAL_STAGE_FIND_VIEW_BY_ID(&showcase_stage_view, stable_id);
}

static egui_view_t *showcase_adapter_create_view(void *user_context, uint16_t view_type)
{
    EGUI_UNUSED(user_context);

    switch (view_type)
    {
    case SHOWCASE_VIEW_TYPE_BUTTON:
    {
        egui_view_button_t *button = (egui_view_button_t *)egui_malloc(sizeof(egui_view_button_t));
        if (button == NULL)
        {
            return NULL;
        }
        memset(button, 0, sizeof(*button));
        egui_view_button_init(EGUI_VIEW_OF(button));
        return EGUI_VIEW_OF(button);
    }
    case SHOWCASE_VIEW_TYPE_TEXTINPUT:
    {
        showcase_live_textinput_view_t *textinput = (showcase_live_textinput_view_t *)egui_malloc(sizeof(showcase_live_textinput_view_t));
        if (textinput == NULL)
        {
            return NULL;
        }
        memset(textinput, 0, sizeof(*textinput));
        egui_view_textinput_init(EGUI_VIEW_OF(&textinput->textinput));
        egui_view_textinput_set_on_text_changed(EGUI_VIEW_OF(&textinput->textinput), showcase_textinput_changed);
        egui_view_override_api_on_focus_changed(EGUI_VIEW_OF(&textinput->textinput), &textinput->focus_api, showcase_textinput_focus_changed);
        return EGUI_VIEW_OF(&textinput->textinput);
    }
    case SHOWCASE_VIEW_TYPE_SWITCH:
    {
        egui_view_switch_t *switch_view = (egui_view_switch_t *)egui_malloc(sizeof(egui_view_switch_t));
        if (switch_view == NULL)
        {
            return NULL;
        }
        memset(switch_view, 0, sizeof(*switch_view));
        egui_view_switch_init(EGUI_VIEW_OF(switch_view));
        egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(switch_view), showcase_switch_changed);
        return EGUI_VIEW_OF(switch_view);
    }
    case SHOWCASE_VIEW_TYPE_CHECKBOX:
    {
        egui_view_checkbox_t *checkbox = (egui_view_checkbox_t *)egui_malloc(sizeof(egui_view_checkbox_t));
        if (checkbox == NULL)
        {
            return NULL;
        }
        memset(checkbox, 0, sizeof(*checkbox));
        egui_view_checkbox_init(EGUI_VIEW_OF(checkbox));
        egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(checkbox), showcase_checkbox_changed);
        return EGUI_VIEW_OF(checkbox);
    }
    case SHOWCASE_VIEW_TYPE_RADIO_BUTTON:
    {
        egui_view_radio_button_t *radio = (egui_view_radio_button_t *)egui_malloc(sizeof(egui_view_radio_button_t));
        if (radio == NULL)
        {
            return NULL;
        }
        memset(radio, 0, sizeof(*radio));
        egui_view_radio_button_init(EGUI_VIEW_OF(radio));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(radio), showcase_radio_click);
        return EGUI_VIEW_OF(radio);
    }
    case SHOWCASE_VIEW_TYPE_TOGGLE_BUTTON:
    {
        egui_view_toggle_button_t *toggle = (egui_view_toggle_button_t *)egui_malloc(sizeof(egui_view_toggle_button_t));
        if (toggle == NULL)
        {
            return NULL;
        }
        memset(toggle, 0, sizeof(*toggle));
        egui_view_toggle_button_init(EGUI_VIEW_OF(toggle));
        egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(toggle), showcase_toggle_changed);
        return EGUI_VIEW_OF(toggle);
    }
    case SHOWCASE_VIEW_TYPE_SLIDER:
    {
        egui_view_slider_t *slider = (egui_view_slider_t *)egui_malloc(sizeof(egui_view_slider_t));
        if (slider == NULL)
        {
            return NULL;
        }
        memset(slider, 0, sizeof(*slider));
        egui_view_slider_init(EGUI_VIEW_OF(slider));
        egui_view_slider_set_on_value_changed_listener(EGUI_VIEW_OF(slider), showcase_slider_changed);
        return EGUI_VIEW_OF(slider);
    }
    case SHOWCASE_VIEW_TYPE_ARC_SLIDER:
    {
        egui_view_arc_slider_t *arc_slider = (egui_view_arc_slider_t *)egui_malloc(sizeof(egui_view_arc_slider_t));
        if (arc_slider == NULL)
        {
            return NULL;
        }
        memset(arc_slider, 0, sizeof(*arc_slider));
        egui_view_arc_slider_init(EGUI_VIEW_OF(arc_slider));
        egui_view_arc_slider_set_on_value_changed_listener(EGUI_VIEW_OF(arc_slider), showcase_arc_slider_changed);
        return EGUI_VIEW_OF(arc_slider);
    }
    case SHOWCASE_VIEW_TYPE_NUMBER_PICKER:
    {
        egui_view_number_picker_t *picker = (egui_view_number_picker_t *)egui_malloc(sizeof(egui_view_number_picker_t));
        if (picker == NULL)
        {
            return NULL;
        }
        memset(picker, 0, sizeof(*picker));
        egui_view_number_picker_init(EGUI_VIEW_OF(picker));
        egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(picker), showcase_number_picker_changed);
        return EGUI_VIEW_OF(picker);
    }
    case SHOWCASE_VIEW_TYPE_ROLLER:
    {
        egui_view_roller_t *roller = (egui_view_roller_t *)egui_malloc(sizeof(egui_view_roller_t));
        if (roller == NULL)
        {
            return NULL;
        }
        memset(roller, 0, sizeof(*roller));
        egui_view_roller_init(EGUI_VIEW_OF(roller));
        egui_view_roller_set_on_selected_listener(EGUI_VIEW_OF(roller), showcase_roller_selected);
        return EGUI_VIEW_OF(roller);
    }
    case SHOWCASE_VIEW_TYPE_COMBOBOX:
    {
        egui_view_combobox_t *combobox = (egui_view_combobox_t *)egui_malloc(sizeof(egui_view_combobox_t));
        if (combobox == NULL)
        {
            return NULL;
        }
        memset(combobox, 0, sizeof(*combobox));
        egui_view_combobox_init(EGUI_VIEW_OF(combobox));
        egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(combobox), showcase_combobox_selected);
        return EGUI_VIEW_OF(combobox);
    }
    case SHOWCASE_VIEW_TYPE_MINI_CALENDAR:
    {
        egui_view_mini_calendar_t *calendar = (egui_view_mini_calendar_t *)egui_malloc(sizeof(egui_view_mini_calendar_t));
        if (calendar == NULL)
        {
            return NULL;
        }
        memset(calendar, 0, sizeof(*calendar));
        egui_view_mini_calendar_init(EGUI_VIEW_OF(calendar));
        egui_view_mini_calendar_set_on_date_selected_listener(EGUI_VIEW_OF(calendar), showcase_calendar_day_selected);
        return EGUI_VIEW_OF(calendar);
    }
    case SHOWCASE_VIEW_TYPE_BUTTON_MATRIX:
    {
        egui_view_button_matrix_t *button_matrix = (egui_view_button_matrix_t *)egui_malloc(sizeof(egui_view_button_matrix_t));
        if (button_matrix == NULL)
        {
            return NULL;
        }
        memset(button_matrix, 0, sizeof(*button_matrix));
        egui_view_button_matrix_init(EGUI_VIEW_OF(button_matrix));
        egui_view_button_matrix_set_on_click(EGUI_VIEW_OF(button_matrix), showcase_button_matrix_click);
        return EGUI_VIEW_OF(button_matrix);
    }
    case SHOWCASE_VIEW_TYPE_TAB_BAR:
    {
        egui_view_tab_bar_t *tab_bar = (egui_view_tab_bar_t *)egui_malloc(sizeof(egui_view_tab_bar_t));
        if (tab_bar == NULL)
        {
            return NULL;
        }
        memset(tab_bar, 0, sizeof(*tab_bar));
        egui_view_tab_bar_init(EGUI_VIEW_OF(tab_bar));
        egui_view_tab_bar_set_on_tab_changed_listener(EGUI_VIEW_OF(tab_bar), showcase_tab_bar_changed);
        return EGUI_VIEW_OF(tab_bar);
    }
    case SHOWCASE_VIEW_TYPE_LIST:
    {
        egui_view_list_t *list = (egui_view_list_t *)egui_malloc(sizeof(egui_view_list_t));
        if (list == NULL)
        {
            return NULL;
        }
        memset(list, 0, sizeof(*list));
        egui_view_list_init(EGUI_VIEW_OF(list));
        egui_view_list_set_item_height(EGUI_VIEW_OF(list), 30);
        return EGUI_VIEW_OF(list);
    }
    case SHOWCASE_VIEW_TYPE_LAYER_CARD:
    {
        egui_view_t *layer = (egui_view_t *)egui_malloc(sizeof(egui_view_t));
        if (layer == NULL)
        {
            return NULL;
        }
        memset(layer, 0, sizeof(*layer));
        egui_view_init(layer);
        egui_view_set_clickable(layer, 1);
        egui_view_set_on_click_listener(layer, showcase_layer_card_click);
        return layer;
    }
    default:
        return NULL;
    }
}

static void showcase_adapter_destroy_view(void *user_context, egui_view_t *view, uint16_t view_type)
{
    EGUI_UNUSED(user_context);

    if (view_type == SHOWCASE_VIEW_TYPE_TEXTINPUT)
    {
        egui_timer_stop_timer(&((showcase_live_textinput_view_t *)view)->textinput.cursor_timer);
    }

    egui_free(view);
}

static void showcase_adapter_bind_view(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    EGUI_UNUSED(user_context);
    EGUI_UNUSED(index);

    switch (desc->view_type)
    {
    case SHOWCASE_VIEW_TYPE_BUTTON:
        showcase_bind_button_view(view, stable_id);
        if (stable_id == SHOWCASE_NODE_THEME_BUTTON)
        {
            egui_view_set_on_click_listener(view, showcase_theme_button_click);
        }
        else if (stable_id == SHOWCASE_NODE_LANG_BUTTON)
        {
            egui_view_set_on_click_listener(view, showcase_language_button_click);
        }
        break;
    case SHOWCASE_VIEW_TYPE_TEXTINPUT:
        showcase_bind_textinput_view(view);
        break;
    case SHOWCASE_VIEW_TYPE_SWITCH:
        showcase_bind_switch_view(view);
        break;
    case SHOWCASE_VIEW_TYPE_CHECKBOX:
        showcase_bind_checkbox_view(view);
        break;
    case SHOWCASE_VIEW_TYPE_RADIO_BUTTON:
        showcase_bind_radio_view(view, stable_id);
        break;
    case SHOWCASE_VIEW_TYPE_TOGGLE_BUTTON:
        showcase_bind_toggle_button_view(view);
        break;
    case SHOWCASE_VIEW_TYPE_SLIDER:
        showcase_bind_slider_view(view);
        break;
    case SHOWCASE_VIEW_TYPE_ARC_SLIDER:
        showcase_bind_arc_slider_view(view);
        break;
    case SHOWCASE_VIEW_TYPE_NUMBER_PICKER:
        showcase_bind_number_picker_view(view);
        break;
    case SHOWCASE_VIEW_TYPE_ROLLER:
        showcase_bind_roller_view(view);
        break;
    case SHOWCASE_VIEW_TYPE_COMBOBOX:
        showcase_bind_combobox_view(view, desc);
        break;
    case SHOWCASE_VIEW_TYPE_MINI_CALENDAR:
        showcase_bind_mini_calendar_view(view);
        break;
    case SHOWCASE_VIEW_TYPE_BUTTON_MATRIX:
        showcase_bind_button_matrix_view(view);
        break;
    case SHOWCASE_VIEW_TYPE_TAB_BAR:
        showcase_bind_tab_bar_view(view);
        break;
    case SHOWCASE_VIEW_TYPE_LIST:
        showcase_bind_list_view(view, desc);
        break;
    case SHOWCASE_VIEW_TYPE_LAYER_CARD:
        showcase_bind_layer_card_view(view, stable_id);
        break;
    default:
        break;
    }
}

static void showcase_adapter_save_state(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    EGUI_UNUSED(user_context);
    EGUI_UNUSED(index);

    switch (desc->view_type)
    {
    case SHOWCASE_VIEW_TYPE_TEXTINPUT:
        showcase_copy_text(showcase_ctx.textinput_text, sizeof(showcase_ctx.textinput_text), egui_view_textinput_get_text(view));
        break;
    case SHOWCASE_VIEW_TYPE_SWITCH:
        showcase_ctx.switch_checked = ((egui_view_switch_t *)view)->is_checked;
        break;
    case SHOWCASE_VIEW_TYPE_CHECKBOX:
        showcase_ctx.checkbox_checked = ((egui_view_checkbox_t *)view)->is_checked;
        break;
    case SHOWCASE_VIEW_TYPE_RADIO_BUTTON:
        if (((egui_view_radio_button_t *)view)->is_checked)
        {
            showcase_ctx.option_index = stable_id == SHOWCASE_NODE_OPTION2 ? 1U : 0U;
        }
        break;
    case SHOWCASE_VIEW_TYPE_TOGGLE_BUTTON:
        showcase_ctx.toggle_checked = egui_view_toggle_button_is_toggled(view);
        break;
    case SHOWCASE_VIEW_TYPE_SLIDER:
        showcase_ctx.slider_value = egui_view_slider_get_value(view);
        break;
    case SHOWCASE_VIEW_TYPE_ARC_SLIDER:
        showcase_ctx.arc_slider_value = egui_view_arc_slider_get_value(view);
        break;
    case SHOWCASE_VIEW_TYPE_NUMBER_PICKER:
        showcase_ctx.numpick_value = egui_view_number_picker_get_value(view);
        break;
    case SHOWCASE_VIEW_TYPE_ROLLER:
        showcase_ctx.roller_index = egui_view_roller_get_current_index(view);
        break;
    case SHOWCASE_VIEW_TYPE_COMBOBOX:
        showcase_ctx.combobox_index = egui_view_combobox_get_current_index(view);
        break;
    case SHOWCASE_VIEW_TYPE_MINI_CALENDAR:
        showcase_ctx.calendar_day = ((egui_view_mini_calendar_t *)view)->day;
        break;
    case SHOWCASE_VIEW_TYPE_TAB_BAR:
        showcase_ctx.tab_index = ((egui_view_tab_bar_t *)view)->current_index;
        break;
    case SHOWCASE_VIEW_TYPE_LIST:
        showcase_ctx.list_scroll_y = EGUI_VIEW_OF(&((egui_view_list_t *)view)->base.container)->region.location.y;
        break;
    default:
        break;
    }
}

static void showcase_adapter_restore_state(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id,
                                           const egui_virtual_stage_node_desc_t *desc)
{
    EGUI_UNUSED(user_context);
    EGUI_UNUSED(index);

    switch (desc->view_type)
    {
    case SHOWCASE_VIEW_TYPE_TEXTINPUT:
        egui_view_textinput_set_text(view, showcase_ctx.textinput_text);
        break;
    case SHOWCASE_VIEW_TYPE_SWITCH:
        ((egui_view_switch_t *)view)->is_checked = showcase_ctx.switch_checked;
        egui_view_invalidate(view);
        break;
    case SHOWCASE_VIEW_TYPE_CHECKBOX:
        ((egui_view_checkbox_t *)view)->is_checked = showcase_ctx.checkbox_checked;
        egui_view_invalidate(view);
        break;
    case SHOWCASE_VIEW_TYPE_RADIO_BUTTON:
        egui_view_radio_button_set_checked(view, stable_id == SHOWCASE_NODE_OPTION2 ? showcase_ctx.option_index == 1U : showcase_ctx.option_index == 0U);
        break;
    case SHOWCASE_VIEW_TYPE_TOGGLE_BUTTON:
        ((egui_view_toggle_button_t *)view)->is_toggled = showcase_ctx.toggle_checked;
        egui_view_invalidate(view);
        break;
    case SHOWCASE_VIEW_TYPE_SLIDER:
        ((egui_view_slider_t *)view)->value = showcase_ctx.slider_value;
        ((egui_view_slider_t *)view)->is_dragging = 0U;
        egui_view_invalidate(view);
        break;
    case SHOWCASE_VIEW_TYPE_ARC_SLIDER:
        ((egui_view_arc_slider_t *)view)->is_dragging = 0U;
        egui_view_arc_slider_set_value(view, showcase_ctx.arc_slider_value);
        break;
    case SHOWCASE_VIEW_TYPE_NUMBER_PICKER:
        ((egui_view_number_picker_t *)view)->value = showcase_ctx.numpick_value;
        egui_view_invalidate(view);
        break;
    case SHOWCASE_VIEW_TYPE_ROLLER:
        egui_view_roller_set_current_index(view, showcase_ctx.roller_index);
        egui_view_invalidate(view);
        break;
    case SHOWCASE_VIEW_TYPE_COMBOBOX:
        egui_view_combobox_set_current_index(view, showcase_ctx.combobox_index);
        egui_view_combobox_collapse(view);
        break;
    case SHOWCASE_VIEW_TYPE_MINI_CALENDAR:
        egui_view_mini_calendar_set_date(view, 2026, 3, showcase_ctx.calendar_day);
        break;
    case SHOWCASE_VIEW_TYPE_TAB_BAR:
        egui_view_tab_bar_set_current_index(view, showcase_ctx.tab_index);
        break;
    case SHOWCASE_VIEW_TYPE_LIST:
        egui_view_scroll_to(EGUI_VIEW_OF(&((egui_view_list_t *)view)->base.container), 0, showcase_ctx.list_scroll_y);
        break;
    default:
        break;
    }
}

static uint8_t showcase_adapter_hit_test(void *user_context, uint32_t index, const egui_virtual_stage_node_desc_t *desc, const egui_region_t *screen_region,
                                         egui_dim_t screen_x, egui_dim_t screen_y)
{
    egui_view_t *live_view;

    EGUI_UNUSED(user_context);
    EGUI_UNUSED(index);

    if (desc->view_type == SHOWCASE_VIEW_TYPE_COMBOBOX)
    {
        live_view = showcase_find_live_view(desc->stable_id);
        if (live_view != NULL)
        {
            return egui_region_pt_in_rect(&live_view->region_screen, screen_x, screen_y) ? 1U : 0U;
        }

        {
            egui_region_t collapsed_region = *screen_region;
            collapsed_region.size.height = 30;
            return egui_region_pt_in_rect(&collapsed_region, screen_x, screen_y) ? 1U : 0U;
        }
    }

    return egui_region_pt_in_rect(screen_region, screen_x, screen_y) ? 1U : 0U;
}

static uint8_t showcase_adapter_should_keep_alive(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id,
                                                  const egui_virtual_stage_node_desc_t *desc)
{
    EGUI_UNUSED(user_context);
    EGUI_UNUSED(index);
    EGUI_UNUSED(stable_id);

    if (desc->view_type == SHOWCASE_VIEW_TYPE_COMBOBOX)
    {
        return egui_view_combobox_is_expanded(view) ? 1U : 0U;
    }

    if (desc->view_type == SHOWCASE_VIEW_TYPE_LIST)
    {
        return 1U;
    }

    return 0U;
}

static void showcase_adapter_draw_node(void *user_context, egui_view_t *page, uint32_t index, const egui_virtual_stage_node_desc_t *desc,
                                       const egui_region_t *screen_region)
{
    EGUI_UNUSED(user_context);
    EGUI_UNUSED(page);
    EGUI_UNUSED(index);

    showcase_draw_origin_x = (egui_dim_t)(screen_region->location.x - desc->region.location.x);
    showcase_draw_origin_y = (egui_dim_t)(screen_region->location.y - desc->region.location.y);

    switch (desc->stable_id)
    {
    case SHOWCASE_NODE_BASIC_STATIC:
        showcase_draw_basic_static();
        break;
    case SHOWCASE_NODE_TOGGLE_STATIC:
        showcase_draw_toggle_static();
        break;
    case SHOWCASE_NODE_PROGRESS_STATIC:
        showcase_draw_progress_static();
        break;
    case SHOWCASE_NODE_CANVAS_STATIC:
        showcase_draw_canvas_static();
        break;
    case SHOWCASE_NODE_SLIDER_STATIC:
        showcase_draw_slider_static();
        break;
    case SHOWCASE_NODE_CHART_STATIC:
        showcase_draw_chart_static();
        break;
    case SHOWCASE_NODE_TIME_STATIC:
        showcase_draw_time_static();
        break;
    case SHOWCASE_NODE_SPECIAL_STATIC:
        showcase_draw_special_static();
        break;
    case SHOWCASE_NODE_DATA_STATIC:
        showcase_draw_data_static();
        break;
    case SHOWCASE_NODE_ALPHA_LABELS:
        showcase_draw_alpha_labels();
        break;
    case SHOWCASE_NODE_PROGRESS_BAR_ANIM:
        showcase_draw_progress_bar_anim();
        break;
    case SHOWCASE_NODE_CIRCULAR_PROGRESS_ANIM:
        showcase_draw_circular_progress_anim();
        break;
    case SHOWCASE_NODE_ACTIVITY_RING_ANIM:
        showcase_draw_activity_ring_anim();
        break;
    case SHOWCASE_NODE_SPINNER_ANIM:
        showcase_draw_spinner_anim();
        break;
    case SHOWCASE_NODE_ANALOG_CLOCK_ANIM:
        showcase_draw_analog_clock_anim();
        break;
    case SHOWCASE_NODE_DIGITAL_TIME_ANIM:
        showcase_draw_digital_time_anim();
        break;
    case SHOWCASE_NODE_HEART_RATE_ANIM:
        showcase_draw_heart_rate_anim();
        break;
    case SHOWCASE_NODE_BUTTON_BASIC:
        showcase_prepare_button_preview(SHOWCASE_NODE_BUTTON_BASIC, 16, 30, 110, 36);
        break;
    case SHOWCASE_NODE_TEXTINPUT:
        showcase_prepare_textinput_preview(16, 252, 110, 30);
        break;
    case SHOWCASE_NODE_SWITCH:
        showcase_prepare_switch_preview(240, 30, 54, 28);
        break;
    case SHOWCASE_NODE_CHECKBOX:
        showcase_prepare_checkbox_preview(240, 66, 160, 24);
        break;
    case SHOWCASE_NODE_OPTION1:
        showcase_prepare_radio_preview(SHOWCASE_NODE_OPTION1, 240, 98, 110, 24);
        break;
    case SHOWCASE_NODE_OPTION2:
        showcase_prepare_radio_preview(SHOWCASE_NODE_OPTION2, 240, 128, 110, 24);
        break;
    case SHOWCASE_NODE_TOGGLE_BUTTON:
        showcase_prepare_toggle_button_preview(240, 160, 110, 34);
        break;
    case SHOWCASE_NODE_SLIDER:
        showcase_prepare_slider_preview(16, 362, 180, 28);
        break;
    case SHOWCASE_NODE_ARC_SLIDER:
        showcase_prepare_arc_slider_preview(11, 393, 120, 120);
        break;
    case SHOWCASE_NODE_NUMBER_PICKER:
        showcase_prepare_number_picker_preview(132, 390, 80, 120);
        break;
    case SHOWCASE_NODE_ROLLER:
        showcase_prepare_roller_preview();
        break;
    case SHOWCASE_NODE_COMBOBOX:
        showcase_prepare_combobox_preview();
        break;
    case SHOWCASE_NODE_MINI_CALENDAR:
        showcase_prepare_mini_calendar_preview();
        break;
    case SHOWCASE_NODE_BUTTON_MATRIX:
        showcase_prepare_button_matrix_preview();
        break;
    case SHOWCASE_NODE_TAB_BAR:
        showcase_prepare_tab_bar_preview();
        break;
    case SHOWCASE_NODE_LIST:
        showcase_prepare_list_preview();
        break;
    case SHOWCASE_NODE_ALPHA1:
        showcase_prepare_layer_card_preview(SHOWCASE_NODE_ALPHA1, 680, 174, 150, 100);
        break;
    case SHOWCASE_NODE_ALPHA2:
        showcase_prepare_layer_card_preview(SHOWCASE_NODE_ALPHA2, 760, 210, 150, 100);
        break;
    case SHOWCASE_NODE_LANG_BUTTON:
        showcase_prepare_button_preview(SHOWCASE_NODE_LANG_BUTTON, 1118, 12, 60, 44);
        break;
    case SHOWCASE_NODE_THEME_BUTTON:
        showcase_prepare_button_preview(SHOWCASE_NODE_THEME_BUTTON, 1188, 12, 60, 44);
        break;
    default:
        break;
    }
}

EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_STATEFUL_BRIDGE_INIT_WITH_LIMIT(showcase_stage_bridge, 0, 0, SHOWCASE_CANVAS_WIDTH, SHOWCASE_CANVAS_HEIGHT,
                                                                   SHOWCASE_LIVE_SLOT_LIMIT, showcase_ctx.nodes, showcase_stage_node_t, desc,
                                                                   showcase_adapter_create_view, showcase_adapter_destroy_view, showcase_adapter_bind_view,
                                                                   showcase_adapter_save_state, showcase_adapter_restore_state, showcase_adapter_draw_node,
                                                                   showcase_adapter_hit_test, showcase_adapter_should_keep_alive, &showcase_ctx);

void test_init_ui(void)
{
    showcase_apply_default_state();
    showcase_init_nodes();

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0U;
#endif

    egui_view_canvas_panner_init(EGUI_VIEW_OF(&showcase_root));
    egui_view_set_size(EGUI_VIEW_OF(&showcase_root), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_canvas_panner_set_canvas_size(EGUI_VIEW_OF(&showcase_root), SHOWCASE_CANVAS_WIDTH, SHOWCASE_CANVAS_HEIGHT);
#if EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK
    if (!EGUI_CONFIG_RECORDING_TEST)
    {
        egui_view_canvas_panner_set_offset(EGUI_VIEW_OF(&showcase_root), SHOWCASE_DIRTY_ANIM_FOCUS_X, SHOWCASE_DIRTY_ANIM_FOCUS_Y);
    }
#endif

    EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(&showcase_stage_view, &showcase_stage_bridge);
    showcase_apply_stage_background();
    egui_view_group_add_child(EGUI_VIEW_OF(&showcase_root), EGUI_VIEW_OF(&showcase_stage_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&showcase_root));
#if !EGUI_SHOWCASE_PARITY_RECORDING
    showcase_pin_fidelity_nodes();
#endif

#if EGUI_SHOWCASE_PARITY_RECORDING
    egui_timer_init_timer(&showcase_bootstrap_timer, NULL, showcase_bootstrap_cb);
    egui_timer_start_timer(&showcase_bootstrap_timer, 1, 0);
#endif

    egui_view_keyboard_init(EGUI_VIEW_OF(&showcase_keyboard_view));
    egui_view_set_position(EGUI_VIEW_OF(&showcase_keyboard_view), 0, SHOWCASE_KEYBOARD_Y);
    egui_view_set_size(EGUI_VIEW_OF(&showcase_keyboard_view), EGUI_CONFIG_SCEEN_WIDTH, SHOWCASE_KEYBOARD_HEIGHT);
    egui_view_keyboard_set_font(EGUI_VIEW_OF(&showcase_keyboard_view), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&showcase_keyboard_view));

#if !EGUI_SHOWCASE_PARITY_RECORDING
    egui_timer_init_timer(&showcase_anim_timer, NULL, showcase_anim_cb);
    egui_timer_start_timer(&showcase_anim_timer, 100, 100);
#endif

    EGUI_VIEW_VIRTUAL_STAGE_INVALIDATE(&showcase_stage_view);
}

#if EGUI_CONFIG_RECORDING_TEST
static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1U;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static void showcase_verify_runtime_state(int action_index)
{
    egui_view_t *live_combobox;
    egui_view_t *live_list;
    int verify_index = action_index;
    uint8_t is_small_screen_recording = showcase_recording_uses_small_screen();

    if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&showcase_stage_view) > SHOWCASE_LIVE_SLOT_LIMIT)
    {
        report_runtime_failure("virtual_stage_showcase exceeded live slot limit");
        return;
    }

    if (is_small_screen_recording)
    {
        if (action_index == 2)
        {
            if (egui_view_canvas_panner_get_offset_x(EGUI_VIEW_OF(&showcase_root)) <= 0 &&
                egui_view_canvas_panner_get_offset_y(EGUI_VIEW_OF(&showcase_root)) <= 0)
            {
                report_runtime_failure("small-screen showcase drag did not move canvas");
            }
            return;
        }

        if (action_index > 2)
        {
            verify_index = action_index - 1;
        }
    }

    switch (verify_index)
    {
    case 0:
        if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&showcase_stage_view) > SHOWCASE_PINNED_SLOT_BUDGET)
        {
            report_runtime_failure("virtual_stage_showcase exceeded pinned live-slot budget at startup");
        }
        break;
    case 1:
        if (showcase_ctx.anim_tick == 0U)
        {
            report_runtime_failure("virtual_stage_showcase animation timer did not advance");
        }
        break;
    case 2:
        if (showcase_ctx.is_dark_theme != 0U)
        {
            report_runtime_failure("theme button did not switch virtual showcase to light mode");
        }
        break;
    case 3:
        if (showcase_ctx.is_chinese != 1U)
        {
            report_runtime_failure("language button did not switch virtual showcase to Chinese");
        }
        else if (strcmp(showcase_ctx.textinput_text, showcase_get_default_textinput_text(1U)) != 0)
        {
            report_runtime_failure("language button did not refresh textinput text");
        }
        break;
    case 4:
        if (showcase_ctx.switch_checked != 0U)
        {
            report_runtime_failure("switch did not toggle off");
        }
        break;
    case 5:
        if (showcase_ctx.checkbox_checked != 0U)
        {
            report_runtime_failure("checkbox did not toggle off");
        }
        break;
    case 6:
        if (showcase_ctx.option_index != 1U)
        {
            report_runtime_failure("radio button did not switch to option 2");
        }
        break;
    case 7:
        if (showcase_ctx.toggle_checked != 0U)
        {
            report_runtime_failure("toggle button did not toggle off");
        }
        break;
    case 8:
        if (showcase_ctx.slider_value >= 45U)
        {
            report_runtime_failure("slider drag did not update showcase state");
        }
        break;
    case 9:
        if (showcase_ctx.arc_slider_value >= 55U)
        {
            report_runtime_failure("arc slider drag did not update showcase state");
        }
        break;
    case 10:
        if (showcase_ctx.numpick_value != 43)
        {
            report_runtime_failure("number picker did not increment");
        }
        break;
    case 11:
        live_combobox = showcase_find_live_view(SHOWCASE_NODE_COMBOBOX);
        if (live_combobox == NULL || !egui_view_combobox_is_expanded(live_combobox))
        {
            report_runtime_failure("combobox did not expand");
        }
        break;
    case 12:
        if (showcase_ctx.combobox_index != 1U)
        {
            report_runtime_failure("combobox did not select the expected item");
        }
        break;
    case 13:
        if (showcase_ctx.roller_index != 2U)
        {
            report_runtime_failure("roller did not scroll to the next item");
        }
        break;
    case 14:
        live_list = showcase_find_live_view(SHOWCASE_NODE_LIST);
        if (live_list != NULL)
        {
            showcase_ctx.list_scroll_y = EGUI_VIEW_OF(&((egui_view_list_t *)live_list)->base.container)->region.location.y;
        }
        if (showcase_ctx.list_scroll_y >= 0)
        {
            report_runtime_failure("list drag did not scroll content");
        }
        break;
    case 15:
        if (showcase_ctx.calendar_day != 18U)
        {
            report_runtime_failure("calendar did not select the expected day");
        }
        break;
    case 16:
        if (showcase_ctx.button_matrix_index != 4U)
        {
            report_runtime_failure("button matrix did not update selected index");
        }
        break;
    case 17:
        if (showcase_ctx.tab_index != 2U)
        {
            report_runtime_failure("tab bar did not switch to the expected tab");
        }
        break;
    case 18:
        if (showcase_ctx.active_layer != 1U)
        {
            report_runtime_failure("layer card did not update active layer");
        }
        break;
    default:
        break;
    }

    if (EGUI_VIEW_OF(&showcase_keyboard_view)->region.size.width != EGUI_CONFIG_SCEEN_WIDTH)
    {
        report_runtime_failure("virtual_stage_showcase keyboard width does not match the showcase canvas");
    }
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
#if EGUI_SHOWCASE_PARITY_RECORDING
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 0);
        return true;
    case 1:
        if (first_call)
        {
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_THEME_BUTTON);
        }
        showcase_sim_set_click_node(p_action, SHOWCASE_NODE_INDEX_THEME_BUTTON, 50, 50, 500);
        return true;
    case 2:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 0);
        return true;
    case 3:
        if (first_call)
        {
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_LANG_BUTTON);
        }
        showcase_sim_set_click_node(p_action, SHOWCASE_NODE_INDEX_LANG_BUTTON, 50, 50, 500);
        return true;
    case 4:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 0);
        return true;
    default:
        return false;
    }
#else
    static int last_action = -1;
    int logical_action_index = action_index;
    uint8_t is_small_screen_recording = showcase_recording_uses_small_screen();
    int first_call = action_index != last_action;

    last_action = action_index;

    showcase_verify_runtime_state(action_index);

    if (is_small_screen_recording)
    {
        if (action_index == 1)
        {
            showcase_sim_set_screen_drag(p_action, 180, 220, 80, 120, 10, 350);
            return true;
        }
        if (action_index > 1)
        {
            logical_action_index = action_index - 1;
        }
    }

    switch (logical_action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 600);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_THEME_BUTTON);
        }
        showcase_sim_set_click_node(p_action, SHOWCASE_NODE_INDEX_THEME_BUTTON, 50, 50, 350);
        return true;
    case 2:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_LANG_BUTTON);
        }
        showcase_sim_set_click_node(p_action, SHOWCASE_NODE_INDEX_LANG_BUTTON, 50, 50, 350);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_SWITCH);
        }
        showcase_sim_set_click_node(p_action, SHOWCASE_NODE_INDEX_SWITCH, 50, 50, 300);
        return true;
    case 4:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_CHECKBOX);
        }
        showcase_sim_set_click_node(p_action, SHOWCASE_NODE_INDEX_CHECKBOX, 50, 50, 300);
        return true;
    case 5:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_OPTION2);
        }
        showcase_sim_set_click_node(p_action, SHOWCASE_NODE_INDEX_OPTION2, 50, 50, 300);
        return true;
    case 6:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_TOGGLE_BUTTON);
        }
        showcase_sim_set_click_node(p_action, SHOWCASE_NODE_INDEX_TOGGLE_BUTTON, 50, 50, 300);
        return true;
    case 7:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_SLIDER);
        }
        showcase_sim_set_drag_node(p_action, SHOWCASE_NODE_INDEX_SLIDER, 60, 50, 22, 50, 8, 350);
        return true;
    case 8:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_ARC_SLIDER);
        }
        showcase_sim_set_arc_slider_drag(p_action, 70U, 32U, 8, 350);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_NUMBER_PICKER);
        }
        showcase_sim_set_click_node(p_action, SHOWCASE_NODE_INDEX_NUMBER_PICKER, 50, 16, 300);
        return true;
    case 10:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_COMBOBOX);
        }
        showcase_sim_set_click_node(p_action, SHOWCASE_NODE_INDEX_COMBOBOX, 50, 14, 350);
        return true;
    case 11:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_COMBOBOX);
        }
        if (!showcase_sim_set_combobox_item_click(p_action, 1U, 350))
        {
            EGUI_SIM_SET_WAIT(p_action, 350);
        }
        return true;
    case 12:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_ROLLER);
        }
        showcase_sim_set_drag_node(p_action, SHOWCASE_NODE_INDEX_ROLLER, 50, 68, 50, 28, 6, 350);
        return true;
    case 13:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_LIST);
        }
        showcase_sim_set_drag_node(p_action, SHOWCASE_NODE_INDEX_LIST, 50, 90, 50, 8, 10, 350);
        return true;
    case 14:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_MINI_CALENDAR);
        }
        if (!showcase_sim_set_calendar_day_click(p_action, 18U, 350))
        {
            EGUI_SIM_SET_WAIT(p_action, 350);
        }
        return true;
    case 15:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_BUTTON_MATRIX);
        }
        showcase_sim_set_button_matrix_click(p_action, 4U, 350);
        return true;
    case 16:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_TAB_BAR);
        }
        showcase_sim_set_tab_bar_click(p_action, 2U, 350);
        return true;
    case 17:
        if (first_call)
        {
            recording_request_snapshot();
            showcase_runtime_focus_node(SHOWCASE_NODE_INDEX_ALPHA1);
        }
        showcase_sim_set_click_node(p_action, SHOWCASE_NODE_INDEX_ALPHA1, 50, 50, 350);
        return true;
    case 18:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    default:
        return false;
    }
#endif
}
#endif
