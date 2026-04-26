#include "egui.h"
#include <stdlib.h>
#include "uicode_disp0.h"

#include "app_egui_resource_generate.h"

/* ========== Page 1: alpha=0 (opaque) STD / QOI / RLE ========== */
static egui_view_image_t img_a0_std, img_a0_qoi, img_a0_rle;
static egui_view_label_t lbl_a0_std, lbl_a0_qoi, lbl_a0_rle;
static egui_view_group_t page1;

/* ========== Page 2: alpha=8 (transparent) STD / QOI / RLE ========== */
static egui_view_image_t img_a8_std, img_a8_qoi, img_a8_rle;
static egui_view_label_t lbl_a8_std, lbl_a8_qoi, lbl_a8_rle;
static egui_view_group_t page2;

/* ========== Page 3: runtime SVG resources ========== */
static egui_image_svg_t runtime_svg_badge_data;
static egui_image_svg_t runtime_svg_ring_data;
static egui_image_svg_t runtime_svg_panel_data;
static egui_view_image_t img_rt_badge, img_rt_ring, img_rt_panel;
static egui_view_label_t lbl_rt_badge, lbl_rt_ring, lbl_rt_panel;
static egui_view_group_t page3;

/* ========== ViewPage ========== */
static egui_view_viewpage_t viewpage;

#define IMG_W 80
#define IMG_H 80
#define SVG_W 120
#define SVG_H 80
#define LBL_W 112
#define LBL_H 16

/* Image params */
EGUI_VIEW_IMAGE_PARAMS_INIT(img_a0_std_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_test_rgb565_0);
EGUI_VIEW_IMAGE_PARAMS_INIT(img_a0_qoi_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_test_qoi_rgb565_0);
EGUI_VIEW_IMAGE_PARAMS_INIT(img_a0_rle_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_test_rle_rgb565_0);

EGUI_VIEW_IMAGE_PARAMS_INIT(img_a8_std_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_star_rgb565_8);
EGUI_VIEW_IMAGE_PARAMS_INIT(img_a8_qoi_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_star_qoi_rgb565_8);
EGUI_VIEW_IMAGE_PARAMS_INIT(img_a8_rle_p, 0, 0, IMG_W, IMG_H, (egui_image_t *)&egui_res_image_star_rle_rgb565_8);

EGUI_VIEW_IMAGE_PARAMS_INIT(img_rt_badge_p, 0, 0, SVG_W, SVG_H, (egui_image_t *)&runtime_svg_badge_data);
EGUI_VIEW_IMAGE_PARAMS_INIT(img_rt_ring_p, 0, 0, SVG_W, SVG_H, (egui_image_t *)&runtime_svg_ring_data);
EGUI_VIEW_IMAGE_PARAMS_INIT(img_rt_panel_p, 0, 0, SVG_W, SVG_H, (egui_image_t *)&runtime_svg_panel_data);

/* Label params */
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a0_std_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a0_qoi_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a0_rle_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a8_std_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a8_qoi_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_a8_rle_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

EGUI_VIEW_LABEL_PARAMS_INIT(lbl_rt_badge_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_rt_ring_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(lbl_rt_panel_p, 0, 0, LBL_W, LBL_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

/* Page / ViewPage params */
EGUI_VIEW_GROUP_PARAMS_INIT(page1_p, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
EGUI_VIEW_GROUP_PARAMS_INIT(page2_p, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
EGUI_VIEW_GROUP_PARAMS_INIT(page3_p, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
EGUI_VIEW_VIEWPAGE_PARAMS_INIT(viewpage_p, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);

static const char runtime_svg_ring[] = "<svg viewBox='0 0 100 100'>"
                                       "<path fill='#FF6B6B' fill-rule='evenodd' "
                                       "d='M50 8 C73 8 92 27 92 50 C92 73 73 92 50 92 C27 92 8 73 8 50 C8 27 27 8 50 8 Z "
                                       "M50 28 C38 28 28 38 28 50 C28 62 38 72 50 72 C62 72 72 62 72 50 C72 38 62 28 50 28 Z'/>"
                                       "</svg>";

static const char runtime_svg_panel[] = "<svg viewBox='0 0 120 80'>"
                                        "<g transform='translate(16,10)'>"
                                        "<rect x='0' y='16' width='88' height='40' fill='#FFD166'/>"
                                        "<g transform='translate(44,36) rotate(-18)'>"
                                        "<path fill='#073B4C' d='M-24 -8 L24 -8 L12 8 L-12 8 Z'/>"
                                        "<path fill='#118AB2' d='M-10 -20 L10 -20 L18 -8 L-18 -8 Z'/>"
                                        "</g>"
                                        "</g>"
                                        "</svg>";

static void init_image_row(egui_core_t *core, egui_view_group_t *page, egui_view_label_t *lbl, const egui_view_label_params_t *lbl_p, const char *text,
                           egui_view_image_t *img, const egui_view_image_params_t *img_p)
{
    egui_view_label_init_with_params(EGUI_VIEW_OF(lbl), core, lbl_p);
    egui_view_label_set_text(EGUI_VIEW_OF(lbl), text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(lbl), EGUI_ALIGN_CENTER);

    egui_view_image_init_with_params(EGUI_VIEW_OF(img), core, img_p);
    egui_view_image_set_image_type(EGUI_VIEW_OF(img), EGUI_VIEW_IMAGE_TYPE_NORMAL);

    egui_view_group_add_child(EGUI_VIEW_OF(page), EGUI_VIEW_OF(lbl));
    egui_view_group_add_child(EGUI_VIEW_OF(page), EGUI_VIEW_OF(img));
}

static void init_runtime_svg_row(egui_core_t *core, egui_view_group_t *page, egui_view_label_t *lbl, const egui_view_label_params_t *lbl_p, const char *text,
                                 egui_view_image_t *img, const egui_view_image_params_t *img_p)
{
    init_image_row(core, page, lbl, lbl_p, text, img, img_p);
    egui_view_image_set_image_type(EGUI_VIEW_OF(img), EGUI_VIEW_IMAGE_TYPE_RESIZE);
}

static void on_viewpage_changed(egui_view_t *self, int page_index)
{
    EGUI_UNUSED(page_index);
    egui_view_invalidate_full(self);
}

void test_init_ui(egui_core_t *core)
{
    egui_image_svg_init(&runtime_svg_badge_data);
    egui_image_svg_init(&runtime_svg_ring_data);
    egui_image_svg_init(&runtime_svg_panel_data);
    if (!egui_image_svg_load_resource(&runtime_svg_badge_data, &egui_res_svg_badge))
    {
        EGUI_LOG_ERR("Failed to load SVG resource: badge.\n");
    }
    if (!egui_image_svg_load_memory(&runtime_svg_ring_data, runtime_svg_ring))
    {
        EGUI_LOG_ERR("Failed to load SVG text: ring.\n");
    }
    if (!egui_image_svg_load_memory(&runtime_svg_panel_data, runtime_svg_panel))
    {
        EGUI_LOG_ERR("Failed to load SVG text: panel.\n");
    }

    /* ----- Page 1: alpha=0 images ----- */
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page1), core, &page1_p);
    init_image_row(core, &page1, &lbl_a0_std, &lbl_a0_std_p, "STD a0", &img_a0_std, &img_a0_std_p);
    init_image_row(core, &page1, &lbl_a0_qoi, &lbl_a0_qoi_p, "QOI a0", &img_a0_qoi, &img_a0_qoi_p);
    init_image_row(core, &page1, &lbl_a0_rle, &lbl_a0_rle_p, "RLE a0", &img_a0_rle, &img_a0_rle_p);
    egui_view_group_layout_childs(EGUI_VIEW_OF(&page1), 0, 0, 0, EGUI_ALIGN_CENTER);

    /* ----- Page 2: alpha=8 images ----- */
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page2), core, &page2_p);
    init_image_row(core, &page2, &lbl_a8_std, &lbl_a8_std_p, "STD a8", &img_a8_std, &img_a8_std_p);
    init_image_row(core, &page2, &lbl_a8_qoi, &lbl_a8_qoi_p, "QOI a8", &img_a8_qoi, &img_a8_qoi_p);
    init_image_row(core, &page2, &lbl_a8_rle, &lbl_a8_rle_p, "RLE a8", &img_a8_rle, &img_a8_rle_p);
    egui_view_group_layout_childs(EGUI_VIEW_OF(&page2), 0, 0, 0, EGUI_ALIGN_CENTER);

    /* ----- Page 3: runtime SVG ----- */
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page3), core, &page3_p);
    init_runtime_svg_row(core, &page3, &lbl_rt_badge, &lbl_rt_badge_p, "RT path", &img_rt_badge, &img_rt_badge_p);
    init_runtime_svg_row(core, &page3, &lbl_rt_ring, &lbl_rt_ring_p, "RT evenodd", &img_rt_ring, &img_rt_ring_p);
    init_runtime_svg_row(core, &page3, &lbl_rt_panel, &lbl_rt_panel_p, "RT group", &img_rt_panel, &img_rt_panel_p);
    egui_view_group_layout_childs(EGUI_VIEW_OF(&page3), 0, 0, 0, EGUI_ALIGN_CENTER);

    /* ----- ViewPage ----- */
    egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&viewpage), core, &viewpage_p);
    egui_view_viewpage_set_on_page_changed(EGUI_VIEW_OF(&viewpage), on_viewpage_changed);
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page1));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page2));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page3));
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static void set_swipe_left_action(egui_sim_action_t *p_action, uint16_t interval_ms)
{
    p_action->type = EGUI_SIM_ACTION_SWIPE;
    p_action->x1 = EGUI_CONFIG_SCREEN_WIDTH * 3 / 4;
    p_action->y1 = EGUI_CONFIG_SCREEN_HEIGHT / 2;
    p_action->x2 = EGUI_CONFIG_SCREEN_WIDTH / 4;
    p_action->y2 = EGUI_CONFIG_SCREEN_HEIGHT / 2;
    p_action->steps = 5;
    p_action->interval_ms = interval_ms;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0: /* Wait on page 1 */
        EGUI_SIM_SET_WAIT(p_action, 1500);
        return true;
    case 1: /* Swipe left -> page 2 */
        set_swipe_left_action(p_action, 800);
        return true;
    case 2: /* Wait on page 2 */
        EGUI_SIM_SET_WAIT(p_action, 1500);
        return true;
    case 3: /* Swipe left -> page 3 */
        set_swipe_left_action(p_action, 800);
        return true;
    case 4: /* Wait on page 3 */
        EGUI_SIM_SET_WAIT(p_action, 1500);
        return true;
    default:
        return false;
    }
}
#endif
