#include "egui.h"
#include "egui_view_time_picker.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define TIME_PICKER_ROOT_W           224
#define TIME_PICKER_ROOT_H           296
#define TIME_PICKER_PRIMARY_W        194
#define TIME_PICKER_PRIMARY_H        126
#define TIME_PICKER_PREVIEW_W        106
#define TIME_PICKER_PREVIEW_H        58
#define TIME_PICKER_BOTTOM_W         218
#define TIME_PICKER_BOTTOM_H         80
#define TIME_PICKER_GUIDE_COLOR      0x768493
#define TIME_PICKER_LABEL_COLOR      0x74808D
#define TIME_PICKER_STATUS_COLOR     0x4C677E
#define TIME_PICKER_COMPACT_LABEL    0x0E776E
#define TIME_PICKER_READ_ONLY_LABEL  0x8994A0
#define TIME_PICKER_STATUS_MUTED_MIX 24

typedef struct time_picker_demo_state time_picker_demo_state_t;
struct time_picker_demo_state
{
    uint8_t hour24;
    uint8_t minute;
    uint8_t opened;
    uint8_t focused_segment;
    egui_color_t status_color;
    const char *status_prefix;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_time_picker_t picker_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_time_picker_t picker_compact;
static egui_view_linearlayout_t read_only_column;
static egui_view_label_t read_only_label;
static egui_view_time_picker_t picker_read_only;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Time Picker";
static const char *guide_text = "Tap field, arrows switch time";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[64] = "Standup 08:30 AM / Open";

static const time_picker_demo_state_t primary_states[] = {
        {8, 30, 1, EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE, EGUI_COLOR_HEX(0x2563EB), "Standup"},
        {10, 45, 1, EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR, EGUI_COLOR_HEX(0x0E7490), "Review"},
        {18, 0, 0, EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD, EGUI_COLOR_HEX(0xB77719), "Quiet hours"},
        {21, 15, 1, EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD, EGUI_COLOR_HEX(0x6D5BD0), "Night sync"},
};

static const time_picker_demo_state_t compact_states[] = {
        {13, 30, 0, EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR, EGUI_COLOR_HEX(0x0E776E), "Compact"},
        {18, 0, 0, EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE, EGUI_COLOR_HEX(0x0E776E), "Compact"},
};

static uint8_t primary_state_index = 0;
static uint8_t compact_state_index = 0;

static void build_time_text(uint8_t hour24, uint8_t minute, uint8_t use_24h, char *buffer, int size)
{
    int pos = 0;
    uint8_t display_hour = hour24;

    if (size <= 0)
    {
        return;
    }

    if (!use_24h)
    {
        display_hour = hour24 % 12;
        if (display_hour == 0)
        {
            display_hour = 12;
        }
    }

    buffer[0] = '\0';
    pos += egui_sprintf_char(&buffer[pos], size - pos, (char)('0' + (display_hour / 10) % 10));
    pos += egui_sprintf_char(&buffer[pos], size - pos, (char)('0' + display_hour % 10));
    pos += egui_sprintf_char(&buffer[pos], size - pos, ':');
    pos += egui_sprintf_char(&buffer[pos], size - pos, (char)('0' + (minute / 10) % 10));
    pos += egui_sprintf_char(&buffer[pos], size - pos, (char)('0' + minute % 10));
    if (!use_24h)
    {
        pos += egui_sprintf_str(&buffer[pos], size - pos, hour24 >= 12 ? " PM" : " AM");
    }
}

static void set_status_from_picker(const char *prefix, egui_view_time_picker_t *picker, egui_color_t color)
{
    char time_buffer[20];
    int pos;
    uint8_t use_24h = egui_view_time_picker_get_use_24h(EGUI_VIEW_OF(picker));
    uint8_t opened = egui_view_time_picker_get_opened(EGUI_VIEW_OF(picker));

    build_time_text(egui_view_time_picker_get_hour24(EGUI_VIEW_OF(picker)), egui_view_time_picker_get_minute(EGUI_VIEW_OF(picker)), use_24h, time_buffer,
                    sizeof(time_buffer));

    status_text[0] = '\0';
    pos = egui_sprintf_str(status_text, sizeof(status_text), prefix);
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, time_buffer);
    }
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, opened ? " / Open" : " / Closed");
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x54697A), TIME_PICKER_STATUS_MUTED_MIX), EGUI_ALPHA_100);
}

static void apply_primary_state(uint8_t index, uint8_t update_status)
{
    const time_picker_demo_state_t *state = &primary_states[index % (sizeof(primary_states) / sizeof(primary_states[0]))];

    primary_state_index = index % (sizeof(primary_states) / sizeof(primary_states[0]));
    egui_view_time_picker_set_time(EGUI_VIEW_OF(&picker_primary), state->hour24, state->minute);
    egui_view_time_picker_set_focused_segment(EGUI_VIEW_OF(&picker_primary), state->focused_segment);
    egui_view_time_picker_set_opened(EGUI_VIEW_OF(&picker_primary), state->opened);
    if (update_status)
    {
        set_status_from_picker(state->status_prefix, &picker_primary, state->status_color);
    }
}

static void apply_compact_state(uint8_t index, uint8_t update_status)
{
    const time_picker_demo_state_t *state = &compact_states[index % (sizeof(compact_states) / sizeof(compact_states[0]))];

    compact_state_index = index % (sizeof(compact_states) / sizeof(compact_states[0]));
    egui_view_time_picker_set_time(EGUI_VIEW_OF(&picker_compact), state->hour24, state->minute);
    egui_view_time_picker_set_focused_segment(EGUI_VIEW_OF(&picker_compact), state->focused_segment);
    if (update_status)
    {
        set_status_from_picker(state->status_prefix, &picker_compact, state->status_color);
    }
}

static void on_primary_time_changed(egui_view_t *self, uint8_t hour24, uint8_t minute)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(hour24);
    EGUI_UNUSED(minute);
    set_status_from_picker("Start", &picker_primary, EGUI_COLOR_HEX(0x2563EB));
}

static void on_primary_open_changed(egui_view_t *self, uint8_t opened)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(opened);
    set_status_from_picker("Start", &picker_primary, EGUI_COLOR_HEX(0x2563EB));
}

static void on_compact_time_changed(egui_view_t *self, uint8_t hour24, uint8_t minute)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(hour24);
    EGUI_UNUSED(minute);
    set_status_from_picker("Compact", &picker_compact, EGUI_COLOR_HEX(0x0E776E));
}

static void on_guide_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_primary_state((uint8_t)((primary_state_index + 1) % (sizeof(primary_states) / sizeof(primary_states[0]))), 1);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_compact_state((uint8_t)((compact_state_index + 1) % (sizeof(compact_states) / sizeof(compact_states[0]))), 1);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), TIME_PICKER_ROOT_W, TIME_PICKER_ROOT_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), TIME_PICKER_ROOT_W, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 9, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), TIME_PICKER_ROOT_W, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(TIME_PICKER_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), TIME_PICKER_ROOT_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(TIME_PICKER_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_time_picker_init(EGUI_VIEW_OF(&picker_primary));
    egui_view_set_size(EGUI_VIEW_OF(&picker_primary), TIME_PICKER_PRIMARY_W, TIME_PICKER_PRIMARY_H);
    egui_view_time_picker_set_font(EGUI_VIEW_OF(&picker_primary), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_time_picker_set_meta_font(EGUI_VIEW_OF(&picker_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_time_picker_set_label(EGUI_VIEW_OF(&picker_primary), "Start time");
    egui_view_time_picker_set_helper(EGUI_VIEW_OF(&picker_primary), "15 minute intervals, AM/PM");
    egui_view_time_picker_set_minute_step(EGUI_VIEW_OF(&picker_primary), 15);
    egui_view_time_picker_set_on_time_changed_listener(EGUI_VIEW_OF(&picker_primary), on_primary_time_changed);
    egui_view_time_picker_set_on_open_changed_listener(EGUI_VIEW_OF(&picker_primary), on_primary_open_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&picker_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&picker_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), TIME_PICKER_ROOT_W, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(TIME_PICKER_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 148, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDEE4EB));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), TIME_PICKER_BOTTOM_W, TIME_PICKER_BOTTOM_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), TIME_PICKER_PREVIEW_W, TIME_PICKER_BOTTOM_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), TIME_PICKER_PREVIEW_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(TIME_PICKER_COMPACT_LABEL), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_time_picker_init(EGUI_VIEW_OF(&picker_compact));
    egui_view_set_size(EGUI_VIEW_OF(&picker_compact), TIME_PICKER_PREVIEW_W, TIME_PICKER_PREVIEW_H);
    egui_view_time_picker_set_font(EGUI_VIEW_OF(&picker_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_time_picker_set_meta_font(EGUI_VIEW_OF(&picker_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_time_picker_set_use_24h(EGUI_VIEW_OF(&picker_compact), 1);
    egui_view_time_picker_set_minute_step(EGUI_VIEW_OF(&picker_compact), 30);
    egui_view_time_picker_set_compact_mode(EGUI_VIEW_OF(&picker_compact), 1);
    egui_view_time_picker_set_palette(EGUI_VIEW_OF(&picker_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD2DDDA), EGUI_COLOR_HEX(0x17302A),
                                      EGUI_COLOR_HEX(0x57756C), EGUI_COLOR_HEX(0x0C8178));
    egui_view_time_picker_set_on_time_changed_listener(EGUI_VIEW_OF(&picker_compact), on_compact_time_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&picker_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&read_only_column));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_column), TIME_PICKER_PREVIEW_W, TIME_PICKER_BOTTOM_H);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&read_only_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&read_only_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&read_only_column));

    egui_view_label_init(EGUI_VIEW_OF(&read_only_label));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_label), TIME_PICKER_PREVIEW_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&read_only_label), "Read Only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&read_only_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&read_only_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&read_only_label), EGUI_COLOR_HEX(TIME_PICKER_READ_ONLY_LABEL), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&read_only_label));

    egui_view_time_picker_init(EGUI_VIEW_OF(&picker_read_only));
    egui_view_set_size(EGUI_VIEW_OF(&picker_read_only), TIME_PICKER_PREVIEW_W, TIME_PICKER_PREVIEW_H);
    egui_view_time_picker_set_font(EGUI_VIEW_OF(&picker_read_only), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_time_picker_set_meta_font(EGUI_VIEW_OF(&picker_read_only), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_time_picker_set_time(EGUI_VIEW_OF(&picker_read_only), 7, 15);
    egui_view_time_picker_set_read_only_mode(EGUI_VIEW_OF(&picker_read_only), 1);
    egui_view_time_picker_set_palette(EGUI_VIEW_OF(&picker_read_only), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xDBE2E8), EGUI_COLOR_HEX(0x536474),
                                      EGUI_COLOR_HEX(0x8896A4), EGUI_COLOR_HEX(0x9AA7B5));
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&picker_read_only));

    apply_primary_state(0, 1);
    apply_compact_state(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&read_only_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_invalidate(EGUI_VIEW_OF(&root_layout));
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&picker_primary));
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
            apply_primary_state(0, 1);
            apply_compact_state(0, 0);
            set_status_from_picker("Standup", &picker_primary, EGUI_COLOR_HEX(0x2563EB));
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 1:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_state(1, 1);
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 3:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_state(2, 1);
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 5:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 6:
        if (first_call)
        {
            apply_primary_state(3, 1);
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 7:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 8:
        if (first_call)
        {
            apply_compact_state(1, 1);
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 9:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 10:
        if (first_call)
        {
            set_status_from_picker("Locked", &picker_read_only, EGUI_COLOR_HEX(0x8191A0));
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 11:
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
