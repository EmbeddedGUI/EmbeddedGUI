#include "egui_theme.h"
#include "core/egui_config.h"
#include "shadow/egui_shadow.h"

/* ------------------------------------------------------------------ */
/* Shadows                                                            */
/* ------------------------------------------------------------------ */

static const egui_shadow_t shadow_sm = {
        .width = EGUI_THEME_SHADOW_WIDTH_SM,
        .ofs_x = 0,
        .ofs_y = EGUI_THEME_SHADOW_OFS_Y_SM,
        .spread = 0,
        .opa = EGUI_THEME_SHADOW_OPA,
        .color = EGUI_COLOR_BLACK,
        .corner_radius = EGUI_THEME_RADIUS_MD,
};

static const egui_shadow_t shadow_md = {
        .width = EGUI_THEME_SHADOW_WIDTH_MD,
        .ofs_x = 0,
        .ofs_y = EGUI_THEME_SHADOW_OFS_Y_MD,
        .spread = 0,
        .opa = EGUI_THEME_SHADOW_OPA,
        .color = EGUI_COLOR_BLACK,
        .corner_radius = EGUI_THEME_RADIUS_LG,
};

/* ================================================================== */
/* Button (1 part: MAIN)                                              */
/* ================================================================== */

static const egui_style_t light_btn_normal = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_TEXT_COLOR | EGUI_STYLE_PROP_SHADOW | EGUI_STYLE_PROP_PADDING,
        .bg_color = EGUI_THEME_PRIMARY,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = EGUI_THEME_PRIMARY,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_MD,
        .pad_top = 6,
        .pad_bottom = 6,
        .pad_left = 12,
        .pad_right = 12,
        .text_color = EGUI_THEME_TEXT,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = &shadow_sm,
};

static const egui_style_t light_btn_pressed = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_TEXT_COLOR | EGUI_STYLE_PROP_PADDING,
        .bg_color = EGUI_THEME_PRIMARY_DARK,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = EGUI_THEME_PRIMARY_DARK,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_MD,
        .pad_top = 6,
        .pad_bottom = 6,
        .pad_left = 12,
        .pad_right = 12,
        .text_color = EGUI_THEME_TEXT,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_btn_disabled = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_TEXT_COLOR | EGUI_STYLE_PROP_PADDING,
        .bg_color = EGUI_THEME_DISABLED,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = EGUI_THEME_DISABLED,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_MD,
        .pad_top = 6,
        .pad_bottom = 6,
        .pad_left = 12,
        .pad_right = 12,
        .text_color = EGUI_THEME_TEXT_SECONDARY,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t *const light_btn_styles[1 * EGUI_STATE_MAX] = {
        &light_btn_normal, &light_btn_pressed, &light_btn_disabled, &light_btn_normal, &light_btn_normal,
};

static const egui_widget_style_desc_t light_btn_desc = {
        .part_count = 1,
        .styles = light_btn_styles,
};

/* ================================================================== */
/* Label (1 part: MAIN)                                               */
/* ================================================================== */

static const egui_style_t light_label_normal = {
        .flags = EGUI_STYLE_PROP_TEXT_COLOR,
        .bg_color = EGUI_THEME_SURFACE,
        .bg_alpha = EGUI_ALPHA_TRANSP,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = 0,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = EGUI_THEME_TEXT_PRIMARY,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_label_disabled = {
        .flags = EGUI_STYLE_PROP_TEXT_COLOR,
        .bg_color = EGUI_THEME_SURFACE,
        .bg_alpha = EGUI_ALPHA_TRANSP,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = 0,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = EGUI_THEME_TEXT_SECONDARY,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t *const light_label_styles[1 * EGUI_STATE_MAX] = {
        &light_label_normal, &light_label_normal, &light_label_disabled, &light_label_normal, &light_label_normal,
};

static const egui_widget_style_desc_t light_label_desc = {
        .part_count = 1,
        .styles = light_label_styles,
};

/* ================================================================== */
/* Switch (2 parts: MAIN=track, INDICATOR=thumb)                      */
/* ================================================================== */

static const egui_style_t light_sw_track_normal = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_TRACK_OFF,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_LG,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_sw_track_disabled = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_DISABLED,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_LG,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_sw_track_checked = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_PRIMARY,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_LG,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_sw_thumb_normal = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_SHADOW,
        .bg_color = EGUI_THEME_THUMB,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_LG,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = &shadow_sm,
};

static const egui_style_t *const light_sw_styles[2 * EGUI_STATE_MAX] = {
        /* MAIN (track): normal, pressed, disabled, focused, checked */
        &light_sw_track_normal,
        &light_sw_track_normal,
        &light_sw_track_disabled,
        &light_sw_track_normal,
        &light_sw_track_checked,
        /* INDICATOR (thumb): normal, pressed, disabled, focused, checked */
        &light_sw_thumb_normal,
        &light_sw_thumb_normal,
        &light_sw_thumb_normal,
        &light_sw_thumb_normal,
        &light_sw_thumb_normal,
};

static const egui_widget_style_desc_t light_sw_desc = {.part_count = 2, .styles = light_sw_styles};

/* ================================================================== */
/* Slider (3 parts: MAIN=track, INDICATOR=fill, KNOB=thumb)           */
/* ================================================================== */

static const egui_style_t light_slider_track = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_TRACK_BG_DARK,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_SM,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_slider_track_disabled = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_DISABLED,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_SM,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_slider_indicator = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_PRIMARY,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_SM,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_slider_indicator_disabled = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_TEXT_SECONDARY,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_SM,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_slider_knob = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_SHADOW,
        .bg_color = EGUI_THEME_THUMB,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_LG,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = &shadow_sm,
};

static const egui_style_t light_slider_knob_pressed = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_SHADOW,
        .bg_color = EGUI_THEME_PRIMARY_LIGHT,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_LG,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = &shadow_md,
};

static const egui_style_t *const light_slider_styles[3 * EGUI_STATE_MAX] = {
        &light_slider_track,     &light_slider_track,     &light_slider_track_disabled, &light_slider_track,
        &light_slider_track,     &light_slider_indicator, &light_slider_indicator,      &light_slider_indicator_disabled,
        &light_slider_indicator, &light_slider_indicator, &light_slider_knob,           &light_slider_knob_pressed,
        &light_slider_knob,      &light_slider_knob,      &light_slider_knob,
};

static const egui_widget_style_desc_t light_slider_desc = {.part_count = 3, .styles = light_slider_styles};

/* ================================================================== */
/* Checkbox (1 part: MAIN)                                            */
/* ================================================================== */

static const egui_style_t light_cb_normal = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_BORDER | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_SURFACE,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = EGUI_THEME_BORDER,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 2,
        .radius = EGUI_THEME_RADIUS_SM,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_cb_disabled = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_BORDER | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_DISABLED,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = EGUI_THEME_DISABLED,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 2,
        .radius = EGUI_THEME_RADIUS_SM,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_cb_checked = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_PRIMARY,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = EGUI_THEME_PRIMARY,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_SM,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = EGUI_THEME_TEXT,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t *const light_cb_styles[1 * EGUI_STATE_MAX] = {
        &light_cb_normal, &light_cb_normal, &light_cb_disabled, &light_cb_normal, &light_cb_checked,
};

static const egui_widget_style_desc_t light_cb_desc = {.part_count = 1, .styles = light_cb_styles};

/* ================================================================== */
/* Card (1 part: MAIN)                                                */
/* ================================================================== */

static const egui_style_t light_card_normal = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_SHADOW | EGUI_STYLE_PROP_PADDING,
        .bg_color = EGUI_THEME_SURFACE,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = EGUI_THEME_BORDER,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_LG,
        .pad_top = 12,
        .pad_bottom = 12,
        .pad_left = 12,
        .pad_right = 12,
        .text_color = EGUI_THEME_TEXT_PRIMARY,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = &shadow_md,
};

static const egui_style_t *const light_card_styles[1 * EGUI_STATE_MAX] = {
        &light_card_normal, &light_card_normal, &light_card_normal, &light_card_normal, &light_card_normal,
};

static const egui_widget_style_desc_t light_card_desc = {.part_count = 1, .styles = light_card_styles};

/* ================================================================== */
/* Progress Bar (2 parts: MAIN=track, INDICATOR=fill)                 */
/* ================================================================== */

static const egui_style_t light_pb_track = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_TRACK_BG_DARK,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_SM,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t light_pb_indicator = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = EGUI_THEME_PRIMARY,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = {0},
        .border_alpha = EGUI_ALPHA_TRANSP,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_SM,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = {0},
        .text_alpha = EGUI_ALPHA_TRANSP,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t *const light_pb_styles[2 * EGUI_STATE_MAX] = {
        &light_pb_track,     &light_pb_track,     &light_pb_track,     &light_pb_track,     &light_pb_track,
        &light_pb_indicator, &light_pb_indicator, &light_pb_indicator, &light_pb_indicator, &light_pb_indicator,
};

static const egui_widget_style_desc_t light_pb_desc = {.part_count = 2, .styles = light_pb_styles};

/* Circular Progress Bar - reuse progress bar styles */
static const egui_widget_style_desc_t light_cpb_desc = {.part_count = 2, .styles = light_pb_styles};

/* ================================================================== */
/* Theme definition                                                   */
/* ================================================================== */

const egui_theme_t egui_theme_light = {
        .name = "light",
        .button = &light_btn_desc,
        .label = &light_label_desc,
        .switch_ctrl = &light_sw_desc,
        .slider = &light_slider_desc,
        .checkbox = &light_cb_desc,
        .card = &light_card_desc,
        .progress_bar = &light_pb_desc,
        .circular_progress_bar = &light_cpb_desc,
};
