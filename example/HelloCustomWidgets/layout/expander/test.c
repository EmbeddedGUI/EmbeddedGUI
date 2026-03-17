#include "egui.h"
#include "egui_view_expander.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define EXPANDER_ROOT_W           224
#define EXPANDER_ROOT_H           296
#define EXPANDER_PRIMARY_W        194
#define EXPANDER_PRIMARY_H        110
#define EXPANDER_PREVIEW_W        108
#define EXPANDER_PREVIEW_H        76
#define EXPANDER_BOTTOM_W         222
#define EXPANDER_BOTTOM_H         92
#define EXPANDER_GUIDE_COLOR      0x788491
#define EXPANDER_LABEL_COLOR      0x74808D
#define EXPANDER_STATUS_COLOR     0x4C657C
#define EXPANDER_COMPACT_COLOR    0x0E776E
#define EXPANDER_READ_ONLY_COLOR  0x8994A0
#define EXPANDER_STATUS_COLOR_MIX 24

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t standard_label;
static egui_view_expander_t panel_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_expander_t panel_compact;
static egui_view_linearlayout_t read_only_column;
static egui_view_label_t read_only_label;
static egui_view_expander_t panel_read_only;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Expander";
static const char *guide_text = "Tap headers or press Enter";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[72] = "Workspace policy: Expanded";

static const egui_view_expander_item_t primary_items[] = {
        {"WORK", "Workspace policy", "Ready", "Pinned groups stay open", "Draft rules stay visible", "Always on", EGUI_VIEW_EXPANDER_TONE_ACCENT, 1},
        {"SYNC", "Sync rules", "3 rules", "Metered uploads wait for Wi-Fi", "Night copy stays local", "Queue review", EGUI_VIEW_EXPANDER_TONE_SUCCESS, 0},
        {"RELEASE", "Release notes", "Hold", "Pilot warnings stay staged", "Manual signoff closes it", "Manual hold", EGUI_VIEW_EXPANDER_TONE_WARNING, 1},
};

static const egui_view_expander_item_t compact_items[] = {
        {"FAST", "Mode", "On", "One row open", "", "Review", EGUI_VIEW_EXPANDER_TONE_ACCENT, 1},
        {"BACKUP", "Backup", "Pause", "Waits for charge", "", "Resume", EGUI_VIEW_EXPANDER_TONE_WARNING, 0},
};

static const egui_view_expander_item_t read_only_items[] = {
        {"AUDIT", "Audit", "Lock", "History locked", "", "Read only", EGUI_VIEW_EXPANDER_TONE_NEUTRAL, 0},
        {"HISTORY", "History", "Arc", "Searchable notes", "", "Frozen", EGUI_VIEW_EXPANDER_TONE_SUCCESS, 0},
};

static egui_color_t expander_status_color(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_EXPANDER_TONE_SUCCESS:
        return EGUI_COLOR_HEX(0x178454);
    case EGUI_VIEW_EXPANDER_TONE_WARNING:
        return EGUI_COLOR_HEX(0xA86F12);
    case EGUI_VIEW_EXPANDER_TONE_NEUTRAL:
        return EGUI_COLOR_HEX(0x738291);
    default:
        return EGUI_COLOR_HEX(0x2563EB);
    }
}

static void set_status_message(const char *title, const char *suffix, egui_color_t color)
{
    int pos;

    status_text[0] = '\0';
    pos = egui_sprintf_str(status_text, sizeof(status_text), title);
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, ": ");
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, suffix);
    }
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x56687A), EXPANDER_STATUS_COLOR_MIX), EGUI_ALPHA_100);
}

static void refresh_primary_status(void)
{
    uint8_t index = egui_view_expander_get_current_index(EGUI_VIEW_OF(&panel_primary));
    uint8_t expanded = egui_view_expander_get_expanded_index(EGUI_VIEW_OF(&panel_primary)) == index;

    set_status_message(primary_items[index].title, expanded ? "Expanded" : "Focused", expander_status_color(primary_items[index].tone));
}

static void refresh_compact_status(void)
{
    uint8_t index = egui_view_expander_get_current_index(EGUI_VIEW_OF(&panel_compact));
    uint8_t expanded = egui_view_expander_get_expanded_index(EGUI_VIEW_OF(&panel_compact)) == index;

    set_status_message(compact_items[index].title, expanded ? "Compact open" : "Compact collapsed", expander_status_color(compact_items[index].tone));
}

static void apply_primary_state(uint8_t index, uint8_t expanded_index)
{
    egui_view_expander_set_current_index(EGUI_VIEW_OF(&panel_primary), index);
    egui_view_expander_set_expanded_index(EGUI_VIEW_OF(&panel_primary), expanded_index);
    refresh_primary_status();
}

static void apply_compact_state(uint8_t index, uint8_t expanded_index, uint8_t update_status)
{
    egui_view_expander_set_current_index(EGUI_VIEW_OF(&panel_compact), index);
    egui_view_expander_set_expanded_index(EGUI_VIEW_OF(&panel_compact), expanded_index);
    if (update_status)
    {
        refresh_compact_status();
    }
}

static void on_primary_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(index);
    refresh_primary_status();
}

static void on_primary_expanded_changed(egui_view_t *self, uint8_t index, uint8_t expanded)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(index);
    EGUI_UNUSED(expanded);
    refresh_primary_status();
}

static void on_compact_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(index);
    refresh_compact_status();
}

static void on_compact_expanded_changed(egui_view_t *self, uint8_t index, uint8_t expanded)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(index);
    EGUI_UNUSED(expanded);
    refresh_compact_status();
}

static void on_guide_click(egui_view_t *self)
{
    uint8_t current = egui_view_expander_get_current_index(EGUI_VIEW_OF(&panel_primary));
    uint8_t expanded = egui_view_expander_get_expanded_index(EGUI_VIEW_OF(&panel_primary));

    EGUI_UNUSED(self);
    if (current == 0 && expanded == 0)
    {
        apply_primary_state(1, 1);
    }
    else if (current == 1 && expanded == 1)
    {
        apply_primary_state(1, EGUI_VIEW_EXPANDER_INDEX_NONE);
    }
    else if (current == 1 && expanded == EGUI_VIEW_EXPANDER_INDEX_NONE)
    {
        apply_primary_state(2, 2);
    }
    else
    {
        apply_primary_state(0, 0);
    }
}

static void on_compact_label_click(egui_view_t *self)
{
    uint8_t current = egui_view_expander_get_current_index(EGUI_VIEW_OF(&panel_compact));
    uint8_t expanded = egui_view_expander_get_expanded_index(EGUI_VIEW_OF(&panel_compact));

    EGUI_UNUSED(self);
    if (current == 0 && expanded == 0)
    {
        apply_compact_state(1, 1, 1);
    }
    else if (current == 1 && expanded == 1)
    {
        apply_compact_state(1, EGUI_VIEW_EXPANDER_INDEX_NONE, 1);
    }
    else
    {
        apply_compact_state(0, 0, 1);
    }
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), EXPANDER_ROOT_W, EXPANDER_ROOT_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), EXPANDER_ROOT_W, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 9, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), EXPANDER_ROOT_W, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(EXPANDER_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&standard_label));
    egui_view_set_size(EGUI_VIEW_OF(&standard_label), EXPANDER_ROOT_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&standard_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&standard_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&standard_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&standard_label), EGUI_COLOR_HEX(EXPANDER_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&standard_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&standard_label));

    egui_view_expander_init(EGUI_VIEW_OF(&panel_primary));
    egui_view_set_size(EGUI_VIEW_OF(&panel_primary), EXPANDER_PRIMARY_W, EXPANDER_PRIMARY_H);
    egui_view_expander_set_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_expander_set_meta_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_expander_set_items(EGUI_VIEW_OF(&panel_primary), primary_items, 3);
    egui_view_expander_set_on_selection_changed_listener(EGUI_VIEW_OF(&panel_primary), on_primary_selection_changed);
    egui_view_expander_set_on_expanded_changed_listener(EGUI_VIEW_OF(&panel_primary), on_primary_expanded_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&panel_primary), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&panel_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), EXPANDER_ROOT_W, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(EXPANDER_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), EXPANDER_BOTTOM_W, EXPANDER_BOTTOM_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_HCENTER);

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), EXPANDER_PREVIEW_W, EXPANDER_BOTTOM_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, 0, 6, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), EXPANDER_PREVIEW_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(EXPANDER_COMPACT_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_expander_init(EGUI_VIEW_OF(&panel_compact));
    egui_view_set_size(EGUI_VIEW_OF(&panel_compact), EXPANDER_PREVIEW_W, EXPANDER_PREVIEW_H);
    egui_view_expander_set_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_expander_set_meta_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_expander_set_items(EGUI_VIEW_OF(&panel_compact), compact_items, 2);
    egui_view_expander_set_compact_mode(EGUI_VIEW_OF(&panel_compact), 1);
    egui_view_expander_set_on_selection_changed_listener(EGUI_VIEW_OF(&panel_compact), on_compact_selection_changed);
    egui_view_expander_set_on_expanded_changed_listener(EGUI_VIEW_OF(&panel_compact), on_compact_expanded_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&panel_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&read_only_column));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_column), EXPANDER_PREVIEW_W, EXPANDER_BOTTOM_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&read_only_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&read_only_column), EGUI_ALIGN_HCENTER);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_column), 6, 0, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&read_only_column));

    egui_view_label_init(EGUI_VIEW_OF(&read_only_label));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_label), EXPANDER_PREVIEW_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&read_only_label), "Read Only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&read_only_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&read_only_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&read_only_label), EGUI_COLOR_HEX(EXPANDER_READ_ONLY_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&read_only_label));

    egui_view_expander_init(EGUI_VIEW_OF(&panel_read_only));
    egui_view_set_size(EGUI_VIEW_OF(&panel_read_only), EXPANDER_PREVIEW_W, EXPANDER_PREVIEW_H);
    egui_view_expander_set_font(EGUI_VIEW_OF(&panel_read_only), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_expander_set_meta_font(EGUI_VIEW_OF(&panel_read_only), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_expander_set_items(EGUI_VIEW_OF(&panel_read_only), read_only_items, 2);
    egui_view_expander_set_compact_mode(EGUI_VIEW_OF(&panel_read_only), 1);
    egui_view_expander_set_current_index(EGUI_VIEW_OF(&panel_read_only), 1);
    egui_view_expander_set_expanded_index(EGUI_VIEW_OF(&panel_read_only), 1);
    egui_view_expander_set_read_only_mode(EGUI_VIEW_OF(&panel_read_only), 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&panel_read_only));

    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    apply_primary_state(0, 0);
    apply_compact_state(0, 0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&read_only_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_invalidate(EGUI_VIEW_OF(&root_layout));
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&panel_primary));
#endif
#if EGUI_CONFIG_RECORDING_TEST
    recording_request_snapshot();
#endif
}

#if EGUI_CONFIG_RECORDING_TEST
static void request_page_snapshot(void)
{
    egui_view_invalidate(EGUI_VIEW_OF(&root_layout));
    recording_request_snapshot();
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
            apply_primary_state(0, 0);
            apply_compact_state(0, 0, 0);
            egui_view_expander_set_current_index(EGUI_VIEW_OF(&panel_read_only), 1);
            egui_view_expander_set_expanded_index(EGUI_VIEW_OF(&panel_read_only), 1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 1:
        if (first_call)
        {
            apply_primary_state(1, 1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_state(1, EGUI_VIEW_EXPANDER_INDEX_NONE);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 3:
        if (first_call)
        {
            apply_primary_state(2, 2);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 4:
        if (first_call)
        {
            apply_compact_state(1, 1, 1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 5:
        if (first_call)
        {
            apply_compact_state(1, EGUI_VIEW_EXPANDER_INDEX_NONE, 1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    case 6:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 520);
        return true;
    default:
        return false;
    }
}
#endif
