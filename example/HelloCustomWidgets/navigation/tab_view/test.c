#include <string.h>

#include "egui.h"
#include "egui_view_tab_view.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define TAB_VIEW_ROOT_WIDTH          224
#define TAB_VIEW_ROOT_HEIGHT         300
#define TAB_VIEW_PRIMARY_WIDTH       198
#define TAB_VIEW_PRIMARY_HEIGHT      128
#define TAB_VIEW_PREVIEW_WIDTH       104
#define TAB_VIEW_PREVIEW_HEIGHT      72
#define TAB_VIEW_BOTTOM_ROW_WIDTH    216
#define TAB_VIEW_BOTTOM_ROW_HEIGHT   90
#define TAB_VIEW_GUIDE_COLOR         0x72808E
#define TAB_VIEW_LABEL_COLOR         0x74808D
#define TAB_VIEW_STATUS_COLOR        0x4E6174
#define TAB_VIEW_COMPACT_LABEL_COLOR 0x0C8A78
#define TAB_VIEW_LOCKED_LABEL_COLOR  0x8995A2

static egui_color_t tone_status_color(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_TAB_VIEW_TONE_SUCCESS:
        return EGUI_COLOR_HEX(0x1A7F4B);
    case EGUI_VIEW_TAB_VIEW_TONE_WARNING:
        return EGUI_COLOR_HEX(0xA16207);
    case EGUI_VIEW_TAB_VIEW_TONE_NEUTRAL:
        return EGUI_COLOR_HEX(0x5F7286);
    default:
        return EGUI_COLOR_HEX(TAB_VIEW_STATUS_COLOR);
    }
}

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_tab_view_t tab_view_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_tab_view_t tab_view_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_tab_view_t tab_view_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Tab View";
static const char *guide_text = "Tap guide to swap workspace";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {150, 0}};
static char status_text[96] = "Docs 1/4 Home";
static uint8_t primary_snapshot_index = 0;
static uint8_t compact_snapshot_index = 0;

static const egui_view_tab_view_tab_t docs_tabs[] = {
        {"Home", "Landing summary", "Workspace", "Body stays linked", "Close button only appears on the active tab", "Ready", "Draft",
         EGUI_VIEW_TAB_VIEW_TONE_ACCENT, 1, 1},
        {"Review", "Comment pass", "Workspace", "Review lane ready", "Different from a plain tab strip", "Track", "Sync", EGUI_VIEW_TAB_VIEW_TONE_WARNING, 0,
         1},
        {"Publish", "Release check", "Workspace", "Footer follows tab", "Add button restores hidden tabs after close", "Ship", "Ready",
         EGUI_VIEW_TAB_VIEW_TONE_SUCCESS, 0, 1},
        {"Audit", "Snapshot log", "Workspace", "Preview stays muted", "Compact track removes heavy chrome", "Audit", "Pinned", EGUI_VIEW_TAB_VIEW_TONE_NEUTRAL,
         0, 0},
};

static const egui_view_tab_view_tab_t ops_tabs[] = {
        {"Queue", "Signal wall", "Operations", "Track reset works", "Useful for multi-workspace review", "Live", "Focus", EGUI_VIEW_TAB_VIEW_TONE_NEUTRAL, 0,
         1},
        {"Alerts", "Escalation", "Operations", "Alert body visible", "Body stays low-noise in 240 x 320", "Watch", "Open", EGUI_VIEW_TAB_VIEW_TONE_WARNING, 1,
         1},
        {"Repair", "Dispatch lane", "Operations", "Close then restore", "Keyboard and touch stay aligned", "Repair", "Ready", EGUI_VIEW_TAB_VIEW_TONE_SUCCESS,
         0, 1},
};

static const egui_view_tab_view_tab_t compact_tabs_a[] = {
        {"Docs", "Pocket", "Compact", "Light shell", "", "Mini", "Focus", EGUI_VIEW_TAB_VIEW_TONE_ACCENT, 0, 1},
        {"Audit", "Pocket", "Compact", "Quiet rail", "", "Mini", "Track", EGUI_VIEW_TAB_VIEW_TONE_WARNING, 0, 0},
};

static const egui_view_tab_view_tab_t compact_tabs_b[] = {
        {"Ops", "Pocket", "Compact", "Track swap", "", "Mini", "Live", EGUI_VIEW_TAB_VIEW_TONE_NEUTRAL, 0, 1},
        {"Ship", "Pocket", "Compact", "Soft body", "", "Mini", "Done", EGUI_VIEW_TAB_VIEW_TONE_SUCCESS, 0, 0},
};

static const egui_view_tab_view_tab_t locked_tabs[] = {
        {"Read", "Locked", "Read only", "Frozen page", "", "Mute", "Frozen", EGUI_VIEW_TAB_VIEW_TONE_NEUTRAL, 0, 1},
        {"Safe", "Locked", "Read only", "Muted shell", "", "Mute", "Audit", EGUI_VIEW_TAB_VIEW_TONE_ACCENT, 0, 0},
};

static const egui_view_tab_view_snapshot_t primary_snapshots[] = {
        {"Docs workspace", "Header + tab rail + body", docs_tabs, 4, 0, 1},
        {"Ops workspace", "Close and restore in one shell", ops_tabs, 3, 1, 1},
};

static const egui_view_tab_view_snapshot_t compact_snapshots[] = {
        {"Compact docs", "Low-noise preview", compact_tabs_a, 2, 0, 1},
        {"Compact ops", "Second preview track", compact_tabs_b, 2, 0, 1},
};

static const egui_view_tab_view_snapshot_t locked_snapshots[] = {
        {"Read only", "Muted reference preview", locked_tabs, 2, 0, 0},
};

static void update_status(void)
{
    const egui_view_tab_view_snapshot_t *snapshot = &primary_snapshots[egui_view_tab_view_get_current_snapshot(EGUI_VIEW_OF(&tab_view_primary))];
    uint8_t current_tab = egui_view_tab_view_get_current_tab(EGUI_VIEW_OF(&tab_view_primary));
    int pos = 0;

    pos += egui_sprintf_str(status_text, sizeof(status_text), snapshot->window_title);
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " ");
    pos += egui_sprintf_int(&status_text[pos], (int)sizeof(status_text) - pos, current_tab + 1);
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, "/");
    pos += egui_sprintf_int(&status_text[pos], (int)sizeof(status_text) - pos, egui_view_tab_view_get_visible_tab_count(EGUI_VIEW_OF(&tab_view_primary)));
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " ");
    if (current_tab != EGUI_VIEW_TAB_VIEW_TAB_NONE)
    {
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, snapshot->tabs[current_tab].title);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), tone_status_color(snapshot->tabs[current_tab].tone), EGUI_ALPHA_100);
    }
    else
    {
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, "Restore");
        egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(0xA16207), EGUI_ALPHA_100);
    }
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
}

static void on_primary_changed(egui_view_t *self, uint8_t snapshot_index, uint8_t tab_index, uint8_t part)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(snapshot_index);
    EGUI_UNUSED(tab_index);
    EGUI_UNUSED(part);
    update_status();
}

static void on_guide_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    primary_snapshot_index = (uint8_t)((primary_snapshot_index + 1) % 2);
    egui_view_tab_view_set_current_snapshot(EGUI_VIEW_OF(&tab_view_primary), primary_snapshot_index);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    compact_snapshot_index = (uint8_t)((compact_snapshot_index + 1) % 2);
    egui_view_tab_view_set_current_snapshot(EGUI_VIEW_OF(&tab_view_compact), compact_snapshot_index);
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
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), TAB_VIEW_ROOT_WIDTH, TAB_VIEW_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), TAB_VIEW_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), TAB_VIEW_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(TAB_VIEW_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), TAB_VIEW_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(TAB_VIEW_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_tab_view_init(EGUI_VIEW_OF(&tab_view_primary));
    egui_view_set_size(EGUI_VIEW_OF(&tab_view_primary), TAB_VIEW_PRIMARY_WIDTH, TAB_VIEW_PRIMARY_HEIGHT);
    egui_view_tab_view_set_font(EGUI_VIEW_OF(&tab_view_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_tab_view_set_meta_font(EGUI_VIEW_OF(&tab_view_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_tab_view_set_snapshots(EGUI_VIEW_OF(&tab_view_primary), primary_snapshots, 2);
    egui_view_tab_view_set_on_changed_listener(EGUI_VIEW_OF(&tab_view_primary), on_primary_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&tab_view_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&tab_view_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), TAB_VIEW_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(TAB_VIEW_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 150, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDEE4EB));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), TAB_VIEW_BOTTOM_ROW_WIDTH, TAB_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), TAB_VIEW_PREVIEW_WIDTH, TAB_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), TAB_VIEW_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(TAB_VIEW_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_tab_view_init(EGUI_VIEW_OF(&tab_view_compact));
    egui_view_set_size(EGUI_VIEW_OF(&tab_view_compact), TAB_VIEW_PREVIEW_WIDTH, TAB_VIEW_PREVIEW_HEIGHT);
    egui_view_tab_view_set_font(EGUI_VIEW_OF(&tab_view_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_tab_view_set_meta_font(EGUI_VIEW_OF(&tab_view_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_tab_view_set_snapshots(EGUI_VIEW_OF(&tab_view_compact), compact_snapshots, 2);
    egui_view_tab_view_set_compact_mode(EGUI_VIEW_OF(&tab_view_compact), 1);
    egui_view_tab_view_set_palette(EGUI_VIEW_OF(&tab_view_compact), EGUI_COLOR_HEX(0xFEFFFF), EGUI_COLOR_HEX(0xC9D9D4), EGUI_COLOR_HEX(0xF1F8F6),
                                   EGUI_COLOR_HEX(0x17312A), EGUI_COLOR_HEX(0x4E6E65), EGUI_COLOR_HEX(0x0E7C70), EGUI_COLOR_HEX(0x178454),
                                   EGUI_COLOR_HEX(0xB87A16), EGUI_COLOR_HEX(0x657A88));
    static egui_view_api_t tab_view_compact_touch_api;
    egui_view_override_api_on_touch(EGUI_VIEW_OF(&tab_view_compact), &tab_view_compact_touch_api, consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&tab_view_compact), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&tab_view_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), TAB_VIEW_PREVIEW_WIDTH, TAB_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), TAB_VIEW_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(TAB_VIEW_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_tab_view_init(EGUI_VIEW_OF(&tab_view_locked));
    egui_view_set_size(EGUI_VIEW_OF(&tab_view_locked), TAB_VIEW_PREVIEW_WIDTH, TAB_VIEW_PREVIEW_HEIGHT);
    egui_view_tab_view_set_font(EGUI_VIEW_OF(&tab_view_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_tab_view_set_meta_font(EGUI_VIEW_OF(&tab_view_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_tab_view_set_snapshots(EGUI_VIEW_OF(&tab_view_locked), locked_snapshots, 1);
    egui_view_tab_view_set_compact_mode(EGUI_VIEW_OF(&tab_view_locked), 1);
    egui_view_tab_view_set_read_only_mode(EGUI_VIEW_OF(&tab_view_locked), 1);
    egui_view_tab_view_set_palette(EGUI_VIEW_OF(&tab_view_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xD8E0E7), EGUI_COLOR_HEX(0xF5F7F9),
                                   EGUI_COLOR_HEX(0x4F6372), EGUI_COLOR_HEX(0x8392A0), EGUI_COLOR_HEX(0x8EA0B5), EGUI_COLOR_HEX(0x9CA3AF),
                                   EGUI_COLOR_HEX(0x9CA3AF), EGUI_COLOR_HEX(0x9AA5B3));
    static egui_view_api_t tab_view_locked_touch_api;
    egui_view_override_api_on_touch(EGUI_VIEW_OF(&tab_view_locked), &tab_view_locked_touch_api, consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&tab_view_locked), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&tab_view_locked));

    update_status();
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
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            primary_snapshot_index = 0;
            compact_snapshot_index = 0;
            egui_view_tab_view_set_current_snapshot(EGUI_VIEW_OF(&tab_view_primary), 0);
            egui_view_tab_view_restore_tabs(EGUI_VIEW_OF(&tab_view_primary));
            egui_view_tab_view_set_current_tab(EGUI_VIEW_OF(&tab_view_primary), 0);
            egui_view_tab_view_set_current_snapshot(EGUI_VIEW_OF(&tab_view_compact), 0);
            update_status();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        if (first_call)
        {
            egui_view_tab_view_set_current_tab(EGUI_VIEW_OF(&tab_view_primary), 2);
            update_status();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 2:
        if (first_call)
        {
            egui_view_tab_view_close_current_tab(EGUI_VIEW_OF(&tab_view_primary));
            update_status();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 3:
        if (first_call)
        {
            egui_view_tab_view_restore_tabs(EGUI_VIEW_OF(&tab_view_primary));
            update_status();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 4:
        if (first_call)
        {
            primary_snapshot_index = 1;
            egui_view_tab_view_set_current_snapshot(EGUI_VIEW_OF(&tab_view_primary), 1);
            update_status();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 5:
        if (first_call)
        {
            compact_snapshot_index = 1;
            egui_view_tab_view_set_current_snapshot(EGUI_VIEW_OF(&tab_view_compact), 1);
            update_status();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 360);
        return true;
    default:
        return false;
    }
}
#endif
