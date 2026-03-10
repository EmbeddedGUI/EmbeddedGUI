#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_signal_beacon.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_signal_beacon_t beacon_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_signal_beacon_t beacon_field;
static egui_view_signal_beacon_t beacon_lock;

static const char *title_text = "Signal Beacon";
static const char *guide_text = "Tap relays to switch incident state";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {147, 0}};
static const egui_color_t primary_status_color = EGUI_COLOR_HEX(0x37B2E3);
static const egui_color_t field_status_color = EGUI_COLOR_HEX(0x55C69A);
static const egui_color_t warn_status_color = EGUI_COLOR_HEX(0xD79D3D);
static const egui_color_t critical_status_color = EGUI_COLOR_HEX(0xE07B7B);
static const egui_color_t lock_status_color = EGUI_COLOR_HEX(0xD9C77A);

static const egui_view_signal_beacon_snapshot_t primary_snapshots[] = {
        {"CORE LINK", "CLEAR", "Dock 4 stable", "uplink clean", 1, 0, 0},
        {"CORE LINK", "WATCH", "Dock 4 jitter", "phase drift", 2, 2, 1},
        {"CORE LINK", "ALERT", "Dock 4 surge", "manual route", 3, 2, 2},
};

static const egui_view_signal_beacon_snapshot_t field_snapshots[] = {
        {"FIELD", "SYNC", "Yard east", "stable", 1, 0, 0},
        {"FIELD", "TRACE", "Yard east", "scan", 2, 2, 1},
        {"FIELD", "SYNC", "Yard west", "handoff", 2, 0, 0},
};

static const egui_view_signal_beacon_snapshot_t lock_snapshots[] = {
        {"LOCK", "IDLE", "Gate hush", "sealed", 1, 2, 0},
        {"LOCK", "HOLD", "Gate hush", "guard", 2, 2, 1},
        {"LOCK", "TRIP", "Gate loud", "audit", 3, 0, 2},
};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_signal_beacon_get_current_snapshot(self) + 1) % 3;
    egui_view_signal_beacon_set_current_snapshot(self, next);
    if (next == 0)
    {
        set_status("Core link clear", primary_status_color);
    }
    else if (next == 1)
    {
        set_status("Core link watch", warn_status_color);
    }
    else
    {
        set_status("Core link alert", critical_status_color);
    }
}

static void on_field_click(egui_view_t *self)
{
    uint8_t next = (egui_view_signal_beacon_get_current_snapshot(self) + 1) % 3;
    egui_view_signal_beacon_set_current_snapshot(self, next);
    if (next == 1)
    {
        set_status("Field relay tracing", field_status_color);
    }
    else
    {
        set_status("Field relay synced", field_status_color);
    }
}

static void on_lock_click(egui_view_t *self)
{
    uint8_t next = (egui_view_signal_beacon_get_current_snapshot(self) + 1) % 3;
    egui_view_signal_beacon_set_current_snapshot(self, next);
    if (next == 2)
    {
        set_status("Lock relay tripped", critical_status_color);
    }
    else if (next == 1)
    {
        set_status("Lock relay on hold", warn_status_color);
    }
    else
    {
        set_status("Lock relay idle", lock_status_color);
    }
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 220, 306);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 220, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0xA5D8FF), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 220, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x64778C), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_signal_beacon_init(EGUI_VIEW_OF(&beacon_primary));
    egui_view_set_size(EGUI_VIEW_OF(&beacon_primary), 192, 116);
    egui_view_signal_beacon_set_snapshots(EGUI_VIEW_OF(&beacon_primary), primary_snapshots, 3);
    egui_view_signal_beacon_set_font(EGUI_VIEW_OF(&beacon_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_set_margin(EGUI_VIEW_OF(&beacon_primary), 0, 0, 0, 7);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&beacon_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&beacon_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 220, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Core link clear");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), primary_status_color, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 148, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0x314454));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 210, 86);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_signal_beacon_init(EGUI_VIEW_OF(&beacon_field));
    egui_view_set_size(EGUI_VIEW_OF(&beacon_field), 108, 80);
    egui_view_signal_beacon_set_snapshots(EGUI_VIEW_OF(&beacon_field), field_snapshots, 3);
    egui_view_signal_beacon_set_font(EGUI_VIEW_OF(&beacon_field), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_signal_beacon_set_compact_mode(EGUI_VIEW_OF(&beacon_field), 1);
    egui_view_signal_beacon_set_palette(
            EGUI_VIEW_OF(&beacon_field),
            EGUI_COLOR_HEX(0x11251E),
            EGUI_COLOR_HEX(0x2A6653),
            EGUI_COLOR_HEX(0xD9FFF0),
            EGUI_COLOR_HEX(0x8BB6A4),
            EGUI_COLOR_HEX(0x3DDBA0),
            EGUI_COLOR_HEX(0xD7B04B),
            EGUI_COLOR_HEX(0xF07575),
            EGUI_COLOR_HEX(0x0B1512));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&beacon_field), on_field_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&beacon_field));

    egui_view_signal_beacon_init(EGUI_VIEW_OF(&beacon_lock));
    egui_view_set_size(EGUI_VIEW_OF(&beacon_lock), 100, 80);
    egui_view_signal_beacon_set_snapshots(EGUI_VIEW_OF(&beacon_lock), lock_snapshots, 3);
    egui_view_signal_beacon_set_font(EGUI_VIEW_OF(&beacon_lock), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_signal_beacon_set_compact_mode(EGUI_VIEW_OF(&beacon_lock), 1);
    egui_view_signal_beacon_set_locked_mode(EGUI_VIEW_OF(&beacon_lock), 1);
    egui_view_signal_beacon_set_palette(
            EGUI_VIEW_OF(&beacon_lock),
            EGUI_COLOR_HEX(0x241B14),
            EGUI_COLOR_HEX(0x6E4D2A),
            EGUI_COLOR_HEX(0xFFF2D9),
            EGUI_COLOR_HEX(0xC7A98B),
            EGUI_COLOR_HEX(0xEAC66A),
            EGUI_COLOR_HEX(0xF59E0B),
            EGUI_COLOR_HEX(0xF07878),
            EGUI_COLOR_HEX(0x140F0B));
    egui_view_set_margin(EGUI_VIEW_OF(&beacon_lock), 2, 0, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&beacon_lock), on_lock_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&beacon_lock));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&beacon_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&beacon_field), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&beacon_lock), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
