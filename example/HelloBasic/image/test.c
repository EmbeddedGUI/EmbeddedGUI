#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

#include "app_egui_resource_generate.h"

/* ========== Page 1: alpha=0 (opaque) STD / QOI / RLE ========== */
static egui_view_image_t img_a0_std, img_a0_qoi, img_a0_rle;
static egui_view_label_t lbl_a0_std, lbl_a0_qoi, lbl_a0_rle;
static egui_view_group_t page1;

/* ========== Page 2: alpha=8 (transparent) STD / QOI / RLE ========== */
static egui_view_image_t img_a8_std, img_a8_qoi, img_a8_rle;
static egui_view_label_t lbl_a8_std, lbl_a8_qoi, lbl_a8_rle;
static egui_view_group_t page2;

/* ========== ViewPage ========== */
static egui_view_viewpage_t viewpage;

#define IMG_W 80
#define IMG_H 80
#define LBL_W 60
#define LBL_H 16

/* Image params */
EGUI_VIEW_IMAGE_PARAMS_INIT(img_a0_std_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_test_rgb565_0);
EGUI_VIEW_IMAGE_PARAMS_INIT(img_a0_qoi_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_test_qoi_rgb565_0);
EGUI_VIEW_IMAGE_PARAMS_INIT(img_a0_rle_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_test_rle_rgb565_0);

EGUI_VIEW_IMAGE_PARAMS_INIT(img_a8_std_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_star_rgb565_8);
EGUI_VIEW_IMAGE_PARAMS_INIT(img_a8_qoi_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_star_qoi_rgb565_8);
EGUI_VIEW_IMAGE_PARAMS_INIT(img_a8_rle_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_star_rle_rgb565_8);

/* Label params */
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a0_std_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a0_qoi_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a0_rle_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a8_std_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a8_qoi_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a8_rle_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

/* Page / ViewPage params */
EGUI_VIEW_GROUP_PARAMS_INIT(page1_p, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
EGUI_VIEW_GROUP_PARAMS_INIT(page2_p, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
EGUI_VIEW_VIEWPAGE_PARAMS_INIT(viewpage_p, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

static void init_image_row(egui_view_group_t *page, egui_view_label_t *lbl, const egui_view_label_params_t *lbl_p, const char *text, egui_view_image_t *img,
                           const egui_view_image_params_t *img_p)
{
    egui_view_label_init_with_params(EGUI_VIEW_OF(lbl), lbl_p);
    egui_view_label_set_text(EGUI_VIEW_OF(lbl), text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(lbl), EGUI_ALIGN_CENTER);

    egui_view_image_init_with_params(EGUI_VIEW_OF(img), img_p);
    egui_view_image_set_image_type(EGUI_VIEW_OF(img), EGUI_VIEW_IMAGE_TYPE_NORMAL);

    egui_view_group_add_child(EGUI_VIEW_OF(page), EGUI_VIEW_OF(lbl));
    egui_view_group_add_child(EGUI_VIEW_OF(page), EGUI_VIEW_OF(img));
}

void test_init_ui(void)
{
    /* ----- Page 1: alpha=0 images ----- */
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page1), &page1_p);
    init_image_row(&page1, &lbl_a0_std, &lbl_a0_std_p, "STD a0", &img_a0_std, &img_a0_std_p);
    init_image_row(&page1, &lbl_a0_qoi, &lbl_a0_qoi_p, "QOI a0", &img_a0_qoi, &img_a0_qoi_p);
    init_image_row(&page1, &lbl_a0_rle, &lbl_a0_rle_p, "RLE a0", &img_a0_rle, &img_a0_rle_p);
    egui_view_group_layout_childs(EGUI_VIEW_OF(&page1), 0, 0, 0, EGUI_ALIGN_CENTER);

    /* ----- Page 2: alpha=8 images ----- */
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page2), &page2_p);
    init_image_row(&page2, &lbl_a8_std, &lbl_a8_std_p, "STD a8", &img_a8_std, &img_a8_std_p);
    init_image_row(&page2, &lbl_a8_qoi, &lbl_a8_qoi_p, "QOI a8", &img_a8_qoi, &img_a8_qoi_p);
    init_image_row(&page2, &lbl_a8_rle, &lbl_a8_rle_p, "RLE a8", &img_a8_rle, &img_a8_rle_p);
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
    case 0: /* Wait on page 1 */
        EGUI_SIM_SET_WAIT(p_action, 1500);
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
    case 2: /* Wait on page 2 */
        EGUI_SIM_SET_WAIT(p_action, 1500);
        return true;
    default:
        return false;
    }
}
#endif
