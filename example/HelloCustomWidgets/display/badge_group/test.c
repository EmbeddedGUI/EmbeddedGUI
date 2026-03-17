#include "egui.h"
#include "egui_view_badge_group.h"
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define BADGE_GROUP_PRIMARY_WIDTH  196
#define BADGE_GROUP_PRIMARY_HEIGHT 118
#define BADGE_GROUP_BOTTOM_WIDTH   212
#define BADGE_GROUP_BOTTOM_HEIGHT  98
#define BADGE_GROUP_COLUMN_WIDTH   104
#define BADGE_GROUP_CARD_HEIGHT    84

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_badge_group_t group_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_badge_group_t group_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_badge_group_t group_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Badge Group";
static const char *guide_text = "Tap groups to review states";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {147, 0}};

static const egui_view_badge_group_item_t primary_items_0[] = {
        {"Review", "4", 0, 1, 0},
        {"Ready", "12", 1, 1, 0},
        {"Risk", "1", 2, 0, 1},
        {"Archive", "7", 3, 0, 1},
};

static const egui_view_badge_group_item_t primary_items_1[] = {
        {"Online", "8", 1, 1, 0},
        {"Shadow", "2", 3, 0, 1},
        {"Sync", "3", 0, 0, 1},
        {"Alert", "1", 2, 1, 0},
};

static const egui_view_badge_group_item_t primary_items_2[] = {
        {"Queued", "5", 3, 0, 1},
        {"Hold", "2", 2, 1, 0},
        {"Owner", "A", 0, 1, 0},
        {"Done", "9", 1, 0, 1},
};

static const egui_view_badge_group_item_t primary_items_3[] = {
        {"Pinned", "6", 0, 0, 1},
        {"Calm", "3", 3, 1, 0},
        {"Watch", "2", 2, 0, 1},
        {"Live", "4", 1, 0, 1},
};

static const egui_view_badge_group_item_t compact_items_0[] = {
        {"Ready", "8", 0, 1, 0},
        {"Muted", "2", 3, 0, 1},
};

static const egui_view_badge_group_item_t compact_items_1[] = {
        {"Hold", "1", 2, 1, 0},
        {"QA", "6", 0, 0, 1},
};

static const egui_view_badge_group_item_t locked_items_0[] = {
        {"Pinned", "4", 3, 0, 1},
        {"Review", "1", 0, 0, 1},
};

static const egui_view_badge_group_item_t locked_items_1[] = {
        {"Quiet", "3", 3, 1, 0},
        {"Ready", "2", 1, 0, 1},
};

static const egui_view_badge_group_snapshot_t primary_snapshots[] = {
        {"TRIAGE", "Release lanes", "Mixed badges stay aligned.", "Summary follows focus.", primary_items_0, 4, 0},
        {"QUEUE", "Ops handoff", "Success tone leads the row.", "Success drives footer.", primary_items_1, 4, 0},
        {"RISK", "Change review", "Warning focus stays visible.", "Warning stays visible.", primary_items_2, 4, 1},
        {"CALM", "Archive sweep", "Neutral focus softens the card.", "Neutral stays calm.", primary_items_3, 4, 1},
};

static const egui_view_badge_group_snapshot_t compact_snapshots[] = {
        {"SET", "Compact", "", "Short row", compact_items_0, 2, 0},
        {"HOLD", "Compact", "", "Warn focus", compact_items_1, 2, 0},
};

static const egui_view_badge_group_snapshot_t locked_snapshots[] = {
        {"LOCK", "Read only", "", "Muted preview.", locked_items_0, 2, 0},
        {"LOCK", "Read only", "", "Passive", locked_items_1, 2, 0},
};

static egui_color_t badge_group_status_color(uint8_t tone)
{
    switch (tone)
    {
    case 1:
        return EGUI_COLOR_HEX(0x177A43);
    case 2:
        return EGUI_COLOR_HEX(0xA96E0F);
    case 3:
        return EGUI_COLOR_HEX(0x6A7480);
    default:
        return EGUI_COLOR_HEX(0x265FC8);
    }
}

static const char *badge_group_status_text(uint8_t tone)
{
    switch (tone)
    {
    case 1:
        return "Success focus badge active";
    case 2:
        return "Warning focus badge active";
    case 3:
        return "Neutral focus badge active";
    default:
        return "Accent focus badge active";
    }
}

static uint8_t badge_group_focus_tone(const egui_view_badge_group_snapshot_t *snapshot)
{
    uint8_t index;

    if (snapshot == NULL || snapshot->items == NULL || snapshot->item_count == 0)
    {
        return 0;
    }

    index = snapshot->focus_index;
    if (index >= snapshot->item_count)
    {
        index = 0;
    }
    return snapshot->items[index].tone;
}

static void set_status(const char *text, egui_color_t color)
{
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void apply_primary_snapshot(uint8_t index)
{
    uint8_t tone = badge_group_focus_tone(&primary_snapshots[index]);

    egui_view_badge_group_set_current_snapshot(EGUI_VIEW_OF(&group_primary), index);
    set_status(badge_group_status_text(tone), badge_group_status_color(tone));
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    uint8_t tone = badge_group_focus_tone(&compact_snapshots[index]);

    egui_view_badge_group_set_current_snapshot(EGUI_VIEW_OF(&group_compact), index);
    if (!update_status)
    {
        return;
    }

    if (tone == 2)
    {
        set_status("Compact warning preview", badge_group_status_color(tone));
    }
    else
    {
        set_status("Compact accent preview", badge_group_status_color(tone));
    }
}

static void on_primary_click(egui_view_t *self)
{
    uint8_t next = (egui_view_badge_group_get_current_snapshot(self) + 1) % 4;

    apply_primary_snapshot(next);
}

static void on_compact_click(egui_view_t *self)
{
    uint8_t next = (egui_view_badge_group_get_current_snapshot(self) + 1) % 2;

    EGUI_UNUSED(self);
    apply_compact_snapshot(next, 1);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 286);
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
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(0x687786), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_badge_group_init(EGUI_VIEW_OF(&group_primary));
    egui_view_set_size(EGUI_VIEW_OF(&group_primary), BADGE_GROUP_PRIMARY_WIDTH, BADGE_GROUP_PRIMARY_HEIGHT);
    egui_view_badge_group_set_snapshots(EGUI_VIEW_OF(&group_primary), primary_snapshots, 4);
    egui_view_badge_group_set_font(EGUI_VIEW_OF(&group_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_badge_group_set_meta_font(EGUI_VIEW_OF(&group_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_badge_group_set_palette(EGUI_VIEW_OF(&group_primary), EGUI_COLOR_HEX(0xFEFEFF), EGUI_COLOR_HEX(0xD1DBE6), EGUI_COLOR_HEX(0x17212B),
                                      EGUI_COLOR_HEX(0x667586), EGUI_COLOR_HEX(0x2E63DA), EGUI_COLOR_HEX(0x178454), EGUI_COLOR_HEX(0xB67619),
                                      EGUI_COLOR_HEX(0x768392));
    egui_view_set_margin(EGUI_VIEW_OF(&group_primary), 0, 0, 0, 2);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&group_primary), on_primary_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&group_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), "Accent focus badge active");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), badge_group_status_color(0), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 148, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDCE4EA));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), BADGE_GROUP_BOTTOM_WIDTH, BADGE_GROUP_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), BADGE_GROUP_COLUMN_WIDTH, BADGE_GROUP_BOTTOM_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), BADGE_GROUP_COLUMN_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(0x176D66), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_badge_group_init(EGUI_VIEW_OF(&group_compact));
    egui_view_set_size(EGUI_VIEW_OF(&group_compact), BADGE_GROUP_COLUMN_WIDTH, BADGE_GROUP_CARD_HEIGHT);
    egui_view_badge_group_set_snapshots(EGUI_VIEW_OF(&group_compact), compact_snapshots, 2);
    egui_view_badge_group_set_font(EGUI_VIEW_OF(&group_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_badge_group_set_meta_font(EGUI_VIEW_OF(&group_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_badge_group_set_compact_mode(EGUI_VIEW_OF(&group_compact), 1);
    egui_view_badge_group_set_palette(EGUI_VIEW_OF(&group_compact), EGUI_COLOR_HEX(0xFCFDFF), EGUI_COLOR_HEX(0xD3DCE7), EGUI_COLOR_HEX(0x213141),
                                      EGUI_COLOR_HEX(0x68778A), EGUI_COLOR_HEX(0x2E63DA), EGUI_COLOR_HEX(0x178454), EGUI_COLOR_HEX(0xB67619),
                                      EGUI_COLOR_HEX(0x768392));
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&group_compact), on_compact_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&group_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), BADGE_GROUP_COLUMN_WIDTH, BADGE_GROUP_BOTTOM_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 4, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), BADGE_GROUP_COLUMN_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(0x7F8C99), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_badge_group_init(EGUI_VIEW_OF(&group_locked));
    egui_view_set_size(EGUI_VIEW_OF(&group_locked), BADGE_GROUP_COLUMN_WIDTH, BADGE_GROUP_CARD_HEIGHT);
    egui_view_badge_group_set_snapshots(EGUI_VIEW_OF(&group_locked), locked_snapshots, 2);
    egui_view_badge_group_set_current_snapshot(EGUI_VIEW_OF(&group_locked), 1);
    egui_view_badge_group_set_font(EGUI_VIEW_OF(&group_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_badge_group_set_meta_font(EGUI_VIEW_OF(&group_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_badge_group_set_compact_mode(EGUI_VIEW_OF(&group_locked), 1);
    egui_view_badge_group_set_locked_mode(EGUI_VIEW_OF(&group_locked), 1);
    egui_view_badge_group_set_palette(EGUI_VIEW_OF(&group_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xD5DEE4), EGUI_COLOR_HEX(0x5E6C79),
                                      EGUI_COLOR_HEX(0x8E99A5), EGUI_COLOR_HEX(0x94A3B1), EGUI_COLOR_HEX(0x7DA488), EGUI_COLOR_HEX(0xB18E4B),
                                      EGUI_COLOR_HEX(0x96A1AC));
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&group_locked));

    apply_primary_snapshot(0);
    apply_compact_snapshot(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&locked_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&bottom_row));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&root_layout));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_layout));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
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
            apply_compact_snapshot(0, 0);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 1:
        if (first_call)
        {
            apply_primary_snapshot(1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_snapshot(2);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 3:
        if (first_call)
        {
            apply_compact_snapshot(1, 1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 240);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_snapshot(3);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 640);
        return true;
    default:
        return false;
    }
}
#endif
