#include "egui.h"
#include "egui_view_auto_suggest_box.h"
#include "uicode.h"
#include "widget/egui_view_combobox.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define AUTO_SUGGEST_BOX_PRIMARY_WIDTH       208
#define AUTO_SUGGEST_BOX_PRIMARY_HEIGHT      34
#define AUTO_SUGGEST_BOX_COMPACT_WIDTH       104
#define AUTO_SUGGEST_BOX_COMPACT_HEIGHT      28
#define AUTO_SUGGEST_BOX_BOTTOM_ROW_WIDTH    216
#define AUTO_SUGGEST_BOX_BOTTOM_ROW_HEIGHT   72
#define AUTO_SUGGEST_BOX_GUIDE_COLOR         0x6E7B8B
#define AUTO_SUGGEST_BOX_PRIMARY_LABEL_COLOR 0x768392
#define AUTO_SUGGEST_BOX_STATUS_COLOR        0x4D6781
#define AUTO_SUGGEST_BOX_COMPACT_LABEL_COLOR 0x0C756C
#define AUTO_SUGGEST_BOX_LOCKED_LABEL_COLOR  0x8794A2

typedef struct auto_suggest_snapshot auto_suggest_snapshot_t;
struct auto_suggest_snapshot
{
    const char **suggestions;
    const char **statuses;
    uint8_t suggestion_count;
    uint8_t selected_index;
    egui_color_t accent_color;
};

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_autocomplete_t control_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_autocomplete_t control_compact;
static egui_view_linearlayout_t read_only_column;
static egui_view_label_t read_only_label;
static egui_view_autocomplete_t control_read_only;

static uint8_t g_primary_snapshot = 0;
static uint8_t g_compact_snapshot = 0;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "AutoSuggest Box";
static const char *guide_text = "Tap guide to cycle datasets";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {143, 0}};

static const char *primary_suggestions_0[] = {"Alice Chen", "Alicia Gomez", "Allen Park", "Amelia Stone"};
static const char *primary_status_0[] = {
        "Suggested teammate: Alice Chen",
        "Suggested teammate: Alicia Gomez",
        "Suggested teammate: Allen Park",
        "Suggested teammate: Amelia Stone",
};

static const char *primary_suggestions_1[] = {"Deploy API", "Deploy Docs", "Deploy Worker"};
static const char *primary_status_1[] = {
        "Command suggestion: Deploy API",
        "Command suggestion: Deploy Docs",
        "Command suggestion: Deploy Worker",
};

static const char *primary_suggestions_2[] = {"Daily Standup", "Demo Sync", "Design Review", "Docs Review"};
static const char *primary_status_2[] = {
        "Meeting suggestion: Daily Standup",
        "Meeting suggestion: Demo Sync",
        "Meeting suggestion: Design Review",
        "Meeting suggestion: Docs Review",
};

static const auto_suggest_snapshot_t primary_snapshots[] = {
        {primary_suggestions_0, primary_status_0, 4, 1, EGUI_COLOR_HEX(0x2563EB)},
        {primary_suggestions_1, primary_status_1, 3, 0, EGUI_COLOR_HEX(0x2563EB)},
        {primary_suggestions_2, primary_status_2, 4, 2, EGUI_COLOR_HEX(0x2563EB)},
};

static const char *compact_suggestions_0[] = {"Recent", "Pinned"};
static const char *compact_status_0[] = {
        "Compact suggestion: Recent",
        "Compact suggestion: Pinned",
};

static const char *compact_suggestions_1[] = {"Manual", "Auto"};
static const char *compact_status_1[] = {
        "Compact mode: Manual",
        "Compact mode: Auto",
};

static const auto_suggest_snapshot_t compact_snapshots[] = {
        {compact_suggestions_0, compact_status_0, 2, 0, EGUI_COLOR_HEX(0x0F766E)},
        {compact_suggestions_1, compact_status_1, 2, 1, EGUI_COLOR_HEX(0x0F766E)},
};

static const char *read_only_suggestions[] = {"Pinned", "Recent", "Saved"};

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x536677), 28), EGUI_ALPHA_100);
}

static void apply_primary_snapshot(uint8_t index, uint8_t update_status)
{
    const auto_suggest_snapshot_t *snapshot = &primary_snapshots[index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]))];

    g_primary_snapshot = index % (sizeof(primary_snapshots) / sizeof(primary_snapshots[0]));
    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&control_primary), snapshot->suggestions, snapshot->suggestion_count);
    egui_view_autocomplete_set_current_index(EGUI_VIEW_OF(&control_primary), snapshot->selected_index);
    egui_view_autocomplete_collapse(EGUI_VIEW_OF(&control_primary));

    if (update_status && snapshot->selected_index < snapshot->suggestion_count)
    {
        set_status(snapshot->statuses[snapshot->selected_index], snapshot->accent_color);
    }
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    const auto_suggest_snapshot_t *snapshot = &compact_snapshots[index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]))];

    g_compact_snapshot = index % (sizeof(compact_snapshots) / sizeof(compact_snapshots[0]));
    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&control_compact), snapshot->suggestions, snapshot->suggestion_count);
    egui_view_autocomplete_set_current_index(EGUI_VIEW_OF(&control_compact), snapshot->selected_index);
    egui_view_autocomplete_collapse(EGUI_VIEW_OF(&control_compact));

    if (update_status && snapshot->selected_index < snapshot->suggestion_count)
    {
        set_status(snapshot->statuses[snapshot->selected_index], snapshot->accent_color);
    }
}

static void on_primary_selected(egui_view_t *self, uint8_t index)
{
    const auto_suggest_snapshot_t *snapshot = &primary_snapshots[g_primary_snapshot];

    EGUI_UNUSED(self);
    if (index < snapshot->suggestion_count)
    {
        set_status(snapshot->statuses[index], snapshot->accent_color);
    }
}

static void on_compact_selected(egui_view_t *self, uint8_t index)
{
    const auto_suggest_snapshot_t *snapshot = &compact_snapshots[g_compact_snapshot];

    EGUI_UNUSED(self);
    if (index < snapshot->suggestion_count)
    {
        set_status(snapshot->statuses[index], snapshot->accent_color);
    }
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
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 280);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(AUTO_SUGGEST_BOX_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), 224, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(AUTO_SUGGEST_BOX_PRIMARY_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_autocomplete_init(EGUI_VIEW_OF(&control_primary));
    egui_view_set_size(EGUI_VIEW_OF(&control_primary), AUTO_SUGGEST_BOX_PRIMARY_WIDTH, AUTO_SUGGEST_BOX_PRIMARY_HEIGHT);
    egui_view_autocomplete_set_font(EGUI_VIEW_OF(&control_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_autocomplete_set_on_selected_listener(EGUI_VIEW_OF(&control_primary), on_primary_selected);
    hcw_auto_suggest_box_apply_standard_style(EGUI_VIEW_OF(&control_primary));
    egui_view_set_margin(EGUI_VIEW_OF(&control_primary), 0, 0, 0, 6);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&control_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Suggested teammate: Alicia Gomez");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(AUTO_SUGGEST_BOX_STATUS_COLOR), EGUI_ALPHA_100);
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
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), AUTO_SUGGEST_BOX_BOTTOM_ROW_WIDTH, AUTO_SUGGEST_BOX_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), AUTO_SUGGEST_BOX_COMPACT_WIDTH, AUTO_SUGGEST_BOX_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), AUTO_SUGGEST_BOX_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(AUTO_SUGGEST_BOX_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_autocomplete_init(EGUI_VIEW_OF(&control_compact));
    egui_view_set_size(EGUI_VIEW_OF(&control_compact), AUTO_SUGGEST_BOX_COMPACT_WIDTH, AUTO_SUGGEST_BOX_COMPACT_HEIGHT);
    egui_view_autocomplete_set_font(EGUI_VIEW_OF(&control_compact), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_autocomplete_set_on_selected_listener(EGUI_VIEW_OF(&control_compact), on_compact_selected);
    hcw_auto_suggest_box_apply_compact_style(EGUI_VIEW_OF(&control_compact));
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&control_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&read_only_column));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_column), AUTO_SUGGEST_BOX_COMPACT_WIDTH, AUTO_SUGGEST_BOX_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&read_only_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&read_only_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&read_only_column));

    egui_view_label_init(EGUI_VIEW_OF(&read_only_label));
    egui_view_set_size(EGUI_VIEW_OF(&read_only_label), AUTO_SUGGEST_BOX_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&read_only_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&read_only_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&read_only_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&read_only_label), EGUI_COLOR_HEX(AUTO_SUGGEST_BOX_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&read_only_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&read_only_label));

    egui_view_autocomplete_init(EGUI_VIEW_OF(&control_read_only));
    egui_view_set_size(EGUI_VIEW_OF(&control_read_only), AUTO_SUGGEST_BOX_COMPACT_WIDTH, AUTO_SUGGEST_BOX_COMPACT_HEIGHT);
    egui_view_autocomplete_set_font(EGUI_VIEW_OF(&control_read_only), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    hcw_auto_suggest_box_apply_read_only_style(EGUI_VIEW_OF(&control_read_only));
    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&control_read_only), read_only_suggestions, 3);
    egui_view_autocomplete_set_current_index(EGUI_VIEW_OF(&control_read_only), 1);
    egui_view_set_enable(EGUI_VIEW_OF(&control_read_only), false);
    egui_view_group_add_child(EGUI_VIEW_OF(&read_only_column), EGUI_VIEW_OF(&control_read_only));

    apply_primary_snapshot(0, 1);
    apply_compact_snapshot(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&read_only_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_request_focus(EGUI_VIEW_OF(&control_primary));
#endif
}

#if EGUI_CONFIG_RECORDING_TEST
static void get_view_center(egui_view_t *view, int *x, int *y)
{
    *x = view->region_screen.location.x + view->region_screen.size.width / 2;
    *y = view->region_screen.location.y + view->region_screen.size.height / 2;
}

static uint8_t get_dropdown_visible_count(egui_view_t *view)
{
    egui_view_combobox_t *local = (egui_view_combobox_t *)view;
    uint8_t visible_count = local->item_count;

    if (visible_count > local->max_visible_items)
    {
        visible_count = local->max_visible_items;
    }
    if (view->region_screen.size.height > local->collapsed_height && local->item_height > 0)
    {
        egui_dim_t item_space = view->region_screen.size.height - local->collapsed_height;
        uint8_t fit_count = (uint8_t)(item_space / local->item_height);

        if (fit_count < visible_count)
        {
            visible_count = fit_count;
        }
    }

    return visible_count;
}

static uint8_t get_dropdown_visible_start_index(const egui_view_combobox_t *local, uint8_t visible_count)
{
    uint8_t start_index = 0;

    if (visible_count == 0 || local->item_count <= visible_count)
    {
        return 0;
    }
    if (local->current_index >= visible_count)
    {
        start_index = (uint8_t)(local->current_index + 1 - visible_count);
    }
    if ((uint16_t)start_index + visible_count > local->item_count)
    {
        start_index = (uint8_t)(local->item_count - visible_count);
    }

    return start_index;
}

static void get_dropdown_item_center(egui_view_t *view, uint8_t item_index, int *x, int *y)
{
    egui_view_combobox_t *local = (egui_view_combobox_t *)view;
    uint8_t visible_count = get_dropdown_visible_count(view);
    uint8_t start_index;
    uint8_t row_index;

    if (visible_count == 0)
    {
        get_view_center(view, x, y);
        return;
    }
    start_index = get_dropdown_visible_start_index(local, visible_count);

    if (item_index < start_index)
    {
        row_index = 0;
    }
    else if (item_index >= (uint8_t)(start_index + visible_count))
    {
        row_index = (uint8_t)(visible_count - 1);
    }
    else
    {
        row_index = (uint8_t)(item_index - start_index);
    }

    *x = view->region_screen.location.x + view->region_screen.size.width / 2;
    *y = view->region_screen.location.y + local->collapsed_height + row_index * local->item_height + local->item_height / 2;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;
    int x = 0;
    int y = 0;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            apply_primary_snapshot(0, 1);
            apply_compact_snapshot(0, 0);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
            egui_view_request_focus(EGUI_VIEW_OF(&control_primary));
#endif
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 1:
        get_view_center(EGUI_VIEW_OF(&control_primary), &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 280;
        return true;
    case 2:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 3:
        get_dropdown_item_center(EGUI_VIEW_OF(&control_primary), 2, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 280;
        return true;
    case 4:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 5:
        get_view_center(EGUI_VIEW_OF(&guide_label), &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 280;
        return true;
    case 6:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 7:
        get_view_center(EGUI_VIEW_OF(&control_primary), &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 280;
        return true;
    case 8:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 9:
        get_dropdown_item_center(EGUI_VIEW_OF(&control_primary), 1, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 280;
        return true;
    case 10:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 11:
        get_view_center(EGUI_VIEW_OF(&compact_label), &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 280;
        return true;
    case 12:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 13:
        get_view_center(EGUI_VIEW_OF(&control_compact), &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 280;
        return true;
    case 14:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 15:
        get_dropdown_item_center(EGUI_VIEW_OF(&control_compact), 1, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 280;
        return true;
    case 16:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 17:
        if (first_call)
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
            egui_view_request_focus(EGUI_VIEW_OF(&control_primary));
#endif
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 640);
        return true;
    default:
        return false;
    }
}
#endif
