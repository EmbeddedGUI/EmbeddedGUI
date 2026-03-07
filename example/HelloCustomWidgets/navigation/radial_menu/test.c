#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_radial_menu.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_radial_menu_t radial_primary;
static egui_view_label_t hint_label;
static egui_view_line_t variants_divider;
static egui_view_label_t variants_label;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_radial_menu_t radial_compact;
static egui_view_label_t compact_label;
static egui_view_linearlayout_t disabled_column;
static egui_view_radial_menu_t radial_disabled;
static egui_view_label_t disabled_label;

static const char *primary_items[] = {"Log", "Pause", "Mark", "Export", "Locate", "Layer"};
static const char *compact_items[] = {"Map", "Zoom", "Tag", "More"};
static const char *title_text = "Radial Menu";
static const char *primary_messages[] = {
        "Log selected",
        "Pause selected",
        "Mark selected",
        "Export selected",
        "Locate selected",
        "Layer selected",
};
static const char *compact_messages[] = {
        "Compact Map",
        "Compact Zoom",
        "Compact Tag",
        "Compact More",
};

static const egui_view_line_point_t divider_points[] = {{0, 0}, {135, 0}};

static void set_hint_state(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&hint_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&hint_label), color, EGUI_ALPHA_100);
}

static void on_primary_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    if (index < (sizeof(primary_messages) / sizeof(primary_messages[0])))
    {
        set_hint_state(primary_messages[index], EGUI_COLOR_HEX(0x38BDF8));
    }
}

static void on_compact_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    if (index < (sizeof(compact_messages) / sizeof(compact_messages[0])))
    {
        set_hint_state(compact_messages[index], EGUI_COLOR_ORANGE);
    }
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 220, 320);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 220, 20);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x38BDF8), EGUI_ALPHA_100);
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 220, 18);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 0, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 220, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), "Tap center and drag to choose");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_DARK_GREY, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_radial_menu_init(EGUI_VIEW_OF(&radial_primary));
    egui_view_set_size(EGUI_VIEW_OF(&radial_primary), 152, 152);
    egui_view_set_margin(EGUI_VIEW_OF(&radial_primary), 0, 0, 0, 2);
    egui_view_radial_menu_set_items(EGUI_VIEW_OF(&radial_primary), primary_items, 6);
    egui_view_radial_menu_set_current_index(EGUI_VIEW_OF(&radial_primary), 2);
    egui_view_radial_menu_set_on_selection_changed_listener(EGUI_VIEW_OF(&radial_primary), on_primary_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&radial_primary));

    egui_view_label_init(EGUI_VIEW_OF(&hint_label));
    egui_view_set_size(EGUI_VIEW_OF(&hint_label), 220, 16);
    egui_view_label_set_text(EGUI_VIEW_OF(&hint_label), primary_messages[2]);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&hint_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&hint_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&hint_label), EGUI_COLOR_HEX(0x38BDF8), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&hint_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&hint_label));

    egui_view_line_init(EGUI_VIEW_OF(&variants_divider));
    egui_view_set_size(EGUI_VIEW_OF(&variants_divider), 136, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&variants_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&variants_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&variants_divider), EGUI_COLOR_HEX(0x334155));
    egui_view_set_margin(EGUI_VIEW_OF(&variants_divider), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&variants_divider));

    egui_view_label_init(EGUI_VIEW_OF(&variants_label));
    egui_view_set_size(EGUI_VIEW_OF(&variants_label), 220, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&variants_label), "Compact and Disabled States");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&variants_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&variants_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&variants_label), EGUI_COLOR_DARK_GREY, EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&variants_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&variants_label));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), 220, 112);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), 102, 108);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_column), 0, 6, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_radial_menu_init(EGUI_VIEW_OF(&radial_compact));
    egui_view_set_size(EGUI_VIEW_OF(&radial_compact), 96, 96);
    egui_view_radial_menu_set_items(EGUI_VIEW_OF(&radial_compact), compact_items, 4);
    egui_view_radial_menu_set_current_index(EGUI_VIEW_OF(&radial_compact), 1);
    egui_view_radial_menu_set_palette(
            EGUI_VIEW_OF(&radial_compact),
            EGUI_COLOR_HEX(0x3F2A1D),
            EGUI_COLOR_HEX(0xF59E0B),
            EGUI_COLOR_HEX(0xD97706));
    egui_view_radial_menu_set_decoration_colors(
            EGUI_VIEW_OF(&radial_compact),
            EGUI_COLOR_HEX(0xFBBF24),
            EGUI_COLOR_HEX(0xFDE68A),
            EGUI_COLOR_HEX(0xFCD34D));
    egui_view_radial_menu_set_font(EGUI_VIEW_OF(&radial_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_radial_menu_set_on_selection_changed_listener(EGUI_VIEW_OF(&radial_compact), on_compact_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&radial_compact));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), 102, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0xFBBF24), EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&disabled_column));
    egui_view_set_size(EGUI_VIEW_OF(&disabled_column), 102, 108);
    egui_view_set_margin(EGUI_VIEW_OF(&disabled_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&disabled_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&disabled_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&disabled_column));

    egui_view_radial_menu_init(EGUI_VIEW_OF(&radial_disabled));
    egui_view_set_size(EGUI_VIEW_OF(&radial_disabled), 96, 96);
    egui_view_radial_menu_set_items(EGUI_VIEW_OF(&radial_disabled), primary_items, 6);
    egui_view_radial_menu_set_palette(
            EGUI_VIEW_OF(&radial_disabled),
            EGUI_COLOR_HEX(0x1F2937),
            EGUI_COLOR_HEX(0x9CA3AF),
            EGUI_COLOR_HEX(0x475569));
    egui_view_radial_menu_set_decoration_colors(
            EGUI_VIEW_OF(&radial_disabled),
            EGUI_COLOR_HEX(0x94A3B8),
            EGUI_COLOR_HEX(0xE2E8F0),
            EGUI_COLOR_HEX(0xCBD5E1));
    egui_view_radial_menu_set_font(EGUI_VIEW_OF(&radial_disabled), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_radial_menu_set_current_index(EGUI_VIEW_OF(&radial_disabled), 4);
    egui_view_set_enable(EGUI_VIEW_OF(&radial_disabled), 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&disabled_column), EGUI_VIEW_OF(&radial_disabled));

    egui_view_label_init(EGUI_VIEW_OF(&disabled_label));
    egui_view_set_size(EGUI_VIEW_OF(&disabled_label), 102, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&disabled_label), "Disabled");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&disabled_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&disabled_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&disabled_label), EGUI_COLOR_HEX(0xCBD5E1), EGUI_ALPHA_100);
    egui_view_group_add_child(EGUI_VIEW_OF(&disabled_column), EGUI_VIEW_OF(&disabled_label));

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&disabled_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
static void set_drag_action(egui_sim_action_t *p_action, egui_view_t *view, float rel_x, float rel_y, int steps, int interval_ms)
{
    int x1;
    int y1;
    int x2;
    int y2;
    egui_sim_get_view_center(view, &x1, &y1);
    egui_sim_get_view_pos(view, rel_x, rel_y, &x2, &y2);
    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = x1;
    p_action->y1 = y1;
    p_action->x2 = x2;
    p_action->y2 = y2;
    p_action->steps = steps;
    p_action->interval_ms = interval_ms;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 400);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&radial_primary), 700);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 250);
        return true;
    case 3:
        set_drag_action(p_action, EGUI_VIEW_OF(&radial_primary), 0.80f, 0.26f, 12, 500);
        return true;
    case 4:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&radial_primary), 700);
        return true;
    case 5:
        EGUI_SIM_SET_WAIT(p_action, 250);
        return true;
    case 6:
        set_drag_action(p_action, EGUI_VIEW_OF(&radial_primary), 0.08f, 0.08f, 10, 400);
        return true;
    case 7:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&radial_compact), 700);
        return true;
    case 8:
        EGUI_SIM_SET_WAIT(p_action, 250);
        return true;
    case 9:
        set_drag_action(p_action, EGUI_VIEW_OF(&radial_compact), 0.16f, 0.50f, 10, 400);
        return true;
    case 10:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&radial_disabled), 700);
        return true;
    case 11:
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
