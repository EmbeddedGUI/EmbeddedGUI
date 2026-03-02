#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

#define SH_FONT_TITLE     EGUI_FONT_OF(&egui_res_font_montserrat_20_4)
#define SH_FONT_CARD      EGUI_FONT_OF(&egui_res_font_montserrat_14_4)
#define SH_FONT_VALUE     EGUI_FONT_OF(&egui_res_font_montserrat_20_4)
#define SH_FONT_VALUE_DEG EGUI_FONT_OF(&egui_res_font_montserrat_medium_20_4)
#define SH_FONT_ICON      EGUI_FONT_OF(&egui_res_font_materialsymbolsoutlined_regular_14_4)

// Theme icon UTF-8: E518=light_mode, E51C=dark_mode
#define ICON_LIGHT_MODE "\xEE\x94\x98"
#define ICON_DARK_MODE  "\xEE\x94\x9C"

// Title
static egui_view_label_t sh_title;

// Theme toggle button
static egui_view_button_t sh_theme_btn;

// Card 1: Living Room
static egui_view_card_t sh_card1;
static egui_view_label_t sh_card1_name;
static egui_view_switch_t sh_card1_sw;
static egui_view_slider_t sh_card1_slider;

// Card 2: Bedroom
static egui_view_card_t sh_card2;
static egui_view_label_t sh_card2_name;
static egui_view_switch_t sh_card2_sw;
static egui_view_slider_t sh_card2_slider;

// Card 3: Kitchen
static egui_view_card_t sh_card3;
static egui_view_label_t sh_card3_name;
static egui_view_switch_t sh_card3_sw;
static egui_view_slider_t sh_card3_slider;

// Card 4: Temperature
static egui_view_card_t sh_card4;
static egui_view_label_t sh_card4_name;
static egui_view_label_t sh_card4_value;
static egui_view_slider_t sh_card4_slider;

// Staggered entrance: use individual animation sets per card
static egui_animation_translate_t sh_anim_trans[4];
static egui_animation_alpha_t sh_anim_alpha[4];
static egui_animation_set_t sh_anim_set[4];

static const egui_animation_translate_params_t sh_trans_params = {.from_x = 0, .to_x = 0, .from_y = 30, .to_y = 0};
static const egui_animation_alpha_params_t sh_alpha_params = {.from_alpha = 0, .to_alpha = EGUI_ALPHA_100};

static egui_view_t *sh_card_views[4];

// Timer for staggered start
static egui_timer_t sh_stagger_timer;
static int sh_stagger_idx = 0;

static void sh_stagger_timer_callback(egui_timer_t *timer)
{
    (void)timer;
    if (sh_stagger_idx < 4)
    {
        egui_animation_start(EGUI_ANIM_OF(&sh_anim_set[sh_stagger_idx]));
        sh_stagger_idx++;
    }
    if (sh_stagger_idx >= 4)
    {
        egui_timer_stop_timer(&sh_stagger_timer);
    }
}

static void sh_theme_btn_click(egui_view_t *self)
{
    (void)self;
    uicode_toggle_theme();
}

void uicode_init_page_smarthome(egui_view_t *parent)
{
    // Title "Smart Home"
    egui_view_label_init(EGUI_VIEW_OF(&sh_title));
    egui_view_set_position(EGUI_VIEW_OF(&sh_title), 10, 5);
    egui_view_set_size(EGUI_VIEW_OF(&sh_title), 150, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&sh_title), "Smart Home");
    egui_view_label_set_font(EGUI_VIEW_OF(&sh_title), SH_FONT_TITLE);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&sh_title), EGUI_COLOR_MAKE(0x1E, 0x29, 0x3B), EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&sh_title), EGUI_ALIGN_LEFT);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&sh_title));

    // Theme toggle button (icon)
    egui_view_button_init(EGUI_VIEW_OF(&sh_theme_btn));
    egui_view_set_position(EGUI_VIEW_OF(&sh_theme_btn), 200, 5);
    egui_view_set_size(EGUI_VIEW_OF(&sh_theme_btn), 32, 28);
    egui_view_label_set_text(EGUI_VIEW_OF(&sh_theme_btn), ICON_LIGHT_MODE);
    egui_view_label_set_font(EGUI_VIEW_OF(&sh_theme_btn), SH_FONT_ICON);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&sh_theme_btn), sh_theme_btn_click);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&sh_theme_btn));

    // Card 1: Living Room (top-left)
    egui_view_card_init(EGUI_VIEW_OF(&sh_card1));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card1), 8, 42);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card1), 108, 130);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&sh_card1), 8);

    egui_view_label_init(EGUI_VIEW_OF(&sh_card1_name));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card1_name), 5, 8);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card1_name), 98, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&sh_card1_name), "Living Room");
    egui_view_label_set_font(EGUI_VIEW_OF(&sh_card1_name), SH_FONT_CARD);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&sh_card1_name), EGUI_COLOR_MAKE(0x1E, 0x29, 0x3B), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card1), EGUI_VIEW_OF(&sh_card1_name));

    egui_view_switch_init(EGUI_VIEW_OF(&sh_card1_sw));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card1_sw), 28, 34);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card1_sw), 50, 26);
    egui_view_switch_set_checked(EGUI_VIEW_OF(&sh_card1_sw), 1);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card1), EGUI_VIEW_OF(&sh_card1_sw));

    egui_view_slider_init(EGUI_VIEW_OF(&sh_card1_slider));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card1_slider), 8, 72);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card1_slider), 90, 30);
    egui_view_slider_set_value(EGUI_VIEW_OF(&sh_card1_slider), 75);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card1), EGUI_VIEW_OF(&sh_card1_slider));

    egui_view_group_add_child(parent, EGUI_VIEW_OF(&sh_card1));

    // Card 2: Bedroom (top-right)
    egui_view_card_init(EGUI_VIEW_OF(&sh_card2));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card2), 124, 42);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card2), 108, 130);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&sh_card2), 8);

    egui_view_label_init(EGUI_VIEW_OF(&sh_card2_name));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card2_name), 5, 8);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card2_name), 98, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&sh_card2_name), "Bedroom");
    egui_view_label_set_font(EGUI_VIEW_OF(&sh_card2_name), SH_FONT_CARD);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&sh_card2_name), EGUI_COLOR_MAKE(0x1E, 0x29, 0x3B), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card2), EGUI_VIEW_OF(&sh_card2_name));

    egui_view_switch_init(EGUI_VIEW_OF(&sh_card2_sw));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card2_sw), 28, 34);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card2_sw), 50, 26);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card2), EGUI_VIEW_OF(&sh_card2_sw));

    egui_view_slider_init(EGUI_VIEW_OF(&sh_card2_slider));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card2_slider), 8, 72);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card2_slider), 90, 30);
    egui_view_slider_set_value(EGUI_VIEW_OF(&sh_card2_slider), 40);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card2), EGUI_VIEW_OF(&sh_card2_slider));

    egui_view_group_add_child(parent, EGUI_VIEW_OF(&sh_card2));

    // Card 3: Kitchen (bottom-left)
    egui_view_card_init(EGUI_VIEW_OF(&sh_card3));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card3), 8, 180);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card3), 108, 130);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&sh_card3), 8);

    egui_view_label_init(EGUI_VIEW_OF(&sh_card3_name));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card3_name), 5, 8);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card3_name), 98, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&sh_card3_name), "Kitchen");
    egui_view_label_set_font(EGUI_VIEW_OF(&sh_card3_name), SH_FONT_CARD);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&sh_card3_name), EGUI_COLOR_MAKE(0x1E, 0x29, 0x3B), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card3), EGUI_VIEW_OF(&sh_card3_name));

    egui_view_switch_init(EGUI_VIEW_OF(&sh_card3_sw));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card3_sw), 28, 34);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card3_sw), 50, 26);
    egui_view_switch_set_checked(EGUI_VIEW_OF(&sh_card3_sw), 1);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card3), EGUI_VIEW_OF(&sh_card3_sw));

    egui_view_slider_init(EGUI_VIEW_OF(&sh_card3_slider));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card3_slider), 8, 72);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card3_slider), 90, 30);
    egui_view_slider_set_value(EGUI_VIEW_OF(&sh_card3_slider), 60);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card3), EGUI_VIEW_OF(&sh_card3_slider));

    egui_view_group_add_child(parent, EGUI_VIEW_OF(&sh_card3));

    // Card 4: Temperature (bottom-right)
    egui_view_card_init(EGUI_VIEW_OF(&sh_card4));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card4), 124, 180);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card4), 108, 130);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&sh_card4), 8);

    egui_view_label_init(EGUI_VIEW_OF(&sh_card4_name));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card4_name), 5, 8);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card4_name), 98, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&sh_card4_name), "Temp");
    egui_view_label_set_font(EGUI_VIEW_OF(&sh_card4_name), SH_FONT_CARD);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&sh_card4_name), EGUI_COLOR_MAKE(0x1E, 0x29, 0x3B), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card4), EGUI_VIEW_OF(&sh_card4_name));

    egui_view_label_init(EGUI_VIEW_OF(&sh_card4_value));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card4_value), 5, 30);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card4_value), 98, 26);
    egui_view_label_set_text(EGUI_VIEW_OF(&sh_card4_value), "22\xC2\xB0"
                                                            "C");
    egui_view_label_set_font(EGUI_VIEW_OF(&sh_card4_value), SH_FONT_VALUE_DEG);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&sh_card4_value), EGUI_COLOR_MAKE(0x25, 0x63, 0xEB), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card4), EGUI_VIEW_OF(&sh_card4_value));

    egui_view_slider_init(EGUI_VIEW_OF(&sh_card4_slider));
    egui_view_set_position(EGUI_VIEW_OF(&sh_card4_slider), 8, 72);
    egui_view_set_size(EGUI_VIEW_OF(&sh_card4_slider), 90, 30);
    egui_view_slider_set_value(EGUI_VIEW_OF(&sh_card4_slider), 50);
    egui_view_card_add_child(EGUI_VIEW_OF(&sh_card4), EGUI_VIEW_OF(&sh_card4_slider));

    egui_view_group_add_child(parent, EGUI_VIEW_OF(&sh_card4));

    // Set up card view pointers
    sh_card_views[0] = EGUI_VIEW_OF(&sh_card1);
    sh_card_views[1] = EGUI_VIEW_OF(&sh_card2);
    sh_card_views[2] = EGUI_VIEW_OF(&sh_card3);
    sh_card_views[3] = EGUI_VIEW_OF(&sh_card4);

    // Init staggered entrance animations for each card
    for (int i = 0; i < 4; i++)
    {
        egui_animation_translate_init(EGUI_ANIM_OF(&sh_anim_trans[i]));
        egui_animation_translate_params_set(&sh_anim_trans[i], &sh_trans_params);
        egui_animation_duration_set(EGUI_ANIM_OF(&sh_anim_trans[i]), 300);

        egui_animation_alpha_init(EGUI_ANIM_OF(&sh_anim_alpha[i]));
        egui_animation_alpha_params_set(&sh_anim_alpha[i], &sh_alpha_params);
        egui_animation_duration_set(EGUI_ANIM_OF(&sh_anim_alpha[i]), 300);

        egui_animation_set_init(EGUI_ANIM_OF(&sh_anim_set[i]));
        egui_animation_set_set_mask(&sh_anim_set[i], 0, 0, 1, 1, 0);
        egui_animation_set_add_animation(&sh_anim_set[i], EGUI_ANIM_OF(&sh_anim_trans[i]));
        egui_animation_set_add_animation(&sh_anim_set[i], EGUI_ANIM_OF(&sh_anim_alpha[i]));
        egui_animation_duration_set(EGUI_ANIM_OF(&sh_anim_set[i]), 300);
        egui_animation_target_view_set(EGUI_ANIM_OF(&sh_anim_set[i]), sh_card_views[i]);
    }

    // Init stagger timer
    egui_timer_init_timer(&sh_stagger_timer, NULL, sh_stagger_timer_callback);
}

void uicode_page_smarthome_on_enter(void)
{
    // Reset cards to invisible
    for (int i = 0; i < 4; i++)
    {
        egui_view_set_alpha(sh_card_views[i], 0);
    }

    // Start staggered animations via timer (100ms between each card)
    sh_stagger_idx = 0;
    egui_animation_start(EGUI_ANIM_OF(&sh_anim_set[0]));
    sh_stagger_idx = 1;
    egui_timer_start_timer(&sh_stagger_timer, 100, 100);
}

void uicode_page_smarthome_update_theme_icon(void)
{
    const char *icon = uicode_is_dark_theme() ? ICON_DARK_MODE : ICON_LIGHT_MODE;
    egui_view_label_set_text(EGUI_VIEW_OF(&sh_theme_btn), icon);
}
