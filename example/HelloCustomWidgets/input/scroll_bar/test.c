#include <string.h>

#include "egui.h"
#include "egui_view_scroll_bar.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define SCROLL_BAR_ROOT_WIDTH          224
#define SCROLL_BAR_ROOT_HEIGHT         288
#define SCROLL_BAR_PRIMARY_WIDTH       196
#define SCROLL_BAR_PRIMARY_HEIGHT      146
#define SCROLL_BAR_PREVIEW_WIDTH       104
#define SCROLL_BAR_PREVIEW_HEIGHT      52
#define SCROLL_BAR_BOTTOM_ROW_WIDTH    216
#define SCROLL_BAR_BOTTOM_ROW_HEIGHT   68
#define SCROLL_BAR_GUIDE_COLOR         0x72808E
#define SCROLL_BAR_LABEL_COLOR         0x74808D
#define SCROLL_BAR_STATUS_COLOR        0x4C667C
#define SCROLL_BAR_COMPACT_LABEL_COLOR 0x0F776E
#define SCROLL_BAR_LOCKED_LABEL_COLOR  0x8995A2
#define SCROLL_BAR_PRIMARY_ACCENT      0x2563EB
#define SCROLL_BAR_COMPACT_ACCENT      0x0D9488
#define SCROLL_BAR_LOCKED_ACCENT       0x9AA7B5
#define SCROLL_BAR_RECORD_WAIT         110
#define SCROLL_BAR_RECORD_FRAME_WAIT   150

typedef struct scroll_bar_snapshot scroll_bar_snapshot_t;
struct scroll_bar_snapshot
{
    const char *label;
    const char *helper;
    const char *status_prefix;
    egui_color_t status_color;
    egui_dim_t content_length;
    egui_dim_t viewport_length;
    egui_dim_t offset;
    egui_dim_t line_step;
    egui_dim_t page_step;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_scroll_bar_t scroll_bar_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_scroll_bar_t scroll_bar_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_scroll_bar_t scroll_bar_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Scroll Bar";
static const char *guide_text = "Tap guide to cycle presets";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[72] = "Docs 168 / 620";
static uint8_t primary_snapshot_index = 0;
static uint8_t compact_snapshot_index = 0;

static const scroll_bar_snapshot_t primary_snapshots[] = {
        {"Document rail", "Buttons step, track pages, thumb drags", "Docs", EGUI_COLOR_HEX(SCROLL_BAR_PRIMARY_ACCENT), 840, 220, 168, 24, 160},
        {"Timeline rail", "Viewport size keeps the thumb honest", "Timeline", EGUI_COLOR_HEX(0xD97706), 640, 180, 92, 18, 120},
        {"Audit rail", "Read logs without turning this into a slider", "Audit", EGUI_COLOR_HEX(0x0F766E), 980, 260, 420, 32, 210},
};

static const scroll_bar_snapshot_t compact_snapshots[] = {
        {NULL, NULL, "Compact", EGUI_COLOR_HEX(SCROLL_BAR_COMPACT_ACCENT), 540, 160, 96, 16, 96},
        {NULL, NULL, "Compact", EGUI_COLOR_HEX(SCROLL_BAR_COMPACT_ACCENT), 720, 240, 188, 24, 144},
};

static const scroll_bar_snapshot_t locked_snapshot = {NULL, NULL, "Locked", EGUI_COLOR_HEX(SCROLL_BAR_LOCKED_ACCENT), 620, 200, 140, 20, 120};

static void refresh_status(const scroll_bar_snapshot_t *snapshot)
{
    int pos = 0;
    egui_dim_t offset = egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&scroll_bar_primary));
    egui_dim_t max_offset = egui_view_scroll_bar_get_max_offset(EGUI_VIEW_OF(&scroll_bar_primary));

    pos += egui_sprintf_str(status_text, sizeof(status_text), snapshot->status_prefix);
    pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
    pos += egui_sprintf_int(&status_text[pos], (int)sizeof(status_text) - pos, offset);
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " / ");
    egui_sprintf_int(&status_text[pos], (int)sizeof(status_text) - pos, max_offset);

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(snapshot->status_color, EGUI_COLOR_HEX(SCROLL_BAR_STATUS_COLOR), 26),
                                   EGUI_ALPHA_100);
}

static void apply_snapshot(egui_view_t *view, const scroll_bar_snapshot_t *snapshot)
{
    if (snapshot->label != NULL)
    {
        egui_view_scroll_bar_set_label(view, snapshot->label);
    }
    if (snapshot->helper != NULL)
    {
        egui_view_scroll_bar_set_helper(view, snapshot->helper);
    }
    egui_view_scroll_bar_set_content_metrics(view, snapshot->content_length, snapshot->viewport_length);
    egui_view_scroll_bar_set_step_size(view, snapshot->line_step, snapshot->page_step);
    egui_view_scroll_bar_set_offset(view, snapshot->offset);
    egui_view_scroll_bar_set_current_part(view, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
}

static void apply_primary_snapshot(uint8_t index, uint8_t update_status)
{
    const scroll_bar_snapshot_t *snapshot = &primary_snapshots[index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]))];

    primary_snapshot_index = index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]));
    apply_snapshot(EGUI_VIEW_OF(&scroll_bar_primary), snapshot);
    if (update_status)
    {
        refresh_status(snapshot);
    }
}

static void apply_compact_snapshot(uint8_t index)
{
    const scroll_bar_snapshot_t *snapshot = &compact_snapshots[index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]))];

    compact_snapshot_index = index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]));
    apply_snapshot(EGUI_VIEW_OF(&scroll_bar_compact), snapshot);
}

static void on_primary_changed(egui_view_t *self, egui_dim_t offset, egui_dim_t max_offset, uint8_t part)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(offset);
    EGUI_UNUSED(max_offset);
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
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), SCROLL_BAR_ROOT_WIDTH, SCROLL_BAR_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), SCROLL_BAR_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), SCROLL_BAR_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(SCROLL_BAR_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), SCROLL_BAR_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(SCROLL_BAR_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_scroll_bar_init(EGUI_VIEW_OF(&scroll_bar_primary));
    egui_view_set_size(EGUI_VIEW_OF(&scroll_bar_primary), SCROLL_BAR_PRIMARY_WIDTH, SCROLL_BAR_PRIMARY_HEIGHT);
    egui_view_scroll_bar_set_font(EGUI_VIEW_OF(&scroll_bar_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_scroll_bar_set_meta_font(EGUI_VIEW_OF(&scroll_bar_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_scroll_bar_set_palette(EGUI_VIEW_OF(&scroll_bar_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD7DFE7), EGUI_COLOR_HEX(0x1A2630),
                                     EGUI_COLOR_HEX(0x72808E), EGUI_COLOR_HEX(SCROLL_BAR_PRIMARY_ACCENT), EGUI_COLOR_HEX(0x5BA5F8));
    egui_view_scroll_bar_set_on_changed_listener(EGUI_VIEW_OF(&scroll_bar_primary), on_primary_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&scroll_bar_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&scroll_bar_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), SCROLL_BAR_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(SCROLL_BAR_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), SCROLL_BAR_BOTTOM_ROW_WIDTH, SCROLL_BAR_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), SCROLL_BAR_PREVIEW_WIDTH, SCROLL_BAR_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), SCROLL_BAR_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(SCROLL_BAR_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_scroll_bar_init(EGUI_VIEW_OF(&scroll_bar_compact));
    egui_view_set_size(EGUI_VIEW_OF(&scroll_bar_compact), SCROLL_BAR_PREVIEW_WIDTH, SCROLL_BAR_PREVIEW_HEIGHT);
    egui_view_scroll_bar_set_font(EGUI_VIEW_OF(&scroll_bar_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_scroll_bar_set_meta_font(EGUI_VIEW_OF(&scroll_bar_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_scroll_bar_set_compact_mode(EGUI_VIEW_OF(&scroll_bar_compact), 1);
    egui_view_scroll_bar_set_palette(EGUI_VIEW_OF(&scroll_bar_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD2DDDA), EGUI_COLOR_HEX(0x17302A),
                                     EGUI_COLOR_HEX(0x57756C), EGUI_COLOR_HEX(SCROLL_BAR_COMPACT_ACCENT), EGUI_COLOR_HEX(0x3BC7B3));
    egui_view_set_on_touch_listener(EGUI_VIEW_OF(&scroll_bar_compact), consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&scroll_bar_compact), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&scroll_bar_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), SCROLL_BAR_PREVIEW_WIDTH, SCROLL_BAR_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 8, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), SCROLL_BAR_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(SCROLL_BAR_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_scroll_bar_init(EGUI_VIEW_OF(&scroll_bar_locked));
    egui_view_set_size(EGUI_VIEW_OF(&scroll_bar_locked), SCROLL_BAR_PREVIEW_WIDTH, SCROLL_BAR_PREVIEW_HEIGHT);
    egui_view_scroll_bar_set_font(EGUI_VIEW_OF(&scroll_bar_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_scroll_bar_set_meta_font(EGUI_VIEW_OF(&scroll_bar_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_scroll_bar_set_compact_mode(EGUI_VIEW_OF(&scroll_bar_locked), 1);
    egui_view_scroll_bar_set_read_only_mode(EGUI_VIEW_OF(&scroll_bar_locked), 1);
    egui_view_scroll_bar_set_palette(EGUI_VIEW_OF(&scroll_bar_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xDBE2E8), EGUI_COLOR_HEX(0x536474),
                                     EGUI_COLOR_HEX(0x8896A4), EGUI_COLOR_HEX(SCROLL_BAR_LOCKED_ACCENT), EGUI_COLOR_HEX(0xA7B6C4));
    egui_view_set_on_touch_listener(EGUI_VIEW_OF(&scroll_bar_locked), consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&scroll_bar_locked), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&scroll_bar_locked));

    apply_primary_snapshot(0, 1);
    apply_compact_snapshot(0);
    apply_snapshot(EGUI_VIEW_OF(&scroll_bar_locked), &locked_snapshot);

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
    EGUI_VIEW_OF(&scroll_bar_primary)->api->on_key_event(EGUI_VIEW_OF(&scroll_bar_primary), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    EGUI_VIEW_OF(&scroll_bar_primary)->api->on_key_event(EGUI_VIEW_OF(&scroll_bar_primary), &event);
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
        }
        EGUI_SIM_SET_WAIT(p_action, SCROLL_BAR_RECORD_WAIT);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, SCROLL_BAR_RECORD_FRAME_WAIT);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_DOWN);
        }
        EGUI_SIM_SET_WAIT(p_action, SCROLL_BAR_RECORD_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, SCROLL_BAR_RECORD_FRAME_WAIT);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_PLUS);
        }
        EGUI_SIM_SET_WAIT(p_action, SCROLL_BAR_RECORD_WAIT);
        return true;
    case 5:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, SCROLL_BAR_RECORD_FRAME_WAIT);
        return true;
    case 6:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_END);
        }
        EGUI_SIM_SET_WAIT(p_action, SCROLL_BAR_RECORD_WAIT);
        return true;
    case 7:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, SCROLL_BAR_RECORD_FRAME_WAIT);
        return true;
    case 8:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &guide_label, SCROLL_BAR_RECORD_WAIT);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, SCROLL_BAR_RECORD_FRAME_WAIT);
        return true;
    case 10:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &compact_label, SCROLL_BAR_RECORD_WAIT);
        return true;
    case 11:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, SCROLL_BAR_RECORD_FRAME_WAIT);
        return true;
    default:
        return false;
    }
}
#endif
