#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

#include "app_egui_resource_generate.h"
#include "app_egui_resource_mp4_test_std.h"
#include "app_egui_resource_mp4_cmp_std.h"
#include "app_egui_resource_mp4_cmp_qoi.h"
#include "app_egui_resource_mp4_cmp_rle.h"

/* ========== Page 1: Full-size STD MP4 ========== */
static egui_view_mp4_t mp4_main;
static egui_view_group_t page1;

/* ========== Page 2: Compressed comparison STD / QOI / RLE ========== */
static egui_view_mp4_t mp4_cmp_std, mp4_cmp_qoi, mp4_cmp_rle;
static egui_view_label_t lbl_cmp_std, lbl_cmp_qoi, lbl_cmp_rle;
static egui_view_group_t page2;

/* ========== ViewPage ========== */
static egui_view_viewpage_t viewpage;

#define MP4_MAIN_W 240
#define MP4_MAIN_H 240

#define MP4_CMP_W 80
#define MP4_CMP_H 80
#define LBL_W     80
#define LBL_H     16

/* View params */
EGUI_VIEW_MP4_PARAMS_INIT(mp4_main_p, 0, 0, MP4_MAIN_W, MP4_MAIN_H);

EGUI_VIEW_MP4_PARAMS_INIT(mp4_cmp_std_p, 0, 0, MP4_CMP_W, MP4_CMP_H);
EGUI_VIEW_MP4_PARAMS_INIT(mp4_cmp_qoi_p, 0, 0, MP4_CMP_W, MP4_CMP_H);
EGUI_VIEW_MP4_PARAMS_INIT(mp4_cmp_rle_p, 0, 0, MP4_CMP_W, MP4_CMP_H);

EGUI_VIEW_LABEL_PARAMS_INIT(lbl_cmp_std_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_cmp_qoi_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_cmp_rle_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

EGUI_VIEW_GROUP_PARAMS_INIT(page1_p, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
EGUI_VIEW_GROUP_PARAMS_INIT(page2_p, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
EGUI_VIEW_VIEWPAGE_PARAMS_INIT(viewpage_p, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

#define MP4_FPS 10

static void mp4_callback(egui_view_mp4_t *view, int is_end)
{
    if (is_end)
    {
        egui_view_mp4_start_work(EGUI_VIEW_OF(view), (1000 / MP4_FPS));
    }
}

static void init_mp4_row(egui_view_group_t *page,
                         egui_view_label_t *lbl, const egui_view_label_params_t *lbl_p, const char *text,
                         egui_view_mp4_t *mp4, const egui_view_mp4_params_t *mp4_p,
                         const egui_image_t **frames, uint16_t count)
{
    egui_view_label_init_with_params(EGUI_VIEW_OF(lbl), lbl_p);
    egui_view_label_set_text(EGUI_VIEW_OF(lbl), text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(lbl), EGUI_ALIGN_CENTER);

    egui_view_mp4_init_with_params(EGUI_VIEW_OF(mp4), mp4_p);
    mp4->mp4_image_list = frames;
    mp4->mp4_image_count = count;
    mp4->callback = mp4_callback;
    egui_view_mp4_start_work(EGUI_VIEW_OF(mp4), (1000 / MP4_FPS));

    egui_view_group_add_child(EGUI_VIEW_OF(page), EGUI_VIEW_OF(lbl));
    egui_view_group_add_child(EGUI_VIEW_OF(page), EGUI_VIEW_OF(mp4));
}

void test_init_ui(void)
{
    /* ----- Page 1: Full-size STD MP4 ----- */
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page1), &page1_p);

    egui_view_mp4_init_with_params(EGUI_VIEW_OF(&mp4_main), &mp4_main_p);
    mp4_main.mp4_image_list = mp4_arr_test_std;
    mp4_main.mp4_image_count = MP4_IMAGE_COUNT_TEST_STD;
    mp4_main.callback = mp4_callback;
    egui_view_mp4_start_work(EGUI_VIEW_OF(&mp4_main), (1000 / MP4_FPS));

    egui_view_group_add_child(EGUI_VIEW_OF(&page1), EGUI_VIEW_OF(&mp4_main));
    egui_view_group_layout_childs(EGUI_VIEW_OF(&page1), 0, 0, 0, EGUI_ALIGN_CENTER);

    /* ----- Page 2: Compressed comparison ----- */
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page2), &page2_p);
    init_mp4_row(&page2, &lbl_cmp_std, &lbl_cmp_std_p, "STD",
                 &mp4_cmp_std, &mp4_cmp_std_p, mp4_arr_cmp_std, MP4_IMAGE_COUNT_CMP_STD);
    init_mp4_row(&page2, &lbl_cmp_qoi, &lbl_cmp_qoi_p, "QOI",
                 &mp4_cmp_qoi, &mp4_cmp_qoi_p, mp4_arr_cmp_qoi, MP4_IMAGE_COUNT_CMP_QOI);
    init_mp4_row(&page2, &lbl_cmp_rle, &lbl_cmp_rle_p, "RLE",
                 &mp4_cmp_rle, &mp4_cmp_rle_p, mp4_arr_cmp_rle, MP4_IMAGE_COUNT_CMP_RLE);
    egui_view_group_layout_childs(EGUI_VIEW_OF(&page2), 0, 0, 0, EGUI_ALIGN_CENTER);

    /* ----- ViewPage ----- */
    egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&viewpage), &viewpage_p);
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page1));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page2));
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0: /* Wait on page 1 (MP4 playback) */
        EGUI_SIM_SET_WAIT(p_action, 3000);
        return true;
    case 1: /* Swipe left -> page 2 */
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    case 2: /* Wait on page 2 (compressed comparison) */
        EGUI_SIM_SET_WAIT(p_action, 3000);
        return true;
    default:
        return false;
    }
}
#endif
