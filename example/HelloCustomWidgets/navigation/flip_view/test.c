#include <string.h>

#include "egui.h"
#include "egui_view_flip_view.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define FLIP_VIEW_ROOT_WIDTH          224
#define FLIP_VIEW_ROOT_HEIGHT         300
#define FLIP_VIEW_PRIMARY_WIDTH       196
#define FLIP_VIEW_PRIMARY_HEIGHT      118
#define FLIP_VIEW_PREVIEW_WIDTH       104
#define FLIP_VIEW_PREVIEW_HEIGHT      64
#define FLIP_VIEW_BOTTOM_ROW_WIDTH    216
#define FLIP_VIEW_BOTTOM_ROW_HEIGHT   82
#define FLIP_VIEW_GUIDE_COLOR         0x72808E
#define FLIP_VIEW_LABEL_COLOR         0x74808D
#define FLIP_VIEW_STATUS_COLOR        0x4E6174
#define FLIP_VIEW_COMPACT_LABEL_COLOR 0x0C8A78
#define FLIP_VIEW_LOCKED_LABEL_COLOR  0x8995A2
#define FLIP_VIEW_RECORD_WAIT         110
#define FLIP_VIEW_RECORD_FRAME_WAIT   150

typedef struct flip_view_track flip_view_track_t;
struct flip_view_track
{
    const char *title;
    const char *helper;
    const char *status_prefix;
    egui_color_t status_color;
    const egui_view_flip_view_item_t *items;
    uint8_t item_count;
    uint8_t current_index;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_flip_view_t flip_view_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_flip_view_t flip_view_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_flip_view_t flip_view_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Flip View";
static const char *guide_text = "Tap guide to switch tracks";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {150, 0}};
static char status_text[80] = "Stories 2 / 4 - Aurora deck";
static uint8_t primary_track_index = 0;
static uint8_t compact_track_index = 0;

static const egui_view_flip_view_item_t stories_items[] = {
        {"Night", "Aurora deck", "Muted hero card with overlay actions", "Move through story cards", EGUI_COLOR_HEX(0xD9EEF6), EGUI_COLOR_HEX(0x2183B8)},
        {"Draft", "Night city", "Use keyboard or buttons to flip focus", "Keyboard and touch stay aligned", EGUI_COLOR_HEX(0xEAE4FB), EGUI_COLOR_HEX(0x6C56CF)},
        {"Focus", "Studio notes", "Compact preview keeps the same content rail", "Preview cards stay low-noise", EGUI_COLOR_HEX(0xE2F5E8),
         EGUI_COLOR_HEX(0x178A5D)},
        {"Safe", "Quiet route", "Read only mode freezes previous and next", "Muted shell keeps the view calm", EGUI_COLOR_HEX(0xF8ECDC),
         EGUI_COLOR_HEX(0xCA7A15)},
};

static const egui_view_flip_view_item_t planner_items[] = {
        {"Week", "Planner stack", "Single hero surface replaces perspective cards", "Different from coverflow strip", EGUI_COLOR_HEX(0xE4F0FF),
         EGUI_COLOR_HEX(0x2563EB)},
        {"Flow", "Review board", "Previous and next stay in overlay buttons", "Different from pips pager", EGUI_COLOR_HEX(0xF6E5FF), EGUI_COLOR_HEX(0x9333EA)},
        {"Lite", "Focus shelf", "Compact preview hides long helper strings", "Same track in smaller shell", EGUI_COLOR_HEX(0xE1F8F2), EGUI_COLOR_HEX(0x0F766E)},
};

static const egui_view_flip_view_item_t archive_items[] = {
        {"Memo", "Field archive", "Hero cards can carry short metadata only", "No image asset required", EGUI_COLOR_HEX(0xF7EFE2), EGUI_COLOR_HEX(0xB45309)},
        {"Scan", "Signal shelf", "Muted footer keeps action hints lightweight", "Footer tracks current snapshot", EGUI_COLOR_HEX(0xE8F0E4),
         EGUI_COLOR_HEX(0x3F7D37)},
        {"Loop", "Review loop", "Guide label swaps the whole content track", "Sample page keeps states visible", EGUI_COLOR_HEX(0xE9EDF9),
         EGUI_COLOR_HEX(0x4F46E5)},
        {"Safe", "Paper trail", "Read only preview shows disabled arrows", "State boundary stays explicit", EGUI_COLOR_HEX(0xF5E8E8), EGUI_COLOR_HEX(0xB91C1C)},
};

static const egui_view_flip_view_item_t compact_story_items[] = {
        {"Lite", "Pocket", "Compact keeps only the essential text", "", EGUI_COLOR_HEX(0xE0F5F1), EGUI_COLOR_HEX(0x0D9488)},
        {"Mini", "Quick", "Buttons stay overlayed on the hero card", "", EGUI_COLOR_HEX(0xE7F0FF), EGUI_COLOR_HEX(0x2563EB)},
        {"Mini", "Review", "Footer still anchors the current context", "", EGUI_COLOR_HEX(0xF3E8FF), EGUI_COLOR_HEX(0x8B5CF6)},
};

static const egui_view_flip_view_item_t compact_notes_items[] = {
        {"Lite", "Memo", "Compact mode removes long helper rows", "", EGUI_COLOR_HEX(0xFFF3E6), EGUI_COLOR_HEX(0xD97706)},
        {"Lite", "Safe", "Different track keeps the compact preview fresh", "", EGUI_COLOR_HEX(0xE5F6EE), EGUI_COLOR_HEX(0x0F766E)},
};

static const egui_view_flip_view_item_t locked_items[] = {
        {"Lock", "Locked", "Navigation is intentionally frozen", "", EGUI_COLOR_HEX(0xEEF2F6), EGUI_COLOR_HEX(0x94A3B8)},
        {"Lock", "Frozen", "Overlay arrows stay disabled", "", EGUI_COLOR_HEX(0xF4F6F8), EGUI_COLOR_HEX(0xA1AAB5)},
        {"Lock", "Archive", "Hero shell keeps the same layout", "", EGUI_COLOR_HEX(0xF2F4F7), EGUI_COLOR_HEX(0x9CA3AF)},
};

static const flip_view_track_t primary_tracks[] = {
        {"Stories", "Overlay previous and next keep the hero card clean", "Stories", EGUI_COLOR_HEX(0x2183B8), stories_items, 4, 1},
        {"Planner", "FlipView keeps only one active card in view", "Planner", EGUI_COLOR_HEX(0x2563EB), planner_items, 3, 0},
        {"Archive", "Guide swaps tracks without changing the widget shape", "Archive", EGUI_COLOR_HEX(0xB45309), archive_items, 4, 2},
};

static const flip_view_track_t compact_tracks[] = {
        {"Compact", "", "Compact", EGUI_COLOR_HEX(0x0D9488), compact_story_items, 3, 0},
        {"Compact", "", "Compact", EGUI_COLOR_HEX(0xD97706), compact_notes_items, 2, 1},
};

static const flip_view_track_t locked_track = {"Read only", "", "Locked", EGUI_COLOR_HEX(0x94A3B8), locked_items, 3, 1};

static void update_status(const flip_view_track_t *track)
{
    const egui_view_flip_view_item_t *item = egui_view_flip_view_get_current_item(EGUI_VIEW_OF(&flip_view_primary));
    uint8_t current_index = egui_view_flip_view_get_current_index(EGUI_VIEW_OF(&flip_view_primary));
    int pos = 0;

    pos += egui_sprintf_str(status_text, sizeof(status_text), track->status_prefix);
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " ");
    pos += egui_sprintf_int(&status_text[pos], (int)sizeof(status_text) - pos, current_index + 1);
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " / ");
    pos += egui_sprintf_int(&status_text[pos], (int)sizeof(status_text) - pos, track->item_count);
    if (item != NULL)
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " - ");
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, item->title);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(track->status_color, EGUI_COLOR_HEX(FLIP_VIEW_STATUS_COLOR), 26), EGUI_ALPHA_100);
}

static void apply_track(egui_view_t *view, const flip_view_track_t *track)
{
    egui_view_flip_view_set_title(view, track->title);
    egui_view_flip_view_set_helper(view, track->helper);
    egui_view_flip_view_set_items(view, track->items, track->item_count, track->current_index);
    egui_view_flip_view_set_current_part(view, EGUI_VIEW_FLIP_VIEW_PART_SURFACE);
}

static void apply_primary_track(uint8_t index, uint8_t update_status_flag)
{
    const flip_view_track_t *track = &primary_tracks[index % (sizeof(primary_tracks) / sizeof(primary_tracks[0]))];

    primary_track_index = index % (sizeof(primary_tracks) / sizeof(primary_tracks[0]));
    apply_track(EGUI_VIEW_OF(&flip_view_primary), track);
    if (update_status_flag)
    {
        update_status(track);
    }
}

static void apply_compact_track(uint8_t index)
{
    const flip_view_track_t *track = &compact_tracks[index % (sizeof(compact_tracks) / sizeof(compact_tracks[0]))];

    compact_track_index = index % (sizeof(compact_tracks) / sizeof(compact_tracks[0]));
    apply_track(EGUI_VIEW_OF(&flip_view_compact), track);
}

static void on_primary_changed(egui_view_t *self, uint8_t current_index, uint8_t item_count, uint8_t part)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(current_index);
    EGUI_UNUSED(item_count);
    EGUI_UNUSED(part);
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
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), FLIP_VIEW_ROOT_WIDTH, FLIP_VIEW_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), FLIP_VIEW_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), FLIP_VIEW_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(FLIP_VIEW_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), FLIP_VIEW_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(FLIP_VIEW_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_flip_view_init(EGUI_VIEW_OF(&flip_view_primary));
    egui_view_set_size(EGUI_VIEW_OF(&flip_view_primary), FLIP_VIEW_PRIMARY_WIDTH, FLIP_VIEW_PRIMARY_HEIGHT);
    egui_view_flip_view_set_font(EGUI_VIEW_OF(&flip_view_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_flip_view_set_meta_font(EGUI_VIEW_OF(&flip_view_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_flip_view_set_palette(EGUI_VIEW_OF(&flip_view_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD7DFE7), EGUI_COLOR_HEX(0x1A2630),
                                    EGUI_COLOR_HEX(0x72808E), EGUI_COLOR_HEX(0xAAB6C3));
    egui_view_flip_view_set_on_changed_listener(EGUI_VIEW_OF(&flip_view_primary), on_primary_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&flip_view_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&flip_view_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), FLIP_VIEW_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(FLIP_VIEW_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), FLIP_VIEW_BOTTOM_ROW_WIDTH, FLIP_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), FLIP_VIEW_PREVIEW_WIDTH, FLIP_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), FLIP_VIEW_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(FLIP_VIEW_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_flip_view_init(EGUI_VIEW_OF(&flip_view_compact));
    egui_view_set_size(EGUI_VIEW_OF(&flip_view_compact), FLIP_VIEW_PREVIEW_WIDTH, FLIP_VIEW_PREVIEW_HEIGHT);
    egui_view_flip_view_set_font(EGUI_VIEW_OF(&flip_view_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_flip_view_set_meta_font(EGUI_VIEW_OF(&flip_view_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_flip_view_set_compact_mode(EGUI_VIEW_OF(&flip_view_compact), 1);
    egui_view_flip_view_set_palette(EGUI_VIEW_OF(&flip_view_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD2DDDA), EGUI_COLOR_HEX(0x17302A),
                                    EGUI_COLOR_HEX(0x57756C), EGUI_COLOR_HEX(0x8FB9B1));
    static egui_view_api_t flip_view_compact_touch_api;
    egui_view_override_api_on_touch(EGUI_VIEW_OF(&flip_view_compact), &flip_view_compact_touch_api, consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&flip_view_compact), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&flip_view_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), FLIP_VIEW_PREVIEW_WIDTH, FLIP_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 8, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), FLIP_VIEW_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(FLIP_VIEW_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_flip_view_init(EGUI_VIEW_OF(&flip_view_locked));
    egui_view_set_size(EGUI_VIEW_OF(&flip_view_locked), FLIP_VIEW_PREVIEW_WIDTH, FLIP_VIEW_PREVIEW_HEIGHT);
    egui_view_flip_view_set_font(EGUI_VIEW_OF(&flip_view_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_flip_view_set_meta_font(EGUI_VIEW_OF(&flip_view_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_flip_view_set_compact_mode(EGUI_VIEW_OF(&flip_view_locked), 1);
    egui_view_flip_view_set_read_only_mode(EGUI_VIEW_OF(&flip_view_locked), 1);
    egui_view_flip_view_set_palette(EGUI_VIEW_OF(&flip_view_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xDBE2E8), EGUI_COLOR_HEX(0x536474),
                                    EGUI_COLOR_HEX(0x8896A4), EGUI_COLOR_HEX(0xB3BFCA));
    static egui_view_api_t flip_view_locked_touch_api;
    egui_view_override_api_on_touch(EGUI_VIEW_OF(&flip_view_locked), &flip_view_locked_touch_api, consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&flip_view_locked), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&flip_view_locked));

    apply_primary_track(0, 1);
    apply_compact_track(0);
    apply_track(EGUI_VIEW_OF(&flip_view_locked), &locked_track);

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
    EGUI_VIEW_OF(&flip_view_primary)->api->on_key_event(EGUI_VIEW_OF(&flip_view_primary), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    EGUI_VIEW_OF(&flip_view_primary)->api->on_key_event(EGUI_VIEW_OF(&flip_view_primary), &event);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, FLIP_VIEW_RECORD_WAIT);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, FLIP_VIEW_RECORD_FRAME_WAIT);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_RIGHT);
        }
        EGUI_SIM_SET_WAIT(p_action, FLIP_VIEW_RECORD_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, FLIP_VIEW_RECORD_FRAME_WAIT);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_END);
        }
        EGUI_SIM_SET_WAIT(p_action, FLIP_VIEW_RECORD_WAIT);
        return true;
    case 5:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, FLIP_VIEW_RECORD_FRAME_WAIT);
        return true;
    case 6:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &guide_label, FLIP_VIEW_RECORD_WAIT);
        return true;
    case 7:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, FLIP_VIEW_RECORD_FRAME_WAIT);
        return true;
    case 8:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &compact_label, FLIP_VIEW_RECORD_WAIT);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, FLIP_VIEW_RECORD_FRAME_WAIT);
        return true;
    default:
        return false;
    }
}
#endif
