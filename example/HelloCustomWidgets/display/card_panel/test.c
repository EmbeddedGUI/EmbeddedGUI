#include <stdlib.h>

#include "egui.h"
#include "egui_view_card_panel.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define CARD_PANEL_PRIMARY_WIDTH  196
#define CARD_PANEL_PRIMARY_HEIGHT 122
#define CARD_PANEL_BOTTOM_WIDTH   212
#define CARD_PANEL_BOTTOM_HEIGHT  104
#define CARD_PANEL_COLUMN_WIDTH   104
#define CARD_PANEL_CARD_HEIGHT    90

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_card_panel_t panel_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_card_panel_t panel_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_card_panel_t panel_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF3F5F8), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Card Panel";
static const char *guide_text = "Tap cards to review states";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {147, 0}};

static const egui_view_card_panel_snapshot_t primary_snapshots[] = {
        {"OVERVIEW", "Workspace status", "Three flows stay aligned.", "98%", "uptime", "Today", "Two checks wait.", "Footer stays readable.", "Open", 0, 1},
        {"SYNC", "Design review", "New handoff needs approval.", "4", "changes", "Next step", "Confirm spacing tokens.", "Summary stays close.", "Review", 2,
         1},
        {"DEPLOY", "Release notes", "Ready for staged publish.", "6", "items", "Channel", "Internal preview for QA.", "Card stays calm on dense pages.",
         "Publish", 1, 0},
        {"ARCHIVE", "Readback summary", "Older detail stays available.", "12", "pages", "History", "Summary stays visible.", "Read only mode still works.",
         "Browse", 3, 0},
};

static const egui_view_card_panel_snapshot_t compact_snapshots[] = {
        {"TASK", "Compact", "Short.", "12", "tasks", "Focus", "", "Clear layout.", "Open", 0, 1},
        {"WARN", "Review", "Warning.", "2", "blocks", "Check", "", "Small card.", "Fix", 2, 1},
};

static const egui_view_card_panel_snapshot_t locked_snapshots[] = {
        {"NOTE", "Locked", "Muted.", "7", "notes", "Archive", "", "Preview only.", "", 3, 0},
        {"STATE", "Stable", "Passive.", "24", "items", "Review", "", "No extra chrome.", "", 0, 0},
};

static egui_color_t card_panel_status_color(uint8_t tone)
{
    switch (tone)
    {
    case 1:
        return EGUI_COLOR_HEX(0x177A43);
    case 2:
        return EGUI_COLOR_HEX(0xA96E0F);
    case 3:
        return EGUI_COLOR_HEX(0x6A7480);
    default:
        return EGUI_COLOR_HEX(0x265FC8);
    }
}

static const char *card_panel_status_text(uint8_t index)
{
    switch (index)
    {
    case 1:
        return "Warning card active";
    case 2:
        return "Success card active";
    case 3:
        return "Neutral card active";
    default:
        return "Accent card active";
    }
}

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void apply_primary_snapshot(uint8_t index)
{
    egui_view_card_panel_set_current_snapshot(EGUI_VIEW_OF(&panel_primary), index);
    set_status(card_panel_status_text(index), card_panel_status_color(primary_snapshots[index].tone));
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    egui_view_card_panel_set_current_snapshot(EGUI_VIEW_OF(&panel_compact), index);
    if (!update_status)
    {
        return;
    }

    if (index == 0)
    {
        set_status("Compact accent preview", card_panel_status_color(compact_snapshots[index].tone));
    }
    else
    {
        set_status("Compact warning preview", card_panel_status_color(compact_snapshots[index].tone));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_card_panel_get_current_snapshot(self) + 1) % 4;

    apply_primary_snapshot(next);
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_card_panel_get_current_snapshot(self) + 1) % 2;

    EGUI_UNUSED(self);
    apply_compact_snapshot(next, 1);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 292);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x6C7B88), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_card_panel_init(EGUI_VIEW_OF(&panel_primary));
    egui_view_set_size(EGUI_VIEW_OF(&panel_primary), CARD_PANEL_PRIMARY_WIDTH, CARD_PANEL_PRIMARY_HEIGHT);
    egui_view_card_panel_set_snapshots(EGUI_VIEW_OF(&panel_primary), primary_snapshots, 4);
    egui_view_card_panel_set_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_card_panel_set_meta_font(EGUI_VIEW_OF(&panel_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_card_panel_set_palette(EGUI_VIEW_OF(&panel_primary), EGUI_COLOR_HEX(0xFEFEFF), EGUI_COLOR_HEX(0xD3DCEA), EGUI_COLOR_HEX(0x17212B),
                                     EGUI_COLOR_HEX(0x657487), EGUI_COLOR_HEX(0x2E63DA), EGUI_COLOR_HEX(0x178454), EGUI_COLOR_HEX(0xB67619),
                                     EGUI_COLOR_HEX(0x768392));
    egui_view_set_margin(EGUI_VIEW_OF(&panel_primary), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&panel_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&panel_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Accent card active");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), card_panel_status_color(0), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 148, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xD9E1E8));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), CARD_PANEL_BOTTOM_WIDTH, CARD_PANEL_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), CARD_PANEL_COLUMN_WIDTH, CARD_PANEL_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), CARD_PANEL_COLUMN_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0x1E756E), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_card_panel_init(EGUI_VIEW_OF(&panel_compact));
    egui_view_set_size(EGUI_VIEW_OF(&panel_compact), CARD_PANEL_COLUMN_WIDTH, CARD_PANEL_CARD_HEIGHT);
    egui_view_card_panel_set_snapshots(EGUI_VIEW_OF(&panel_compact), compact_snapshots, 2);
    egui_view_card_panel_set_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_card_panel_set_meta_font(EGUI_VIEW_OF(&panel_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_card_panel_set_compact_mode(EGUI_VIEW_OF(&panel_compact), 1);
    egui_view_card_panel_set_palette(EGUI_VIEW_OF(&panel_compact), EGUI_COLOR_HEX(0xFCFDFF), EGUI_COLOR_HEX(0xD5DEEA), EGUI_COLOR_HEX(0x233243),
                                     EGUI_COLOR_HEX(0x66788C), EGUI_COLOR_HEX(0x2E63DA), EGUI_COLOR_HEX(0x178454), EGUI_COLOR_HEX(0xB67619),
                                     EGUI_COLOR_HEX(0x768392));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&panel_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&panel_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), CARD_PANEL_COLUMN_WIDTH, CARD_PANEL_BOTTOM_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), CARD_PANEL_COLUMN_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(0x8592A0), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_card_panel_init(EGUI_VIEW_OF(&panel_locked));
    egui_view_set_size(EGUI_VIEW_OF(&panel_locked), CARD_PANEL_COLUMN_WIDTH, CARD_PANEL_CARD_HEIGHT);
    egui_view_card_panel_set_snapshots(EGUI_VIEW_OF(&panel_locked), locked_snapshots, 2);
    egui_view_card_panel_set_current_snapshot(EGUI_VIEW_OF(&panel_locked), 1);
    egui_view_card_panel_set_font(EGUI_VIEW_OF(&panel_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_card_panel_set_meta_font(EGUI_VIEW_OF(&panel_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_card_panel_set_compact_mode(EGUI_VIEW_OF(&panel_locked), 1);
    egui_view_card_panel_set_locked_mode(EGUI_VIEW_OF(&panel_locked), 1);
    egui_view_card_panel_set_palette(EGUI_VIEW_OF(&panel_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xD8E0E6), EGUI_COLOR_HEX(0x5E6C79),
                                     EGUI_COLOR_HEX(0x8E99A5), EGUI_COLOR_HEX(0x94A3B1), EGUI_COLOR_HEX(0x7DA488), EGUI_COLOR_HEX(0xB18E4B),
                                     EGUI_COLOR_HEX(0x96A1AC));
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&panel_locked));

    apply_primary_snapshot(0);
    apply_compact_snapshot(0, 0);

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
            apply_primary_snapshot(0);
            apply_compact_snapshot(0, 0);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 1:
        if (first_call)
        {
            apply_primary_snapshot(1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_snapshot(2);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 3:
        if (first_call)
        {
            apply_compact_snapshot(1, 1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_snapshot(3);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 640);
        return true;
    default:
        return false;
    }
}
#endif
