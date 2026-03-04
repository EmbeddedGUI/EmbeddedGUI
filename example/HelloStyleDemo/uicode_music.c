#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

#define MU_FONT_TITLE  EGUI_FONT_OF(&egui_res_font_montserrat_20_4)
#define MU_FONT_ARTIST EGUI_FONT_OF(&egui_res_font_montserrat_14_4)
#define MU_FONT_TIME   EGUI_FONT_OF(&egui_res_font_montserrat_14_4)
#define MU_FONT_ICON   EGUI_FONT_OF(&egui_res_font_materialsymbolsoutlined_regular_20_4)
#define MU_FONT_ICON_L EGUI_FONT_OF(&egui_res_font_materialsymbolsoutlined_regular_40_4)
#define MU_FONT_ICON_S EGUI_FONT_OF(&egui_res_font_materialsymbolsoutlined_regular_14_4)

#define MU_ICON_PLAY  "\xEE\x80\xB7"
#define MU_ICON_PAUSE "\xEE\x80\xB4"
#define MU_ICON_PREV  "\xEE\x81\x85"
#define MU_ICON_NEXT  "\xEE\x81\x84"

#define MU_COL_ACCENT     EGUI_COLOR_MAKE(0x7C, 0xB8, 0xFF)
#define MU_COL_CARD       EGUI_COLOR_MAKE(0x14, 0x18, 0x24)
#define MU_COL_CTRL       EGUI_COLOR_MAKE(0xD0, 0xD7, 0xE8)
#define MU_COL_MUTED      EGUI_COLOR_MAKE(0x78, 0x84, 0x9E)
#define MU_COL_ARTIST     EGUI_COLOR_MAKE(0x9E, 0xA8, 0xC0)
#define MU_COL_RING_OUTER EGUI_COLOR_MAKE(0x2A, 0x31, 0x44)
#define MU_COL_RING_INNER EGUI_COLOR_MAKE(0x35, 0x3E, 0x55)

static egui_view_card_t mu_album_card;
static egui_view_activity_ring_t mu_disc_rings;
static egui_view_spinner_t mu_disc_spinner;
static egui_view_label_t mu_disc_center_icon;
static egui_view_label_t mu_play_state;
static egui_view_label_t mu_song_title;
static egui_view_label_t mu_artist;
static egui_view_label_t mu_time_cur;
static egui_view_label_t mu_time_total;
static egui_view_slider_t mu_progress;
static egui_view_label_t mu_ctrl_prev;
static egui_view_label_t mu_ctrl_play;
static egui_view_label_t mu_ctrl_next;

static uint8_t mu_is_playing = 0;
static egui_timer_t mu_progress_timer;
static int mu_current_sec = 83;
static int mu_total_sec = 225;

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

static char mu_time_cur_buf[8];
static char mu_time_total_buf[8];

static void mu_format_time(char *buf, int seconds)
{
    int min = seconds / 60;
    int sec = seconds % 60;
    sprintf(buf, "%d:%02d", min, sec);
}

static void mu_update_time_display(void)
{
    mu_format_time(mu_time_cur_buf, mu_current_sec);
    mu_format_time(mu_time_total_buf, mu_total_sec);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_time_cur), mu_time_cur_buf);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_time_total), mu_time_total_buf);
}

static void mu_progress_value_changed(egui_view_t *self, uint8_t value)
{
    (void)self;
    int sec = (value * mu_total_sec) / 100;
    if (sec < 0)
    {
        sec = 0;
    }
    if (sec > mu_total_sec)
    {
        sec = mu_total_sec;
    }
    mu_current_sec = sec;
    mu_update_time_display();
}

static void mu_progress_timer_callback(egui_timer_t *timer)
{
    (void)timer;
    if (!mu_is_playing)
        return;
    mu_current_sec++;
    if (mu_current_sec >= mu_total_sec)
    {
        mu_current_sec = mu_total_sec;
        mu_is_playing = 0;
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_ctrl_play), MU_ICON_PLAY);
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_disc_center_icon), MU_ICON_PLAY);
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_play_state), "Paused");
        egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_play_state), MU_COL_MUTED, EGUI_ALPHA_100);
        egui_view_spinner_stop(EGUI_VIEW_OF(&mu_disc_spinner));
        egui_timer_stop_timer(&mu_progress_timer);
    }
    int progress = (mu_current_sec * 100) / mu_total_sec;
    egui_view_slider_set_value(EGUI_VIEW_OF(&mu_progress), progress);
    mu_update_time_display();
}

static void mu_btn_play_click(egui_view_t *self)
{
    (void)self;
    mu_is_playing = !mu_is_playing;
    if (mu_is_playing)
    {
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_ctrl_play), MU_ICON_PAUSE);
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_disc_center_icon), MU_ICON_PAUSE);
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_play_state), "Playing");
        egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_play_state), MU_COL_ACCENT, EGUI_ALPHA_100);
        egui_view_spinner_start(EGUI_VIEW_OF(&mu_disc_spinner));
        egui_timer_start_timer(&mu_progress_timer, 1000, 1000);
    }
    else
    {
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_ctrl_play), MU_ICON_PLAY);
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_disc_center_icon), MU_ICON_PLAY);
        egui_view_label_set_text(EGUI_VIEW_OF(&mu_play_state), "Paused");
        egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_play_state), MU_COL_MUTED, EGUI_ALPHA_100);
        egui_view_spinner_stop(EGUI_VIEW_OF(&mu_disc_spinner));
        egui_timer_stop_timer(&mu_progress_timer);
    }
}

static void mu_apply_song(void)
{
    const mu_song_info_t *song = &mu_songs[mu_current_song];
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_song_title), song->title);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_artist), song->artist);
    mu_total_sec = song->duration_sec;
    mu_current_sec = 0;
    egui_view_slider_set_value(EGUI_VIEW_OF(&mu_progress), 0);
    mu_update_time_display();
}

static void mu_switch_song(int direction)
{
    mu_current_song = (mu_current_song + direction + MU_SONG_COUNT) % MU_SONG_COUNT;
    mu_apply_song();
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
    egui_view_card_init(EGUI_VIEW_OF(&mu_album_card));
    egui_view_set_position(EGUI_VIEW_OF(&mu_album_card), 40, 10);
    egui_view_set_size(EGUI_VIEW_OF(&mu_album_card), 160, 160);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&mu_album_card), 16);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&mu_album_card), MU_COL_CARD, EGUI_ALPHA_100);

    egui_view_activity_ring_init(EGUI_VIEW_OF(&mu_disc_rings));
    egui_view_set_position(EGUI_VIEW_OF(&mu_disc_rings), 14, 14);
    egui_view_set_size(EGUI_VIEW_OF(&mu_disc_rings), 132, 132);
    egui_view_activity_ring_set_ring_count(EGUI_VIEW_OF(&mu_disc_rings), 2);
    egui_view_activity_ring_set_stroke_width(EGUI_VIEW_OF(&mu_disc_rings), 9);
    egui_view_activity_ring_set_ring_gap(EGUI_VIEW_OF(&mu_disc_rings), 5);
    egui_view_activity_ring_set_show_round_cap(EGUI_VIEW_OF(&mu_disc_rings), 0);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&mu_disc_rings), 0, 100);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&mu_disc_rings), 1, 100);
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&mu_disc_rings), 0, MU_COL_RING_OUTER);
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&mu_disc_rings), 1, MU_COL_RING_INNER);
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&mu_disc_rings), 0, MU_COL_CARD);
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&mu_disc_rings), 1, MU_COL_CARD);
    egui_view_card_add_child(EGUI_VIEW_OF(&mu_album_card), EGUI_VIEW_OF(&mu_disc_rings));

    egui_view_spinner_init(EGUI_VIEW_OF(&mu_disc_spinner));
    egui_view_set_position(EGUI_VIEW_OF(&mu_disc_spinner), 20, 20);
    egui_view_set_size(EGUI_VIEW_OF(&mu_disc_spinner), 120, 120);
    egui_view_spinner_set_color(EGUI_VIEW_OF(&mu_disc_spinner), MU_COL_ACCENT);
    egui_view_card_add_child(EGUI_VIEW_OF(&mu_album_card), EGUI_VIEW_OF(&mu_disc_spinner));

    egui_view_label_init(EGUI_VIEW_OF(&mu_disc_center_icon));
    egui_view_set_position(EGUI_VIEW_OF(&mu_disc_center_icon), 44, 44);
    egui_view_set_size(EGUI_VIEW_OF(&mu_disc_center_icon), 72, 72);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_disc_center_icon), MU_ICON_PLAY);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_disc_center_icon), MU_FONT_ICON_L);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_disc_center_icon), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_disc_center_icon), EGUI_ALIGN_CENTER);
    egui_view_card_add_child(EGUI_VIEW_OF(&mu_album_card), EGUI_VIEW_OF(&mu_disc_center_icon));
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_album_card));

    egui_view_label_init(EGUI_VIEW_OF(&mu_play_state));
    egui_view_set_position(EGUI_VIEW_OF(&mu_play_state), 10, 174);
    egui_view_set_size(EGUI_VIEW_OF(&mu_play_state), 220, 20);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_play_state), "Paused");
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_play_state), MU_FONT_ARTIST);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_play_state), MU_COL_MUTED, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_play_state), EGUI_ALIGN_CENTER);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_play_state));

    egui_view_label_init(EGUI_VIEW_OF(&mu_song_title));
    egui_view_set_position(EGUI_VIEW_OF(&mu_song_title), 10, 192);
    egui_view_set_size(EGUI_VIEW_OF(&mu_song_title), 220, 26);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_song_title), "Midnight Dreams");
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_song_title), MU_FONT_TITLE);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_song_title), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_song_title), EGUI_ALIGN_CENTER);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_song_title));

    egui_view_label_init(EGUI_VIEW_OF(&mu_artist));
    egui_view_set_position(EGUI_VIEW_OF(&mu_artist), 10, 216);
    egui_view_set_size(EGUI_VIEW_OF(&mu_artist), 220, 17);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_artist), "The Synthwave");
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_artist), MU_FONT_ARTIST);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_artist), MU_COL_ARTIST, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_artist), EGUI_ALIGN_CENTER);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_artist));

    egui_view_label_init(EGUI_VIEW_OF(&mu_time_cur));
    egui_view_set_position(EGUI_VIEW_OF(&mu_time_cur), 10, 238);
    egui_view_set_size(EGUI_VIEW_OF(&mu_time_cur), 45, 15);
    mu_format_time(mu_time_cur_buf, mu_current_sec);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_time_cur), mu_time_cur_buf);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_time_cur), MU_FONT_TIME);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_time_cur), MU_COL_MUTED, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_time_cur), EGUI_ALIGN_LEFT);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_time_cur));

    egui_view_slider_init(EGUI_VIEW_OF(&mu_progress));
    egui_view_set_position(EGUI_VIEW_OF(&mu_progress), 57, 238);
    egui_view_set_size(EGUI_VIEW_OF(&mu_progress), 130, 18);
    egui_view_slider_set_value(EGUI_VIEW_OF(&mu_progress), 37);
    egui_view_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&mu_progress), mu_progress_value_changed);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_progress));

    egui_view_label_init(EGUI_VIEW_OF(&mu_time_total));
    egui_view_set_position(EGUI_VIEW_OF(&mu_time_total), 185, 238);
    egui_view_set_size(EGUI_VIEW_OF(&mu_time_total), 45, 15);
    mu_format_time(mu_time_total_buf, mu_total_sec);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_time_total), mu_time_total_buf);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_time_total), MU_FONT_TIME);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_time_total), MU_COL_MUTED, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_time_total), EGUI_ALIGN_RIGHT);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_time_total));

    egui_view_label_init(EGUI_VIEW_OF(&mu_ctrl_prev));
    egui_view_set_position(EGUI_VIEW_OF(&mu_ctrl_prev), 58, 266);
    egui_view_set_size(EGUI_VIEW_OF(&mu_ctrl_prev), 36, 34);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_ctrl_prev), MU_ICON_PREV);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_ctrl_prev), MU_FONT_ICON);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_ctrl_prev), MU_COL_CTRL, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_ctrl_prev), EGUI_ALIGN_CENTER);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&mu_ctrl_prev), mu_btn_prev_click);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_ctrl_prev));

    egui_view_label_init(EGUI_VIEW_OF(&mu_ctrl_play));
    egui_view_set_position(EGUI_VIEW_OF(&mu_ctrl_play), 97, 263);
    egui_view_set_size(EGUI_VIEW_OF(&mu_ctrl_play), 46, 38);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_ctrl_play), MU_ICON_PLAY);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_ctrl_play), MU_FONT_ICON);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_ctrl_play), MU_COL_ACCENT, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_ctrl_play), EGUI_ALIGN_CENTER);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&mu_ctrl_play), mu_btn_play_click);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_ctrl_play));

    egui_view_label_init(EGUI_VIEW_OF(&mu_ctrl_next));
    egui_view_set_position(EGUI_VIEW_OF(&mu_ctrl_next), 146, 266);
    egui_view_set_size(EGUI_VIEW_OF(&mu_ctrl_next), 36, 34);
    egui_view_label_set_text(EGUI_VIEW_OF(&mu_ctrl_next), MU_ICON_NEXT);
    egui_view_label_set_font(EGUI_VIEW_OF(&mu_ctrl_next), MU_FONT_ICON);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&mu_ctrl_next), MU_COL_CTRL, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&mu_ctrl_next), EGUI_ALIGN_CENTER);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&mu_ctrl_next), mu_btn_next_click);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&mu_ctrl_next));

    egui_timer_init_timer(&mu_progress_timer, NULL, mu_progress_timer_callback);
}

void uicode_page_music_on_enter(void)
{
    if (!mu_is_playing)
    {
        mu_btn_play_click(NULL);
    }
}