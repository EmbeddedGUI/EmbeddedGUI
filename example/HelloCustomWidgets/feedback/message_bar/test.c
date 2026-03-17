#include <stdlib.h>

#include "egui.h"
#include "egui_view_message_bar.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_message_bar_t bar_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_message_bar_t bar_compact;
static egui_view_linearlayout_t persistent_column;
static egui_view_label_t persistent_label;
static egui_view_message_bar_t bar_persistent;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF4F6F8), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Message Bar";
static const char *guide_text = "Tap bars to rotate severity";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {139, 0}};

static const egui_view_message_bar_snapshot_t primary_snapshots[] = {
        {"Updates ready", "Open latest release notes.", "View notes", 0, 1, 1},
        {"Settings saved", "New defaults are active.", "Open panel", 1, 1, 1},
        {"Storage almost full", "Clear logs before next sync.", "View logs", 2, 1, 1},
        {"Connection lost", "Uploads pause. Link is down.", "Retry now", 3, 1, 1},
};

static const egui_view_message_bar_snapshot_t compact_snapshots[] = {
        {"Quota alert", "Archive logs.", "View", 2, 0, 1},
        {"Sync failed", "Retry required.", "Retry", 3, 0, 1},
};

static const egui_view_message_bar_snapshot_t persistent_snapshots[] = {
        {"Policy note", "Admin pinned.", NULL, 0, 0, 0},
};

static egui_color_t message_bar_status_color(uint8_t severity)
{
    switch (severity)
    {
    case 1:
        return EGUI_COLOR_HEX(0x2D7C4D);
    case 2:
        return EGUI_COLOR_HEX(0xA86A0B);
    case 3:
        return EGUI_COLOR_HEX(0xB13B35);
    default:
        return EGUI_COLOR_HEX(0x1F4EA3);
    }
}

static const char *message_bar_status_text(uint8_t severity)
{
    switch (severity)
    {
    case 1:
        return "Success state active";
    case 2:
        return "Warning needs review";
    case 3:
        return "Error needs retry";
    default:
        return "Info state active";
    }
}

static void set_status_by_snapshot(const egui_view_message_bar_snapshot_t *snapshot, const char *fallback)
{
    const char *text = fallback;
    egui_color_t color = message_bar_status_color(0);

    if (snapshot != NULL)
    {
        text = message_bar_status_text(snapshot->severity);
        color = message_bar_status_color(snapshot->severity);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_message_bar_get_current_snapshot(self) + 1) % 4;

    egui_view_message_bar_set_current_snapshot(self, next);
    set_status_by_snapshot(&primary_snapshots[next], "Info state active");
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_message_bar_get_current_snapshot(self) + 1) % 2;

    egui_view_message_bar_set_current_snapshot(self, next);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), next == 0 ? "Compact warning preview" : "Compact error preview");
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), next == 0 ? message_bar_status_color(2) : message_bar_status_color(3), EGUI_ALPHA_100);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x223041), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 9, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x687383), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_message_bar_init(EGUI_VIEW_OF(&bar_primary));
    egui_view_set_size(EGUI_VIEW_OF(&bar_primary), 196, 96);
    egui_view_message_bar_set_snapshots(EGUI_VIEW_OF(&bar_primary), primary_snapshots, 4);
    egui_view_message_bar_set_current_snapshot(EGUI_VIEW_OF(&bar_primary), 0);
    egui_view_message_bar_set_font(EGUI_VIEW_OF(&bar_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&bar_primary), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&bar_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bar_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Info state active");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), message_bar_status_color(0), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 140, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xD8DEE6));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 212, 96);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), 104, 96);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), 104, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0x66717F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_message_bar_init(EGUI_VIEW_OF(&bar_compact));
    egui_view_set_size(EGUI_VIEW_OF(&bar_compact), 104, 82);
    egui_view_message_bar_set_snapshots(EGUI_VIEW_OF(&bar_compact), compact_snapshots, 2);
    egui_view_message_bar_set_current_snapshot(EGUI_VIEW_OF(&bar_compact), 0);
    egui_view_message_bar_set_font(EGUI_VIEW_OF(&bar_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_message_bar_set_compact_mode(EGUI_VIEW_OF(&bar_compact), 1);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&bar_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&bar_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&persistent_column));
    egui_view_set_size(EGUI_VIEW_OF(&persistent_column), 104, 96);
    egui_view_set_margin(EGUI_VIEW_OF(&persistent_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&persistent_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&persistent_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&persistent_column));

    egui_view_label_init(EGUI_VIEW_OF(&persistent_label));
    egui_view_set_size(EGUI_VIEW_OF(&persistent_label), 104, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&persistent_label), "Persistent");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&persistent_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&persistent_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&persistent_label), EGUI_COLOR_HEX(0x66717F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&persistent_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&persistent_column), EGUI_VIEW_OF(&persistent_label));

    egui_view_message_bar_init(EGUI_VIEW_OF(&bar_persistent));
    egui_view_set_size(EGUI_VIEW_OF(&bar_persistent), 104, 82);
    egui_view_message_bar_set_snapshots(EGUI_VIEW_OF(&bar_persistent), persistent_snapshots, 1);
    egui_view_message_bar_set_current_snapshot(EGUI_VIEW_OF(&bar_persistent), 0);
    egui_view_message_bar_set_font(EGUI_VIEW_OF(&bar_persistent), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_message_bar_set_compact_mode(EGUI_VIEW_OF(&bar_persistent), 1);
    egui_view_message_bar_set_locked_mode(EGUI_VIEW_OF(&bar_persistent), 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&persistent_column), EGUI_VIEW_OF(&bar_persistent));

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&persistent_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 400);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&bar_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 250);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&bar_primary), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 250);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&bar_primary), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 250);
        return true;
    case 7:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&bar_compact), 700);
        return true;
    case 8:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
