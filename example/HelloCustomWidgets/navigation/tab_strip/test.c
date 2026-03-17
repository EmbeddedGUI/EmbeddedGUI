#include <stdlib.h>

#include "egui.h"
#include "egui_view_tab_strip.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define TAB_STRIP_PRIMARY_WIDTH        204
#define TAB_STRIP_PRIMARY_HEIGHT       46
#define TAB_STRIP_COMPACT_WIDTH        106
#define TAB_STRIP_COMPACT_HEIGHT       36
#define TAB_STRIP_BOTTOM_ROW_WIDTH     216
#define TAB_STRIP_BOTTOM_ROW_HEIGHT    76
#define TAB_STRIP_GUIDE_COLOR          0x6E7B8B
#define TAB_STRIP_PRIMARY_LABEL_COLOR  0x768392
#define TAB_STRIP_STATUS_COLOR         0x4D6781
#define TAB_STRIP_COMPACT_LABEL_COLOR  0x0C756C
#define TAB_STRIP_COMPACT_BORDER_COLOR 0xD3DEDA
#define TAB_STRIP_COMPACT_ACCENT_COLOR 0x0C7C73
#define TAB_STRIP_LOCKED_LABEL_COLOR   0x8794A2
#define TAB_STRIP_LOCKED_BORDER_COLOR  0xDBE2E8
#define TAB_STRIP_LOCKED_ACCENT_COLOR  0x96A4B4

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_tab_strip_t bar_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_tab_strip_t bar_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_tab_strip_t bar_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Tab Strip";
static const char *guide_text = "Tap tabs to switch focus";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {143, 0}};

static const char *primary_tabs[] = {"Overview", "Usage", "Access"};
static const char *primary_statuses[] = {"Overview pane ready", "Usage summary open", "Access rules active"};
static const char *compact_tabs[] = {"Home", "Logs"};
static const char *locked_tabs[] = {"Usage", "Audit"};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x536677), 28), EGUI_ALPHA_100);
}

static void on_primary_tab_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);

    if (index < 3)
    {
        set_status(primary_statuses[index], EGUI_COLOR_HEX(0x2563EB));
    }
}

static void on_compact_tab_changed(egui_view_t *self, uint8_t index)
{
    static const char *compact_statuses[] = {"Compact home ready", "Compact logs view"};

    EGUI_UNUSED(self);

    if (index < 2)
    {
        set_status(compact_statuses[index], EGUI_COLOR_HEX(0x0F766E));
    }
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 284);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(TAB_STRIP_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), 224, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(TAB_STRIP_PRIMARY_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_tab_strip_init(EGUI_VIEW_OF(&bar_primary));
    egui_view_set_size(EGUI_VIEW_OF(&bar_primary), TAB_STRIP_PRIMARY_WIDTH, TAB_STRIP_PRIMARY_HEIGHT);
    egui_view_tab_strip_set_tabs(EGUI_VIEW_OF(&bar_primary), primary_tabs, 3);
    egui_view_tab_strip_set_current_index(EGUI_VIEW_OF(&bar_primary), 0);
    egui_view_tab_strip_set_font(EGUI_VIEW_OF(&bar_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_tab_strip_set_on_tab_changed_listener(EGUI_VIEW_OF(&bar_primary), on_primary_tab_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&bar_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bar_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), primary_statuses[0]);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(TAB_STRIP_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), TAB_STRIP_BOTTOM_ROW_WIDTH, TAB_STRIP_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), TAB_STRIP_COMPACT_WIDTH, TAB_STRIP_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), TAB_STRIP_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(TAB_STRIP_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_tab_strip_init(EGUI_VIEW_OF(&bar_compact));
    egui_view_set_size(EGUI_VIEW_OF(&bar_compact), TAB_STRIP_COMPACT_WIDTH, TAB_STRIP_COMPACT_HEIGHT);
    egui_view_tab_strip_set_tabs(EGUI_VIEW_OF(&bar_compact), compact_tabs, 2);
    egui_view_tab_strip_set_current_index(EGUI_VIEW_OF(&bar_compact), 0);
    egui_view_tab_strip_set_font(EGUI_VIEW_OF(&bar_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_tab_strip_set_compact_mode(EGUI_VIEW_OF(&bar_compact), 1);
    egui_view_tab_strip_set_palette(EGUI_VIEW_OF(&bar_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(TAB_STRIP_COMPACT_BORDER_COLOR),
                                    EGUI_COLOR_HEX(0x203039), EGUI_COLOR_HEX(0x71818D), EGUI_COLOR_HEX(TAB_STRIP_COMPACT_ACCENT_COLOR));
    egui_view_tab_strip_set_on_tab_changed_listener(EGUI_VIEW_OF(&bar_compact), on_compact_tab_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&bar_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), TAB_STRIP_COMPACT_WIDTH, TAB_STRIP_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), TAB_STRIP_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(TAB_STRIP_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_tab_strip_init(EGUI_VIEW_OF(&bar_locked));
    egui_view_set_size(EGUI_VIEW_OF(&bar_locked), TAB_STRIP_COMPACT_WIDTH, TAB_STRIP_COMPACT_HEIGHT);
    egui_view_tab_strip_set_tabs(EGUI_VIEW_OF(&bar_locked), locked_tabs, 2);
    egui_view_tab_strip_set_current_index(EGUI_VIEW_OF(&bar_locked), 0);
    egui_view_tab_strip_set_font(EGUI_VIEW_OF(&bar_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_tab_strip_set_compact_mode(EGUI_VIEW_OF(&bar_locked), 1);
    egui_view_tab_strip_set_palette(EGUI_VIEW_OF(&bar_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(TAB_STRIP_LOCKED_BORDER_COLOR),
                                    EGUI_COLOR_HEX(0x556473), EGUI_COLOR_HEX(0x8B98A7), EGUI_COLOR_HEX(TAB_STRIP_LOCKED_ACCENT_COLOR));
    egui_view_tab_strip_set_locked_mode(EGUI_VIEW_OF(&bar_locked), 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&bar_locked));

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
            egui_view_tab_strip_set_current_index(EGUI_VIEW_OF(&bar_primary), 0);
            egui_view_tab_strip_set_current_index(EGUI_VIEW_OF(&bar_compact), 0);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    case 1:
        if (first_call)
        {
            egui_view_tab_strip_set_current_index(EGUI_VIEW_OF(&bar_primary), 1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    case 2:
        if (first_call)
        {
            egui_view_tab_strip_set_current_index(EGUI_VIEW_OF(&bar_primary), 2);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 260);
        return true;
    case 3:
        if (first_call)
        {
            egui_view_tab_strip_set_current_index(EGUI_VIEW_OF(&bar_compact), 1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 640);
        return true;
    default:
        return false;
    }
}
#endif
