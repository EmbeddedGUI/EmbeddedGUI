#include <stdio.h>

#include "egui.h"
#include "egui_view_shortcut_recorder.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define SHORTCUT_ROOT_WIDTH     224
#define SHORTCUT_ROOT_HEIGHT    304
#define SHORTCUT_PRIMARY_WIDTH  194
#define SHORTCUT_PRIMARY_HEIGHT 138
#define SHORTCUT_PREVIEW_WIDTH  106
#define SHORTCUT_PREVIEW_HEIGHT 82
#define SHORTCUT_BOTTOM_WIDTH   218
#define SHORTCUT_BOTTOM_HEIGHT  92
#define SHORTCUT_GUIDE_COLOR    0x7D8A96
#define SHORTCUT_LABEL_COLOR    0x72808D
#define SHORTCUT_STATUS_COLOR   0x677685
#define SHORTCUT_COMPACT_COLOR  0x0F766E
#define SHORTCUT_READONLY_COLOR 0x8A96A3

typedef struct shortcut_scene shortcut_scene_t;
struct shortcut_scene
{
    const char *status_prefix;
    egui_color_t status_color;
    uint8_t has_binding;
    uint8_t key_code;
    uint8_t is_shift;
    uint8_t is_ctrl;
    uint8_t listening;
    uint8_t current_part;
    uint8_t current_preset;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_shortcut_recorder_t recorder_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_shortcut_recorder_t recorder_compact;
static egui_view_linearlayout_t readonly_column;
static egui_view_label_t readonly_label;
static egui_view_shortcut_recorder_t recorder_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Shortcut Recorder";
static const char *guide_text = "Tap field or press Enter";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {148, 0}};
static char status_text[72] = "Ready: Ctrl + K";

static const egui_view_shortcut_recorder_preset_t primary_presets[] = {
        {"Search files", "Workspace", EGUI_KEY_CODE_F, 1, 1},
        {"Command bar", "Quick command", EGUI_KEY_CODE_P, 1, 1},
        {"Pin focus", "One tap", EGUI_KEY_CODE_1, 0, 1},
};

static const egui_view_shortcut_recorder_preset_t compact_presets[] = {
        {"Review", "Preset", EGUI_KEY_CODE_P, 1, 1},
        {"Queue", "Preset", EGUI_KEY_CODE_1, 0, 1},
};

static const shortcut_scene_t primary_scenes[] = {
        {"Ready", EGUI_COLOR_HEX(0x2563EB), 1, EGUI_KEY_CODE_K, 0, 1, 0, EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 0},
        {"Listening", EGUI_COLOR_HEX(0xD97706), 1, EGUI_KEY_CODE_K, 0, 1, 1, EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 0},
        {"Preset", EGUI_COLOR_HEX(0x7C3AED), 1, EGUI_KEY_CODE_P, 1, 1, 0, EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET, 1},
        {"Preset", EGUI_COLOR_HEX(0x0F766E), 1, EGUI_KEY_CODE_1, 0, 1, 0, EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET, 2},
        {"Cleared", EGUI_COLOR_HEX(0xBE5168), 0, EGUI_KEY_CODE_NONE, 0, 0, 0, EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 2},
};

static const shortcut_scene_t compact_scenes[] = {
        {"Compact", EGUI_COLOR_HEX(0x0F766E), 1, EGUI_KEY_CODE_P, 1, 1, 0, EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 0},
        {"Compact", EGUI_COLOR_HEX(0x2563EB), 1, EGUI_KEY_CODE_1, 0, 1, 0, EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 1},
};

static uint8_t current_primary_scene = 0;
static uint8_t current_compact_scene = 0;

static void set_status_from_binding(const char *prefix, egui_view_shortcut_recorder_t *recorder, egui_color_t color)
{
    char binding_text[32];

    egui_view_shortcut_recorder_get_binding_text(EGUI_VIEW_OF(recorder), binding_text, sizeof(binding_text));
    if (egui_view_shortcut_recorder_is_listening(EGUI_VIEW_OF(recorder)))
    {
        snprintf(status_text, sizeof(status_text), "%s: Press a key", prefix);
    }
    else
    {
        snprintf(status_text, sizeof(status_text), "%s: %s", prefix, binding_text);
    }
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void apply_scene_to_recorder(egui_view_shortcut_recorder_t *recorder, const shortcut_scene_t *scene)
{
    if (scene->has_binding)
    {
        egui_view_shortcut_recorder_set_binding(EGUI_VIEW_OF(recorder), scene->key_code, scene->is_shift, scene->is_ctrl);
    }
    else
    {
        egui_view_shortcut_recorder_clear_binding(EGUI_VIEW_OF(recorder));
    }
    egui_view_shortcut_recorder_set_current_preset(EGUI_VIEW_OF(recorder), scene->current_preset);
    egui_view_shortcut_recorder_set_current_part(EGUI_VIEW_OF(recorder), scene->current_part);
    egui_view_shortcut_recorder_set_listening(EGUI_VIEW_OF(recorder), scene->listening);
}

static void apply_primary_scene(uint8_t scene_index)
{
    current_primary_scene = scene_index;
    apply_scene_to_recorder(&recorder_primary, &primary_scenes[scene_index]);
    set_status_from_binding(primary_scenes[scene_index].status_prefix, &recorder_primary, primary_scenes[scene_index].status_color);
}

static void apply_compact_scene(uint8_t scene_index, uint8_t update_status)
{
    current_compact_scene = scene_index;
    apply_scene_to_recorder(&recorder_compact, &compact_scenes[scene_index]);
    if (update_status)
    {
        set_status_from_binding(compact_scenes[scene_index].status_prefix, &recorder_compact, compact_scenes[scene_index].status_color);
    }
}

static void on_primary_changed(egui_view_t *self, uint8_t part, uint8_t preset_index)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(part);
    EGUI_UNUSED(preset_index);
    set_status_from_binding("Primary", &recorder_primary, EGUI_COLOR_HEX(0x2563EB));
}

static void on_guide_click(egui_view_t *self)
{
    uint8_t next = (uint8_t)((current_primary_scene + 1) % 4);

    EGUI_UNUSED(self);
    apply_primary_scene(next);
}

static void on_compact_label_click(egui_view_t *self)
{
    uint8_t next = (uint8_t)((current_compact_scene + 1) % 2);

    EGUI_UNUSED(self);
    apply_compact_scene(next, 1);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), SHORTCUT_ROOT_WIDTH, SHORTCUT_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), SHORTCUT_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x22313F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 8, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), SHORTCUT_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(SHORTCUT_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), SHORTCUT_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(SHORTCUT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_shortcut_recorder_init(EGUI_VIEW_OF(&recorder_primary));
    egui_view_set_size(EGUI_VIEW_OF(&recorder_primary), SHORTCUT_PRIMARY_WIDTH, SHORTCUT_PRIMARY_HEIGHT);
    egui_view_shortcut_recorder_set_font(EGUI_VIEW_OF(&recorder_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_shortcut_recorder_set_meta_font(EGUI_VIEW_OF(&recorder_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_shortcut_recorder_set_header(EGUI_VIEW_OF(&recorder_primary), "Quick launch", "Capture a shortcut", "Ready to capture");
    egui_view_shortcut_recorder_set_presets(EGUI_VIEW_OF(&recorder_primary), primary_presets, 3);
    egui_view_shortcut_recorder_set_on_changed_listener(EGUI_VIEW_OF(&recorder_primary), on_primary_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&recorder_primary), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&recorder_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), SHORTCUT_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(SHORTCUT_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 148, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDCE5EC));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 5);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), SHORTCUT_BOTTOM_WIDTH, SHORTCUT_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_HCENTER);

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), SHORTCUT_PREVIEW_WIDTH, SHORTCUT_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), SHORTCUT_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(SHORTCUT_COMPACT_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_shortcut_recorder_init(EGUI_VIEW_OF(&recorder_compact));
    egui_view_set_size(EGUI_VIEW_OF(&recorder_compact), SHORTCUT_PREVIEW_WIDTH, SHORTCUT_PREVIEW_HEIGHT);
    egui_view_shortcut_recorder_set_font(EGUI_VIEW_OF(&recorder_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_shortcut_recorder_set_meta_font(EGUI_VIEW_OF(&recorder_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_shortcut_recorder_set_header(EGUI_VIEW_OF(&recorder_compact), "Compact", "Preset preview", "Peek");
    egui_view_shortcut_recorder_set_presets(EGUI_VIEW_OF(&recorder_compact), compact_presets, 2);
    egui_view_shortcut_recorder_set_compact_mode(EGUI_VIEW_OF(&recorder_compact), 1);
    egui_view_shortcut_recorder_set_palette(EGUI_VIEW_OF(&recorder_compact), EGUI_COLOR_HEX(0xFCFFFE), EGUI_COLOR_HEX(0xCBE4DE), EGUI_COLOR_HEX(0x12463F),
                                            EGUI_COLOR_HEX(0x5B7D77), EGUI_COLOR_HEX(0x0F766E), EGUI_COLOR_HEX(0xD97706), EGUI_COLOR_HEX(0x0F766E),
                                            EGUI_COLOR_HEX(0xBE5168));
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&recorder_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&readonly_column));
    egui_view_set_size(EGUI_VIEW_OF(&readonly_column), SHORTCUT_PREVIEW_WIDTH, SHORTCUT_BOTTOM_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&readonly_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&readonly_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&readonly_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&readonly_column));

    egui_view_label_init(EGUI_VIEW_OF(&readonly_label));
    egui_view_set_size(EGUI_VIEW_OF(&readonly_label), SHORTCUT_PREVIEW_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&readonly_label), "Read-only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&readonly_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&readonly_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&readonly_label), EGUI_COLOR_HEX(SHORTCUT_READONLY_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&readonly_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&readonly_column), EGUI_VIEW_OF(&readonly_label));

    egui_view_shortcut_recorder_init(EGUI_VIEW_OF(&recorder_locked));
    egui_view_set_size(EGUI_VIEW_OF(&recorder_locked), SHORTCUT_PREVIEW_WIDTH, SHORTCUT_PREVIEW_HEIGHT);
    egui_view_shortcut_recorder_set_font(EGUI_VIEW_OF(&recorder_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_shortcut_recorder_set_meta_font(EGUI_VIEW_OF(&recorder_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_shortcut_recorder_set_header(EGUI_VIEW_OF(&recorder_locked), "Read only", "Locked binding", "Locked");
    egui_view_shortcut_recorder_set_read_only_mode(EGUI_VIEW_OF(&recorder_locked), 1);
    egui_view_shortcut_recorder_set_binding(EGUI_VIEW_OF(&recorder_locked), EGUI_KEY_CODE_S, 0, 1);
    egui_view_shortcut_recorder_set_palette(EGUI_VIEW_OF(&recorder_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xD7DFE6), EGUI_COLOR_HEX(0x54616D),
                                            EGUI_COLOR_HEX(0x8A97A3), EGUI_COLOR_HEX(0x93A3B4), EGUI_COLOR_HEX(0xD97706), EGUI_COLOR_HEX(0x93A3B4),
                                            EGUI_COLOR_HEX(0xBE5168));
    egui_view_group_add_child(EGUI_VIEW_OF(&readonly_column), EGUI_VIEW_OF(&recorder_locked));

    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    apply_primary_scene(0);
    apply_compact_scene(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&readonly_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_invalidate(EGUI_VIEW_OF(&root_layout));
#if EGUI_CONFIG_RECORDING_TEST
    recording_request_snapshot();
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&recorder_primary));
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
            apply_primary_scene(0);
            apply_compact_scene(0, 0);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 1:
        if (first_call)
        {
            apply_primary_scene(1);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_scene(2);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 3:
        if (first_call)
        {
            apply_primary_scene(3);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_scene(4);
            apply_compact_scene(1, 0);
            request_page_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 520);
        return true;
    default:
        return false;
    }
}
#endif
