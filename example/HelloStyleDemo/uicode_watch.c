#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

#define WT_FONT_CLOCK    EGUI_FONT_OF(&egui_res_font_montserrat_32_4)
#define WT_FONT_DATE     EGUI_FONT_OF(&egui_res_font_montserrat_16_4)
#define WT_FONT_CARD     EGUI_FONT_OF(&egui_res_font_montserrat_14_4)
#define WT_FONT_ICON     EGUI_FONT_OF(&egui_res_font_materialsymbolsoutlined_regular_14_4)
#define WT_FONT_CARD_DEG EGUI_FONT_OF(&egui_res_font_montserrat_medium_14_4)

// Theme icon UTF-8: E518=light_mode, E51C=dark_mode
#define ICON_LIGHT_MODE "\xEE\x94\x98"
#define ICON_DARK_MODE  "\xEE\x94\x9C"

// Digital clock
static egui_view_digital_clock_t wt_clock;

// Date label
static egui_view_label_t wt_date;

// Activity ring
static egui_view_activity_ring_t wt_ring;

// Heart rate
static egui_view_heart_rate_t wt_heart;

// Bottom cards
static egui_view_card_t wt_weather_card;
static egui_view_label_t wt_weather_label;
static egui_view_button_t wt_theme_btn;

// Ring growth animation
static egui_timer_t wt_ring_timer;
static int wt_ring_frame = 0;
#define WT_RING_FRAMES   25
#define WT_RING_INTERVAL 40
static const uint8_t wt_ring_targets[] = {75, 60, 45};

// Weather card fade-in animation
static egui_animation_alpha_t wt_weather_fade;
static const egui_animation_alpha_params_t wt_weather_fade_params = {.from_alpha = 0, .to_alpha = EGUI_ALPHA_100};

static void wt_ring_timer_callback(egui_timer_t *timer)
{
    (void)timer;
    wt_ring_frame++;
    if (wt_ring_frame > WT_RING_FRAMES)
    {
        egui_timer_stop_timer(&wt_ring_timer);
        return;
    }

    int progress = (wt_ring_frame * 100) / WT_RING_FRAMES;
    // decelerate easing
    int t_inv = 100 - progress;
    int decel = 100 - (t_inv * t_inv) / 100;

    for (int i = 0; i < 3; i++)
    {
        uint8_t val = (uint8_t)((wt_ring_targets[i] * decel) / 100);
        egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wt_ring), i, val);
    }
}

static void wt_theme_btn_click(egui_view_t *self)
{
    (void)self;
    uicode_toggle_theme();
}

void uicode_init_page_watch(egui_view_t *parent)
{
    // Digital clock (top center)
    egui_view_digital_clock_init(EGUI_VIEW_OF(&wt_clock));
    egui_view_set_position(EGUI_VIEW_OF(&wt_clock), 30, 8);
    egui_view_set_size(EGUI_VIEW_OF(&wt_clock), 180, 42);
    egui_view_digital_clock_set_time(EGUI_VIEW_OF(&wt_clock), 12, 45, 0);
    egui_view_digital_clock_set_format(EGUI_VIEW_OF(&wt_clock), 1);
    egui_view_label_set_font(EGUI_VIEW_OF(&wt_clock), WT_FONT_CLOCK);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&wt_clock), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&wt_clock));

    // Date label
    egui_view_label_init(EGUI_VIEW_OF(&wt_date));
    egui_view_set_position(EGUI_VIEW_OF(&wt_date), 50, 52);
    egui_view_set_size(EGUI_VIEW_OF(&wt_date), 140, 22);
    egui_view_label_set_text(EGUI_VIEW_OF(&wt_date), "Wed, Feb 25");
    egui_view_label_set_font(EGUI_VIEW_OF(&wt_date), WT_FONT_DATE);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&wt_date), EGUI_COLOR_MAKE(0x88, 0x88, 0xAA), EGUI_ALPHA_100);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&wt_date));

    // Activity ring (center) - start at 0, will animate to target
    egui_view_activity_ring_init(EGUI_VIEW_OF(&wt_ring));
    egui_view_set_position(EGUI_VIEW_OF(&wt_ring), 50, 72);
    egui_view_set_size(EGUI_VIEW_OF(&wt_ring), 140, 140);
    egui_view_activity_ring_set_ring_count(EGUI_VIEW_OF(&wt_ring), 3);
    egui_view_activity_ring_set_stroke_width(EGUI_VIEW_OF(&wt_ring), 14);
    egui_view_activity_ring_set_ring_gap(EGUI_VIEW_OF(&wt_ring), 3);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wt_ring), 0, 0);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wt_ring), 1, 0);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wt_ring), 2, 0);
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&wt_ring), 0, EGUI_COLOR_MAKE(0xEF, 0x44, 0x44));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&wt_ring), 1, EGUI_COLOR_MAKE(0x10, 0xB9, 0x81));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&wt_ring), 2, EGUI_COLOR_MAKE(0x38, 0xBD, 0xF8));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wt_ring), 0, EGUI_COLOR_MAKE(0x3B, 0x15, 0x15));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wt_ring), 1, EGUI_COLOR_MAKE(0x0A, 0x2E, 0x20));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&wt_ring), 2, EGUI_COLOR_MAKE(0x0E, 0x2F, 0x3E));
    egui_view_activity_ring_set_show_round_cap(EGUI_VIEW_OF(&wt_ring), 1);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&wt_ring));

    // Heart rate
    egui_view_heart_rate_init(EGUI_VIEW_OF(&wt_heart));
    egui_view_set_position(EGUI_VIEW_OF(&wt_heart), 70, 220);
    egui_view_set_size(EGUI_VIEW_OF(&wt_heart), 100, 40);
    egui_view_heart_rate_set_bpm(EGUI_VIEW_OF(&wt_heart), 72);
    egui_view_heart_rate_set_animate(EGUI_VIEW_OF(&wt_heart), 1);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&wt_heart));

    // Weather card (bottom-left) - start invisible for fade-in
    egui_view_card_init(EGUI_VIEW_OF(&wt_weather_card));
    egui_view_set_position(EGUI_VIEW_OF(&wt_weather_card), 10, 270);
    egui_view_set_size(EGUI_VIEW_OF(&wt_weather_card), 105, 40);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&wt_weather_card), 8);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&wt_weather_card), EGUI_COLOR_MAKE(0x33, 0x41, 0x55), EGUI_ALPHA_100);
    egui_view_set_alpha(EGUI_VIEW_OF(&wt_weather_card), 0);

    egui_view_label_init(EGUI_VIEW_OF(&wt_weather_label));
    egui_view_set_position(EGUI_VIEW_OF(&wt_weather_label), 5, 5);
    egui_view_set_size(EGUI_VIEW_OF(&wt_weather_label), 95, 28);
    egui_view_label_set_text(EGUI_VIEW_OF(&wt_weather_label), "18\xC2\xB0"
                                                              "C Sunny");
    egui_view_label_set_font(EGUI_VIEW_OF(&wt_weather_label), WT_FONT_CARD_DEG);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&wt_weather_label), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&wt_weather_card), EGUI_VIEW_OF(&wt_weather_label));

    egui_view_group_add_child(parent, EGUI_VIEW_OF(&wt_weather_card));

    // Theme toggle button (bottom-right, icon)
    egui_view_button_init(EGUI_VIEW_OF(&wt_theme_btn));
    egui_view_set_position(EGUI_VIEW_OF(&wt_theme_btn), 125, 270);
    egui_view_set_size(EGUI_VIEW_OF(&wt_theme_btn), 105, 40);
    egui_view_label_set_text(EGUI_VIEW_OF(&wt_theme_btn), ICON_LIGHT_MODE);
    egui_view_label_set_font(EGUI_VIEW_OF(&wt_theme_btn), WT_FONT_ICON);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&wt_theme_btn), wt_theme_btn_click);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&wt_theme_btn));

    // Init ring growth timer
    egui_timer_init_timer(&wt_ring_timer, NULL, wt_ring_timer_callback);

    // Init weather fade animation
    egui_animation_alpha_init(EGUI_ANIM_OF(&wt_weather_fade));
    egui_animation_alpha_params_set(&wt_weather_fade, &wt_weather_fade_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&wt_weather_fade), 500);
    egui_animation_target_view_set(EGUI_ANIM_OF(&wt_weather_fade), EGUI_VIEW_OF(&wt_weather_card));
}

void uicode_page_watch_update_theme_icon(void)
{
    const char *icon = uicode_is_dark_theme() ? ICON_DARK_MODE : ICON_LIGHT_MODE;
    egui_view_label_set_text(EGUI_VIEW_OF(&wt_theme_btn), icon);
}

void uicode_page_watch_on_enter(void)
{
    // Reset ring values to 0
    wt_ring_frame = 0;
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wt_ring), 0, 0);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wt_ring), 1, 0);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&wt_ring), 2, 0);

    // Start ring growth animation
    egui_timer_start_timer(&wt_ring_timer, WT_RING_INTERVAL, WT_RING_INTERVAL);

    // Start weather card fade-in
    egui_view_set_alpha(EGUI_VIEW_OF(&wt_weather_card), 0);
    egui_animation_start(EGUI_ANIM_OF(&wt_weather_fade));
}
