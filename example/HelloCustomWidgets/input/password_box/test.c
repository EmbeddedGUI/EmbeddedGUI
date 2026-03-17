#include <string.h>

#include "egui.h"
#include "egui_view_password_box.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define PASSWORD_BOX_PRIMARY_WIDTH       196
#define PASSWORD_BOX_PRIMARY_HEIGHT      70
#define PASSWORD_BOX_COMPACT_WIDTH       106
#define PASSWORD_BOX_COMPACT_HEIGHT      44
#define PASSWORD_BOX_BOTTOM_ROW_WIDTH    216
#define PASSWORD_BOX_BOTTOM_ROW_HEIGHT   64
#define PASSWORD_BOX_GUIDE_COLOR         0x6F7C8B
#define PASSWORD_BOX_LABEL_COLOR         0x788492
#define PASSWORD_BOX_STATUS_COLOR        0x536678
#define PASSWORD_BOX_COMPACT_LABEL_COLOR 0x8A6C1B
#define PASSWORD_BOX_LOCKED_LABEL_COLOR  0x8A97A4
#define PASSWORD_BOX_PRIMARY_ACCENT      0x2563EB
#define PASSWORD_BOX_PRIMARY_BORDER      0xD7DFE7
#define PASSWORD_BOX_COMPACT_ACCENT      0xB7791F
#define PASSWORD_BOX_COMPACT_BORDER      0xDBE1E7
#define PASSWORD_BOX_LOCKED_ACCENT       0x9AA6B4
#define PASSWORD_BOX_LOCKED_BORDER       0xDCE3E9
#define PASSWORD_BOX_RECORD_WAIT         100
#define PASSWORD_BOX_RECORD_FRAME_WAIT   150

typedef struct password_box_snapshot password_box_snapshot_t;
struct password_box_snapshot
{
    const char *label;
    const char *helper;
    const char *placeholder;
    const char *text;
    const char *status_prefix;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_password_box_t box_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_password_box_t box_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_password_box_t box_locked;

static uint8_t primary_snapshot_index = 0;
static uint8_t compact_snapshot_index = 0;
static char status_text[64] = "Wi-Fi 9 chars hidden";

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Password Box";
static const char *guide_text = "Tap labels to switch presets";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {143, 0}};

static const password_box_snapshot_t primary_snapshots[] = {
        {"Wi-Fi passphrase", "Use 8 to 32 chars", "Enter password", "studio-24", "Wi-Fi"},
        {"Deploy secret", "Keep this hidden on shared screens", "Secret", "release-key-7", "Deploy"},
};

static const password_box_snapshot_t compact_snapshots[] = {
        {NULL, NULL, "Quick pin", "7429", "Compact"},
        {NULL, NULL, "Quick pin", "A-17", "Compact"},
};

static void refresh_status(const password_box_snapshot_t *snapshot, egui_view_t *view, egui_color_t color)
{
    const char *text = egui_view_password_box_get_text(view);
    uint8_t revealed = egui_view_password_box_get_revealed(view);
    int pos = 0;
    int length = text ? (int)strlen(text) : 0;

    pos += egui_sprintf_str(status_text, sizeof(status_text), snapshot->status_prefix);
    pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
    pos += egui_sprintf_int(&status_text[pos], (int)sizeof(status_text) - pos, length);
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " chars ");
    egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, revealed ? "visible" : "hidden");

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(PASSWORD_BOX_STATUS_COLOR), 28), EGUI_ALPHA_100);
}

static void apply_snapshot(egui_view_t *view, const password_box_snapshot_t *snapshot)
{
    if (snapshot->label)
    {
        egui_view_password_box_set_label(view, snapshot->label);
    }
    if (snapshot->helper)
    {
        egui_view_password_box_set_helper(view, snapshot->helper);
    }
    egui_view_password_box_set_placeholder(view, snapshot->placeholder);
    egui_view_password_box_set_text(view, snapshot->text);
    egui_view_password_box_set_revealed(view, 0);
    egui_view_password_box_set_current_part(view, EGUI_VIEW_PASSWORD_BOX_PART_FIELD);
}

static void apply_primary_snapshot(uint8_t index, uint8_t update_status)
{
    const password_box_snapshot_t *snapshot = &primary_snapshots[index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]))];

    primary_snapshot_index = index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]));
    apply_snapshot(EGUI_VIEW_OF(&box_primary), snapshot);
    if (update_status)
    {
        refresh_status(snapshot, EGUI_VIEW_OF(&box_primary), EGUI_COLOR_HEX(PASSWORD_BOX_PRIMARY_ACCENT));
    }
}

static void apply_compact_snapshot(uint8_t index)
{
    const password_box_snapshot_t *snapshot = &compact_snapshots[index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]))];

    compact_snapshot_index = index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]));
    apply_snapshot(EGUI_VIEW_OF(&box_compact), snapshot);
}

static void on_primary_changed(egui_view_t *self, const char *text, uint8_t revealed, uint8_t part)
{
    EGUI_UNUSED(text);
    EGUI_UNUSED(revealed);
    EGUI_UNUSED(part);
    refresh_status(&primary_snapshots[primary_snapshot_index], self, EGUI_COLOR_HEX(PASSWORD_BOX_PRIMARY_ACCENT));
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

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 278);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 224, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 9, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(PASSWORD_BOX_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), 224, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(PASSWORD_BOX_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_password_box_init(EGUI_VIEW_OF(&box_primary));
    egui_view_set_size(EGUI_VIEW_OF(&box_primary), PASSWORD_BOX_PRIMARY_WIDTH, PASSWORD_BOX_PRIMARY_HEIGHT);
    egui_view_password_box_set_font(EGUI_VIEW_OF(&box_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_password_box_set_meta_font(EGUI_VIEW_OF(&box_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_password_box_set_palette(EGUI_VIEW_OF(&box_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(PASSWORD_BOX_PRIMARY_BORDER),
                                       EGUI_COLOR_HEX(0x1F2A35), EGUI_COLOR_HEX(0x74818E), EGUI_COLOR_HEX(PASSWORD_BOX_PRIMARY_ACCENT));
    egui_view_password_box_set_on_changed_listener(EGUI_VIEW_OF(&box_primary), on_primary_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&box_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&box_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(PASSWORD_BOX_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 144, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDEE4EB));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), PASSWORD_BOX_BOTTOM_ROW_WIDTH, PASSWORD_BOX_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), PASSWORD_BOX_COMPACT_WIDTH, PASSWORD_BOX_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), PASSWORD_BOX_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(PASSWORD_BOX_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_password_box_init(EGUI_VIEW_OF(&box_compact));
    egui_view_set_size(EGUI_VIEW_OF(&box_compact), PASSWORD_BOX_COMPACT_WIDTH, PASSWORD_BOX_COMPACT_HEIGHT);
    egui_view_password_box_set_font(EGUI_VIEW_OF(&box_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_password_box_set_meta_font(EGUI_VIEW_OF(&box_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_password_box_set_compact_mode(EGUI_VIEW_OF(&box_compact), 1);
    egui_view_password_box_set_palette(EGUI_VIEW_OF(&box_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(PASSWORD_BOX_COMPACT_BORDER),
                                       EGUI_COLOR_HEX(0x27333D), EGUI_COLOR_HEX(0x7A8794), EGUI_COLOR_HEX(PASSWORD_BOX_COMPACT_ACCENT));
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&box_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), PASSWORD_BOX_COMPACT_WIDTH, PASSWORD_BOX_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), PASSWORD_BOX_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(PASSWORD_BOX_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_password_box_init(EGUI_VIEW_OF(&box_locked));
    egui_view_set_size(EGUI_VIEW_OF(&box_locked), PASSWORD_BOX_COMPACT_WIDTH, PASSWORD_BOX_COMPACT_HEIGHT);
    egui_view_password_box_set_font(EGUI_VIEW_OF(&box_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_password_box_set_meta_font(EGUI_VIEW_OF(&box_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_password_box_set_compact_mode(EGUI_VIEW_OF(&box_locked), 1);
    egui_view_password_box_set_read_only_mode(EGUI_VIEW_OF(&box_locked), 1);
    egui_view_password_box_set_palette(EGUI_VIEW_OF(&box_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(PASSWORD_BOX_LOCKED_BORDER),
                                       EGUI_COLOR_HEX(0x586473), EGUI_COLOR_HEX(0x8F9AA7), EGUI_COLOR_HEX(PASSWORD_BOX_LOCKED_ACCENT));
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&box_locked));

    apply_primary_snapshot(0, 1);
    apply_compact_snapshot(0);
    egui_view_password_box_set_text(EGUI_VIEW_OF(&box_locked), "fleet-admin");

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&locked_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
static void apply_primary_key(uint8_t key_code, uint8_t is_shift)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    event.is_shift = is_shift ? 1 : 0;
    EGUI_VIEW_OF(&box_primary)->api->on_key_event(EGUI_VIEW_OF(&box_primary), &event);

    event.type = EGUI_KEY_EVENT_ACTION_UP;
    EGUI_VIEW_OF(&box_primary)->api->on_key_event(EGUI_VIEW_OF(&box_primary), &event);
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
        EGUI_SIM_SET_WAIT(p_action, PASSWORD_BOX_RECORD_FRAME_WAIT);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &box_primary, PASSWORD_BOX_RECORD_WAIT);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_BACKSPACE, 0);
        }
        EGUI_SIM_SET_WAIT(p_action, PASSWORD_BOX_RECORD_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_2, 0);
        }
        EGUI_SIM_SET_WAIT(p_action, PASSWORD_BOX_RECORD_WAIT);
        return true;
    case 4:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, PASSWORD_BOX_RECORD_FRAME_WAIT);
        return true;
    case 5:
        if (first_call)
        {
            egui_view_password_box_set_revealed(EGUI_VIEW_OF(&box_primary), 1);
        }
        EGUI_SIM_SET_WAIT(p_action, PASSWORD_BOX_RECORD_WAIT);
        return true;
    case 6:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, PASSWORD_BOX_RECORD_FRAME_WAIT);
        return true;
    case 7:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB, 0);
        }
        EGUI_SIM_SET_WAIT(p_action, PASSWORD_BOX_RECORD_WAIT);
        return true;
    case 8:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_SPACE, 0);
        }
        EGUI_SIM_SET_WAIT(p_action, PASSWORD_BOX_RECORD_WAIT);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, PASSWORD_BOX_RECORD_FRAME_WAIT);
        return true;
    case 10:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &guide_label, PASSWORD_BOX_RECORD_WAIT);
        return true;
    case 11:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, PASSWORD_BOX_RECORD_FRAME_WAIT);
        return true;
    case 12:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &compact_label, PASSWORD_BOX_RECORD_WAIT);
        return true;
    case 13:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, PASSWORD_BOX_RECORD_FRAME_WAIT);
        return true;
    default:
        return false;
    }
}
#endif
