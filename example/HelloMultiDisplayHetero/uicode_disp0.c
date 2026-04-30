/**
 * HelloMultiDisplayHetero main display:
 * 240x320 viewpage with three colored pages.
 */
#include "egui.h"
#include "uicode_disp0.h"
#include "uicode_disp1.h"
#if EGUI_PORT == EGUI_PORT_TYPE_PC
#include "sdl_port.h"
#endif

static egui_view_viewpage_t s_main_viewpage;
static egui_view_label_t s_page_labels[3];

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(hello_multi_display_hetero_bg_page0_param, EGUI_COLOR_RED, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(hello_multi_display_hetero_bg_page0_params, &hello_multi_display_hetero_bg_page0_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(hello_multi_display_hetero_bg_page0, &hello_multi_display_hetero_bg_page0_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(hello_multi_display_hetero_bg_page1_param, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(hello_multi_display_hetero_bg_page1_params, &hello_multi_display_hetero_bg_page1_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(hello_multi_display_hetero_bg_page1, &hello_multi_display_hetero_bg_page1_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(hello_multi_display_hetero_bg_page2_param, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(hello_multi_display_hetero_bg_page2_params, &hello_multi_display_hetero_bg_page2_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(hello_multi_display_hetero_bg_page2, &hello_multi_display_hetero_bg_page2_params);

static const egui_background_color_t *s_hello_multi_display_hetero_page_bgs[] = {
        &hello_multi_display_hetero_bg_page0,
        &hello_multi_display_hetero_bg_page1,
        &hello_multi_display_hetero_bg_page2,
};

static const char *s_hello_multi_display_hetero_page_texts[] = {
        "Page 1 - Red",
        "Page 2 - Green",
        "Page 3 - Blue",
};

#if EGUI_PORT == EGUI_PORT_TYPE_PC
#define HELLO_MULTI_DISPLAY_HETERO_PAGE_SYNC_TIMEOUT_MS 1000

static void hello_multi_display_hetero_sub_update_page_label_task(egui_core_t *core, uintptr_t user_data)
{
    EGUI_UNUSED(core);
    hello_multi_display_hetero_sub_update_page_label((int)user_data);
}
#endif

static void hello_multi_display_hetero_on_page_changed(egui_view_t *self, int page_index)
{
    EGUI_UNUSED(self);

#if EGUI_PORT == EGUI_PORT_TYPE_PC
    {
        egui_core_t *sub_core = egui_port_get_core_by_display_id(1);

        if (sub_core != NULL)
        {
            if (!egui_port_post_core_task_sync_named(sub_core, hello_multi_display_hetero_sub_update_page_label_task, (uintptr_t)page_index,
                                                     HELLO_MULTI_DISPLAY_HETERO_PAGE_SYNC_TIMEOUT_MS, "hetero_page_sync"))
            {
                printf("[PC_CORE_TASK_CALLER] context=hetero_page_sync display=1 reason=sync_failed page=%d\n", page_index);
            }
            return;
        }
    }
#endif

    hello_multi_display_hetero_sub_update_page_label(page_index);
}

void uicode_disp0_init(egui_core_t *core)
{
    int i;

    egui_view_viewpage_init(EGUI_VIEW_OF(&s_main_viewpage), core);
    egui_view_set_position(EGUI_VIEW_OF(&s_main_viewpage), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&s_main_viewpage), EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);

    for (i = 0; i < (int)EGUI_ARRAY_SIZE(s_page_labels); i++)
    {
        egui_view_label_init(EGUI_VIEW_OF(&s_page_labels[i]), core);
        egui_view_set_size(EGUI_VIEW_OF(&s_page_labels[i]), EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
        egui_view_label_set_text(EGUI_VIEW_OF(&s_page_labels[i]), s_hello_multi_display_hetero_page_texts[i]);
        egui_view_label_set_align_type(EGUI_VIEW_OF(&s_page_labels[i]), EGUI_ALIGN_CENTER);
        egui_view_label_set_font(EGUI_VIEW_OF(&s_page_labels[i]), (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&s_page_labels[i]), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
        egui_view_set_background(EGUI_VIEW_OF(&s_page_labels[i]), (egui_background_t *)s_hello_multi_display_hetero_page_bgs[i]);
        egui_view_viewpage_add_child(EGUI_VIEW_OF(&s_main_viewpage), EGUI_VIEW_OF(&s_page_labels[i]));
    }

    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&s_main_viewpage));
    egui_view_viewpage_set_on_page_changed(EGUI_VIEW_OF(&s_main_viewpage), hello_multi_display_hetero_on_page_changed);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&s_main_viewpage));
}
