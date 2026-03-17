#include <string.h>

#include "egui.h"
#include "egui_view_token_input.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define TOKEN_PRIMARY_WIDTH       196
#define TOKEN_PRIMARY_HEIGHT      92
#define TOKEN_COMPACT_WIDTH       106
#define TOKEN_COMPACT_HEIGHT      48
#define TOKEN_BOTTOM_ROW_WIDTH    216
#define TOKEN_BOTTOM_ROW_HEIGHT   70
#define TOKEN_GUIDE_COLOR         0x6F7C8B
#define TOKEN_LABEL_COLOR         0x788492
#define TOKEN_STATUS_COLOR        0x5B6D81
#define TOKEN_COMPACT_LABEL_COLOR 0x936D18
#define TOKEN_LOCKED_LABEL_COLOR  0x8894A2
#define TOKEN_STANDARD_ACCENT     0x4E84D6
#define TOKEN_STANDARD_BORDER     0xD7DFE7
#define TOKEN_STANDARD_SHADOW     0xDBE3EC
#define TOKEN_COMPACT_ACCENT      0xC78C16
#define TOKEN_COMPACT_BORDER      0xDADFE5
#define TOKEN_LOCKED_ACCENT       0x9DA9B5
#define TOKEN_LOCKED_BORDER       0xD9E1E8
#define TOKEN_RECORD_STEP_WAIT    90
#define TOKEN_RECORD_FRAME_WAIT   130
#define TOKEN_RECORD_FINAL_WAIT   320

typedef struct token_demo_snapshot token_demo_snapshot_t;
struct token_demo_snapshot
{
    const char *status_prefix;
    const char *placeholder;
    const char **tokens;
    uint8_t token_count;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_token_input_t editor_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_token_input_t editor_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_token_input_t editor_locked;

static uint8_t g_primary_snapshot = 0;
static uint8_t g_compact_snapshot = 0;
static uint8_t g_ignore_status_change = 0;
static char status_text[64] = "Recipients 3";

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Token Input";
static const char *guide_text = "Tap labels to switch presets";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {143, 0}};

static const char *primary_tokens_0[] = {"Alice", "Ops", "QA"};
static const char *primary_tokens_1[] = {"Design", "Demo", "VIP", "Build"};
static const char *compact_tokens_0[] = {"AL", "BO"};
static const char *compact_tokens_1[] = {"UI", "QA", "OPS", "SYS", "NET"};
static const char *locked_tokens[] = {"Audit", "Ops", "QA", "Net", "Sys"};

static const token_demo_snapshot_t primary_snapshots[] = {
        {"Recipients", "Type name", primary_tokens_0, 3},
        {"Tags", "Add tag", primary_tokens_1, 4},
};

static const token_demo_snapshot_t compact_snapshots[] = {
        {"Compact", "Add", compact_tokens_0, 2},
        {"Compact", "Add", compact_tokens_1, 5},
};

static void update_status_line(const token_demo_snapshot_t *snapshot, egui_view_t *view, egui_color_t color)
{
    const char *draft = egui_view_token_input_get_draft_text(view);
    uint8_t token_count = egui_view_token_input_get_token_count(view);
    int pos = 0;

    if (snapshot == NULL)
    {
        return;
    }

    pos += egui_sprintf_str(status_text, sizeof(status_text), snapshot->status_prefix);
    pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
    pos += egui_sprintf_int(&status_text[pos], (int)sizeof(status_text) - pos, token_count);
    if (draft != NULL && draft[0] != '\0')
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " / ");
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, draft);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(TOKEN_STATUS_COLOR), 28), EGUI_ALPHA_100);
}

static void apply_snapshot_to_editor(egui_view_t *view, const token_demo_snapshot_t *snapshot)
{
    egui_view_token_input_set_placeholder(view, snapshot->placeholder);
    egui_view_token_input_set_tokens(view, snapshot->tokens, snapshot->token_count);
    egui_view_token_input_set_current_part(view, EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
}

static void apply_primary_snapshot(uint8_t index, uint8_t update_status)
{
    const token_demo_snapshot_t *snapshot = &primary_snapshots[index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]))];

    g_primary_snapshot = index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]));
    g_ignore_status_change = 1;
    apply_snapshot_to_editor(EGUI_VIEW_OF(&editor_primary), snapshot);
    g_ignore_status_change = 0;
    if (update_status)
    {
        update_status_line(snapshot, EGUI_VIEW_OF(&editor_primary), EGUI_COLOR_HEX(TOKEN_STANDARD_ACCENT));
    }
}

static void apply_compact_snapshot(uint8_t index)
{
    const token_demo_snapshot_t *snapshot = &compact_snapshots[index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]))];

    g_compact_snapshot = index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]));
    apply_snapshot_to_editor(EGUI_VIEW_OF(&editor_compact), snapshot);
}

static void on_primary_changed(egui_view_t *self, uint8_t token_count, uint8_t part)
{
    EGUI_UNUSED(token_count);
    EGUI_UNUSED(part);
    if (g_ignore_status_change)
    {
        return;
    }
    update_status_line(&primary_snapshots[g_primary_snapshot], self, EGUI_COLOR_HEX(TOKEN_STANDARD_ACCENT));
}

static void on_guide_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_primary_snapshot((uint8_t)(g_primary_snapshot + 1), 1);
}

static void on_compact_label_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    apply_compact_snapshot((uint8_t)(g_compact_snapshot + 1));
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 278);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(TOKEN_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), 224, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(TOKEN_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 1);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_token_input_init(EGUI_VIEW_OF(&editor_primary));
    egui_view_set_size(EGUI_VIEW_OF(&editor_primary), TOKEN_PRIMARY_WIDTH, TOKEN_PRIMARY_HEIGHT);
    egui_view_token_input_set_font(EGUI_VIEW_OF(&editor_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_token_input_set_meta_font(EGUI_VIEW_OF(&editor_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_token_input_set_palette(EGUI_VIEW_OF(&editor_primary), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(TOKEN_STANDARD_BORDER), EGUI_COLOR_HEX(0x1E2832),
                                      EGUI_COLOR_HEX(0x71808F), EGUI_COLOR_HEX(TOKEN_STANDARD_ACCENT), EGUI_COLOR_HEX(TOKEN_STANDARD_SHADOW));
    egui_view_token_input_set_on_changed_listener(EGUI_VIEW_OF(&editor_primary), on_primary_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&editor_primary), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&editor_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(TOKEN_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 144, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDEE4EB));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), TOKEN_BOTTOM_ROW_WIDTH, TOKEN_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), TOKEN_COMPACT_WIDTH, TOKEN_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), TOKEN_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(TOKEN_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_token_input_init(EGUI_VIEW_OF(&editor_compact));
    egui_view_set_size(EGUI_VIEW_OF(&editor_compact), TOKEN_COMPACT_WIDTH, TOKEN_COMPACT_HEIGHT);
    egui_view_token_input_set_font(EGUI_VIEW_OF(&editor_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_token_input_set_meta_font(EGUI_VIEW_OF(&editor_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&editor_compact), 1);
    egui_view_token_input_set_palette(EGUI_VIEW_OF(&editor_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(TOKEN_COMPACT_BORDER), EGUI_COLOR_HEX(0x2B3138),
                                      EGUI_COLOR_HEX(0x74808C), EGUI_COLOR_HEX(TOKEN_COMPACT_ACCENT), EGUI_COLOR_HEX(0xE0E6EC));
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&editor_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), TOKEN_COMPACT_WIDTH, TOKEN_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), TOKEN_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(TOKEN_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_token_input_init(EGUI_VIEW_OF(&editor_locked));
    egui_view_set_size(EGUI_VIEW_OF(&editor_locked), TOKEN_COMPACT_WIDTH, TOKEN_COMPACT_HEIGHT);
    egui_view_token_input_set_font(EGUI_VIEW_OF(&editor_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_token_input_set_meta_font(EGUI_VIEW_OF(&editor_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&editor_locked), 1);
    egui_view_token_input_set_read_only_mode(EGUI_VIEW_OF(&editor_locked), 1);
    egui_view_token_input_set_palette(EGUI_VIEW_OF(&editor_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(TOKEN_LOCKED_BORDER), EGUI_COLOR_HEX(0x596878),
                                      EGUI_COLOR_HEX(0x8C98A5), EGUI_COLOR_HEX(TOKEN_LOCKED_ACCENT), EGUI_COLOR_HEX(0xE3E9EF));
    egui_view_token_input_set_tokens(EGUI_VIEW_OF(&editor_locked), locked_tokens, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&editor_locked));

    apply_primary_snapshot(0, 1);
    apply_compact_snapshot(0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&locked_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&editor_primary));
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
    egui_key_event_t event;

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&editor_primary));
#endif
    memset(&event, 0, sizeof(event));
    event.key_code = key_code;
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    EGUI_VIEW_OF(&editor_primary)->api->on_key_event(EGUI_VIEW_OF(&editor_primary), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    EGUI_VIEW_OF(&editor_primary)->api->on_key_event(EGUI_VIEW_OF(&editor_primary), &event);
}

static void set_click_token_part(egui_sim_action_t *p_action, egui_view_t *view, uint8_t part, int interval_ms)
{
    egui_region_t region;

    if (!egui_view_token_input_get_part_region(view, part, &region))
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

static void set_click_token_remove(egui_sim_action_t *p_action, egui_view_t *view, uint8_t index, int interval_ms)
{
    egui_region_t region;

    if (!egui_view_token_input_get_remove_region(view, index, &region))
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
            apply_primary_snapshot(0, 1);
            apply_compact_snapshot(0);
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_STEP_WAIT);
        return true;
    case 1:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_FRAME_WAIT);
        return true;
    case 2:
        set_click_token_part(p_action, EGUI_VIEW_OF(&editor_primary), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, TOKEN_RECORD_STEP_WAIT);
        return true;
    case 3:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_FRAME_WAIT);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_N);
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_STEP_WAIT);
        return true;
    case 5:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_E);
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_STEP_WAIT);
        return true;
    case 6:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_T);
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_STEP_WAIT);
        return true;
    case 7:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_ENTER);
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_STEP_WAIT);
        return true;
    case 8:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_FRAME_WAIT);
        return true;
    case 9:
        set_click_token_remove(p_action, EGUI_VIEW_OF(&editor_primary), 3, TOKEN_RECORD_STEP_WAIT);
        return true;
    case 10:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_FRAME_WAIT);
        return true;
    case 11:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_HOME);
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_STEP_WAIT);
        return true;
    case 12:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_FRAME_WAIT);
        return true;
    case 13:
        if (first_call)
        {
            apply_primary_key(EGUI_KEY_CODE_END);
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_STEP_WAIT);
        return true;
    case 14:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_FRAME_WAIT);
        return true;
    case 15:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&guide_label), TOKEN_RECORD_STEP_WAIT);
        return true;
    case 16:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_FRAME_WAIT);
        return true;
    case 17:
        EGUI_SIM_SET_CLICK_VIEW(p_action, EGUI_VIEW_OF(&compact_label), TOKEN_RECORD_STEP_WAIT);
        return true;
    case 18:
        if (first_call)
        {
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, TOKEN_RECORD_FINAL_WAIT);
        return true;
    default:
        return false;
    }
}
#endif
