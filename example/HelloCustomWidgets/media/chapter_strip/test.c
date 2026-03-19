#include <string.h>

#include "egui.h"
#include "egui_view_chapter_strip.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define CHAPTER_STRIP_ROOT_WIDTH            224
#define CHAPTER_STRIP_ROOT_HEIGHT           312
#define CHAPTER_STRIP_PRIMARY_WIDTH         196
#define CHAPTER_STRIP_PRIMARY_HEIGHT        126
#define CHAPTER_STRIP_PREVIEW_COLUMN_WIDTH  104
#define CHAPTER_STRIP_PREVIEW_CONTENT_WIDTH 92
#define CHAPTER_STRIP_PREVIEW_HEIGHT        70
#define CHAPTER_STRIP_BOTTOM_ROW_WIDTH      212
#define CHAPTER_STRIP_BOTTOM_ROW_HEIGHT     96
#define CHAPTER_STRIP_RECORD_WAIT           110
#define CHAPTER_STRIP_RECORD_FRAME_WAIT     150

typedef struct chapter_strip_session chapter_strip_session_t;
struct chapter_strip_session
{
    const char *title;
    const char *helper;
    const char *status_prefix;
    egui_color_t status_color;
    const egui_view_chapter_strip_item_t *items;
    uint8_t count;
    uint8_t current_index;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_chapter_strip_t chapter_strip_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_chapter_strip_t chapter_strip_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_chapter_strip_t chapter_strip_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_status_pill_param, EGUI_COLOR_HEX(0xF8FBFF), EGUI_ALPHA_100, 7, 1, EGUI_COLOR_HEX(0xD8E3EF),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_status_pill_params, &bg_status_pill_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_status_pill, &bg_status_pill_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_status_pill_warm_param, EGUI_COLOR_HEX(0xFFF8EF), EGUI_ALPHA_100, 7, 1, EGUI_COLOR_HEX(0xEFD9B8),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_status_pill_warm_params, &bg_status_pill_warm_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_status_pill_warm, &bg_status_pill_warm_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_status_pill_fresh_param, EGUI_COLOR_HEX(0xF3FBF6), EGUI_ALPHA_100, 7, 1, EGUI_COLOR_HEX(0xCFE7D7),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_status_pill_fresh_params, &bg_status_pill_fresh_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_status_pill_fresh, &bg_status_pill_fresh_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_guide_chip_param, EGUI_COLOR_HEX(0xF4F8FC), EGUI_ALPHA_100, 7, 1, EGUI_COLOR_HEX(0xD7E3EE),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_guide_chip_params, &bg_guide_chip_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_guide_chip, &bg_guide_chip_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_section_pill_param, EGUI_COLOR_HEX(0xFAFBFD), EGUI_ALPHA_100, 6, 1, EGUI_COLOR_HEX(0xE1E7EE),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_section_pill_params, &bg_section_pill_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_section_pill, &bg_section_pill_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_compact_card_param, EGUI_COLOR_HEX(0xF6FBFA), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xD7E8E2),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_compact_card_params, &bg_compact_card_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_compact_card, &bg_compact_card_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_compact_header_param, EGUI_COLOR_HEX(0xEFF4F1), EGUI_ALPHA_100, 6, 1, EGUI_COLOR_HEX(0xCDDCD6),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_compact_header_params, &bg_compact_header_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_compact_header, &bg_compact_header_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_locked_card_param, EGUI_COLOR_HEX(0xF7FAFC), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xDFE6EC),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_locked_card_params, &bg_locked_card_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_locked_card, &bg_locked_card_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_locked_header_param, EGUI_COLOR_HEX(0xF0F4F7), EGUI_ALPHA_100, 6, 1, EGUI_COLOR_HEX(0xD6DDE4),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_locked_header_params, &bg_locked_header_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_locked_header, &bg_locked_header_params);

static const char *title_text = "Chapter Strip";
static const char *guide_text = "Tap guide to swap tracks";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {112, 0}};
static char status_text[96] = "Podcast 02 Scene setup";
static uint8_t primary_session_index = 0;
static uint8_t compact_session_index = 0;

static const egui_view_chapter_strip_item_t podcast_items[] = {
        {"Intro", "Cold open", "00:12", EGUI_COLOR_HEX(0x2563EB)},  {"Host", "Scene setup", "02:18", EGUI_COLOR_HEX(0x2563EB)},
        {"Guest", "Ad break", "07:40", EGUI_COLOR_HEX(0x2563EB)},   {"Call", "Deep dive", "12:32", EGUI_COLOR_HEX(0x2563EB)},
        {"Wrap", "Closing cue", "18:45", EGUI_COLOR_HEX(0x2563EB)},
};
static const egui_view_chapter_strip_item_t trailer_items[] = {
        {"Beat", "Logo hit", "00:08", EGUI_COLOR_HEX(0xD97706)},
        {"Scene", "Tension rise", "00:27", EGUI_COLOR_HEX(0xD97706)},
        {"Reveal", "Hero turn", "00:51", EGUI_COLOR_HEX(0xD97706)},
        {"Punch", "Final tag", "01:12", EGUI_COLOR_HEX(0xD97706)},
};
static const egui_view_chapter_strip_item_t lesson_items[] = {
        {"Plan", "Kickoff", "00:30", EGUI_COLOR_HEX(0x15803D)},     {"Board", "Problem framing", "04:10", EGUI_COLOR_HEX(0x15803D)},
        {"Demo", "Live coding", "10:35", EGUI_COLOR_HEX(0x15803D)}, {"Fix", "Refactor pass", "16:42", EGUI_COLOR_HEX(0x15803D)},
        {"Review", "Q and A", "23:18", EGUI_COLOR_HEX(0x15803D)},   {"Wrap", "Homework", "28:04", EGUI_COLOR_HEX(0x15803D)},
};
static const egui_view_chapter_strip_item_t compact_mix_items[] = {
        {"A", "Pocket mix", "00:18", EGUI_COLOR_HEX(0x0D9488)},
        {"B", "Hook", "01:12", EGUI_COLOR_HEX(0x0D9488)},
        {"C", "Solo", "02:24", EGUI_COLOR_HEX(0x0D9488)},
        {"D", "Drop", "03:07", EGUI_COLOR_HEX(0x0D9488)},
};
static const egui_view_chapter_strip_item_t compact_story_items[] = {
        {"1", "Panel 1", "00:14", EGUI_COLOR_HEX(0x7C3AED)},
        {"2", "Panel 2", "00:46", EGUI_COLOR_HEX(0x7C3AED)},
        {"3", "Panel 3", "01:08", EGUI_COLOR_HEX(0x7C3AED)},
};
static const egui_view_chapter_strip_item_t locked_items[] = {
        {"Audit", "Delivery", "00:20", EGUI_COLOR_HEX(0x94A3B8)},
        {"Audit", "Checksum", "01:14", EGUI_COLOR_HEX(0x94A3B8)},
        {"Audit", "Archive", "02:06", EGUI_COLOR_HEX(0x94A3B8)},
        {"Audit", "Notes", "03:12", EGUI_COLOR_HEX(0x94A3B8)},
};

static const chapter_strip_session_t primary_sessions[] = {
        {"Reference track", "Arrow keys move chapters.", "Podcast", EGUI_COLOR_HEX(0x2563EB), podcast_items,
         (uint8_t)(sizeof(podcast_items) / sizeof(podcast_items[0])), 1},
        {"Reference track", "Discrete chapters, no scrubbing.", "Trailer", EGUI_COLOR_HEX(0xD97706), trailer_items,
         (uint8_t)(sizeof(trailer_items) / sizeof(trailer_items[0])), 0},
        {"Reference track", "Compact rail mirrors lesson cut.", "Workshop", EGUI_COLOR_HEX(0x15803D), lesson_items,
         (uint8_t)(sizeof(lesson_items) / sizeof(lesson_items[0])), 2},
};
static const chapter_strip_session_t compact_sessions[] = {
        {NULL, NULL, "Compact", EGUI_COLOR_HEX(0x0D9488), compact_mix_items, (uint8_t)(sizeof(compact_mix_items) / sizeof(compact_mix_items[0])), 1},
        {NULL, NULL, "Compact", EGUI_COLOR_HEX(0x7C3AED), compact_story_items, (uint8_t)(sizeof(compact_story_items) / sizeof(compact_story_items[0])), 0},
};
static const chapter_strip_session_t locked_session = {
        NULL, NULL, "Read only", EGUI_COLOR_HEX(0x94A3B8), locked_items, (uint8_t)(sizeof(locked_items) / sizeof(locked_items[0])), 2};

static void format_index_label(uint8_t index, char *buffer, int buffer_size)
{
    int pos = 0;

    if (buffer == NULL || buffer_size <= 0)
    {
        return;
    }
    if (index + 1 < 10)
    {
        pos += egui_sprintf_char(buffer, buffer_size, '0');
    }
    egui_sprintf_int(&buffer[pos], buffer_size - pos, index + 1);
}

static void update_status(const chapter_strip_session_t *session)
{
    char index_buffer[8];
    uint8_t current_index = egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&chapter_strip_primary));
    int pos = 0;

    if (current_index >= session->count)
    {
        current_index = 0;
    }
    format_index_label(current_index, index_buffer, sizeof(index_buffer));
    pos += egui_sprintf_str(status_text, sizeof(status_text), session->status_prefix);
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " ");
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, index_buffer);
    pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " ");
    egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, session->items[current_index].title);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    if (session == &primary_sessions[1])
    {
        egui_view_set_background(EGUI_VIEW_OF(&status_label), EGUI_BG_OF(&bg_status_pill_warm));
    }
    else if (session == &primary_sessions[2])
    {
        egui_view_set_background(EGUI_VIEW_OF(&status_label), EGUI_BG_OF(&bg_status_pill_fresh));
    }
    else
    {
        egui_view_set_background(EGUI_VIEW_OF(&status_label), EGUI_BG_OF(&bg_status_pill));
    }
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(session->status_color, EGUI_COLOR_HEX(0x4E6174), 28), EGUI_ALPHA_100);
}

static void apply_session(egui_view_t *view, const chapter_strip_session_t *session)
{
    egui_view_chapter_strip_set_title(view, session->title);
    egui_view_chapter_strip_set_helper(view, session->helper);
    egui_view_chapter_strip_set_items(view, session->items, session->count, session->current_index);
}

static void apply_primary_session(uint8_t index, uint8_t update_status_flag)
{
    const chapter_strip_session_t *session = &primary_sessions[index % (sizeof(primary_sessions) / sizeof(primary_sessions[0]))];

    primary_session_index = index % (sizeof(primary_sessions) / sizeof(primary_sessions[0]));
    apply_session(EGUI_VIEW_OF(&chapter_strip_primary), session);
    if (update_status_flag)
    {
        update_status(session);
    }
}

static void apply_compact_session(uint8_t index)
{
    const chapter_strip_session_t *session = &compact_sessions[index % (sizeof(compact_sessions) / sizeof(compact_sessions[0]))];

    compact_session_index = index % (sizeof(compact_sessions) / sizeof(compact_sessions[0]));
    apply_session(EGUI_VIEW_OF(&chapter_strip_compact), session);
}

static void on_primary_changed(egui_view_t *self, uint8_t current_index, uint8_t chapter_count, uint8_t current_part)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(current_index);
    EGUI_UNUSED(chapter_count);
    EGUI_UNUSED(current_part);
    update_status(&primary_sessions[primary_session_index]);
}

static void on_guide_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_primary_session((uint8_t)(primary_session_index + 1), 1);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_compact_session((uint8_t)(compact_session_index + 1));
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
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), CHAPTER_STRIP_ROOT_WIDTH, CHAPTER_STRIP_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), CHAPTER_STRIP_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 7, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 166, 14);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x64788D), EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&guide_label), EGUI_BG_OF(&bg_guide_chip));
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 1, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), 58, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(0x74808D), EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&primary_label), EGUI_BG_OF(&bg_section_pill));
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_chapter_strip_init(EGUI_VIEW_OF(&chapter_strip_primary));
    egui_view_set_size(EGUI_VIEW_OF(&chapter_strip_primary), CHAPTER_STRIP_PRIMARY_WIDTH, CHAPTER_STRIP_PRIMARY_HEIGHT);
    egui_view_chapter_strip_set_font(EGUI_VIEW_OF(&chapter_strip_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_chapter_strip_set_meta_font(EGUI_VIEW_OF(&chapter_strip_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_chapter_strip_set_palette(EGUI_VIEW_OF(&chapter_strip_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD7DFE7), EGUI_COLOR_HEX(0x1A2630),
                                        EGUI_COLOR_HEX(0x72808E), EGUI_COLOR_HEX(0xAAB6C3));
    egui_view_chapter_strip_set_on_changed_listener(EGUI_VIEW_OF(&chapter_strip_primary), on_primary_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&chapter_strip_primary), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&chapter_strip_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 138, 14);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(0x4E6174), EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&status_label), EGUI_BG_OF(&bg_status_pill));
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 112, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xE7EDF3));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 1, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), CHAPTER_STRIP_BOTTOM_ROW_WIDTH, CHAPTER_STRIP_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), CHAPTER_STRIP_PREVIEW_COLUMN_WIDTH, CHAPTER_STRIP_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&compact_column), EGUI_BG_OF(&bg_compact_card));
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), 66, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0x0C8A78), EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&compact_label), EGUI_BG_OF(&bg_compact_header));
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 5, 0, 4);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_chapter_strip_init(EGUI_VIEW_OF(&chapter_strip_compact));
    egui_view_set_size(EGUI_VIEW_OF(&chapter_strip_compact), CHAPTER_STRIP_PREVIEW_CONTENT_WIDTH, CHAPTER_STRIP_PREVIEW_HEIGHT);
    egui_view_chapter_strip_set_font(EGUI_VIEW_OF(&chapter_strip_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_chapter_strip_set_meta_font(EGUI_VIEW_OF(&chapter_strip_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_chapter_strip_set_compact_mode(EGUI_VIEW_OF(&chapter_strip_compact), 1);
    egui_view_chapter_strip_set_palette(EGUI_VIEW_OF(&chapter_strip_compact), EGUI_COLOR_HEX(0xF6FCFA), EGUI_COLOR_HEX(0xD5E3DF), EGUI_COLOR_HEX(0x17302A),
                                        EGUI_COLOR_HEX(0x57756C), EGUI_COLOR_HEX(0x8FB9B1));
    egui_view_set_margin(EGUI_VIEW_OF(&chapter_strip_compact), 0, 1, 0, 1);
    static egui_view_api_t chapter_strip_compact_touch_api;
    egui_view_override_api_on_touch(EGUI_VIEW_OF(&chapter_strip_compact), &chapter_strip_compact_touch_api, consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&chapter_strip_compact), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&chapter_strip_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), CHAPTER_STRIP_PREVIEW_COLUMN_WIDTH, CHAPTER_STRIP_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&locked_column), EGUI_BG_OF(&bg_locked_card));
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), 68, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(0x84909D), EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&locked_label), EGUI_BG_OF(&bg_locked_header));
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 6, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_chapter_strip_init(EGUI_VIEW_OF(&chapter_strip_locked));
    egui_view_set_size(EGUI_VIEW_OF(&chapter_strip_locked), CHAPTER_STRIP_PREVIEW_CONTENT_WIDTH, CHAPTER_STRIP_PREVIEW_HEIGHT);
    egui_view_chapter_strip_set_font(EGUI_VIEW_OF(&chapter_strip_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_chapter_strip_set_meta_font(EGUI_VIEW_OF(&chapter_strip_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_chapter_strip_set_compact_mode(EGUI_VIEW_OF(&chapter_strip_locked), 1);
    egui_view_chapter_strip_set_read_only_mode(EGUI_VIEW_OF(&chapter_strip_locked), 1);
    egui_view_chapter_strip_set_palette(EGUI_VIEW_OF(&chapter_strip_locked), EGUI_COLOR_HEX(0xF9FBFC), EGUI_COLOR_HEX(0xDDE4EA), EGUI_COLOR_HEX(0x536474),
                                        EGUI_COLOR_HEX(0x8896A4), EGUI_COLOR_HEX(0xB3BFCA));
    egui_view_set_margin(EGUI_VIEW_OF(&chapter_strip_locked), 0, 1, 0, 1);
    static egui_view_api_t chapter_strip_locked_touch_api;
    egui_view_override_api_on_touch(EGUI_VIEW_OF(&chapter_strip_locked), &chapter_strip_locked_touch_api, consume_preview_touch);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&chapter_strip_locked), false);
#endif
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&chapter_strip_locked));

    apply_primary_session(0, 1);
    apply_compact_session(0);
    apply_session(EGUI_VIEW_OF(&chapter_strip_locked), &locked_session);

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
    EGUI_VIEW_OF(&chapter_strip_primary)->api->on_key_event(EGUI_VIEW_OF(&chapter_strip_primary), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    EGUI_VIEW_OF(&chapter_strip_primary)->api->on_key_event(EGUI_VIEW_OF(&chapter_strip_primary), &event);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, CHAPTER_STRIP_RECORD_WAIT);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, CHAPTER_STRIP_RECORD_FRAME_WAIT);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_RIGHT);
        }
        EGUI_SIM_SET_WAIT(p_action, CHAPTER_STRIP_RECORD_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, CHAPTER_STRIP_RECORD_FRAME_WAIT);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_END);
        }
        EGUI_SIM_SET_WAIT(p_action, CHAPTER_STRIP_RECORD_WAIT);
        return true;
    case 5:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, CHAPTER_STRIP_RECORD_FRAME_WAIT);
        return true;
    case 6:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &guide_label, CHAPTER_STRIP_RECORD_WAIT);
        return true;
    case 7:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, CHAPTER_STRIP_RECORD_FRAME_WAIT);
        return true;
    case 8:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &compact_label, CHAPTER_STRIP_RECORD_WAIT);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, CHAPTER_STRIP_RECORD_FRAME_WAIT);
        return true;
    default:
        return false;
    }
}
#endif
