#include <string.h>

#include "egui.h"
#include "egui_view_calendar_view.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define CALENDAR_VIEW_ROOT_WIDTH          224
#define CALENDAR_VIEW_ROOT_HEIGHT         292
#define CALENDAR_VIEW_PRIMARY_WIDTH       196
#define CALENDAR_VIEW_PRIMARY_HEIGHT      144
#define CALENDAR_VIEW_PREVIEW_WIDTH       104
#define CALENDAR_VIEW_PREVIEW_HEIGHT      50
#define CALENDAR_VIEW_BOTTOM_ROW_WIDTH    216
#define CALENDAR_VIEW_BOTTOM_ROW_HEIGHT   66
#define CALENDAR_VIEW_GUIDE_COLOR         0x72808E
#define CALENDAR_VIEW_LABEL_COLOR         0x74808D
#define CALENDAR_VIEW_STATUS_COLOR        0x4C667C
#define CALENDAR_VIEW_COMPACT_LABEL_COLOR 0x0F776E
#define CALENDAR_VIEW_LOCKED_LABEL_COLOR  0x8995A2
#define CALENDAR_VIEW_PRIMARY_ACCENT      0x2563EB
#define CALENDAR_VIEW_COMPACT_ACCENT      0x0D9488
#define CALENDAR_VIEW_LOCKED_ACCENT       0x9AA7B5
#define CALENDAR_VIEW_RECORD_WAIT         110
#define CALENDAR_VIEW_RECORD_FRAME_WAIT   150

typedef struct calendar_view_snapshot calendar_view_snapshot_t;
struct calendar_view_snapshot
{
    const char *label;
    const char *helper;
    const char *status_prefix;
    egui_color_t status_color;
    uint16_t year;
    uint8_t month;
    uint8_t start_day;
    uint8_t end_day;
    uint16_t display_year;
    uint8_t display_month;
    uint16_t today_year;
    uint8_t today_month;
    uint8_t today_day;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_calendar_view_t calendar_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_calendar_view_t calendar_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_calendar_view_t calendar_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Calendar View";
static const char *guide_text = "Tap guide to cycle presets";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[72] = "Sprint Mar 09-13";
static uint8_t primary_snapshot_index = 0;
static uint8_t compact_snapshot_index = 0;

static const calendar_view_snapshot_t primary_snapshots[] = {
        {"Sprint planning", "Enter anchors a range, arrows extend it", "Sprint", EGUI_COLOR_HEX(CALENDAR_VIEW_PRIMARY_ACCENT), 2026, 3, 9, 13, 2026, 3, 2026, 3,
         15},
        {"Travel window", "Browse first, then commit the range", "Travel", EGUI_COLOR_HEX(0xD97706), 2026, 4, 21, 24, 2026, 4, 2026, 4, 22},
        {"Freeze week", "Read only stays muted for audits", "Freeze", EGUI_COLOR_HEX(0x0F766E), 2026, 11, 3, 7, 2026, 11, 2026, 11, 5},
};

static const calendar_view_snapshot_t compact_snapshots[] = {
        {NULL, NULL, "Compact", EGUI_COLOR_HEX(CALENDAR_VIEW_COMPACT_ACCENT), 2026, 5, 5, 8, 2026, 5, 2026, 5, 6},
        {NULL, NULL, "Compact", EGUI_COLOR_HEX(CALENDAR_VIEW_COMPACT_ACCENT), 2026, 6, 12, 12, 2026, 6, 2026, 6, 12},
};

static const calendar_view_snapshot_t locked_snapshot = {
        NULL, NULL, "Locked", EGUI_COLOR_HEX(CALENDAR_VIEW_LOCKED_ACCENT), 2026, 7, 18, 22, 2026, 7, 2026, 7, 20,
};

static void format_month_title(uint16_t year, uint8_t month, char *buffer)
{
    static const char *month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    buffer[0] = month_names[month - 1][0];
    buffer[1] = month_names[month - 1][1];
    buffer[2] = month_names[month - 1][2];
    buffer[3] = ' ';
    buffer[4] = (char)('0' + (year / 1000) % 10);
    buffer[5] = (char)('0' + (year / 100) % 10);
    buffer[6] = (char)('0' + (year / 10) % 10);
    buffer[7] = (char)('0' + year % 10);
    buffer[8] = '\0';
}

static void format_range(uint16_t year, uint8_t month, uint8_t start_day, uint8_t end_day, char *buffer)
{
    int pos = 0;
    char month_title[16];

    format_month_title(year, month, month_title);
    pos += egui_sprintf_str(buffer, 24, month_title);
    pos += egui_sprintf_char(&buffer[pos], 24 - pos, ' ');
    pos += egui_sprintf_int_pad(&buffer[pos], 24 - pos, start_day, 2, '0');
    if (start_day != end_day)
    {
        pos += egui_sprintf_char(&buffer[pos], 24 - pos, '-');
        egui_sprintf_int_pad(&buffer[pos], 24 - pos, end_day, 2, '0');
    }
}

static void refresh_status(void)
{
    const calendar_view_snapshot_t *snapshot = &primary_snapshots[primary_snapshot_index];
    uint16_t selection_year = egui_view_calendar_view_get_selection_year(EGUI_VIEW_OF(&calendar_primary));
    uint8_t selection_month = egui_view_calendar_view_get_selection_month(EGUI_VIEW_OF(&calendar_primary));
    uint8_t start_day = egui_view_calendar_view_get_start_day(EGUI_VIEW_OF(&calendar_primary));
    uint8_t end_day = egui_view_calendar_view_get_end_day(EGUI_VIEW_OF(&calendar_primary));
    uint16_t display_year = egui_view_calendar_view_get_display_year(EGUI_VIEW_OF(&calendar_primary));
    uint8_t display_month = egui_view_calendar_view_get_display_month(EGUI_VIEW_OF(&calendar_primary));
    uint8_t focus_day = egui_view_calendar_view_get_focus_day(EGUI_VIEW_OF(&calendar_primary));
    uint8_t editing = egui_view_calendar_view_get_editing_range(EGUI_VIEW_OF(&calendar_primary));
    char range_text[24];
    char month_text[16];
    int pos = 0;

    status_text[0] = '\0';
    pos += egui_sprintf_str(status_text, sizeof(status_text), snapshot->status_prefix);
    pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');

    if (display_year != selection_year || display_month != selection_month)
    {
        format_month_title(display_year, display_month, month_text);
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, "Browse ");
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, month_text);
    }
    else if (editing)
    {
        format_range(selection_year, selection_month, start_day, end_day, range_text);
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, "Anchor ");
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, range_text);
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " / ");
        egui_sprintf_int_pad(&status_text[pos], (int)sizeof(status_text) - pos, focus_day, 2, '0');
    }
    else
    {
        format_range(selection_year, selection_month, start_day, end_day, range_text);
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, range_text);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(snapshot->status_color, EGUI_COLOR_HEX(CALENDAR_VIEW_STATUS_COLOR), 26),
                                   EGUI_ALPHA_100);
}

static void apply_snapshot(egui_view_t *view, const calendar_view_snapshot_t *snapshot)
{
    if (snapshot->label != NULL)
    {
        egui_view_calendar_view_set_label(view, snapshot->label);
    }
    if (snapshot->helper != NULL)
    {
        egui_view_calendar_view_set_helper(view, snapshot->helper);
    }
    egui_view_calendar_view_set_today(view, snapshot->today_year, snapshot->today_month, snapshot->today_day);
    egui_view_calendar_view_set_display_month(view, snapshot->display_year, snapshot->display_month);
    egui_view_calendar_view_set_range(view, snapshot->year, snapshot->month, snapshot->start_day, snapshot->end_day);
    egui_view_calendar_view_set_current_part(view, EGUI_VIEW_CALENDAR_VIEW_PART_GRID);
}

static void apply_primary_snapshot(uint8_t index, uint8_t update_status)
{
    const calendar_view_snapshot_t *snapshot = &primary_snapshots[index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]))];

    primary_snapshot_index = index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]));
    apply_snapshot(EGUI_VIEW_OF(&calendar_primary), snapshot);
    if (update_status)
    {
        refresh_status();
    }
}

static void apply_compact_snapshot(uint8_t index)
{
    const calendar_view_snapshot_t *snapshot = &compact_snapshots[index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]))];

    compact_snapshot_index = index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]));
    apply_snapshot(EGUI_VIEW_OF(&calendar_compact), snapshot);
}

static void on_primary_changed(egui_view_t *self, uint16_t year, uint8_t month, uint8_t start_day, uint8_t end_day, uint8_t focus_day, uint8_t editing_range)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(year);
    EGUI_UNUSED(month);
    EGUI_UNUSED(start_day);
    EGUI_UNUSED(end_day);
    EGUI_UNUSED(focus_day);
    EGUI_UNUSED(editing_range);
    refresh_status();
}

static void on_primary_display_month_changed(egui_view_t *self, uint16_t year, uint8_t month)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(year);
    EGUI_UNUSED(month);
    refresh_status();
}

static void on_guide_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_primary_snapshot((uint8_t)(primary_snapshot_index + 1), 1);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_compact_snapshot((uint8_t)(compact_snapshot_index + 1));
}

static int consume_preview_touch(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(event);
    return 1;
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), CALENDAR_VIEW_ROOT_WIDTH, CALENDAR_VIEW_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), CALENDAR_VIEW_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), CALENDAR_VIEW_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(CALENDAR_VIEW_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), CALENDAR_VIEW_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(CALENDAR_VIEW_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_calendar_view_init(EGUI_VIEW_OF(&calendar_primary));
    egui_view_set_size(EGUI_VIEW_OF(&calendar_primary), CALENDAR_VIEW_PRIMARY_WIDTH, CALENDAR_VIEW_PRIMARY_HEIGHT);
    egui_view_calendar_view_set_font(EGUI_VIEW_OF(&calendar_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_calendar_view_set_meta_font(EGUI_VIEW_OF(&calendar_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_calendar_view_set_palette(EGUI_VIEW_OF(&calendar_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD7DFE7), EGUI_COLOR_HEX(0x1A2630),
                                        EGUI_COLOR_HEX(0x72808E), EGUI_COLOR_HEX(CALENDAR_VIEW_PRIMARY_ACCENT), EGUI_COLOR_HEX(0x0F766E));
    egui_view_calendar_view_set_on_changed_listener(EGUI_VIEW_OF(&calendar_primary), on_primary_changed);
    egui_view_calendar_view_set_on_display_month_changed_listener(EGUI_VIEW_OF(&calendar_primary), on_primary_display_month_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&calendar_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&calendar_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), CALENDAR_VIEW_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(CALENDAR_VIEW_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), CALENDAR_VIEW_BOTTOM_ROW_WIDTH, CALENDAR_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), CALENDAR_VIEW_PREVIEW_WIDTH, CALENDAR_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), CALENDAR_VIEW_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(CALENDAR_VIEW_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_calendar_view_init(EGUI_VIEW_OF(&calendar_compact));
    egui_view_set_size(EGUI_VIEW_OF(&calendar_compact), CALENDAR_VIEW_PREVIEW_WIDTH, CALENDAR_VIEW_PREVIEW_HEIGHT);
    egui_view_calendar_view_set_font(EGUI_VIEW_OF(&calendar_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_calendar_view_set_meta_font(EGUI_VIEW_OF(&calendar_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_calendar_view_set_compact_mode(EGUI_VIEW_OF(&calendar_compact), 1);
    egui_view_calendar_view_set_palette(EGUI_VIEW_OF(&calendar_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD2DDDA), EGUI_COLOR_HEX(0x17302A),
                                        EGUI_COLOR_HEX(0x57756C), EGUI_COLOR_HEX(CALENDAR_VIEW_COMPACT_ACCENT), EGUI_COLOR_HEX(0x0F766E));
    egui_view_set_on_touch_listener(EGUI_VIEW_OF(&calendar_compact), consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&calendar_compact), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&calendar_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), CALENDAR_VIEW_PREVIEW_WIDTH, CALENDAR_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 8, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), CALENDAR_VIEW_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(CALENDAR_VIEW_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_calendar_view_init(EGUI_VIEW_OF(&calendar_locked));
    egui_view_set_size(EGUI_VIEW_OF(&calendar_locked), CALENDAR_VIEW_PREVIEW_WIDTH, CALENDAR_VIEW_PREVIEW_HEIGHT);
    egui_view_calendar_view_set_font(EGUI_VIEW_OF(&calendar_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_calendar_view_set_meta_font(EGUI_VIEW_OF(&calendar_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_calendar_view_set_compact_mode(EGUI_VIEW_OF(&calendar_locked), 1);
    egui_view_calendar_view_set_read_only_mode(EGUI_VIEW_OF(&calendar_locked), 1);
    egui_view_calendar_view_set_palette(EGUI_VIEW_OF(&calendar_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xDBE2E8), EGUI_COLOR_HEX(0x536474),
                                        EGUI_COLOR_HEX(0x8896A4), EGUI_COLOR_HEX(CALENDAR_VIEW_LOCKED_ACCENT), EGUI_COLOR_HEX(0x8AA3B5));
    egui_view_set_on_touch_listener(EGUI_VIEW_OF(&calendar_locked), consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&calendar_locked), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&calendar_locked));

    apply_primary_snapshot(0, 1);
    apply_compact_snapshot(0);
    apply_snapshot(EGUI_VIEW_OF(&calendar_locked), &locked_snapshot);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&locked_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
static void apply_primary_key(uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    EGUI_VIEW_OF(&calendar_primary)->api->on_key_event(EGUI_VIEW_OF(&calendar_primary), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    EGUI_VIEW_OF(&calendar_primary)->api->on_key_event(EGUI_VIEW_OF(&calendar_primary), &event);
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
            apply_primary_snapshot(0, 1);
            apply_compact_snapshot(0);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, CALENDAR_VIEW_RECORD_FRAME_WAIT);
        return true;
    case 1:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_ENTER);
        }
        EGUI_SIM_SET_WAIT(p_action, CALENDAR_VIEW_RECORD_WAIT);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_RIGHT);
            apply_primary_key(EGUI_KEY_CODE_RIGHT);
        }
        EGUI_SIM_SET_WAIT(p_action, CALENDAR_VIEW_RECORD_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, CALENDAR_VIEW_RECORD_FRAME_WAIT);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_ENTER);
        }
        EGUI_SIM_SET_WAIT(p_action, CALENDAR_VIEW_RECORD_WAIT);
        return true;
    case 5:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_PLUS);
        }
        EGUI_SIM_SET_WAIT(p_action, CALENDAR_VIEW_RECORD_WAIT);
        return true;
    case 6:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, CALENDAR_VIEW_RECORD_FRAME_WAIT);
        return true;
    case 7:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &guide_label, CALENDAR_VIEW_RECORD_WAIT);
        return true;
    case 8:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, CALENDAR_VIEW_RECORD_FRAME_WAIT);
        return true;
    case 9:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &compact_label, CALENDAR_VIEW_RECORD_WAIT);
        return true;
    case 10:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, CALENDAR_VIEW_RECORD_FRAME_WAIT);
        return true;
    default:
        return false;
    }
}
#endif
