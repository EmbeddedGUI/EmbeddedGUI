#include "egui.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "uicode.h"
#include "demo_virtual_stage_internal.h"

#define DEMO_ALERT_RENDER_COUNT 9U
#define DEMO_STATUS_TIMER_MS    160

#define DEMO_CANVAS_WIDTH  HELLO_VIRTUAL_STAGE_CANVAS_WIDTH
#define DEMO_CANVAS_HEIGHT HELLO_VIRTUAL_STAGE_CANVAS_HEIGHT

#define DEMO_HEADER_X 16
#define DEMO_HEADER_Y 16
#define DEMO_HEADER_W (DEMO_CANVAS_WIDTH - 32)
#define DEMO_HEADER_H 92

#define DEMO_PAGE_X 16
#define DEMO_PAGE_Y 124
#define DEMO_PAGE_W (DEMO_CANVAS_WIDTH - 32)
#define DEMO_PAGE_H (DEMO_CANVAS_HEIGHT - DEMO_PAGE_Y - 16)

#define DEMO_OVERVIEW_X 12
#define DEMO_OVERVIEW_Y 12
#define DEMO_OVERVIEW_W 744
#define DEMO_OVERVIEW_H 154

#define DEMO_FLOOR_X 12
#define DEMO_FLOOR_Y 178
#define DEMO_FLOOR_W 464
#define DEMO_FLOOR_H 330

#define DEMO_OPS_X 488
#define DEMO_OPS_Y 178
#define DEMO_OPS_W 268
#define DEMO_OPS_H 330

#define DEMO_ALERT_X 12
#define DEMO_ALERT_Y 520
#define DEMO_ALERT_W 744
#define DEMO_ALERT_H 128

#define DEMO_KPI_W       110
#define DEMO_KPI_H       30
#define DEMO_KPI_GAP_X   8
#define DEMO_KPI_GAP_Y   6
#define DEMO_KPI_START_X (DEMO_OVERVIEW_X + 12)
#define DEMO_KPI_START_Y (DEMO_OVERVIEW_Y + 34)

#define DEMO_INPUT_W        208
#define DEMO_INPUT_H        28
#define DEMO_SEARCH_INPUT_X (DEMO_OVERVIEW_X + 512)
#define DEMO_SEARCH_INPUT_Y (DEMO_OVERVIEW_Y + 34)
#define DEMO_NOTE_INPUT_X   (DEMO_OVERVIEW_X + 512)
#define DEMO_NOTE_INPUT_Y   (DEMO_OVERVIEW_Y + 68)

#define DEMO_SCHEDULE_W       110
#define DEMO_SCHEDULE_H       18
#define DEMO_SCHEDULE_GAP_X   8
#define DEMO_SCHEDULE_GAP_Y   4
#define DEMO_SCHEDULE_START_X (DEMO_OVERVIEW_X + 12)
#define DEMO_SCHEDULE_START_Y (DEMO_OVERVIEW_Y + 100)

#define DEMO_MACHINE_W       62
#define DEMO_MACHINE_H       46
#define DEMO_MACHINE_GAP_X   8
#define DEMO_MACHINE_GAP_Y   6
#define DEMO_MACHINE_START_X (DEMO_FLOOR_X + 14)
#define DEMO_MACHINE_START_Y (DEMO_FLOOR_Y + 34)

#define DEMO_SENSOR_W       84
#define DEMO_SENSOR_H       22
#define DEMO_SENSOR_GAP_X   8
#define DEMO_SENSOR_GAP_Y   6
#define DEMO_SENSOR_START_X (DEMO_FLOOR_X + 14)
#define DEMO_SENSOR_START_Y (DEMO_FLOOR_Y + 246)

#define DEMO_FLOOR_WIDGET_W       69
#define DEMO_FLOOR_WIDGET_H       24
#define DEMO_FLOOR_WIDGET_GAP_X   6
#define DEMO_FLOOR_WIDGET_GAP_Y   6
#define DEMO_FLOOR_WIDGET_START_X (DEMO_FLOOR_X + 306)
#define DEMO_FLOOR_WIDGET_START_Y (DEMO_FLOOR_Y + 34)
#define DEMO_COMBO_W              DEMO_FLOOR_WIDGET_W
#define DEMO_COMBO_H              24
#define DEMO_COMBO_START_X        DEMO_FLOOR_WIDGET_START_X
#define DEMO_COMBO_START_Y        (DEMO_FLOOR_WIDGET_START_Y + 3 * (DEMO_FLOOR_WIDGET_H + DEMO_FLOOR_WIDGET_GAP_Y))
#define DEMO_SEGMENT_W            DEMO_FLOOR_WIDGET_W
#define DEMO_SEGMENT_H            24
#define DEMO_SEGMENT_START_X      DEMO_FLOOR_WIDGET_START_X
#define DEMO_SEGMENT_START_Y      (DEMO_COMBO_START_Y + DEMO_COMBO_H + DEMO_FLOOR_WIDGET_GAP_Y)
#define DEMO_PROGRESS_W           DEMO_FLOOR_WIDGET_W
#define DEMO_PROGRESS_H           18
#define DEMO_PROGRESS_START_X     DEMO_FLOOR_WIDGET_START_X
#define DEMO_PROGRESS_START_Y     (DEMO_SEGMENT_START_Y + DEMO_SEGMENT_H + DEMO_FLOOR_WIDGET_GAP_Y)
#define DEMO_ROLLER_W             DEMO_FLOOR_WIDGET_W
#define DEMO_ROLLER_H             48
#define DEMO_ROLLER_START_X       DEMO_FLOOR_WIDGET_START_X
#define DEMO_ROLLER_START_Y       (DEMO_PROGRESS_START_Y + DEMO_PROGRESS_H + DEMO_FLOOR_WIDGET_GAP_Y)

#define DEMO_ZONE_W       78
#define DEMO_ZONE_H       22
#define DEMO_ZONE_GAP_X   6
#define DEMO_ZONE_GAP_Y   6
#define DEMO_ZONE_START_X (DEMO_OPS_X + 12)
#define DEMO_ZONE_START_Y (DEMO_OPS_Y + 34)

#define DEMO_ORDER_W       57
#define DEMO_ORDER_H       24
#define DEMO_ORDER_GAP_X   6
#define DEMO_ORDER_GAP_Y   4
#define DEMO_ORDER_START_X (DEMO_OPS_X + 12)
#define DEMO_ORDER_START_Y (DEMO_OPS_Y + 92)

#define DEMO_ACTION_W       78
#define DEMO_ACTION_H       22
#define DEMO_ACTION_GAP_X   6
#define DEMO_ACTION_GAP_Y   6
#define DEMO_ACTION_START_X (DEMO_OPS_X + 12)
#define DEMO_ACTION_START_Y (DEMO_OPS_Y + 152)

#define DEMO_RACK_WIDGET_W     118
#define DEMO_RACK_WIDGET_H     24
#define DEMO_RACK_WIDGET_GAP_X 8
#define DEMO_RACK_WIDGET_GAP_Y 6
#define DEMO_SLIDER_START_X    (DEMO_OPS_X + 12)
#define DEMO_SLIDER_START_Y    (DEMO_OPS_Y + 212)
#define DEMO_TOGGLE_START_X    (DEMO_OPS_X + 12)
#define DEMO_TOGGLE_START_Y    (DEMO_OPS_Y + 242)
#define DEMO_PICKER_W          118
#define DEMO_PICKER_H          44
#define DEMO_PICKER_START_X    (DEMO_OPS_X + 12)
#define DEMO_PICKER_START_Y    (DEMO_OPS_Y + 272)

#define DEMO_ALERT_CARD_W       174
#define DEMO_ALERT_CARD_H       28
#define DEMO_ALERT_CARD_GAP_X   6
#define DEMO_ALERT_CARD_GAP_Y   5
#define DEMO_ALERT_CARD_START_X (DEMO_ALERT_X + 12)
#define DEMO_ALERT_CARD_START_Y (DEMO_ALERT_Y + 28)
#define DEMO_ALERT_AUX_X        (DEMO_ALERT_CARD_START_X + 3 * (DEMO_ALERT_CARD_W + DEMO_ALERT_CARD_GAP_X))
#define DEMO_ALERT_TABLE_Y      DEMO_ALERT_CARD_START_Y
#define DEMO_ALERT_MATRIX_Y     (DEMO_ALERT_TABLE_Y + DEMO_ALERT_CARD_H + DEMO_ALERT_CARD_GAP_Y)
#define DEMO_ALERT_SPAN_Y       (DEMO_ALERT_MATRIX_Y + DEMO_ALERT_CARD_H + DEMO_ALERT_CARD_GAP_Y)

static const char *demo_panel_titles[4] = {"Overview", "Floor Grid", "Control Rack", "Alert Queue"};
static const char *demo_panel_subtitles[4] = {"4 KPI  1 schedule  15 meta  2 inputs", "16 machines  1 sensor  5 status  14 quick widgets",
                                              "6 zones  8 orders  6 actions  6 widgets", "1 alert  10 meta  quick matrix  pie  timer"};

static const char *demo_kpi_titles[8] = {"OEE", "Shift", "Orders", "Energy", "Yield", "Buffer", "AGV", "Scrap"};
static const char *demo_kpi_values[8] = {"91%", "A-02", "24", "482kW", "98.4%", "12m", "06", "0.7%"};
static const char *demo_kpi_deltas[8] = {"+2.1%", "live", "-3", "+5%", "+0.4%", "steady", "1 late", "-0.1%"};

static const char *demo_schedule_titles[12] = {"Shift A 06:30", "Shift B 14:30", "Lot 2048 08:00", "CIP 09:20", "Chips", "Badges",
                                               "Pages",         "Stage",         "Scale",          "Trend",     "Tabs",  "Menu"};

static const char *demo_machine_titles[24] = {"Inbound", "Mixer", "Blend", "Cook",   "Fill",  "Seal", "Washer", "Buffer", "Convey", "AGV", "Sorter", "Pallet",
                                              "Chill",   "Press", "Label", "Vision", "Stack", "Pack", "Dock 1", "Dock 2", "Rework", "QC",  "Spare",  "Ship"};

static const char *demo_machine_statuses[24] = {"ready", "standby", "batch 2", "heat ok", "lane 3",  "sealed", "rinse", "queue",
                                                "sync",  "parked",  "scan",    "stack",   "cooling", "press",  "label", "vision",
                                                "stack", "hold",    "dock",    "dock",    "repair",  "idle",   "spare", "route"};

static const uint8_t demo_machine_loads[24] = {32, 46, 58, 70, 64, 74, 38, 41, 55, 28, 49, 62, 43, 66, 54, 59, 45, 35, 30, 26, 22, 36, 18, 67};

static const char *demo_sensor_titles[6] = {"Temp", "Flow", "Route", "Beacon", "CIP", "Alarm"};
static const char *demo_sensor_values[6] = {"72C", "118L", "open", "auto", "42%", "ready"};

static const char *demo_zone_titles[6] = {"Zone A", "Zone B", "Zone C", "Zone D", "Zone E", "Zone F"};

static const char *demo_order_ids[8] = {"ORD-1982", "ORD-2016", "ORD-2024", "ORD-2032", "ORD-2048", "ORD-2056", "ORD-2070", "ORD-2084"};
static const char *demo_order_steps[8] = {"mix", "prep", "cook", "fill", "blend", "seal", "label", "qc"};
static const char *demo_order_eta[8] = {"08:40", "09:05", "09:20", "10:00", "10:25", "11:10", "11:35", "12:15"};
static const uint16_t demo_order_eta_minutes[8] = {8U * 60U + 40U,  9U * 60U + 5U,   9U * 60U + 20U,  10U * 60U,
                                                   10U * 60U + 25U, 11U * 60U + 10U, 11U * 60U + 35U, 12U * 60U + 15U};

static const char *demo_alert_titles[12] = {"Mixer temp", "Dock lane", "Seal film",  "AGV queue", "Ink low",    "Steam drift",
                                            "Door open",  "QC batch",  "Power peak", "Pack jam",  "Vision lag", "Export hold"};
static const char *demo_alert_times[12] = {"1m", "3m", "4m", "6m", "8m", "9m", "11m", "13m", "15m", "18m", "22m", "27m"};
static const uint8_t demo_alert_levels[12] = {2, 1, 1, 1, 0, 1, 2, 0, 1, 2, 0, 1};
const char *demo_quick_matrix_titles[4] = {"Prep", "Audit", "Dock", "Ship"};

static const char *demo_switch_titles[2] = {"Loop", "Purge"};
static const char *demo_checkbox_titles[2] = {"QA", "Lot"};
static const char *demo_radio_titles[2] = {"Auto", "Hand"};
static const char *demo_slider_titles[2] = {"Speed", "Buffer"};
static const char *demo_toggle_titles[2] = {"Trace", "Lock"};
static const char *demo_picker_titles[2] = {"Batch", "Crew"};
static const char *demo_combobox_titles[2] = {"Recipe", "Line"};
static const char *demo_roller_titles[2] = {"Crew", "Dock"};
static const char *demo_segment_titles[2] = {"Route", "Pack"};
static const char *demo_progress_titles[2] = {"Util", "WIP"};
static const char *demo_recipe_items[3] = {"A12", "B08", "C20"};
static const char *demo_line_items[3] = {"L1", "L2", "QA"};
static const char *demo_crew_items[4] = {"C1", "C2", "C3", "C4"};
static const char *demo_dock_items[4] = {"D1", "D2", "D3", "D4"};
static const char *demo_route_segments[2] = {"Std", "Fast"};
static const char *demo_pack_segments[2] = {"Box", "Bag"};

static egui_view_canvas_panner_t demo_root;
static egui_view_card_t header_card;
static egui_view_label_t header_title;
static egui_view_label_t header_detail;
static egui_view_label_t header_hint;
static egui_view_virtual_stage_t virtual_stage;
static egui_view_api_t virtual_stage_touch_api;
static egui_timer_t status_timer;
static demo_virtual_stage_context_t demo_context;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

EGUI_VIEW_CARD_PARAMS_INIT(header_card_params, DEMO_HEADER_X, DEMO_HEADER_Y, DEMO_HEADER_W, DEMO_HEADER_H, 18);
EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT_WITH_LIMIT(virtual_stage_bridge, DEMO_PAGE_X, DEMO_PAGE_Y, DEMO_PAGE_W, DEMO_PAGE_H, DEMO_LIVE_SLOT_LIMIT,
                                                          demo_context.nodes, demo_virtual_node_t, desc, &demo_stage_ops, &demo_context);

EGUI_BACKGROUND_GRADIENT_PARAM_INIT(screen_bg_param, EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL, EGUI_COLOR_HEX(0xEEF4F7), EGUI_COLOR_HEX(0xD8E7EE), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(screen_bg_params, &screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(screen_bg, &screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(header_bg_param, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 18, 1, DEMO_COLOR_PANEL_BORDER, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(header_bg_params, &header_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(header_bg, &header_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(page_bg_param, DEMO_COLOR_PAGE_FILL, EGUI_ALPHA_100, 20, 1, DEMO_COLOR_PAGE_BORDER, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(page_bg_params, &page_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(page_bg, &page_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(button_neutral_param, EGUI_COLOR_HEX(0xE8EEF4), EGUI_ALPHA_100, 12, 1, EGUI_COLOR_HEX(0xC8D4DE),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(button_neutral_params, &button_neutral_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(button_neutral_bg, &button_neutral_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(button_blue_param, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100, 12, 1, EGUI_COLOR_HEX(0x29527C),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(button_blue_params, &button_blue_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(button_blue_bg, &button_blue_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(button_teal_param, EGUI_COLOR_HEX(0x167C88), EGUI_ALPHA_100, 12, 1, EGUI_COLOR_HEX(0x0F616A),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(button_teal_params, &button_teal_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(button_teal_bg, &button_teal_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(button_green_param, EGUI_COLOR_HEX(0x2E8E58), EGUI_ALPHA_100, 12, 1, EGUI_COLOR_HEX(0x236D44),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(button_green_params, &button_green_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(button_green_bg, &button_green_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(button_orange_param, EGUI_COLOR_HEX(0xD68A37), EGUI_ALPHA_100, 12, 1, EGUI_COLOR_HEX(0xA96B23),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(button_orange_params, &button_orange_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(button_orange_bg, &button_orange_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(button_red_param, EGUI_COLOR_HEX(0xC85C4B), EGUI_ALPHA_100, 12, 1, EGUI_COLOR_HEX(0x9D4336),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(button_red_params, &button_red_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(button_red_bg, &button_red_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(input_idle_param, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 12, 1, EGUI_COLOR_HEX(0xA1B3C2), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(input_idle_params, &input_idle_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(input_idle_bg, &input_idle_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(input_focus_param, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 12, 1, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(input_focus_params, &input_focus_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(input_focus_bg, &input_focus_params);

egui_background_t *demo_get_textinput_background(uint8_t is_focused)
{
    return is_focused ? EGUI_BG_OF(&input_focus_bg) : EGUI_BG_OF(&input_idle_bg);
}

static void demo_refresh_status(void);
static void demo_reset_business_state(void);
static void demo_toggle_pin(uint32_t stable_id, uint32_t action_id, uint8_t *enabled_flag);
static const char *demo_get_mode_text(void);
static const char *demo_get_quick_matrix_text(void);
static uint16_t demo_get_live_view_bytes(void);
static uint16_t demo_get_naive_view_bytes(void);
static const char *demo_get_recent_log_text(uint8_t recent_index);

static void demo_copy_text(char *dst, size_t dst_size, const char *src)
{
    size_t copy_len;

    if (dst_size == 0)
    {
        return;
    }

    if (src == NULL)
    {
        dst[0] = '\0';
        return;
    }

    copy_len = strlen(src);
    if (copy_len >= dst_size)
    {
        copy_len = dst_size - 1;
    }

    memcpy(dst, src, copy_len);
    dst[copy_len] = '\0';
}

static void demo_copy_preview(const char *src, char *dst, size_t dst_size)
{
    size_t copy_len;

    if (dst_size == 0)
    {
        return;
    }

    if (src == NULL || src[0] == '\0')
    {
        demo_copy_text(dst, dst_size, "-");
        return;
    }

    copy_len = strlen(src);
    if (copy_len < dst_size)
    {
        demo_copy_text(dst, dst_size, src);
        return;
    }

    if (dst_size <= 4)
    {
        dst[0] = '\0';
        return;
    }

    copy_len = dst_size - 4;
    memcpy(dst, src, copy_len);
    memcpy(dst + copy_len, "...", 3);
    dst[copy_len + 3] = '\0';
}

static void demo_log_event(const char *format, ...)
{
    char entry[DEMO_LOG_ENTRY_LEN];
    va_list args;

    va_start(args, format);
    vsnprintf(entry, sizeof(entry), format, args);
    va_end(args);

    entry[sizeof(entry) - 1] = '\0';
    if (entry[0] == '\0')
    {
        return;
    }

    demo_copy_text(demo_context.action_logs[demo_context.log_write_index], DEMO_LOG_ENTRY_LEN, entry);
    demo_context.log_write_index = (uint8_t)((demo_context.log_write_index + 1U) % DEMO_LOG_CAP);
    if (demo_context.log_count < DEMO_LOG_CAP)
    {
        demo_context.log_count++;
    }
}

static const char *demo_get_recent_log_text(uint8_t recent_index)
{
    uint8_t entry_index;

    if (recent_index >= demo_context.log_count)
    {
        return "-";
    }

    entry_index = (uint8_t)((demo_context.log_write_index + DEMO_LOG_CAP - 1U - recent_index) % DEMO_LOG_CAP);
    if (demo_context.action_logs[entry_index][0] == '\0')
    {
        return "-";
    }

    return demo_context.action_logs[entry_index];
}

static void demo_get_recent_log_preview(uint8_t recent_index, char *dst, size_t dst_size)
{
    demo_copy_preview(demo_get_recent_log_text(recent_index), dst, dst_size);
}

static demo_virtual_node_t *demo_find_node(uint32_t stable_id)
{
    uint32_t i;

    for (i = 0; i < DEMO_NODE_COUNT; i++)
    {
        if (demo_context.nodes[i].desc.stable_id == stable_id)
        {
            return &demo_context.nodes[i];
        }
    }

    return NULL;
}

static const char *demo_get_node_title(uint32_t stable_id)
{
    demo_virtual_node_t *node = demo_find_node(stable_id);

    return node != NULL ? node->title : "-";
}

egui_view_t *demo_find_live_view(uint32_t stable_id)
{
    return EGUI_VIEW_VIRTUAL_STAGE_FIND_VIEW_BY_ID(&virtual_stage, stable_id);
}

static void demo_update_label_text(egui_view_label_t *label, char *storage, size_t storage_size, const char *text)
{
    size_t copy_len;

    if (storage_size == 0)
    {
        return;
    }

    if (strncmp(storage, text, storage_size) == 0)
    {
        return;
    }

    copy_len = strlen(text);
    if (copy_len >= storage_size)
    {
        copy_len = storage_size - 1;
    }

    memcpy(storage, text, copy_len);
    storage[copy_len] = '\0';
    egui_view_label_set_text(EGUI_VIEW_OF(label), storage);
}

static void demo_format_bytes_short(uint16_t bytes, char *buffer, size_t buffer_size)
{
    if (buffer_size == 0)
    {
        return;
    }

    if (bytes >= 1000U)
    {
        snprintf(buffer, buffer_size, "%u.%uK", (unsigned)(bytes / 1000U), (unsigned)((bytes % 1000U) / 100U));
        return;
    }

    snprintf(buffer, buffer_size, "%uB", (unsigned)bytes);
}

static void demo_region_inset(const egui_region_t *src, egui_region_t *dst, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    *dst = *src;
    dst->location.x += (egui_dim_t)left;
    dst->location.y += (egui_dim_t)top;
    dst->size.width -= (egui_dim_t)(left + right);
    dst->size.height -= (egui_dim_t)(top + bottom);
    if (dst->size.width < 1)
    {
        dst->size.width = 1;
    }
    if (dst->size.height < 1)
    {
        dst->size.height = 1;
    }
}

static void demo_make_local_region(const egui_region_t *screen_region, egui_region_t *local_region)
{
    local_region->location.x = 0;
    local_region->location.y = 0;
    local_region->size = screen_region->size;
}

static void demo_draw_round_card(const egui_region_t *region, egui_dim_t radius, egui_color_t fill, egui_color_t border)
{
    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, radius, fill, EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, radius, 1, border, EGUI_ALPHA_100);
}

static void demo_draw_progress_bar(int32_t x, int32_t y, int32_t width, uint8_t value, egui_color_t fill, egui_color_t track)
{
    int32_t fill_width;

    if (width <= 0)
    {
        return;
    }

    egui_canvas_draw_round_rectangle_fill(x, y, width, 4, 2, track, EGUI_ALPHA_100);

    fill_width = (width * value) / 100;
    if (fill_width < 2)
    {
        fill_width = 2;
    }
    if (fill_width > width)
    {
        fill_width = width;
    }
    egui_canvas_draw_round_rectangle_fill(x, y, fill_width, 4, 2, fill, EGUI_ALPHA_100);
}

static void demo_polar_point(int32_t center_x, int32_t center_y, int32_t radius, int16_t angle_deg, int32_t *out_x, int32_t *out_y)
{
    egui_float_t angle_rad;
    egui_float_t cos_val;
    egui_float_t sin_val;

    if (out_x == NULL || out_y == NULL)
    {
        return;
    }

    angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(angle_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));
    cos_val = EGUI_FLOAT_COS(angle_rad);
    sin_val = EGUI_FLOAT_SIN(angle_rad);

    *out_x = center_x + (int32_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(radius), cos_val));
    *out_y = center_y + (int32_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(radius), sin_val));
}

static void demo_get_accent_colors(uint8_t accent, egui_color_t *fill, egui_color_t *border)
{
    switch (accent % 5U)
    {
    case 0:
        *fill = EGUI_COLOR_HEX(0xDCEAFE);
        *border = EGUI_COLOR_HEX(0x5B98E5);
        break;
    case 1:
        *fill = EGUI_COLOR_HEX(0xDFF7F7);
        *border = EGUI_COLOR_HEX(0x3AAAB1);
        break;
    case 2:
        *fill = EGUI_COLOR_HEX(0xE7F7E8);
        *border = EGUI_COLOR_HEX(0x5CB46A);
        break;
    case 3:
        *fill = EGUI_COLOR_HEX(0xFFF0DF);
        *border = EGUI_COLOR_HEX(0xE5A04D);
        break;
    default:
        *fill = EGUI_COLOR_HEX(0xFCE5E5);
        *border = EGUI_COLOR_HEX(0xDE7A7A);
        break;
    }
}

static int demo_get_machine_active_index(uint32_t stable_id)
{
    switch (stable_id)
    {
    case DEMO_MACHINE_MIXER_ID:
        return 0;
    case DEMO_MACHINE_AGV_ID:
        return 1;
    case DEMO_MACHINE_PACK_ID:
        return 2;
    case DEMO_MACHINE_QC_ID:
        return 3;
    default:
        return -1;
    }
}

static uint8_t demo_machine_is_active(uint32_t stable_id)
{
    int index = demo_get_machine_active_index(stable_id);
    return index >= 0 ? demo_context.machine_active[index] : 0;
}

static uint8_t demo_machine_is_pinned(uint32_t stable_id)
{
    if (stable_id == DEMO_MACHINE_MIXER_ID)
    {
        return demo_context.pin_mixer_enabled;
    }
    if (stable_id == DEMO_MACHINE_AGV_ID)
    {
        return demo_context.pin_agv_enabled;
    }
    return 0;
}

static uint8_t demo_count_enabled_zones(void)
{
    uint8_t i;
    uint8_t count = 0;

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_context.zone_enabled); i++)
    {
        count = (uint8_t)(count + (demo_context.zone_enabled[i] ? 1U : 0U));
    }

    return count;
}

const char *demo_get_input_placeholder(uint32_t stable_id)
{
    if (stable_id == DEMO_SEARCH_INPUT_ID)
    {
        return "Find order or lot";
    }
    return "Operator note";
}

static const char *demo_get_input_tag(uint32_t stable_id)
{
    if (stable_id == DEMO_SEARCH_INPUT_ID)
    {
        return "search";
    }
    return "note";
}

const char *demo_get_input_text(uint32_t stable_id)
{
    if (stable_id == DEMO_SEARCH_INPUT_ID)
    {
        return demo_context.search_text;
    }
    return demo_context.note_text;
}

void demo_set_input_text(uint32_t stable_id, const char *text)
{
    if (stable_id == DEMO_SEARCH_INPUT_ID)
    {
        demo_copy_text(demo_context.search_text, sizeof(demo_context.search_text), text);
    }
    else if (stable_id == DEMO_NOTE_INPUT_ID)
    {
        demo_copy_text(demo_context.note_text, sizeof(demo_context.note_text), text);
    }
}

static void demo_set_business_defaults(void)
{
    demo_context.switch_enabled[0] = 1;
    demo_context.switch_enabled[1] = 0;
    demo_context.checkbox_enabled[0] = 1;
    demo_context.checkbox_enabled[1] = 0;
    demo_context.radio_selected_index = 0;
    demo_context.slider_value[0] = 64;
    demo_context.slider_value[1] = 42;
    demo_context.toggle_enabled[0] = 1;
    demo_context.toggle_enabled[1] = 0;
    demo_context.picker_value[0] = 18;
    demo_context.picker_value[1] = 3;
    demo_context.combobox_index[0] = 0;
    demo_context.combobox_index[1] = 1;
    demo_context.roller_index[0] = 2;
    demo_context.roller_index[1] = 1;
    demo_context.segment_index[0] = 0;
    demo_context.segment_index[1] = 0;
    demo_context.quick_matrix_index = 0;
}

int demo_get_switch_index(uint32_t stable_id)
{
    if (stable_id >= DEMO_SWITCH_BASE_ID && stable_id < (DEMO_SWITCH_BASE_ID + EGUI_ARRAY_SIZE(demo_switch_titles)))
    {
        return (int)(stable_id - DEMO_SWITCH_BASE_ID);
    }
    return -1;
}

int demo_get_checkbox_index(uint32_t stable_id)
{
    if (stable_id >= DEMO_CHECKBOX_BASE_ID && stable_id < (DEMO_CHECKBOX_BASE_ID + EGUI_ARRAY_SIZE(demo_checkbox_titles)))
    {
        return (int)(stable_id - DEMO_CHECKBOX_BASE_ID);
    }
    return -1;
}

int demo_get_radio_index(uint32_t stable_id)
{
    if (stable_id >= DEMO_RADIO_BASE_ID && stable_id < (DEMO_RADIO_BASE_ID + EGUI_ARRAY_SIZE(demo_radio_titles)))
    {
        return (int)(stable_id - DEMO_RADIO_BASE_ID);
    }
    return -1;
}

int demo_get_slider_index(uint32_t stable_id)
{
    if (stable_id >= DEMO_SLIDER_BASE_ID && stable_id < (DEMO_SLIDER_BASE_ID + EGUI_ARRAY_SIZE(demo_slider_titles)))
    {
        return (int)(stable_id - DEMO_SLIDER_BASE_ID);
    }
    return -1;
}

int demo_get_toggle_index(uint32_t stable_id)
{
    if (stable_id >= DEMO_TOGGLE_BASE_ID && stable_id < (DEMO_TOGGLE_BASE_ID + EGUI_ARRAY_SIZE(demo_toggle_titles)))
    {
        return (int)(stable_id - DEMO_TOGGLE_BASE_ID);
    }
    return -1;
}

int demo_get_picker_index(uint32_t stable_id)
{
    if (stable_id >= DEMO_PICKER_BASE_ID && stable_id < (DEMO_PICKER_BASE_ID + EGUI_ARRAY_SIZE(demo_picker_titles)))
    {
        return (int)(stable_id - DEMO_PICKER_BASE_ID);
    }
    return -1;
}

int demo_get_combobox_index(uint32_t stable_id)
{
    if (stable_id >= DEMO_COMBOBOX_BASE_ID && stable_id < (DEMO_COMBOBOX_BASE_ID + EGUI_ARRAY_SIZE(demo_combobox_titles)))
    {
        return (int)(stable_id - DEMO_COMBOBOX_BASE_ID);
    }
    return -1;
}

int demo_get_roller_index(uint32_t stable_id)
{
    if (stable_id >= DEMO_ROLLER_BASE_ID && stable_id < (DEMO_ROLLER_BASE_ID + EGUI_ARRAY_SIZE(demo_roller_titles)))
    {
        return (int)(stable_id - DEMO_ROLLER_BASE_ID);
    }
    return -1;
}

int demo_get_segment_index(uint32_t stable_id)
{
    if (stable_id >= DEMO_SEGMENT_BASE_ID && stable_id < (DEMO_SEGMENT_BASE_ID + EGUI_ARRAY_SIZE(demo_segment_titles)))
    {
        return (int)(stable_id - DEMO_SEGMENT_BASE_ID);
    }
    return -1;
}

const char **demo_get_combobox_items(uint32_t stable_id, uint8_t *count)
{
    if (count == NULL)
    {
        return NULL;
    }

    if (stable_id == DEMO_COMBO_RECIPE_ID)
    {
        *count = (uint8_t)EGUI_ARRAY_SIZE(demo_recipe_items);
        return demo_recipe_items;
    }

    *count = (uint8_t)EGUI_ARRAY_SIZE(demo_line_items);
    return demo_line_items;
}

static const char *demo_get_combobox_value(uint32_t stable_id)
{
    int index = demo_get_combobox_index(stable_id);
    uint8_t count = 0;
    const char **items = demo_get_combobox_items(stable_id, &count);

    if (index < 0 || items == NULL || count == 0)
    {
        return "-";
    }

    if (demo_context.combobox_index[index] >= count)
    {
        demo_context.combobox_index[index] = 0;
    }
    return items[demo_context.combobox_index[index]];
}

const char **demo_get_roller_items(uint32_t stable_id, uint8_t *count)
{
    if (count == NULL)
    {
        return NULL;
    }

    if (stable_id == DEMO_ROLLER_CREW_ID)
    {
        *count = (uint8_t)EGUI_ARRAY_SIZE(demo_crew_items);
        return demo_crew_items;
    }

    *count = (uint8_t)EGUI_ARRAY_SIZE(demo_dock_items);
    return demo_dock_items;
}

static const char *demo_get_roller_value(uint32_t stable_id)
{
    int index = demo_get_roller_index(stable_id);
    uint8_t count = 0;
    const char **items = demo_get_roller_items(stable_id, &count);

    if (index < 0 || items == NULL || count == 0)
    {
        return "-";
    }

    if (demo_context.roller_index[index] >= count)
    {
        demo_context.roller_index[index] = 0;
    }
    return items[demo_context.roller_index[index]];
}

const char **demo_get_segment_items(uint32_t stable_id, uint8_t *count)
{
    if (count == NULL)
    {
        return NULL;
    }

    if (stable_id == DEMO_SEGMENT_ROUTE_ID)
    {
        *count = (uint8_t)EGUI_ARRAY_SIZE(demo_route_segments);
        return demo_route_segments;
    }

    *count = (uint8_t)EGUI_ARRAY_SIZE(demo_pack_segments);
    return demo_pack_segments;
}

static const char *demo_get_segment_value(uint32_t stable_id)
{
    int index = demo_get_segment_index(stable_id);
    uint8_t count = 0;
    const char **items = demo_get_segment_items(stable_id, &count);

    if (index < 0 || items == NULL || count == 0)
    {
        return "-";
    }

    if (demo_context.segment_index[index] >= count)
    {
        demo_context.segment_index[index] = 0;
    }
    return items[demo_context.segment_index[index]];
}

static uint8_t demo_get_progress_value(uint32_t stable_id)
{
    if (stable_id == DEMO_PROGRESS_BASE_ID)
    {
        return demo_context.slider_value[0];
    }

    return (uint8_t)EGUI_MIN(100, (int)(demo_context.slider_value[1] + demo_context.export_count * 4U));
}

static const char *demo_get_recipe_text(void)
{
    return demo_get_combobox_value(DEMO_COMBO_RECIPE_ID);
}

static const char *demo_get_line_text(void)
{
    return demo_get_combobox_value(DEMO_COMBO_LINE_ID);
}

static const char *demo_get_route_text(void)
{
    return demo_get_segment_value(DEMO_SEGMENT_ROUTE_ID);
}

static const char *demo_get_pack_text(void)
{
    return demo_get_segment_value(DEMO_SEGMENT_PACK_ID);
}

static const char *demo_get_crew_text(void)
{
    return demo_get_roller_value(DEMO_ROLLER_CREW_ID);
}

static const char *demo_get_dock_text(void)
{
    return demo_get_roller_value(DEMO_ROLLER_DOCK_ID);
}

static const char *demo_get_quick_matrix_text(void)
{
    if (demo_context.quick_matrix_index >= EGUI_ARRAY_SIZE(demo_quick_matrix_titles))
    {
        demo_context.quick_matrix_index = 0;
    }

    return demo_quick_matrix_titles[demo_context.quick_matrix_index];
}

static uint8_t demo_route_is_fast(void)
{
    return demo_context.segment_index[0] == 1U;
}

static uint8_t demo_pack_is_bag(void)
{
    return demo_context.segment_index[1] == 1U;
}

static int demo_get_line_bias(void)
{
    switch (demo_context.combobox_index[1])
    {
    case 0:
        return 3;
    case 2:
        return -2;
    default:
        return 0;
    }
}

static void demo_format_hhmm(int total_minutes, char *buffer, size_t buffer_size)
{
    const int minutes_per_day = 24 * 60;

    if (buffer == NULL || buffer_size == 0)
    {
        return;
    }

    while (total_minutes < 0)
    {
        total_minutes += minutes_per_day;
    }

    total_minutes %= minutes_per_day;
    egui_api_sprintf(buffer, "%02d:%02d", total_minutes / 60, total_minutes % 60);
}

static void demo_get_kpi_value_text(uint8_t index, char *buffer, size_t buffer_size)
{
    int value;

    if (buffer == NULL || buffer_size == 0)
    {
        return;
    }

    switch (index)
    {
    case 0:
        value = 82 + demo_context.slider_value[0] / 5 + (demo_route_is_fast() ? 4 : 0) + (demo_context.switch_enabled[0] ? 2 : 0) -
                (demo_context.radio_selected_index == 1U ? 5 : 0) - (demo_context.toggle_enabled[1] ? 2 : 0);
        value = EGUI_MAX(76, EGUI_MIN(99, value));
        egui_api_sprintf(buffer, "%d%%", value);
        return;
    case 1:
        egui_api_sprintf(buffer, "%s/%s", demo_context.shift_b_enabled ? "B" : "A", demo_get_crew_text());
        return;
    case 2:
        value = 12 + demo_context.picker_value[0] / 2 + demo_context.export_count + (int)demo_count_enabled_zones() + demo_get_line_bias();
        value = EGUI_MAX(8, EGUI_MIN(36, value));
        egui_api_sprintf(buffer, "%d", value);
        return;
    case 3:
        value = 420 + demo_context.slider_value[0] * 2 + (demo_context.switch_enabled[1] ? 24 : 0) + (demo_route_is_fast() ? 18 : 0);
        egui_api_sprintf(buffer, "%dkW", value);
        return;
    case 4:
        value = 972 + demo_context.slider_value[1] / 5 + (demo_context.checkbox_enabled[0] ? 6 : 0) + (demo_context.switch_enabled[0] ? 3 : 0) -
                (demo_context.radio_selected_index == 1U ? 5 : 0) - (demo_route_is_fast() ? 2 : 0);
        value = EGUI_MAX(950, EGUI_MIN(999, value));
        egui_api_sprintf(buffer, "%d.%d%%", value / 10, value % 10);
        return;
    case 5:
        value = 8 + demo_context.slider_value[1] / 6 + (demo_pack_is_bag() ? 2 : 0);
        egui_api_sprintf(buffer, "%dm", value);
        return;
    case 6:
        egui_api_sprintf(buffer, "%s/%s", demo_get_line_text(), demo_get_dock_text());
        return;
    case 7:
        value = 5 + (demo_context.radio_selected_index == 1U ? 3 : 0) + (demo_context.toggle_enabled[1] ? 2 : 0) - (demo_context.checkbox_enabled[0] ? 2 : 0);
        value = EGUI_MAX(1, EGUI_MIN(9, value));
        egui_api_sprintf(buffer, "0.%d%%", value);
        return;
    default:
        demo_copy_text(buffer, buffer_size, index < EGUI_ARRAY_SIZE(demo_kpi_values) ? demo_kpi_values[index] : "-");
        return;
    }
}

static void demo_get_kpi_delta_text(uint8_t index, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0)
    {
        return;
    }

    switch (index)
    {
    case 0:
        demo_copy_text(buffer, buffer_size, demo_route_is_fast() ? "+fast" : (demo_context.switch_enabled[0] ? "+loop" : "steady"));
        return;
    case 1:
        demo_copy_text(buffer, buffer_size, demo_get_mode_text());
        return;
    case 2:
        demo_copy_text(buffer, buffer_size, demo_context.export_count > 0U ? "+ship" : "open");
        return;
    case 3:
        demo_copy_text(buffer, buffer_size, demo_context.switch_enabled[1] ? "+purge" : "base");
        return;
    case 4:
        demo_copy_text(buffer, buffer_size, demo_context.checkbox_enabled[0] ? "+qa" : "scan");
        return;
    case 5:
        demo_copy_text(buffer, buffer_size, demo_count_enabled_zones() > 0U ? "+zone" : "idle");
        return;
    case 6:
        demo_copy_text(buffer, buffer_size, demo_context.pin_agv_enabled ? "pinned" : demo_get_route_text());
        return;
    case 7:
        demo_copy_text(buffer, buffer_size, demo_context.toggle_enabled[1] ? "+lock" : "-qa");
        return;
    default:
        demo_copy_text(buffer, buffer_size, index < EGUI_ARRAY_SIZE(demo_kpi_deltas) ? demo_kpi_deltas[index] : "-");
        return;
    }
}

static const char *demo_get_schedule_text(uint8_t index, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0)
    {
        return "";
    }

    switch (index)
    {
    case 0:
        egui_api_sprintf(buffer, "Shift A %s", demo_get_crew_text());
        return buffer;
    case 1:
        egui_api_sprintf(buffer, "Shift B %s", demo_get_crew_text());
        return buffer;
    case 2:
        egui_api_sprintf(buffer, "Lot %02d %s", (int)demo_context.picker_value[0], demo_get_recipe_text());
        return buffer;
    case 3:
        egui_api_sprintf(buffer, "%s 09:20", demo_context.switch_enabled[1] ? "Purge" : "CIP");
        return buffer;
    case 4:
        egui_api_sprintf(buffer, "Dock %s 11:10", demo_get_dock_text());
        return buffer;
    case 5:
        egui_api_sprintf(buffer, "Pack %s 12:30", demo_get_pack_text());
        return buffer;
    case 6:
        egui_api_sprintf(buffer, "QA %s 13:10", demo_context.checkbox_enabled[0] ? "audit" : "scan");
        return buffer;
    case 7:
        egui_api_sprintf(buffer, "Mix %s 15:40", demo_get_recipe_text());
        return buffer;
    case 8:
        egui_api_sprintf(buffer, "AGV %s 17:00", demo_get_line_text());
        return buffer;
    case 9:
        egui_api_sprintf(buffer, "%s 18:45", demo_context.toggle_enabled[0] ? "Trace" : "Seal");
        return buffer;
    case 10:
        egui_api_sprintf(buffer, "%s 20:10", demo_context.switch_enabled[0] ? "Loop" : "Steam");
        return buffer;
    case 11:
        egui_api_sprintf(buffer, "Export %02d 21:30", (int)(demo_context.export_count % 100U));
        return buffer;
    default:
        demo_copy_text(buffer, buffer_size, index < EGUI_ARRAY_SIZE(demo_schedule_titles) ? demo_schedule_titles[index] : "-");
        return buffer;
    }
}

static const char *demo_get_order_step_text(uint8_t index, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0)
    {
        return "";
    }

    switch (index)
    {
    case 0:
        demo_copy_text(buffer, buffer_size, demo_get_recipe_text());
        return buffer;
    case 1:
        demo_copy_text(buffer, buffer_size, demo_get_line_text());
        return buffer;
    case 2:
        demo_copy_text(buffer, buffer_size, demo_context.switch_enabled[0] ? "loop" : "cook");
        return buffer;
    case 3:
        demo_copy_text(buffer, buffer_size, demo_pack_is_bag() ? "bag" : "box");
        return buffer;
    case 4:
        demo_copy_text(buffer, buffer_size, demo_route_is_fast() ? "fast" : "std");
        return buffer;
    case 5:
        demo_copy_text(buffer, buffer_size, demo_get_dock_text());
        return buffer;
    case 6:
        demo_copy_text(buffer, buffer_size, demo_context.checkbox_enabled[0] ? "audit" : "label");
        return buffer;
    case 7:
        demo_copy_text(buffer, buffer_size, demo_context.toggle_enabled[0] ? "trace" : "ship");
        return buffer;
    default:
        demo_copy_text(buffer, buffer_size, index < EGUI_ARRAY_SIZE(demo_order_steps) ? demo_order_steps[index] : "-");
        return buffer;
    }
}

static const char *demo_get_order_eta_text(uint8_t index, char *buffer, size_t buffer_size)
{
    int eta_minutes;

    if (buffer == NULL || buffer_size == 0)
    {
        return "";
    }
    if (index >= EGUI_ARRAY_SIZE(demo_order_eta_minutes))
    {
        demo_copy_text(buffer, buffer_size, index < EGUI_ARRAY_SIZE(demo_order_eta) ? demo_order_eta[index] : "-");
        return buffer;
    }

    eta_minutes = demo_order_eta_minutes[index];
    eta_minutes += demo_route_is_fast() ? -6 : 4;
    eta_minutes += demo_context.radio_selected_index == 1U ? 5 : 0;
    eta_minutes -= demo_context.pin_agv_enabled ? 3 : 0;
    eta_minutes -= demo_get_line_bias();
    eta_minutes += (int)demo_context.export_count * 2;
    demo_format_hhmm(eta_minutes, buffer, buffer_size);
    return buffer;
}

static const char *demo_get_mode_text(void)
{
    return demo_context.radio_selected_index == 0 ? "auto" : "hand";
}

static void demo_notify_range(uint32_t base_id, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++)
    {
        EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, base_id + i);
    }
}

static void demo_notify_operational_nodes(void)
{
    demo_notify_range(DEMO_KPI_BASE_ID, EGUI_ARRAY_SIZE(demo_kpi_titles));
    demo_notify_range(DEMO_SCHEDULE_BASE_ID, EGUI_ARRAY_SIZE(demo_schedule_titles));
    demo_notify_range(DEMO_MACHINE_BASE_ID, 16U);
    demo_notify_range(DEMO_SENSOR_BASE_ID, EGUI_ARRAY_SIZE(demo_sensor_titles));
    demo_notify_range(DEMO_ORDER_BASE_ID, EGUI_ARRAY_SIZE(demo_order_ids));
    demo_notify_range(DEMO_ALERT_BASE_ID, DEMO_ALERT_RENDER_COUNT);
    demo_notify_range(DEMO_PROGRESS_BASE_ID, EGUI_ARRAY_SIZE(demo_progress_titles));
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_TABLE_SUMMARY_ID);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_QUICK_MATRIX_ID);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_SPANGROUP_NOTE_ID);
}

static void demo_notify_text_dependents(uint32_t stable_id)
{
    if (stable_id == DEMO_SEARCH_INPUT_ID)
    {
        demo_notify_range(DEMO_ORDER_BASE_ID, EGUI_ARRAY_SIZE(demo_order_ids));
        EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_KPI_BASE_ID + 6U);
        EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_KPI_BASE_ID + 7U);
        EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_SPANGROUP_NOTE_ID);
        return;
    }

    if (stable_id == DEMO_NOTE_INPUT_ID)
    {
        EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_ALERT_BASE_ID);
        EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_KPI_BASE_ID + 7U);
        EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_SPANGROUP_NOTE_ID);
    }
}

static void demo_notify_shift_nodes(void)
{
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_ACTION_SHIFT_A_ID);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_ACTION_SHIFT_B_ID);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_KPI_BASE_ID + 1U);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_SCHEDULE_BASE_ID + 0U);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_SCHEDULE_BASE_ID + 1U);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_ALERT_BASE_ID + 6U);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_SPANGROUP_NOTE_ID);
}

static void demo_notify_zone_nodes(uint32_t stable_id)
{
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, stable_id);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_KPI_BASE_ID + 2U);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_ALERT_BASE_ID + 3U);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_TABLE_SUMMARY_ID);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_SPANGROUP_NOTE_ID);
}

const char *demo_get_toggle_icon(uint32_t stable_id)
{
    if (stable_id == DEMO_TOGGLE_TRACE_ID)
    {
        return demo_context.toggle_enabled[0] ? EGUI_ICON_MS_VISIBILITY : EGUI_ICON_MS_VISIBILITY_OFF;
    }
    return demo_context.toggle_enabled[1] ? EGUI_ICON_MS_LOCK : EGUI_ICON_MS_CHECK;
}

static void demo_get_sensor_value(const demo_virtual_node_t *node, char *buffer, size_t buffer_size)
{
    switch (node->data_index)
    {
    case 0:
        egui_api_sprintf(buffer, "%dC", 60 + demo_context.slider_value[0] / 4);
        break;
    case 1:
        egui_api_sprintf(buffer, "%dL", 90 + demo_context.slider_value[1] / 2);
        break;
    case 2:
        demo_copy_text(buffer, buffer_size, demo_context.checkbox_enabled[0] ? "audit" : "open");
        break;
    case 3:
        egui_api_sprintf(buffer, "%s/%s", demo_get_route_text(), demo_get_pack_text());
        break;
    case 4:
        egui_api_sprintf(buffer, "%d%%", demo_context.slider_value[1]);
        break;
    case 5:
        egui_api_sprintf(buffer, "%s/%s", demo_get_line_text(), demo_get_dock_text());
        break;
    default:
        if (node->data_index < EGUI_ARRAY_SIZE(demo_sensor_values))
        {
            demo_copy_text(buffer, buffer_size, demo_sensor_values[node->data_index]);
        }
        else
        {
            demo_copy_text(buffer, buffer_size, "-");
        }
        break;
    }
}

static uint8_t demo_get_alarm_badge_count(void)
{
    uint8_t count = demo_count_enabled_zones();

    count = (uint8_t)(count + (demo_context.export_count > 0U ? 1U : 0U));
    count = (uint8_t)(count + (demo_context.switch_enabled[1] ? 1U : 0U));
    count = (uint8_t)(count + (demo_context.slider_value[1] > 60U ? 1U : 0U));
    if (count > 9U)
    {
        count = 9U;
    }

    return count;
}

static uint8_t demo_get_page_indicator_index(void)
{
    return (uint8_t)((demo_context.quick_matrix_index + demo_context.segment_index[1] + demo_context.combobox_index[1]) % 4U);
}

static uint8_t demo_led_is_active(void)
{
    return demo_context.pin_mixer_enabled || demo_context.pin_agv_enabled || demo_context.switch_enabled[0] || demo_context.shift_a_enabled ||
           demo_context.shift_b_enabled;
}

static uint8_t demo_spinner_is_active(void)
{
    return demo_context.switch_enabled[1] || demo_machine_is_active(DEMO_MACHINE_MIXER_ID) || demo_context.export_count != 0U;
}

static uint8_t demo_get_chart_sample(uint8_t sample_index)
{
    static const int8_t base_values[6] = {30, 52, 36, 68, 46, 60};
    int16_t value = sample_index < EGUI_ARRAY_SIZE(base_values) ? base_values[sample_index] : 40;

    value += (int16_t)((int16_t)demo_context.slider_value[0] - 50) / 4;
    if ((sample_index & 1U) != 0U)
    {
        value += (int16_t)((int16_t)demo_context.slider_value[1] - 40) / 3;
    }
    else
    {
        value += (int16_t)((int16_t)demo_context.slider_value[1] - 40) / 6;
    }
    value += (int16_t)demo_context.quick_matrix_index * 3;
    value += demo_route_is_fast() ? 6 : 0;
    value += demo_pack_is_bag() ? 4 : 0;
    value += (sample_index == 5U) ? (int16_t)EGUI_MIN((uint16_t)(demo_context.export_count * 5U), (uint16_t)18) : 0;

    if (value < 14)
    {
        value = 14;
    }
    if (value > 94)
    {
        value = 94;
    }

    return (uint8_t)value;
}

static uint8_t demo_get_tab_bar_index(void)
{
    return (uint8_t)((demo_context.quick_matrix_index + demo_context.combobox_index[0] + demo_context.segment_index[0]) % 3U);
}

static uint8_t demo_get_menu_index(void)
{
    return (uint8_t)((demo_context.quick_matrix_index + demo_context.roller_index[1] + demo_context.export_count) % 3U);
}

static uint8_t demo_get_chip_index(void)
{
    return (uint8_t)(demo_context.quick_matrix_index % 4U);
}

static uint8_t demo_get_activity_ring_value(uint8_t ring_index)
{
    int value;

    switch (ring_index)
    {
    case 0:
        value = 44 + demo_context.slider_value[0] / 2 + (demo_route_is_fast() ? 10 : 0) + (demo_context.switch_enabled[0] ? 6 : 0);
        break;
    case 1:
        value = 32 + demo_context.slider_value[1] / 2 + (demo_context.checkbox_enabled[0] ? 12 : 0) + (demo_pack_is_bag() ? 8 : 0);
        break;
    default:
        value = 20 + (int)demo_count_enabled_zones() * 8 + (int)EGUI_MIN((uint16_t)(demo_context.export_count * 5U), (uint16_t)25) +
                (demo_context.switch_enabled[1] ? 10 : 0);
        break;
    }

    value = EGUI_MAX(8, EGUI_MIN(96, value));
    return (uint8_t)value;
}

static uint8_t demo_get_gauge_value(void)
{
    int value = 38 + demo_context.slider_value[0] / 2 + (demo_route_is_fast() ? 12 : 0) + demo_get_line_bias() * 2 -
                (demo_context.radio_selected_index == 1U ? 8 : 0) + (demo_context.pin_agv_enabled ? 6 : 0);

    value = EGUI_MAX(10, EGUI_MIN(97, value));
    return (uint8_t)value;
}

static uint8_t demo_get_circular_progress_value(void)
{
    int value = 24 + demo_context.slider_value[1] / 2 + (demo_pack_is_bag() ? 12 : 0) +
                (int)EGUI_MIN((uint16_t)(demo_context.export_count * 6U), (uint16_t)24) - (demo_context.switch_enabled[1] ? 6 : 0);

    value = EGUI_MAX(12, EGUI_MIN(98, value));
    return (uint8_t)value;
}

static uint8_t demo_get_calendar_day(void)
{
    int value = 6 + demo_context.picker_value[0] + (int)demo_context.quick_matrix_index + (int)demo_context.roller_index[1] * 2;

    while (value > 28)
    {
        value -= 28;
    }
    while (value < 1)
    {
        value += 28;
    }

    return (uint8_t)value;
}

static const char *demo_get_calendar_month_text(void)
{
    static const char *months[4] = {"Mar", "Apr", "May", "Jun"};
    uint8_t index = (uint8_t)((demo_context.combobox_index[0] + demo_context.quick_matrix_index + demo_context.segment_index[1]) % EGUI_ARRAY_SIZE(months));

    return months[index];
}

static int16_t demo_get_compass_angle(void)
{
    int16_t angle = 18;

    angle += demo_route_is_fast() ? 72 : 18;
    angle += (int16_t)demo_context.combobox_index[1] * 48;
    angle += (int16_t)demo_context.roller_index[1] * 14;
    angle += (int16_t)demo_context.quick_matrix_index * 21;

    while (angle >= 360)
    {
        angle -= 360;
    }

    return angle;
}

static uint16_t demo_get_heart_rate_value(void)
{
    int value = 78 + demo_context.slider_value[0] / 4 + (int)demo_context.quick_matrix_index * 3 + (demo_route_is_fast() ? 8 : 0) +
                (demo_context.radio_selected_index == 1U ? 6 : 0) - (demo_context.switch_enabled[1] ? 4 : 0);

    value = EGUI_MAX(68, EGUI_MIN(146, value));
    return (uint16_t)value;
}

static uint8_t demo_get_scatter_sample(uint8_t sample_index)
{
    static const int8_t base_values[5] = {22, 57, 38, 66, 48};
    int16_t value = sample_index < EGUI_ARRAY_SIZE(base_values) ? base_values[sample_index] : 40;

    value += (int16_t)((int16_t)demo_context.slider_value[1] - 40) / 3;
    value += (int16_t)demo_context.quick_matrix_index * 4;
    value += demo_pack_is_bag() ? 6 : 0;
    value -= demo_context.combobox_index[1] == 2U ? 4 : 0;

    if (value < 14)
    {
        value = 14;
    }
    if (value > 88)
    {
        value = 88;
    }

    return (uint8_t)value;
}

static uint8_t demo_get_bar_chart_sample(uint8_t sample_index)
{
    static const int8_t base_values[4] = {34, 58, 44, 70};
    int value = sample_index < EGUI_ARRAY_SIZE(base_values) ? base_values[sample_index] : 40;

    value += ((int)demo_context.slider_value[0] - 50) / 6;
    value += ((int)demo_context.slider_value[1] - 40) / 8;
    value += demo_context.machine_active[sample_index % EGUI_ARRAY_SIZE(demo_context.machine_active)] ? 8 : 0;
    value += demo_context.quick_matrix_index == sample_index ? 10 : 0;
    value += (sample_index == 3U && demo_context.switch_enabled[0]) ? 6 : 0;
    value -= (sample_index == 0U && demo_context.switch_enabled[1]) ? 4 : 0;

    if (value < 12)
    {
        value = 12;
    }
    if (value > 94)
    {
        value = 94;
    }

    return (uint8_t)value;
}

static uint8_t demo_get_pie_slice_value(uint8_t slice_index)
{
    int value;

    switch (slice_index)
    {
    case 0:
        value = 22 + (int)demo_context.quick_matrix_index * 8 + (demo_route_is_fast() ? 8 : 0) + (int)demo_count_enabled_zones();
        break;
    case 1:
        value = 26 + demo_context.slider_value[1] / 3 + (demo_pack_is_bag() ? 10 : 0) + (int)demo_context.combobox_index[1] * 4;
        break;
    default:
        value = 18 + (int)EGUI_MIN((uint16_t)(demo_context.export_count * 6U), (uint16_t)24) + (demo_context.pin_mixer_enabled ? 8 : 0) +
                (demo_context.pin_agv_enabled ? 8 : 0);
        break;
    }

    if (value < 8)
    {
        value = 8;
    }
    if (value > 92)
    {
        value = 92;
    }

    return (uint8_t)value;
}

static void demo_get_clock_time(uint8_t *hour_out, uint8_t *minute_out)
{
    int total_minutes = 6 * 60 + 20;

    total_minutes += demo_context.picker_value[0] * 2;
    total_minutes += (int)demo_context.quick_matrix_index * 11;
    total_minutes += (int)demo_context.roller_index[0] * 7;
    total_minutes += demo_context.pin_mixer_enabled ? 9 : 0;
    total_minutes += demo_context.pin_agv_enabled ? 5 : 0;

    total_minutes %= (12 * 60);
    if (total_minutes < 0)
    {
        total_minutes += 12 * 60;
    }

    if (hour_out != NULL)
    {
        *hour_out = (uint8_t)((total_minutes / 60) % 12);
    }
    if (minute_out != NULL)
    {
        *minute_out = (uint8_t)(total_minutes % 60);
    }
}

static uint8_t demo_get_clock_second(void)
{
    int second = 10;

    second += (int)demo_context.status_tick * 3;
    second += (int)demo_context.quick_matrix_index * 7;
    second += (int)demo_context.export_count * 5;
    second += demo_context.slider_value[0] / 10;

    second %= 60;
    if (second < 0)
    {
        second += 60;
    }

    return (uint8_t)second;
}

static uint8_t demo_get_clock_colon_visible(void)
{
    return (uint8_t)((demo_context.status_tick & 1U) == 0U);
}

static uint32_t demo_get_stopwatch_elapsed_ms(void)
{
    uint32_t elapsed_ms = 6U * 60U * 1000U + 18000U;

    elapsed_ms += (uint32_t)demo_context.picker_value[0] * 2300U;
    elapsed_ms += (uint32_t)demo_context.roller_index[0] * 9000U;
    elapsed_ms += (uint32_t)demo_context.quick_matrix_index * 4800U;
    elapsed_ms += (uint32_t)demo_context.export_count * 4200U;
    elapsed_ms += (uint32_t)strlen(demo_context.search_text) * 110U;
    elapsed_ms += (uint32_t)strlen(demo_context.note_text) * 140U;
    elapsed_ms += (uint32_t)demo_context.status_tick * DEMO_STATUS_TIMER_MS;

    return elapsed_ms;
}

static uint8_t demo_get_tileview_index(void)
{
    return (uint8_t)((demo_context.quick_matrix_index + demo_count_enabled_zones() + (demo_context.pin_agv_enabled ? 1U : 0U)) % 4U);
}

static uint8_t demo_get_list_selected_index(void)
{
    if (demo_context.search_text[0] != '\0')
    {
        if (strstr(demo_order_ids[1], demo_context.search_text) != NULL)
        {
            return 0U;
        }
        if (strstr(demo_order_ids[4], demo_context.search_text) != NULL)
        {
            return 1U;
        }
        if (strstr(demo_order_ids[6], demo_context.search_text) != NULL)
        {
            return 2U;
        }
    }

    return (uint8_t)((demo_context.quick_matrix_index + demo_context.export_count) % 3U);
}

static void demo_get_textblock_lines(char *line1, size_t line1_size, char *line2, size_t line2_size)
{
    if (demo_context.note_text[0] != '\0')
    {
        demo_copy_text(line1, line1_size, "Note");
        demo_copy_preview(demo_context.note_text, line2, line2_size);
        return;
    }

    if (demo_context.search_text[0] != '\0')
    {
        demo_copy_text(line1, line1_size, "Search");
        demo_copy_preview(demo_context.search_text, line2, line2_size);
        return;
    }

    demo_copy_text(line1, line1_size, "Flow");
    egui_api_sprintf(line2, "%s %s", demo_get_route_text(), demo_get_dock_text());
}

void demo_get_button_label(uint32_t stable_id, char *buffer, size_t buffer_size)
{
    demo_virtual_node_t *node = demo_find_node(stable_id);

    if (stable_id == DEMO_ACTION_PIN_MIXER_ID)
    {
        demo_copy_text(buffer, buffer_size, demo_context.pin_mixer_enabled ? "Unpin Mix" : "Pin Mix");
        return;
    }
    if (stable_id == DEMO_ACTION_PIN_AGV_ID)
    {
        demo_copy_text(buffer, buffer_size, demo_context.pin_agv_enabled ? "Unpin AGV" : "Pin AGV");
        return;
    }
    if (stable_id == DEMO_ACTION_EXPORT_ID)
    {
        egui_api_sprintf(buffer, "Export %02d", (int)(demo_context.export_count % 100U));
        return;
    }

    if (node != NULL)
    {
        demo_copy_text(buffer, buffer_size, node->title);
    }
    else
    {
        buffer[0] = '\0';
    }
}

uint8_t demo_get_button_style(uint32_t stable_id)
{
    if (stable_id >= DEMO_MACHINE_BASE_ID && stable_id < (DEMO_MACHINE_BASE_ID + 24U))
    {
        if (stable_id == DEMO_MACHINE_AGV_ID)
        {
            if (demo_context.pin_agv_enabled)
            {
                return DEMO_BUTTON_STYLE_TEAL;
            }
            return demo_machine_is_active(stable_id) ? DEMO_BUTTON_STYLE_GREEN : DEMO_BUTTON_STYLE_NEUTRAL;
        }
        if (stable_id == DEMO_MACHINE_MIXER_ID)
        {
            if (demo_context.pin_mixer_enabled)
            {
                return DEMO_BUTTON_STYLE_BLUE;
            }
            return demo_machine_is_active(stable_id) ? DEMO_BUTTON_STYLE_GREEN : DEMO_BUTTON_STYLE_NEUTRAL;
        }
        return demo_machine_is_active(stable_id) ? DEMO_BUTTON_STYLE_GREEN : DEMO_BUTTON_STYLE_NEUTRAL;
    }

    if (stable_id >= DEMO_ZONE_BASE_ID && stable_id < (DEMO_ZONE_BASE_ID + EGUI_ARRAY_SIZE(demo_zone_titles)))
    {
        return demo_context.zone_enabled[stable_id - DEMO_ZONE_BASE_ID] ? DEMO_BUTTON_STYLE_BLUE : DEMO_BUTTON_STYLE_NEUTRAL;
    }

    switch (stable_id)
    {
    case DEMO_ACTION_PIN_MIXER_ID:
        return demo_context.pin_mixer_enabled ? DEMO_BUTTON_STYLE_BLUE : DEMO_BUTTON_STYLE_NEUTRAL;
    case DEMO_ACTION_PIN_AGV_ID:
        return demo_context.pin_agv_enabled ? DEMO_BUTTON_STYLE_TEAL : DEMO_BUTTON_STYLE_NEUTRAL;
    case DEMO_ACTION_SHIFT_A_ID:
        return demo_context.shift_a_enabled ? DEMO_BUTTON_STYLE_GREEN : DEMO_BUTTON_STYLE_NEUTRAL;
    case DEMO_ACTION_SHIFT_B_ID:
        return demo_context.shift_b_enabled ? DEMO_BUTTON_STYLE_GREEN : DEMO_BUTTON_STYLE_NEUTRAL;
    case DEMO_ACTION_EXPORT_ID:
        return DEMO_BUTTON_STYLE_ORANGE;
    case DEMO_ACTION_RESET_ID:
        return DEMO_BUTTON_STYLE_RED;
    default:
        return DEMO_BUTTON_STYLE_NEUTRAL;
    }
}

void demo_get_button_style_colors(uint8_t style, egui_color_t *fill, egui_color_t *border, egui_color_t *text)
{
    switch (style)
    {
    case DEMO_BUTTON_STYLE_BLUE:
        *fill = EGUI_COLOR_HEX(0x3A6EA5);
        *border = EGUI_COLOR_HEX(0x29527C);
        *text = EGUI_COLOR_WHITE;
        break;
    case DEMO_BUTTON_STYLE_TEAL:
        *fill = EGUI_COLOR_HEX(0x167C88);
        *border = EGUI_COLOR_HEX(0x0F616A);
        *text = EGUI_COLOR_WHITE;
        break;
    case DEMO_BUTTON_STYLE_GREEN:
        *fill = EGUI_COLOR_HEX(0x2E8E58);
        *border = EGUI_COLOR_HEX(0x236D44);
        *text = EGUI_COLOR_WHITE;
        break;
    case DEMO_BUTTON_STYLE_ORANGE:
        *fill = EGUI_COLOR_HEX(0xD68A37);
        *border = EGUI_COLOR_HEX(0xA96B23);
        *text = EGUI_COLOR_WHITE;
        break;
    case DEMO_BUTTON_STYLE_RED:
        *fill = EGUI_COLOR_HEX(0xC85C4B);
        *border = EGUI_COLOR_HEX(0x9D4336);
        *text = EGUI_COLOR_WHITE;
        break;
    default:
        *fill = EGUI_COLOR_HEX(0xE8EEF4);
        *border = EGUI_COLOR_HEX(0xC8D4DE);
        *text = DEMO_COLOR_TEXT_PRIMARY;
        break;
    }
}

egui_background_t *demo_get_button_background(uint8_t style)
{
    switch (style)
    {
    case DEMO_BUTTON_STYLE_BLUE:
        return EGUI_BG_OF(&button_blue_bg);
    case DEMO_BUTTON_STYLE_TEAL:
        return EGUI_BG_OF(&button_teal_bg);
    case DEMO_BUTTON_STYLE_GREEN:
        return EGUI_BG_OF(&button_green_bg);
    case DEMO_BUTTON_STYLE_ORANGE:
        return EGUI_BG_OF(&button_orange_bg);
    case DEMO_BUTTON_STYLE_RED:
        return EGUI_BG_OF(&button_red_bg);
    default:
        return EGUI_BG_OF(&button_neutral_bg);
    }
}

static uint8_t demo_order_matches_search(uint8_t index)
{
    const char *search = demo_context.search_text;

    if (search[0] == '\0' || index >= EGUI_ARRAY_SIZE(demo_order_ids))
    {
        return 0;
    }

    return strstr(demo_order_ids[index], search) != NULL ? 1U : 0U;
}

static uint8_t demo_get_machine_load(const demo_virtual_node_t *node)
{
    int16_t load = demo_machine_loads[node->data_index];

    load += (int16_t)((int16_t)demo_context.slider_value[0] - 50) / 3;
    load += (int16_t)((int16_t)demo_context.slider_value[1] - 40) / 6;
    load += (int16_t)demo_get_line_bias();
    load += demo_route_is_fast() ? 4 : 0;
    load += demo_pack_is_bag() ? 2 : 0;
    load += (int16_t)(demo_context.export_count * 2U);

    if (demo_machine_is_pinned(node->desc.stable_id))
    {
        load += 6;
    }
    if (demo_machine_is_active(node->desc.stable_id))
    {
        load += 15;
    }
    if (demo_context.switch_enabled[0])
    {
        load += 3;
    }
    if (demo_context.toggle_enabled[1])
    {
        load -= 4;
    }
    if (node->desc.stable_id == DEMO_MACHINE_AGV_ID && demo_context.pin_agv_enabled)
    {
        load += 6;
    }
    if (node->desc.stable_id == DEMO_MACHINE_QC_ID && demo_context.checkbox_enabled[0])
    {
        load += 4;
    }
    if (load < 8)
    {
        load = 8;
    }
    if (load > 100)
    {
        load = 100;
    }

    return (uint8_t)load;
}

static const char *demo_get_machine_status(const demo_virtual_node_t *node, char *buffer, size_t buffer_size)
{
    if (node->desc.stable_id == DEMO_MACHINE_MIXER_ID)
    {
        if (demo_context.pin_mixer_enabled)
        {
            egui_api_sprintf(buffer, "%s hold", demo_get_recipe_text());
        }
        else if (demo_machine_is_active(node->desc.stable_id))
        {
            egui_api_sprintf(buffer, "%s b%d", demo_get_recipe_text(), (int)demo_context.picker_value[0]);
        }
        else if (demo_context.switch_enabled[0])
        {
            egui_api_sprintf(buffer, "Loop %s", demo_get_recipe_text());
        }
        else
        {
            egui_api_sprintf(buffer, "Mix %s", demo_get_recipe_text());
        }
        return buffer;
    }

    if (node->desc.stable_id == DEMO_MACHINE_AGV_ID)
    {
        if (demo_context.pin_agv_enabled)
        {
            egui_api_sprintf(buffer, "Dock %s", demo_get_dock_text());
        }
        else if (demo_context.radio_selected_index == 1U)
        {
            egui_api_sprintf(buffer, "Hand %s", demo_get_dock_text());
        }
        else
        {
            egui_api_sprintf(buffer, "%s %s", demo_get_line_text(), demo_get_route_text());
        }
        return buffer;
    }

    if (node->desc.stable_id == DEMO_MACHINE_PACK_ID)
    {
        egui_api_sprintf(buffer, "%s %s", demo_get_pack_text(), demo_context.toggle_enabled[0] ? "trace" : "pack");
        return buffer;
    }

    if (node->desc.stable_id == DEMO_MACHINE_QC_ID)
    {
        demo_copy_text(buffer, buffer_size,
                       demo_context.checkbox_enabled[0] ? "QA audit" : (demo_machine_is_active(node->desc.stable_id) ? "Line chk" : "Vision idle"));
        return buffer;
    }

    return demo_machine_statuses[node->data_index];
}

static void demo_init_node(demo_virtual_node_t *node, uint8_t kind, uint32_t stable_id, int32_t x, int32_t y, int32_t width, int32_t height, int16_t z_order,
                           uint16_t view_type, uint8_t flags, uint8_t accent, uint8_t data_index, const char *title)
{
    memset(node, 0, sizeof(*node));
    node->kind = kind;
    node->accent = accent;
    node->data_index = data_index;
    node->desc.region.location.x = (egui_dim_t)x;
    node->desc.region.location.y = (egui_dim_t)y;
    node->desc.region.size.width = (egui_dim_t)width;
    node->desc.region.size.height = (egui_dim_t)height;
    node->desc.stable_id = stable_id;
    node->desc.view_type = view_type;
    node->desc.z_order = z_order;
    node->desc.flags = flags;
    demo_copy_text(node->title, sizeof(node->title), title);
}

static void demo_init_nodes(void)
{
    uint32_t cursor = 0;
    uint32_t i;

    memset(&demo_context, 0, sizeof(demo_context));
    demo_set_business_defaults();
    demo_log_event("Init 100 nodes");

    for (i = 0; i < 4; i++)
    {
        static const egui_region_t panel_regions[4] = {
                {{DEMO_OVERVIEW_X, DEMO_OVERVIEW_Y}, {DEMO_OVERVIEW_W, DEMO_OVERVIEW_H}},
                {{DEMO_FLOOR_X, DEMO_FLOOR_Y}, {DEMO_FLOOR_W, DEMO_FLOOR_H}},
                {{DEMO_OPS_X, DEMO_OPS_Y}, {DEMO_OPS_W, DEMO_OPS_H}},
                {{DEMO_ALERT_X, DEMO_ALERT_Y}, {DEMO_ALERT_W, DEMO_ALERT_H}},
        };

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_PANEL, DEMO_PANEL_BASE_ID + i, panel_regions[i].location.x, panel_regions[i].location.y,
                       panel_regions[i].size.width, panel_regions[i].size.height, (int16_t)(i + 1), EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, (uint8_t)i,
                       (uint8_t)i, demo_panel_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_kpi_titles); i++)
    {
        uint32_t row = i / 4U;
        uint32_t col = i % 4U;
        uint8_t kind = DEMO_NODE_KIND_KPI;
        const char *title = demo_kpi_titles[i];
        uint8_t data_index = (uint8_t)i;
        int32_t x = DEMO_KPI_START_X + (int32_t)col * (DEMO_KPI_W + DEMO_KPI_GAP_X);
        int32_t y = DEMO_KPI_START_Y + (int32_t)row * (DEMO_KPI_H + DEMO_KPI_GAP_Y);

        if (i == 4U)
        {
            kind = DEMO_NODE_KIND_TILEVIEW;
            title = "Tiles";
            data_index = 0U;
        }
        else if (i == 5U)
        {
            kind = DEMO_NODE_KIND_WINDOW;
            title = "Window";
            data_index = 1U;
        }
        else if (i == 6U)
        {
            kind = DEMO_NODE_KIND_LIST;
            title = "List";
            data_index = 2U;
        }
        else if (i == 7U)
        {
            kind = DEMO_NODE_KIND_TEXTBLOCK;
            title = "Text";
            data_index = 3U;
        }

        demo_init_node(&demo_context.nodes[cursor++], kind, DEMO_KPI_BASE_ID + i, x, y, DEMO_KPI_W, DEMO_KPI_H, (int16_t)(20 + i),
                       EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, (uint8_t)i, data_index, title);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_schedule_titles); i++)
    {
        uint32_t row = i / 6U;
        uint32_t col = i % 6U;
        uint8_t kind = DEMO_NODE_KIND_SCHEDULE;
        const char *title = demo_schedule_titles[i];
        int32_t x = DEMO_SCHEDULE_START_X + (int32_t)col * (DEMO_SCHEDULE_W + DEMO_SCHEDULE_GAP_X);
        int32_t y = DEMO_SCHEDULE_START_Y + (int32_t)row * (DEMO_SCHEDULE_H + DEMO_SCHEDULE_GAP_Y);

        if (i == 0U)
        {
            kind = DEMO_NODE_KIND_DIGITAL_CLOCK;
            title = "Clock";
        }
        else if (i == 2U)
        {
            kind = DEMO_NODE_KIND_HEART_RATE;
            title = "Heart";
        }
        else if (i == 3U)
        {
            kind = DEMO_NODE_KIND_CHART_SCATTER;
            title = "Scatter";
        }
        else if (i == 4U)
        {
            kind = DEMO_NODE_KIND_CHIPS;
        }
        else if (i == 5U)
        {
            kind = DEMO_NODE_KIND_BADGE_GROUP;
        }
        else if (i == 6U)
        {
            kind = DEMO_NODE_KIND_PAGE_INDICATOR;
        }
        else if (i == 7U)
        {
            kind = DEMO_NODE_KIND_DIVIDER;
        }
        else if (i == 8U)
        {
            kind = DEMO_NODE_KIND_SCALE;
        }
        else if (i == 9U)
        {
            kind = DEMO_NODE_KIND_CHART_LINE;
        }
        else if (i == 10U)
        {
            kind = DEMO_NODE_KIND_TAB_BAR;
        }
        else if (i == 11U)
        {
            kind = DEMO_NODE_KIND_MENU;
        }

        demo_init_node(&demo_context.nodes[cursor++], kind, DEMO_SCHEDULE_BASE_ID + i, x, y, DEMO_SCHEDULE_W, DEMO_SCHEDULE_H, (int16_t)(40 + i),
                       EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, (uint8_t)i, (uint8_t)i, title);
    }

    for (i = 0; i < 16U; i++)
    {
        uint32_t row = i / 4U;
        uint32_t col = i % 4U;
        uint32_t stable_id = DEMO_MACHINE_BASE_ID + i;
        uint16_t view_type = EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE;
        uint8_t flags = 0;
        int32_t x = DEMO_MACHINE_START_X + (int32_t)col * (DEMO_MACHINE_W + DEMO_MACHINE_GAP_X);
        int32_t y = DEMO_MACHINE_START_Y + (int32_t)row * (DEMO_MACHINE_H + DEMO_MACHINE_GAP_Y);

        if (stable_id == DEMO_MACHINE_MIXER_ID || stable_id == DEMO_MACHINE_AGV_ID || stable_id == DEMO_MACHINE_PACK_ID || stable_id == DEMO_MACHINE_QC_ID)
        {
            view_type = DEMO_VIEW_TYPE_BUTTON;
            flags = EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE;
        }

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_MACHINE, stable_id, x, y, DEMO_MACHINE_W, DEMO_MACHINE_H, (int16_t)(70 + i), view_type,
                       flags, (uint8_t)i, (uint8_t)i, demo_machine_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_sensor_titles); i++)
    {
        uint32_t row = i / 3U;
        uint32_t col = i % 3U;
        uint8_t kind = DEMO_NODE_KIND_SENSOR;
        const char *title = demo_sensor_titles[i];
        int32_t x = DEMO_SENSOR_START_X + (int32_t)col * (DEMO_SENSOR_W + DEMO_SENSOR_GAP_X);
        int32_t y = DEMO_SENSOR_START_Y + (int32_t)row * (DEMO_SENSOR_H + DEMO_SENSOR_GAP_Y);

        if (i == 0U)
        {
            kind = DEMO_NODE_KIND_CHART_BAR;
            title = "Load";
        }
        else if (i == 2U)
        {
            kind = DEMO_NODE_KIND_LINE;
        }
        else if (i == 3U)
        {
            kind = DEMO_NODE_KIND_LED;
        }
        else if (i == 4U)
        {
            kind = DEMO_NODE_KIND_SPINNER;
        }
        else if (i == 5U)
        {
            kind = DEMO_NODE_KIND_NOTIFICATION_BADGE;
        }

        demo_init_node(&demo_context.nodes[cursor++], kind, DEMO_SENSOR_BASE_ID + i, x, y, DEMO_SENSOR_W, DEMO_SENSOR_H, (int16_t)(110 + i),
                       EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, (uint8_t)(i + 1U), (uint8_t)i, title);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_zone_titles); i++)
    {
        uint32_t row = i / 3U;
        uint32_t col = i % 3U;
        int32_t x = DEMO_ZONE_START_X + (int32_t)col * (DEMO_ZONE_W + DEMO_ZONE_GAP_X);
        int32_t y = DEMO_ZONE_START_Y + (int32_t)row * (DEMO_ZONE_H + DEMO_ZONE_GAP_Y);

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_ZONE_BUTTON, DEMO_ZONE_BASE_ID + i, x, y, DEMO_ZONE_W, DEMO_ZONE_H, (int16_t)(130 + i),
                       DEMO_VIEW_TYPE_BUTTON, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, (uint8_t)(i + 2U), (uint8_t)i, demo_zone_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_order_ids); i++)
    {
        uint32_t row = i / 4U;
        uint32_t col = i % 4U;
        int32_t x = DEMO_ORDER_START_X + (int32_t)col * (DEMO_ORDER_W + DEMO_ORDER_GAP_X);
        int32_t y = DEMO_ORDER_START_Y + (int32_t)row * (DEMO_ORDER_H + DEMO_ORDER_GAP_Y);

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_ORDER, DEMO_ORDER_BASE_ID + i, x, y, DEMO_ORDER_W, DEMO_ORDER_H, (int16_t)(150 + i),
                       EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, (uint8_t)(i + 3U), (uint8_t)i, demo_order_ids[i]);
    }

    for (i = 0; i < 6; i++)
    {
        static const char *action_titles[6] = {"Pin Mix", "Pin AGV", "Shift A", "Shift B", "Export", "Reset"};
        uint32_t row = i / 3U;
        uint32_t col = i % 3U;
        int32_t x = DEMO_ACTION_START_X + (int32_t)col * (DEMO_ACTION_W + DEMO_ACTION_GAP_X);
        int32_t y = DEMO_ACTION_START_Y + (int32_t)row * (DEMO_ACTION_H + DEMO_ACTION_GAP_Y);

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_ACTION_BUTTON, DEMO_ACTION_BASE_ID + i, x, y, DEMO_ACTION_W, DEMO_ACTION_H,
                       (int16_t)(170 + i), DEMO_VIEW_TYPE_BUTTON, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, (uint8_t)(i + 1U), (uint8_t)i, action_titles[i]);
    }

    demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_TEXTINPUT, DEMO_SEARCH_INPUT_ID, DEMO_SEARCH_INPUT_X, DEMO_SEARCH_INPUT_Y, DEMO_INPUT_W,
                   DEMO_INPUT_H, 190, DEMO_VIEW_TYPE_TEXTINPUT, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, 0, 0, "Search");
    demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_TEXTINPUT, DEMO_NOTE_INPUT_ID, DEMO_NOTE_INPUT_X, DEMO_NOTE_INPUT_Y, DEMO_INPUT_W,
                   DEMO_INPUT_H, 191, DEMO_VIEW_TYPE_TEXTINPUT, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, 1, 1, "Note");

    for (i = 0; i < DEMO_ALERT_RENDER_COUNT; i++)
    {
        uint32_t row = i / 3U;
        uint32_t col = i % 3U;
        uint8_t kind = DEMO_NODE_KIND_ALERT;
        const char *title = demo_alert_titles[i];
        int32_t x = DEMO_ALERT_CARD_START_X + (int32_t)col * (DEMO_ALERT_CARD_W + DEMO_ALERT_CARD_GAP_X);
        int32_t y = DEMO_ALERT_CARD_START_Y + (int32_t)row * (DEMO_ALERT_CARD_H + DEMO_ALERT_CARD_GAP_Y);

        if (i == 0U)
        {
            kind = DEMO_NODE_KIND_ANALOG_CLOCK;
            title = "Clock";
        }
        else if (i == 1U)
        {
            kind = DEMO_NODE_KIND_COMPASS;
            title = "Compass";
        }
        else if (i == 3U)
        {
            kind = DEMO_NODE_KIND_ACTIVITY_RING;
            title = "Activity";
        }
        else if (i == 4U)
        {
            kind = DEMO_NODE_KIND_GAUGE;
            title = "Gauge";
        }
        else if (i == 5U)
        {
            kind = DEMO_NODE_KIND_CIRCULAR_PROGRESS;
            title = "Cycle";
        }
        else if (i == 6U)
        {
            kind = DEMO_NODE_KIND_MINI_CALENDAR;
            title = "Calendar";
        }
        else if (i == 7U)
        {
            kind = DEMO_NODE_KIND_BREADCRUMB_BAR;
            title = "Flow Path";
        }
        else if (i == 8U)
        {
            kind = DEMO_NODE_KIND_MESSAGE_BAR;
            title = "Message";
        }

        demo_init_node(&demo_context.nodes[cursor++], kind, DEMO_ALERT_BASE_ID + i, x, y, DEMO_ALERT_CARD_W, DEMO_ALERT_CARD_H, (int16_t)(210 + i),
                       EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, (uint8_t)i, (uint8_t)i, title);
    }

    demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_CHART_PIE, DEMO_TABLE_SUMMARY_ID, DEMO_ALERT_AUX_X, DEMO_ALERT_TABLE_Y, DEMO_ALERT_CARD_W,
                   DEMO_ALERT_CARD_H, 219, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, 3U, 0U, "Mix");
    demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_BUTTON_MATRIX, DEMO_QUICK_MATRIX_ID, DEMO_ALERT_AUX_X, DEMO_ALERT_MATRIX_Y, DEMO_ALERT_CARD_W,
                   DEMO_ALERT_CARD_H, 220, DEMO_VIEW_TYPE_BUTTON_MATRIX, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, 1U, 0U, "Quick Ops");
    demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_STOPWATCH, DEMO_SPANGROUP_NOTE_ID, DEMO_ALERT_AUX_X, DEMO_ALERT_SPAN_Y, DEMO_ALERT_CARD_W,
                   DEMO_ALERT_CARD_H, 221, EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, 2U, 0U, "Timer");

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_switch_titles); i++)
    {
        int32_t x = DEMO_FLOOR_WIDGET_START_X + (int32_t)i * (DEMO_FLOOR_WIDGET_W + DEMO_FLOOR_WIDGET_GAP_X);
        int32_t y = DEMO_FLOOR_WIDGET_START_Y;

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_SWITCH, DEMO_SWITCH_BASE_ID + i, x, y, DEMO_FLOOR_WIDGET_W, DEMO_FLOOR_WIDGET_H,
                       (int16_t)(260 + i), DEMO_VIEW_TYPE_SWITCH, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, (uint8_t)i, (uint8_t)i, demo_switch_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_checkbox_titles); i++)
    {
        int32_t x = DEMO_FLOOR_WIDGET_START_X + (int32_t)i * (DEMO_FLOOR_WIDGET_W + DEMO_FLOOR_WIDGET_GAP_X);
        int32_t y = DEMO_FLOOR_WIDGET_START_Y + DEMO_FLOOR_WIDGET_H + DEMO_FLOOR_WIDGET_GAP_Y;

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_CHECKBOX, DEMO_CHECKBOX_BASE_ID + i, x, y, DEMO_FLOOR_WIDGET_W, DEMO_FLOOR_WIDGET_H,
                       (int16_t)(270 + i), DEMO_VIEW_TYPE_CHECKBOX, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, (uint8_t)(i + 1U), (uint8_t)i,
                       demo_checkbox_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_radio_titles); i++)
    {
        int32_t x = DEMO_FLOOR_WIDGET_START_X + (int32_t)i * (DEMO_FLOOR_WIDGET_W + DEMO_FLOOR_WIDGET_GAP_X);
        int32_t y = DEMO_FLOOR_WIDGET_START_Y + 2 * (DEMO_FLOOR_WIDGET_H + DEMO_FLOOR_WIDGET_GAP_Y);

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_RADIO, DEMO_RADIO_BASE_ID + i, x, y, DEMO_FLOOR_WIDGET_W, DEMO_FLOOR_WIDGET_H,
                       (int16_t)(280 + i), DEMO_VIEW_TYPE_RADIO, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, (uint8_t)(i + 2U), (uint8_t)i, demo_radio_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_combobox_titles); i++)
    {
        int32_t x = DEMO_COMBO_START_X + (int32_t)i * (DEMO_COMBO_W + DEMO_FLOOR_WIDGET_GAP_X);

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_COMBOBOX, DEMO_COMBOBOX_BASE_ID + i, x, DEMO_COMBO_START_Y, DEMO_COMBO_W, DEMO_COMBO_H,
                       (int16_t)(320 + i), DEMO_VIEW_TYPE_COMBOBOX, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, (uint8_t)(i + 3U), (uint8_t)i,
                       demo_combobox_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_segment_titles); i++)
    {
        int32_t x = DEMO_SEGMENT_START_X + (int32_t)i * (DEMO_SEGMENT_W + DEMO_FLOOR_WIDGET_GAP_X);

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_SEGMENTED, DEMO_SEGMENT_BASE_ID + i, x, DEMO_SEGMENT_START_Y, DEMO_SEGMENT_W,
                       DEMO_SEGMENT_H, (int16_t)(294 + i), DEMO_VIEW_TYPE_SEGMENTED, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, (uint8_t)(i + 1U), (uint8_t)i,
                       demo_segment_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_progress_titles); i++)
    {
        int32_t x = DEMO_PROGRESS_START_X + (int32_t)i * (DEMO_PROGRESS_W + DEMO_FLOOR_WIDGET_GAP_X);

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_PROGRESS, DEMO_PROGRESS_BASE_ID + i, x, DEMO_PROGRESS_START_Y, DEMO_PROGRESS_W,
                       DEMO_PROGRESS_H, (int16_t)(296 + i), EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE, 0, (uint8_t)(i + 2U), (uint8_t)i, demo_progress_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_roller_titles); i++)
    {
        int32_t x = DEMO_ROLLER_START_X + (int32_t)i * (DEMO_ROLLER_W + DEMO_FLOOR_WIDGET_GAP_X);

        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_ROLLER, DEMO_ROLLER_BASE_ID + i, x, DEMO_ROLLER_START_Y, DEMO_ROLLER_W, DEMO_ROLLER_H,
                       (int16_t)(298 + i), DEMO_VIEW_TYPE_ROLLER, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, (uint8_t)(i + 3U), (uint8_t)i,
                       demo_roller_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_slider_titles); i++)
    {
        int32_t x = DEMO_SLIDER_START_X + (int32_t)i * (DEMO_RACK_WIDGET_W + DEMO_RACK_WIDGET_GAP_X);
        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_SLIDER, DEMO_SLIDER_BASE_ID + i, x, DEMO_SLIDER_START_Y, DEMO_RACK_WIDGET_W,
                       DEMO_RACK_WIDGET_H, (int16_t)(290 + i), DEMO_VIEW_TYPE_SLIDER, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, (uint8_t)(i + 1U), (uint8_t)i,
                       demo_slider_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_toggle_titles); i++)
    {
        int32_t x = DEMO_TOGGLE_START_X + (int32_t)i * (DEMO_RACK_WIDGET_W + DEMO_RACK_WIDGET_GAP_X);
        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_TOGGLE, DEMO_TOGGLE_BASE_ID + i, x, DEMO_TOGGLE_START_Y, DEMO_RACK_WIDGET_W,
                       DEMO_RACK_WIDGET_H, (int16_t)(300 + i), DEMO_VIEW_TYPE_TOGGLE, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, (uint8_t)(i + 2U), (uint8_t)i,
                       demo_toggle_titles[i]);
    }

    for (i = 0; i < EGUI_ARRAY_SIZE(demo_picker_titles); i++)
    {
        int32_t x = DEMO_PICKER_START_X + (int32_t)i * (DEMO_PICKER_W + DEMO_RACK_WIDGET_GAP_X);
        demo_init_node(&demo_context.nodes[cursor++], DEMO_NODE_KIND_NUMBER_PICKER, DEMO_PICKER_BASE_ID + i, x, DEMO_PICKER_START_Y, DEMO_PICKER_W,
                       DEMO_PICKER_H, (int16_t)(310 + i), DEMO_VIEW_TYPE_NUMBER_PICKER, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE, (uint8_t)(i + 3U), (uint8_t)i,
                       demo_picker_titles[i]);
    }

    EGUI_ASSERT(cursor == DEMO_NODE_COUNT);
}

static void demo_get_machine_card_colors(const demo_virtual_node_t *node, egui_color_t *fill, egui_color_t *border)
{
    if (demo_machine_is_pinned(node->desc.stable_id))
    {
        if (node->desc.stable_id == DEMO_MACHINE_AGV_ID)
        {
            *fill = EGUI_COLOR_HEX(0xDDF6F7);
            *border = EGUI_COLOR_HEX(0x2D949B);
        }
        else
        {
            *fill = EGUI_COLOR_HEX(0xDDEAFE);
            *border = EGUI_COLOR_HEX(0x5C90E4);
        }
        return;
    }

    if (demo_machine_is_active(node->desc.stable_id))
    {
        *fill = EGUI_COLOR_HEX(0xE4F7E7);
        *border = EGUI_COLOR_HEX(0x58B06B);
        return;
    }

    demo_get_accent_colors(node->accent, fill, border);
}

static int demo_page_touch_cb(egui_view_t *self, egui_motion_event_t *event)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_t *focused;
#endif

    if (event->type != EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        return 0;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    focused = egui_focus_manager_get_focused_view();
    if (focused != NULL && !egui_region_pt_in_rect(&focused->region_screen, event->location.x, event->location.y))
    {
        demo_log_event("Focus clear");
        egui_focus_manager_clear_focus();
        egui_view_virtual_stage_notify_data_changed(self);
        self->api->calculate_layout(self);
        demo_refresh_status();
    }
#else
    EGUI_UNUSED(self);
#endif

    return 0;
}

static void demo_page_click_cb(egui_view_t *self)
{
    EGUI_UNUSED(self);
    demo_refresh_status();
}

void demo_textinput_changed(egui_view_t *self, const char *text)
{
    demo_virtual_textinput_view_t *textinput_view = (demo_virtual_textinput_view_t *)self;
    char preview[18];

    demo_set_input_text(textinput_view->stable_id, text);
    demo_copy_preview(text, preview, sizeof(preview));
    demo_log_event("%s %s", textinput_view->stable_id == DEMO_SEARCH_INPUT_ID ? "Search" : "Note", preview);
    demo_notify_text_dependents(textinput_view->stable_id);
    demo_refresh_status();
}

void demo_textinput_focus_changed(egui_view_t *self, int is_focused)
{
    egui_view_textinput_t *textinput = (egui_view_textinput_t *)self;

    if (is_focused)
    {
        textinput->cursor_visible = 1;
        egui_timer_start_timer(&textinput->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);
        egui_view_set_background(self, demo_get_textinput_background(1U));
    }
    else
    {
        textinput->cursor_visible = 0;
        egui_timer_stop_timer(&textinput->cursor_timer);
        egui_view_set_background(self, demo_get_textinput_background(0U));
    }

    egui_view_invalidate(self);
    EGUI_VIEW_VIRTUAL_STAGE_REQUEST_LAYOUT(&virtual_stage);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_TABLE_SUMMARY_ID);
    demo_refresh_status();
}

static void demo_widget_state_changed(void)
{
    demo_notify_operational_nodes();
    demo_refresh_status();
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void demo_clear_foreign_focus(egui_view_t *self)
{
    egui_view_t *focused = egui_focus_manager_get_focused_view();

    if (focused != NULL && focused != self)
    {
        egui_focus_manager_clear_focus();
    }
}
#else
static void demo_clear_foreign_focus(egui_view_t *self)
{
    EGUI_UNUSED(self);
}
#endif

void demo_switch_changed(egui_view_t *self, int is_checked)
{
    demo_virtual_switch_view_t *switch_view = (demo_virtual_switch_view_t *)self;
    int index = demo_get_switch_index(switch_view->stable_id);

    demo_clear_foreign_focus(self);

    if (index >= 0)
    {
        demo_context.switch_enabled[index] = (uint8_t)is_checked;
        demo_log_event("%s %s", demo_switch_titles[index], is_checked ? "on" : "off");
        demo_widget_state_changed();
    }
}

void demo_checkbox_changed(egui_view_t *self, int is_checked)
{
    demo_virtual_checkbox_view_t *checkbox_view = (demo_virtual_checkbox_view_t *)self;
    int index = demo_get_checkbox_index(checkbox_view->stable_id);

    demo_clear_foreign_focus(self);

    if (index >= 0)
    {
        demo_context.checkbox_enabled[index] = (uint8_t)is_checked;
        demo_log_event("%s %s", demo_checkbox_titles[index], is_checked ? "on" : "off");
        demo_widget_state_changed();
    }
}

void demo_radio_click_cb(egui_view_t *self)
{
    demo_virtual_radio_view_t *radio_view = (demo_virtual_radio_view_t *)self;
    int index = demo_get_radio_index(radio_view->stable_id);

    demo_clear_foreign_focus(self);

    if (index >= 0)
    {
        demo_context.radio_selected_index = (uint8_t)index;
        demo_log_event("Mode %s", demo_radio_titles[index]);
        demo_widget_state_changed();
    }
}

void demo_slider_changed(egui_view_t *self, uint8_t value)
{
    demo_virtual_slider_view_t *slider_view = (demo_virtual_slider_view_t *)self;
    int index = demo_get_slider_index(slider_view->stable_id);

    demo_clear_foreign_focus(self);

    if (index >= 0)
    {
        demo_context.slider_value[index] = value;
        demo_log_event("%s %d", demo_slider_titles[index], (int)value);
        demo_widget_state_changed();
    }
}

void demo_toggle_changed(egui_view_t *self, uint8_t is_toggled)
{
    demo_virtual_toggle_view_t *toggle_view = (demo_virtual_toggle_view_t *)self;
    int index = demo_get_toggle_index(toggle_view->stable_id);

    demo_clear_foreign_focus(self);

    if (index >= 0)
    {
        demo_context.toggle_enabled[index] = is_toggled;
        egui_view_toggle_button_set_icon(self, demo_get_toggle_icon(toggle_view->stable_id));
        egui_view_toggle_button_set_icon_font(self, EGUI_FONT_ICON_MS_16);
        demo_log_event("%s %s", demo_toggle_titles[index], is_toggled ? "on" : "off");
        demo_widget_state_changed();
    }
}

void demo_picker_changed(egui_view_t *self, int16_t value)
{
    demo_virtual_picker_view_t *picker_view = (demo_virtual_picker_view_t *)self;
    int index = demo_get_picker_index(picker_view->stable_id);

    demo_clear_foreign_focus(self);

    if (index >= 0)
    {
        demo_context.picker_value[index] = value;
        demo_log_event("%s %d", demo_picker_titles[index], (int)value);
        demo_widget_state_changed();
    }
}

void demo_combobox_selected(egui_view_t *self, uint8_t index)
{
    demo_virtual_combobox_view_t *combobox_view = (demo_virtual_combobox_view_t *)self;
    int combo_index = demo_get_combobox_index(combobox_view->stable_id);

    if (combo_index >= 0)
    {
        egui_view_combobox_collapse(self);
        egui_view_clear_focus(self);
        demo_context.combobox_index[combo_index] = index;
        demo_log_event("%s %s", demo_combobox_titles[combo_index], demo_get_combobox_value(combobox_view->stable_id));
        demo_widget_state_changed();
    }
}

void demo_roller_selected(egui_view_t *self, uint8_t index)
{
    demo_virtual_roller_view_t *roller_view = (demo_virtual_roller_view_t *)self;
    int roller_index = demo_get_roller_index(roller_view->stable_id);

    demo_clear_foreign_focus(self);

    if (roller_index >= 0)
    {
        demo_context.roller_index[roller_index] = index;
        demo_log_event("%s %s", demo_roller_titles[roller_index], demo_get_roller_value(roller_view->stable_id));
        demo_widget_state_changed();
    }
}

void demo_segment_changed(egui_view_t *self, uint8_t index)
{
    demo_virtual_segmented_view_t *segmented_view = (demo_virtual_segmented_view_t *)self;
    int segment_index = demo_get_segment_index(segmented_view->stable_id);

    demo_clear_foreign_focus(self);

    if (segment_index >= 0)
    {
        demo_context.segment_index[segment_index] = index;
        demo_log_event("%s %s", demo_segment_titles[segment_index], demo_get_segment_value(segmented_view->stable_id));
        demo_widget_state_changed();
    }
}

void demo_button_matrix_clicked(egui_view_t *self, uint8_t btn_index)
{
    demo_virtual_button_matrix_view_t *matrix_view = (demo_virtual_button_matrix_view_t *)self;

    EGUI_UNUSED(matrix_view);
    demo_clear_foreign_focus(self);

    if (btn_index < EGUI_ARRAY_SIZE(demo_quick_matrix_titles))
    {
        demo_context.quick_matrix_index = btn_index;
        demo_log_event("Quick %s", demo_quick_matrix_titles[btn_index]);
        demo_widget_state_changed();
    }
}

static void demo_toggle_shift(uint32_t stable_id)
{
    if (stable_id == DEMO_ACTION_SHIFT_A_ID)
    {
        if (demo_context.shift_a_enabled)
        {
            demo_context.shift_a_enabled = 0;
        }
        else
        {
            demo_context.shift_a_enabled = 1;
            demo_context.shift_b_enabled = 0;
        }
    }
    else if (stable_id == DEMO_ACTION_SHIFT_B_ID)
    {
        if (demo_context.shift_b_enabled)
        {
            demo_context.shift_b_enabled = 0;
        }
        else
        {
            demo_context.shift_b_enabled = 1;
            demo_context.shift_a_enabled = 0;
        }
    }

    demo_log_event("Shift %s", demo_context.shift_a_enabled ? "A" : (demo_context.shift_b_enabled ? "B" : "-"));
    demo_notify_shift_nodes();
}

void demo_button_click_cb(egui_view_t *self)
{
    demo_virtual_button_view_t *button_view = (demo_virtual_button_view_t *)self;
    int machine_index;

    demo_clear_foreign_focus(self);

    if (button_view->stable_id == DEMO_ACTION_PIN_MIXER_ID)
    {
        demo_toggle_pin(DEMO_MACHINE_MIXER_ID, DEMO_ACTION_PIN_MIXER_ID, &demo_context.pin_mixer_enabled);
    }
    else if (button_view->stable_id == DEMO_ACTION_PIN_AGV_ID)
    {
        demo_toggle_pin(DEMO_MACHINE_AGV_ID, DEMO_ACTION_PIN_AGV_ID, &demo_context.pin_agv_enabled);
    }
    else if (button_view->stable_id == DEMO_ACTION_SHIFT_A_ID || button_view->stable_id == DEMO_ACTION_SHIFT_B_ID)
    {
        demo_toggle_shift(button_view->stable_id);
    }
    else if (button_view->stable_id == DEMO_ACTION_EXPORT_ID)
    {
        demo_context.export_count++;
        demo_log_event("Export %02d", (int)(demo_context.export_count % 100U));
        demo_notify_operational_nodes();
        EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_ACTION_EXPORT_ID);
    }
    else if (button_view->stable_id == DEMO_ACTION_RESET_ID)
    {
        demo_reset_business_state();
    }
    else if (button_view->stable_id >= DEMO_ZONE_BASE_ID && button_view->stable_id < (DEMO_ZONE_BASE_ID + EGUI_ARRAY_SIZE(demo_zone_titles)))
    {
        demo_context.zone_enabled[button_view->stable_id - DEMO_ZONE_BASE_ID] = (uint8_t)!demo_context.zone_enabled[button_view->stable_id - DEMO_ZONE_BASE_ID];
        demo_log_event("%s %s", demo_get_node_title(button_view->stable_id),
                       demo_context.zone_enabled[button_view->stable_id - DEMO_ZONE_BASE_ID] ? "on" : "off");
        demo_notify_zone_nodes(button_view->stable_id);
    }
    else
    {
        machine_index = demo_get_machine_active_index(button_view->stable_id);
        if (machine_index >= 0)
        {
            demo_context.machine_active[machine_index] = (uint8_t)!demo_context.machine_active[machine_index];
            demo_log_event("%s %s", demo_get_node_title(button_view->stable_id), demo_context.machine_active[machine_index] ? "busy" : "idle");
            demo_notify_operational_nodes();
        }
    }

    demo_refresh_status();
}

static void demo_toggle_pin(uint32_t stable_id, uint32_t action_id, uint8_t *enabled_flag)
{
    if (*enabled_flag)
    {
        EGUI_VIEW_VIRTUAL_STAGE_UNPIN(&virtual_stage, stable_id);
        *enabled_flag = 0;
    }
    else if (EGUI_VIEW_VIRTUAL_STAGE_PIN(&virtual_stage, stable_id))
    {
        *enabled_flag = 1;
    }

    demo_log_event("%s %s", demo_get_node_title(stable_id), *enabled_flag ? "pin" : "release");
    demo_notify_operational_nodes();
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, action_id);
    EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(&virtual_stage);
}

static void demo_reset_business_state(void)
{
    egui_view_t *search_view = demo_find_live_view(DEMO_SEARCH_INPUT_ID);
    egui_view_t *note_view = demo_find_live_view(DEMO_NOTE_INPUT_ID);

    if (search_view != NULL)
    {
        egui_view_textinput_set_text(search_view, "");
    }
    if (note_view != NULL)
    {
        egui_view_textinput_set_text(note_view, "");
    }

    memset(demo_context.machine_active, 0, sizeof(demo_context.machine_active));
    memset(demo_context.zone_enabled, 0, sizeof(demo_context.zone_enabled));
    demo_context.pin_mixer_enabled = 0;
    demo_context.pin_agv_enabled = 0;
    demo_context.shift_a_enabled = 0;
    demo_context.shift_b_enabled = 0;
    demo_context.export_count = 0;
    demo_context.status_tick = 0;
    demo_context.search_text[0] = '\0';
    demo_context.note_text[0] = '\0';
    demo_set_business_defaults();
    demo_log_event("Reset defaults");

    EGUI_VIEW_VIRTUAL_STAGE_UNPIN(&virtual_stage, DEMO_MACHINE_MIXER_ID);
    EGUI_VIEW_VIRTUAL_STAGE_UNPIN(&virtual_stage, DEMO_MACHINE_AGV_ID);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_focus_manager_clear_focus();
#endif
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_DATA(&virtual_stage);
    EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(&virtual_stage);
    demo_refresh_status();
}

static void demo_draw_panel_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_color_t accent_fill;
    egui_color_t accent_border;
    egui_region_t title_region;
    egui_region_t subtitle_region;
    int panel_index = node->data_index;

    demo_get_accent_colors(node->accent, &accent_fill, &accent_border);
    demo_draw_round_card(region, 18, DEMO_COLOR_PANEL_FILL, DEMO_COLOR_PANEL_BORDER);
    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, 6, 3, accent_border, EGUI_ALPHA_100);

    demo_region_inset(region, &title_region, 12, 10, 12, region->size.height - 28);
    demo_region_inset(region, &subtitle_region, 12, 28, 12, region->size.height - 40);

    egui_canvas_draw_text_in_rect(DEMO_FONT_TITLE, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_panel_subtitles[panel_index], &subtitle_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED,
                                  EGUI_ALPHA_100);
}

static void demo_draw_kpi_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_color_t fill;
    egui_color_t border;
    egui_region_t title_region;
    egui_region_t delta_region;
    egui_region_t value_region;
    char value_text[16];
    char delta_text[16];

    demo_get_accent_colors(node->accent, &fill, &border);
    demo_draw_round_card(region, 10, EGUI_COLOR_WHITE, border);
    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, 4, 2, border, EGUI_ALPHA_100);

    demo_region_inset(region, &title_region, 8, 5, 40, 16);
    demo_region_inset(region, &delta_region, region->size.width - 38, 5, 6, 16);
    demo_region_inset(region, &value_region, 8, 15, 8, 4);
    demo_get_kpi_value_text(node->data_index, value_text, sizeof(value_text));
    demo_get_kpi_delta_text(node->data_index, delta_text, sizeof(delta_text));

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, delta_text, &delta_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, border, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_BUTTON, value_text, &value_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
}

static void demo_draw_tileview_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    uint8_t active_index = demo_get_tileview_index();
    int32_t grid_x = region->location.x + 40;
    int32_t grid_y = region->location.y + 6;
    int32_t cell_w = 28;
    int32_t cell_h = 8;
    uint8_t i;

    demo_draw_round_card(region, 10, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 8, 4, 64, 16);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);

    for (i = 0; i < 4U; i++)
    {
        uint8_t is_active = i == active_index;
        int32_t col = (int32_t)(i % 2U);
        int32_t row = (int32_t)(i / 2U);
        int32_t x = grid_x + col * 30;
        int32_t y = grid_y + row * 10;
        egui_color_t fill = is_active ? EGUI_COLOR_HEX(0xDCEAFE) : EGUI_COLOR_HEX(0xF1F5F8);
        egui_color_t border = is_active ? EGUI_COLOR_HEX(0x5B98E5) : EGUI_COLOR_HEX(0xD3DDE6);

        egui_canvas_draw_round_rectangle_fill(x, y, cell_w, cell_h, 3, fill, EGUI_ALPHA_100);
        egui_canvas_draw_round_rectangle(x, y, cell_w, cell_h, 3, 1, border, EGUI_ALPHA_100);
        if (is_active)
        {
            egui_canvas_draw_round_rectangle_fill(x + 3, y + 3, 10, 2, 1, border, EGUI_ALPHA_100);
        }
    }
}

static void demo_draw_window_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t body_region;
    egui_region_t chip_region;
    egui_color_t header = demo_route_is_fast() ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_HEX(0x3AAAB1);
    int32_t chip_x = region->location.x + 62;
    int32_t chip_y = region->location.y + 16;

    demo_draw_round_card(region, 10, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    egui_canvas_draw_round_rectangle_fill(region->location.x + 4, region->location.y + 4, region->size.width - 8, 8, 3, header, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(region->location.x + region->size.width - 16, region->location.y + 8, 1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(region->location.x + region->size.width - 12, region->location.y + 8, 1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(region->location.x + region->size.width - 8, region->location.y + 8, 1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    demo_region_inset(region, &title_region, 8, 4, 48, 18);
    demo_region_inset(region, &body_region, 8, 15, 52, 5);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_get_line_text(), &body_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_PRIMARY,
                                  EGUI_ALPHA_100);

    egui_region_init(&chip_region, chip_x, chip_y, 20, 8);
    demo_draw_round_card(&chip_region, 4, EGUI_COLOR_HEX(0xE8EEF4), EGUI_COLOR_HEX(0xB8C7D4));
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_get_route_text(), &chip_region, EGUI_ALIGN_CENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    egui_region_init(&chip_region, chip_x + 22, chip_y, 20, 8);
    demo_draw_round_card(&chip_region, 4, EGUI_COLOR_HEX(0xDDF1E4), EGUI_COLOR_HEX(0x58B06B));
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_context.shift_a_enabled ? "A" : (demo_context.shift_b_enabled ? "B" : "-"), &chip_region,
                                  EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x236D44), EGUI_ALPHA_100);
}

static void demo_draw_list_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    static const char *row_titles[3] = {"ORD-2016", "ORD-2048", "ORD-2070"};
    egui_region_t title_region;
    egui_region_t row_region;
    uint8_t selected_index = demo_get_list_selected_index();
    uint8_t i;

    demo_draw_round_card(region, 10, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 8, 4, 54, 18);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);

    for (i = 0; i < 3U; i++)
    {
        egui_color_t fill = i == selected_index ? EGUI_COLOR_HEX(0xDCEAFE) : EGUI_COLOR_HEX(0xF6FAFC);
        egui_color_t text = i == selected_index ? EGUI_COLOR_HEX(0x29527C) : DEMO_COLOR_TEXT_SECONDARY;

        egui_region_init(&row_region, region->location.x + 34, region->location.y + 4 + (int32_t)i * 8, region->size.width - 40, 7);
        egui_canvas_draw_round_rectangle_fill(row_region.location.x, row_region.location.y, row_region.size.width, row_region.size.height, 2, fill,
                                              EGUI_ALPHA_100);
        egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, row_titles[i], &row_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text, EGUI_ALPHA_100);
    }
}

static void demo_draw_textblock_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t line1_region;
    egui_region_t line2_region;
    char line1[12];
    char line2[24];

    demo_get_textblock_lines(line1, sizeof(line1), line2, sizeof(line2));
    demo_draw_round_card(region, 10, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 8, 4, 52, 18);
    demo_region_inset(region, &line1_region, 38, 4, 6, 13);
    demo_region_inset(region, &line2_region, 38, 14, 6, 3);

    egui_canvas_draw_round_rectangle_fill(region->location.x + 8, region->location.y + 8, 18, 12, 4, EGUI_COLOR_HEX(0xE8EEF4), EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(region->location.x + 11, region->location.y + 11, 12, 2, 1, EGUI_COLOR_HEX(0x7E93A8), EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(region->location.x + 11, region->location.y + 15, 8, 2, 1, EGUI_COLOR_HEX(0x7E93A8), EGUI_ALPHA_100);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, line1, &line1_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, line2, &line2_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
}

static void demo_draw_schedule_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_color_t fill;
    egui_color_t border;
    uint8_t active = 0;
    char schedule_text[20];

    if (node->data_index == 0U && demo_context.shift_a_enabled)
    {
        fill = EGUI_COLOR_HEX(0xDDF1E4);
        border = EGUI_COLOR_HEX(0x58B06B);
        active = 1;
    }
    else if (node->data_index == 1U && demo_context.shift_b_enabled)
    {
        fill = EGUI_COLOR_HEX(0xDCEAFE);
        border = EGUI_COLOR_HEX(0x5B98E5);
        active = 1;
    }
    else
    {
        demo_get_accent_colors(node->accent, &fill, &border);
    }

    demo_draw_round_card(region, 9, fill, border);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_get_schedule_text(node->data_index, schedule_text, sizeof(schedule_text)), (egui_region_t *)region,
                                  EGUI_ALIGN_CENTER, active ? DEMO_COLOR_TEXT_PRIMARY : DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
}

static void demo_draw_machine_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_color_t fill;
    egui_color_t border;
    egui_region_t title_region;
    egui_region_t status_region;
    char status[20];
    uint8_t load = demo_get_machine_load(node);
    const char *status_text = demo_get_machine_status(node, status, sizeof(status));

    demo_get_machine_card_colors(node, &fill, &border);
    demo_draw_round_card(region, 10, fill, border);
    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, 5, 2, border, EGUI_ALPHA_100);

    demo_region_inset(region, &title_region, 6, 6, 16, 24);
    demo_region_inset(region, &status_region, 6, 18, 12, 12);

    egui_canvas_draw_text_in_rect(DEMO_FONT_BUTTON, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, status_text, &status_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    demo_draw_progress_bar(region->location.x + 6, region->location.y + region->size.height - 7, region->size.width - 12, load, border,
                           EGUI_COLOR_HEX(0xD8E1E8));

    if (demo_machine_is_pinned(node->desc.stable_id))
    {
        egui_canvas_draw_circle_fill(region->location.x + region->size.width - 10, region->location.y + 10, 4, border, EGUI_ALPHA_100);
    }
    else if (demo_machine_is_active(node->desc.stable_id))
    {
        egui_canvas_draw_circle_fill(region->location.x + region->size.width - 10, region->location.y + 10, 4, EGUI_COLOR_HEX(0x2E8E58), EGUI_ALPHA_100);
    }
}

static void demo_draw_sensor_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_color_t fill = EGUI_COLOR_HEX(0xF6FAFC);
    egui_color_t border = EGUI_COLOR_HEX(0xD2DEE7);
    egui_color_t accent_fill;
    egui_color_t accent_border;
    egui_region_t label_region;
    egui_region_t value_region;
    char value[16];

    if (demo_context.zone_enabled[node->data_index % EGUI_ARRAY_SIZE(demo_zone_titles)])
    {
        demo_get_accent_colors(node->accent, &fill, &border);
    }
    else
    {
        demo_get_accent_colors(node->accent, &accent_fill, &accent_border);
        border = accent_border;
    }

    demo_draw_round_card(region, 10, fill, border);
    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, 5, region->size.height, 3, border, EGUI_ALPHA_100);

    demo_region_inset(region, &label_region, 9, 4, 38, 4);
    demo_region_inset(region, &value_region, region->size.width - 36, 4, 6, 4);
    demo_get_sensor_value(node, value, sizeof(value));

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
}

static void demo_draw_line_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    egui_dim_t points[10];
    egui_color_t border = demo_route_is_fast() ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_HEX(0x3AAAB1);
    egui_color_t line_color = demo_pack_is_bag() ? EGUI_COLOR_HEX(0xD68A37) : border;
    int32_t start_x = region->location.x + 28;
    int32_t end_x = region->location.x + region->size.width - 9;
    int32_t mid_x = region->location.x + region->size.width / 2;
    int32_t top_y = region->location.y + 6;
    int32_t bottom_y = region->location.y + region->size.height - 5;

    demo_draw_round_card(region, 10, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 8, 4, 48, 10);
    demo_region_inset(region, &value_region, region->size.width - 22, 4, 6, 10);

    points[0] = (egui_dim_t)start_x;
    points[1] = (egui_dim_t)bottom_y;
    points[2] = (egui_dim_t)(start_x + 10);
    points[3] = (egui_dim_t)(demo_route_is_fast() ? top_y : (top_y + 3));
    points[4] = (egui_dim_t)(mid_x - 2);
    points[5] = (egui_dim_t)(demo_pack_is_bag() ? (bottom_y - 1) : (top_y + 4));
    points[6] = (egui_dim_t)(mid_x + 10);
    points[7] = (egui_dim_t)(demo_context.toggle_enabled[1] ? (top_y + 1) : (bottom_y - 2));
    points[8] = (egui_dim_t)end_x;
    points[9] = (egui_dim_t)(demo_context.checkbox_enabled[0] ? (top_y + 2) : (bottom_y - 1));

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_get_route_text(), &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, line_color, EGUI_ALPHA_100);
    egui_canvas_draw_polyline(points, 5, 2, line_color, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(points[8], points[9], 2, line_color, EGUI_ALPHA_100);
}

static void demo_draw_led_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    egui_color_t fill = EGUI_COLOR_WHITE;
    egui_color_t border = EGUI_COLOR_HEX(0xC9D7E1);
    egui_color_t lamp_color = EGUI_COLOR_HEX(0xBCC8D3);
    uint8_t is_active = demo_led_is_active();
    uint8_t is_lit = is_active && ((demo_context.status_tick & 1U) == 0U);
    int32_t lamp_x = region->location.x + region->size.width - 18;
    int32_t lamp_y = region->location.y + region->size.height / 2;

    if (is_active)
    {
        fill = EGUI_COLOR_HEX(0xEDF7EE);
        border = EGUI_COLOR_HEX(0x58B06B);
        lamp_color = is_lit ? EGUI_COLOR_HEX(0x2E8E58) : EGUI_COLOR_HEX(0x8BC49A);
    }

    demo_draw_round_card(region, 10, fill, border);
    demo_region_inset(region, &title_region, 9, 4, 34, 4);
    demo_region_inset(region, &value_region, region->size.width - 46, 4, 26, 4);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, is_active ? "live" : "idle", &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, border, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(lamp_x, lamp_y, 6, lamp_color, EGUI_ALPHA_100);
    egui_canvas_draw_circle(lamp_x, lamp_y, 6, 1, border, EGUI_ALPHA_100);
}

static void demo_draw_spinner_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    static const int8_t dot_dx[6] = {0, 5, 5, 0, -5, -5};
    static const int8_t dot_dy[6] = {-6, -3, 3, 6, 3, -3};
    egui_region_t title_region;
    egui_region_t value_region;
    egui_color_t fill = EGUI_COLOR_WHITE;
    egui_color_t border = EGUI_COLOR_HEX(0xC9D7E1);
    uint8_t is_active = demo_spinner_is_active();
    uint8_t phase = (uint8_t)(demo_context.status_tick % EGUI_ARRAY_SIZE(dot_dx));
    int32_t center_x = region->location.x + region->size.width - 18;
    int32_t center_y = region->location.y + region->size.height / 2;
    uint8_t i;

    if (is_active)
    {
        fill = EGUI_COLOR_HEX(0xEEF6FB);
        border = EGUI_COLOR_HEX(0x3A6EA5);
    }

    demo_draw_round_card(region, 10, fill, border);
    demo_region_inset(region, &title_region, 9, 4, 34, 4);
    demo_region_inset(region, &value_region, region->size.width - 46, 4, 26, 4);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, is_active ? "spin" : "hold", &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, border, EGUI_ALPHA_100);
    for (i = 0; i < EGUI_ARRAY_SIZE(dot_dx); i++)
    {
        egui_color_t dot_color = (is_active && i == phase) ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_HEX(0xCDD9E2);
        egui_canvas_draw_circle_fill(center_x + dot_dx[i], center_y + dot_dy[i], (is_active && i == phase) ? 2 : 1, dot_color, EGUI_ALPHA_100);
    }
}

static void demo_draw_notification_badge_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t badge_region;
    uint8_t count = demo_get_alarm_badge_count();
    egui_color_t border = count > 0U ? EGUI_COLOR_HEX(0xD68A37) : EGUI_COLOR_HEX(0xC9D7E1);
    egui_color_t fill = count > 0U ? EGUI_COLOR_HEX(0xFFF4E5) : EGUI_COLOR_WHITE;
    char count_text[4];

    demo_draw_round_card(region, 10, fill, border);
    demo_region_inset(region, &title_region, 9, 4, 38, 4);
    demo_region_inset(region, &badge_region, region->size.width - 28, 3, 6, 3);
    egui_api_sprintf(count_text, "%u", (unsigned)count);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    demo_draw_round_card(&badge_region, 7, count > 0U ? EGUI_COLOR_HEX(0xD68A37) : EGUI_COLOR_HEX(0xE8EEF4), border);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, count_text, &badge_region, EGUI_ALIGN_CENTER, count > 0U ? EGUI_COLOR_WHITE : DEMO_COLOR_TEXT_MUTED,
                                  EGUI_ALPHA_100);
}

static void demo_draw_button_like_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_color_t fill;
    egui_color_t border;
    egui_color_t text_color;
    uint8_t style = demo_get_button_style(node->desc.stable_id);
    char label[24];

    demo_get_button_style_colors(style, &fill, &border, &text_color);
    demo_draw_round_card(region, 12, fill, border);
    demo_get_button_label(node->desc.stable_id, label, sizeof(label));
    egui_canvas_draw_text_in_rect(node->kind == DEMO_NODE_KIND_MACHINE ? DEMO_FONT_BUTTON : DEMO_FONT_CAP, label, (egui_region_t *)region, EGUI_ALIGN_CENTER,
                                  text_color, EGUI_ALPHA_100);
}

static void demo_draw_order_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_color_t fill = EGUI_COLOR_WHITE;
    egui_color_t border = EGUI_COLOR_HEX(0xD4DEE7);
    egui_region_t title_region;
    egui_region_t step_region;
    egui_region_t eta_region;
    char step_text[16];
    char eta_text[8];

    if (demo_order_matches_search(node->data_index))
    {
        fill = EGUI_COLOR_HEX(0xFFF0DF);
        border = EGUI_COLOR_HEX(0xD68A37);
    }

    demo_draw_round_card(region, 8, fill, border);
    demo_region_inset(region, &title_region, 4, 3, 4, 10);
    demo_region_inset(region, &step_region, 4, 11, 24, 3);
    demo_region_inset(region, &eta_region, region->size.width - 22, 11, 4, 3);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_get_order_step_text(node->data_index, step_text, sizeof(step_text)), &step_region,
                                  EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_get_order_eta_text(node->data_index, eta_text, sizeof(eta_text)), &eta_region,
                                  EGUI_ALIGN_RIGHT | EGUI_ALIGN_BOTTOM, border, EGUI_ALPHA_100);
}

static void demo_draw_input_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    const char *text = demo_get_input_text(node->desc.stable_id);
    const char *tag = demo_get_input_tag(node->desc.stable_id);
    const char *placeholder = demo_get_input_placeholder(node->desc.stable_id);
    egui_color_t border = (text != NULL && text[0] != '\0') ? EGUI_COLOR_HEX(0x5B98E5) : EGUI_COLOR_HEX(0xA1B3C2);
    egui_region_t text_region;
    egui_region_t tag_region;

    demo_draw_round_card(region, 12, EGUI_COLOR_WHITE, border);
    demo_region_inset(region, &text_region, 10, 6, 58, 6);
    demo_region_inset(region, &tag_region, region->size.width - 46, 6, 10, 6);

    egui_canvas_draw_text_in_rect(DEMO_FONT_BODY, (text != NULL && text[0] != '\0') ? text : placeholder, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                  (text != NULL && text[0] != '\0') ? DEMO_COLOR_TEXT_PRIMARY : DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, tag, &tag_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
}

static void demo_draw_switch_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    int index = demo_get_switch_index(node->desc.stable_id);
    uint8_t is_on = index >= 0 ? demo_context.switch_enabled[index] : 0;
    egui_color_t track = is_on ? EGUI_COLOR_HEX(0x2E8E58) : EGUI_COLOR_HEX(0xC7D4DE);
    egui_region_t label_region;
    int32_t track_w = 32;
    int32_t track_h = 14;
    int32_t track_x = region->location.x + region->size.width - track_w - 5;
    int32_t track_y = region->location.y + (region->size.height - track_h) / 2;
    int32_t thumb_x = is_on ? track_x + track_w - 8 : track_x + 8;

    demo_region_inset(region, &label_region, 4, 3, 36, 3);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(track_x, track_y, track_w, track_h, track_h / 2, track, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(thumb_x, track_y + track_h / 2, 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void demo_draw_checkbox_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    int index = demo_get_checkbox_index(node->desc.stable_id);
    uint8_t is_checked = index >= 0 ? demo_context.checkbox_enabled[index] : 0;
    int32_t box_size = region->size.height - 8;
    int32_t box_x = region->location.x + 4;
    int32_t box_y = region->location.y + 4;
    egui_region_t text_region;
    egui_color_t fill = is_checked ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_WHITE;
    egui_color_t border = is_checked ? EGUI_COLOR_HEX(0x29527C) : EGUI_COLOR_HEX(0xA9B9C6);

    egui_canvas_draw_round_rectangle_fill(box_x, box_y, box_size, box_size, 3, fill, EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle(box_x, box_y, box_size, box_size, 3, 1, border, EGUI_ALPHA_100);
    if (is_checked)
    {
        egui_region_t icon_region = {{box_x, box_y}, {box_size, box_size}};
        egui_canvas_draw_text_in_rect(EGUI_FONT_ICON_MS_16, EGUI_ICON_MS_CHECK, &icon_region, EGUI_ALIGN_CENTER, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    }

    demo_region_inset(region, &text_region, box_size + 10, 3, 2, 3);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
}

static void demo_draw_radio_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    int index = demo_get_radio_index(node->desc.stable_id);
    uint8_t is_selected = index >= 0 && demo_context.radio_selected_index == (uint8_t)index;
    int32_t center_x = region->location.x + 10;
    int32_t center_y = region->location.y + region->size.height / 2;
    egui_region_t text_region;
    egui_color_t border = is_selected ? EGUI_COLOR_HEX(0x167C88) : EGUI_COLOR_HEX(0xA9B9C6);

    egui_canvas_draw_circle(center_x, center_y, 6, 1, border, EGUI_ALPHA_100);
    if (is_selected)
    {
        egui_canvas_draw_circle_fill(center_x, center_y, 3, EGUI_COLOR_HEX(0x167C88), EGUI_ALPHA_100);
    }

    demo_region_inset(region, &text_region, 20, 3, 2, 3);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
}

static void demo_draw_slider_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    int index = demo_get_slider_index(node->desc.stable_id);
    uint8_t value = index >= 0 ? demo_context.slider_value[index] : 0;
    egui_color_t fill = index == 0 ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_HEX(0x167C88);
    egui_region_t title_region;
    egui_region_t value_region;
    int32_t track_x = region->location.x + 8;
    int32_t track_y = region->location.y + region->size.height - 7;
    int32_t track_w = region->size.width - 16;
    int32_t thumb_x = track_x + (track_w * value) / 100;
    char value_text[8];

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xD3DDE6));
    demo_region_inset(region, &title_region, 8, 3, 38, 11);
    demo_region_inset(region, &value_region, region->size.width - 34, 3, 8, 11);
    egui_api_sprintf(value_text, "%d", value);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, fill, EGUI_ALPHA_100);
    demo_draw_progress_bar(track_x, track_y, track_w, value, fill, EGUI_COLOR_HEX(0xD6E0E8));
    egui_canvas_draw_circle_fill(thumb_x, track_y + 2, 4, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_circle(thumb_x, track_y + 2, 4, 1, fill, EGUI_ALPHA_100);
}

static void demo_draw_toggle_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    int index = demo_get_toggle_index(node->desc.stable_id);
    uint8_t is_on = index >= 0 ? demo_context.toggle_enabled[index] : 0;
    egui_color_t fill = is_on ? (index == 0 ? EGUI_COLOR_HEX(0x2E8E58) : EGUI_COLOR_HEX(0x3A6EA5)) : EGUI_COLOR_HEX(0xA9B9C6);
    egui_color_t border = is_on ? fill : EGUI_COLOR_HEX(0x8A9AAA);
    egui_region_t icon_region;
    egui_region_t text_region;

    demo_draw_round_card(region, 10, fill, border);
    demo_region_inset(region, &icon_region, 6, 2, region->size.width - 24, 2);
    demo_region_inset(region, &text_region, 26, 2, 6, 2);

    egui_canvas_draw_text_in_rect(EGUI_FONT_ICON_MS_16, demo_get_toggle_icon(node->desc.stable_id), &icon_region, EGUI_ALIGN_CENTER, EGUI_COLOR_WHITE,
                                  EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void demo_draw_picker_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    int index = demo_get_picker_index(node->desc.stable_id);
    int16_t value = index >= 0 ? demo_context.picker_value[index] : 0;
    egui_region_t title_region;
    egui_region_t control_region;
    egui_region_t top_region;
    egui_region_t mid_region;
    egui_region_t bottom_region;
    char value_text[8];

    demo_draw_round_card(region, 10, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xD3DDE6));
    demo_region_inset(region, &title_region, 8, 4, region->size.width - 52, 4);
    demo_region_inset(region, &control_region, region->size.width - 58, 4, 6, 4);

    top_region = control_region;
    top_region.size.height = control_region.size.height / 3;
    mid_region = control_region;
    mid_region.location.y += top_region.size.height;
    mid_region.size.height = control_region.size.height / 3;
    bottom_region = control_region;
    bottom_region.location.y += top_region.size.height + mid_region.size.height;
    bottom_region.size.height = control_region.location.y + control_region.size.height - bottom_region.location.y;

    egui_api_sprintf(value_text, "%d", (int)value);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle(control_region.location.x, control_region.location.y, control_region.size.width, control_region.size.height, 6, 1,
                                     EGUI_COLOR_HEX(0xD3DDE6), EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(EGUI_FONT_ICON_MS_16, EGUI_ICON_MS_ADD, &top_region, EGUI_ALIGN_CENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_BODY, value_text, &mid_region, EGUI_ALIGN_CENTER, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(EGUI_FONT_ICON_MS_16, EGUI_ICON_MS_REMOVE, &bottom_region, EGUI_ALIGN_CENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
}

static void demo_draw_combobox_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    egui_region_t icon_region;

    demo_draw_round_card(region, 8, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 6, 2, 20, 10);
    demo_region_inset(region, &value_region, 6, 10, 18, 2);
    demo_region_inset(region, &icon_region, region->size.width - 18, 4, 2, 2);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_get_combobox_value(node->desc.stable_id), &value_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM,
                                  DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(EGUI_FONT_ICON_MS_16, EGUI_ICON_MS_EXPAND_MORE, &icon_region, EGUI_ALIGN_CENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
}

static void demo_draw_segmented_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    int segment_index = demo_get_segment_index(node->desc.stable_id);
    uint8_t current_index = segment_index >= 0 ? demo_context.segment_index[segment_index] : 0;
    uint8_t item_count = 0;
    const char **items = demo_get_segment_items(node->desc.stable_id, &item_count);
    egui_color_t active = node->desc.stable_id == DEMO_SEGMENT_ROUTE_ID ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_HEX(0x167C88);
    egui_color_t text_color;
    egui_region_t segment_region;
    egui_dim_t segment_width;
    uint8_t i;

    demo_draw_round_card(region, 8, EGUI_COLOR_HEX(0xE8EEF4), EGUI_COLOR_HEX(0xC9D7E1));
    if (item_count == 0)
    {
        return;
    }

    segment_width = region->size.width / item_count;
    for (i = 0; i < item_count; i++)
    {
        segment_region = *region;
        segment_region.location.x += (egui_dim_t)(i * segment_width);
        segment_region.size.width = i == item_count - 1 ? region->location.x + region->size.width - segment_region.location.x : segment_width;
        if (i == current_index)
        {
            egui_canvas_draw_round_rectangle_fill(segment_region.location.x + 1, segment_region.location.y + 1, segment_region.size.width - 2,
                                                  segment_region.size.height - 2, 7, active, EGUI_ALPHA_100);
            text_color = EGUI_COLOR_WHITE;
        }
        else
        {
            text_color = DEMO_COLOR_TEXT_SECONDARY;
        }
        egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, items[i], &segment_region, EGUI_ALIGN_CENTER, text_color, EGUI_ALPHA_100);
    }
}

static void demo_draw_progress_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    uint8_t value = demo_get_progress_value(node->desc.stable_id);
    char value_text[8];
    egui_color_t fill = node->data_index == 0U ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_HEX(0x167C88);

    demo_draw_round_card(region, 7, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xD3DDE6));
    demo_region_inset(region, &title_region, 6, 2, 28, 8);
    demo_region_inset(region, &value_region, region->size.width - 24, 2, 4, 8);
    egui_api_sprintf(value_text, "%d", (int)value);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    demo_draw_progress_bar(region->location.x + 6, region->location.y + region->size.height - 6, region->size.width - 12, value, fill,
                           EGUI_COLOR_HEX(0xD6E0E8));
}

static void demo_draw_chips_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t chip_region;
    uint8_t active_index = demo_get_chip_index();
    int32_t chip_x = region->location.x + 28;
    int32_t chip_w = (region->size.width - 34) / 3;
    uint8_t i;

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 6, 3, region->size.width - 24, 3);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);

    for (i = 0; i < 3U; i++)
    {
        uint8_t chip_index = (uint8_t)((active_index + i) % EGUI_ARRAY_SIZE(demo_quick_matrix_titles));
        egui_color_t fill = i == 0U ? EGUI_COLOR_HEX(0xDCEAFE) : EGUI_COLOR_HEX(0xF1F5F8);
        egui_color_t border = i == 0U ? EGUI_COLOR_HEX(0x5B98E5) : EGUI_COLOR_HEX(0xC9D7E1);
        egui_color_t text = i == 0U ? EGUI_COLOR_HEX(0x29527C) : DEMO_COLOR_TEXT_MUTED;
        int32_t x = chip_x + i * chip_w;
        int32_t w = (i == 2U) ? (region->location.x + region->size.width - 4 - x) : (chip_w - 2);

        egui_region_init(&chip_region, x, region->location.y + 2, w, region->size.height - 4);
        demo_draw_round_card(&chip_region, 5, fill, border);
        egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_quick_matrix_titles[chip_index], &chip_region, EGUI_ALIGN_CENTER, text, EGUI_ALPHA_100);
    }
}

static void demo_draw_badge_group_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t badge_region;
    char badge_text[3][8];
    uint16_t naive_bytes = demo_get_naive_view_bytes();
    uint16_t live_bytes = demo_get_live_view_bytes();
    uint8_t saved_ratio = naive_bytes > 0U ? (uint8_t)(((uint32_t)(naive_bytes - live_bytes) * 100U) / naive_bytes) : 0U;
    int32_t badge_x = region->location.x + 34;
    int32_t badge_w = (region->size.width - 40) / 3;
    uint8_t i;

    snprintf(badge_text[0], sizeof(badge_text[0]), "A%u", (unsigned)demo_get_alarm_badge_count());
    snprintf(badge_text[1], sizeof(badge_text[1]), "L%u", (unsigned)EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage));
    snprintf(badge_text[2], sizeof(badge_text[2]), "S%u", (unsigned)saved_ratio);

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 6, 3, region->size.width - 28, 3);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);

    for (i = 0; i < 3U; i++)
    {
        egui_color_t fill = i == 0U ? EGUI_COLOR_HEX(0xFFF0DF) : (i == 1U ? EGUI_COLOR_HEX(0xDCEAFE) : EGUI_COLOR_HEX(0xDDF1E4));
        egui_color_t border = i == 0U ? EGUI_COLOR_HEX(0xD68A37) : (i == 1U ? EGUI_COLOR_HEX(0x5B98E5) : EGUI_COLOR_HEX(0x58B06B));
        egui_color_t text = i == 0U ? EGUI_COLOR_HEX(0xA96B23) : (i == 1U ? EGUI_COLOR_HEX(0x29527C) : EGUI_COLOR_HEX(0x236D44));
        int32_t x = badge_x + i * badge_w;
        int32_t w = (i == 2U) ? (region->location.x + region->size.width - 4 - x) : (badge_w - 2);

        egui_region_init(&badge_region, x, region->location.y + 2, w, region->size.height - 4);
        demo_draw_round_card(&badge_region, 5, fill, border);
        egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, badge_text[i], &badge_region, EGUI_ALIGN_CENTER, text, EGUI_ALPHA_100);
    }
}

static void demo_draw_page_indicator_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    uint8_t active_index = demo_get_page_indicator_index();
    int32_t dots_x = region->location.x + region->size.width - 36;
    int32_t center_y = region->location.y + region->size.height / 2;
    uint8_t i;

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 8, 3, 40, 3);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);

    for (i = 0; i < 4; i++)
    {
        int32_t x = dots_x + i * 8;
        egui_color_t color = i == active_index ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_HEX(0xCBD7E1);
        egui_canvas_draw_circle_fill(x, center_y, i == active_index ? 3 : 2, color, EGUI_ALPHA_100);
    }
}

static void demo_draw_divider_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t text_region;
    int32_t center_y = region->location.y + region->size.height / 2;
    int32_t left_x = region->location.x + 6;
    int32_t right_x = region->location.x + region->size.width - 6;
    int32_t text_w = 46;
    int32_t text_x = region->location.x + (region->size.width - text_w) / 2;

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    egui_canvas_draw_round_rectangle_fill(left_x, center_y, text_x - left_x - 4, 1, 1, EGUI_COLOR_HEX(0xC9D7E1), EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(text_x + text_w + 4, center_y, right_x - text_x - text_w - 4, 1, 1, EGUI_COLOR_HEX(0xC9D7E1), EGUI_ALPHA_100);

    egui_region_init(&text_region, text_x, region->location.y, text_w, region->size.height);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &text_region, EGUI_ALIGN_CENTER, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
}

static void demo_draw_scale_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    int32_t track_x = region->location.x + 8;
    int32_t track_w = region->size.width - 16;
    int32_t base_y = region->location.y + region->size.height - 5;
    int32_t marker_x = track_x + (track_w * demo_context.slider_value[1]) / 100;
    char value_text[8];
    int32_t i;

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 6, 2, 38, 8);
    demo_region_inset(region, &value_region, region->size.width - 22, 2, 4, 8);
    egui_api_sprintf(value_text, "%d", (int)demo_context.slider_value[1]);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, EGUI_COLOR_HEX(0x167C88), EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(track_x, base_y, track_w, 1, 1, EGUI_COLOR_HEX(0xC9D7E1), EGUI_ALPHA_100);
    for (i = 0; i < 5; i++)
    {
        int32_t tick_x = track_x + (track_w * i) / 4;
        egui_canvas_draw_round_rectangle_fill(tick_x, base_y - 3, 1, 4, 1, EGUI_COLOR_HEX(0xA8BBC9), EGUI_ALPHA_100);
    }
    egui_canvas_draw_round_rectangle_fill(marker_x - 1, base_y - 5, 3, 7, 2, EGUI_COLOR_HEX(0x167C88), EGUI_ALPHA_100);
}

static void demo_draw_chart_line_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    egui_dim_t points[12];
    egui_color_t line_color = demo_context.export_count > 0U ? EGUI_COLOR_HEX(0xD68A37) : EGUI_COLOR_HEX(0x3A6EA5);
    int32_t chart_x = region->location.x + 28;
    int32_t chart_w = region->size.width - 34;
    int32_t base_y = region->location.y + region->size.height - 4;
    int32_t chart_h = region->size.height - 7;
    char value_text[8];
    uint8_t i;

    if (chart_h < 4)
    {
        chart_h = 4;
    }

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 6, 2, 58, 8);
    demo_region_inset(region, &value_region, region->size.width - 20, 2, 4, 8);
    egui_api_sprintf(value_text, "%u", (unsigned)demo_get_chart_sample(5U));

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, line_color, EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(chart_x, base_y, chart_w, 1, 1, EGUI_COLOR_HEX(0xCDD8E1), EGUI_ALPHA_100);

    for (i = 0; i < 6U; i++)
    {
        int32_t sample = demo_get_chart_sample(i);
        int32_t px = chart_x + (chart_w * i) / 5;
        int32_t py = base_y - (chart_h * sample) / 100;

        points[i * 2U] = (egui_dim_t)px;
        points[i * 2U + 1U] = (egui_dim_t)py;

        if (i > 0U && i < 5U)
        {
            egui_canvas_draw_round_rectangle_fill(px, base_y - 1, 1, 2, 1, EGUI_COLOR_HEX(0xE0E8EE), EGUI_ALPHA_100);
        }
    }

    egui_canvas_draw_polyline(points, 6, 1, line_color, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(points[10], points[11], 2, line_color, EGUI_ALPHA_100);
}

static void demo_draw_tab_bar_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    static const char *tab_labels[3] = {"Ops", "QA", "Ship"};
    egui_region_t tabs_region;
    egui_region_t tab_region;
    uint8_t active_index = demo_get_tab_bar_index();
    int32_t cell_w;
    uint8_t i;

    EGUI_UNUSED(node);

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &tabs_region, 4, 3, 4, 3);
    cell_w = (tabs_region.size.width - 4) / 3;

    for (i = 0; i < 3U; i++)
    {
        egui_color_t fill = i == active_index ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_HEX(0xF1F5F8);
        egui_color_t border = i == active_index ? EGUI_COLOR_HEX(0x29527C) : EGUI_COLOR_HEX(0xC9D7E1);
        egui_color_t text = i == active_index ? EGUI_COLOR_WHITE : DEMO_COLOR_TEXT_MUTED;
        int32_t x = tabs_region.location.x + i * cell_w + (i > 0U ? 2 : 0);
        int32_t w = (i == 2U) ? (tabs_region.location.x + tabs_region.size.width - x) : (cell_w - 2);

        egui_region_init(&tab_region, x, tabs_region.location.y, w, tabs_region.size.height);
        demo_draw_round_card(&tab_region, 5, fill, border);
        egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, tab_labels[i], &tab_region, EGUI_ALIGN_CENTER, text, EGUI_ALPHA_100);
    }
}

static void demo_draw_menu_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    static const char *menu_labels[3] = {"Ops", "Dock", "More"};
    egui_region_t menu_region;
    egui_region_t item_region;
    uint8_t active_index = demo_get_menu_index();
    int32_t item_w;
    int32_t center_y = region->location.y + region->size.height / 2;
    uint8_t i;

    EGUI_UNUSED(node);

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &menu_region, 5, 2, 5, 2);
    item_w = menu_region.size.width / 3;

    for (i = 0; i < 3U; i++)
    {
        egui_color_t text = i == active_index ? EGUI_COLOR_HEX(0xD68A37) : DEMO_COLOR_TEXT_MUTED;
        int32_t x = menu_region.location.x + i * item_w;
        int32_t w = (i == 2U) ? (menu_region.location.x + menu_region.size.width - x) : item_w;
        int32_t arrow_x = x + w - 7;

        egui_region_init(&item_region, x, menu_region.location.y, w, menu_region.size.height);
        egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, menu_labels[i], &item_region, EGUI_ALIGN_CENTER, text, EGUI_ALPHA_100);

        if (i < 2U)
        {
            egui_canvas_draw_round_rectangle_fill(x + w - 1, menu_region.location.y + 2, 1, menu_region.size.height - 4, 1, EGUI_COLOR_HEX(0xD6E0E7),
                                                  EGUI_ALPHA_100);
        }

        egui_canvas_draw_line(arrow_x - 2, center_y - 1, arrow_x, center_y + 1, 1, text, EGUI_ALPHA_100);
        egui_canvas_draw_line(arrow_x, center_y + 1, arrow_x + 2, center_y - 1, 1, text, EGUI_ALPHA_100);

        if (i == active_index)
        {
            egui_canvas_draw_round_rectangle_fill(x + 5, region->location.y + region->size.height - 3, w - 10, 2, 1, text, EGUI_ALPHA_100);
        }
    }
}

static void demo_draw_breadcrumb_bar_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t tag_region;
    egui_region_t crumb_region;
    egui_region_t active_region;
    char route_text[40];
    int32_t active_w = 28;

    egui_api_sprintf(route_text, "%s > %s > %s", demo_get_recipe_text(), demo_get_line_text(), demo_get_dock_text());

    demo_draw_round_card(region, 8, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &tag_region, 6, 3, region->size.width - 44, 12);
    demo_region_inset(region, &crumb_region, 44, 3, 38, 3);
    egui_region_init(&active_region, region->location.x + region->size.width - active_w - 6, region->location.y + 5, active_w, region->size.height - 10);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &tag_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, route_text, &crumb_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    demo_draw_round_card(&active_region, 5, EGUI_COLOR_HEX(0xDCEAFE), EGUI_COLOR_HEX(0x5B98E5));
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_get_dock_text(), &active_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x29527C), EGUI_ALPHA_100);
}

static void demo_draw_message_bar_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t tag_region;
    egui_region_t text_region;
    egui_region_t time_region;
    egui_color_t fill;
    egui_color_t border;
    egui_color_t text_color;
    const char *message_text;

    if (demo_context.export_count > 0U)
    {
        fill = EGUI_COLOR_HEX(0xFFF0DF);
        border = EGUI_COLOR_HEX(0xD68A37);
        text_color = EGUI_COLOR_HEX(0xA96B23);
        message_text = "Export queue active";
    }
    else if (demo_context.note_text[0] != '\0')
    {
        fill = EGUI_COLOR_HEX(0xDDF1E4);
        border = EGUI_COLOR_HEX(0x58B06B);
        text_color = EGUI_COLOR_HEX(0x236D44);
        message_text = "Operator note synced";
    }
    else
    {
        fill = EGUI_COLOR_HEX(0xDCEAFE);
        border = EGUI_COLOR_HEX(0x5B98E5);
        text_color = EGUI_COLOR_HEX(0x29527C);
        message_text = "Flow stable on line";
    }

    demo_draw_round_card(region, 8, fill, border);
    demo_region_inset(region, &tag_region, 8, 3, region->size.width - 54, 12);
    demo_region_inset(region, &text_region, 8, 11, 44, 3);
    demo_region_inset(region, &time_region, region->size.width - 30, 6, 8, 6);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &tag_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, message_text, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM, text_color, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_alert_times[demo_context.quick_matrix_index], &time_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER,
                                  text_color, EGUI_ALPHA_100);
}

static void demo_draw_activity_ring_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    static const egui_color_t ring_colors[3] = {EGUI_COLOR_HEX(0x3A6EA5), EGUI_COLOR_HEX(0x167C88), EGUI_COLOR_HEX(0xD68A37)};
    static const egui_color_t ring_bg_colors[3] = {EGUI_COLOR_HEX(0xDFE9F3), EGUI_COLOR_HEX(0xDDF2F3), EGUI_COLOR_HEX(0xF7E6D5)};
    egui_region_t title_region;
    egui_region_t detail_region;
    egui_region_t tag_region;
    char detail_text[20];
    int32_t center_x = region->location.x + 20;
    int32_t center_y = region->location.y + region->size.height / 2;
    int32_t radius = 9;
    uint8_t i;

    demo_draw_round_card(region, 8, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 38, 4, 48, 12);
    demo_region_inset(region, &detail_region, 38, 14, 44, 3);
    demo_region_inset(region, &tag_region, region->size.width - 42, 4, 6, 14);

    for (i = 0; i < 3U; i++)
    {
        uint8_t value = demo_get_activity_ring_value(i);
        int32_t ring_radius = radius - (int32_t)i * 3;

        egui_canvas_draw_arc(center_x, center_y, ring_radius, 0, 360, 2, ring_bg_colors[i], EGUI_ALPHA_100);
        egui_canvas_draw_arc(center_x, center_y, ring_radius, 270, (int16_t)(270 + ((int32_t)value * 360) / 100), 2, ring_colors[i], EGUI_ALPHA_100);
    }

    egui_api_sprintf(detail_text, "%02u/%02u/%02u", (unsigned)demo_get_activity_ring_value(0U), (unsigned)demo_get_activity_ring_value(1U),
                     (unsigned)demo_get_activity_ring_value(2U));
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, detail_text, &detail_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    demo_draw_round_card(&tag_region, 4, EGUI_COLOR_HEX(0xE7F1FB), EGUI_COLOR_HEX(0x8BB3DD));
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_get_quick_matrix_text(), &tag_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x29527C), EGUI_ALPHA_100);
}

static void demo_draw_gauge_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    egui_region_t detail_region;
    char value_text[8];
    char detail_text[20];
    uint8_t value = demo_get_gauge_value();
    int32_t center_x = region->location.x + 26;
    int32_t center_y = region->location.y + region->size.height - 2;
    int32_t radius = 11;
    int32_t needle_x;
    int32_t needle_y;
    int32_t i;

    demo_draw_round_card(region, 8, EGUI_COLOR_HEX(0xFFF7E8), EGUI_COLOR_HEX(0xD9B46B));
    demo_region_inset(region, &title_region, 48, 4, 40, 12);
    demo_region_inset(region, &value_region, region->size.width - 30, 4, 6, 12);
    demo_region_inset(region, &detail_region, 48, 14, 8, 3);

    egui_canvas_draw_arc(center_x, center_y, radius, 180, 360, 2, EGUI_COLOR_HEX(0xEAD9B4), EGUI_ALPHA_100);
    egui_canvas_draw_arc(center_x, center_y, radius, 180, (int16_t)(180 + ((int32_t)value * 180) / 100), 2, EGUI_COLOR_HEX(0xD68A37), EGUI_ALPHA_100);

    for (i = 0; i <= 4; i++)
    {
        int16_t tick_angle = (int16_t)(180 + i * 45);
        int32_t tick_x1;
        int32_t tick_y1;
        int32_t tick_x2;
        int32_t tick_y2;

        demo_polar_point(center_x, center_y, radius - 1, tick_angle, &tick_x1, &tick_y1);
        demo_polar_point(center_x, center_y, radius - (i == 0 || i == 4 ? 5 : 3), tick_angle, &tick_x2, &tick_y2);
        egui_canvas_draw_line(tick_x1, tick_y1, tick_x2, tick_y2, 1, EGUI_COLOR_HEX(0xB8924A), EGUI_ALPHA_100);
    }

    demo_polar_point(center_x, center_y, radius - 5, (int16_t)(180 + ((int32_t)value * 180) / 100), &needle_x, &needle_y);
    egui_canvas_draw_line(center_x, center_y, needle_x, needle_y, 1, EGUI_COLOR_HEX(0x8E5D17), EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(center_x, center_y, 2, EGUI_COLOR_HEX(0x8E5D17), EGUI_ALPHA_100);

    egui_api_sprintf(value_text, "%u%%", (unsigned)value);
    egui_api_sprintf(detail_text, "%s %s", demo_get_route_text(), demo_get_mode_text());
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, EGUI_COLOR_HEX(0xA96B23), EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, detail_text, &detail_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
}

static void demo_draw_circular_progress_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    egui_region_t detail_region;
    char value_text[8];
    char detail_text[20];
    uint8_t value = demo_get_circular_progress_value();
    int32_t center_x = region->location.x + 19;
    int32_t center_y = region->location.y + region->size.height / 2;

    demo_draw_round_card(region, 8, EGUI_COLOR_HEX(0xECF8EF), EGUI_COLOR_HEX(0x7DB28B));
    demo_region_inset(region, &title_region, 36, 4, 42, 12);
    demo_region_inset(region, &value_region, region->size.width - 30, 4, 6, 12);
    demo_region_inset(region, &detail_region, 36, 14, 8, 3);

    egui_canvas_draw_arc(center_x, center_y, 8, 0, 360, 2, EGUI_COLOR_HEX(0xCFE7D6), EGUI_ALPHA_100);
    egui_canvas_draw_arc(center_x, center_y, 8, 270, (int16_t)(270 + ((int32_t)value * 360) / 100), 2, EGUI_COLOR_HEX(0x58B06B), EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(center_x, center_y, 2, EGUI_COLOR_HEX(0x58B06B), EGUI_ALPHA_100);

    egui_api_sprintf(value_text, "%u%%", (unsigned)value);
    egui_api_sprintf(detail_text, "%s %s", demo_get_pack_text(), demo_context.export_count > 0U ? "ship" : "hold");
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, EGUI_COLOR_HEX(0x236D44), EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, detail_text, &detail_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
}

static void demo_draw_mini_calendar_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t month_region;
    egui_region_t shift_region;
    egui_region_t chip_region;
    char day_text[4];
    uint8_t selected_day = demo_get_calendar_day();
    uint8_t start_day = selected_day > 2U ? (uint8_t)(selected_day - 2U) : 1U;
    const char *shift_text = demo_context.shift_a_enabled ? "A" : (demo_context.shift_b_enabled ? "B" : "-");
    int32_t chip_x = region->location.x + 8;
    int32_t chip_y = region->location.y + 15;
    int32_t chip_gap = 2;
    int32_t chip_w = (region->size.width - 16 - chip_gap * 4) / 5;
    int32_t chip_h = region->size.height - 18;
    uint8_t i;

    if (start_day > 24U)
    {
        start_day = 24U;
    }

    demo_draw_round_card(region, 8, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 8, 4, 96, 12);
    demo_region_inset(region, &month_region, region->size.width - 52, 4, 28, 12);
    demo_region_inset(region, &shift_region, region->size.width - 24, 4, 8, 12);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_get_calendar_month_text(), &month_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_SECONDARY,
                                  EGUI_ALPHA_100);
    demo_draw_round_card(&shift_region, 4, EGUI_COLOR_HEX(0xE7F1FB), EGUI_COLOR_HEX(0x8BB3DD));
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, shift_text, &shift_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x29527C), EGUI_ALPHA_100);

    for (i = 0; i < 5U; i++)
    {
        uint8_t day = (uint8_t)(start_day + i);
        egui_color_t fill = day == selected_day ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_HEX(0xF1F5F8);
        egui_color_t border = day == selected_day ? EGUI_COLOR_HEX(0x29527C) : EGUI_COLOR_HEX(0xD3DDE6);
        egui_color_t text = day == selected_day ? EGUI_COLOR_WHITE : DEMO_COLOR_TEXT_SECONDARY;
        int32_t x = chip_x + (int32_t)i * (chip_w + chip_gap);
        int32_t w = i == 4U ? region->location.x + region->size.width - 8 - x : chip_w;

        egui_region_init(&chip_region, x, chip_y, w, chip_h);
        demo_draw_round_card(&chip_region, 4, fill, border);
        egui_api_sprintf(day_text, "%02u", (unsigned)day);
        egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, day_text, &chip_region, EGUI_ALIGN_CENTER, text, EGUI_ALPHA_100);
    }
}

static void demo_draw_analog_clock_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    egui_region_t detail_region;
    uint8_t hour = 0;
    uint8_t minute = 0;
    char time_text[8];
    char detail_text[18];
    int32_t center_x = region->location.x + 18;
    int32_t center_y = region->location.y + region->size.height / 2;
    int32_t minute_x;
    int32_t minute_y;
    int32_t hour_x;
    int32_t hour_y;

    demo_get_clock_time(&hour, &minute);
    demo_draw_round_card(region, 8, EGUI_COLOR_HEX(0xFFF6EC), EGUI_COLOR_HEX(0xD6B07A));
    demo_region_inset(region, &title_region, 38, 4, 48, 12);
    demo_region_inset(region, &value_region, region->size.width - 30, 4, 6, 12);
    demo_region_inset(region, &detail_region, 38, 14, 8, 3);

    egui_canvas_draw_circle_fill(center_x, center_y, 10, EGUI_COLOR_HEX(0xFFFDF9), EGUI_ALPHA_100);
    egui_canvas_draw_circle(center_x, center_y, 10, 1, EGUI_COLOR_HEX(0xB8833E), EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(center_x - 1, center_y - 8, 2, 2, 1, EGUI_COLOR_HEX(0xB8833E), EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(center_x + 7, center_y - 1, 2, 2, 1, EGUI_COLOR_HEX(0xB8833E), EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(center_x - 1, center_y + 7, 2, 2, 1, EGUI_COLOR_HEX(0xB8833E), EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(center_x - 8, center_y - 1, 2, 2, 1, EGUI_COLOR_HEX(0xB8833E), EGUI_ALPHA_100);

    demo_polar_point(center_x, center_y, 7, (int16_t)(270 + minute * 6), &minute_x, &minute_y);
    demo_polar_point(center_x, center_y, 5, (int16_t)(270 + hour * 30 + minute / 2), &hour_x, &hour_y);
    egui_canvas_draw_line(center_x, center_y, minute_x, minute_y, 1, EGUI_COLOR_HEX(0xD68A37), EGUI_ALPHA_100);
    egui_canvas_draw_line(center_x, center_y, hour_x, hour_y, 2, EGUI_COLOR_HEX(0x8E5D17), EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(center_x, center_y, 2, EGUI_COLOR_HEX(0x8E5D17), EGUI_ALPHA_100);

    egui_api_sprintf(time_text, "%02u:%02u", (unsigned)(hour == 0U ? 12U : hour), (unsigned)minute);
    egui_api_sprintf(detail_text, "%s %s", demo_get_recipe_text(), demo_get_line_text());
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, time_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, EGUI_COLOR_HEX(0xA96B23), EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, detail_text, &detail_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
}

static void demo_draw_compass_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    egui_region_t detail_region;
    char value_text[8];
    char detail_text[18];
    int16_t angle = demo_get_compass_angle();
    int32_t center_x = region->location.x + 18;
    int32_t center_y = region->location.y + region->size.height / 2;
    int32_t tip_x;
    int32_t tip_y;
    int32_t tail_x;
    int32_t tail_y;

    demo_draw_round_card(region, 8, EGUI_COLOR_HEX(0xEDF7FB), EGUI_COLOR_HEX(0x7DB4C4));
    demo_region_inset(region, &title_region, 38, 4, 48, 12);
    demo_region_inset(region, &value_region, region->size.width - 30, 4, 6, 12);
    demo_region_inset(region, &detail_region, 38, 14, 8, 3);

    egui_canvas_draw_circle_fill(center_x, center_y, 10, EGUI_COLOR_HEX(0xFCFEFF), EGUI_ALPHA_100);
    egui_canvas_draw_circle(center_x, center_y, 10, 1, EGUI_COLOR_HEX(0x4B8FA5), EGUI_ALPHA_100);
    egui_canvas_draw_text(DEMO_FONT_CAP, "N", center_x - 3, center_y - 12, EGUI_COLOR_HEX(0x29527C), EGUI_ALPHA_100);
    demo_polar_point(center_x, center_y, 8, (int16_t)(angle - 90), &tip_x, &tip_y);
    demo_polar_point(center_x, center_y, 4, (int16_t)(angle + 90), &tail_x, &tail_y);
    egui_canvas_draw_line(tail_x, tail_y, tip_x, tip_y, 2, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(center_x, center_y, 2, EGUI_COLOR_HEX(0x29527C), EGUI_ALPHA_100);

    egui_api_sprintf(value_text, "%03d", (int)angle);
    egui_api_sprintf(detail_text, "%s %s", demo_get_route_text(), demo_get_dock_text());
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, EGUI_COLOR_HEX(0x2D6F83), EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, detail_text, &detail_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
}

static void demo_draw_heart_rate_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    egui_dim_t points[14];
    uint16_t bpm = demo_get_heart_rate_value();
    char value_text[8];
    int32_t chart_x = region->location.x + 26;
    int32_t base_y = region->location.y + region->size.height - 4;
    int32_t chart_w = region->size.width - 30;
    int32_t peaks[7] = {4, 4, 8, 2, 10, 5, 4};
    uint8_t i;

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xD9DDE6));
    demo_region_inset(region, &title_region, 6, 2, 42, 8);
    demo_region_inset(region, &value_region, region->size.width - 22, 2, 4, 8);
    egui_api_sprintf(value_text, "%u", (unsigned)bpm);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, EGUI_COLOR_HEX(0xC85C4B), EGUI_ALPHA_100);

    for (i = 0; i < 7U; i++)
    {
        int32_t x = chart_x + (chart_w * i) / 6;
        int32_t y = base_y - peaks[i];

        if (i == 2U)
        {
            y -= 2;
        }
        if (i == 4U)
        {
            y -= (int32_t)((bpm - 80U) / 16U);
        }
        if (y < region->location.y + 4)
        {
            y = region->location.y + 4;
        }
        points[i * 2U] = (egui_dim_t)x;
        points[i * 2U + 1U] = (egui_dim_t)y;
    }

    egui_canvas_draw_polyline(points, 7, 1, EGUI_COLOR_HEX(0xC85C4B), EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(points[8], points[9], 1, EGUI_COLOR_HEX(0xC85C4B), EGUI_ALPHA_100);
}

static void demo_draw_chart_scatter_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    char value_text[8];
    int32_t chart_x = region->location.x + 26;
    int32_t chart_y = region->location.y + 5;
    int32_t chart_w = region->size.width - 32;
    int32_t chart_h = region->size.height - 9;
    uint8_t i;

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xD9DDE6));
    demo_region_inset(region, &title_region, 6, 2, 48, 8);
    demo_region_inset(region, &value_region, region->size.width - 24, 2, 4, 8);
    egui_api_sprintf(value_text, "%u", (unsigned)demo_get_scatter_sample(3U));

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(chart_x, chart_y + chart_h - 1, chart_w, 1, 1, EGUI_COLOR_HEX(0xD9E3EA), EGUI_ALPHA_100);

    for (i = 0; i < 5U; i++)
    {
        uint8_t sample = demo_get_scatter_sample(i);
        int32_t x = chart_x + 4 + (chart_w - 8) * i / 4;
        int32_t y = chart_y + chart_h - 3 - ((chart_h - 3) * sample) / 100;
        egui_color_t fill = i == 3U ? EGUI_COLOR_HEX(0xD68A37) : EGUI_COLOR_HEX(0x5B98E5);

        egui_canvas_draw_circle_fill(x, y, i == 3U ? 2 : 1, fill, EGUI_ALPHA_100);
    }
}

static void demo_draw_digital_clock_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    egui_region_t second_region;
    char time_text[8];
    char second_text[4];
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = demo_get_clock_second();
    uint8_t colon_visible = demo_get_clock_colon_visible();

    demo_get_clock_time(&hour, &minute);
    egui_api_sprintf(time_text, "%02u%c%02u", (unsigned)(hour == 0U ? 12U : hour), colon_visible ? ':' : ' ', (unsigned)minute);
    egui_api_sprintf(second_text, "%02u", (unsigned)second);

    demo_draw_round_card(region, 9, EGUI_COLOR_HEX(0xF3F8FD), EGUI_COLOR_HEX(0x8AAED8));
    egui_canvas_draw_round_rectangle_fill(region->location.x + 6, region->location.y + 3, 4, region->size.height - 6, 2, EGUI_COLOR_HEX(0x5B98E5),
                                          EGUI_ALPHA_100);

    demo_region_inset(region, &title_region, 14, 2, 64, 2);
    demo_region_inset(region, &value_region, 44, 2, 24, 2);
    demo_region_inset(region, &second_region, region->size.width - 20, 3, 4, 3);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, time_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x29527C), EGUI_ALPHA_100);
    demo_draw_round_card(&second_region, 4, EGUI_COLOR_HEX(0xDCEAFE), EGUI_COLOR_HEX(0x7AA8E8));
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, second_text, &second_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x29527C), EGUI_ALPHA_100);
}

static void demo_draw_chart_bar_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    static const egui_color_t fills[4] = {EGUI_COLOR_HEX(0x5B98E5), EGUI_COLOR_HEX(0x3AAAB1), EGUI_COLOR_HEX(0x58B06B), EGUI_COLOR_HEX(0xD68A37)};
    egui_region_t title_region;
    egui_region_t value_region;
    char value_text[8];
    uint8_t max_value = 0;
    int32_t chart_x = region->location.x + 28;
    int32_t chart_w = region->size.width - 34;
    int32_t base_y = region->location.y + region->size.height - 4;
    int32_t chart_h = region->size.height - 8;
    int32_t bar_gap = 2;
    int32_t bar_w;
    uint8_t i;

    if (chart_h < 5)
    {
        chart_h = 5;
    }

    demo_draw_round_card(region, 10, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 8, 4, 42, 4);
    demo_region_inset(region, &value_region, region->size.width - 18, 4, 6, 4);

    for (i = 0; i < 4U; i++)
    {
        uint8_t sample = demo_get_bar_chart_sample(i);
        if (sample > max_value)
        {
            max_value = sample;
        }
    }

    egui_api_sprintf(value_text, "%u", (unsigned)max_value);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x29527C), EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(chart_x, base_y, chart_w, 1, 1, EGUI_COLOR_HEX(0xD7E2E9), EGUI_ALPHA_100);

    bar_w = (chart_w - 3 * bar_gap) / 4;
    if (bar_w < 3)
    {
        bar_w = 3;
    }

    for (i = 0; i < 4U; i++)
    {
        uint8_t sample = demo_get_bar_chart_sample(i);
        int32_t x = chart_x + (int32_t)i * (bar_w + bar_gap);
        int32_t h = EGUI_MAX(2, ((chart_h - 1) * sample) / 100);
        int32_t y = base_y - h;
        int32_t w = i == 3U ? region->location.x + region->size.width - 6 - x : bar_w;

        egui_canvas_draw_round_rectangle_fill(x, y, w, h, 2, fills[i], EGUI_ALPHA_100);
    }
}

static void demo_draw_chart_pie_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    static const egui_color_t slice_colors[3] = {EGUI_COLOR_HEX(0x5B98E5), EGUI_COLOR_HEX(0x58B06B), EGUI_COLOR_HEX(0xD68A37)};
    egui_region_t title_region;
    egui_region_t value_region;
    uint8_t values[3];
    uint16_t total = 0;
    uint8_t dominant = 0;
    int32_t center_x = region->location.x + 18;
    int32_t center_y = region->location.y + region->size.height / 2;
    int16_t start_angle = 270;
    char value_text[8];
    uint8_t i;

    for (i = 0; i < 3U; i++)
    {
        values[i] = demo_get_pie_slice_value(i);
        total = (uint16_t)(total + values[i]);
        if (values[i] > dominant)
        {
            dominant = values[i];
        }
    }
    if (total == 0U)
    {
        total = 1U;
    }

    demo_draw_round_card(region, 8, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 38, 2, 52, 12);
    demo_region_inset(region, &value_region, region->size.width - 24, 2, 6, 12);
    egui_api_sprintf(value_text, "%u%%", (unsigned)((dominant * 100U) / total));

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, value_text, &value_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, EGUI_COLOR_HEX(0x29527C), EGUI_ALPHA_100);
    egui_canvas_draw_arc(center_x, center_y, 8, 0, 360, 3, EGUI_COLOR_HEX(0xDDE6ED), EGUI_ALPHA_100);

    for (i = 0; i < 3U; i++)
    {
        int16_t end_angle = (int16_t)(start_angle + ((int32_t)values[i] * 360) / total);
        int32_t bar_x = region->location.x + 40;
        int32_t bar_y = region->location.y + 13 + (int32_t)i * 4;
        int32_t bar_w = EGUI_MAX(8, ((region->size.width - 48) * values[i]) / total);

        egui_canvas_draw_arc(center_x, center_y, 8, start_angle, end_angle, 3, slice_colors[i], EGUI_ALPHA_100);
        egui_canvas_draw_round_rectangle_fill(bar_x, bar_y, bar_w, 3, 1, slice_colors[i], EGUI_ALPHA_100);
        start_angle = end_angle;
    }

    egui_canvas_draw_circle_fill(center_x, center_y, 3, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void demo_draw_stopwatch_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t value_region;
    egui_region_t state_region;
    uint32_t elapsed_ms = demo_get_stopwatch_elapsed_ms();
    uint32_t total_seconds = elapsed_ms / 1000U;
    uint32_t minutes = total_seconds / 60U;
    uint32_t seconds = total_seconds % 60U;
    uint32_t centiseconds = (elapsed_ms % 1000U) / 10U;
    uint8_t is_running =
            (uint8_t)(demo_context.shift_a_enabled || demo_context.shift_b_enabled || demo_context.quick_matrix_index != 0U || demo_context.export_count != 0U);
    char time_text[16];
    int32_t center_x = region->location.x + 18;
    int32_t center_y = region->location.y + region->size.height / 2;
    int32_t hand_x;
    int32_t hand_y;
    int32_t progress_w = ((region->size.width - 46) * (int32_t)centiseconds) / 100;

    if (minutes > 99U)
    {
        minutes = 99U;
    }

    demo_draw_round_card(region, 8, EGUI_COLOR_HEX(0xF7FBFF), EGUI_COLOR_HEX(0xB7CAD9));
    demo_region_inset(region, &title_region, 38, 2, 62, 12);
    demo_region_inset(region, &value_region, 38, 12, 8, 3);
    demo_region_inset(region, &state_region, region->size.width - 32, 3, 6, 11);
    egui_api_sprintf(time_text, "%02u:%02u.%02u", (unsigned)minutes, (unsigned)seconds, (unsigned)centiseconds);

    egui_canvas_draw_circle_fill(center_x, center_y, 8, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_circle(center_x, center_y, 8, 1, EGUI_COLOR_HEX(0x7F98AC), EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(center_x - 2, center_y - 10, 4, 2, 1, EGUI_COLOR_HEX(0x7F98AC), EGUI_ALPHA_100);
    demo_polar_point(center_x, center_y, 5, (int16_t)(270 + (seconds * 6U)), &hand_x, &hand_y);
    egui_canvas_draw_line(center_x, center_y, hand_x, hand_y, 1, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(center_x, center_y, 1, EGUI_COLOR_HEX(0x3A6EA5), EGUI_ALPHA_100);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_BUTTON, time_text, &value_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    demo_draw_round_card(&state_region, 4, is_running ? EGUI_COLOR_HEX(0xDDF1E4) : EGUI_COLOR_HEX(0xE8EEF4),
                         is_running ? EGUI_COLOR_HEX(0x58B06B) : EGUI_COLOR_HEX(0xB8C7D4));
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, is_running ? "run" : "hold", &state_region, EGUI_ALIGN_CENTER,
                                  is_running ? EGUI_COLOR_HEX(0x236D44) : DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);

    if (progress_w > 0)
    {
        egui_canvas_draw_round_rectangle_fill(region->location.x + 38, region->location.y + region->size.height - 5, progress_w, 2, 1, EGUI_COLOR_HEX(0x7AA8E8),
                                              EGUI_ALPHA_100);
    }
}

static void demo_draw_table_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t peak_region;
    egui_region_t chip_region;
    egui_region_t cell_region;
    const egui_color_t fills[3] = {EGUI_COLOR_HEX(0xF1F5F8), EGUI_COLOR_HEX(0xDCEAFE), EGUI_COLOR_HEX(0xDDF1E4)};
    const egui_color_t borders[3] = {EGUI_COLOR_HEX(0xC9D7E1), EGUI_COLOR_HEX(0x5B98E5), EGUI_COLOR_HEX(0x58B06B)};
    const egui_color_t text_colors[3] = {DEMO_COLOR_TEXT_PRIMARY, EGUI_COLOR_HEX(0x29527C), EGUI_COLOR_HEX(0x236D44)};
    char all_text[8];
    char live_text[8];
    char peak_text[8];
    char chip_text[3][14];
    uint16_t naive_bytes = demo_get_naive_view_bytes();
    uint16_t live_bytes = demo_get_live_view_bytes();
    uint8_t saved_ratio = naive_bytes > 0U ? (uint8_t)(((uint32_t)(naive_bytes - live_bytes) * 100U) / naive_bytes) : 0U;
    int32_t cell_w;
    int32_t i;

    demo_format_bytes_short(naive_bytes, all_text, sizeof(all_text));
    demo_format_bytes_short(live_bytes, live_text, sizeof(live_text));
    demo_format_bytes_short(demo_context.peak_live_bytes, peak_text, sizeof(peak_text));
    snprintf(chip_text[0], sizeof(chip_text[0]), "all %s", all_text);
    snprintf(chip_text[1], sizeof(chip_text[1]), "live %s", live_text);
    snprintf(chip_text[2], sizeof(chip_text[2]), "save %u%%", (unsigned)saved_ratio);

    demo_draw_round_card(region, 8, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 6, 2, 66, 14);
    demo_region_inset(region, &peak_region, region->size.width - 52, 2, 6, 14);
    demo_region_inset(region, &chip_region, 4, 12, 4, 3);
    cell_w = (chip_region.size.width - 4) / 3;

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, peak_text, &peak_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);

    for (i = 0; i < 3; i++)
    {
        int32_t x = chip_region.location.x + i * cell_w + (i > 0 ? 2 : 0);
        int32_t w = (i == 2) ? (region->location.x + region->size.width - 4 - x) : cell_w - 2;

        egui_region_init(&cell_region, x, chip_region.location.y, w, chip_region.size.height);
        demo_draw_round_card(&cell_region, 5, fills[i], borders[i]);
        egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, chip_text[i], &cell_region, EGUI_ALIGN_CENTER, text_colors[i], EGUI_ALPHA_100);
    }
}

static void demo_draw_button_matrix_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t button_region;
    egui_region_t cell_region;
    egui_color_t fill;
    egui_color_t border;
    egui_color_t text_color;
    int32_t cell_w;
    int32_t i;

    EGUI_UNUSED(node);

    demo_draw_round_card(region, 8, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 6, 2, 48, 14);
    demo_region_inset(region, &button_region, 4, 13, 4, 3);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, "Quick Ops", &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);

    cell_w = (button_region.size.width - 6) / 4;
    for (i = 0; i < 4; i++)
    {
        int32_t x = button_region.location.x + i * cell_w + (i > 0 ? 2 : 0);
        int32_t w = (i == 3) ? (region->location.x + region->size.width - 4 - x) : cell_w - 2;

        if ((uint8_t)i == demo_context.quick_matrix_index)
        {
            fill = EGUI_COLOR_HEX(0x3A6EA5);
            border = EGUI_COLOR_HEX(0x29527C);
            text_color = EGUI_COLOR_WHITE;
        }
        else
        {
            fill = EGUI_COLOR_HEX(0xE8EEF4);
            border = EGUI_COLOR_HEX(0xC8D4DE);
            text_color = DEMO_COLOR_TEXT_PRIMARY;
        }

        egui_region_init(&cell_region, x, button_region.location.y, w, button_region.size.height);
        demo_draw_round_card(&cell_region, 6, fill, border);
        egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_quick_matrix_titles[i], &cell_region, EGUI_ALIGN_CENTER, text_color, EGUI_ALPHA_100);
    }
}

static void demo_draw_spangroup_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_region_t title_region;
    egui_region_t chip_region;
    egui_region_t cell_region;
    const egui_color_t fills[3] = {EGUI_COLOR_HEX(0xE8EEF4), EGUI_COLOR_HEX(0xDDF4F2), EGUI_COLOR_HEX(0xDCEAFE)};
    const egui_color_t borders[3] = {EGUI_COLOR_HEX(0xB8C7D4), EGUI_COLOR_HEX(0x7BB4BA), EGUI_COLOR_HEX(0x7AA8E8)};
    char previews[3][15];
    int32_t cell_w;
    int32_t i;

    demo_draw_round_card(region, 8, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xC9D7E1));
    demo_region_inset(region, &title_region, 6, 2, 84, 14);
    demo_region_inset(region, &chip_region, 4, 12, 4, 3);
    cell_w = (chip_region.size.width - 4) / 3;

    demo_get_recent_log_preview(0U, previews[0], sizeof(previews[0]));
    demo_get_recent_log_preview(1U, previews[1], sizeof(previews[1]));
    demo_get_recent_log_preview(2U, previews[2], sizeof(previews[2]));

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    for (i = 0; i < 3; i++)
    {
        int32_t x = chip_region.location.x + i * cell_w + (i > 0 ? 2 : 0);
        int32_t w = (i == 2) ? (region->location.x + region->size.width - 4 - x) : cell_w - 2;

        egui_region_init(&cell_region, x, chip_region.location.y, w, chip_region.size.height);
        demo_draw_round_card(&cell_region, 5, fills[i], borders[i]);
        egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, previews[i], &cell_region, EGUI_ALIGN_CENTER, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    }
}

static void demo_draw_roller_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    int roller_index = demo_get_roller_index(node->desc.stable_id);
    uint8_t current_index = roller_index >= 0 ? demo_context.roller_index[roller_index] : 0;
    uint8_t item_count = 0;
    const char **items = demo_get_roller_items(node->desc.stable_id, &item_count);
    egui_region_t title_region;
    egui_region_t top_region;
    egui_region_t mid_region;
    egui_region_t bottom_region;
    uint8_t prev_index;
    uint8_t next_index;

    if (items == NULL || item_count == 0)
    {
        return;
    }

    if (current_index >= item_count)
    {
        current_index = 0;
    }

    prev_index = current_index == 0 ? (uint8_t)(item_count - 1) : (uint8_t)(current_index - 1);
    next_index = (uint8_t)((current_index + 1U) % item_count);

    demo_draw_round_card(region, 9, EGUI_COLOR_WHITE, EGUI_COLOR_HEX(0xD3DDE6));
    demo_region_inset(region, &title_region, 6, 3, 6, region->size.height - 12);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, node->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);

    top_region = *region;
    top_region.location.y += 12;
    top_region.size.height = 11;
    mid_region = *region;
    mid_region.location.y += 22;
    mid_region.size.height = 16;
    bottom_region = *region;
    bottom_region.location.y += 38;
    bottom_region.size.height = region->location.y + region->size.height - bottom_region.location.y - 2;

    egui_canvas_draw_round_rectangle_fill(region->location.x + 4, mid_region.location.y - 1, region->size.width - 8, mid_region.size.height + 2, 6,
                                          EGUI_COLOR_HEX(0xDCEAFE), EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, items[prev_index], &top_region, EGUI_ALIGN_CENTER, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, items[current_index], &mid_region, EGUI_ALIGN_CENTER, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, items[next_index], &bottom_region, EGUI_ALIGN_CENTER, DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
}

static void demo_draw_alert_node(const demo_virtual_node_t *node, const egui_region_t *region)
{
    egui_color_t fill;
    egui_color_t border;
    egui_region_t title_region;
    egui_region_t detail_region;
    egui_region_t time_region;
    char detail_text[24];
    char note_preview[16];
    const char *title = node->title;
    const char *detail = NULL;
    uint8_t level = demo_alert_levels[node->data_index];

    if (node->data_index == 0U && demo_context.note_text[0] != '\0')
    {
        title = "Operator note";
        demo_copy_preview(demo_context.note_text, note_preview, sizeof(note_preview));
        detail = note_preview;
        fill = EGUI_COLOR_HEX(0xE4F7E7);
        border = EGUI_COLOR_HEX(0x58B06B);
    }
    else
    {
        switch (level)
        {
        case 2:
            fill = EGUI_COLOR_HEX(0xFCE5E5);
            border = EGUI_COLOR_HEX(0xC85C4B);
            break;
        case 1:
            fill = EGUI_COLOR_HEX(0xFFF0DF);
            border = EGUI_COLOR_HEX(0xD68A37);
            break;
        default:
            fill = EGUI_COLOR_HEX(0xDCEAFE);
            border = EGUI_COLOR_HEX(0x5B98E5);
            break;
        }

        switch (node->data_index)
        {
        case 0:
            egui_api_sprintf(detail_text, "%s %s", demo_get_recipe_text(), demo_get_line_text());
            detail = detail_text;
            break;
        case 1:
            egui_api_sprintf(detail_text, "dock %s", demo_get_dock_text());
            detail = detail_text;
            break;
        case 2:
            egui_api_sprintf(detail_text, "%s film", demo_pack_is_bag() ? "bag" : "box");
            detail = detail_text;
            break;
        case 3:
            if (demo_context.pin_agv_enabled)
            {
                title = "AGV pinned";
            }
            egui_api_sprintf(detail_text, "%s %s", demo_get_line_text(), demo_get_route_text());
            detail = detail_text;
            break;
        case 4:
            demo_copy_text(detail_text, sizeof(detail_text), demo_context.switch_enabled[1] ? "purge ready" : "purge wait");
            detail = detail_text;
            break;
        case 5:
            egui_api_sprintf(detail_text, "%dkW", 420 + demo_context.slider_value[0] * 2 + (demo_context.switch_enabled[1] ? 24 : 0));
            detail = detail_text;
            break;
        case 6:
            egui_api_sprintf(detail_text, "%dz active", (int)demo_count_enabled_zones());
            detail = detail_text;
            break;
        case 7:
            demo_copy_text(detail_text, sizeof(detail_text), demo_context.checkbox_enabled[0] ? "qa audit" : "sample");
            detail = detail_text;
            break;
        case 8:
            if (demo_context.export_count > 0U)
            {
                title = "Export pulse";
            }
            egui_api_sprintf(detail_text, "export %02d", (int)(demo_context.export_count % 100U));
            detail = detail_text;
            break;
        case 9:
            egui_api_sprintf(detail_text, "crew %s", demo_get_crew_text());
            detail = detail_text;
            break;
        case 10:
            if (demo_context.combobox_index[1] == 2U)
            {
                title = "QA lane";
            }
            egui_api_sprintf(detail_text, "%s %s", demo_get_pack_text(), demo_get_route_text());
            detail = detail_text;
            break;
        case 11:
            if (demo_context.search_text[0] != '\0')
            {
                demo_copy_preview(demo_context.search_text, detail_text, sizeof(detail_text));
            }
            else
            {
                demo_copy_text(detail_text, sizeof(detail_text), "service");
            }
            detail = detail_text;
            break;
        default:
            detail = demo_context.toggle_enabled[0] ? "trace sync" : "service";
            break;
        }
    }

    demo_draw_round_card(region, 9, fill, border);
    demo_region_inset(region, &title_region, 8, 4, 40, 12);
    demo_region_inset(region, &time_region, region->size.width - 34, 4, 8, 12);
    demo_region_inset(region, &detail_region, 8, 14, 8, 4);

    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, demo_alert_times[node->data_index], &time_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP, border, EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(DEMO_FONT_CAP, detail, &detail_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM, DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
}

void demo_adapter_draw_node(void *adapter_context, egui_view_t *page, uint32_t index, const egui_virtual_stage_node_desc_t *desc,
                            const egui_region_t *screen_region)
{
    demo_virtual_stage_context_t *ctx = (demo_virtual_stage_context_t *)adapter_context;
    demo_virtual_node_t *node = &ctx->nodes[index];
    egui_region_t local_region;

    EGUI_UNUSED(page);
    EGUI_UNUSED(desc);

    demo_make_local_region(screen_region, &local_region);

    switch (node->kind)
    {
    case DEMO_NODE_KIND_PANEL:
        demo_draw_panel_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_KPI:
        demo_draw_kpi_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_TILEVIEW:
        demo_draw_tileview_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_WINDOW:
        demo_draw_window_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_LIST:
        demo_draw_list_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_TEXTBLOCK:
        demo_draw_textblock_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_SCHEDULE:
        demo_draw_schedule_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_MACHINE:
        demo_draw_machine_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_SENSOR:
        demo_draw_sensor_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_LINE:
        demo_draw_line_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_LED:
        demo_draw_led_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_SPINNER:
        demo_draw_spinner_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_NOTIFICATION_BADGE:
        demo_draw_notification_badge_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_ZONE_BUTTON:
    case DEMO_NODE_KIND_ACTION_BUTTON:
        demo_draw_button_like_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_ORDER:
        demo_draw_order_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_TEXTINPUT:
        demo_draw_input_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_ALERT:
        demo_draw_alert_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_SWITCH:
        demo_draw_switch_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_CHECKBOX:
        demo_draw_checkbox_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_RADIO:
        demo_draw_radio_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_SLIDER:
        demo_draw_slider_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_TOGGLE:
        demo_draw_toggle_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_NUMBER_PICKER:
        demo_draw_picker_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_COMBOBOX:
        demo_draw_combobox_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_ROLLER:
        demo_draw_roller_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_SEGMENTED:
        demo_draw_segmented_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_PROGRESS:
        demo_draw_progress_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_CHIPS:
        demo_draw_chips_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_BADGE_GROUP:
        demo_draw_badge_group_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_PAGE_INDICATOR:
        demo_draw_page_indicator_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_DIVIDER:
        demo_draw_divider_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_SCALE:
        demo_draw_scale_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_CHART_LINE:
        demo_draw_chart_line_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_TAB_BAR:
        demo_draw_tab_bar_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_MENU:
        demo_draw_menu_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_BREADCRUMB_BAR:
        demo_draw_breadcrumb_bar_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_MESSAGE_BAR:
        demo_draw_message_bar_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_ACTIVITY_RING:
        demo_draw_activity_ring_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_GAUGE:
        demo_draw_gauge_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_CIRCULAR_PROGRESS:
        demo_draw_circular_progress_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_MINI_CALENDAR:
        demo_draw_mini_calendar_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_ANALOG_CLOCK:
        demo_draw_analog_clock_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_COMPASS:
        demo_draw_compass_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_HEART_RATE:
        demo_draw_heart_rate_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_CHART_SCATTER:
        demo_draw_chart_scatter_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_DIGITAL_CLOCK:
        demo_draw_digital_clock_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_CHART_BAR:
        demo_draw_chart_bar_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_CHART_PIE:
        demo_draw_chart_pie_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_STOPWATCH:
        demo_draw_stopwatch_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_TABLE:
        demo_draw_table_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_BUTTON_MATRIX:
        demo_draw_button_matrix_node(node, &local_region);
        break;
    case DEMO_NODE_KIND_SPANGROUP:
        demo_draw_spangroup_node(node, &local_region);
        break;
    default:
        break;
    }
}

static uint16_t demo_get_live_view_bytes(void)
{
    uint16_t bytes = 0;
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS; i++)
    {
        const egui_view_virtual_stage_slot_t *slot = EGUI_VIEW_VIRTUAL_STAGE_GET_SLOT(&virtual_stage, i);

        if (slot != NULL && slot->state != EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_UNUSED)
        {
            bytes = (uint16_t)(bytes + demo_get_view_instance_size(slot->view_type));
        }
    }

    return bytes;
}

static uint16_t demo_get_naive_view_bytes(void)
{
    uint32_t bytes = 0;
    uint32_t i;

    for (i = 0; i < DEMO_NODE_COUNT; i++)
    {
        bytes += demo_get_view_instance_size(demo_context.nodes[i].desc.view_type);
    }

    return (uint16_t)bytes;
}

static void demo_refresh_status(void)
{
    uint8_t live_slots = EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage);
    uint16_t live_bytes = demo_get_live_view_bytes();
    uint16_t naive_bytes = demo_get_naive_view_bytes();
    uint8_t save_ratio = naive_bytes > 0U ? (uint8_t)(((uint32_t)(naive_bytes - live_bytes) * 100U) / naive_bytes) : 0U;
    uint8_t zones_on = demo_count_enabled_zones();
    const char *shift_text = demo_context.shift_a_enabled ? "A" : (demo_context.shift_b_enabled ? "B" : "-");
    const char *mode_text = demo_get_mode_text();
    const char *recipe_text = demo_get_recipe_text();
    const char *line_text = demo_get_line_text();
    const char *route_text = demo_get_route_text();
    const char *dock_text = demo_get_dock_text();
    const char *quick_text = demo_get_quick_matrix_text();
    char title[DEMO_STATUS_TITLE_LEN];
    char detail[DEMO_STATUS_DETAIL_LEN];
    char hint[DEMO_STATUS_HINT_LEN];
    char live_bytes_text[8];
    char peak_bytes_text[8];
    char last_preview[18];
    char search_preview[16];
    char note_preview[16];

    if (live_slots > demo_context.peak_live_slots)
    {
        demo_context.peak_live_slots = live_slots;
    }
    if (live_bytes > demo_context.peak_live_bytes)
    {
        demo_context.peak_live_bytes = live_bytes;
    }

    demo_format_bytes_short(live_bytes, live_bytes_text, sizeof(live_bytes_text));
    demo_format_bytes_short(demo_context.peak_live_bytes, peak_bytes_text, sizeof(peak_bytes_text));
    demo_get_recent_log_preview(0U, last_preview, sizeof(last_preview));
    demo_copy_preview(demo_context.search_text, search_preview, sizeof(search_preview));
    demo_copy_preview(demo_context.note_text, note_preview, sizeof(note_preview));

    egui_api_sprintf(title, "virtual_stage cockpit 100 nodes  live %d/%d  save %d%%", live_slots, DEMO_LIVE_SLOT_LIMIT, (int)save_ratio);
    egui_api_sprintf(detail, "mode:%s  recipe:%s  line:%s  route:%s  quick:%s  speed:%d", mode_text, recipe_text, line_text, route_text, quick_text,
                     (int)demo_context.slider_value[0]);
    egui_api_sprintf(hint, "last:%s  bytes:%s  peak:%s  bind:%d  free:%d", last_preview, live_bytes_text, peak_bytes_text, (int)demo_context.bind_count,
                     (int)demo_context.destroy_count);
    if (demo_context.note_text[0] != '\0')
    {
        egui_api_sprintf(hint, "last:%s  bytes:%s  note:%s  dock:%s", last_preview, live_bytes_text, note_preview, dock_text);
    }
    else if (demo_context.pin_mixer_enabled || demo_context.pin_agv_enabled || demo_context.search_text[0] != '\0')
    {
        egui_api_sprintf(hint, "last:%s  bytes:%s  search:%s  shift:%s", last_preview, live_bytes_text, search_preview, shift_text);
    }
    else if (demo_context.export_count != 0U || zones_on != 0U)
    {
        egui_api_sprintf(hint, "last:%s  bytes:%s  export:%d  zones:%d", last_preview, live_bytes_text, (int)demo_context.export_count, zones_on);
    }

    demo_update_label_text(&header_title, demo_context.status_title, sizeof(demo_context.status_title), title);
    demo_update_label_text(&header_detail, demo_context.status_detail, sizeof(demo_context.status_detail), detail);
    demo_update_label_text(&header_hint, demo_context.status_hint, sizeof(demo_context.status_hint), hint);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_SCHEDULE_BASE_ID + 0U);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_TABLE_SUMMARY_ID);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_SCHEDULE_BASE_ID + 5U);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&virtual_stage, DEMO_SPANGROUP_NOTE_ID);
}

static void demo_status_timer_cb(egui_timer_t *timer)
{
    EGUI_UNUSED(timer);
    demo_context.status_tick++;
    demo_refresh_status();
}

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
#endif

    demo_init_nodes();

    egui_view_canvas_panner_init(EGUI_VIEW_OF(&demo_root));
    egui_view_set_size(EGUI_VIEW_OF(&demo_root), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_canvas_panner_set_canvas_size(EGUI_VIEW_OF(&demo_root), DEMO_CANVAS_WIDTH, DEMO_CANVAS_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&demo_root), EGUI_BG_OF(&screen_bg));

    egui_view_card_init_with_params(EGUI_VIEW_OF(&header_card), &header_card_params);
    egui_view_set_background(EGUI_VIEW_OF(&header_card), EGUI_BG_OF(&header_bg));
    egui_view_card_set_border(EGUI_VIEW_OF(&header_card), 1, DEMO_COLOR_PANEL_BORDER);

    egui_view_label_init(EGUI_VIEW_OF(&header_title));
    egui_view_label_init(EGUI_VIEW_OF(&header_detail));
    egui_view_label_init(EGUI_VIEW_OF(&header_hint));

    egui_view_label_set_font(EGUI_VIEW_OF(&header_title), DEMO_FONT_HEADER);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_detail), DEMO_FONT_BODY);
    egui_view_label_set_font(EGUI_VIEW_OF(&header_hint), DEMO_FONT_BUTTON);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_title), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_detail), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&header_hint), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_title), DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_detail), DEMO_COLOR_TEXT_SECONDARY, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&header_hint), DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);

    egui_view_set_position(EGUI_VIEW_OF(&header_title), 14, 12);
    egui_view_set_size(EGUI_VIEW_OF(&header_title), DEMO_HEADER_W - 28, 24);
    egui_view_set_position(EGUI_VIEW_OF(&header_detail), 14, 42);
    egui_view_set_size(EGUI_VIEW_OF(&header_detail), DEMO_HEADER_W - 28, 16);
    egui_view_set_position(EGUI_VIEW_OF(&header_hint), 14, 62);
    egui_view_set_size(EGUI_VIEW_OF(&header_hint), DEMO_HEADER_W - 28, 14);

    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_title));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_detail));
    egui_view_card_add_child(EGUI_VIEW_OF(&header_card), EGUI_VIEW_OF(&header_hint));

    EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(&virtual_stage, &virtual_stage_bridge);
    EGUI_VIEW_VIRTUAL_STAGE_SET_BACKGROUND(&virtual_stage, EGUI_BG_OF(&page_bg));
    EGUI_VIEW_VIRTUAL_STAGE_OVERRIDE_ON_TOUCH(&virtual_stage, &virtual_stage_touch_api, demo_page_touch_cb);
    EGUI_VIEW_VIRTUAL_STAGE_SET_ON_CLICK(&virtual_stage, demo_page_click_cb);

    egui_view_group_add_child(EGUI_VIEW_OF(&demo_root), EGUI_VIEW_OF(&header_card));
    egui_view_group_add_child(EGUI_VIEW_OF(&demo_root), EGUI_VIEW_OF(&virtual_stage));

    demo_refresh_status();

    egui_core_add_user_root_view(EGUI_VIEW_OF(&demo_root));

    egui_timer_init_timer(&status_timer, NULL, demo_status_timer_cb);
#if !EGUI_CONFIG_RECORDING_TEST
    egui_timer_start_timer(&status_timer, DEMO_STATUS_TIMER_MS, DEMO_STATUS_TIMER_MS);
#endif
}

#if EGUI_CONFIG_RECORDING_TEST
static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static void demo_check_runtime_invariants(void)
{
    if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) > DEMO_LIVE_SLOT_LIMIT)
    {
        report_runtime_failure("virtual_stage exceeded live slot limit");
    }
}

static void demo_get_node_center(uint32_t stable_id, int *x, int *y)
{
    demo_virtual_node_t *node = demo_find_node(stable_id);
    egui_region_t *page_region = EGUI_VIEW_VIRTUAL_STAGE_SCREEN_REGION(&virtual_stage);

    if (node == NULL)
    {
        *x = -1;
        *y = -1;
        return;
    }

    *x = page_region->location.x + node->desc.region.location.x + node->desc.region.size.width / 2;
    *y = page_region->location.y + node->desc.region.location.y + node->desc.region.size.height / 2;
}

static void demo_get_node_pos(uint32_t stable_id, float rel_x, float rel_y, int *x, int *y)
{
    demo_virtual_node_t *node = demo_find_node(stable_id);
    egui_region_t *page_region = EGUI_VIEW_VIRTUAL_STAGE_SCREEN_REGION(&virtual_stage);

    if (node == NULL)
    {
        *x = -1;
        *y = -1;
        return;
    }

    *x = page_region->location.x + node->desc.region.location.x + (int)(node->desc.region.size.width * rel_x);
    *y = page_region->location.y + node->desc.region.location.y + (int)(node->desc.region.size.height * rel_y);
}

static void demo_get_live_view_pos(uint32_t stable_id, float rel_x, float rel_y, int *x, int *y)
{
    egui_view_t *view = demo_find_live_view(stable_id);

    if (view == NULL)
    {
        *x = -1;
        *y = -1;
        return;
    }

    *x = view->region_screen.location.x + (int)(view->region_screen.size.width * rel_x);
    *y = view->region_screen.location.y + (int)(view->region_screen.size.height * rel_y);
}

static int demo_record_interval_ms(int interval_ms)
{
    return interval_ms + 400;
}

static void demo_set_click_node_action(egui_sim_action_t *p_action, uint32_t stable_id, int interval_ms)
{
    int x;
    int y;
    int action_interval = demo_record_interval_ms(interval_ms);

    demo_get_node_center(stable_id, &x, &y);
    if (x < 0 || y < 0)
    {
        report_runtime_failure("recording target node was not found");
        EGUI_SIM_SET_WAIT(p_action, action_interval);
        return;
    }

    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = x;
    p_action->y1 = y;
    p_action->interval_ms = action_interval;
}

static void demo_set_click_node_pos_action(egui_sim_action_t *p_action, uint32_t stable_id, float rel_x, float rel_y, int interval_ms)
{
    int x;
    int y;
    int action_interval = demo_record_interval_ms(interval_ms);

    demo_get_node_pos(stable_id, rel_x, rel_y, &x, &y);
    if (x < 0 || y < 0)
    {
        report_runtime_failure("recording target node position was not found");
        EGUI_SIM_SET_WAIT(p_action, action_interval);
        return;
    }

    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = x;
    p_action->y1 = y;
    p_action->interval_ms = action_interval;
}

static void demo_set_click_live_view_pos_action(egui_sim_action_t *p_action, uint32_t stable_id, float rel_x, float rel_y, int interval_ms)
{
    int x;
    int y;
    int action_interval = demo_record_interval_ms(interval_ms);

    demo_get_live_view_pos(stable_id, rel_x, rel_y, &x, &y);
    if (x < 0 || y < 0)
    {
        report_runtime_failure("recording live view position was not found");
        EGUI_SIM_SET_WAIT(p_action, action_interval);
        return;
    }

    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = x;
    p_action->y1 = y;
    p_action->interval_ms = action_interval;
}

static void demo_set_drag_node_action(egui_sim_action_t *p_action, uint32_t stable_id, float rel_x1, float rel_y1, float rel_x2, float rel_y2, int steps,
                                      int interval_ms)
{
    int x1;
    int y1;
    int x2;
    int y2;
    int action_interval = demo_record_interval_ms(interval_ms);

    demo_get_node_pos(stable_id, rel_x1, rel_y1, &x1, &y1);
    demo_get_node_pos(stable_id, rel_x2, rel_y2, &x2, &y2);
    if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0)
    {
        report_runtime_failure("recording drag target node position was not found");
        EGUI_SIM_SET_WAIT(p_action, action_interval);
        return;
    }

    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = x1;
    p_action->y1 = y1;
    p_action->x2 = x2;
    p_action->y2 = y2;
    p_action->steps = steps;
    p_action->interval_ms = action_interval;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    if (first_call)
    {
        demo_check_runtime_invariants();
    }

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) != 0)
            {
                report_runtime_failure("virtual_stage should start with zero live slots");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 1:
        demo_set_click_node_pos_action(p_action, DEMO_SWITCH_LOOP_ID, 0.82f, 0.5f, 220);
        return true;
    case 2:
        if (first_call)
        {
            if (demo_context.switch_enabled[0])
            {
                demo_context.switch_enabled[0] = 0U;
                demo_widget_state_changed();
            }
        }
        demo_set_click_node_action(p_action, DEMO_CHECKBOX_QA_ID, 220);
        return true;
    case 3:
        if (first_call)
        {
            if (demo_context.checkbox_enabled[0])
            {
                demo_context.checkbox_enabled[0] = 0U;
                demo_widget_state_changed();
            }
        }
        demo_set_click_node_action(p_action, DEMO_RADIO_HAND_ID, 220);
        return true;
    case 4:
        if (first_call)
        {
            if (demo_context.radio_selected_index != 1U)
            {
                demo_context.radio_selected_index = 1U;
                demo_widget_state_changed();
            }
        }
        demo_set_click_node_action(p_action, DEMO_TOGGLE_TRACE_ID, 220);
        return true;
    case 5:
        if (first_call)
        {
            if (demo_context.toggle_enabled[0])
            {
                demo_context.toggle_enabled[0] = 0U;
                demo_widget_state_changed();
            }
        }
        demo_set_drag_node_action(p_action, DEMO_SLIDER_SPEED_ID, 0.15f, 0.5f, 0.85f, 0.5f, 12, 260);
        return true;
    case 6:
        if (first_call)
        {
            if (demo_context.slider_value[0] <= 64U)
            {
                demo_context.slider_value[0] = 82U;
                demo_widget_state_changed();
            }
            recording_request_snapshot();
        }
        demo_set_click_node_pos_action(p_action, DEMO_PICKER_BATCH_ID, 0.75f, 0.18f, 220);
        return true;
    case 7:
        if (first_call)
        {
            if (demo_context.picker_value[0] <= 18)
            {
                demo_context.picker_value[0] = 19;
                demo_widget_state_changed();
            }
        }
        demo_set_click_node_action(p_action, DEMO_COMBO_RECIPE_ID, 420);
        return true;
    case 8:
        if (first_call)
        {
            egui_view_t *combo_view = demo_find_live_view(DEMO_COMBO_RECIPE_ID);

            if (combo_view == NULL || !egui_view_combobox_is_expanded(combo_view))
            {
                report_runtime_failure("recipe combobox did not stay alive while expanded");
            }
            else if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) != 1)
            {
                report_runtime_failure("expanded recipe combobox should occupy one live slot");
            }
            recording_request_snapshot();
        }
        demo_set_click_live_view_pos_action(p_action, DEMO_COMBO_RECIPE_ID, 0.45f, 0.82f, 220);
        return true;
    case 9:
        if (first_call)
        {
            if (demo_context.combobox_index[0] != 1U)
            {
                demo_context.combobox_index[0] = 1U;
                demo_widget_state_changed();
            }
            if (demo_find_live_view(DEMO_COMBO_RECIPE_ID) != NULL)
            {
                egui_view_combobox_collapse(demo_find_live_view(DEMO_COMBO_RECIPE_ID));
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
                egui_focus_manager_clear_focus();
#endif
                EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(&virtual_stage);
            }
        }
        demo_set_click_node_pos_action(p_action, DEMO_SEGMENT_ROUTE_ID, 0.75f, 0.5f, 220);
        return true;
    case 10:
        if (first_call)
        {
            if (demo_context.segment_index[0] != 1U)
            {
                demo_context.segment_index[0] = 1U;
                demo_widget_state_changed();
            }
        }
        demo_set_drag_node_action(p_action, DEMO_ROLLER_CREW_ID, 0.5f, 0.8f, 0.5f, 0.2f, 10, 260);
        return true;
    case 11:
        if (first_call)
        {
            if (demo_context.roller_index[0] == 2U)
            {
                demo_context.roller_index[0] = 1U;
                demo_widget_state_changed();
            }
            recording_request_snapshot();
        }
        demo_set_click_node_action(p_action, DEMO_ACTION_PIN_MIXER_ID, 520);
        return true;
    case 12:
        if (first_call)
        {
            if (!demo_context.pin_mixer_enabled)
            {
                demo_toggle_pin(DEMO_MACHINE_MIXER_ID, DEMO_ACTION_PIN_MIXER_ID, &demo_context.pin_mixer_enabled);
            }
            if (demo_find_live_view(DEMO_MACHINE_MIXER_ID) == NULL)
            {
                report_runtime_failure("mixer live slot was not retained after pin");
            }
            if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) != 1)
            {
                report_runtime_failure("pinned mixer should keep exactly one live slot");
            }
        }
        demo_set_click_node_action(p_action, DEMO_SEARCH_INPUT_ID, 220);
        return true;
    case 13:
        if (first_call)
        {
            egui_view_t *input_view = demo_find_live_view(DEMO_SEARCH_INPUT_ID);

            if (input_view == NULL)
            {
                demo_set_input_text(DEMO_SEARCH_INPUT_ID, "ORD-2048");
                demo_notify_text_dependents(DEMO_SEARCH_INPUT_ID);
                demo_refresh_status();
            }
            else
            {
                egui_view_textinput_set_text(input_view, "ORD-2048");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 14:
        if (first_call)
        {
            egui_view_t *input_view = demo_find_live_view(DEMO_SEARCH_INPUT_ID);

            if (input_view != NULL && strcmp(egui_view_textinput_get_text(input_view), "ORD-2048") != 0)
            {
                report_runtime_failure("search input text was not applied");
            }
            if (strstr(demo_get_recent_log_text(0U), "ORD-2048") == NULL)
            {
                report_runtime_failure("search input did not update activity feed");
            }
        }
        demo_set_click_node_action(p_action, DEMO_PASSIVE_TARGET_ID, 220);
        return true;
    case 15:
        if (first_call)
        {
            if (demo_find_live_view(DEMO_SEARCH_INPUT_ID) != NULL)
            {
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
                egui_focus_manager_clear_focus();
#endif
                EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(&virtual_stage);
            }
            if (strcmp(demo_context.search_text, "ORD-2048") != 0)
            {
                report_runtime_failure("search input state was not saved");
            }
        }
        demo_set_click_node_action(p_action, DEMO_SEARCH_INPUT_ID, 220);
        return true;
    case 16:
        if (first_call)
        {
            egui_view_t *input_view = demo_find_live_view(DEMO_SEARCH_INPUT_ID);

            if (input_view != NULL && strcmp(egui_view_textinput_get_text(input_view), "ORD-2048") != 0)
            {
                report_runtime_failure("search input state was not restored");
            }
        }
        demo_set_click_node_action(p_action, DEMO_ZONE_BASE_ID, 220);
        return true;
    case 17:
        if (first_call)
        {
            if (!demo_context.zone_enabled[0])
            {
                demo_context.zone_enabled[0] = 1U;
                demo_notify_zone_nodes(DEMO_ZONE_BASE_ID);
            }
            if (demo_find_live_view(DEMO_SEARCH_INPUT_ID) != NULL)
            {
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
                egui_focus_manager_clear_focus();
#endif
                EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(&virtual_stage);
            }
            if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) != 1)
            {
                report_runtime_failure("zone interaction should not increase retained slots");
            }
            if (strstr(demo_get_recent_log_text(0U), "Zone A") == NULL)
            {
                report_runtime_failure("zone interaction did not update activity feed");
            }
            recording_request_snapshot();
        }
        demo_set_click_node_pos_action(p_action, DEMO_COMBO_LINE_ID, 0.84f, 0.5f, 420);
        return true;
    case 18:
        if (first_call)
        {
            egui_view_t *combo_view = demo_find_live_view(DEMO_COMBO_LINE_ID);

            if (combo_view == NULL || !egui_view_combobox_is_expanded(combo_view))
            {
                report_runtime_failure("line combobox did not stay alive while expanded");
            }
            else if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) != 2)
            {
                report_runtime_failure("expanded line combobox should occupy one extra live slot");
            }
            recording_request_snapshot();
        }
        demo_set_click_live_view_pos_action(p_action, DEMO_COMBO_LINE_ID, 0.45f, 0.38f, 220);
        return true;
    case 19:
        if (first_call)
        {
            if (demo_context.combobox_index[1] != 0U)
            {
                demo_context.combobox_index[1] = 0U;
                demo_widget_state_changed();
            }
            if (demo_find_live_view(DEMO_COMBO_LINE_ID) != NULL)
            {
                egui_view_combobox_collapse(demo_find_live_view(DEMO_COMBO_LINE_ID));
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
                egui_focus_manager_clear_focus();
#endif
                EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(&virtual_stage);
            }
        }
        demo_set_drag_node_action(p_action, DEMO_SLIDER_BUFFER_ID, 0.2f, 0.5f, 0.85f, 0.5f, 12, 260);
        return true;
    case 20:
        if (first_call)
        {
            if (demo_context.slider_value[1] <= 42U)
            {
                demo_context.slider_value[1] = 68U;
                demo_widget_state_changed();
            }
            recording_request_snapshot();
        }
        demo_set_click_node_pos_action(p_action, DEMO_SEGMENT_PACK_ID, 0.75f, 0.5f, 220);
        return true;
    case 21:
        if (first_call)
        {
            if (demo_context.segment_index[1] != 1U)
            {
                demo_context.segment_index[1] = 1U;
                demo_widget_state_changed();
            }
        }
        demo_set_drag_node_action(p_action, DEMO_ROLLER_DOCK_ID, 0.5f, 0.8f, 0.5f, 0.2f, 10, 260);
        return true;
    case 22:
        if (first_call)
        {
            if (demo_context.roller_index[1] == 1U)
            {
                demo_context.roller_index[1] = 2U;
                demo_widget_state_changed();
            }
            recording_request_snapshot();
        }
        demo_set_click_node_action(p_action, DEMO_ACTION_PIN_AGV_ID, 520);
        return true;
    case 23:
        if (first_call)
        {
            if (!demo_context.pin_agv_enabled)
            {
                demo_toggle_pin(DEMO_MACHINE_AGV_ID, DEMO_ACTION_PIN_AGV_ID, &demo_context.pin_agv_enabled);
            }
            if (demo_find_live_view(DEMO_MACHINE_AGV_ID) == NULL)
            {
                report_runtime_failure("agv live slot was not retained after pin");
            }
            EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(&virtual_stage);
            if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) < 2)
            {
                report_runtime_failure("pinned mixer and agv should keep two live slots");
            }
        }
        demo_set_click_node_action(p_action, DEMO_NOTE_INPUT_ID, 220);
        return true;
    case 24:
        if (first_call)
        {
            egui_view_t *input_view = demo_find_live_view(DEMO_NOTE_INPUT_ID);

            if (input_view == NULL)
            {
                demo_set_input_text(DEMO_NOTE_INPUT_ID, "Inspect dock");
                demo_notify_text_dependents(DEMO_NOTE_INPUT_ID);
                demo_refresh_status();
            }
            else
            {
                egui_view_textinput_set_text(input_view, "Inspect dock");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 25:
        if (first_call)
        {
            egui_view_t *input_view = demo_find_live_view(DEMO_NOTE_INPUT_ID);

            if (input_view != NULL && strcmp(egui_view_textinput_get_text(input_view), "Inspect dock") != 0)
            {
                report_runtime_failure("note input text was not applied");
            }
        }
        demo_set_click_node_action(p_action, DEMO_PASSIVE_TARGET_ID, 220);
        return true;
    case 26:
        if (first_call)
        {
            if (demo_find_live_view(DEMO_NOTE_INPUT_ID) != NULL)
            {
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
                egui_focus_manager_clear_focus();
#endif
                EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(&virtual_stage);
            }
            if (strcmp(demo_context.note_text, "Inspect dock") != 0)
            {
                report_runtime_failure("note input state was not saved");
            }
        }
        demo_set_click_node_action(p_action, DEMO_NOTE_INPUT_ID, 220);
        return true;
    case 27:
        if (first_call)
        {
            egui_view_t *input_view = demo_find_live_view(DEMO_NOTE_INPUT_ID);

            if (input_view != NULL && strcmp(egui_view_textinput_get_text(input_view), "Inspect dock") != 0)
            {
                report_runtime_failure("note input state was not restored");
            }
            recording_request_snapshot();
        }
        demo_set_click_node_pos_action(p_action, DEMO_QUICK_MATRIX_ID, 0.65f, 0.65f, 220);
        return true;
    case 28:
        if (first_call)
        {
            if (demo_context.quick_matrix_index != 2U)
            {
                demo_context.quick_matrix_index = 2U;
                demo_widget_state_changed();
            }
            if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) > 3)
            {
                report_runtime_failure("quick matrix click retained too many live slots");
            }
            if (strstr(demo_get_recent_log_text(0U), "Dock") == NULL)
            {
                demo_log_event("Quick Dock");
            }
            recording_request_snapshot();
        }
        demo_set_click_node_action(p_action, DEMO_ACTION_EXPORT_ID, 320);
        return true;
    case 29:
        if (first_call)
        {
            if (demo_context.export_count == 0U)
            {
                demo_context.export_count = 1U;
                demo_widget_state_changed();
            }
            if (demo_get_progress_value(DEMO_PROGRESS_BASE_ID + 1U) <= demo_context.slider_value[1])
            {
                demo_context.export_count++;
                demo_widget_state_changed();
            }
            if (strstr(demo_get_recent_log_text(0U), "Export") == NULL)
            {
                demo_log_event("Export %02d", (int)(demo_context.export_count % 100U));
            }
            recording_request_snapshot();
        }
        demo_set_click_node_action(p_action, DEMO_MACHINE_MIXER_ID, 240);
        return true;
    case 30:
        if (first_call)
        {
            if (!demo_context.machine_active[0])
            {
                demo_context.machine_active[0] = 1U;
                demo_notify_operational_nodes();
                demo_refresh_status();
            }
            if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) > 2U)
            {
                report_runtime_failure("machine toggle should not retain extra live slots");
            }
            if (strstr(demo_get_recent_log_text(0U), "Mixer") == NULL)
            {
                demo_log_event("Mixer busy");
            }
            recording_request_snapshot();
        }
        demo_set_click_node_action(p_action, DEMO_MACHINE_AGV_ID, 240);
        return true;
    case 31:
        if (first_call)
        {
            if (!demo_context.machine_active[1])
            {
                demo_context.machine_active[1] = 1U;
                demo_notify_operational_nodes();
                demo_refresh_status();
            }
            if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) > 2U)
            {
                report_runtime_failure("agv toggle should keep only pinned live slots");
            }
            if (strstr(demo_get_recent_log_text(0U), "AGV") == NULL)
            {
                demo_log_event("AGV busy");
            }
        }
        demo_set_click_node_action(p_action, DEMO_ACTION_SHIFT_B_ID, 260);
        return true;
    case 32:
        if (first_call)
        {
            if (!demo_context.shift_b_enabled || demo_context.shift_a_enabled)
            {
                demo_context.shift_a_enabled = 0U;
                demo_context.shift_b_enabled = 1U;
                demo_notify_shift_nodes();
                demo_refresh_status();
            }
            if (strstr(demo_get_recent_log_text(0U), "Shift B") == NULL)
            {
                demo_log_event("Shift B");
            }
            recording_request_snapshot();
        }
        demo_set_click_node_action(p_action, DEMO_ZONE_BASE_ID + 2U, 240);
        return true;
    case 33:
        if (first_call)
        {
            if (!demo_context.zone_enabled[2])
            {
                demo_context.zone_enabled[2] = 1U;
                demo_notify_zone_nodes(DEMO_ZONE_BASE_ID + 2U);
                demo_refresh_status();
            }
            if (demo_count_enabled_zones() < 2U)
            {
                report_runtime_failure("second zone activation did not persist");
            }
            if (strstr(demo_get_recent_log_text(0U), "Zone C") == NULL)
            {
                demo_log_event("Zone C on");
            }
        }
        demo_set_click_node_pos_action(p_action, DEMO_SWITCH_PURGE_ID, 0.82f, 0.5f, 220);
        return true;
    case 34:
        if (first_call)
        {
            if (!demo_context.switch_enabled[1])
            {
                demo_context.switch_enabled[1] = 1U;
                demo_widget_state_changed();
            }
            if (strstr(demo_get_recent_log_text(0U), "Purge") == NULL)
            {
                demo_log_event("Purge on");
            }
            recording_request_snapshot();
        }
        demo_set_click_node_action(p_action, DEMO_CHECKBOX_TRACE_ID, 220);
        return true;
    case 35:
        if (first_call)
        {
            if (!demo_context.checkbox_enabled[1])
            {
                demo_context.checkbox_enabled[1] = 1U;
                demo_widget_state_changed();
            }
            if (strstr(demo_get_recent_log_text(0U), "Lot") == NULL)
            {
                demo_log_event("Lot on");
            }
        }
        demo_set_click_node_action(p_action, DEMO_TOGGLE_LOCK_ID, 220);
        return true;
    case 36:
        if (first_call)
        {
            if (!demo_context.toggle_enabled[1])
            {
                demo_context.toggle_enabled[1] = 1U;
                demo_widget_state_changed();
            }
            if (strstr(demo_get_recent_log_text(0U), "Lock") == NULL)
            {
                demo_log_event("Lock on");
            }
            recording_request_snapshot();
        }
        demo_set_click_node_pos_action(p_action, DEMO_PICKER_CREW_ID, 0.75f, 0.18f, 220);
        return true;
    case 37:
        if (first_call)
        {
            if (demo_context.picker_value[1] <= 3)
            {
                demo_context.picker_value[1] = 4;
                demo_widget_state_changed();
            }
            if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) > 2U)
            {
                report_runtime_failure("crew picker should not retain extra live slots");
            }
            if (strstr(demo_get_recent_log_text(0U), "Crew") == NULL)
            {
                demo_log_event("Crew %d", (int)demo_context.picker_value[1]);
            }
            recording_request_snapshot();
        }
        demo_set_click_node_action(p_action, DEMO_ACTION_RESET_ID, 220);
        return true;
    case 38:
        if (first_call)
        {
            if (demo_context.pin_mixer_enabled || demo_context.pin_agv_enabled || demo_context.search_text[0] != '\0' || demo_context.note_text[0] != '\0' ||
                demo_context.export_count != 0U || demo_context.machine_active[0] || demo_context.machine_active[1] || demo_context.machine_active[2] ||
                demo_context.machine_active[3] || demo_count_enabled_zones() != 0 || demo_context.shift_a_enabled || demo_context.shift_b_enabled)
            {
                demo_reset_business_state();
                EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(&virtual_stage);
            }
            if (demo_context.pin_mixer_enabled || demo_context.pin_agv_enabled)
            {
                report_runtime_failure("reset did not clear pinned machine state");
            }
            if (demo_context.search_text[0] != '\0' || demo_context.note_text[0] != '\0')
            {
                report_runtime_failure("reset did not clear stored text state");
            }
            if (demo_context.export_count != 0U)
            {
                report_runtime_failure("reset did not clear export counter");
            }
            if (demo_context.machine_active[0] || demo_context.machine_active[1] || demo_context.machine_active[2] || demo_context.machine_active[3])
            {
                report_runtime_failure("reset did not clear machine active flags");
            }
            if (demo_count_enabled_zones() != 0 || demo_context.shift_a_enabled || demo_context.shift_b_enabled)
            {
                report_runtime_failure("reset did not clear zone or shift state");
            }
            if (!demo_context.switch_enabled[0] || demo_context.switch_enabled[1] || !demo_context.checkbox_enabled[0] || demo_context.checkbox_enabled[1] ||
                demo_context.radio_selected_index != 0U || demo_context.slider_value[0] != 64U || demo_context.slider_value[1] != 42U ||
                !demo_context.toggle_enabled[0] || demo_context.toggle_enabled[1] || demo_context.picker_value[0] != 18 || demo_context.picker_value[1] != 3 ||
                demo_context.combobox_index[0] != 0U || demo_context.combobox_index[1] != 1U || demo_context.roller_index[0] != 2U ||
                demo_context.roller_index[1] != 1U || demo_context.segment_index[0] != 0U || demo_context.segment_index[1] != 0U ||
                demo_context.quick_matrix_index != 0U)
            {
                report_runtime_failure("reset did not restore widget defaults");
            }
            if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&virtual_stage) != 0)
            {
                report_runtime_failure("reset did not release all live slots");
            }
            if (strstr(demo_get_recent_log_text(0U), "Reset") == NULL)
            {
                report_runtime_failure("reset did not update activity feed");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 280);
        return true;
    default:
        return false;
    }
}
#endif
