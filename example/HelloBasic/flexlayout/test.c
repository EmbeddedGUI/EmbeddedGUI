#include "egui.h"
#include "uicode_disp0.h"

/*
 * FlexLayout demo — two sections stacked vertically:
 *
 *  [TOP HALF]  ROW + WRAP + SPACE_AROUND: 4 coloured squares arranged in a
 *              wrapping row that distributes space evenly around items.
 *
 *  [BOT HALF]  COLUMN + flex_grow: a header label of fixed height + a body
 *              label that grows to fill the remaining space.
 */

/* ------------------------------------------------------------------ */
/*  Top section: flex row + wrap                                        */
/* ------------------------------------------------------------------ */

static egui_view_flexlayout_t s_row_layout;
static egui_view_label_t s_item0;
static egui_view_label_t s_item1;
static egui_view_label_t s_item2;
static egui_view_label_t s_item3;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_bg0_param, EGUI_THEME_PRIMARY, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_bg1_param, EGUI_THEME_SECONDARY, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_bg2_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_bg3_param, EGUI_THEME_WARNING, EGUI_ALPHA_100);

EGUI_BACKGROUND_PARAM_INIT(s_bp0, &s_bg0_param, NULL, NULL);
EGUI_BACKGROUND_PARAM_INIT(s_bp1, &s_bg1_param, NULL, NULL);
EGUI_BACKGROUND_PARAM_INIT(s_bp2, &s_bg2_param, NULL, NULL);
EGUI_BACKGROUND_PARAM_INIT(s_bp3, &s_bg3_param, NULL, NULL);

EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_bg0, &s_bp0);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_bg1, &s_bp1);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_bg2, &s_bp2);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_bg3, &s_bp3);

/* ------------------------------------------------------------------ */
/*  Bottom section: flex column with flex_grow                         */
/* ------------------------------------------------------------------ */

static egui_view_flexlayout_t s_col_layout;
static egui_view_label_t s_header_label;
static egui_view_label_t s_body_label;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_hdr_bg_param, EGUI_THEME_PRIMARY, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_hdr_bp, &s_hdr_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_hdr_bg, &s_hdr_bp);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_body_bg_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_body_bp, &s_body_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_body_bg, &s_body_bp);

/* ------------------------------------------------------------------ */

#define SCREEN_W  EGUI_CONFIG_SCREEN_WIDTH
#define SCREEN_H  EGUI_CONFIG_SCREEN_HEIGHT
#define HALF_H    (SCREEN_H / 2)
#define ITEM_SIZE 60

void test_init_ui(egui_core_t *core)
{
    /* ---- top: row + wrap ---- */
    egui_view_flexlayout_init(EGUI_VIEW_OF(&s_row_layout), core);
    egui_view_set_position(EGUI_VIEW_OF(&s_row_layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&s_row_layout), SCREEN_W, HALF_H);
    egui_view_flexlayout_set_wrap(EGUI_VIEW_OF(&s_row_layout), EGUI_FLEX_WRAP_WRAP);
    egui_view_flexlayout_set_justify_content(EGUI_VIEW_OF(&s_row_layout), EGUI_FLEX_JUSTIFY_SPACE_AROUND);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&s_row_layout), EGUI_FLEX_ALIGN_CENTER);
    egui_view_flexlayout_set_align_content(EGUI_VIEW_OF(&s_row_layout), EGUI_FLEX_JUSTIFY_SPACE_EVENLY);
    egui_view_set_padding(EGUI_VIEW_OF(&s_row_layout), 8, 8, 8, 8);

    EGUI_VIEW_LABEL_PARAMS_INIT(p0, 0, 0, ITEM_SIZE, ITEM_SIZE, "", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    EGUI_VIEW_LABEL_PARAMS_INIT(p1, 0, 0, ITEM_SIZE, ITEM_SIZE, "", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    EGUI_VIEW_LABEL_PARAMS_INIT(p2, 0, 0, ITEM_SIZE, ITEM_SIZE, "", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    EGUI_VIEW_LABEL_PARAMS_INIT(p3, 0, 0, ITEM_SIZE, ITEM_SIZE, "", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);

    egui_view_label_init_with_params(EGUI_VIEW_OF(&s_item0), core, &p0);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&s_item1), core, &p1);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&s_item2), core, &p2);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&s_item3), core, &p3);

    egui_view_set_background(EGUI_VIEW_OF(&s_item0), EGUI_BG_OF(&s_bg0));
    egui_view_set_background(EGUI_VIEW_OF(&s_item1), EGUI_BG_OF(&s_bg1));
    egui_view_set_background(EGUI_VIEW_OF(&s_item2), EGUI_BG_OF(&s_bg2));
    egui_view_set_background(EGUI_VIEW_OF(&s_item3), EGUI_BG_OF(&s_bg3));

    egui_view_group_add_child(EGUI_VIEW_OF(&s_row_layout), EGUI_VIEW_OF(&s_item0));
    egui_view_group_add_child(EGUI_VIEW_OF(&s_row_layout), EGUI_VIEW_OF(&s_item1));
    egui_view_group_add_child(EGUI_VIEW_OF(&s_row_layout), EGUI_VIEW_OF(&s_item2));
    egui_view_group_add_child(EGUI_VIEW_OF(&s_row_layout), EGUI_VIEW_OF(&s_item3));

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&s_row_layout));

    /* ---- bottom: column + flex_grow ---- */
    egui_view_flexlayout_init(EGUI_VIEW_OF(&s_col_layout), core);
    egui_view_set_position(EGUI_VIEW_OF(&s_col_layout), 0, HALF_H);
    egui_view_set_size(EGUI_VIEW_OF(&s_col_layout), SCREEN_W, SCREEN_H - HALF_H);
    egui_view_flexlayout_set_direction(EGUI_VIEW_OF(&s_col_layout), EGUI_FLEX_DIRECTION_COLUMN);
    egui_view_flexlayout_set_align_items(EGUI_VIEW_OF(&s_col_layout), EGUI_FLEX_ALIGN_STRETCH);

    EGUI_VIEW_LABEL_PARAMS_INIT(hdr_p, 0, 0, SCREEN_W, 36, "FlexLayout Demo", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&s_header_label), core, &hdr_p);
    egui_view_set_background(EGUI_VIEW_OF(&s_header_label), EGUI_BG_OF(&s_hdr_bg));

    EGUI_VIEW_LABEL_PARAMS_INIT(body_p, 0, 0, SCREEN_W, 40, "flex_grow=1", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&s_body_label), core, &body_p);
    egui_view_set_flex_grow(EGUI_VIEW_OF(&s_body_label), 1);
    egui_view_set_background(EGUI_VIEW_OF(&s_body_label), EGUI_BG_OF(&s_body_bg));

    egui_view_group_add_child(EGUI_VIEW_OF(&s_col_layout), EGUI_VIEW_OF(&s_header_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&s_col_layout), EGUI_VIEW_OF(&s_body_label));

    egui_view_flexlayout_layout_childs(EGUI_VIEW_OF(&s_col_layout));

    /* ---- add both sections to root ---- */
    egui_core_add_user_root_view(EGUI_VIEW_OF(&s_row_layout));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&s_col_layout));
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 1)
    {
        return false;
    }
    EGUI_SIM_SET_WAIT(p_action, 1500);
    return true;
}
#endif
