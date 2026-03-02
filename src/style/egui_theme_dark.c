#include "egui_theme.h"
#include "core/egui_config.h"
#include "shadow/egui_shadow.h"

/* Dark theme color tokens */
#define DARK_BG         EGUI_COLOR_MAKE(0x1E, 0x29, 0x3B) /* Slate 800 */
#define DARK_SURFACE    EGUI_COLOR_MAKE(0x33, 0x41, 0x55) /* Slate 700 */
#define DARK_BORDER     EGUI_COLOR_MAKE(0x47, 0x55, 0x69) /* Slate 600 */
#define DARK_TEXT       EGUI_COLOR_MAKE(0xF1, 0xF5, 0xF9) /* Slate 100 */
#define DARK_TEXT_SEC   EGUI_COLOR_MAKE(0x94, 0xA3, 0xB8) /* Slate 400 */
#define DARK_PRIMARY    EGUI_COLOR_MAKE(0x60, 0xA5, 0xFA) /* Blue 400 */
#define DARK_PRIMARY_DK EGUI_COLOR_MAKE(0x3B, 0x82, 0xF6) /* Blue 500 */
#define DARK_TRACK      EGUI_COLOR_MAKE(0x47, 0x55, 0x69) /* Slate 600 */
#define DARK_DISABLED   EGUI_COLOR_MAKE(0x64, 0x74, 0x8B) /* Slate 500 */

/* ------------------------------------------------------------------ */
/* Shadows                                                            */
/* ------------------------------------------------------------------ */

static const egui_shadow_t dark_shadow_sm = {
        .width = EGUI_THEME_SHADOW_WIDTH_SM,
        .ofs_x = 0,
        .ofs_y = EGUI_THEME_SHADOW_OFS_Y_SM,
        .spread = 0,
        .opa = EGUI_THEME_SHADOW_OPA,
        .color = EGUI_COLOR_BLACK,
        .corner_radius = EGUI_THEME_RADIUS_MD,
};

static const egui_shadow_t dark_shadow_md = {
        .width = EGUI_THEME_SHADOW_WIDTH_MD,
        .ofs_x = 0,
        .ofs_y = EGUI_THEME_SHADOW_OFS_Y_MD,
        .spread = 0,
        .opa = EGUI_THEME_SHADOW_OPA,
        .color = EGUI_COLOR_BLACK,
        .corner_radius = EGUI_THEME_RADIUS_LG,
};

/* ================================================================== */
/* Button                                                             */
/* ================================================================== */

static const egui_style_t dark_btn_normal = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_TEXT_COLOR | EGUI_STYLE_PROP_SHADOW | EGUI_STYLE_PROP_PADDING,
        .bg_color = DARK_PRIMARY,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = DARK_PRIMARY,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_MD,
        .pad_top = 6,
        .pad_bottom = 6,
        .pad_left = 12,
        .pad_right = 12,
        .text_color = DARK_BG,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = &dark_shadow_sm,
};

static const egui_style_t dark_btn_pressed = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_TEXT_COLOR | EGUI_STYLE_PROP_PADDING,
        .bg_color = DARK_PRIMARY_DK,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = DARK_PRIMARY_DK,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_MD,
        .pad_top = 6,
        .pad_bottom = 6,
        .pad_left = 12,
        .pad_right = 12,
        .text_color = DARK_BG,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t dark_btn_disabled = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_TEXT_COLOR | EGUI_STYLE_PROP_PADDING,
        .bg_color = DARK_DISABLED,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = DARK_DISABLED,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_MD,
        .pad_top = 6,
        .pad_bottom = 6,
        .pad_left = 12,
        .pad_right = 12,
        .text_color = DARK_TEXT_SEC,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t *const dark_btn_styles[1 * EGUI_STATE_MAX] = {
        &dark_btn_normal, &dark_btn_pressed, &dark_btn_disabled, &dark_btn_normal, &dark_btn_normal,
};

static const egui_widget_style_desc_t dark_btn_desc = {.part_count = 1, .styles = dark_btn_styles};

/* ================================================================== */
/* Label                                                              */
/* ================================================================== */

static const egui_style_t dark_label_normal = {
        .flags = EGUI_STYLE_PROP_TEXT_COLOR,
        .bg_color = {0},
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
        .text_color = DARK_TEXT,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t dark_label_disabled = {
        .flags = EGUI_STYLE_PROP_TEXT_COLOR,
        .bg_color = {0},
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
        .text_color = DARK_TEXT_SEC,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t *const dark_label_styles[1 * EGUI_STATE_MAX] = {
        &dark_label_normal, &dark_label_normal, &dark_label_disabled, &dark_label_normal, &dark_label_normal,
};

static const egui_widget_style_desc_t dark_label_desc = {.part_count = 1, .styles = dark_label_styles};

/* ================================================================== */
/* Switch (2 parts: MAIN=track, INDICATOR=thumb)                      */
/* ================================================================== */

static const egui_style_t dark_sw_track_normal = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_TRACK,
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

static const egui_style_t dark_sw_track_disabled = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_DISABLED,
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

static const egui_style_t dark_sw_track_checked = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_PRIMARY,
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

static const egui_style_t dark_sw_thumb_normal = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_SHADOW,
        .bg_color = DARK_TEXT,
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
        .shadow = &dark_shadow_sm,
};

static const egui_style_t *const dark_sw_styles[2 * EGUI_STATE_MAX] = {
        &dark_sw_track_normal, &dark_sw_track_normal, &dark_sw_track_disabled, &dark_sw_track_normal, &dark_sw_track_checked,
        &dark_sw_thumb_normal, &dark_sw_thumb_normal, &dark_sw_thumb_normal,   &dark_sw_thumb_normal, &dark_sw_thumb_normal,
};

static const egui_widget_style_desc_t dark_sw_desc = {.part_count = 2, .styles = dark_sw_styles};

/* ================================================================== */
/* Slider (3 parts: MAIN=track, INDICATOR=fill, KNOB=thumb)           */
/* ================================================================== */

static const egui_style_t dark_slider_track = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_TRACK,
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

static const egui_style_t dark_slider_track_disabled = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_DISABLED,
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

static const egui_style_t dark_slider_indicator = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_PRIMARY,
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

static const egui_style_t dark_slider_indicator_disabled = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_TEXT_SEC,
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

static const egui_style_t dark_slider_knob = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_SHADOW,
        .bg_color = DARK_TEXT,
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
        .shadow = &dark_shadow_sm,
};

static const egui_style_t dark_slider_knob_pressed = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_SHADOW,
        .bg_color = DARK_PRIMARY,
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
        .shadow = &dark_shadow_md,
};

static const egui_style_t *const dark_slider_styles[3 * EGUI_STATE_MAX] = {
        &dark_slider_track,     &dark_slider_track,     &dark_slider_track_disabled, &dark_slider_track,
        &dark_slider_track,     &dark_slider_indicator, &dark_slider_indicator,      &dark_slider_indicator_disabled,
        &dark_slider_indicator, &dark_slider_indicator, &dark_slider_knob,           &dark_slider_knob_pressed,
        &dark_slider_knob,      &dark_slider_knob,      &dark_slider_knob,
};

static const egui_widget_style_desc_t dark_slider_desc = {.part_count = 3, .styles = dark_slider_styles};

/* ================================================================== */
/* Checkbox                                                           */
/* ================================================================== */

static const egui_style_t dark_cb_normal = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_BORDER | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_SURFACE,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = DARK_BORDER,
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

static const egui_style_t dark_cb_disabled = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_BORDER | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_DISABLED,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = DARK_DISABLED,
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

static const egui_style_t dark_cb_checked = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_PRIMARY,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = DARK_PRIMARY,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_SM,
        .pad_top = 0,
        .pad_bottom = 0,
        .pad_left = 0,
        .pad_right = 0,
        .text_color = DARK_BG,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = NULL,
};

static const egui_style_t *const dark_cb_styles[1 * EGUI_STATE_MAX] = {
        &dark_cb_normal, &dark_cb_normal, &dark_cb_disabled, &dark_cb_normal, &dark_cb_checked,
};

static const egui_widget_style_desc_t dark_cb_desc = {.part_count = 1, .styles = dark_cb_styles};

/* ================================================================== */
/* Card                                                               */
/* ================================================================== */

static const egui_style_t dark_card_normal = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS | EGUI_STYLE_PROP_SHADOW | EGUI_STYLE_PROP_PADDING,
        .bg_color = DARK_SURFACE,
        .bg_alpha = EGUI_ALPHA_COVER,
        .bg_gradient = NULL,
        .border_color = DARK_BORDER,
        .border_alpha = EGUI_ALPHA_COVER,
        .border_width = 0,
        .radius = EGUI_THEME_RADIUS_LG,
        .pad_top = 12,
        .pad_bottom = 12,
        .pad_left = 12,
        .pad_right = 12,
        .text_color = DARK_TEXT,
        .text_alpha = EGUI_ALPHA_COVER,
        .text_font = NULL,
        .shadow = &dark_shadow_md,
};

static const egui_style_t *const dark_card_styles[1 * EGUI_STATE_MAX] = {
        &dark_card_normal, &dark_card_normal, &dark_card_normal, &dark_card_normal, &dark_card_normal,
};

static const egui_widget_style_desc_t dark_card_desc = {.part_count = 1, .styles = dark_card_styles};

/* ================================================================== */
/* Progress Bar (2 parts: MAIN=track, INDICATOR=fill)                 */
/* ================================================================== */

static const egui_style_t dark_pb_track = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_TRACK,
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

static const egui_style_t dark_pb_indicator = {
        .flags = EGUI_STYLE_PROP_BG_COLOR | EGUI_STYLE_PROP_RADIUS,
        .bg_color = DARK_PRIMARY,
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

static const egui_style_t *const dark_pb_styles[2 * EGUI_STATE_MAX] = {
        &dark_pb_track,     &dark_pb_track,     &dark_pb_track,     &dark_pb_track,     &dark_pb_track,
        &dark_pb_indicator, &dark_pb_indicator, &dark_pb_indicator, &dark_pb_indicator, &dark_pb_indicator,
};

static const egui_widget_style_desc_t dark_pb_desc = {.part_count = 2, .styles = dark_pb_styles};

/* Circular Progress Bar - reuse progress bar styles */
static const egui_widget_style_desc_t dark_cpb_desc = {.part_count = 2, .styles = dark_pb_styles};

/* ================================================================== */
/* Theme definition                                                   */
/* ================================================================== */

const egui_theme_t egui_theme_dark = {
        .name = "dark",
        .button = &dark_btn_desc,
        .label = &dark_label_desc,
        .switch_ctrl = &dark_sw_desc,
        .slider = &dark_slider_desc,
        .checkbox = &dark_cb_desc,
        .card = &dark_card_desc,
        .progress_bar = &dark_pb_desc,
        .circular_progress_bar = &dark_cpb_desc,
};
