#include "egui.h"
#include "uicode_disp0.h"
#include "widget/egui_style.h"

/*
 * Style Cascade demo — three rows, each 80 px tall, showing three cascade scenarios:
 *
 *  Row 0  "Shared style": view with only a style (PRIMARY background).
 *  Row 1  "Style stack":  two styles stacked; SECONDARY wins over PRIMARY.
 *  Row 2  "Inline wins":  same style stack as row 1, but an inline background
 *                         (SURFACE_VARIANT) overrides both styles.
 *
 * A label describes each row's behaviour.
 */

#define SCREEN_W    EGUI_CONFIG_SCREEN_WIDTH
#define SCREEN_H    EGUI_CONFIG_SCREEN_HEIGHT
#define ROW_COUNT   3
#define ROW_H       (SCREEN_H / ROW_COUNT)
#define SWATCH_W    (SCREEN_W / 3)

/* ------------------------------------------------------------------ */
/*  Shared style definitions (static const — zero runtime overhead)   */
/* ------------------------------------------------------------------ */

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_primary_param,    EGUI_THEME_PRIMARY,         EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_secondary_param,  EGUI_THEME_SECONDARY,       EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_surface_param,    EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);

EGUI_BACKGROUND_PARAM_INIT(s_primary_bp,   &s_primary_param,   NULL, NULL);
EGUI_BACKGROUND_PARAM_INIT(s_secondary_bp, &s_secondary_param, NULL, NULL);
EGUI_BACKGROUND_PARAM_INIT(s_surface_bp,   &s_surface_param,   NULL, NULL);

EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_primary_bg,   &s_primary_bp);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_secondary_bg, &s_secondary_bp);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_surface_bg,   &s_surface_bp);

static const egui_view_style_t s_style_primary   = EGUI_STYLE_INIT_BACKGROUND(EGUI_BG_OF(&s_primary_bg));
static const egui_view_style_t s_style_secondary = EGUI_STYLE_INIT_BACKGROUND(EGUI_BG_OF(&s_secondary_bg));

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_root_bg_param, EGUI_THEME_SURFACE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_root_bp, &s_root_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_root_bg, &s_root_bp);

/* ------------------------------------------------------------------ */
/*  View objects                                                       */
/* ------------------------------------------------------------------ */

static egui_view_group_t s_root;

/* Row 0: shared style */
static egui_view_t       s_swatch0;
static egui_view_label_t s_label0;

/* Row 1: style stack */
static egui_view_t       s_swatch1;
static egui_view_label_t s_label1;

/* Row 2: inline override */
static egui_view_t       s_swatch2;
static egui_view_label_t s_label2;

/* ------------------------------------------------------------------ */
/* Label param blocks — positions are compile-time constants           */
/* ------------------------------------------------------------------ */

EGUI_VIEW_LABEL_PARAMS_INIT(s_lp0, SWATCH_W, 0,           SCREEN_W - SWATCH_W, ROW_H,
                            "Style: PRIMARY",
                            EGUI_CONFIG_FONT_DEFAULT,
                            EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(s_lp1, SWATCH_W, ROW_H,
                            SCREEN_W - SWATCH_W, ROW_H,
                            "Stack: SECONDARY",
                            EGUI_CONFIG_FONT_DEFAULT,
                            EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(s_lp2, SWATCH_W, 2 * ROW_H,
                            SCREEN_W - SWATCH_W, ROW_H,
                            "Inline: SURFACE",
                            EGUI_CONFIG_FONT_DEFAULT,
                            EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);

/* ------------------------------------------------------------------ */

void test_init_ui(egui_core_t *core)
{
    /* root group covers the whole screen */
    egui_view_group_init(EGUI_VIEW_OF(&s_root), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_root), SCREEN_W, SCREEN_H);
    egui_view_set_position(EGUI_VIEW_OF(&s_root), 0, 0);
    egui_view_set_background(EGUI_VIEW_OF(&s_root), EGUI_BG_OF(&s_root_bg));

    /* ---- Row 0: single shared style (PRIMARY) ---- */
    egui_view_init(&s_swatch0, core);
    egui_view_set_position(&s_swatch0, 0, 0);
    egui_view_set_size(&s_swatch0, SWATCH_W, ROW_H);
    egui_view_add_style(&s_swatch0, &s_style_primary);

    egui_view_label_init_with_params(EGUI_VIEW_OF(&s_label0), core, &s_lp0);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_label0), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_set_padding(EGUI_VIEW_OF(&s_label0), 10, 4, 0, 0);

    egui_view_group_add_child(EGUI_VIEW_OF(&s_root), &s_swatch0);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_root), EGUI_VIEW_OF(&s_label0));

    /* ---- Row 1: style stack (PRIMARY < SECONDARY; SECONDARY wins) ---- */
    egui_view_init(&s_swatch1, core);
    egui_view_set_position(&s_swatch1, 0, ROW_H);
    egui_view_set_size(&s_swatch1, SWATCH_W, ROW_H);
    egui_view_add_style(&s_swatch1, &s_style_primary);    /* lower priority */
    egui_view_add_style(&s_swatch1, &s_style_secondary);  /* higher priority */

    egui_view_label_init_with_params(EGUI_VIEW_OF(&s_label1), core, &s_lp1);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_label1), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_set_padding(EGUI_VIEW_OF(&s_label1), 10, 4, 0, 0);

    egui_view_group_add_child(EGUI_VIEW_OF(&s_root), &s_swatch1);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_root), EGUI_VIEW_OF(&s_label1));

    /* ---- Row 2: inline override beats both styles ---- */
    egui_view_init(&s_swatch2, core);
    egui_view_set_position(&s_swatch2, 0, 2 * ROW_H);
    egui_view_set_size(&s_swatch2, SWATCH_W, ROW_H);
    egui_view_add_style(&s_swatch2, &s_style_primary);    /* lower priority */
    egui_view_add_style(&s_swatch2, &s_style_secondary);  /* higher priority */
    egui_view_set_background(&s_swatch2, EGUI_BG_OF(&s_surface_bg)); /* inline wins */

    egui_view_label_init_with_params(EGUI_VIEW_OF(&s_label2), core, &s_lp2);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_label2), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_set_padding(EGUI_VIEW_OF(&s_label2), 10, 4, 0, 0);

    egui_view_group_add_child(EGUI_VIEW_OF(&s_root), &s_swatch2);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_root), EGUI_VIEW_OF(&s_label2));

    /* attach root to the display core */
    egui_core_add_user_root_view(EGUI_VIEW_OF(&s_root));
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
