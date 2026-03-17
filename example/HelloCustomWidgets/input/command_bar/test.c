#include "egui.h"
#include "egui_view_command_bar.h"
#include "uicode.h"
#include "utils/egui_sprintf.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define COMMAND_BAR_ROOT_WIDTH          224
#define COMMAND_BAR_ROOT_HEIGHT         296
#define COMMAND_BAR_PRIMARY_WIDTH       196
#define COMMAND_BAR_PRIMARY_HEIGHT      88
#define COMMAND_BAR_COMPACT_WIDTH       106
#define COMMAND_BAR_COMPACT_HEIGHT      80
#define COMMAND_BAR_BOTTOM_ROW_WIDTH    218
#define COMMAND_BAR_BOTTOM_ROW_HEIGHT   92
#define COMMAND_BAR_STATUS_MIX          28
#define COMMAND_BAR_GUIDE_COLOR         0x7E8D9B
#define COMMAND_BAR_LABEL_COLOR         0x768291
#define COMMAND_BAR_STATUS_COLOR        0x4F677E
#define COMMAND_BAR_COMPACT_LABEL_COLOR 0x0D776E
#define COMMAND_BAR_DISABLED_LABEL      0x87929F

static egui_view_linearlayout_t root_layout;
static egui_view_label_t title_label;
static egui_view_label_t guide_label;
static egui_view_label_t primary_label;
static egui_view_command_bar_t bar_primary;
static egui_view_label_t status_label;
static egui_view_line_t section_divider;
static egui_view_linearlayout_t bottom_row;
static egui_view_linearlayout_t compact_column;
static egui_view_label_t compact_label;
static egui_view_command_bar_t bar_compact;
static egui_view_linearlayout_t disabled_column;
static egui_view_label_t disabled_label;
static egui_view_command_bar_t bar_disabled;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_page_panel_param, EGUI_COLOR_HEX(0xF5F7F9), EGUI_ALPHA_100, 14);
EGUI_BACKGROUND_PARAM_INIT(bg_page_panel_params, &bg_page_panel_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_panel, &bg_page_panel_params);

static const char *title_text = "Command Bar";
static const char *guide_text = "Tap commands, tap guide";
static const egui_view_line_point_t divider_points[] = {{0, 0}, {136, 0}};
static char status_text[64] = "Edit: Save";

static const egui_view_command_bar_item_t primary_items_0[] = {
        {"SV", "Save", EGUI_VIEW_COMMAND_BAR_TONE_ACCENT, 1, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"CL", "Clone", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"SH", "Share", EGUI_VIEW_COMMAND_BAR_TONE_ACCENT, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"...", "Overflow", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW},
};

static const egui_view_command_bar_item_t primary_items_1[] = {
        {"AP", "Approve", EGUI_VIEW_COMMAND_BAR_TONE_SUCCESS, 1, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"NT", "Note", EGUI_VIEW_COMMAND_BAR_TONE_ACCENT, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"BL", "Block", EGUI_VIEW_COMMAND_BAR_TONE_DANGER, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"...", "Overflow", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW},
};

static const egui_view_command_bar_item_t primary_items_2[] = {
        {"AL", "Align", EGUI_VIEW_COMMAND_BAR_TONE_ACCENT, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"DS", "Dense", EGUI_VIEW_COMMAND_BAR_TONE_WARNING, 1, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"GR", "Grid", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"...", "Overflow", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW},
};

static const egui_view_command_bar_item_t primary_items_3[] = {
        {"SD", "Ship", EGUI_VIEW_COMMAND_BAR_TONE_ACCENT, 1, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"ST", "Stage", EGUI_VIEW_COMMAND_BAR_TONE_WARNING, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"HD", "Hold", EGUI_VIEW_COMMAND_BAR_TONE_DANGER, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"...", "Overflow", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW},
};

static const egui_view_command_bar_item_t compact_items_0[] = {
        {"SV", "Save", EGUI_VIEW_COMMAND_BAR_TONE_ACCENT, 1, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"SH", "Share", EGUI_VIEW_COMMAND_BAR_TONE_ACCENT, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"CL", "Clone", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"...", "Overflow", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW},
};

static const egui_view_command_bar_item_t compact_items_1[] = {
        {"AP", "Approve", EGUI_VIEW_COMMAND_BAR_TONE_SUCCESS, 1, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"CM", "Comment", EGUI_VIEW_COMMAND_BAR_TONE_ACCENT, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"BL", "Block", EGUI_VIEW_COMMAND_BAR_TONE_DANGER, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"...", "Overflow", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW},
};

static const egui_view_command_bar_item_t disabled_items_0[] = {
        {"SV", "Save", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 1, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"SH", "Share", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 0, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"AR", "Archive", EGUI_VIEW_COMMAND_BAR_TONE_DANGER, 0, 0, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"...", "Overflow", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 0, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW},
};

static const egui_view_command_bar_snapshot_t primary_snapshots[] = {
        {"Edit", "Page commands", "Canvas", "Save, share, or overflow", primary_items_0, 4, 0},
        {"Review", "Review commands", "Build", "Approve, note, or block", primary_items_1, 4, 1},
        {"Layout", "Layout commands", "Panels", "Tune density before ship", primary_items_2, 4, 1},
        {"Publish", "Publish commands", "Release", "Ship now, stage later", primary_items_3, 4, 0},
};

static const egui_view_command_bar_snapshot_t compact_snapshots[] = {
        {"Quick", "Compact rail", "Quick", "Tight icon rail", compact_items_0, 4, 0},
        {"Review", "Compact rail", "Review", "Compact review bar", compact_items_1, 4, 0},
};

static const egui_view_command_bar_snapshot_t disabled_snapshots[] = {
        {"Locked", "Disabled rail", "Read only", "Visible but inactive", disabled_items_0, 4, 0},
};

static egui_color_t command_bar_status_color(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_COMMAND_BAR_TONE_SUCCESS:
        return EGUI_COLOR_HEX(0x178454);
    case EGUI_VIEW_COMMAND_BAR_TONE_WARNING:
        return EGUI_COLOR_HEX(0xA96E12);
    case EGUI_VIEW_COMMAND_BAR_TONE_DANGER:
        return EGUI_COLOR_HEX(0xB13A35);
    case EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL:
        return EGUI_COLOR_HEX(0x728191);
    default:
        return EGUI_COLOR_HEX(0x2563EB);
    }
}

static void set_status(const char *prefix, const egui_view_command_bar_snapshot_t *snapshot, const egui_view_command_bar_item_t *item)
{
    int pos;
    egui_color_t color = command_bar_status_color(item == NULL ? EGUI_VIEW_COMMAND_BAR_TONE_ACCENT : item->tone);

    status_text[0] = '\0';
    pos = egui_sprintf_str(status_text, sizeof(status_text), prefix);
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ':');
    }
    if (pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_char(&status_text[pos], (int)sizeof(status_text) - pos, ' ');
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, item == NULL ? "Idle" : item->label);
    }
    if (snapshot != NULL && snapshot->scope != NULL && snapshot->scope[0] != '\0' && pos < (int)sizeof(status_text) - 1)
    {
        pos += egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, " / ");
        egui_sprintf_str(&status_text[pos], (int)sizeof(status_text) - pos, snapshot->scope);
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), egui_rgb_mix(color, EGUI_COLOR_HEX(0x56687A), COMMAND_BAR_STATUS_MIX), EGUI_ALPHA_100);
}

static void update_primary_status(void)
{
    uint8_t snapshot_index = egui_view_command_bar_get_current_snapshot(EGUI_VIEW_OF(&bar_primary));
    uint8_t item_index = egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&bar_primary));
    const egui_view_command_bar_snapshot_t *snapshot = &primary_snapshots[snapshot_index];
    const egui_view_command_bar_item_t *item = item_index < snapshot->item_count ? &snapshot->items[item_index] : NULL;

    set_status(snapshot->eyebrow, snapshot, item);
}

static void apply_primary_snapshot(uint8_t index)
{
    egui_view_command_bar_set_current_snapshot(EGUI_VIEW_OF(&bar_primary), index);
    update_primary_status();
}

static void apply_compact_snapshot(uint8_t index, uint8_t update_status)
{
    const egui_view_command_bar_snapshot_t *snapshot = &compact_snapshots[index];
    uint8_t item_index;
    const egui_view_command_bar_item_t *item;

    egui_view_command_bar_set_current_snapshot(EGUI_VIEW_OF(&bar_compact), index);
    if (!update_status)
    {
        return;
    }

    item_index = egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&bar_compact));
    item = item_index < snapshot->item_count ? &snapshot->items[item_index] : NULL;
    set_status("Compact", snapshot, item);
}

static void on_primary_selection_changed(egui_view_t *self, uint8_t index)
{
    uint8_t snapshot_index = egui_view_command_bar_get_current_snapshot(self);
    const egui_view_command_bar_snapshot_t *snapshot = &primary_snapshots[snapshot_index];

    set_status(snapshot->eyebrow, snapshot, &snapshot->items[index]);
}

static void on_compact_selection_changed(egui_view_t *self, uint8_t index)
{
    uint8_t snapshot_index = egui_view_command_bar_get_current_snapshot(self);
    const egui_view_command_bar_snapshot_t *snapshot = &compact_snapshots[snapshot_index];

    set_status("Compact", snapshot, &snapshot->items[index]);
}

static void on_guide_click(egui_view_t *self)
{
    uint8_t next = (egui_view_command_bar_get_current_snapshot(EGUI_VIEW_OF(&bar_primary)) + 1) % 4;

    EGUI_UNUSED(self);
    apply_primary_snapshot(next);
}

static void on_compact_label_click(egui_view_t *self)
{
    uint8_t next = (egui_view_command_bar_get_current_snapshot(EGUI_VIEW_OF(&bar_compact)) + 1) % 2;

    EGUI_UNUSED(self);
    apply_compact_snapshot(next, 1);
}

void test_init_ui(void)
{
    egui_view_linearlayout_init(EGUI_VIEW_OF(&root_layout));
    egui_view_set_size(EGUI_VIEW_OF(&root_layout), COMMAND_BAR_ROOT_WIDTH, COMMAND_BAR_ROOT_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&root_layout), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&root_layout), EGUI_ALIGN_HCENTER);
    egui_view_set_background(EGUI_VIEW_OF(&root_layout), EGUI_BG_OF(&bg_page_panel));

    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), COMMAND_BAR_ROOT_WIDTH, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), title_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&title_label), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&title_label), EGUI_COLOR_HEX(0x21303F), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&title_label), 0, 7, 0, 0);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&title_label));

    egui_view_label_init(EGUI_VIEW_OF(&guide_label));
    egui_view_set_size(EGUI_VIEW_OF(&guide_label), COMMAND_BAR_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&guide_label), guide_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&guide_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&guide_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&guide_label), EGUI_COLOR_HEX(COMMAND_BAR_GUIDE_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&guide_label), 0, 0, 0, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&guide_label), on_guide_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&guide_label));

    egui_view_label_init(EGUI_VIEW_OF(&primary_label));
    egui_view_set_size(EGUI_VIEW_OF(&primary_label), COMMAND_BAR_ROOT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&primary_label), "Standard bar");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&primary_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&primary_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&primary_label), EGUI_COLOR_HEX(COMMAND_BAR_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&primary_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&primary_label));

    egui_view_command_bar_init(EGUI_VIEW_OF(&bar_primary));
    egui_view_set_size(EGUI_VIEW_OF(&bar_primary), COMMAND_BAR_PRIMARY_WIDTH, COMMAND_BAR_PRIMARY_HEIGHT);
    egui_view_command_bar_set_font(EGUI_VIEW_OF(&bar_primary), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_command_bar_set_meta_font(EGUI_VIEW_OF(&bar_primary), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_command_bar_set_snapshots(EGUI_VIEW_OF(&bar_primary), primary_snapshots, 4);
    egui_view_command_bar_set_on_selection_changed_listener(EGUI_VIEW_OF(&bar_primary), on_primary_selection_changed);
    egui_view_set_margin(EGUI_VIEW_OF(&bar_primary), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bar_primary));

    egui_view_label_init(EGUI_VIEW_OF(&status_label));
    egui_view_set_size(EGUI_VIEW_OF(&status_label), COMMAND_BAR_ROOT_WIDTH, 12);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&status_label), (const egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_HEX(COMMAND_BAR_STATUS_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&status_label), 0, 0, 0, 3);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&status_label));

    egui_view_line_init(EGUI_VIEW_OF(&section_divider));
    egui_view_set_size(EGUI_VIEW_OF(&section_divider), 136, 2);
    egui_view_line_set_points(EGUI_VIEW_OF(&section_divider), divider_points, 2);
    egui_view_line_set_line_width(EGUI_VIEW_OF(&section_divider), 1);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&section_divider), EGUI_COLOR_HEX(0xE4EBF1));
    egui_view_set_margin(EGUI_VIEW_OF(&section_divider), 0, 0, 0, 4);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&section_divider));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&bottom_row));
    egui_view_set_size(EGUI_VIEW_OF(&bottom_row), COMMAND_BAR_BOTTOM_ROW_WIDTH, COMMAND_BAR_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&bottom_row), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&bottom_row), EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&root_layout), EGUI_VIEW_OF(&bottom_row));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&compact_column));
    egui_view_set_size(EGUI_VIEW_OF(&compact_column), COMMAND_BAR_COMPACT_WIDTH, COMMAND_BAR_BOTTOM_ROW_HEIGHT);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&compact_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&compact_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&compact_column));

    egui_view_label_init(EGUI_VIEW_OF(&compact_label));
    egui_view_set_size(EGUI_VIEW_OF(&compact_label), COMMAND_BAR_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&compact_label), "Compact");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&compact_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&compact_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&compact_label), EGUI_COLOR_HEX(COMMAND_BAR_COMPACT_LABEL_COLOR), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&compact_label), 0, 0, 0, 2);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&compact_label), on_compact_label_click);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&compact_label));

    egui_view_command_bar_init(EGUI_VIEW_OF(&bar_compact));
    egui_view_set_size(EGUI_VIEW_OF(&bar_compact), COMMAND_BAR_COMPACT_WIDTH, COMMAND_BAR_COMPACT_HEIGHT);
    egui_view_command_bar_set_font(EGUI_VIEW_OF(&bar_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_command_bar_set_meta_font(EGUI_VIEW_OF(&bar_compact), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_command_bar_set_snapshots(EGUI_VIEW_OF(&bar_compact), compact_snapshots, 2);
    egui_view_command_bar_set_compact_mode(EGUI_VIEW_OF(&bar_compact), 1);
    egui_view_command_bar_set_on_selection_changed_listener(EGUI_VIEW_OF(&bar_compact), on_compact_selection_changed);
    egui_view_group_add_child(EGUI_VIEW_OF(&compact_column), EGUI_VIEW_OF(&bar_compact));

    egui_view_linearlayout_init(EGUI_VIEW_OF(&disabled_column));
    egui_view_set_size(EGUI_VIEW_OF(&disabled_column), COMMAND_BAR_COMPACT_WIDTH, COMMAND_BAR_BOTTOM_ROW_HEIGHT);
    egui_view_set_margin(EGUI_VIEW_OF(&disabled_column), 6, 0, 0, 0);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&disabled_column), 0);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&disabled_column), EGUI_ALIGN_HCENTER);
    egui_view_group_add_child(EGUI_VIEW_OF(&bottom_row), EGUI_VIEW_OF(&disabled_column));

    egui_view_label_init(EGUI_VIEW_OF(&disabled_label));
    egui_view_set_size(EGUI_VIEW_OF(&disabled_label), COMMAND_BAR_COMPACT_WIDTH, 11);
    egui_view_label_set_text(EGUI_VIEW_OF(&disabled_label), "Disabled");
    egui_view_label_set_align_type(EGUI_VIEW_OF(&disabled_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&disabled_label), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&disabled_label), EGUI_COLOR_HEX(COMMAND_BAR_DISABLED_LABEL), EGUI_ALPHA_100);
    egui_view_set_margin(EGUI_VIEW_OF(&disabled_label), 0, 0, 0, 2);
    egui_view_group_add_child(EGUI_VIEW_OF(&disabled_column), EGUI_VIEW_OF(&disabled_label));

    egui_view_command_bar_init(EGUI_VIEW_OF(&bar_disabled));
    egui_view_set_size(EGUI_VIEW_OF(&bar_disabled), COMMAND_BAR_COMPACT_WIDTH, COMMAND_BAR_COMPACT_HEIGHT);
    egui_view_command_bar_set_font(EGUI_VIEW_OF(&bar_disabled), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_command_bar_set_meta_font(EGUI_VIEW_OF(&bar_disabled), (const egui_font_t *)&egui_res_font_montserrat_8_4);
    egui_view_command_bar_set_snapshots(EGUI_VIEW_OF(&bar_disabled), disabled_snapshots, 1);
    egui_view_command_bar_set_compact_mode(EGUI_VIEW_OF(&bar_disabled), 1);
    egui_view_command_bar_set_disabled_mode(EGUI_VIEW_OF(&bar_disabled), 1);
    egui_view_command_bar_set_palette(EGUI_VIEW_OF(&bar_disabled), EGUI_COLOR_HEX(0xFBFCFD), EGUI_COLOR_HEX(0xF7F9FB), EGUI_COLOR_HEX(0xD8E0E7),
                                      EGUI_COLOR_HEX(0x50606F), EGUI_COLOR_HEX(0x8C98A5), EGUI_COLOR_HEX(0x90A0AE), EGUI_COLOR_HEX(0x93A594),
                                      EGUI_COLOR_HEX(0xB29A67), EGUI_COLOR_HEX(0xA48B88), EGUI_COLOR_HEX(0x95A2AF));
    egui_view_group_add_child(EGUI_VIEW_OF(&disabled_column), EGUI_VIEW_OF(&bar_disabled));

    apply_primary_snapshot(0);
    apply_compact_snapshot(0, 0);

    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&compact_column));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&disabled_column));
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
            apply_compact_snapshot(1, 0);
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
