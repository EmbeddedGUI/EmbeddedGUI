#include <stdio.h>
#include <string.h>

#include "egui.h"
#include "egui_view_toggle_button.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define TOGGLE_BUTTON_ROOT_WIDTH           224
#define TOGGLE_BUTTON_ROOT_HEIGHT          254
#define TOGGLE_BUTTON_PRIMARY_WIDTH        184
#define TOGGLE_BUTTON_PRIMARY_HEIGHT       54
#define TOGGLE_BUTTON_COMPACT_WIDTH        100
#define TOGGLE_BUTTON_COMPACT_HEIGHT       54
#define TOGGLE_BUTTON_BOTTOM_ROW_WIDTH     208
#define TOGGLE_BUTTON_BOTTOM_ROW_HEIGHT    66
#define TOGGLE_BUTTON_RECORD_WAIT          90
#define TOGGLE_BUTTON_RECORD_FRAME_WAIT    170
#define TOGGLE_BUTTON_STATUS_MIX           6
#define TOGGLE_BUTTON_GUIDE_COLOR          0x94A0A8
#define TOGGLE_BUTTON_LABEL_COLOR          0x7F8992
#define TOGGLE_BUTTON_STATUS_COLOR         0x98A1AA
#define TOGGLE_BUTTON_COMPACT_LABEL_COLOR  0x37625F
#define TOGGLE_BUTTON_READONLY_LABEL_COLOR 0x7D8891

typedef struct toggle_button_snapshot toggle_button_snapshot_t;
struct toggle_button_snapshot
{
    const char *text;
    const char *icon;
    const char *status_prefix;
    egui_color_t on_color;
    egui_color_t off_color;
    uint8_t toggled;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_toggle_button_t button_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_toggle_button_t button_compact;
static egui_view_linearlayout_t readonly_column;
static egui_view_label_t readonly_label;
static egui_view_toggle_button_t button_readonly;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF7F8FA), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Toggle Button";
static const char *guide_text = "Tap guide to rotate";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {144, 0}};

static const toggle_button_snapshot_t primary_snapshots[] = {
        {"Alerts", EGUI_ICON_MS_NOTIFICATIONS, "Alerts", EGUI_COLOR_HEX(0x2563EB), EGUI_COLOR_HEX(0xEAF1FB), 1},
        {"Visible", EGUI_ICON_MS_VISIBILITY, "Visible", EGUI_COLOR_HEX(0x0C7C73), EGUI_COLOR_HEX(0xE7F4F1), 0},
        {"Favorite", EGUI_ICON_MS_HEART, "Favorite", EGUI_COLOR_HEX(0xC0567C), EGUI_COLOR_HEX(0xF8EBF1), 1},
};

static const toggle_button_snapshot_t compact_snapshots[] = {
        {"Alerts", EGUI_ICON_MS_NOTIFICATIONS, "Compact alerts", EGUI_COLOR_HEX(0x0C7C73), EGUI_COLOR_HEX(0xDBEAE5), 0},
        {"Visible", EGUI_ICON_MS_VISIBILITY, "Compact visible", EGUI_COLOR_HEX(0x2563EB), EGUI_COLOR_HEX(0xEAF1FB), 1},
};

static const toggle_button_snapshot_t readonly_snapshot = {
        "Pinned", EGUI_ICON_MS_HEART, "Read only", EGUI_COLOR_HEX(0xAFB8C3), EGUI_COLOR_HEX(0xF3F6F8), 1,
};

static uint8_t current_primary_snapshot = 0;
static uint8_t current_compact_snapshot = 0;
static char status_buffer[64];

static void set_status_text(const char *prefix, uint8_t toggled, egui_color_t accent_color)
{
    egui_color_t mixed_color = toggled ? egui_rgb_mix(EGUI_COLOR_HEX(TOGGLE_BUTTON_STATUS_COLOR), accent_color, TOGGLE_BUTTON_STATUS_MIX)
                                       : EGUI_COLOR_HEX(TOGGLE_BUTTON_STATUS_COLOR);

    snprintf(status_buffer, sizeof(status_buffer), "%s: %s", prefix, toggled ? "On" : "Off");
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_buffer);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), mixed_color, EGUI_ALPHA_100);
}

static void apply_snapshot_to_button(egui_view_toggle_button_t *button, const toggle_button_snapshot_t *snapshot)
{
    button->on_color = snapshot->on_color;
    button->off_color = snapshot->off_color;
    egui_view_toggle_button_set_icon(EGUI_VIEW_OF(button), snapshot->icon);
    egui_view_toggle_button_set_text(EGUI_VIEW_OF(button), snapshot->text);
    egui_view_toggle_button_set_toggled(EGUI_VIEW_OF(button), snapshot->toggled);
}

static void update_primary_status(uint8_t toggled)
{
    const toggle_button_snapshot_t *snapshot = &primary_snapshots[current_primary_snapshot];

    set_status_text(snapshot->status_prefix, toggled, snapshot->on_color);
}

static void update_compact_status(uint8_t toggled)
{
    const toggle_button_snapshot_t *snapshot = &compact_snapshots[current_compact_snapshot];

    set_status_text(snapshot->status_prefix, toggled, snapshot->on_color);
}

static void apply_primary_snapshot(uint8_t snapshot_index)
{
    current_primary_snapshot = snapshot_index;
    apply_snapshot_to_button(&button_primary, &primary_snapshots[snapshot_index]);
    update_primary_status(primary_snapshots[snapshot_index].toggled);
}

static void apply_compact_snapshot(uint8_t snapshot_index, uint8_t update_status)
{
    current_compact_snapshot = snapshot_index;
    apply_snapshot_to_button(&button_compact, &compact_snapshots[snapshot_index]);
    if (update_status)
    {
        update_compact_status(compact_snapshots[snapshot_index].toggled);
    }
}

static void apply_read_only_snapshot(void)
{
    apply_snapshot_to_button(&button_readonly, &readonly_snapshot);
}

static void on_primary_toggled(egui_view_t *self, uint8_t is_toggled)
{
    EGUI_UNUSED(self);
    update_primary_status(is_toggled);
}

static void on_compact_toggled(egui_view_t *self, uint8_t is_toggled)
{
    EGUI_UNUSED(self);
    update_compact_status(is_toggled);
}

static void on_guide_click(egui_view_t *self)
{
    uint8_t next = (uint8_t)((current_primary_snapshot + 1) % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0])));

    EGUI_UNUSED(self);
    apply_primary_snapshot(next);
}

static void on_compact_label_click(egui_view_t *self)
{
    uint8_t next = (uint8_t)((current_compact_snapshot + 1) % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0])));

    EGUI_UNUSED(self);
    apply_compact_snapshot(next, 1);
}

static int consume_preview_touch(egui_view_t *self, egui_motion_event_t *event)
{
    if (event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
    {
        egui_view_set_pressed(self, 0);
    }
    return 1;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int consume_preview_key(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(event);
    return 1;
}
#endif

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), TOGGLE_BUTTON_ROOT_WIDTH, TOGGLE_BUTTON_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), TOGGLE_BUTTON_ROOT_WIDTH, 16);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x2A3744), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 5, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), TOGGLE_BUTTON_ROOT_WIDTH, 10);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(TOGGLE_BUTTON_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 1);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), 1);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), TOGGLE_BUTTON_ROOT_WIDTH, 10);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(TOGGLE_BUTTON_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_toggle_button_init(EGUI_VIEW_OF(&button_primary));
    egui_view_set_size(EGUI_VIEW_OF(&button_primary), TOGGLE_BUTTON_PRIMARY_WIDTH, TOGGLE_BUTTON_PRIMARY_HEIGHT);
    egui_view_toggle_button_set_font(EGUI_VIEW_OF(&button_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&button_primary), EGUI_FONT_ICON_MS_16);
    hcw_toggle_button_apply_standard_style(EGUI_VIEW_OF(&button_primary));
    egui_view_set_padding(EGUI_VIEW_OF(&button_primary), 2, 2, 1, 0);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&button_primary), on_primary_toggled);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&button_primary), 1);
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    static egui_view_api_t button_primary_key_api;
    egui_view_override_api_on_key(EGUI_VIEW_OF(&button_primary), &button_primary_key_api, hcw_toggle_button_on_key_event);
#endif
    egui_view_set_margin(EGUI_VIEW_OF(&button_primary), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&button_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), TOGGLE_BUTTON_ROOT_WIDTH, 9);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Alerts: On");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(TOGGLE_BUTTON_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 140, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xEFF3F6));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), TOGGLE_BUTTON_BOTTOM_ROW_WIDTH, TOGGLE_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), TOGGLE_BUTTON_COMPACT_WIDTH, TOGGLE_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), TOGGLE_BUTTON_COMPACT_WIDTH, 10);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(TOGGLE_BUTTON_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 2);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), 1);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_toggle_button_init(EGUI_VIEW_OF(&button_compact));
    egui_view_set_size(EGUI_VIEW_OF(&button_compact), TOGGLE_BUTTON_COMPACT_WIDTH, TOGGLE_BUTTON_COMPACT_HEIGHT);
    egui_view_toggle_button_set_font(EGUI_VIEW_OF(&button_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&button_compact), EGUI_FONT_ICON_MS_16);
    hcw_toggle_button_apply_compact_style(EGUI_VIEW_OF(&button_compact));
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&button_compact), on_compact_toggled);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&button_compact), 1);
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    static egui_view_api_t button_compact_key_api;
    egui_view_override_api_on_key(EGUI_VIEW_OF(&button_compact), &button_compact_key_api, hcw_toggle_button_on_key_event);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&button_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&readonly_column));
    egui_view_set_size(EGUI_VIEW_OF(&readonly_column), TOGGLE_BUTTON_COMPACT_WIDTH, TOGGLE_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&readonly_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&readonly_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&readonly_column));

    egui_view_label_init(EGUI_VIEW_OF(&readonly_label));
    egui_view_set_size(EGUI_VIEW_OF(&readonly_label), TOGGLE_BUTTON_COMPACT_WIDTH, 10);
    egui_view_label_set_text(EGUI_VIEW_OF(&readonly_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&readonly_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&readonly_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&readonly_label), EGUI_COLOR_HEX(TOGGLE_BUTTON_READONLY_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&readonly_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&readonly_column), EGUI_VIEW_OF(&readonly_label));

    egui_view_toggle_button_init(EGUI_VIEW_OF(&button_readonly));
    egui_view_set_size(EGUI_VIEW_OF(&button_readonly), TOGGLE_BUTTON_COMPACT_WIDTH, TOGGLE_BUTTON_COMPACT_HEIGHT);
    egui_view_toggle_button_set_font(EGUI_VIEW_OF(&button_readonly), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&button_readonly), EGUI_FONT_ICON_MS_16);
    hcw_toggle_button_apply_read_only_style(EGUI_VIEW_OF(&button_readonly));
    static egui_view_api_t button_readonly_api;
    egui_view_override_api_on_touch(EGUI_VIEW_OF(&button_readonly), &button_readonly_api, consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    egui_view_override_api_on_key(EGUI_VIEW_OF(&button_readonly), &button_readonly_api, consume_preview_key);
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&button_readonly), 0);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&readonly_column), EGUI_VIEW_OF(&button_readonly));

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&readonly_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

    apply_primary_snapshot(0);
    apply_compact_snapshot(0, 0);
    apply_read_only_snapshot();
}

#if EGUI_CONFIG_RECORDING_TEST
static void apply_primary_key(uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    EGUI_VIEW_OF(&button_primary)->api->dispatch_key_event(EGUI_VIEW_OF(&button_primary), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    EGUI_VIEW_OF(&button_primary)->api->dispatch_key_event(EGUI_VIEW_OF(&button_primary), &event);
}

static void apply_primary_focus(uint8_t focused)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (focused)
    {
        egui_view_request_focus(EGUI_VIEW_OF(&button_primary));
    }
    else
    {
        egui_view_clear_focus(EGUI_VIEW_OF(&button_primary));
    }
#else
    EGUI_UNUSED(focused);
#endif
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_BUTTON_RECORD_WAIT);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_BUTTON_RECORD_FRAME_WAIT);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_focus(1);
            apply_primary_key(EGUI_KEY_CODE_SPACE);
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_BUTTON_RECORD_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_BUTTON_RECORD_FRAME_WAIT);
        return true;
    case 4:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &guide_label, TOGGLE_BUTTON_RECORD_WAIT);
        return true;
    case 5:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_BUTTON_RECORD_FRAME_WAIT);
        return true;
    case 6:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_ENTER);
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_BUTTON_RECORD_WAIT);
        return true;
    case 7:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_BUTTON_RECORD_FRAME_WAIT);
        return true;
    case 8:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &compact_label, TOGGLE_BUTTON_RECORD_WAIT);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_BUTTON_RECORD_FRAME_WAIT);
        return true;
    case 10:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &guide_label, TOGGLE_BUTTON_RECORD_WAIT);
        return true;
    case 11:
        if (first_call)
        {
            apply_primary_focus(0);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 520);
        return true;
    default:
        return false;
    }
}
#endif
