#include "egui.h"
#include "egui_view_parallax_view.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define PARALLAX_ROOT_WIDTH      224
#define PARALLAX_ROOT_HEIGHT     304
#define PARALLAX_PRIMARY_WIDTH   194
#define PARALLAX_PRIMARY_HEIGHT  136
#define PARALLAX_PREVIEW_WIDTH   106
#define PARALLAX_PREVIEW_HEIGHT  82
#define PARALLAX_BOTTOM_WIDTH    218
#define PARALLAX_BOTTOM_HEIGHT   90
#define PARALLAX_GUIDE_COLOR     0x778492
#define PARALLAX_LABEL_COLOR     0x74808D
#define PARALLAX_STATUS_COLOR    0x4C667D
#define PARALLAX_COMPACT_COLOR   0x0E776E
#define PARALLAX_READ_ONLY_COLOR 0x8994A0
#define PARALLAX_STATUS_MIX      26

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_parallax_view_t parallax_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_parallax_view_t parallax_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_parallax_view_t parallax_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Parallax View";
static const char *guide_text = "Tap rows or use +/- keys";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {146, 0}};
static char status_text[64] = "Active Hero Banner";

static const egui_view_parallax_view_row_t primary_rows[] = {
        {"Hero Banner", "Top", 0, EGUI_VIEW_PARALLAX_VIEW_TONE_ACCENT},
        {"Pinned Deck", "Mid", 180, EGUI_VIEW_PARALLAX_VIEW_TONE_SUCCESS},
        {"Quiet Layer", "Hold", 360, EGUI_VIEW_PARALLAX_VIEW_TONE_NEUTRAL},
        {"System Cards", "Tail", 560, EGUI_VIEW_PARALLAX_VIEW_TONE_WARNING},
};

static const egui_view_parallax_view_row_t compact_rows[] = {
        {"Front Rail", "Top", 0, EGUI_VIEW_PARALLAX_VIEW_TONE_ACCENT},
        {"Depth Strip", "Mid", 180, EGUI_VIEW_PARALLAX_VIEW_TONE_SUCCESS},
        {"Quiet Stack", "Tail", 360, EGUI_VIEW_PARALLAX_VIEW_TONE_NEUTRAL},
};

static const egui_view_parallax_view_row_t locked_rows[] = {
        {"Audit Layer", "Fixed", 0, EGUI_VIEW_PARALLAX_VIEW_TONE_NEUTRAL},
        {"Review Shelf", "Lock", 180, EGUI_VIEW_PARALLAX_VIEW_TONE_WARNING},
        {"Pinned Notes", "Still", 320, EGUI_VIEW_PARALLAX_VIEW_TONE_SUCCESS},
};

static egui_color_t parallax_status_color(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_PARALLAX_VIEW_TONE_SUCCESS:
        return EGUI_COLOR_HEX(0x178454);
    case EGUI_VIEW_PARALLAX_VIEW_TONE_WARNING:
        return EGUI_COLOR_HEX(0xA86F12);
    case EGUI_VIEW_PARALLAX_VIEW_TONE_NEUTRAL:
        return EGUI_COLOR_HEX(0x738291);
    default:
        return EGUI_COLOR_HEX(0x2563EB);
    }
}

static const egui_view_parallax_view_row_t *get_row(const egui_view_parallax_view_row_t *rows, uint8_t count, uint8_t index)
{
    if (rows == NULL || index >= count)
    {
        return NULL;
    }
    return &rows[index];
}

static void set_status(const char *prefix, const egui_view_parallax_view_row_t *row, egui_color_t color)
{
    int pos = 0;

    status_text[0] = '\0';
    if (prefix != NULL)
    {
        pos += egui_sprintf_str(status_text, sizeof(status_text), prefix);
    }
    if (row != NULL && row->title != NULL && row->title[0] != '\0' && pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, row->title);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x56687A), PARALLAX_STATUS_MIX), EGUI_ALPHA_100);
}

static void apply_primary_state(uint8_t index)
{
    const egui_view_parallax_view_row_t *row = get_row(primary_rows, 4, index);

    if (row == NULL)
    {
        return;
    }

    egui_view_parallax_view_set_offset(EGUI_VIEW_OF(&parallax_primary), row->anchor_offset);
    set_status("Active", row, parallax_status_color(row->tone));
}

static void apply_compact_state(uint8_t index, uint8_t update_status)
{
    const egui_view_parallax_view_row_t *row = get_row(compact_rows, 3, index);

    if (row == NULL)
    {
        return;
    }

    egui_view_parallax_view_set_offset(EGUI_VIEW_OF(&parallax_compact), row->anchor_offset);
    if (update_status)
    {
        set_status("Compact", row, EGUI_COLOR_HEX(0x0F766E));
    }
}

static void on_primary_changed(egui_view_t *self, egui_dim_t offset, uint8_t active_row)
{
    const egui_view_parallax_view_row_t *row = get_row(primary_rows, 4, active_row);

    EGUI_UNUSED(self);
    EGUI_UNUSED(offset);
    if (row != NULL)
    {
        set_status("Active", row, parallax_status_color(row->tone));
    }
}

static void on_guide_click(egui_view_t *self)
{
    uint8_t next = (uint8_t)((egui_view_parallax_view_get_active_row(EGUI_VIEW_OF(&parallax_primary)) + 1) % 4);

    EGUI_UNUSED(self);
    apply_primary_state(next);
}

static void on_compact_label_click(egui_view_t *self)
{
    uint8_t next = (uint8_t)((egui_view_parallax_view_get_active_row(EGUI_VIEW_OF(&parallax_compact)) + 1) % 3);

    EGUI_UNUSED(self);
    apply_compact_state(next, 1);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), PARALLAX_ROOT_WIDTH, PARALLAX_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), PARALLAX_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), PARALLAX_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(PARALLAX_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), PARALLAX_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard view");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(PARALLAX_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_parallax_view_init(EGUI_VIEW_OF(&parallax_primary));
    egui_view_set_size(EGUI_VIEW_OF(&parallax_primary), PARALLAX_PRIMARY_WIDTH, PARALLAX_PRIMARY_HEIGHT);
    egui_view_parallax_view_set_font(EGUI_VIEW_OF(&parallax_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_parallax_view_set_meta_font(EGUI_VIEW_OF(&parallax_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_parallax_view_set_header(EGUI_VIEW_OF(&parallax_primary), "Parallax deck", "Offset drives hero depth", "Active");
    egui_view_parallax_view_set_rows(EGUI_VIEW_OF(&parallax_primary), primary_rows, 4);
    egui_view_parallax_view_set_content_metrics(EGUI_VIEW_OF(&parallax_primary), 720, 160);
    egui_view_parallax_view_set_vertical_shift(EGUI_VIEW_OF(&parallax_primary), 18);
    egui_view_parallax_view_set_step_size(EGUI_VIEW_OF(&parallax_primary), 60, 180);
    egui_view_parallax_view_set_on_changed_listener(EGUI_VIEW_OF(&parallax_primary), on_primary_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&parallax_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&parallax_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), PARALLAX_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(PARALLAX_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 146, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDEE6ED));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), PARALLAX_BOTTOM_WIDTH, PARALLAX_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_HCENTER);

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), PARALLAX_PREVIEW_WIDTH, PARALLAX_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), PARALLAX_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact view");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(PARALLAX_COMPACT_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_parallax_view_init(EGUI_VIEW_OF(&parallax_compact));
    egui_view_set_size(EGUI_VIEW_OF(&parallax_compact), PARALLAX_PREVIEW_WIDTH, PARALLAX_PREVIEW_HEIGHT);
    egui_view_parallax_view_set_font(EGUI_VIEW_OF(&parallax_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_parallax_view_set_meta_font(EGUI_VIEW_OF(&parallax_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_parallax_view_set_header(EGUI_VIEW_OF(&parallax_compact), "Compact", "Short offset rail", "Peek");
    egui_view_parallax_view_set_rows(EGUI_VIEW_OF(&parallax_compact), compact_rows, 3);
    egui_view_parallax_view_set_content_metrics(EGUI_VIEW_OF(&parallax_compact), 520, 160);
    egui_view_parallax_view_set_vertical_shift(EGUI_VIEW_OF(&parallax_compact), 10);
    egui_view_parallax_view_set_step_size(EGUI_VIEW_OF(&parallax_compact), 60, 180);
    egui_view_parallax_view_set_compact_mode(EGUI_VIEW_OF(&parallax_compact), 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&parallax_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), PARALLAX_PREVIEW_WIDTH, PARALLAX_BOTTOM_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), PARALLAX_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read-only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(PARALLAX_READ_ONLY_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_parallax_view_init(EGUI_VIEW_OF(&parallax_locked));
    egui_view_set_size(EGUI_VIEW_OF(&parallax_locked), PARALLAX_PREVIEW_WIDTH, PARALLAX_PREVIEW_HEIGHT);
    egui_view_parallax_view_set_font(EGUI_VIEW_OF(&parallax_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_parallax_view_set_meta_font(EGUI_VIEW_OF(&parallax_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_parallax_view_set_header(EGUI_VIEW_OF(&parallax_locked), "Read only", "Locked hero depth", "Fixed");
    egui_view_parallax_view_set_rows(EGUI_VIEW_OF(&parallax_locked), locked_rows, 3);
    egui_view_parallax_view_set_content_metrics(EGUI_VIEW_OF(&parallax_locked), 460, 160);
    egui_view_parallax_view_set_vertical_shift(EGUI_VIEW_OF(&parallax_locked), 8);
    egui_view_parallax_view_set_step_size(EGUI_VIEW_OF(&parallax_locked), 60, 180);
    egui_view_parallax_view_set_compact_mode(EGUI_VIEW_OF(&parallax_locked), 1);
    egui_view_parallax_view_set_locked_mode(EGUI_VIEW_OF(&parallax_locked), 1);
    egui_view_parallax_view_set_offset(EGUI_VIEW_OF(&parallax_locked), locked_rows[1].anchor_offset);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&parallax_locked));

    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    apply_primary_state(0);
    apply_compact_state(1, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&locked_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_invalidate(EGUI_VIEW_OF(&root_layout));
#if EGUI_CONFIG_RECORDING_TEST
    recording_request_snapshot();
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&parallax_primary));
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
            apply_primary_state(0);
            apply_compact_state(1, 0);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 1:
        if (first_call)
        {
            apply_primary_state(1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_state(2);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 3:
        if (first_call)
        {
            apply_compact_state(2, 1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_state(3);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 520);
        return true;
    default:
        return false;
    }
}
#endif
