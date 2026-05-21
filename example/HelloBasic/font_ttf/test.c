/*
 * Runtime TTF Font Demo
 *
 * Demonstrates egui_font_ttf_t: a runtime TTF font backed by stb_truetype.
 *
 * Layout:
 *   - Title bar at the top.
 *   - Three text labels showing the same string rendered at different scales
 *     using the TTF font (small / medium / large).
 *   - A status label at the bottom showing whether the font loaded.
 *
 * The TTF binary is loaded at init time from the file system
 * (scripts/tools/build_in/DejaVuSans.ttf, relative to the working directory).
 * A graceful fallback message is shown when the file is not available.
 */

#include "egui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uicode_disp0.h"
#include "font/egui_font_ttf.h"

#define SCREEN_W EGUI_CONFIG_SCREEN_WIDTH
#define SCREEN_H EGUI_CONFIG_SCREEN_HEIGHT

/* ------------------------------------------------------------------ */
/* TTF font instances (three sizes)                                    */
/* ------------------------------------------------------------------ */

static uint8_t         *s_ttf_buf;
static uint32_t         s_ttf_size;

static egui_font_ttf_t  s_font_sm;  /* Small  – 16 px */
static egui_font_ttf_t  s_font_md;  /* Medium – 24 px */
static egui_font_ttf_t  s_font_lg;  /* Large  – 32 px */

/* ------------------------------------------------------------------ */
/* View objects                                                        */
/* ------------------------------------------------------------------ */

static egui_view_label_t s_title_label;
static egui_view_label_t s_label_sm;
static egui_view_label_t s_label_md;
static egui_view_label_t s_label_lg;
static egui_view_label_t s_status_label;

/* ------------------------------------------------------------------ */
/* Title (compile-time params)                                         */
/* ------------------------------------------------------------------ */

EGUI_VIEW_LABEL_PARAMS_INIT(s_title_params, 0, 4, SCREEN_W, 28,
                            "Runtime TTF Font",
                            EGUI_CONFIG_FONT_DEFAULT,
                            EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);

/* ------------------------------------------------------------------ */
/* TTF loader                                                          */
/* ------------------------------------------------------------------ */

static int load_ttf_font(void)
{
    const char *path = "scripts/tools/build_in/DejaVuSans.ttf";
    FILE *f = fopen(path, "rb");
    if (f == NULL)
    {
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0)
    {
        fclose(f);
        return 0;
    }
    s_ttf_buf = (uint8_t *)malloc((size_t)sz);
    if (s_ttf_buf == NULL)
    {
        fclose(f);
        return 0;
    }
    s_ttf_size = (uint32_t)sz;
    if (fread(s_ttf_buf, 1, (size_t)sz, f) != (size_t)sz)
    {
        fclose(f);
        free(s_ttf_buf);
        s_ttf_buf  = NULL;
        s_ttf_size = 0;
        return 0;
    }
    fclose(f);
    return 1;
}

/* ------------------------------------------------------------------ */
/* test_init_ui                                                        */
/* ------------------------------------------------------------------ */

void test_init_ui(egui_core_t *core)
{
    int font_ok = load_ttf_font();

    /* Title */
    egui_view_label_init_with_params(EGUI_VIEW_OF(&s_title_label), core,
                                     &s_title_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_title_label),
                                   EGUI_ALIGN_CENTER);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&s_title_label));

    if (font_ok &&
        egui_font_ttf_init(&s_font_sm, s_ttf_buf, s_ttf_size, 16) == 0 &&
        egui_font_ttf_init(&s_font_md, s_ttf_buf, s_ttf_size, 24) == 0 &&
        egui_font_ttf_init(&s_font_lg, s_ttf_buf, s_ttf_size, 32) == 0)
    {
        /* Small label */
        egui_view_label_init(EGUI_VIEW_OF(&s_label_sm), core);
        egui_view_set_position(EGUI_VIEW_OF(&s_label_sm),
                               4, 38);
        egui_view_set_size(EGUI_VIEW_OF(&s_label_sm),
                           SCREEN_W - 8, 24);
        egui_view_label_set_text(EGUI_VIEW_OF(&s_label_sm), "Hello, TTF 16px");
        egui_view_label_set_font(EGUI_VIEW_OF(&s_label_sm),
                                 (egui_font_t *)&s_font_sm);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&s_label_sm),
                                       EGUI_COLOR_WHITE, EGUI_ALPHA_100);
        egui_core_add_user_root_view(EGUI_VIEW_OF(&s_label_sm));

        /* Medium label */
        egui_view_label_init(EGUI_VIEW_OF(&s_label_md), core);
        egui_view_set_position(EGUI_VIEW_OF(&s_label_md),
                               4, 68);
        egui_view_set_size(EGUI_VIEW_OF(&s_label_md),
                           SCREEN_W - 8, 36);
        egui_view_label_set_text(EGUI_VIEW_OF(&s_label_md), "Hello, TTF 24px");
        egui_view_label_set_font(EGUI_VIEW_OF(&s_label_md),
                                 (egui_font_t *)&s_font_md);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&s_label_md),
                                       EGUI_COLOR_WHITE, EGUI_ALPHA_100);
        egui_core_add_user_root_view(EGUI_VIEW_OF(&s_label_md));

        /* Large label */
        egui_view_label_init(EGUI_VIEW_OF(&s_label_lg), core);
        egui_view_set_position(EGUI_VIEW_OF(&s_label_lg),
                               4, 110);
        egui_view_set_size(EGUI_VIEW_OF(&s_label_lg),
                           SCREEN_W - 8, 48);
        egui_view_label_set_text(EGUI_VIEW_OF(&s_label_lg), "Hello, 32px");
        egui_view_label_set_font(EGUI_VIEW_OF(&s_label_lg),
                                 (egui_font_t *)&s_font_lg);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&s_label_lg),
                                       EGUI_COLOR_WHITE, EGUI_ALPHA_100);
        egui_core_add_user_root_view(EGUI_VIEW_OF(&s_label_lg));

        /* Status */
        egui_view_label_init(EGUI_VIEW_OF(&s_status_label), core);
        egui_view_set_position(EGUI_VIEW_OF(&s_status_label),
                               4, SCREEN_H - 24);
        egui_view_set_size(EGUI_VIEW_OF(&s_status_label),
                           SCREEN_W - 8, 20);
        egui_view_label_set_text(EGUI_VIEW_OF(&s_status_label),
                                 "DejaVuSans.ttf loaded OK");
        egui_view_label_set_align_type(EGUI_VIEW_OF(&s_status_label),
                                       EGUI_ALIGN_CENTER);
        egui_core_add_user_root_view(EGUI_VIEW_OF(&s_status_label));
    }
    else
    {
        /* Font not available – show a status message with the compact font. */
        egui_view_label_init(EGUI_VIEW_OF(&s_status_label), core);
        egui_view_set_position(EGUI_VIEW_OF(&s_status_label),
                               4, SCREEN_H / 2 - 12);
        egui_view_set_size(EGUI_VIEW_OF(&s_status_label),
                           SCREEN_W - 8, 24);
        egui_view_label_set_text(EGUI_VIEW_OF(&s_status_label),
                                 "TTF font not found");
        egui_view_label_set_align_type(EGUI_VIEW_OF(&s_status_label),
                                       EGUI_ALIGN_CENTER);
        egui_core_add_user_root_view(EGUI_VIEW_OF(&s_status_label));
    }
}

/* ------------------------------------------------------------------ */
/* Recording test                                                      */
/* ------------------------------------------------------------------ */

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        /* Capture initial render of TTF text. */
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    default:
        return false;
    }
}
#endif
