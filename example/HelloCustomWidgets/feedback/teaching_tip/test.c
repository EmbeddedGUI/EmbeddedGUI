#include "egui.h"
#include "egui_view_teaching_tip.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#include "core/egui_key_event.h"
#endif

#define TEACHING_TIP_ROOT_WIDTH        224
#define TEACHING_TIP_ROOT_HEIGHT       300
#define TEACHING_TIP_PRIMARY_WIDTH     196
#define TEACHING_TIP_PRIMARY_HEIGHT    132
#define TEACHING_TIP_PREVIEW_WIDTH     108
#define TEACHING_TIP_PREVIEW_HEIGHT    80
#define TEACHING_TIP_BOTTOM_ROW_WIDTH  222
#define TEACHING_TIP_BOTTOM_ROW_HEIGHT 90
#define TEACHING_TIP_GUIDE_COLOR       0x8B98A5
#define TEACHING_TIP_LABEL_COLOR       0x8693A0
#define TEACHING_TIP_STATUS_COLOR      0x536879
#define TEACHING_TIP_COMPACT_LABEL     0x0D786F
#define TEACHING_TIP_LOCKED_LABEL      0x7C8895
#define TEACHING_TIP_STATUS_MIX        28
#define TEACHING_TIP_STATUS_WIDTH      148
#define TEACHING_TIP_STATUS_HEIGHT     15
#define PRIMARY_SNAPSHOT_COUNT         ((uint8_t)(sizeof(primary_snapshots_template) / sizeof(primary_snapshots_template[0])))
#define COMPACT_SNAPSHOT_COUNT         ((uint8_t)(sizeof(compact_snapshots) / sizeof(compact_snapshots[0])))
#define LOCKED_SNAPSHOT_COUNT          ((uint8_t)(sizeof(locked_snapshots) / sizeof(locked_snapshots[0])))

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_teaching_tip_t tip_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_teaching_tip_t tip_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_teaching_tip_t tip_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_status_accent_panel_param, EGUI_COLOR_HEX(0xF4F8FD), EGUI_ALPHA_100, 7);
EGUI_BACKGROUND_PARAM_INIT(bg_status_accent_panel_params, &bg_status_accent_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_status_accent_panel, &bg_status_accent_panel_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_status_success_panel_param, EGUI_COLOR_HEX(0xF1F8F4), EGUI_ALPHA_100, 7);
EGUI_BACKGROUND_PARAM_INIT(bg_status_success_panel_params, &bg_status_success_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_status_success_panel, &bg_status_success_panel_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_status_warning_panel_param, EGUI_COLOR_HEX(0xFCF6EC), EGUI_ALPHA_100, 7);
EGUI_BACKGROUND_PARAM_INIT(bg_status_warning_panel_params, &bg_status_warning_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_status_warning_panel, &bg_status_warning_panel_params);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_status_neutral_panel_param, EGUI_COLOR_HEX(0xF4F6F8), EGUI_ALPHA_100, 7);
EGUI_BACKGROUND_PARAM_INIT(bg_status_neutral_panel_params, &bg_status_neutral_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_status_neutral_panel, &bg_status_neutral_panel_params);

static const char *title_text = "Teaching Tip";
static const char *guide_text = "Tap guide to cycle states";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {136, 0}};
static char status_text[64] = "Focus Pin tip";

static const egui_view_teaching_tip_snapshot_t primary_snapshots_template[] = {
        {"Quick filters", "Coachmark", "Pin today view", "Keep ship dates nearby.", "Pin tip", "Later", "Below target", EGUI_VIEW_TEACHING_TIP_TONE_ACCENT,
         EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM, 1, 1, 1, 1, EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, -18},
        {"Cmd palette", "Shortcut", "Press slash to search", "Jump to commands fast.", "Got it", "Tips", "Placement above target",
         EGUI_VIEW_TEACHING_TIP_TONE_ACCENT, EGUI_VIEW_TEACHING_TIP_PLACEMENT_TOP, 1, 1, 1, 1, EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, 22},
        {"Sync draft", "Warning", "Reconnect before publish", "Offline edits still queued.", "Review", "Dismiss", "Anchored warning tip",
         EGUI_VIEW_TEACHING_TIP_TONE_WARNING, EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM, 1, 1, 1, 1, EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, -10},
        {"Quick filters", "", "Tip hidden", "Tap target to reopen", "", "", "", EGUI_VIEW_TEACHING_TIP_TONE_NEUTRAL, EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM, 0,
         0, 0, 0, EGUI_VIEW_TEACHING_TIP_PART_TARGET, 0},
};

static const egui_view_teaching_tip_snapshot_t compact_snapshots[] = {
        {"Quick tip", "Hint", "Pin filters", "Keep this nearby.", "Open", "", "", EGUI_VIEW_TEACHING_TIP_TONE_ACCENT, EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM,
         1, 1, 0, 1, EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, -8},
        {"Search", "Hint", "Find", "Launch quick find.", "Try", "", "", EGUI_VIEW_TEACHING_TIP_TONE_SUCCESS, EGUI_VIEW_TEACHING_TIP_PLACEMENT_TOP, 1, 1, 0, 1,
         EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, 8},
};

static const egui_view_teaching_tip_snapshot_t locked_snapshots[] = {
        {"Preview", "Read only", "Review", "Preview only.", "Read", "", "Read-only preview", EGUI_VIEW_TEACHING_TIP_TONE_NEUTRAL,
         EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM, 1, 1, 0, 0, EGUI_VIEW_TEACHING_TIP_PART_TARGET, 6},
};

static egui_view_teaching_tip_snapshot_t primary_snapshots[PRIMARY_SNAPSHOT_COUNT];
static uint8_t primary_snapshot_index = 0;
static uint8_t compact_snapshot_index = 0;

static void reset_primary_snapshots(void)
{
    uint8_t index;

    for (index = 0; index < PRIMARY_SNAPSHOT_COUNT; index++)
    {
        primary_snapshots[index] = primary_snapshots_template[index];
    }
}

static void sync_closed_snapshot_from(uint8_t source_index)
{
    if (source_index >= PRIMARY_SNAPSHOT_COUNT || source_index == PRIMARY_SNAPSHOT_COUNT - 1)
    {
        return;
    }

    primary_snapshots[PRIMARY_SNAPSHOT_COUNT - 1] = primary_snapshots_template[PRIMARY_SNAPSHOT_COUNT - 1];
    primary_snapshots[PRIMARY_SNAPSHOT_COUNT - 1].target_label = primary_snapshots[source_index].target_label;
    primary_snapshots[PRIMARY_SNAPSHOT_COUNT - 1].tone = primary_snapshots[source_index].tone;
    primary_snapshots[PRIMARY_SNAPSHOT_COUNT - 1].target_offset_x = primary_snapshots[source_index].target_offset_x;
}

static egui_color_t teaching_tip_status_color(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_TEACHING_TIP_TONE_SUCCESS:
        return EGUI_COLOR_HEX(0x178454);
    case EGUI_VIEW_TEACHING_TIP_TONE_WARNING:
        return EGUI_COLOR_HEX(0xB77719);
    case EGUI_VIEW_TEACHING_TIP_TONE_NEUTRAL:
        return EGUI_COLOR_HEX(0x71808E);
    default:
        return EGUI_COLOR_HEX(0x2563EB);
    }
}

static egui_background_t *teaching_tip_status_background(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_TEACHING_TIP_TONE_SUCCESS:
        return EGUI_BG_OF(&bg_status_success_panel);
    case EGUI_VIEW_TEACHING_TIP_TONE_WARNING:
        return EGUI_BG_OF(&bg_status_warning_panel);
    case EGUI_VIEW_TEACHING_TIP_TONE_NEUTRAL:
        return EGUI_BG_OF(&bg_status_neutral_panel);
    default:
        return EGUI_BG_OF(&bg_status_accent_panel);
    }
}

static const char *teaching_tip_part_text(const egui_view_teaching_tip_snapshot_t *snapshot, uint8_t part)
{
    if (snapshot == NULL)
    {
        return "Idle";
    }

    switch (part)
    {
    case EGUI_VIEW_TEACHING_TIP_PART_TARGET:
        return snapshot->target_label;
    case EGUI_VIEW_TEACHING_TIP_PART_PRIMARY:
        return (snapshot->primary_action != NULL && snapshot->primary_action[0] != '\0') ? snapshot->primary_action : "Primary";
    case EGUI_VIEW_TEACHING_TIP_PART_SECONDARY:
        return (snapshot->secondary_action != NULL && snapshot->secondary_action[0] != '\0') ? snapshot->secondary_action : "Secondary";
    case EGUI_VIEW_TEACHING_TIP_PART_CLOSE:
        return "Close";
    default:
        return "Idle";
    }
}

static void set_status(const char *prefix, const egui_view_teaching_tip_snapshot_t *snapshot, uint8_t part)
{
    int pos;

    status_text[0] = '\0';
    pos = egui_sprintf_str(status_text, sizeof(status_text), prefix);
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, teaching_tip_part_text(snapshot, part));
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label),
                                   egui_rgb_mix(teaching_tip_status_color(snapshot == NULL ? EGUI_VIEW_TEACHING_TIP_TONE_ACCENT : snapshot->tone),
                                                EGUI_COLOR_HEX(0x54687A), TEACHING_TIP_STATUS_MIX),
                                   EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&status_label),
                             teaching_tip_status_background(snapshot == NULL ? EGUI_VIEW_TEACHING_TIP_TONE_ACCENT : snapshot->tone));
}

static void apply_primary_snapshot(uint8_t index)
{
    primary_snapshot_index = (uint8_t)(index % PRIMARY_SNAPSHOT_COUNT);
    egui_view_teaching_tip_set_current_snapshot(EGUI_VIEW_OF(&tip_primary), primary_snapshot_index);
    set_status("Focus", &primary_snapshots[primary_snapshot_index], egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&tip_primary)));
}

static void apply_compact_snapshot(uint8_t index)
{
    compact_snapshot_index = (uint8_t)(index % COMPACT_SNAPSHOT_COUNT);
    egui_view_teaching_tip_set_current_snapshot(EGUI_VIEW_OF(&tip_compact), compact_snapshot_index);
}

static void on_primary_part_changed(egui_view_t *self, uint8_t part)
{
    const egui_view_teaching_tip_snapshot_t *snapshot = &primary_snapshots[egui_view_teaching_tip_get_current_snapshot(self)];

    if (part == EGUI_VIEW_TEACHING_TIP_PART_CLOSE && primary_snapshot_index != PRIMARY_SNAPSHOT_COUNT - 1)
    {
        sync_closed_snapshot_from(primary_snapshot_index);
        apply_primary_snapshot((uint8_t)(PRIMARY_SNAPSHOT_COUNT - 1));
        egui_view_teaching_tip_set_current_part(EGUI_VIEW_OF(&tip_primary), EGUI_VIEW_TEACHING_TIP_PART_TARGET);
        set_status("Dismissed", &primary_snapshots[PRIMARY_SNAPSHOT_COUNT - 1], EGUI_VIEW_TEACHING_TIP_PART_TARGET);
        return;
    }

    if (part == EGUI_VIEW_TEACHING_TIP_PART_TARGET && primary_snapshot_index == PRIMARY_SNAPSHOT_COUNT - 1)
    {
        apply_primary_snapshot(0);
        egui_view_teaching_tip_set_current_part(EGUI_VIEW_OF(&tip_primary), EGUI_VIEW_TEACHING_TIP_PART_TARGET);
        set_status("Reopen", &primary_snapshots[0], EGUI_VIEW_TEACHING_TIP_PART_TARGET);
        return;
    }

    set_status("Focus", snapshot, part);
}

static void on_compact_part_changed(egui_view_t *self, uint8_t part)
{
    const egui_view_teaching_tip_snapshot_t *snapshot = &compact_snapshots[egui_view_teaching_tip_get_current_snapshot(self)];
    set_status("Compact", snapshot, part);
}

static void on_guide_click(egui_view_t *self)
{
    uint8_t next_index;

    EGUI_UNUSED(self);
    next_index = (uint8_t)((primary_snapshot_index + 1) % PRIMARY_SNAPSHOT_COUNT);
    if (next_index == PRIMARY_SNAPSHOT_COUNT - 1 && primary_snapshot_index != PRIMARY_SNAPSHOT_COUNT - 1)
    {
        sync_closed_snapshot_from(primary_snapshot_index);
    }
    apply_primary_snapshot(next_index);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_compact_snapshot((uint8_t)(compact_snapshot_index + 1));
}

void test_init_ui(void)
{
    reset_primary_snapshots();

    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), TEACHING_TIP_ROOT_WIDTH, TEACHING_TIP_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), TEACHING_TIP_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), TEACHING_TIP_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(TEACHING_TIP_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 5);
    egui_view_set_clickable(EGUI_VIEW_OF(&guide_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), TEACHING_TIP_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Anchored teaching tip");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(TEACHING_TIP_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_teaching_tip_init(EGUI_VIEW_OF(&tip_primary));
    egui_view_set_size(EGUI_VIEW_OF(&tip_primary), TEACHING_TIP_PRIMARY_WIDTH, TEACHING_TIP_PRIMARY_HEIGHT);
    egui_view_teaching_tip_set_font(EGUI_VIEW_OF(&tip_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_teaching_tip_set_meta_font(EGUI_VIEW_OF(&tip_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&tip_primary), primary_snapshots, PRIMARY_SNAPSHOT_COUNT);
    egui_view_teaching_tip_set_on_part_changed_listener(EGUI_VIEW_OF(&tip_primary), on_primary_part_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&tip_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&tip_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), TEACHING_TIP_STATUS_WIDTH, TEACHING_TIP_STATUS_HEIGHT);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(TEACHING_TIP_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_background(EGUI_VIEW_OF(&status_label), EGUI_BG_OF(&bg_status_accent_panel));
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 137, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xE4EAF1));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), TEACHING_TIP_BOTTOM_ROW_WIDTH, TEACHING_TIP_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&bottom_row), 0, 1, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), TEACHING_TIP_PREVIEW_WIDTH, TEACHING_TIP_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), TEACHING_TIP_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(TEACHING_TIP_COMPACT_LABEL), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_clickable(EGUI_VIEW_OF(&compact_label), true);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_teaching_tip_init(EGUI_VIEW_OF(&tip_compact));
    egui_view_set_size(EGUI_VIEW_OF(&tip_compact), TEACHING_TIP_PREVIEW_WIDTH, TEACHING_TIP_PREVIEW_HEIGHT);
    egui_view_teaching_tip_set_font(EGUI_VIEW_OF(&tip_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_teaching_tip_set_meta_font(EGUI_VIEW_OF(&tip_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&tip_compact), compact_snapshots, COMPACT_SNAPSHOT_COUNT);
    egui_view_teaching_tip_set_compact_mode(EGUI_VIEW_OF(&tip_compact), 1);
    egui_view_teaching_tip_set_on_part_changed_listener(EGUI_VIEW_OF(&tip_compact), on_compact_part_changed);
    egui_view_teaching_tip_set_palette(EGUI_VIEW_OF(&tip_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xD3DDD9), EGUI_COLOR_HEX(0x17302A),
                                       EGUI_COLOR_HEX(0x57756C), EGUI_COLOR_HEX(0x0C8178), EGUI_COLOR_HEX(0x178454), EGUI_COLOR_HEX(0xB77719),
                                       EGUI_COLOR_HEX(0x728091), EGUI_COLOR_HEX(0xD7E0DE));
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&tip_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), TEACHING_TIP_PREVIEW_WIDTH, TEACHING_TIP_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), TEACHING_TIP_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read-only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(TEACHING_TIP_LOCKED_LABEL), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_teaching_tip_init(EGUI_VIEW_OF(&tip_locked));
    egui_view_set_size(EGUI_VIEW_OF(&tip_locked), TEACHING_TIP_PREVIEW_WIDTH, TEACHING_TIP_PREVIEW_HEIGHT);
    egui_view_teaching_tip_set_font(EGUI_VIEW_OF(&tip_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_teaching_tip_set_meta_font(EGUI_VIEW_OF(&tip_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&tip_locked), locked_snapshots, LOCKED_SNAPSHOT_COUNT);
    egui_view_teaching_tip_set_compact_mode(EGUI_VIEW_OF(&tip_locked), 1);
    egui_view_teaching_tip_set_read_only_mode(EGUI_VIEW_OF(&tip_locked), 1);
    egui_view_teaching_tip_set_palette(EGUI_VIEW_OF(&tip_locked), EGUI_COLOR_HEX(0xF8FAFB), EGUI_COLOR_HEX(0xD2DBE3), EGUI_COLOR_HEX(0x4E5F70),
                                       EGUI_COLOR_HEX(0x7D8B99), EGUI_COLOR_HEX(0x8D9CAB), EGUI_COLOR_HEX(0x8D9CAB), EGUI_COLOR_HEX(0x8D9CAB),
                                       EGUI_COLOR_HEX(0x8D9CAB), EGUI_COLOR_HEX(0xDDE5EB));
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&tip_locked));

    apply_primary_snapshot(0);
    apply_compact_snapshot(0);
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&locked_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&tip_primary));
#endif

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
static void apply_primary_key(uint8_t key_code)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&tip_primary));
#endif
    egui_view_teaching_tip_handle_navigation_key(EGUI_VIEW_OF(&tip_primary), key_code);
}

static void set_click_tip_part(egui_sim_action_t *p_action, egui_view_t *tip_view, uint8_t part, int interval_ms)
{
    egui_region_t region;

    if (!egui_view_teaching_tip_get_part_region(tip_view, part, &region))
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
            apply_primary_snapshot(0);
            apply_compact_snapshot(0);
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
        set_click_tip_part(p_action, EGUI_VIEW_OF(&tip_primary), EGUI_VIEW_TEACHING_TIP_PART_TARGET, 120);
        return true;
    case 3:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 4:
        set_click_tip_part(p_action, EGUI_VIEW_OF(&tip_primary), EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, 120);
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
            apply_primary_snapshot(1);
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
        set_click_tip_part(p_action, EGUI_VIEW_OF(&tip_primary), EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, 120);
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
            apply_primary_snapshot(2);
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
        set_click_tip_part(p_action, EGUI_VIEW_OF(&tip_primary), EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, 120);
        return true;
    case 13:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 14:
        set_click_tip_part(p_action, EGUI_VIEW_OF(&tip_primary), EGUI_VIEW_TEACHING_TIP_PART_CLOSE, 120);
        return true;
    case 15:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 16:
        set_click_tip_part(p_action, EGUI_VIEW_OF(&tip_primary), EGUI_VIEW_TEACHING_TIP_PART_TARGET, 120);
        return true;
    case 17:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 18:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_RIGHT);
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 19:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 20:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_RIGHT);
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 21:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 22:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_ESCAPE);
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 23:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 24:
        if (first_call)
        {
            apply_compact_snapshot(1);
        }
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 25:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 26:
        set_click_tip_part(p_action, EGUI_VIEW_OF(&tip_compact), EGUI_VIEW_TEACHING_TIP_PART_TARGET, 120);
        return true;
    case 27:
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
