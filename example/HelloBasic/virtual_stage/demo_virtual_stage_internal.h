#ifndef _DEMO_VIRTUAL_STAGE_INTERNAL_H_
#define _DEMO_VIRTUAL_STAGE_INTERNAL_H_

#include <stddef.h>
#include "egui.h"

#define DEMO_NODE_COUNT              100U
#define DEMO_LOG_CAP                 4U
#define DEMO_LOG_ENTRY_LEN           32
#define DEMO_STATUS_TITLE_LEN        96
#define DEMO_STATUS_DETAIL_LEN       128
#define DEMO_STATUS_HINT_LEN         128
#define DEMO_LIVE_SLOT_LIMIT         4

#define DEMO_PANEL_BASE_ID           100U
#define DEMO_KPI_BASE_ID             200U
#define DEMO_SCHEDULE_BASE_ID        300U
#define DEMO_MACHINE_BASE_ID         400U
#define DEMO_SENSOR_BASE_ID          500U
#define DEMO_ZONE_BASE_ID            600U
#define DEMO_ORDER_BASE_ID           700U
#define DEMO_ACTION_BASE_ID          800U
#define DEMO_INPUT_BASE_ID           900U
#define DEMO_ALERT_BASE_ID           1000U
#define DEMO_SWITCH_BASE_ID          1100U
#define DEMO_CHECKBOX_BASE_ID        1200U
#define DEMO_RADIO_BASE_ID           1300U
#define DEMO_SLIDER_BASE_ID          1400U
#define DEMO_TOGGLE_BASE_ID          1500U
#define DEMO_PICKER_BASE_ID          1600U
#define DEMO_COMBOBOX_BASE_ID        1700U
#define DEMO_ROLLER_BASE_ID          1800U
#define DEMO_SEGMENT_BASE_ID         1900U
#define DEMO_PROGRESS_BASE_ID        2000U
#define DEMO_TABLE_SUMMARY_ID        2100U
#define DEMO_QUICK_MATRIX_ID         2200U
#define DEMO_SPANGROUP_NOTE_ID       2300U

#define DEMO_MACHINE_MIXER_ID        401U
#define DEMO_MACHINE_AGV_ID          409U
#define DEMO_MACHINE_PACK_ID         412U
#define DEMO_MACHINE_QC_ID           415U

#define DEMO_ACTION_PIN_MIXER_ID     800U
#define DEMO_ACTION_PIN_AGV_ID       801U
#define DEMO_ACTION_SHIFT_A_ID       802U
#define DEMO_ACTION_SHIFT_B_ID       803U
#define DEMO_ACTION_EXPORT_ID        804U
#define DEMO_ACTION_RESET_ID         805U

#define DEMO_SEARCH_INPUT_ID         900U
#define DEMO_NOTE_INPUT_ID           901U
#define DEMO_PASSIVE_TARGET_ID       200U
#define DEMO_SWITCH_LOOP_ID          1100U
#define DEMO_SWITCH_PURGE_ID         1101U
#define DEMO_CHECKBOX_QA_ID          1200U
#define DEMO_CHECKBOX_TRACE_ID       1201U
#define DEMO_RADIO_AUTO_ID           1300U
#define DEMO_RADIO_HAND_ID           1301U
#define DEMO_SLIDER_SPEED_ID         1400U
#define DEMO_SLIDER_BUFFER_ID        1401U
#define DEMO_TOGGLE_TRACE_ID         1500U
#define DEMO_TOGGLE_LOCK_ID          1501U
#define DEMO_PICKER_BATCH_ID         1600U
#define DEMO_PICKER_CREW_ID          1601U
#define DEMO_COMBO_RECIPE_ID         1700U
#define DEMO_COMBO_LINE_ID           1701U
#define DEMO_ROLLER_CREW_ID          1800U
#define DEMO_ROLLER_DOCK_ID          1801U
#define DEMO_SEGMENT_ROUTE_ID        1900U
#define DEMO_SEGMENT_PACK_ID         1901U

#define DEMO_COLOR_TEXT_PRIMARY      EGUI_COLOR_HEX(0x183B56)
#define DEMO_COLOR_TEXT_SECONDARY    EGUI_COLOR_HEX(0x55708A)
#define DEMO_COLOR_TEXT_MUTED        EGUI_COLOR_HEX(0x7E93A8)
#define DEMO_COLOR_PANEL_FILL        EGUI_COLOR_HEX(0xF8FBFD)
#define DEMO_COLOR_PANEL_BORDER      EGUI_COLOR_HEX(0xCAD7E2)
#define DEMO_COLOR_PAGE_FILL         EGUI_COLOR_HEX(0xF3F8FB)
#define DEMO_COLOR_PAGE_BORDER       EGUI_COLOR_HEX(0xD5E1EA)

#define DEMO_FONT_HEADER             ((const egui_font_t *)&egui_res_font_montserrat_18_4)
#define DEMO_FONT_TITLE              ((const egui_font_t *)&egui_res_font_montserrat_14_4)
#define DEMO_FONT_BODY               ((const egui_font_t *)&egui_res_font_montserrat_12_4)
#define DEMO_FONT_BUTTON             ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define DEMO_FONT_CAP                ((const egui_font_t *)&egui_res_font_montserrat_8_4)

enum
{
    DEMO_VIEW_TYPE_BUTTON = 1,
    DEMO_VIEW_TYPE_TEXTINPUT = 2,
    DEMO_VIEW_TYPE_SWITCH = 3,
    DEMO_VIEW_TYPE_CHECKBOX = 4,
    DEMO_VIEW_TYPE_RADIO = 5,
    DEMO_VIEW_TYPE_SLIDER = 6,
    DEMO_VIEW_TYPE_TOGGLE = 7,
    DEMO_VIEW_TYPE_NUMBER_PICKER = 8,
    DEMO_VIEW_TYPE_COMBOBOX = 9,
    DEMO_VIEW_TYPE_ROLLER = 10,
    DEMO_VIEW_TYPE_SEGMENTED = 11,
    DEMO_VIEW_TYPE_BUTTON_MATRIX = 12,
};

enum
{
    DEMO_NODE_KIND_PANEL = 0,
    DEMO_NODE_KIND_KPI,
    DEMO_NODE_KIND_SCHEDULE,
    DEMO_NODE_KIND_MACHINE,
    DEMO_NODE_KIND_SENSOR,
    DEMO_NODE_KIND_LINE,
    DEMO_NODE_KIND_LED,
    DEMO_NODE_KIND_SPINNER,
    DEMO_NODE_KIND_NOTIFICATION_BADGE,
    DEMO_NODE_KIND_ZONE_BUTTON,
    DEMO_NODE_KIND_ORDER,
    DEMO_NODE_KIND_ACTION_BUTTON,
    DEMO_NODE_KIND_TEXTINPUT,
    DEMO_NODE_KIND_ALERT,
    DEMO_NODE_KIND_SWITCH,
    DEMO_NODE_KIND_CHECKBOX,
    DEMO_NODE_KIND_RADIO,
    DEMO_NODE_KIND_SLIDER,
    DEMO_NODE_KIND_TOGGLE,
    DEMO_NODE_KIND_NUMBER_PICKER,
    DEMO_NODE_KIND_COMBOBOX,
    DEMO_NODE_KIND_ROLLER,
    DEMO_NODE_KIND_SEGMENTED,
    DEMO_NODE_KIND_PROGRESS,
    DEMO_NODE_KIND_CHIPS,
    DEMO_NODE_KIND_BADGE_GROUP,
    DEMO_NODE_KIND_PAGE_INDICATOR,
    DEMO_NODE_KIND_DIVIDER,
    DEMO_NODE_KIND_SCALE,
    DEMO_NODE_KIND_CHART_LINE,
    DEMO_NODE_KIND_TAB_BAR,
    DEMO_NODE_KIND_MENU,
    DEMO_NODE_KIND_BREADCRUMB_BAR,
    DEMO_NODE_KIND_MESSAGE_BAR,
    DEMO_NODE_KIND_ACTIVITY_RING,
    DEMO_NODE_KIND_GAUGE,
    DEMO_NODE_KIND_CIRCULAR_PROGRESS,
    DEMO_NODE_KIND_MINI_CALENDAR,
    DEMO_NODE_KIND_ANALOG_CLOCK,
    DEMO_NODE_KIND_COMPASS,
    DEMO_NODE_KIND_HEART_RATE,
    DEMO_NODE_KIND_CHART_SCATTER,
    DEMO_NODE_KIND_DIGITAL_CLOCK,
    DEMO_NODE_KIND_CHART_BAR,
    DEMO_NODE_KIND_CHART_PIE,
    DEMO_NODE_KIND_STOPWATCH,
    DEMO_NODE_KIND_TILEVIEW,
    DEMO_NODE_KIND_WINDOW,
    DEMO_NODE_KIND_LIST,
    DEMO_NODE_KIND_TEXTBLOCK,
    DEMO_NODE_KIND_TABLE,
    DEMO_NODE_KIND_BUTTON_MATRIX,
    DEMO_NODE_KIND_SPANGROUP,
};

enum
{
    DEMO_BUTTON_STYLE_NEUTRAL = 0,
    DEMO_BUTTON_STYLE_BLUE,
    DEMO_BUTTON_STYLE_TEAL,
    DEMO_BUTTON_STYLE_GREEN,
    DEMO_BUTTON_STYLE_ORANGE,
    DEMO_BUTTON_STYLE_RED,
};

typedef struct demo_virtual_button_view demo_virtual_button_view_t;
typedef struct demo_virtual_textinput_view demo_virtual_textinput_view_t;
typedef struct demo_virtual_switch_view demo_virtual_switch_view_t;
typedef struct demo_virtual_checkbox_view demo_virtual_checkbox_view_t;
typedef struct demo_virtual_radio_view demo_virtual_radio_view_t;
typedef struct demo_virtual_slider_view demo_virtual_slider_view_t;
typedef struct demo_virtual_toggle_view demo_virtual_toggle_view_t;
typedef struct demo_virtual_picker_view demo_virtual_picker_view_t;
typedef struct demo_virtual_combobox_view demo_virtual_combobox_view_t;
typedef struct demo_virtual_roller_view demo_virtual_roller_view_t;
typedef struct demo_virtual_segmented_view demo_virtual_segmented_view_t;
typedef struct demo_virtual_button_matrix_view demo_virtual_button_matrix_view_t;
typedef struct demo_virtual_node demo_virtual_node_t;
typedef struct demo_virtual_stage_context demo_virtual_stage_context_t;

struct demo_virtual_button_view
{
    egui_view_button_t button;
    uint32_t stable_id;
    char label[24];
};

struct demo_virtual_textinput_view
{
    egui_view_textinput_t textinput;
    egui_view_api_t focus_api;
    uint32_t stable_id;
};

struct demo_virtual_switch_view
{
    egui_view_switch_t switch_view;
    uint32_t stable_id;
};

struct demo_virtual_checkbox_view
{
    egui_view_checkbox_t checkbox;
    uint32_t stable_id;
};

struct demo_virtual_radio_view
{
    egui_view_radio_button_t radio;
    uint32_t stable_id;
};

struct demo_virtual_slider_view
{
    egui_view_slider_t slider;
    uint32_t stable_id;
};

struct demo_virtual_toggle_view
{
    egui_view_toggle_button_t toggle;
    uint32_t stable_id;
};

struct demo_virtual_picker_view
{
    egui_view_number_picker_t picker;
    uint32_t stable_id;
};

struct demo_virtual_combobox_view
{
    egui_view_combobox_t combobox;
    uint32_t stable_id;
};

struct demo_virtual_roller_view
{
    egui_view_roller_t roller;
    uint32_t stable_id;
};

struct demo_virtual_segmented_view
{
    egui_view_segmented_control_t segmented;
    uint32_t stable_id;
};

struct demo_virtual_button_matrix_view
{
    egui_view_button_matrix_t matrix;
    uint32_t stable_id;
};

struct demo_virtual_node
{
    egui_virtual_stage_node_desc_t desc;
    uint8_t kind;
    uint8_t accent;
    uint8_t data_index;
    uint8_t reserved;
    char title[24];
};

struct demo_virtual_stage_context
{
    uint8_t pin_mixer_enabled;
    uint8_t pin_agv_enabled;
    uint8_t shift_a_enabled;
    uint8_t shift_b_enabled;
    uint8_t machine_active[4];
    uint8_t zone_enabled[6];
    uint8_t switch_enabled[2];
    uint8_t checkbox_enabled[2];
    uint8_t radio_selected_index;
    uint8_t slider_value[2];
    uint8_t toggle_enabled[2];
    uint8_t combobox_index[2];
    uint8_t roller_index[2];
    uint8_t segment_index[2];
    uint8_t quick_matrix_index;
    uint8_t peak_live_slots;
    int16_t picker_value[2];
    uint16_t bind_count;
    uint16_t destroy_count;
    uint16_t export_count;
    uint16_t peak_live_bytes;
    uint8_t status_tick;
    uint8_t log_write_index;
    uint8_t log_count;
    char search_text[EGUI_CONFIG_TEXTINPUT_MAX_LENGTH + 1];
    char note_text[EGUI_CONFIG_TEXTINPUT_MAX_LENGTH + 1];
    char action_logs[DEMO_LOG_CAP][DEMO_LOG_ENTRY_LEN];
    char status_title[DEMO_STATUS_TITLE_LEN];
    char status_detail[DEMO_STATUS_DETAIL_LEN];
    char status_hint[DEMO_STATUS_HINT_LEN];
    demo_virtual_node_t nodes[DEMO_NODE_COUNT];
};

extern const char *demo_quick_matrix_titles[4];
extern const egui_view_virtual_stage_adapter_t demo_adapter;

egui_background_t *demo_get_textinput_background(uint8_t is_focused);
egui_view_t *demo_find_live_view(uint32_t stable_id);
const char *demo_get_input_placeholder(uint32_t stable_id);
const char *demo_get_input_text(uint32_t stable_id);
void demo_set_input_text(uint32_t stable_id, const char *text);
int demo_get_switch_index(uint32_t stable_id);
int demo_get_checkbox_index(uint32_t stable_id);
int demo_get_radio_index(uint32_t stable_id);
int demo_get_slider_index(uint32_t stable_id);
int demo_get_toggle_index(uint32_t stable_id);
int demo_get_picker_index(uint32_t stable_id);
int demo_get_combobox_index(uint32_t stable_id);
int demo_get_roller_index(uint32_t stable_id);
int demo_get_segment_index(uint32_t stable_id);
const char **demo_get_combobox_items(uint32_t stable_id, uint8_t *count);
const char **demo_get_roller_items(uint32_t stable_id, uint8_t *count);
const char **demo_get_segment_items(uint32_t stable_id, uint8_t *count);
const char *demo_get_toggle_icon(uint32_t stable_id);
void demo_get_button_label(uint32_t stable_id, char *buffer, size_t buffer_size);
uint8_t demo_get_button_style(uint32_t stable_id);
void demo_get_button_style_colors(uint8_t style, egui_color_t *fill, egui_color_t *border, egui_color_t *text);
egui_background_t *demo_get_button_background(uint8_t style);
void demo_textinput_changed(egui_view_t *self, const char *text);
void demo_textinput_focus_changed(egui_view_t *self, int is_focused);
void demo_switch_changed(egui_view_t *self, int is_checked);
void demo_checkbox_changed(egui_view_t *self, int is_checked);
void demo_radio_click_cb(egui_view_t *self);
void demo_slider_changed(egui_view_t *self, uint8_t value);
void demo_toggle_changed(egui_view_t *self, uint8_t is_toggled);
void demo_picker_changed(egui_view_t *self, int16_t value);
void demo_combobox_selected(egui_view_t *self, uint8_t index);
void demo_roller_selected(egui_view_t *self, uint8_t index);
void demo_segment_changed(egui_view_t *self, uint8_t index);
void demo_button_matrix_clicked(egui_view_t *self, uint8_t btn_index);
void demo_button_click_cb(egui_view_t *self);
void demo_adapter_draw_node(void *adapter_context, egui_view_t *page, uint32_t index, const egui_virtual_stage_node_desc_t *desc,
                            const egui_region_t *screen_region);
uint16_t demo_get_view_instance_size(uint16_t view_type);

#endif /* _DEMO_VIRTUAL_STAGE_INTERNAL_H_ */
