#include <string.h>

#include "egui.h"
#include "egui_view_transport_bar.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define TRANSPORT_BAR_ROOT_WIDTH        224
#define TRANSPORT_BAR_ROOT_HEIGHT       312
#define TRANSPORT_BAR_PRIMARY_WIDTH     196
#define TRANSPORT_BAR_PRIMARY_HEIGHT    132
#define TRANSPORT_BAR_PREVIEW_WIDTH     104
#define TRANSPORT_BAR_PREVIEW_HEIGHT    72
#define TRANSPORT_BAR_BOTTOM_ROW_WIDTH  216
#define TRANSPORT_BAR_BOTTOM_ROW_HEIGHT 90
#define TRANSPORT_BAR_RECORD_WAIT       110
#define TRANSPORT_BAR_RECORD_FRAME_WAIT 150

typedef struct transport_bar_track transport_bar_track_t;
struct transport_bar_track
{
    const char *title;
    const char *helper;
    const char *status_prefix;
    egui_color_t status_color;
    const egui_view_transport_bar_snapshot_t *snapshot;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_transport_bar_t transport_bar_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_transport_bar_t transport_bar_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_transport_bar_t transport_bar_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Transport Bar";
static const char *guide_text = "Tap guide to swap tracks";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {150, 0}};
static char status_text[96] = "Studio mix - Playing - 1:28 / 3:42";
static uint8_t primary_track_index = 0;
static uint8_t compact_track_index = 0;

static const egui_view_transport_bar_snapshot_t studio_snapshot = {
        "Live", "Studio mix", "Cue-ready host marker.", "Space toggles pause and resume.", EGUI_COLOR_HEX(0xE8F3FF), EGUI_COLOR_HEX(0x2563EB), 88, 222, 1};
static const egui_view_transport_bar_snapshot_t podcast_snapshot = {"Voice",
                                                                    "Podcast edit",
                                                                    "Chapter preview with intro cue.",
                                                                    "Plus/minus nudge the seek rail.",
                                                                    EGUI_COLOR_HEX(0xF2EBFF),
                                                                    EGUI_COLOR_HEX(0x7C3AED),
                                                                    154,
                                                                    428,
                                                                    0};
static const egui_view_transport_bar_snapshot_t trailer_snapshot = {"Scene",
                                                                    "Trailer stem",
                                                                    "Jump 30 sec with previous or next.",
                                                                    "Guide swaps between reference decks.",
                                                                    EGUI_COLOR_HEX(0xE8F7EE),
                                                                    EGUI_COLOR_HEX(0x15803D),
                                                                    42,
                                                                    96,
                                                                    1};
static const egui_view_transport_bar_snapshot_t compact_mix_snapshot = {"Mini", "Pocket", "", "", EGUI_COLOR_HEX(0xE0F5F1), EGUI_COLOR_HEX(0x0D9488),
                                                                        36,     180,      1};
static const egui_view_transport_bar_snapshot_t compact_clip_snapshot = {"Mini", "Clip", "", "", EGUI_COLOR_HEX(0xFFF4E5), EGUI_COLOR_HEX(0xD97706), 12, 64, 0};
static const egui_view_transport_bar_snapshot_t locked_snapshot = {"Lock", "Audit", "", "", EGUI_COLOR_HEX(0xF1F4F7), EGUI_COLOR_HEX(0x94A3B8), 96, 240, 0};

static const transport_bar_track_t primary_tracks[] = {
        {"Now playing", "Tab focus, Space toggle, +/- seek.", "Studio mix", EGUI_COLOR_HEX(0x2563EB), &studio_snapshot},
        {"Now playing", "Guide swaps the active show.", "Podcast edit", EGUI_COLOR_HEX(0x7C3AED), &podcast_snapshot},
        {"Now playing", "Previous and next jump 30 sec.", "Trailer stem", EGUI_COLOR_HEX(0x15803D), &trailer_snapshot},
};

static const transport_bar_track_t compact_tracks[] = {
        {NULL, NULL, "Compact", EGUI_COLOR_HEX(0x0D9488), &compact_mix_snapshot},
        {NULL, NULL, "Compact", EGUI_COLOR_HEX(0xD97706), &compact_clip_snapshot},
};

static const transport_bar_track_t locked_track = {NULL, NULL, "Read only", EGUI_COLOR_HEX(0x94A3B8), &locked_snapshot};

static void format_time_pair(uint16_t position_seconds, uint16_t duration_seconds, char *buffer, int buffer_size)
{
    char left[12];
    char right[12];
    int pos = 0;

    if (buffer == NULL || buffer_size <= 0)
    {
        return;
    }

    egui_sprintf_int(left, sizeof(left), position_seconds / 60);
    egui_sprintf_char(&left[strlen(left)], sizeof(left) - (int)strlen(left), ':');
    egui_sprintf_int_pad(&left[strlen(left)], sizeof(left) - (int)strlen(left), position_seconds % 60, 2, '0');

    egui_sprintf_int(right, sizeof(right), duration_seconds / 60);
    egui_sprintf_char(&right[strlen(right)], sizeof(right) - (int)strlen(right), ':');
    egui_sprintf_int_pad(&right[strlen(right)], sizeof(right) - (int)strlen(right), duration_seconds % 60, 2, '0');

    pos += egui_sprintf_str(buffer, buffer_size, left);
    pos += egui_sprintf_str(&buffer[pos], buffer_size - pos, " / ");
    egui_sprintf_str(&buffer[pos], buffer_size - pos, right);
}

static const char *part_to_text(uint8_t current_part, uint8_t playing)
{
    switch (current_part)
    {
    case EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS:
        return "Previous";
    case EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE:
        return playing ? "Playing" : "Paused";
    case EGUI_VIEW_TRANSPORT_BAR_PART_NEXT:
        return "Next";
    case EGUI_VIEW_TRANSPORT_BAR_PART_SEEK:
        return "Seek";
    default:
        return playing ? "Playing" : "Paused";
    }
}

static void update_status(const transport_bar_track_t *track)
{
    char time_buffer[24];
    int pos = 0;

    format_time_pair(egui_view_transport_bar_get_position_seconds(EGUI_VIEW_OF(&transport_bar_primary)),
                     egui_view_transport_bar_get_duration_seconds(EGUI_VIEW_OF(&transport_bar_primary)), time_buffer, sizeof(time_buffer));

    pos += egui_sprintf_str(status_text, sizeof(status_text), track->status_prefix);
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " - ");
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos,
                            part_to_text(egui_view_transport_bar_get_current_part(EGUI_VIEW_OF(&transport_bar_primary)),
                                         egui_view_transport_bar_get_playing(EGUI_VIEW_OF(&transport_bar_primary))));
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " - ");
    egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, time_buffer);

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(track->status_color, EGUI_COLOR_HEX(0x4E6174), 26), EGUI_ALPHA_100);
}

static void apply_track(egui_view_t *view, const transport_bar_track_t *track)
{
    egui_view_transport_bar_set_title(view, track->title);
    egui_view_transport_bar_set_helper(view, track->helper);
    egui_view_transport_bar_set_snapshot(view, track->snapshot);
}

static void apply_primary_track(uint8_t index, uint8_t update_status_flag)
{
    const transport_bar_track_t *track = &primary_tracks[index % (sizeof(primary_tracks) / sizeof(primary_tracks[0]))];

    primary_track_index = index % (sizeof(primary_tracks) / sizeof(primary_tracks[0]));
    apply_track(EGUI_VIEW_OF(&transport_bar_primary), track);
    if (update_status_flag)
    {
        update_status(track);
    }
}

static void apply_compact_track(uint8_t index)
{
    const transport_bar_track_t *track = &compact_tracks[index % (sizeof(compact_tracks) / sizeof(compact_tracks[0]))];

    compact_track_index = index % (sizeof(compact_tracks) / sizeof(compact_tracks[0]));
    apply_track(EGUI_VIEW_OF(&transport_bar_compact), track);
}

static void on_primary_changed(egui_view_t *self, uint16_t position_seconds, uint16_t duration_seconds, uint8_t playing, uint8_t current_part)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(position_seconds);
    EGUI_UNUSED(duration_seconds);
    EGUI_UNUSED(playing);
    EGUI_UNUSED(current_part);
    update_status(&primary_tracks[primary_track_index]);
}

static void on_guide_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_primary_track((uint8_t)(primary_track_index + 1), 1);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_compact_track((uint8_t)(compact_track_index + 1));
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
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), TRANSPORT_BAR_ROOT_WIDTH, TRANSPORT_BAR_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), TRANSPORT_BAR_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), TRANSPORT_BAR_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x72808E), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), TRANSPORT_BAR_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(0x74808D), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_transport_bar_init(EGUI_VIEW_OF(&transport_bar_primary));
    egui_view_set_size(EGUI_VIEW_OF(&transport_bar_primary), TRANSPORT_BAR_PRIMARY_WIDTH, TRANSPORT_BAR_PRIMARY_HEIGHT);
    egui_view_transport_bar_set_font(EGUI_VIEW_OF(&transport_bar_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_transport_bar_set_meta_font(EGUI_VIEW_OF(&transport_bar_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_transport_bar_set_palette(EGUI_VIEW_OF(&transport_bar_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD7DFE7), EGUI_COLOR_HEX(0x1A2630),
                                        EGUI_COLOR_HEX(0x72808E), EGUI_COLOR_HEX(0xAAB6C3));
    egui_view_transport_bar_set_on_changed_listener(EGUI_VIEW_OF(&transport_bar_primary), on_primary_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&transport_bar_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&transport_bar_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), TRANSPORT_BAR_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(0x4E6174), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 150, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDEE4EB));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), TRANSPORT_BAR_BOTTOM_ROW_WIDTH, TRANSPORT_BAR_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), TRANSPORT_BAR_PREVIEW_WIDTH, TRANSPORT_BAR_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), TRANSPORT_BAR_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0x0C8A78), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_transport_bar_init(EGUI_VIEW_OF(&transport_bar_compact));
    egui_view_set_size(EGUI_VIEW_OF(&transport_bar_compact), TRANSPORT_BAR_PREVIEW_WIDTH, TRANSPORT_BAR_PREVIEW_HEIGHT);
    egui_view_transport_bar_set_font(EGUI_VIEW_OF(&transport_bar_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_transport_bar_set_meta_font(EGUI_VIEW_OF(&transport_bar_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_transport_bar_set_compact_mode(EGUI_VIEW_OF(&transport_bar_compact), 1);
    egui_view_transport_bar_set_palette(EGUI_VIEW_OF(&transport_bar_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD2DDDA), EGUI_COLOR_HEX(0x17302A),
                                        EGUI_COLOR_HEX(0x57756C), EGUI_COLOR_HEX(0x8FB9B1));
    static egui_view_api_t transport_bar_compact_touch_api;
    egui_view_override_api_on_touch(EGUI_VIEW_OF(&transport_bar_compact), &transport_bar_compact_touch_api, consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&transport_bar_compact), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&transport_bar_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), TRANSPORT_BAR_PREVIEW_WIDTH, TRANSPORT_BAR_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 8, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), TRANSPORT_BAR_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(0x8995A2), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_transport_bar_init(EGUI_VIEW_OF(&transport_bar_locked));
    egui_view_set_size(EGUI_VIEW_OF(&transport_bar_locked), TRANSPORT_BAR_PREVIEW_WIDTH, TRANSPORT_BAR_PREVIEW_HEIGHT);
    egui_view_transport_bar_set_font(EGUI_VIEW_OF(&transport_bar_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_transport_bar_set_meta_font(EGUI_VIEW_OF(&transport_bar_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_transport_bar_set_compact_mode(EGUI_VIEW_OF(&transport_bar_locked), 1);
    egui_view_transport_bar_set_read_only_mode(EGUI_VIEW_OF(&transport_bar_locked), 1);
    egui_view_transport_bar_set_palette(EGUI_VIEW_OF(&transport_bar_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xDBE2E8), EGUI_COLOR_HEX(0x536474),
                                        EGUI_COLOR_HEX(0x8896A4), EGUI_COLOR_HEX(0xB3BFCA));
    static egui_view_api_t transport_bar_locked_touch_api;
    egui_view_override_api_on_touch(EGUI_VIEW_OF(&transport_bar_locked), &transport_bar_locked_touch_api, consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&transport_bar_locked), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&transport_bar_locked));

    apply_primary_track(0, 1);
    apply_compact_track(0);
    apply_track(EGUI_VIEW_OF(&transport_bar_locked), &locked_track);

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
    EGUI_VIEW_OF(&transport_bar_primary)->api->on_key_event(EGUI_VIEW_OF(&transport_bar_primary), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    EGUI_VIEW_OF(&transport_bar_primary)->api->on_key_event(EGUI_VIEW_OF(&transport_bar_primary), &event);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, TRANSPORT_BAR_RECORD_WAIT);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TRANSPORT_BAR_RECORD_FRAME_WAIT);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_SPACE);
        }
        EGUI_SIM_SET_WAIT(p_action, TRANSPORT_BAR_RECORD_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TRANSPORT_BAR_RECORD_FRAME_WAIT);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_PLUS);
        }
        EGUI_SIM_SET_WAIT(p_action, TRANSPORT_BAR_RECORD_WAIT);
        return true;
    case 5:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TRANSPORT_BAR_RECORD_FRAME_WAIT);
        return true;
    case 6:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &guide_label, TRANSPORT_BAR_RECORD_WAIT);
        return true;
    case 7:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TRANSPORT_BAR_RECORD_FRAME_WAIT);
        return true;
    case 8:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &compact_label, TRANSPORT_BAR_RECORD_WAIT);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TRANSPORT_BAR_RECORD_FRAME_WAIT);
        return true;
    default:
        return false;
    }
}
#endif
