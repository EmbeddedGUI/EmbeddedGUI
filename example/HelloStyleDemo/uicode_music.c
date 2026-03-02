#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

#define MU_FONT_TITLE  EGUI_FONT_OF(&egui_res_font_montserrat_20_4)
#define MU_FONT_ARTIST EGUI_FONT_OF(&egui_res_font_montserrat_16_4)
#define MU_FONT_TIME   EGUI_FONT_OF(&egui_res_font_montserrat_12_4)
#define MU_FONT_ICON   EGUI_FONT_OF(&egui_res_font_materialsymbolsoutlined_regular_20_4)
#define MU_FONT_ICON_S EGUI_FONT_OF(&egui_res_font_materialsymbolsoutlined_regular_14_4)

// Material icon UTF-8 sequences
// E037=play_arrow, E034=pause, E044=skip_previous, E045=skip_next, E050=volume_up
#define MU_ICON_PLAY  "\xEE\x80\xB7"
#define MU_ICON_PAUSE "\xEE\x80\xB4"
#define MU_ICON_PREV  "\xEE\x81\x84"
#define MU_ICON_NEXT  "\xEE\x81\x85"
#define MU_ICON_VOL   "\xEE\x81\x90"

// Spinner for rotating arc around album
static egui_view_spinner_t mu_spinner;

// Album cover image
static egui_view_image_t mu_album_img;

// Song info
static egui_view_label_t mu_song_title;
static egui_view_label_t mu_artist;

// Progress
static egui_view_slider_t mu_progress;
static egui_view_label_t mu_time_cur;
static egui_view_label_t mu_time_total;

// Controls
static egui_view_button_t mu_btn_prev;
static egui_view_button_t mu_btn_play;
static egui_view_button_t mu_btn_next;

// Volume
static egui_view_label_t mu_vol_label;
static egui_view_slider_t mu_volume;

// Playback state
static uint8_t mu_is_playing = 0;
static egui_timer_t mu_progress_timer;
static int mu_current_sec = 83; // 1:23
static int mu_total_sec = 225;  // 3:45

// Song database
typedef struct
{
    const char *title;
    const char *artist;
    int duration_sec;
} mu_song_info_t;

static const mu_song_info_t mu_songs[] = {
        {"Midnight Dreams", "The Synthwave", 225},
        {"Neon Lights", "Cyber Pulse", 198},
        {"Ocean Waves", "Ambient Sky", 252},
};
#define MU_SONG_COUNT 3
static int mu_current_song = 0;

// Fade animation for song switching
static egui_animation_alpha_t mu_fade_out_anim;
static egui_animation_alpha_t mu_fade_in_anim;
static const egui_animation_alpha_params_t mu_fade_out_params = {.from_alpha = EGUI_ALPHA_100, .to_alpha = 0};
static const egui_animation_alpha_params_t mu_fade_in_params = {.from_alpha = 0, .to_alpha = EGUI_ALPHA_100};

// Text buffer for time display
static char mu_time_cur_buf[8];
static char mu_time_total_buf[8];

// Forward declarations
static void mu_update_time_display(void);

static void mu_format_time(char *buf, int seconds)
{
    int min = seconds / 60;
    int sec = seconds % 60;
    sprintf(buf, "%d:%02d", min, sec);
}

static void mu_progress_timer_callback(egui_timer_t *timer)
{
    (void)timer;
    if (!mu_is_playing)
    {
        return;
    }
    mu_current_sec++;
    if (mu_current_sec >= mu_total_sec)
    {
        mu_current_sec = mu_total_sec;
        mu_is_playing = 0;
        egui_view_spinner_stop(EGUI_VIEW_OF(&mu_spinner));
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_btn_play), MU_ICON_PLAY);
        egui_timer_stop_timer(&mu_progress_timer);
    }
    // Update progress slider (0-100)
    int progress = (mu_current_sec * 100) / mu_total_sec;
    egui_view_slider_set_value(EGUI_VIEW_OF(&mu_progress), progress);
    mu_update_time_display();
}

static void mu_update_time_display(void)
{
    mu_format_time(mu_time_cur_buf, mu_current_sec);
    mu_format_time(mu_time_total_buf, mu_total_sec);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_time_cur), mu_time_cur_buf);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_time_total), mu_time_total_buf);
}

static void mu_btn_play_click(egui_view_t *self)
{
    (void)self;
    mu_is_playing = !mu_is_playing;
    if (mu_is_playing)
    {
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_btn_play), MU_ICON_PAUSE);
        egui_view_spinner_start(EGUI_VIEW_OF(&mu_spinner));
        egui_timer_start_timer(&mu_progress_timer, 1000, 1000);
    }
    else
    {
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_btn_play), MU_ICON_PLAY);
        egui_view_spinner_stop(EGUI_VIEW_OF(&mu_spinner));
        egui_timer_stop_timer(&mu_progress_timer);
    }
}

// Fade-out animation end handler: switch song data then fade in
static void mu_fade_out_end(egui_animation_t *anim)
{
    (void)anim;
    // Update song info
    const mu_song_info_t *song = &mu_songs[mu_current_song];
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_song_title), song->title);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_artist), song->artist);
    mu_total_sec = song->duration_sec;
    mu_current_sec = 0;
    egui_view_slider_set_value(EGUI_VIEW_OF(&mu_progress), 0);
    mu_update_time_display();

    // Start fade-in
    egui_animation_start(EGUI_ANIM_OF(&mu_fade_in_anim));
}

static const egui_animation_handle_t mu_fade_out_handle = {
        .start = NULL,
        .end = mu_fade_out_end,
        .repeat = NULL,
};

static void mu_switch_song(int direction)
{
    mu_current_song = (mu_current_song + direction + MU_SONG_COUNT) % MU_SONG_COUNT;
    // Start fade-out animation on song title
    egui_animation_start(EGUI_ANIM_OF(&mu_fade_out_anim));
}

static void mu_btn_prev_click(egui_view_t *self)
{
    (void)self;
    mu_switch_song(-1);
}

static void mu_btn_next_click(egui_view_t *self)
{
    (void)self;
    mu_switch_song(1);
}

void uicode_init_page_music(egui_view_t *parent)
{
    // Spinner for rotating arc (background, centered)
    egui_view_spinner_init(EGUI_VIEW_OF(&mu_spinner));
    egui_view_set_position(EGUI_VIEW_OF(&mu_spinner), 55, 5);
    egui_view_set_size(EGUI_VIEW_OF(&mu_spinner), 130, 130);
    egui_view_spinner_set_color(EGUI_VIEW_OF(&mu_spinner), EGUI_COLOR_MAKE(0x9C, 0x6C, 0xE0));
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_spinner));

    // Album cover image (on top of spinner, centered)
    egui_view_image_init(EGUI_VIEW_OF(&mu_album_img));
    egui_view_set_position(EGUI_VIEW_OF(&mu_album_img), 80, 30);
    egui_view_set_size(EGUI_VIEW_OF(&mu_album_img), 80, 80);
    egui_view_image_set_image(EGUI_VIEW_OF(&mu_album_img), (egui_image_t *)&egui_res_image_album_cover_rgb565_4);
    egui_view_image_set_image_type(EGUI_VIEW_OF(&mu_album_img), EGUI_VIEW_IMAGE_TYPE_NORMAL);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_album_img));

    // Song title
    egui_view_label_init(EGUI_VIEW_OF(&mu_song_title));
    egui_view_set_position(EGUI_VIEW_OF(&mu_song_title), 10, 145);
    egui_view_set_size(EGUI_VIEW_OF(&mu_song_title), 220, 24);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_song_title), "Midnight Dreams");
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_song_title), MU_FONT_TITLE);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_song_title), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_song_title));

    // Artist
    egui_view_label_init(EGUI_VIEW_OF(&mu_artist));
    egui_view_set_position(EGUI_VIEW_OF(&mu_artist), 10, 170);
    egui_view_set_size(EGUI_VIEW_OF(&mu_artist), 220, 20);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_artist), "The Synthwave");
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_artist), MU_FONT_ARTIST);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_artist), EGUI_COLOR_MAKE(0x99, 0x99, 0xBB), EGUI_ALPHA_100);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_artist));

    // Progress slider
    egui_view_slider_init(EGUI_VIEW_OF(&mu_progress));
    egui_view_set_position(EGUI_VIEW_OF(&mu_progress), 10, 198);
    egui_view_set_size(EGUI_VIEW_OF(&mu_progress), 220, 26);
    egui_view_slider_set_value(EGUI_VIEW_OF(&mu_progress), 37);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_progress));

    // Time current
    egui_view_label_init(EGUI_VIEW_OF(&mu_time_cur));
    egui_view_set_position(EGUI_VIEW_OF(&mu_time_cur), 10, 226);
    egui_view_set_size(EGUI_VIEW_OF(&mu_time_cur), 50, 16);
    mu_format_time(mu_time_cur_buf, mu_current_sec);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_time_cur), mu_time_cur_buf);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_time_cur), MU_FONT_TIME);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_time_cur), EGUI_COLOR_MAKE(0x99, 0x99, 0xBB), EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_time_cur), EGUI_ALIGN_LEFT);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_time_cur));

    // Time total
    egui_view_label_init(EGUI_VIEW_OF(&mu_time_total));
    egui_view_set_position(EGUI_VIEW_OF(&mu_time_total), 180, 226);
    egui_view_set_size(EGUI_VIEW_OF(&mu_time_total), 50, 16);
    mu_format_time(mu_time_total_buf, mu_total_sec);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_time_total), mu_time_total_buf);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_time_total), MU_FONT_TIME);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_time_total), EGUI_COLOR_MAKE(0x99, 0x99, 0xBB), EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_time_total), EGUI_ALIGN_RIGHT);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_time_total));

    // Control buttons
    egui_view_button_init(EGUI_VIEW_OF(&mu_btn_prev));
    egui_view_set_position(EGUI_VIEW_OF(&mu_btn_prev), 30, 245);
    egui_view_set_size(EGUI_VIEW_OF(&mu_btn_prev), 50, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_btn_prev), MU_ICON_PREV);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_btn_prev), MU_FONT_ICON);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&mu_btn_prev), mu_btn_prev_click);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_btn_prev));

    egui_view_button_init(EGUI_VIEW_OF(&mu_btn_play));
    egui_view_set_position(EGUI_VIEW_OF(&mu_btn_play), 95, 245);
    egui_view_set_size(EGUI_VIEW_OF(&mu_btn_play), 50, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_btn_play), MU_ICON_PLAY);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_btn_play), MU_FONT_ICON);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&mu_btn_play), mu_btn_play_click);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_btn_play));

    egui_view_button_init(EGUI_VIEW_OF(&mu_btn_next));
    egui_view_set_position(EGUI_VIEW_OF(&mu_btn_next), 160, 245);
    egui_view_set_size(EGUI_VIEW_OF(&mu_btn_next), 50, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_btn_next), MU_ICON_NEXT);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_btn_next), MU_FONT_ICON);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&mu_btn_next), mu_btn_next_click);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_btn_next));

    // Volume label
    egui_view_label_init(EGUI_VIEW_OF(&mu_vol_label));
    egui_view_set_position(EGUI_VIEW_OF(&mu_vol_label), 10, 285);
    egui_view_set_size(EGUI_VIEW_OF(&mu_vol_label), 30, 16);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_vol_label), MU_ICON_VOL);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_vol_label), MU_FONT_ICON_S);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_vol_label), EGUI_COLOR_MAKE(0x99, 0x99, 0xBB), EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_vol_label), EGUI_ALIGN_LEFT);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_vol_label));

    // Volume slider
    egui_view_slider_init(EGUI_VIEW_OF(&mu_volume));
    egui_view_set_position(EGUI_VIEW_OF(&mu_volume), 45, 281);
    egui_view_set_size(EGUI_VIEW_OF(&mu_volume), 185, 26);
    egui_view_slider_set_value(EGUI_VIEW_OF(&mu_volume), 65);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_volume));

    // Init progress timer
    egui_timer_init_timer(&mu_progress_timer, NULL, mu_progress_timer_callback);

    // Init fade animations for song switching
    egui_animation_alpha_init(EGUI_ANIM_OF(&mu_fade_out_anim));
    egui_animation_alpha_params_set(&mu_fade_out_anim, &mu_fade_out_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&mu_fade_out_anim), 200);
    egui_animation_target_view_set(EGUI_ANIM_OF(&mu_fade_out_anim), EGUI_VIEW_OF(&mu_song_title));
    egui_animation_handle_set(EGUI_ANIM_OF(&mu_fade_out_anim), &mu_fade_out_handle);

    egui_animation_alpha_init(EGUI_ANIM_OF(&mu_fade_in_anim));
    egui_animation_alpha_params_set(&mu_fade_in_anim, &mu_fade_in_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&mu_fade_in_anim), 200);
    egui_animation_target_view_set(EGUI_ANIM_OF(&mu_fade_in_anim), EGUI_VIEW_OF(&mu_song_title));
}

void uicode_page_music_on_enter(void)
{
    // Auto-start playback when entering music page
    if (!mu_is_playing)
    {
        mu_btn_play_click(NULL);
    }
}
