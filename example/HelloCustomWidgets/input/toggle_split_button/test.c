#include <string.h>

#include "egui.h"
#include "egui_view_toggle_split_button.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define TOGGLE_SPLIT_BUTTON_ROOT_WIDTH          224
#define TOGGLE_SPLIT_BUTTON_ROOT_HEIGHT         292
#define TOGGLE_SPLIT_BUTTON_PRIMARY_WIDTH       196
#define TOGGLE_SPLIT_BUTTON_PRIMARY_HEIGHT      76
#define TOGGLE_SPLIT_BUTTON_PREVIEW_WIDTH       106
#define TOGGLE_SPLIT_BUTTON_PREVIEW_HEIGHT      68
#define TOGGLE_SPLIT_BUTTON_BOTTOM_ROW_WIDTH    218
#define TOGGLE_SPLIT_BUTTON_BOTTOM_ROW_HEIGHT   84
#define TOGGLE_SPLIT_BUTTON_GUIDE_COLOR         0x7A8896
#define TOGGLE_SPLIT_BUTTON_LABEL_COLOR         0x74808D
#define TOGGLE_SPLIT_BUTTON_STATUS_COLOR        0x4E677E
#define TOGGLE_SPLIT_BUTTON_COMPACT_LABEL_COLOR 0x0E776E
#define TOGGLE_SPLIT_BUTTON_LOCKED_LABEL_COLOR  0x8994A0
#define TOGGLE_SPLIT_BUTTON_RECORD_WAIT         110
#define TOGGLE_SPLIT_BUTTON_RECORD_FRAME_WAIT   150

typedef struct toggle_split_button_snapshot toggle_split_button_snapshot_t;
struct toggle_split_button_snapshot
{
    const char *prefix;
    egui_color_t status_color;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_toggle_split_button_t button_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_toggle_split_button_t button_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_toggle_split_button_t button_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Toggle Split Button";
static const char *guide_text = "Tap main to toggle, chevron to change mode";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[72] = "Alert routing: On / Primary";

static const egui_view_toggle_split_button_snapshot_t primary_snapshots[] = {
        {"Alert routing", "AL", "Alerts", "Keep critical alerts armed while the menu changes mode", EGUI_VIEW_TOGGLE_SPLIT_BUTTON_TONE_ACCENT, 1, 1, 1,
         EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY},
        {"Sync monitor", "SY", "Sync", "Arm background sync and use the menu to change the channel", EGUI_VIEW_TOGGLE_SPLIT_BUTTON_TONE_SUCCESS, 0, 1, 1,
         EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU},
        {"Follow thread", "FW", "Follow", "Stay subscribed but pick another route from the menu", EGUI_VIEW_TOGGLE_SPLIT_BUTTON_TONE_WARNING, 1, 1, 1,
         EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY},
        {"Record scene", "RC", "Record", "Arm capture and swap destination from the split menu", EGUI_VIEW_TOGGLE_SPLIT_BUTTON_TONE_DANGER, 0, 1, 1,
         EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU},
};

static const egui_view_toggle_split_button_snapshot_t compact_snapshots[] = {
        {"Quick", "QK", "Quick", "Compact preset", EGUI_VIEW_TOGGLE_SPLIT_BUTTON_TONE_ACCENT, 1, 1, 1, EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY},
        {"Review", "RV", "Review", "Compact preset", EGUI_VIEW_TOGGLE_SPLIT_BUTTON_TONE_NEUTRAL, 0, 1, 1, EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU},
};

static const egui_view_toggle_split_button_snapshot_t locked_snapshot = {
        "Locked", "LK", "Publish", "Visible but read only", EGUI_VIEW_TOGGLE_SPLIT_BUTTON_TONE_NEUTRAL, 1, 1, 1, EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY};

static const toggle_split_button_snapshot_t status_snapshots[] = {
        {"Alert routing", EGUI_COLOR_HEX(0x2563EB)},
        {"Sync monitor", EGUI_COLOR_HEX(0x178454)},
        {"Follow thread", EGUI_COLOR_HEX(0xB87A16)},
        {"Record scene", EGUI_COLOR_HEX(0xB13A35)},
};

static void update_status(egui_view_t *self, uint8_t part)
{
    int pos = 0;
    uint8_t index = egui_view_toggle_split_button_get_current_snapshot(self);
    uint8_t checked = egui_view_toggle_split_button_get_checked(self);
    const char *part_text = part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU ? "Menu" : "Primary";

    pos += egui_sprintf_str(status_text, sizeof(status_text), status_snapshots[index].prefix);
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, ": ");
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, checked ? "On" : "Off");
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " / ");
    egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, part_text);

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(status_snapshots[index].status_color, EGUI_COLOR_HEX(0x56687A), 26),
                                   EGUI_ALPHA_100);
}

static void on_button_changed(egui_view_t *self, uint8_t snapshot_index, uint8_t checked, uint8_t part)
{
    EGUI_UNUSED(snapshot_index);
    EGUI_UNUSED(checked);
    update_status(self, part);
}

static void on_guide_click(egui_view_t *self)
{
    uint8_t next = (uint8_t)((egui_view_toggle_split_button_get_current_snapshot(EGUI_VIEW_OF(&button_primary)) + 1) % 4);

    EGUI_UNUSED(self);
    egui_view_toggle_split_button_set_current_snapshot(EGUI_VIEW_OF(&button_primary), next);
    update_status(EGUI_VIEW_OF(&button_primary), egui_view_toggle_split_button_get_current_part(EGUI_VIEW_OF(&button_primary)));
}

static void on_compact_label_click(egui_view_t *self)
{
    uint8_t next = (uint8_t)((egui_view_toggle_split_button_get_current_snapshot(EGUI_VIEW_OF(&button_compact)) + 1) % 2);

    EGUI_UNUSED(self);
    egui_view_toggle_split_button_set_current_snapshot(EGUI_VIEW_OF(&button_compact), next);
}

static int consume_preview_touch(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(event);
    return 1;
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), TOGGLE_SPLIT_BUTTON_ROOT_WIDTH, TOGGLE_SPLIT_BUTTON_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), TOGGLE_SPLIT_BUTTON_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 9, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), TOGGLE_SPLIT_BUTTON_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(TOGGLE_SPLIT_BUTTON_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), TOGGLE_SPLIT_BUTTON_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(TOGGLE_SPLIT_BUTTON_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_toggle_split_button_init(EGUI_VIEW_OF(&button_primary));
    egui_view_set_size(EGUI_VIEW_OF(&button_primary), TOGGLE_SPLIT_BUTTON_PRIMARY_WIDTH, TOGGLE_SPLIT_BUTTON_PRIMARY_HEIGHT);
    egui_view_toggle_split_button_set_font(EGUI_VIEW_OF(&button_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_toggle_split_button_set_meta_font(EGUI_VIEW_OF(&button_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_toggle_split_button_set_snapshots(EGUI_VIEW_OF(&button_primary), primary_snapshots, 4);
    egui_view_toggle_split_button_set_on_changed_listener(EGUI_VIEW_OF(&button_primary), on_button_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&button_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&button_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), TOGGLE_SPLIT_BUTTON_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(TOGGLE_SPLIT_BUTTON_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 148, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDFE7EE));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), TOGGLE_SPLIT_BUTTON_BOTTOM_ROW_WIDTH, TOGGLE_SPLIT_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), TOGGLE_SPLIT_BUTTON_PREVIEW_WIDTH, TOGGLE_SPLIT_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), TOGGLE_SPLIT_BUTTON_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(TOGGLE_SPLIT_BUTTON_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 2);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_toggle_split_button_init(EGUI_VIEW_OF(&button_compact));
    egui_view_set_size(EGUI_VIEW_OF(&button_compact), TOGGLE_SPLIT_BUTTON_PREVIEW_WIDTH, TOGGLE_SPLIT_BUTTON_PREVIEW_HEIGHT);
    egui_view_toggle_split_button_set_font(EGUI_VIEW_OF(&button_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_toggle_split_button_set_meta_font(EGUI_VIEW_OF(&button_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_toggle_split_button_set_snapshots(EGUI_VIEW_OF(&button_compact), compact_snapshots, 2);
    egui_view_toggle_split_button_set_compact_mode(EGUI_VIEW_OF(&button_compact), 1);
    static egui_view_api_t button_compact_touch_api;
    egui_view_override_api_on_touch(EGUI_VIEW_OF(&button_compact), &button_compact_touch_api, consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&button_compact), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&button_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), TOGGLE_SPLIT_BUTTON_PREVIEW_WIDTH, TOGGLE_SPLIT_BUTTON_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 8, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), TOGGLE_SPLIT_BUTTON_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(TOGGLE_SPLIT_BUTTON_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_toggle_split_button_init(EGUI_VIEW_OF(&button_locked));
    egui_view_set_size(EGUI_VIEW_OF(&button_locked), TOGGLE_SPLIT_BUTTON_PREVIEW_WIDTH, TOGGLE_SPLIT_BUTTON_PREVIEW_HEIGHT);
    egui_view_toggle_split_button_set_font(EGUI_VIEW_OF(&button_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_toggle_split_button_set_meta_font(EGUI_VIEW_OF(&button_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_toggle_split_button_set_snapshots(EGUI_VIEW_OF(&button_locked), &locked_snapshot, 1);
    egui_view_toggle_split_button_set_compact_mode(EGUI_VIEW_OF(&button_locked), 1);
    egui_view_toggle_split_button_set_read_only_mode(EGUI_VIEW_OF(&button_locked), 1);
    static egui_view_api_t button_locked_touch_api;
    egui_view_override_api_on_touch(EGUI_VIEW_OF(&button_locked), &button_locked_touch_api, consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&button_locked), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&button_locked));

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&locked_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    update_status(EGUI_VIEW_OF(&button_primary), egui_view_toggle_split_button_get_current_part(EGUI_VIEW_OF(&button_primary)));
}

#if EGUI_CONFIG_RECORDING_TEST
static void apply_primary_key(uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    EGUI_VIEW_OF(&button_primary)->api->on_key_event(EGUI_VIEW_OF(&button_primary), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    EGUI_VIEW_OF(&button_primary)->api->on_key_event(EGUI_VIEW_OF(&button_primary), &event);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_SPLIT_BUTTON_RECORD_WAIT);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_SPLIT_BUTTON_RECORD_FRAME_WAIT);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_SPACE);
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_SPLIT_BUTTON_RECORD_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_SPLIT_BUTTON_RECORD_FRAME_WAIT);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_RIGHT);
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_SPLIT_BUTTON_RECORD_WAIT);
        return true;
    case 5:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_ENTER);
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_SPLIT_BUTTON_RECORD_WAIT);
        return true;
    case 6:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_SPLIT_BUTTON_RECORD_FRAME_WAIT);
        return true;
    case 7:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_LEFT);
            apply_primary_key(EGUI_KEY_CODE_SPACE);
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_SPLIT_BUTTON_RECORD_WAIT);
        return true;
    case 8:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_SPLIT_BUTTON_RECORD_FRAME_WAIT);
        return true;
    case 9:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &guide_label, TOGGLE_SPLIT_BUTTON_RECORD_WAIT);
        return true;
    case 10:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_SPLIT_BUTTON_RECORD_FRAME_WAIT);
        return true;
    case 11:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &compact_label, TOGGLE_SPLIT_BUTTON_RECORD_WAIT);
        return true;
    case 12:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOGGLE_SPLIT_BUTTON_RECORD_FRAME_WAIT);
        return true;
    default:
        return false;
    }
}
#endif
