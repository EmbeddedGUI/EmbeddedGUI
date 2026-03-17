#include "egui.h"
#include "egui_view_date_picker.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define DATE_PICKER_ROOT_W                224
#define DATE_PICKER_ROOT_H                308
#define DATE_PICKER_PRIMARY_W             194
#define DATE_PICKER_PRIMARY_OPEN_H        180
#define DATE_PICKER_PRIMARY_CLOSED_H      82
#define DATE_PICKER_PREVIEW_W             106
#define DATE_PICKER_PREVIEW_H             48
#define DATE_PICKER_BOTTOM_W              218
#define DATE_PICKER_BOTTOM_H              60
#define DATE_PICKER_GUIDE_COLOR           0x768493
#define DATE_PICKER_LABEL_COLOR           0x74808D
#define DATE_PICKER_STATUS_COLOR          0x4C677E
#define DATE_PICKER_COMPACT_LABEL         0x0E776E
#define DATE_PICKER_READ_ONLY_LABEL       0x8994A0
#define DATE_PICKER_STATUS_MUTED_MIX      24
#define DATE_PICKER_PRIMARY_BOTTOM_MARGIN 3

typedef struct date_picker_demo_state date_picker_demo_state_t;
struct date_picker_demo_state
{
    uint16_t year;
    uint16_t panel_year;
    uint8_t month;
    uint8_t panel_month;
    uint8_t day;
    uint8_t opened;
    egui_color_t status_color;
    const char *status_prefix;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_date_picker_t picker_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_date_picker_t picker_compact;
static egui_view_linearlayout_t read_only_column;
static egui_view_label_t read_only_label;
static egui_view_date_picker_t picker_read_only;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Date Picker";
static const char *guide_text = "Tap field, chevrons browse month";
static const char *month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[72] = "Launch 2026-03-18 / Open";
static char primary_helper_text[64] = "Tap day, +/- browse";

static const date_picker_demo_state_t primary_states[] = {
        {2026, 2026, 3, 3, 18, 1, EGUI_COLOR_HEX(0x2563EB), "Launch"},      {2026, 2026, 3, 4, 18, 1, EGUI_COLOR_HEX(0x0E7490), "Browse"},
        {2026, 2026, 4, 4, 2, 1, EGUI_COLOR_HEX(0x1D6FD6), "Review"},       {2026, 2026, 11, 11, 27, 0, EGUI_COLOR_HEX(0xB77719), "Holiday"},
        {2026, 2027, 11, 1, 27, 1, EGUI_COLOR_HEX(0x7A56C2), "Cross Year"}, {2027, 2027, 1, 1, 5, 1, EGUI_COLOR_HEX(0x6D5BD0), "Kickoff"},
};

static const date_picker_demo_state_t compact_states[] = {
        {2026, 2026, 3, 3, 18, 0, EGUI_COLOR_HEX(0x0E776E), "Compact"},
        {2026, 2026, 6, 6, 9, 0, EGUI_COLOR_HEX(0x0E776E), "Compact"},
};

static const date_picker_demo_state_t read_only_states[] = {
        {2026, 2026, 4, 4, 5, 0, EGUI_COLOR_HEX(0x9AA7B5), "Read Only"},
        {2026, 2026, 4, 4, 18, 0, EGUI_COLOR_HEX(0x9AA7B5), "Read Only"},
};

static uint8_t primary_state_index = 0;
static uint8_t compact_state_index = 0;
static const char *primary_status_prefix = "Launch";
static egui_color_t primary_status_color = EGUI_COLOR_HEX(0x2563EB);
static uint8_t read_only_state_index = 0;
static uint8_t page_attached = 0;

static const date_picker_demo_state_t *find_primary_state(uint16_t year, uint8_t month, uint8_t day, uint16_t panel_year, uint8_t panel_month, uint8_t opened,
                                                          uint8_t *matched_index)
{
    uint8_t i;

    for (i = 0; i < (uint8_t)(sizeof(primary_states) / sizeof(primary_states[0])); ++i)
    {
        const date_picker_demo_state_t *state = &primary_states[i];

        if (state->year == year && state->month == month && state->day == day && state->panel_year == panel_year && state->panel_month == panel_month &&
            state->opened == opened)
        {
            if (matched_index != NULL)
            {
                *matched_index = i;
            }
            return state;
        }
    }

    return NULL;
}

static void build_date_text(uint16_t year, uint8_t month, uint8_t day, char *buffer, int size)
{
    if (size < 11)
    {
        return;
    }

    buffer[0] = (char)('0' + (year / 1000) % 10);
    buffer[1] = (char)('0' + (year / 100) % 10);
    buffer[2] = (char)('0' + (year / 10) % 10);
    buffer[3] = (char)('0' + year % 10);
    buffer[4] = '-';
    buffer[5] = (char)('0' + (month / 10) % 10);
    buffer[6] = (char)('0' + month % 10);
    buffer[7] = '-';
    buffer[8] = (char)('0' + (day / 10) % 10);
    buffer[9] = (char)('0' + day % 10);
    buffer[10] = '\0';
}

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month < 1 || month > 12)
    {
        return 30;
    }

    if (month == 2)
    {
        if ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)))
        {
            return 29;
        }
    }

    return days[month - 1];
}

static void set_status_from_picker(const char *prefix, egui_view_date_picker_t *picker, egui_color_t color)
{
    char date_buffer[16];
    int pos;
    uint8_t opened = egui_view_date_picker_get_opened(EGUI_VIEW_OF(picker));

    build_date_text(egui_view_date_picker_get_year(EGUI_VIEW_OF(picker)), egui_view_date_picker_get_month(EGUI_VIEW_OF(picker)),
                    egui_view_date_picker_get_day(EGUI_VIEW_OF(picker)), date_buffer, sizeof(date_buffer));

    status_text[0] = '\0';
    pos = egui_sprintf_str(status_text, sizeof(status_text), prefix);
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, date_buffer);
    }
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, opened ? " / Open" : " / Closed");
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x54697A), DATE_PICKER_STATUS_MUTED_MIX), EGUI_ALPHA_100);
}

static void sync_primary_status_profile(void)
{
    uint16_t year = egui_view_date_picker_get_year(EGUI_VIEW_OF(&picker_primary));
    uint8_t month = egui_view_date_picker_get_month(EGUI_VIEW_OF(&picker_primary));
    uint8_t day = egui_view_date_picker_get_day(EGUI_VIEW_OF(&picker_primary));
    uint16_t panel_year = egui_view_date_picker_get_display_year(EGUI_VIEW_OF(&picker_primary));
    uint8_t panel_month = egui_view_date_picker_get_display_month(EGUI_VIEW_OF(&picker_primary));
    uint8_t opened = egui_view_date_picker_get_opened(EGUI_VIEW_OF(&picker_primary));
    uint8_t matched_index;
    const date_picker_demo_state_t *matched_state = find_primary_state(year, month, day, panel_year, panel_month, opened, &matched_index);

    if (matched_state != NULL)
    {
        primary_state_index = matched_index;
        primary_status_prefix = matched_state->status_prefix;
        primary_status_color = matched_state->status_color;
        return;
    }

    if (opened && (panel_year != year || panel_month != month))
    {
        primary_status_prefix = "Browse";
        primary_status_color = EGUI_COLOR_HEX(0x0E7490);
        return;
    }

    if (opened)
    {
        primary_status_prefix = "Review";
        primary_status_color = EGUI_COLOR_HEX(0x1D6FD6);
        return;
    }

    primary_status_prefix = "Closed";
    primary_status_color = EGUI_COLOR_HEX(0x7B8793);
}

static void refresh_primary_helper(void)
{
    uint16_t year = egui_view_date_picker_get_year(EGUI_VIEW_OF(&picker_primary));
    uint8_t month = egui_view_date_picker_get_month(EGUI_VIEW_OF(&picker_primary));
    uint8_t day = egui_view_date_picker_get_day(EGUI_VIEW_OF(&picker_primary));
    uint16_t panel_year = egui_view_date_picker_get_display_year(EGUI_VIEW_OF(&picker_primary));
    uint8_t panel_month = egui_view_date_picker_get_display_month(EGUI_VIEW_OF(&picker_primary));
    uint8_t opened = egui_view_date_picker_get_opened(EGUI_VIEW_OF(&picker_primary));
    uint8_t anchor_day = day;
    uint8_t panel_max_day = days_in_month(panel_year, panel_month);
    int pos;

    if (anchor_day > panel_max_day)
    {
        anchor_day = panel_max_day;
    }

    if (!opened)
    {
        egui_view_date_picker_set_helper(EGUI_VIEW_OF(&picker_primary), "Tap field or Enter");
        return;
    }

    if (panel_year != year || panel_month != month)
    {
        primary_helper_text[0] = '\0';
        pos = egui_sprintf_str(primary_helper_text, sizeof(primary_helper_text), "Browse ");
        if (pos < (int)sizeof(primary_helper_text) - 1)
        {
            pos += egui_sprintf_str(&primary_helper_text[pos], (int)sizeof(primary_helper_text) - pos, month_names[panel_month - 1]);
        }
        if (panel_year != year && pos < (int)sizeof(primary_helper_text) - 1)
        {
            pos += egui_sprintf_char(&primary_helper_text[pos], (int)sizeof(primary_helper_text) - pos, ' ');
            pos += egui_sprintf_char(&primary_helper_text[pos], (int)sizeof(primary_helper_text) - pos, (char)('0' + (panel_year / 1000) % 10));
            pos += egui_sprintf_char(&primary_helper_text[pos], (int)sizeof(primary_helper_text) - pos, (char)('0' + (panel_year / 100) % 10));
            pos += egui_sprintf_char(&primary_helper_text[pos], (int)sizeof(primary_helper_text) - pos, (char)('0' + (panel_year / 10) % 10));
            pos += egui_sprintf_char(&primary_helper_text[pos], (int)sizeof(primary_helper_text) - pos, (char)('0' + panel_year % 10));
        }
        if (pos < (int)sizeof(primary_helper_text) - 1)
        {
            pos += egui_sprintf_str(&primary_helper_text[pos], (int)sizeof(primary_helper_text) - pos, ", Enter ");
            pos += egui_sprintf_char(&primary_helper_text[pos], (int)sizeof(primary_helper_text) - pos, (char)('0' + (anchor_day / 10) % 10));
            pos += egui_sprintf_char(&primary_helper_text[pos], (int)sizeof(primary_helper_text) - pos, (char)('0' + anchor_day % 10));
        }
        if (anchor_day < 10)
        {
            primary_helper_text[pos - 2] = primary_helper_text[pos - 1];
            primary_helper_text[pos - 1] = '\0';
        }
        egui_view_date_picker_set_helper(EGUI_VIEW_OF(&picker_primary), primary_helper_text);
        return;
    }

    egui_view_date_picker_set_helper(EGUI_VIEW_OF(&picker_primary), "Tap day, +/- browse");
}

static void refresh_primary_status(void)
{
    sync_primary_status_profile();
    refresh_primary_helper();
    set_status_from_picker(primary_status_prefix, &picker_primary, primary_status_color);
}

static void layout_local_page(void)
{
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&read_only_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));
}

static void refresh_page_layout(void)
{
    layout_local_page();
    if (!page_attached)
    {
        return;
    }

    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_invalidate(EGUI_VIEW_OF(&root_layout));
}

static void sync_primary_picker_layout(void)
{
    uint8_t opened = egui_view_date_picker_get_opened(EGUI_VIEW_OF(&picker_primary));
    egui_dim_t picker_h = opened ? DATE_PICKER_PRIMARY_OPEN_H : DATE_PICKER_PRIMARY_CLOSED_H;

    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), opened ? EGUI_ALIGN_HCENTER : (EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER));
    egui_view_set_size(EGUI_VIEW_OF(&picker_primary), DATE_PICKER_PRIMARY_W, picker_h);
    egui_view_set_margin(EGUI_VIEW_OF(&picker_primary), 0, 0, 0, DATE_PICKER_PRIMARY_BOTTOM_MARGIN);
    refresh_page_layout();
}

static void apply_primary_state(uint8_t index, uint8_t update_status)
{
    const date_picker_demo_state_t *state = &primary_states[index % (sizeof(primary_states) / sizeof(primary_states[0]))];

    primary_state_index = index % (sizeof(primary_states) / sizeof(primary_states[0]));
    egui_view_date_picker_set_date(EGUI_VIEW_OF(&picker_primary), state->year, state->month, state->day);
    egui_view_date_picker_set_display_month(EGUI_VIEW_OF(&picker_primary), state->panel_year, state->panel_month);
    egui_view_date_picker_set_opened(EGUI_VIEW_OF(&picker_primary), state->opened);
    sync_primary_picker_layout();
    if (update_status)
    {
        refresh_primary_status();
    }
}

static void apply_compact_state(uint8_t index)
{
    const date_picker_demo_state_t *state = &compact_states[index % (sizeof(compact_states) / sizeof(compact_states[0]))];

    compact_state_index = index % (sizeof(compact_states) / sizeof(compact_states[0]));
    egui_view_date_picker_set_date(EGUI_VIEW_OF(&picker_compact), state->year, state->month, state->day);
    egui_view_date_picker_set_display_month(EGUI_VIEW_OF(&picker_compact), state->panel_year, state->panel_month);
}

static void apply_read_only_state(uint8_t index)
{
    const date_picker_demo_state_t *state = &read_only_states[index % (sizeof(read_only_states) / sizeof(read_only_states[0]))];

    read_only_state_index = index % (sizeof(read_only_states) / sizeof(read_only_states[0]));
    egui_view_date_picker_set_date(EGUI_VIEW_OF(&picker_read_only), state->year, state->month, state->day);
    egui_view_date_picker_set_display_month(EGUI_VIEW_OF(&picker_read_only), state->panel_year, state->panel_month);
}

static void on_primary_date_changed(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(year);
    EGUI_UNUSED(month);
    EGUI_UNUSED(day);
    refresh_primary_status();
}

static void on_primary_display_month_changed(egui_view_t *self, uint16_t year, uint8_t month)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(year);
    EGUI_UNUSED(month);
    refresh_primary_status();
}

static void on_primary_open_changed(egui_view_t *self, uint8_t opened)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(opened);
    sync_primary_picker_layout();
    refresh_primary_status();
}

static void on_guide_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_primary_state((uint8_t)((primary_state_index + 1) % (sizeof(primary_states) / sizeof(primary_states[0]))), 1);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_compact_state((uint8_t)((compact_state_index + 1) % (sizeof(compact_states) / sizeof(compact_states[0]))));
}

static int consume_preview_touch(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(self);

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        egui_view_clear_focus(EGUI_VIEW_OF(&picker_primary));
    }
#endif
    return 1;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static uint8_t point_in_view_work_region(egui_view_t *view, egui_dim_t x, egui_dim_t y)
{
    egui_region_t region;

    egui_view_get_work_region(view, &region);
    return egui_region_pt_in_rect(&region, x, y) ? 1 : 0;
}
#endif

static int dismiss_primary_focus_on_down(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(self);

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        if (!point_in_view_work_region(EGUI_VIEW_OF(&picker_primary), event->location.x, event->location.y))
        {
            egui_view_clear_focus(EGUI_VIEW_OF(&picker_primary));
        }
    }
#endif
    return 1;
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), DATE_PICKER_ROOT_W, DATE_PICKER_ROOT_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));
    egui_view_set_on_touch_listener(EGUI_VIEW_OF(&root_layout), dismiss_primary_focus_on_down);

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), DATE_PICKER_ROOT_W, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), DATE_PICKER_ROOT_W, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(DATE_PICKER_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), DATE_PICKER_ROOT_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(DATE_PICKER_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_date_picker_init(EGUI_VIEW_OF(&picker_primary));
    egui_view_set_size(EGUI_VIEW_OF(&picker_primary), DATE_PICKER_PRIMARY_W, DATE_PICKER_PRIMARY_OPEN_H);
    egui_view_date_picker_set_font(EGUI_VIEW_OF(&picker_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_date_picker_set_meta_font(EGUI_VIEW_OF(&picker_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_date_picker_set_label(EGUI_VIEW_OF(&picker_primary), "Ship date");
    egui_view_date_picker_set_helper(EGUI_VIEW_OF(&picker_primary), "Mon first week, tap days");
    egui_view_date_picker_set_today(EGUI_VIEW_OF(&picker_primary), 2026, 3, 15);
    egui_view_date_picker_set_first_day_of_week(EGUI_VIEW_OF(&picker_primary), 1);
    egui_view_date_picker_set_on_date_changed_listener(EGUI_VIEW_OF(&picker_primary), on_primary_date_changed);
    egui_view_date_picker_set_on_open_changed_listener(EGUI_VIEW_OF(&picker_primary), on_primary_open_changed);
    egui_view_date_picker_set_on_display_month_changed_listener(EGUI_VIEW_OF(&picker_primary), on_primary_display_month_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&picker_primary), 0, 0, 0, DATE_PICKER_PRIMARY_BOTTOM_MARGIN);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&picker_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), DATE_PICKER_ROOT_W, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(DATE_PICKER_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), DATE_PICKER_BOTTOM_W, DATE_PICKER_BOTTOM_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), DATE_PICKER_PREVIEW_W, DATE_PICKER_BOTTOM_H);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), DATE_PICKER_PREVIEW_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(DATE_PICKER_COMPACT_LABEL), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 1);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_date_picker_init(EGUI_VIEW_OF(&picker_compact));
    egui_view_set_size(EGUI_VIEW_OF(&picker_compact), DATE_PICKER_PREVIEW_W, DATE_PICKER_PREVIEW_H);
    egui_view_date_picker_set_font(EGUI_VIEW_OF(&picker_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_date_picker_set_meta_font(EGUI_VIEW_OF(&picker_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_date_picker_set_compact_mode(EGUI_VIEW_OF(&picker_compact), 1);
    egui_view_date_picker_set_today(EGUI_VIEW_OF(&picker_compact), 2026, 3, 15);
    egui_view_date_picker_set_first_day_of_week(EGUI_VIEW_OF(&picker_compact), 1);
    egui_view_date_picker_set_palette(EGUI_VIEW_OF(&picker_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD2DDDA), EGUI_COLOR_HEX(0x17302A),
                                      EGUI_COLOR_HEX(0x57756C), EGUI_COLOR_HEX(0x0C8178), EGUI_COLOR_HEX(0x139A8F));
    egui_view_set_on_touch_listener(EGUI_VIEW_OF(&picker_compact), consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&picker_compact), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&picker_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&read_only_column));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_column), DATE_PICKER_PREVIEW_W, DATE_PICKER_BOTTOM_H);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&read_only_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&read_only_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&read_only_column));

    egui_view_label_init(EGUI_VIEW_OF(&read_only_label));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_label), DATE_PICKER_PREVIEW_W, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&read_only_label), "Read Only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&read_only_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&read_only_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&read_only_label), EGUI_COLOR_HEX(DATE_PICKER_READ_ONLY_LABEL), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_label), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&read_only_label));

    egui_view_date_picker_init(EGUI_VIEW_OF(&picker_read_only));
    egui_view_set_size(EGUI_VIEW_OF(&picker_read_only), DATE_PICKER_PREVIEW_W, DATE_PICKER_PREVIEW_H);
    egui_view_date_picker_set_font(EGUI_VIEW_OF(&picker_read_only), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_date_picker_set_meta_font(EGUI_VIEW_OF(&picker_read_only), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_date_picker_set_today(EGUI_VIEW_OF(&picker_read_only), 2026, 3, 15);
    egui_view_date_picker_set_first_day_of_week(EGUI_VIEW_OF(&picker_read_only), 1);
    egui_view_date_picker_set_compact_mode(EGUI_VIEW_OF(&picker_read_only), 1);
    egui_view_date_picker_set_read_only_mode(EGUI_VIEW_OF(&picker_read_only), 1);
    egui_view_date_picker_set_palette(EGUI_VIEW_OF(&picker_read_only), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xDBE2E8), EGUI_COLOR_HEX(0x536474),
                                      EGUI_COLOR_HEX(0x8896A4), EGUI_COLOR_HEX(0x9AA7B5), EGUI_COLOR_HEX(0x9AA7B5));
    egui_view_set_on_touch_listener(EGUI_VIEW_OF(&picker_read_only), consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&picker_read_only), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&picker_read_only));

    apply_primary_state(0, 1);
    apply_compact_state(0);
    apply_read_only_state(0);
    layout_local_page();

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    page_attached = 1;
    refresh_page_layout();
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
            apply_compact_state(0);
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
            apply_primary_state(4, 1);
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
            apply_primary_state(5, 1);
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 11:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 12:
        if (first_call)
        {
            apply_compact_state(1);
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 13:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 14:
        if (first_call)
        {
            apply_read_only_state((uint8_t)((read_only_state_index + 1) % (sizeof(read_only_states) / sizeof(read_only_states[0]))));
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 15:
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
