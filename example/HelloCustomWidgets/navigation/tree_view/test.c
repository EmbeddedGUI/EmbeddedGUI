#include <stdlib.h>

#include "egui.h"
#include "egui_view_tree_view.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define TREE_VIEW_PRIMARY_WIDTH        194
#define TREE_VIEW_PRIMARY_HEIGHT       122
#define TREE_VIEW_COMPACT_WIDTH        106
#define TREE_VIEW_COMPACT_HEIGHT       88
#define TREE_VIEW_BOTTOM_ROW_WIDTH     218
#define TREE_VIEW_BOTTOM_ROW_HEIGHT    102
#define TREE_VIEW_GUIDE_COLOR          0x74818F
#define TREE_VIEW_PRIMARY_LABEL_COLOR  0x7B8795
#define TREE_VIEW_STATUS_COLOR         0x4E6980
#define TREE_VIEW_COMPACT_LABEL_COLOR  0x0B756C
#define TREE_VIEW_COMPACT_BORDER_COLOR 0xD3DDDA
#define TREE_VIEW_COMPACT_ACCENT_COLOR 0x0D8076
#define TREE_VIEW_LOCKED_LABEL_COLOR   0x7F8C99
#define TREE_VIEW_LOCKED_BORDER_COLOR  0xD3DCE4
#define TREE_VIEW_LOCKED_ACCENT_COLOR  0x94A2AF
#define TREE_VIEW_STATUS_MUTED_MIX     32

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_tree_view_t tree_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_tree_view_t tree_compact;
static egui_view_linearlayout_t locked_column;
static egui_view_label_t locked_label;
static egui_view_tree_view_t tree_locked;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Tree View";
static const char *guide_text = "Tap rows to change focus";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {135, 0}};
static char status_text[48] = "Focus Branches";

static const egui_view_tree_view_item_t primary_items_0[] = {
        {"Workspaces", "2", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Controls", "12", 1, EGUI_VIEW_TREE_VIEW_TONE_ACCENT, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Buttons", "", 2, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Tree View", "Draft", 2, EGUI_VIEW_TREE_VIEW_TONE_ACCENT, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Resources", "3", 1, EGUI_VIEW_TREE_VIEW_TONE_WARNING, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 0},
        {"Settings", "", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 0},
};

static const egui_view_tree_view_item_t primary_items_1[] = {
        {"Workspaces", "2", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Docs", "7", 1, EGUI_VIEW_TREE_VIEW_TONE_ACCENT, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Overview", "", 2, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"API", "New", 2, EGUI_VIEW_TREE_VIEW_TONE_SUCCESS, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Samples", "", 2, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Settings", "", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 0},
};

static const egui_view_tree_view_item_t primary_items_2[] = {
        {"Workspaces", "2", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Resources", "3", 1, EGUI_VIEW_TREE_VIEW_TONE_WARNING, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Icons", "", 2, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Tokens", "Sync", 2, EGUI_VIEW_TREE_VIEW_TONE_WARNING, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Docs", "7", 1, EGUI_VIEW_TREE_VIEW_TONE_ACCENT, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 0},
        {"Settings", "", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 0},
};

static const egui_view_tree_view_item_t primary_items_3[] = {
        {"Workspaces", "2", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Settings", "4", 1, EGUI_VIEW_TREE_VIEW_TONE_ACCENT, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Preferences", "", 2, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Themes", "Ready", 2, EGUI_VIEW_TREE_VIEW_TONE_SUCCESS, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Accounts", "", 2, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Docs", "7", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 0},
};

static const egui_view_tree_view_snapshot_t primary_snapshots[] = {
        {"Solution", "6 visible", "Controls open", primary_items_0, 6, 3},
        {"Solution", "6 visible", "Docs open", primary_items_1, 6, 3},
        {"Solution", "6 visible", "Resources open", primary_items_2, 6, 3},
        {"Solution", "6 visible", "Settings open", primary_items_3, 6, 3},
};

static const egui_view_tree_view_item_t compact_items_0[] = {
        {"Home", "", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 0},
        {"Library", "4", 0, EGUI_VIEW_TREE_VIEW_TONE_ACCENT, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Recent", "", 1, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Settings", "", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 0},
};

static const egui_view_tree_view_item_t compact_items_1[] = {
        {"Home", "", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 0},
        {"Library", "4", 0, EGUI_VIEW_TREE_VIEW_TONE_ACCENT, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Review", "1", 1, EGUI_VIEW_TREE_VIEW_TONE_SUCCESS, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Settings", "", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 0},
};

static const egui_view_tree_view_snapshot_t compact_snapshots[] = {
        {"Compact", "4 rows", "Library branch", compact_items_0, 4, 2},
        {"Compact", "4 rows", "Review branch", compact_items_1, 4, 2},
};

static const egui_view_tree_view_item_t locked_items[] = {
        {"Workspace", "", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Release", "", 1, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Audit", "", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Exports", "", 1, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
};

static const egui_view_tree_view_snapshot_t locked_snapshots[] = {
        {"Read only", "4 rows", "Selection fixed", locked_items, 4, 2},
};

static egui_color_t tree_view_status_color(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_TREE_VIEW_TONE_SUCCESS:
        return EGUI_COLOR_HEX(0x178454);
    case EGUI_VIEW_TREE_VIEW_TONE_WARNING:
        return EGUI_COLOR_HEX(0xA46F1D);
    case EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL:
        return EGUI_COLOR_HEX(0x708090);
    default:
        return EGUI_COLOR_HEX(0x2563EB);
    }
}

static const egui_view_tree_view_item_t *get_snapshot_item(const egui_view_tree_view_snapshot_t *snapshot, uint8_t index)
{
    if (snapshot == NULL || snapshot->items == NULL || index >= snapshot->item_count)
    {
        return NULL;
    }
    return &snapshot->items[index];
}

static void set_status(const char *prefix, const egui_view_tree_view_item_t *item, egui_color_t color)
{
    int pos = 0;

    status_text[0] = '\0';
    if (prefix != NULL)
    {
        pos += egui_sprintf_str(status_text, sizeof(status_text), prefix);
    }
    if (item != NULL && item->title != NULL && item->title[0] != '\0' && pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, item->title);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x536677), TREE_VIEW_STATUS_MUTED_MIX), EGUI_ALPHA_100);
}

static void apply_primary_snapshot(uint8_t index)
{
    const egui_view_tree_view_snapshot_t *snapshot = &primary_snapshots[index];
    const egui_view_tree_view_item_t *item = get_snapshot_item(snapshot, snapshot->focus_index);

    egui_view_tree_view_set_current_snapshot(EGUI_VIEW_OF(&tree_primary), index);
    set_status("Focus", item, tree_view_status_color(item == NULL ? EGUI_VIEW_TREE_VIEW_TONE_ACCENT : item->tone));
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    const egui_view_tree_view_snapshot_t *snapshot = &compact_snapshots[index];
    const egui_view_tree_view_item_t *item = get_snapshot_item(snapshot, snapshot->focus_index);

    egui_view_tree_view_set_current_snapshot(EGUI_VIEW_OF(&tree_compact), index);
    if (update_status)
    {
        set_status("Compact", item, EGUI_COLOR_HEX(0x0F766E));
    }
}

static void on_primary_selection_changed(egui_view_t *self, uint8_t index)
{
    const egui_view_tree_view_snapshot_t *snapshot = &primary_snapshots[egui_view_tree_view_get_current_snapshot(self)];
    const egui_view_tree_view_item_t *item = get_snapshot_item(snapshot, index);

    set_status("Focus", item, tree_view_status_color(item == NULL ? EGUI_VIEW_TREE_VIEW_TONE_ACCENT : item->tone));
}

static void on_compact_selection_changed(egui_view_t *self, uint8_t index)
{
    const egui_view_tree_view_snapshot_t *snapshot = &compact_snapshots[egui_view_tree_view_get_current_snapshot(self)];
    const egui_view_tree_view_item_t *item = get_snapshot_item(snapshot, index);

    EGUI_UNUSED(self);
    set_status("Compact", item, EGUI_COLOR_HEX(0x0F766E));
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), 224, 300);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 224, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 7, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(TREE_VIEW_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), 224, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard view");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(TREE_VIEW_PRIMARY_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_tree_view_init(EGUI_VIEW_OF(&tree_primary));
    egui_view_set_size(EGUI_VIEW_OF(&tree_primary), TREE_VIEW_PRIMARY_WIDTH, TREE_VIEW_PRIMARY_HEIGHT);
    egui_view_tree_view_set_font(EGUI_VIEW_OF(&tree_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_tree_view_set_meta_font(EGUI_VIEW_OF(&tree_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_tree_view_set_snapshots(EGUI_VIEW_OF(&tree_primary), primary_snapshots, 4);
    egui_view_tree_view_set_on_selection_changed_listener(EGUI_VIEW_OF(&tree_primary), on_primary_selection_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&tree_primary), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&tree_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), 224, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(TREE_VIEW_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 136, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xDEE4EB));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), TREE_VIEW_BOTTOM_ROW_WIDTH, TREE_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), TREE_VIEW_COMPACT_WIDTH, TREE_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), TREE_VIEW_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact view");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(TREE_VIEW_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_tree_view_init(EGUI_VIEW_OF(&tree_compact));
    egui_view_set_size(EGUI_VIEW_OF(&tree_compact), TREE_VIEW_COMPACT_WIDTH, TREE_VIEW_COMPACT_HEIGHT);
    egui_view_tree_view_set_font(EGUI_VIEW_OF(&tree_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_tree_view_set_meta_font(EGUI_VIEW_OF(&tree_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_tree_view_set_snapshots(EGUI_VIEW_OF(&tree_compact), compact_snapshots, 2);
    egui_view_tree_view_set_compact_mode(EGUI_VIEW_OF(&tree_compact), 1);
    egui_view_tree_view_set_palette(EGUI_VIEW_OF(&tree_compact), EGUI_COLOR_HEX(0xFFFFFF), EGUI_COLOR_HEX(0xF8FBFA),
                                    EGUI_COLOR_HEX(TREE_VIEW_COMPACT_BORDER_COLOR), EGUI_COLOR_HEX(0x21323B), EGUI_COLOR_HEX(0x72828F),
                                    EGUI_COLOR_HEX(TREE_VIEW_COMPACT_ACCENT_COLOR), EGUI_COLOR_HEX(0x178454), EGUI_COLOR_HEX(0xB77719),
                                    EGUI_COLOR_HEX(0x7A8796));
    egui_view_tree_view_set_on_selection_changed_listener(EGUI_VIEW_OF(&tree_compact), on_compact_selection_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&tree_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&locked_column));
    egui_view_set_size(EGUI_VIEW_OF(&locked_column), TREE_VIEW_COMPACT_WIDTH, TREE_VIEW_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&locked_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&locked_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&locked_column));

    egui_view_label_init(EGUI_VIEW_OF(&locked_label));
    egui_view_set_size(EGUI_VIEW_OF(&locked_label), TREE_VIEW_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&locked_label), "Read-only");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&locked_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&locked_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&locked_label), EGUI_COLOR_HEX(TREE_VIEW_LOCKED_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&locked_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&locked_label));

    egui_view_tree_view_init(EGUI_VIEW_OF(&tree_locked));
    egui_view_set_size(EGUI_VIEW_OF(&tree_locked), TREE_VIEW_COMPACT_WIDTH, TREE_VIEW_COMPACT_HEIGHT);
    egui_view_tree_view_set_font(EGUI_VIEW_OF(&tree_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_tree_view_set_meta_font(EGUI_VIEW_OF(&tree_locked), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_tree_view_set_snapshots(EGUI_VIEW_OF(&tree_locked), locked_snapshots, 1);
    egui_view_tree_view_set_compact_mode(EGUI_VIEW_OF(&tree_locked), 1);
    egui_view_tree_view_set_locked_mode(EGUI_VIEW_OF(&tree_locked), 1);
    egui_view_tree_view_set_palette(EGUI_VIEW_OF(&tree_locked), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xFBFCFD),
                                    EGUI_COLOR_HEX(TREE_VIEW_LOCKED_BORDER_COLOR), EGUI_COLOR_HEX(0x566473), EGUI_COLOR_HEX(0x8C98A6),
                                    EGUI_COLOR_HEX(TREE_VIEW_LOCKED_ACCENT_COLOR), EGUI_COLOR_HEX(0x93A39A), EGUI_COLOR_HEX(0xB59A68),
                                    EGUI_COLOR_HEX(0x9AA6B4));
    egui_view_group_add_child(EGUI_VIEW_OF(&locked_column), EGUI_VIEW_OF(&tree_locked));

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
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 170);
        return true;
    case 2:
        if (first_call)
        {
            apply_primary_snapshot(1);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 170);
        return true;
    case 4:
        if (first_call)
        {
            apply_primary_snapshot(2);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 5:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 170);
        return true;
    case 6:
        if (first_call)
        {
            apply_compact_snapshot(1, 1);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 7:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 170);
        return true;
    case 8:
        if (first_call)
        {
            apply_primary_snapshot(3);
        }
        EGUI_SIM_SET_WAIT(p_action, 90);
        return true;
    case 9:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 520);
        return true;
    default:
        return false;
    }
}
#endif
