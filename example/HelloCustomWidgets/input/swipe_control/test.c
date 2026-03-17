#include <string.h>

#include "egui.h"
#include "egui_view_swipe_control.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define SWIPE_CONTROL_ROOT_WIDTH        224
#define SWIPE_CONTROL_ROOT_HEIGHT       300
#define SWIPE_CONTROL_PRIMARY_WIDTH     196
#define SWIPE_CONTROL_PRIMARY_HEIGHT    118
#define SWIPE_CONTROL_PREVIEW_WIDTH     104
#define SWIPE_CONTROL_PREVIEW_HEIGHT    64
#define SWIPE_CONTROL_BOTTOM_ROW_WIDTH  216
#define SWIPE_CONTROL_BOTTOM_ROW_HEIGHT 82
#define SWIPE_CONTROL_RECORD_WAIT       110
#define SWIPE_CONTROL_RECORD_FRAME_WAIT 150

typedef struct swipe_control_track swipe_control_track_t;
struct swipe_control_track
{
    const char *title;
    const char *helper;
    const char *status_prefix;
    egui_color_t status_color;
    const egui_view_swipe_control_item_t *item;
    const egui_view_swipe_control_action_t *start_action;
    const egui_view_swipe_control_action_t *end_action;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_swipe_control_t swipe_control_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_swipe_control_t swipe_control_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_swipe_control_t swipe_control_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Swipe Control";
static const char *guide_text = "Tap guide to swap rows";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {150, 0}};
static char status_text[96] = "Inbox - Surface - Invoice follow-up";
static uint8_t primary_track_index = 0;
static uint8_t compact_track_index = 0;

static const egui_view_swipe_control_item_t inbox_item = {"Mail",      "Invoice follow-up",      "Reveal pin or delete.",
                                                          "Due today", EGUI_COLOR_HEX(0xE9F4FF), EGUI_COLOR_HEX(0x2563EB)};
static const egui_view_swipe_control_action_t inbox_start_action = {"Pin", "Keep", EGUI_COLOR_HEX(0x0F766E), EGUI_COLOR_HEX(0xFFFFFF)};
static const egui_view_swipe_control_action_t inbox_end_action = {"Delete", "Remove", EGUI_COLOR_HEX(0xDC2626), EGUI_COLOR_HEX(0xFFFFFF)};

static const egui_view_swipe_control_item_t planner_item = {"Plan",        "Planner sync",           "Row shifts to expose actions.",
                                                            "Board ready", EGUI_COLOR_HEX(0xF2E8FF), EGUI_COLOR_HEX(0x9333EA)};
static const egui_view_swipe_control_action_t planner_start_action = {"Flag", "Review", EGUI_COLOR_HEX(0x7C3AED), EGUI_COLOR_HEX(0xFFFFFF)};
static const egui_view_swipe_control_action_t planner_end_action = {"Archive", "Store", EGUI_COLOR_HEX(0xB45309), EGUI_COLOR_HEX(0xFFFFFF)};

static const egui_view_swipe_control_item_t review_item = {"Build",      "Renderer check",         "Keys and touch share one rail.",
                                                           "Waiting QA", EGUI_COLOR_HEX(0xE5F7EC), EGUI_COLOR_HEX(0x15803D)};
static const egui_view_swipe_control_action_t review_start_action = {"Done", "Close", EGUI_COLOR_HEX(0x15803D), EGUI_COLOR_HEX(0xFFFFFF)};
static const egui_view_swipe_control_action_t review_end_action = {"Snooze", "Later", EGUI_COLOR_HEX(0x2563EB), EGUI_COLOR_HEX(0xFFFFFF)};

static const egui_view_swipe_control_item_t compact_mail_item = {"Mini", "Pocket", "", "", EGUI_COLOR_HEX(0xE0F5F1), EGUI_COLOR_HEX(0x0D9488)};
static const egui_view_swipe_control_action_t compact_mail_start_action = {"Pin", "", EGUI_COLOR_HEX(0x0F766E), EGUI_COLOR_HEX(0xFFFFFF)};
static const egui_view_swipe_control_action_t compact_mail_end_action = {"Delete", "", EGUI_COLOR_HEX(0xDC2626), EGUI_COLOR_HEX(0xFFFFFF)};

static const egui_view_swipe_control_item_t compact_queue_item = {"Mini", "Queue", "", "", EGUI_COLOR_HEX(0xFFF2E5), EGUI_COLOR_HEX(0xD97706)};
static const egui_view_swipe_control_action_t compact_queue_start_action = {"Flag", "", EGUI_COLOR_HEX(0x7C3AED), EGUI_COLOR_HEX(0xFFFFFF)};
static const egui_view_swipe_control_action_t compact_queue_end_action = {"Archive", "", EGUI_COLOR_HEX(0xB45309), EGUI_COLOR_HEX(0xFFFFFF)};

static const egui_view_swipe_control_item_t locked_item = {"Lock", "Locked", "", "", EGUI_COLOR_HEX(0xF1F4F7), EGUI_COLOR_HEX(0x94A3B8)};
static const egui_view_swipe_control_action_t locked_start_action = {"Pin", "", EGUI_COLOR_HEX(0x9AA5B1), EGUI_COLOR_HEX(0xFFFFFF)};
static const egui_view_swipe_control_action_t locked_end_action = {"Delete", "", EGUI_COLOR_HEX(0xB8C1CB), EGUI_COLOR_HEX(0xFFFFFF)};

static const swipe_control_track_t primary_tracks[] = {
        {"Inbox", "Right=Pin, Left=Delete.", "Inbox", EGUI_COLOR_HEX(0x2563EB), &inbox_item, &inbox_start_action, &inbox_end_action},
        {"Planner", "Shift row to expose actions.", "Planner", EGUI_COLOR_HEX(0x9333EA), &planner_item, &planner_start_action, &planner_end_action},
        {"Review", "Keys and touch share one state.", "Review", EGUI_COLOR_HEX(0x15803D), &review_item, &review_start_action, &review_end_action},
};

static const swipe_control_track_t compact_tracks[] = {
        {"Compact", "", "Compact", EGUI_COLOR_HEX(0x0D9488), &compact_mail_item, &compact_mail_start_action, &compact_mail_end_action},
        {"Compact", "", "Compact", EGUI_COLOR_HEX(0xD97706), &compact_queue_item, &compact_queue_start_action, &compact_queue_end_action},
};

static const swipe_control_track_t locked_track = {"Read only", "", "Locked", EGUI_COLOR_HEX(0x94A3B8), &locked_item, &locked_start_action, &locked_end_action};

static void update_status(const swipe_control_track_t *track)
{
    const egui_view_swipe_control_item_t *item = egui_view_swipe_control_get_item(EGUI_VIEW_OF(&swipe_control_primary));
    uint8_t reveal_state = egui_view_swipe_control_get_reveal_state(EGUI_VIEW_OF(&swipe_control_primary));
    const char *state_text = "Surface";
    int pos = 0;

    if (reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_START && track->start_action != NULL)
    {
        state_text = track->start_action->label;
    }
    else if (reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_END && track->end_action != NULL)
    {
        state_text = track->end_action->label;
    }

    pos += egui_sprintf_str(status_text, sizeof(status_text), track->status_prefix);
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " - ");
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, state_text);
    if (item != NULL)
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " - ");
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, item->title);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(track->status_color, EGUI_COLOR_HEX(0x4E6174), 26), EGUI_ALPHA_100);
}

static void apply_track(egui_view_t *view, const swipe_control_track_t *track)
{
    egui_view_swipe_control_set_title(view, track->title);
    egui_view_swipe_control_set_helper(view, track->helper);
    egui_view_swipe_control_set_item(view, track->item);
    egui_view_swipe_control_set_actions(view, track->start_action, track->end_action);
    egui_view_swipe_control_set_reveal_state(view, EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE);
}

static void apply_primary_track(uint8_t index, uint8_t update_status_flag)
{
    const swipe_control_track_t *track = &primary_tracks[index % (sizeof(primary_tracks) / sizeof(primary_tracks[0]))];

    primary_track_index = index % (sizeof(primary_tracks) / sizeof(primary_tracks[0]));
    apply_track(EGUI_VIEW_OF(&swipe_control_primary), track);
    if (update_status_flag)
    {
        update_status(track);
    }
}

static void apply_compact_track(uint8_t index)
{
    const swipe_control_track_t *track = &compact_tracks[index % (sizeof(compact_tracks) / sizeof(compact_tracks[0]))];

    compact_track_index = index % (sizeof(compact_tracks) / sizeof(compact_tracks[0]));
    apply_track(EGUI_VIEW_OF(&swipe_control_compact), track);
}

static void on_primary_changed(egui_view_t *self, uint8_t reveal_state, uint8_t current_part)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(reveal_state);
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
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), SWIPE_CONTROL_ROOT_WIDTH, SWIPE_CONTROL_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), SWIPE_CONTROL_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), SWIPE_CONTROL_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x72808E), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), SWIPE_CONTROL_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(0x74808D), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_swipe_control_init(EGUI_VIEW_OF(&swipe_control_primary));
    egui_view_set_size(EGUI_VIEW_OF(&swipe_control_primary), SWIPE_CONTROL_PRIMARY_WIDTH, SWIPE_CONTROL_PRIMARY_HEIGHT);
    egui_view_swipe_control_set_font(EGUI_VIEW_OF(&swipe_control_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_swipe_control_set_meta_font(EGUI_VIEW_OF(&swipe_control_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_swipe_control_set_palette(EGUI_VIEW_OF(&swipe_control_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD7DFE7), EGUI_COLOR_HEX(0x1A2630),
                                        EGUI_COLOR_HEX(0x72808E), EGUI_COLOR_HEX(0xAAB6C3));
    egui_view_swipe_control_set_on_changed_listener(EGUI_VIEW_OF(&swipe_control_primary), on_primary_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&swipe_control_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&swipe_control_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), SWIPE_CONTROL_ROOT_WIDTH, 12);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), SWIPE_CONTROL_BOTTOM_ROW_WIDTH, SWIPE_CONTROL_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), SWIPE_CONTROL_PREVIEW_WIDTH, SWIPE_CONTROL_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), SWIPE_CONTROL_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0x0C8A78), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_swipe_control_init(EGUI_VIEW_OF(&swipe_control_compact));
    egui_view_set_size(EGUI_VIEW_OF(&swipe_control_compact), SWIPE_CONTROL_PREVIEW_WIDTH, SWIPE_CONTROL_PREVIEW_HEIGHT);
    egui_view_swipe_control_set_font(EGUI_VIEW_OF(&swipe_control_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_swipe_control_set_meta_font(EGUI_VIEW_OF(&swipe_control_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_swipe_control_set_compact_mode(EGUI_VIEW_OF(&swipe_control_compact), 1);
    egui_view_swipe_control_set_palette(EGUI_VIEW_OF(&swipe_control_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD2DDDA), EGUI_COLOR_HEX(0x17302A),
                                        EGUI_COLOR_HEX(0x57756C), EGUI_COLOR_HEX(0x8FB9B1));
    egui_view_set_on_touch_listener(EGUI_VIEW_OF(&swipe_control_compact), consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&swipe_control_compact), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&swipe_control_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), SWIPE_CONTROL_PREVIEW_WIDTH, SWIPE_CONTROL_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 8, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), SWIPE_CONTROL_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(0x8995A2), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_swipe_control_init(EGUI_VIEW_OF(&swipe_control_locked));
    egui_view_set_size(EGUI_VIEW_OF(&swipe_control_locked), SWIPE_CONTROL_PREVIEW_WIDTH, SWIPE_CONTROL_PREVIEW_HEIGHT);
    egui_view_swipe_control_set_font(EGUI_VIEW_OF(&swipe_control_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_swipe_control_set_meta_font(EGUI_VIEW_OF(&swipe_control_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_swipe_control_set_compact_mode(EGUI_VIEW_OF(&swipe_control_locked), 1);
    egui_view_swipe_control_set_read_only_mode(EGUI_VIEW_OF(&swipe_control_locked), 1);
    egui_view_swipe_control_set_palette(EGUI_VIEW_OF(&swipe_control_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xDBE2E8), EGUI_COLOR_HEX(0x536474),
                                        EGUI_COLOR_HEX(0x8896A4), EGUI_COLOR_HEX(0xB3BFCA));
    egui_view_set_on_touch_listener(EGUI_VIEW_OF(&swipe_control_locked), consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&swipe_control_locked), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&swipe_control_locked));

    apply_primary_track(0, 1);
    apply_compact_track(0);
    apply_track(EGUI_VIEW_OF(&swipe_control_locked), &locked_track);

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
    EGUI_VIEW_OF(&swipe_control_primary)->api->on_key_event(EGUI_VIEW_OF(&swipe_control_primary), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    EGUI_VIEW_OF(&swipe_control_primary)->api->on_key_event(EGUI_VIEW_OF(&swipe_control_primary), &event);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, SWIPE_CONTROL_RECORD_WAIT);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, SWIPE_CONTROL_RECORD_FRAME_WAIT);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_RIGHT);
        }
        EGUI_SIM_SET_WAIT(p_action, SWIPE_CONTROL_RECORD_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, SWIPE_CONTROL_RECORD_FRAME_WAIT);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_LEFT);
        }
        EGUI_SIM_SET_WAIT(p_action, SWIPE_CONTROL_RECORD_WAIT);
        return true;
    case 5:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, SWIPE_CONTROL_RECORD_FRAME_WAIT);
        return true;
    case 6:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &guide_label, SWIPE_CONTROL_RECORD_WAIT);
        return true;
    case 7:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, SWIPE_CONTROL_RECORD_FRAME_WAIT);
        return true;
    case 8:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &compact_label, SWIPE_CONTROL_RECORD_WAIT);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, SWIPE_CONTROL_RECORD_FRAME_WAIT);
        return true;
    default:
        return false;
    }
}
#endif
