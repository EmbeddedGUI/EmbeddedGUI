#include "egui.h"
#include "egui_view_rating_control.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define RATING_PRIMARY_WIDTH       196
#define RATING_PRIMARY_HEIGHT      92
#define RATING_COMPACT_WIDTH       106
#define RATING_COMPACT_HEIGHT      42
#define RATING_BOTTOM_ROW_WIDTH    216
#define RATING_BOTTOM_ROW_HEIGHT   68
#define RATING_GUIDE_COLOR         0x6F7C8B
#define RATING_PRIMARY_LABEL_COLOR 0x788492
#define RATING_STATUS_COLOR        0x5B6D81
#define RATING_COMPACT_LABEL_COLOR 0x936D18
#define RATING_LOCKED_LABEL_COLOR  0x8894A2
#define RATING_STANDARD_ACCENT     0xD59E20
#define RATING_STANDARD_BORDER     0xD7DFE7
#define RATING_STANDARD_SHADOW     0xDBE3EC
#define RATING_COMPACT_ACCENT      0xC78C16
#define RATING_COMPACT_BORDER      0xDADFE5
#define RATING_LOCKED_ACCENT       0x9DA9B5
#define RATING_LOCKED_BORDER       0xD9E1E8
#define RATING_RECORD_STEP_WAIT    90
#define RATING_RECORD_FRAME_WAIT   130
#define RATING_RECORD_FINAL_WAIT   320

typedef struct rating_demo_snapshot rating_demo_snapshot_t;
struct rating_demo_snapshot
{
    const char *title;
    const char *low_label;
    const char *high_label;
    const char **value_labels;
    const char *status_prefix;
    uint8_t label_count;
    uint8_t item_count;
    uint8_t value;
    uint8_t clear_enabled;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_rating_control_t control_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_rating_control_t control_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_rating_control_t control_locked;

static uint8_t g_primary_snapshot = 0;
static uint8_t g_compact_snapshot = 0;
static uint8_t g_ignore_status_change = 0;
static char status_text[48] = "Service 4/5 Great";

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Rating Control";
static const char *guide_text = "Tap guide to cycle scenarios";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {143, 0}};

static const char *primary_labels_0[] = {"No rating", "Poor", "Fair", "Good", "Great", "Excellent"};
static const char *primary_labels_1[] = {"No rating", "Very slow", "Slow", "On time", "Fast", "Rapid"};
static const char *primary_labels_2[] = {"No rating", "Hard", "Rough", "Okay", "Smooth", "Effortless"};

static const rating_demo_snapshot_t primary_snapshots[] = {
        {"Service quality", "Low", "High", primary_labels_0, "Service", 6, 5, 4, 1},
        {"Delivery speed", "Slow", "Fast", primary_labels_1, "Delivery", 6, 5, 2, 1},
        {"Setup experience", "Hard", "Easy", primary_labels_2, "Setup", 6, 5, 5, 1},
};

static const char *compact_labels_0[] = {"No rating", "Poor", "Fair", "Good", "Great", "Excellent"};
static const char *compact_labels_1[] = {"No rating", "Weak", "Okay", "Good", "Strong", "Top"};

static const rating_demo_snapshot_t compact_snapshots[] = {
        {NULL, NULL, NULL, compact_labels_0, "Compact", 6, 5, 3, 0},
        {NULL, NULL, NULL, compact_labels_1, "Compact", 6, 5, 3, 0},
};

static const char *snapshot_label(const rating_demo_snapshot_t *snapshot, uint8_t value)
{
    if (snapshot == NULL || snapshot->value_labels == NULL || value >= snapshot->label_count)
    {
        return NULL;
    }
    return snapshot->value_labels[value];
}

static void set_status_line(const rating_demo_snapshot_t *snapshot, uint8_t value, egui_color_t color)
{
    int pos = 0;
    const char *label = snapshot_label(snapshot, value);

    if (snapshot == NULL)
    {
        return;
    }

    pos += egui_sprintf_str(status_text, sizeof(status_text), snapshot->status_prefix);
    pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
    pos += egui_sprintf_int(&status_text[pos], (int)sizeof(status_text) - pos, value);
    pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, '/');
    pos += egui_sprintf_int(&status_text[pos], (int)sizeof(status_text) - pos, snapshot->item_count);
    if (label != NULL)
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, label);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(RATING_STATUS_COLOR), 24), EGUI_ALPHA_100);
}

static void apply_snapshot(egui_view_t *view, const rating_demo_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    egui_view_rating_control_set_title(view, snapshot->title);
    egui_view_rating_control_set_low_label(view, snapshot->low_label);
    egui_view_rating_control_set_high_label(view, snapshot->high_label);
    egui_view_rating_control_set_value_labels(view, snapshot->value_labels, snapshot->label_count);
    egui_view_rating_control_set_item_count(view, snapshot->item_count);
    egui_view_rating_control_set_clear_enabled(view, snapshot->clear_enabled);
    egui_view_rating_control_set_value(view, snapshot->value);
}

static void apply_primary_snapshot(uint8_t index, uint8_t update_status)
{
    const rating_demo_snapshot_t *snapshot = &primary_snapshots[index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]))];

    g_primary_snapshot = index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]));
    g_ignore_status_change = 1;
    apply_snapshot(EGUI_VIEW_OF(&control_primary), snapshot);
    g_ignore_status_change = 0;
    if (update_status)
    {
        set_status_line(snapshot, snapshot->value, EGUI_COLOR_HEX(RATING_STANDARD_ACCENT));
    }
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    const rating_demo_snapshot_t *snapshot = &compact_snapshots[index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]))];

    g_compact_snapshot = index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]));
    g_ignore_status_change = 1;
    apply_snapshot(EGUI_VIEW_OF(&control_compact), snapshot);
    g_ignore_status_change = 0;
    if (update_status)
    {
        set_status_line(snapshot, snapshot->value, EGUI_COLOR_HEX(RATING_COMPACT_ACCENT));
    }
}

static void on_primary_changed(egui_view_t *self, uint8_t value, uint8_t part)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(part);
    if (g_ignore_status_change)
    {
        return;
    }
    set_status_line(&primary_snapshots[g_primary_snapshot], value, EGUI_COLOR_HEX(RATING_STANDARD_ACCENT));
}

static void on_compact_changed(egui_view_t *self, uint8_t value, uint8_t part)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(part);
    if (g_ignore_status_change)
    {
        return;
    }
    set_status_line(&compact_snapshots[g_compact_snapshot], value, EGUI_COLOR_HEX(RATING_COMPACT_ACCENT));
}

static void on_guide_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_primary_snapshot((uint8_t)(g_primary_snapshot + 1), 1);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_compact_snapshot((uint8_t)(g_compact_snapshot + 1), 1);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 288);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(RATING_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), 224, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(RATING_PRIMARY_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_rating_control_init(EGUI_VIEW_OF(&control_primary));
    egui_view_set_size(EGUI_VIEW_OF(&control_primary), RATING_PRIMARY_WIDTH, RATING_PRIMARY_HEIGHT);
    egui_view_rating_control_set_font(EGUI_VIEW_OF(&control_primary), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_rating_control_set_meta_font(EGUI_VIEW_OF(&control_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_rating_control_set_palette(EGUI_VIEW_OF(&control_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(RATING_STANDARD_BORDER),
                                         EGUI_COLOR_HEX(0x1E2832), EGUI_COLOR_HEX(0x71808F), EGUI_COLOR_HEX(RATING_STANDARD_ACCENT),
                                         EGUI_COLOR_HEX(RATING_STANDARD_SHADOW));
    egui_view_set_margin(EGUI_VIEW_OF(&control_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&control_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(RATING_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), RATING_BOTTOM_ROW_WIDTH, RATING_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), RATING_COMPACT_WIDTH, RATING_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), RATING_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(RATING_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_rating_control_init(EGUI_VIEW_OF(&control_compact));
    egui_view_set_size(EGUI_VIEW_OF(&control_compact), RATING_COMPACT_WIDTH, RATING_COMPACT_HEIGHT);
    egui_view_rating_control_set_font(EGUI_VIEW_OF(&control_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_rating_control_set_meta_font(EGUI_VIEW_OF(&control_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_rating_control_set_compact_mode(EGUI_VIEW_OF(&control_compact), 1);
    egui_view_rating_control_set_palette(EGUI_VIEW_OF(&control_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(RATING_COMPACT_BORDER),
                                         EGUI_COLOR_HEX(0x2B3138), EGUI_COLOR_HEX(0x74808C), EGUI_COLOR_HEX(RATING_COMPACT_ACCENT), EGUI_COLOR_HEX(0xE0E6EC));
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&control_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), RATING_COMPACT_WIDTH, RATING_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), RATING_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(RATING_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_rating_control_init(EGUI_VIEW_OF(&control_locked));
    egui_view_set_size(EGUI_VIEW_OF(&control_locked), RATING_COMPACT_WIDTH, RATING_COMPACT_HEIGHT);
    egui_view_rating_control_set_font(EGUI_VIEW_OF(&control_locked), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_rating_control_set_meta_font(EGUI_VIEW_OF(&control_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_rating_control_set_compact_mode(EGUI_VIEW_OF(&control_locked), 1);
    egui_view_rating_control_set_read_only_mode(EGUI_VIEW_OF(&control_locked), 1);
    egui_view_rating_control_set_palette(EGUI_VIEW_OF(&control_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(RATING_LOCKED_BORDER),
                                         EGUI_COLOR_HEX(0x596878), EGUI_COLOR_HEX(0x8C98A5), EGUI_COLOR_HEX(RATING_LOCKED_ACCENT), EGUI_COLOR_HEX(0xE3E9EF));
    egui_view_rating_control_set_value_labels(EGUI_VIEW_OF(&control_locked), primary_labels_0, 6);
    egui_view_rating_control_set_value(EGUI_VIEW_OF(&control_locked), 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&control_locked));

    egui_view_rating_control_set_on_changed_listener(EGUI_VIEW_OF(&control_primary), on_primary_changed);
    egui_view_rating_control_set_on_changed_listener(EGUI_VIEW_OF(&control_compact), on_compact_changed);

    apply_primary_snapshot(0, 1);
    apply_compact_snapshot(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&locked_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&control_primary));
#endif

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_invalidate(EGUI_VIEW_OF(&root_layout));
#if EGUI_CONFIG_RECORDING_TEST
    recording_request_snapshot();
#endif
}

#if EGUI_CONFIG_RECORDING_TEST
static void apply_primary_key(uint8_t key_code)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&control_primary));
#endif
    egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&control_primary), key_code);
}

static void set_click_rating_part(egui_sim_action_t *p_action, egui_view_t *view, uint8_t part, int interval_ms)
{
    egui_region_t region;

    if (!egui_view_rating_control_get_part_region(view, part, &region))
    {
        EGUI_SIM_SET_WAIT(p_action, interval_ms);
        return;
    }

    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = region.location.x + region.size.width / 2;
    p_action->y1 = region.location.y + region.size.height / 2;
    p_action->x2 = 0;
    p_action->y2 = 0;
    p_action->steps = 0;
    p_action->interval_ms = interval_ms;
}

static void set_click_rating_gap_toward_next(egui_sim_action_t *p_action, egui_view_t *view, uint8_t left_part, int interval_ms)
{
    egui_region_t left_region;
    egui_region_t right_region;
    egui_dim_t gap_start;
    egui_dim_t gap_width;

    if (!egui_view_rating_control_get_part_region(view, left_part, &left_region) ||
        !egui_view_rating_control_get_part_region(view, (uint8_t)(left_part + 1), &right_region))
    {
        EGUI_SIM_SET_WAIT(p_action, interval_ms);
        return;
    }

    gap_start = left_region.location.x + left_region.size.width;
    gap_width = right_region.location.x - gap_start;
    if (gap_width <= 0)
    {
        set_click_rating_part(p_action, view, (uint8_t)(left_part + 1), interval_ms);
        return;
    }

    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = gap_start + gap_width - 1;
    p_action->y1 = left_region.location.y + left_region.size.height / 2;
    p_action->x2 = 0;
    p_action->y2 = 0;
    p_action->steps = 0;
    p_action->interval_ms = interval_ms;
}

static void set_drag_rating_parts(egui_sim_action_t *p_action, egui_view_t *view, uint8_t start_part, uint8_t end_part, int steps, int interval_ms)
{
    egui_region_t start_region;
    egui_region_t end_region;

    if (!egui_view_rating_control_get_part_region(view, start_part, &start_region) || !egui_view_rating_control_get_part_region(view, end_part, &end_region))
    {
        EGUI_SIM_SET_WAIT(p_action, interval_ms);
        return;
    }

    p_action->type = EGUI_SIM_ACTION_DRAG;
    p_action->x1 = start_region.location.x + start_region.size.width / 2;
    p_action->y1 = start_region.location.y + start_region.size.height / 2;
    p_action->x2 = end_region.location.x + end_region.size.width / 2;
    p_action->y2 = end_region.location.y + end_region.size.height / 2;
    p_action->steps = steps;
    p_action->interval_ms = interval_ms;
}

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
            apply_primary_snapshot(0, 1);
            apply_compact_snapshot(0, 0);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 1:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 2:
        set_click_rating_part(p_action, EGUI_VIEW_OF(&control_primary), 5, RATING_RECORD_STEP_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 4:
        set_click_rating_part(p_action, EGUI_VIEW_OF(&control_primary), EGUI_VIEW_RATING_CONTROL_PART_CLEAR, RATING_RECORD_STEP_WAIT);
        return true;
    case 5:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 6:
        if (first_call)
        {
            apply_primary_snapshot(1, 1);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 7:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 8:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_END);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 9:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 10:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_LEFT);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 11:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 12:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 13:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 14:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 15:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_LEFT);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 16:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 17:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 18:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 19:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_ESCAPE);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 20:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 21:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_HOME);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 22:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 23:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 24:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_SPACE);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 25:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 26:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 27:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_ENTER);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 28:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 29:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 30:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 31:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 32:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_SPACE);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 33:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 34:
        if (first_call)
        {
            egui_view_rating_control_set_value(EGUI_VIEW_OF(&control_primary), 3);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 35:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 36:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 37:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_TAB);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 38:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 39:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_ENTER);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 40:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 41:
        if (first_call)
        {
            egui_view_rating_control_set_value(EGUI_VIEW_OF(&control_primary), 4);
            apply_compact_snapshot(1, 1);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 42:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 43:
        set_click_rating_gap_toward_next(p_action, EGUI_VIEW_OF(&control_compact), 3, RATING_RECORD_STEP_WAIT);
        return true;
    case 44:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 45:
        if (first_call)
        {
            apply_primary_snapshot(1, 1);
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_STEP_WAIT);
        return true;
    case 46:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FRAME_WAIT);
        return true;
    case 47:
        set_drag_rating_parts(p_action, EGUI_VIEW_OF(&control_primary), 2, 5, 6, RATING_RECORD_STEP_WAIT);
        return true;
    case 48:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, RATING_RECORD_FINAL_WAIT);
        return true;
    default:
        return false;
    }
}
#endif
