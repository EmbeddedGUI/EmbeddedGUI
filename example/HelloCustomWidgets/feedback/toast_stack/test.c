#include <stdlib.h>

#include "egui.h"
#include "egui_view_toast_stack.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define TOAST_PRIMARY_WIDTH  196
#define TOAST_PRIMARY_HEIGHT 108
#define TOAST_BOTTOM_WIDTH   212
#define TOAST_BOTTOM_HEIGHT  96
#define TOAST_COLUMN_WIDTH   104
#define TOAST_CARD_HEIGHT    83

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_toast_stack_t stack_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_toast_stack_t stack_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_toast_stack_t stack_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Toast Stack";
static const char *guide_text = "Tap stacks to review states";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {143, 0}};

static const egui_view_toast_stack_snapshot_t primary_snapshots[] = {
        {"Backup ready", "Open the latest note.", "Open", "Now", "Shift starts", "Daily sync", 0, 1},
        {"Draft", "Team can review the build.", "Review", "1 min", "Queue", "Pinned", 1, 1},
        {"Storage low", "Archive logs before sync.", "Manage", "4 min", "Sync waiting", "Quota alert", 2, 1},
        {"Upload failed", "Reconnect to send report.", "Retry", "Offline", "Auth expired", "Queue paused", 3, 1},
};

static const egui_view_toast_stack_snapshot_t compact_snapshots[] = {
        {"Quota alert", "Archive.", "Manage", "Soon", "Sync", "Pinned note", 2, 0},
        {"Upload failed", "Retry send.", "Retry", "Offline", "Draft sent", "Queue paused", 3, 0},
};

static const egui_view_toast_stack_snapshot_t locked_snapshots[] = {
        {"Policy note", "Pinned for this page.", NULL, "Pinned", "Daily sync", "Workspace", 0, 0},
        {"Review ready", "Read only.", NULL, "Locked", "Queue settled", "Pinned note", 1, 0},
};

static egui_color_t toast_status_color(uint8_t severity)
{
    switch (severity)
    {
    case 1:
        return EGUI_COLOR_HEX(0x177A43);
    case 2:
        return EGUI_COLOR_HEX(0xA96E0F);
    case 3:
        return EGUI_COLOR_HEX(0xB53E39);
    default:
        return EGUI_COLOR_HEX(0x265FC8);
    }
}

static const char *toast_primary_status(uint8_t severity)
{
    switch (severity)
    {
    case 1:
        return "Success toast active";
    case 2:
        return "Warning toast active";
    case 3:
        return "Error toast active";
    default:
        return "Info toast active";
    }
}

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void apply_primary_snapshot(uint8_t index)
{
    egui_view_toast_stack_set_current_snapshot(EGUI_VIEW_OF(&stack_primary), index);
    set_status(toast_primary_status(primary_snapshots[index].severity), toast_status_color(primary_snapshots[index].severity));
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    egui_view_toast_stack_set_current_snapshot(EGUI_VIEW_OF(&stack_compact), index);
    if (!update_status)
    {
        return;
    }

    if (index == 0)
    {
        set_status("Compact warning preview", toast_status_color(2));
    }
    else
    {
        set_status("Compact error preview", toast_status_color(3));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_toast_stack_get_current_snapshot(self) + 1) % 4;

    apply_primary_snapshot(next);
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_toast_stack_get_current_snapshot(self) + 1) % 2;

    EGUI_UNUSED(self);
    apply_compact_snapshot(next, 1);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x6C7B88), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_toast_stack_init(EGUI_VIEW_OF(&stack_primary));
    egui_view_set_size(EGUI_VIEW_OF(&stack_primary), TOAST_PRIMARY_WIDTH, TOAST_PRIMARY_HEIGHT);
    egui_view_toast_stack_set_snapshots(EGUI_VIEW_OF(&stack_primary), primary_snapshots, 4);
    egui_view_toast_stack_set_font(EGUI_VIEW_OF(&stack_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_toast_stack_set_meta_font(EGUI_VIEW_OF(&stack_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_set_margin(EGUI_VIEW_OF(&stack_primary), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&stack_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&stack_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), toast_primary_status(0));
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), toast_status_color(0), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 144, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDAE2E8));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), TOAST_BOTTOM_WIDTH, TOAST_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), TOAST_COLUMN_WIDTH, TOAST_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), TOAST_COLUMN_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0x146C63), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_toast_stack_init(EGUI_VIEW_OF(&stack_compact));
    egui_view_set_size(EGUI_VIEW_OF(&stack_compact), TOAST_COLUMN_WIDTH, TOAST_CARD_HEIGHT);
    egui_view_toast_stack_set_snapshots(EGUI_VIEW_OF(&stack_compact), compact_snapshots, 2);
    egui_view_toast_stack_set_font(EGUI_VIEW_OF(&stack_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_toast_stack_set_meta_font(EGUI_VIEW_OF(&stack_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_toast_stack_set_compact_mode(EGUI_VIEW_OF(&stack_compact), 1);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&stack_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&stack_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), TOAST_COLUMN_WIDTH, TOAST_BOTTOM_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), TOAST_COLUMN_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(0x7D8B98), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_toast_stack_init(EGUI_VIEW_OF(&stack_locked));
    egui_view_set_size(EGUI_VIEW_OF(&stack_locked), TOAST_COLUMN_WIDTH, TOAST_CARD_HEIGHT);
    egui_view_toast_stack_set_snapshots(EGUI_VIEW_OF(&stack_locked), locked_snapshots, 2);
    egui_view_toast_stack_set_current_snapshot(EGUI_VIEW_OF(&stack_locked), 1);
    egui_view_toast_stack_set_font(EGUI_VIEW_OF(&stack_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_toast_stack_set_meta_font(EGUI_VIEW_OF(&stack_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_toast_stack_set_compact_mode(EGUI_VIEW_OF(&stack_locked), 1);
    egui_view_toast_stack_set_locked_mode(EGUI_VIEW_OF(&stack_locked), 1);
    egui_view_toast_stack_set_palette(EGUI_VIEW_OF(&stack_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xD8E0E6), EGUI_COLOR_HEX(0x5E6C79),
                                      EGUI_COLOR_HEX(0x8E99A5), EGUI_COLOR_HEX(0x94A3B1), EGUI_COLOR_HEX(0x5B81C6), EGUI_COLOR_HEX(0x6C8F77),
                                      EGUI_COLOR_HEX(0xB18E4B), EGUI_COLOR_HEX(0xB18A8A));
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&stack_locked));

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
            apply_primary_snapshot(3);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 4:
        if (first_call)
        {
            apply_compact_snapshot(1, 1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 640);
        return true;
    default:
        return false;
    }
}
#endif
