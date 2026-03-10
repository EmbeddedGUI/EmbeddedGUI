#define AVATAR_ROOT_WIDTH 230
#define AVATAR_ROOT_HEIGHT 306
#define AVATAR_TITLE_COLOR 0x63DDFF
#define AVATAR_GUIDE_TEXT "Tap stacks to cycle"
#define AVATAR_GUIDE_COLOR 0x839AB6
#define AVATAR_GUIDE_MARGIN_BOTTOM 3
#define AVATAR_PRIMARY_WIDTH 188
#define AVATAR_PRIMARY_HEIGHT 138
#define AVATAR_PRIMARY_MARGIN_BOTTOM 4
#define AVATAR_STATUS_TEXT_A "Core A"
#define AVATAR_STATUS_TEXT_B "Core B"
#define AVATAR_STATUS_TEXT_COMPACT_A "Compact A"
#define AVATAR_STATUS_TEXT_COMPACT_B "Compact B"
#define AVATAR_STATUS_PRIMARY_COLOR 0x56D3FF
#define AVATAR_STATUS_COMPACT_COLOR 0xF59E0B
#define AVATAR_STATUS_MARGIN_BOTTOM 4
#define AVATAR_DIVIDER_WIDTH 170
#define AVATAR_DIVIDER_POINTS 169
#define AVATAR_DIVIDER_PRIMARY 0x5C8BBC
#define AVATAR_DIVIDER_COMPACT 0x9A6A32
#define AVATAR_DIVIDER_MARGIN_BOTTOM 5
#define AVATAR_BOTTOM_ROW_WIDTH 230
#define AVATAR_BOTTOM_ROW_HEIGHT 106
#define AVATAR_COLUMN_WIDTH 108
#define AVATAR_COLUMN_HEIGHT 108
#define AVATAR_COLUMN_GAP 6
#define AVATAR_CARD_HEIGHT 92
#define AVATAR_COMPACT_LABEL_TEXT "Compact"
#define AVATAR_LOCKED_LABEL_TEXT "Locked"
#define AVATAR_COMPACT_LABEL_IDLE 0xB7C5D4
#define AVATAR_COMPACT_LABEL_ACTIVE 0xF4C96D
#define AVATAR_LOCKED_LABEL_COLOR 0xCCD6E1

#include <stdlib.h>

#include "egui.h"
#include "egui_view_avatar_stack.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_avatar_stack_t stack_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_avatar_stack_t stack_compact;
static egui_view_label_t compact_label;
static egui_view_linearlayout_t locked_column;
static egui_view_avatar_stack_t stack_locked;
static egui_view_label_t locked_label;

static const char *title_text = "Avatar Stack";
static const char *guide_text = AVATAR_GUIDE_TEXT;
static const egui_view_line_point_t section_divider_points[] = {{0, 0}, {AVATAR_DIVIDER_POINTS, 0}};

static const char *primary_initials_a[] = {"AL", "BK", "CY", "DN"};
static const char *primary_captions_a[] = {"Lead: Alice", "Focus: Blake", "Owner: Cindy", "Review: Dean"};
static const char *primary_initials_b[] = {"EM", "FN", "GR", "HS"};
static const char *primary_captions_b[] = {"Lead: Emma", "Focus: Finn", "Owner: Gray", "Review: Hana"};
static const egui_view_avatar_stack_snapshot_t primary_snapshots[] = {
        {"Squad A", primary_initials_a, primary_captions_a, 4, 7, 1},
        {"Squad B", primary_initials_b, primary_captions_b, 4, 6, 2},
};

static const char *compact_initials_a[] = {"QA", "UX", "PM"};
static const char *compact_captions_a[] = {"QA", "UX", "PM"};
static const char *compact_initials_b[] = {"FE", "BE", "OP"};
static const char *compact_captions_b[] = {"FE", "BE", "OP"};
static const egui_view_avatar_stack_snapshot_t compact_snapshots[] = {
        {"Compact A", compact_initials_a, compact_captions_a, 3, 5, 1},
        {"Compact B", compact_initials_b, compact_captions_b, 3, 4, 2},
};

static void set_status(const char *text, egui_color_t color, egui_color_t guide_color, egui_color_t divider_color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), guide_color, EGUI_ALPHA_100);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), divider_color);
}

static void apply_compact_state(uint8_t index, uint8_t is_active)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), AVATAR_COMPACT_LABEL_TEXT);
    egui_view_avatar_stack_set_focus_member(EGUI_VIEW_OF(&stack_compact), (index % 2) == 0 ? 1 : 2);
    if (is_active)
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(AVATAR_COMPACT_LABEL_ACTIVE), EGUI_ALPHA_100);
        egui_view_avatar_stack_set_palette(
                EGUI_VIEW_OF(&stack_compact),
                EGUI_COLOR_HEX(0x23160A),
                EGUI_COLOR_HEX(0x8B5E1A),
                EGUI_COLOR_HEX(0xFDE68A),
                EGUI_COLOR_HEX(0xD6A15B),
                EGUI_COLOR_HEX(0xF59E0B));
    }
    else
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(AVATAR_COMPACT_LABEL_IDLE), EGUI_ALPHA_100);
        egui_view_avatar_stack_set_palette(
                EGUI_VIEW_OF(&stack_compact),
                EGUI_COLOR_HEX(0x121A28),
                EGUI_COLOR_HEX(0x3D5068),
                EGUI_COLOR_HEX(0xCBD5E1),
                EGUI_COLOR_HEX(0x7F92A7),
                EGUI_COLOR_HEX(0x60A5FA));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_avatar_stack_get_current_snapshot(self) + 1) % 2;

    egui_view_avatar_stack_set_current_snapshot(self, next);
    egui_view_avatar_stack_set_focus_member(self, next == 0 ? 1 : 2);
    apply_compact_state(egui_view_avatar_stack_get_current_snapshot(EGUI_VIEW_OF(&stack_compact)), 0);
    set_status((next == 0) ? AVATAR_STATUS_TEXT_A : AVATAR_STATUS_TEXT_B, EGUI_COLOR_HEX(AVATAR_STATUS_PRIMARY_COLOR),
               EGUI_COLOR_HEX(AVATAR_GUIDE_COLOR), EGUI_COLOR_HEX(AVATAR_DIVIDER_PRIMARY));
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_avatar_stack_get_current_snapshot(self) + 1) % 2;

    egui_view_avatar_stack_set_current_snapshot(self, next);
    apply_compact_state(next, 1);
    set_status((next == 0) ? AVATAR_STATUS_TEXT_COMPACT_A : AVATAR_STATUS_TEXT_COMPACT_B, EGUI_COLOR_HEX(AVATAR_STATUS_COMPACT_COLOR),
               EGUI_COLOR_HEX(0xA07A42), EGUI_COLOR_HEX(AVATAR_DIVIDER_COMPACT));
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), AVATAR_ROOT_WIDTH, AVATAR_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), AVATAR_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(AVATAR_TITLE_COLOR), EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), AVATAR_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(AVATAR_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, AVATAR_GUIDE_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_avatar_stack_init(EGUI_VIEW_OF(&stack_primary));
    egui_view_set_size(EGUI_VIEW_OF(&stack_primary), AVATAR_PRIMARY_WIDTH, AVATAR_PRIMARY_HEIGHT);
    egui_view_avatar_stack_set_snapshots(EGUI_VIEW_OF(&stack_primary), primary_snapshots, 2);
    egui_view_avatar_stack_set_current_snapshot(EGUI_VIEW_OF(&stack_primary), 0);
    egui_view_avatar_stack_set_focus_member(EGUI_VIEW_OF(&stack_primary), 1);
    egui_view_avatar_stack_set_palette(
            EGUI_VIEW_OF(&stack_primary),
            EGUI_COLOR_HEX(0x0F1728),
            EGUI_COLOR_HEX(0x536379),
            EGUI_COLOR_HEX(0xE2E8F0),
            EGUI_COLOR_HEX(0x93A5BC),
            EGUI_COLOR_HEX(0x38BDF8));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&stack_primary), on_primary_click);
    egui_view_set_margin(EGUI_VIEW_OF(&stack_primary), 0, 0, 0, AVATAR_PRIMARY_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&stack_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), AVATAR_ROOT_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), AVATAR_STATUS_TEXT_A);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(AVATAR_STATUS_PRIMARY_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, AVATAR_STATUS_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), AVATAR_DIVIDER_WIDTH, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), section_divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(AVATAR_DIVIDER_PRIMARY));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, AVATAR_DIVIDER_MARGIN_BOTTOM);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), AVATAR_BOTTOM_ROW_WIDTH, AVATAR_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), AVATAR_COLUMN_WIDTH, AVATAR_COLUMN_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, AVATAR_COLUMN_GAP, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), AVATAR_COLUMN_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), AVATAR_COMPACT_LABEL_TEXT);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(AVATAR_COMPACT_LABEL_IDLE), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_avatar_stack_init(EGUI_VIEW_OF(&stack_compact));
    egui_view_set_size(EGUI_VIEW_OF(&stack_compact), AVATAR_COLUMN_WIDTH, AVATAR_CARD_HEIGHT);
    egui_view_avatar_stack_set_snapshots(EGUI_VIEW_OF(&stack_compact), compact_snapshots, 2);
    egui_view_avatar_stack_set_current_snapshot(EGUI_VIEW_OF(&stack_compact), 0);
    egui_view_avatar_stack_set_font(EGUI_VIEW_OF(&stack_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_avatar_stack_set_show_header(EGUI_VIEW_OF(&stack_compact), 0);
    egui_view_avatar_stack_set_compact_mode(EGUI_VIEW_OF(&stack_compact), 1);
    apply_compact_state(0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&stack_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&stack_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), AVATAR_COLUMN_WIDTH, AVATAR_COLUMN_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), AVATAR_COLUMN_GAP, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), AVATAR_COLUMN_WIDTH, 13);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), AVATAR_LOCKED_LABEL_TEXT);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(AVATAR_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_avatar_stack_init(EGUI_VIEW_OF(&stack_locked));
    egui_view_set_size(EGUI_VIEW_OF(&stack_locked), AVATAR_COLUMN_WIDTH, AVATAR_CARD_HEIGHT);
    egui_view_avatar_stack_set_snapshots(EGUI_VIEW_OF(&stack_locked), compact_snapshots, 2);
    egui_view_avatar_stack_set_current_snapshot(EGUI_VIEW_OF(&stack_locked), 0);
    egui_view_avatar_stack_set_focus_member(EGUI_VIEW_OF(&stack_locked), 1);
    egui_view_avatar_stack_set_show_header(EGUI_VIEW_OF(&stack_locked), 0);
    egui_view_avatar_stack_set_compact_mode(EGUI_VIEW_OF(&stack_locked), 1);
    egui_view_avatar_stack_set_palette(
            EGUI_VIEW_OF(&stack_locked),
            EGUI_COLOR_HEX(0x0E1522),
            EGUI_COLOR_HEX(0x46566B),
            EGUI_COLOR_HEX(0xCBD5E1),
            EGUI_COLOR_HEX(0x8695A9),
            EGUI_COLOR_HEX(0x758E83));
    egui_view_set_enable(EGUI_VIEW_OF(&stack_locked), 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&stack_locked));

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
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 400);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&stack_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&stack_compact), 700);
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 5:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&stack_locked), 700);
        return true;
    case 6:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
