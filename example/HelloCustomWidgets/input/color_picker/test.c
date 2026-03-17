#include <string.h>

#include "egui.h"
#include "egui_view_color_picker.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define COLOR_PICKER_ROOT_WIDTH          224
#define COLOR_PICKER_ROOT_HEIGHT         286
#define COLOR_PICKER_PRIMARY_WIDTH       196
#define COLOR_PICKER_PRIMARY_HEIGHT      112
#define COLOR_PICKER_PREVIEW_WIDTH       104
#define COLOR_PICKER_PREVIEW_HEIGHT      52
#define COLOR_PICKER_BOTTOM_ROW_WIDTH    216
#define COLOR_PICKER_BOTTOM_ROW_HEIGHT   68
#define COLOR_PICKER_GUIDE_COLOR         0x72808E
#define COLOR_PICKER_LABEL_COLOR         0x74808D
#define COLOR_PICKER_STATUS_COLOR        0x4C667C
#define COLOR_PICKER_COMPACT_LABEL_COLOR 0x0F776E
#define COLOR_PICKER_LOCKED_LABEL_COLOR  0x8995A2
#define COLOR_PICKER_PRIMARY_ACCENT      0x2563EB
#define COLOR_PICKER_COMPACT_ACCENT      0x0D9488
#define COLOR_PICKER_LOCKED_ACCENT       0x9AA7B5
#define COLOR_PICKER_RECORD_WAIT         110
#define COLOR_PICKER_RECORD_FRAME_WAIT   150

typedef struct color_picker_snapshot color_picker_snapshot_t;
struct color_picker_snapshot
{
    const char *label;
    const char *helper;
    const char *status_prefix;
    egui_color_t status_color;
    uint8_t hue_index;
    uint8_t saturation_index;
    uint8_t value_index;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_color_picker_t picker_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_color_picker_t picker_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_color_picker_t picker_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Color Picker";
static const char *guide_text = "Tap guide to cycle presets";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[64] = "Ocean #2A7BD9";
static uint8_t primary_snapshot_index = 0;
static uint8_t compact_snapshot_index = 0;

static const color_picker_snapshot_t primary_snapshots[] = {
        {"Accent color", "Use arrows or tap the hue rail", "Ocean", EGUI_COLOR_HEX(COLOR_PICKER_PRIMARY_ACCENT), 7, 4, 1},
        {"Signal color", "Warm families surface alerts", "Coral", EGUI_COLOR_HEX(0xD97706), 1, 5, 1},
        {"Surface tint", "Muted tones keep cards calm", "Moss", EGUI_COLOR_HEX(0x2F855A), 4, 3, 2},
};

static const color_picker_snapshot_t compact_snapshots[] = {
        {NULL, NULL, "Mint", EGUI_COLOR_HEX(COLOR_PICKER_COMPACT_ACCENT), 5, 4, 1},
        {NULL, NULL, "Sun", EGUI_COLOR_HEX(COLOR_PICKER_COMPACT_ACCENT), 1, 5, 0},
};

static const color_picker_snapshot_t locked_snapshot = {NULL, NULL, "Locked", EGUI_COLOR_HEX(COLOR_PICKER_LOCKED_ACCENT), 8, 2, 2};

static void refresh_status(const color_picker_snapshot_t *snapshot)
{
    int pos = 0;
    const char *hex_text = egui_view_color_picker_get_hex_text(EGUI_VIEW_OF(&picker_primary));

    pos += egui_sprintf_str(status_text, sizeof(status_text), snapshot->status_prefix);
    pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
    egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, hex_text);

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(snapshot->status_color, EGUI_COLOR_HEX(COLOR_PICKER_STATUS_COLOR), 26),
                                   EGUI_ALPHA_100);
}

static void apply_snapshot(egui_view_t *view, const color_picker_snapshot_t *snapshot)
{
    if (snapshot->label != NULL)
    {
        egui_view_color_picker_set_label(view, snapshot->label);
    }
    if (snapshot->helper != NULL)
    {
        egui_view_color_picker_set_helper(view, snapshot->helper);
    }
    egui_view_color_picker_set_selection(view, snapshot->hue_index, snapshot->saturation_index, snapshot->value_index);
    egui_view_color_picker_set_current_part(view, EGUI_VIEW_COLOR_PICKER_PART_PALETTE);
}

static void apply_primary_snapshot(uint8_t index, uint8_t update_status)
{
    const color_picker_snapshot_t *snapshot = &primary_snapshots[index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]))];

    primary_snapshot_index = index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]));
    apply_snapshot(EGUI_VIEW_OF(&picker_primary), snapshot);
    if (update_status)
    {
        refresh_status(snapshot);
    }
}

static void apply_compact_snapshot(uint8_t index)
{
    const color_picker_snapshot_t *snapshot = &compact_snapshots[index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]))];

    compact_snapshot_index = index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]));
    apply_snapshot(EGUI_VIEW_OF(&picker_compact), snapshot);
}

static void on_primary_changed(egui_view_t *self, egui_color_t color, uint8_t hue_index, uint8_t saturation_index, uint8_t value_index, uint8_t part)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(color);
    EGUI_UNUSED(hue_index);
    EGUI_UNUSED(saturation_index);
    EGUI_UNUSED(value_index);
    EGUI_UNUSED(part);
    refresh_status(&primary_snapshots[primary_snapshot_index]);
}

static void on_guide_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_primary_snapshot((uint8_t)(primary_snapshot_index + 1), 1);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_compact_snapshot((uint8_t)(compact_snapshot_index + 1));
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
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), COLOR_PICKER_ROOT_WIDTH, COLOR_PICKER_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), COLOR_PICKER_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), COLOR_PICKER_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(COLOR_PICKER_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), COLOR_PICKER_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(COLOR_PICKER_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_color_picker_init(EGUI_VIEW_OF(&picker_primary));
    egui_view_set_size(EGUI_VIEW_OF(&picker_primary), COLOR_PICKER_PRIMARY_WIDTH, COLOR_PICKER_PRIMARY_HEIGHT);
    egui_view_color_picker_set_font(EGUI_VIEW_OF(&picker_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_color_picker_set_meta_font(EGUI_VIEW_OF(&picker_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_color_picker_set_palette(EGUI_VIEW_OF(&picker_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD7DFE7), EGUI_COLOR_HEX(0x1A2630),
                                       EGUI_COLOR_HEX(0x72808E), EGUI_COLOR_HEX(COLOR_PICKER_PRIMARY_ACCENT));
    egui_view_color_picker_set_on_changed_listener(EGUI_VIEW_OF(&picker_primary), on_primary_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&picker_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&picker_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), COLOR_PICKER_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(COLOR_PICKER_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 148, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDEE4EB));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), COLOR_PICKER_BOTTOM_ROW_WIDTH, COLOR_PICKER_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), COLOR_PICKER_PREVIEW_WIDTH, COLOR_PICKER_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), COLOR_PICKER_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(COLOR_PICKER_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_color_picker_init(EGUI_VIEW_OF(&picker_compact));
    egui_view_set_size(EGUI_VIEW_OF(&picker_compact), COLOR_PICKER_PREVIEW_WIDTH, COLOR_PICKER_PREVIEW_HEIGHT);
    egui_view_color_picker_set_font(EGUI_VIEW_OF(&picker_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_color_picker_set_meta_font(EGUI_VIEW_OF(&picker_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_color_picker_set_compact_mode(EGUI_VIEW_OF(&picker_compact), 1);
    egui_view_color_picker_set_palette(EGUI_VIEW_OF(&picker_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD2DDDA), EGUI_COLOR_HEX(0x17302A),
                                       EGUI_COLOR_HEX(0x57756C), EGUI_COLOR_HEX(COLOR_PICKER_COMPACT_ACCENT));
    egui_view_set_on_touch_listener(EGUI_VIEW_OF(&picker_compact), consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&picker_compact), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&picker_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), COLOR_PICKER_PREVIEW_WIDTH, COLOR_PICKER_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 8, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), COLOR_PICKER_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(COLOR_PICKER_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_color_picker_init(EGUI_VIEW_OF(&picker_locked));
    egui_view_set_size(EGUI_VIEW_OF(&picker_locked), COLOR_PICKER_PREVIEW_WIDTH, COLOR_PICKER_PREVIEW_HEIGHT);
    egui_view_color_picker_set_font(EGUI_VIEW_OF(&picker_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_color_picker_set_meta_font(EGUI_VIEW_OF(&picker_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_color_picker_set_compact_mode(EGUI_VIEW_OF(&picker_locked), 1);
    egui_view_color_picker_set_read_only_mode(EGUI_VIEW_OF(&picker_locked), 1);
    egui_view_color_picker_set_palette(EGUI_VIEW_OF(&picker_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xDBE2E8), EGUI_COLOR_HEX(0x536474),
                                       EGUI_COLOR_HEX(0x8896A4), EGUI_COLOR_HEX(COLOR_PICKER_LOCKED_ACCENT));
    egui_view_set_on_touch_listener(EGUI_VIEW_OF(&picker_locked), consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&picker_locked), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&picker_locked));

    apply_primary_snapshot(0, 1);
    apply_compact_snapshot(0);
    apply_snapshot(EGUI_VIEW_OF(&picker_locked), &locked_snapshot);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&locked_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
static void apply_primary_key(uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    EGUI_VIEW_OF(&picker_primary)->api->on_key_event(EGUI_VIEW_OF(&picker_primary), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    EGUI_VIEW_OF(&picker_primary)->api->on_key_event(EGUI_VIEW_OF(&picker_primary), &event);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            apply_primary_snapshot(0, 1);
            apply_compact_snapshot(0);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, COLOR_PICKER_RECORD_FRAME_WAIT);
        return true;
    case 1:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_RIGHT);
        }
        EGUI_SIM_SET_WAIT(p_action, COLOR_PICKER_RECORD_WAIT);
        return true;
    case 2:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, COLOR_PICKER_RECORD_FRAME_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
            apply_primary_key(EGUI_KEY_CODE_DOWN);
        }
        EGUI_SIM_SET_WAIT(p_action, COLOR_PICKER_RECORD_WAIT);
        return true;
    case 4:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, COLOR_PICKER_RECORD_FRAME_WAIT);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &guide_label, COLOR_PICKER_RECORD_WAIT);
        return true;
    case 6:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, COLOR_PICKER_RECORD_FRAME_WAIT);
        return true;
    case 7:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &compact_label, COLOR_PICKER_RECORD_WAIT);
        return true;
    case 8:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, COLOR_PICKER_RECORD_FRAME_WAIT);
        return true;
    default:
        return false;
    }
}
#endif
