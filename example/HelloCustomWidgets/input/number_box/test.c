#include <stdlib.h>

#include "egui.h"
#include "egui_view_number_box.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define NUMBER_BOX_PRIMARY_WIDTH        196
#define NUMBER_BOX_PRIMARY_HEIGHT       70
#define NUMBER_BOX_COMPACT_WIDTH        106
#define NUMBER_BOX_COMPACT_HEIGHT       44
#define NUMBER_BOX_BOTTOM_ROW_WIDTH     216
#define NUMBER_BOX_BOTTOM_ROW_HEIGHT    64
#define NUMBER_BOX_GUIDE_COLOR          0x6F7C8B
#define NUMBER_BOX_PRIMARY_LABEL_COLOR  0x768391
#define NUMBER_BOX_STATUS_COLOR         0x4D6880
#define NUMBER_BOX_COMPACT_LABEL_COLOR  0x0C7A6F
#define NUMBER_BOX_COMPACT_BORDER_COLOR 0xD2DDDA
#define NUMBER_BOX_COMPACT_ACCENT_COLOR 0x0C8178
#define NUMBER_BOX_LOCKED_LABEL_COLOR   0x8794A2
#define NUMBER_BOX_LOCKED_BORDER_COLOR  0xDBE2E8
#define NUMBER_BOX_LOCKED_ACCENT_COLOR  0x9AA7B5
#define NUMBER_BOX_STATUS_MUTED_MIX     28

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_number_box_t box_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_number_box_t box_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_number_box_t box_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Number Box";
static const char *guide_text = "Tap minus or plus to step";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {143, 0}};
static char status_text[32] = "Spacing 24 px";

static void set_status(const char *prefix, int16_t value, const char *suffix, egui_color_t color)
{
    int pos = 0;

    if (prefix != NULL)
    {
        pos += egui_sprintf_str(status_text, sizeof(status_text), prefix);
    }
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
    }
    pos += egui_sprintf_int(&status_text[pos], (int)sizeof(status_text) - pos, value);
    if (suffix != NULL && suffix[0] != '\0')
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, suffix);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x536677), NUMBER_BOX_STATUS_MUTED_MIX), EGUI_ALPHA_100);
}

static void on_primary_value_changed(egui_view_t *self, int16_t value)
{
    EGUI_UNUSED(self);
    set_status("Spacing", value, "px", EGUI_COLOR_HEX(0x2563EB));
}

static void on_compact_value_changed(egui_view_t *self, int16_t value)
{
    EGUI_UNUSED(self);
    set_status("Compact delay", value, "ms", EGUI_COLOR_HEX(0x0F766E));
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(NUMBER_BOX_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), 224, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(NUMBER_BOX_PRIMARY_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_number_box_init(EGUI_VIEW_OF(&box_primary));
    egui_view_set_size(EGUI_VIEW_OF(&box_primary), NUMBER_BOX_PRIMARY_WIDTH, NUMBER_BOX_PRIMARY_HEIGHT);
    egui_view_number_box_set_font(EGUI_VIEW_OF(&box_primary), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_number_box_set_meta_font(EGUI_VIEW_OF(&box_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_number_box_set_label(EGUI_VIEW_OF(&box_primary), "Spacing");
    egui_view_number_box_set_suffix(EGUI_VIEW_OF(&box_primary), "px");
    egui_view_number_box_set_helper(EGUI_VIEW_OF(&box_primary), "0 to 64, step 4");
    egui_view_number_box_set_range(EGUI_VIEW_OF(&box_primary), 0, 64);
    egui_view_number_box_set_step(EGUI_VIEW_OF(&box_primary), 4);
    egui_view_number_box_set_value(EGUI_VIEW_OF(&box_primary), 24);
    egui_view_number_box_set_on_value_changed_listener(EGUI_VIEW_OF(&box_primary), on_primary_value_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&box_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&box_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(NUMBER_BOX_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), NUMBER_BOX_BOTTOM_ROW_WIDTH, NUMBER_BOX_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), NUMBER_BOX_COMPACT_WIDTH, NUMBER_BOX_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), NUMBER_BOX_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(NUMBER_BOX_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_number_box_init(EGUI_VIEW_OF(&box_compact));
    egui_view_set_size(EGUI_VIEW_OF(&box_compact), NUMBER_BOX_COMPACT_WIDTH, NUMBER_BOX_COMPACT_HEIGHT);
    egui_view_number_box_set_font(EGUI_VIEW_OF(&box_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_number_box_set_meta_font(EGUI_VIEW_OF(&box_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_number_box_set_suffix(EGUI_VIEW_OF(&box_compact), "ms");
    egui_view_number_box_set_range(EGUI_VIEW_OF(&box_compact), 0, 24);
    egui_view_number_box_set_step(EGUI_VIEW_OF(&box_compact), 2);
    egui_view_number_box_set_value(EGUI_VIEW_OF(&box_compact), 12);
    egui_view_number_box_set_compact_mode(EGUI_VIEW_OF(&box_compact), 1);
    egui_view_number_box_set_palette(EGUI_VIEW_OF(&box_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(NUMBER_BOX_COMPACT_BORDER_COLOR),
                                     EGUI_COLOR_HEX(0x22323A), EGUI_COLOR_HEX(0x71818D), EGUI_COLOR_HEX(NUMBER_BOX_COMPACT_ACCENT_COLOR));
    egui_view_number_box_set_on_value_changed_listener(EGUI_VIEW_OF(&box_compact), on_compact_value_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&box_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), NUMBER_BOX_COMPACT_WIDTH, NUMBER_BOX_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), NUMBER_BOX_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(NUMBER_BOX_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_number_box_init(EGUI_VIEW_OF(&box_locked));
    egui_view_set_size(EGUI_VIEW_OF(&box_locked), NUMBER_BOX_COMPACT_WIDTH, NUMBER_BOX_COMPACT_HEIGHT);
    egui_view_number_box_set_font(EGUI_VIEW_OF(&box_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_number_box_set_meta_font(EGUI_VIEW_OF(&box_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_number_box_set_suffix(EGUI_VIEW_OF(&box_locked), "px");
    egui_view_number_box_set_value(EGUI_VIEW_OF(&box_locked), 16);
    egui_view_number_box_set_compact_mode(EGUI_VIEW_OF(&box_locked), 1);
    egui_view_number_box_set_locked_mode(EGUI_VIEW_OF(&box_locked), 1);
    egui_view_number_box_set_palette(EGUI_VIEW_OF(&box_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(NUMBER_BOX_LOCKED_BORDER_COLOR),
                                     EGUI_COLOR_HEX(0x566473), EGUI_COLOR_HEX(0x8C98A6), EGUI_COLOR_HEX(NUMBER_BOX_LOCKED_ACCENT_COLOR));
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&box_locked));

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
            egui_view_number_box_set_value(EGUI_VIEW_OF(&box_primary), 24);
            egui_view_number_box_set_value(EGUI_VIEW_OF(&box_compact), 12);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 1:
        if (first_call)
        {
            egui_view_number_box_set_value(EGUI_VIEW_OF(&box_primary), 28);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 2:
        if (first_call)
        {
            egui_view_number_box_set_value(EGUI_VIEW_OF(&box_primary), 32);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 3:
        if (first_call)
        {
            egui_view_number_box_set_value(EGUI_VIEW_OF(&box_primary), 28);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 4:
        if (first_call)
        {
            egui_view_number_box_set_value(EGUI_VIEW_OF(&box_compact), 14);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 640);
        return true;
    default:
        return false;
    }
}
#endif
