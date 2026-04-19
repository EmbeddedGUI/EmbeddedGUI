/**
 * HelloMultiDisplayHetero sub display:
 * 128x64 status panel showing tick counter and current page.
 */
#include "egui.h"
#include "uicode_disp1.h"
#if EGUI_PORT == EGUI_PORT_TYPE_PC
#include "sdl_port.h"
#endif

egui_core_t hello_multi_display_disp1_core;
egui_color_int_t hello_multi_display_disp1_pfb[1][EGUI_CONFIG_PFB_1_WIDTH * EGUI_CONFIG_PFB_1_HEIGHT];
static egui_view_linearlayout_t s_hello_multi_display_hetero_sub_layout;
static egui_view_label_t s_hello_multi_display_hetero_sub_tick_label;
static egui_view_label_t s_hello_multi_display_hetero_sub_page_label;
static egui_timer_t s_hello_multi_display_hetero_tick_timer;
static char s_hello_multi_display_hetero_sub_tick_str[20];
static char s_hello_multi_display_hetero_sub_page_str[20];
static uint32_t s_hello_multi_display_hetero_tick_counter = 0;
static int s_hello_multi_display_hetero_sub_page_index = 0;
static uint8_t s_hello_multi_display_hetero_page_label_alt_color = 0;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(hello_multi_display_hetero_bg_sub_param, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(hello_multi_display_hetero_bg_sub_params, &hello_multi_display_hetero_bg_sub_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(hello_multi_display_hetero_bg_sub, &hello_multi_display_hetero_bg_sub_params);

static void hello_multi_display_hetero_tick_timer_callback(egui_timer_t *timer)
{
    EGUI_UNUSED(timer);

    s_hello_multi_display_hetero_tick_counter++;
    egui_api_sprintf(s_hello_multi_display_hetero_sub_tick_str, "Tick: %u", (unsigned int)s_hello_multi_display_hetero_tick_counter);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_tick_label), s_hello_multi_display_hetero_sub_tick_str);
}

static void hello_multi_display_hetero_sub_refresh_page_label_color(void)
{
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_page_label),
                                   s_hello_multi_display_hetero_page_label_alt_color ? EGUI_COLOR_CYAN : EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
}

static void hello_multi_display_hetero_sub_click(egui_view_t *self)
{
    EGUI_UNUSED(self);

    s_hello_multi_display_hetero_tick_counter = 0;
    egui_api_sprintf(s_hello_multi_display_hetero_sub_tick_str, "Tick: 0");
    egui_view_label_set_text(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_tick_label), s_hello_multi_display_hetero_sub_tick_str);

    s_hello_multi_display_hetero_page_label_alt_color = !s_hello_multi_display_hetero_page_label_alt_color;
    hello_multi_display_hetero_sub_refresh_page_label_color();
}

void hello_multi_display_hetero_sub_update_page_label(int page_index)
{
    s_hello_multi_display_hetero_sub_page_index = page_index;
    egui_api_sprintf(s_hello_multi_display_hetero_sub_page_str, "Page: %d/3", page_index + 1);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_page_label), s_hello_multi_display_hetero_sub_page_str);
}

egui_view_t *hello_multi_display_hetero_sub_get_interaction_target(void)
{
    return EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_layout);
}

uint32_t hello_multi_display_hetero_sub_get_tick_counter(void)
{
    return s_hello_multi_display_hetero_tick_counter;
}

int hello_multi_display_hetero_sub_get_page_index(void)
{
    return s_hello_multi_display_hetero_sub_page_index;
}

void uicode_disp1_init(egui_core_t *core)
{
    s_hello_multi_display_hetero_sub_page_index = 0;

    egui_view_linearlayout_init(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_layout), core);
    egui_view_set_position(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_layout), EGUI_CONFIG_SCEEN_1_WIDTH, EGUI_CONFIG_SCEEN_1_HEIGHT);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_layout), EGUI_ALIGN_CENTER);
    egui_view_set_background(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_layout), (egui_background_t *)&hello_multi_display_hetero_bg_sub);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_layout), hello_multi_display_hetero_sub_click);

    egui_api_sprintf(s_hello_multi_display_hetero_sub_tick_str, "Tick: 0");
    egui_view_label_init(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_tick_label), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_tick_label), EGUI_CONFIG_SCEEN_1_WIDTH - 4, 24);
    egui_view_set_margin_all(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_tick_label), 2);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_tick_label), s_hello_multi_display_hetero_sub_tick_str);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_tick_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_tick_label), (egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_tick_label), EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    egui_api_sprintf(s_hello_multi_display_hetero_sub_page_str, "Page: 1/3");
    egui_view_label_init(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_page_label), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_page_label), EGUI_CONFIG_SCEEN_1_WIDTH - 4, 24);
    egui_view_set_margin_all(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_page_label), 2);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_page_label), s_hello_multi_display_hetero_sub_page_str);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_page_label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_page_label), (egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_tick_label), hello_multi_display_hetero_sub_click);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_page_label), hello_multi_display_hetero_sub_click);
    hello_multi_display_hetero_sub_refresh_page_label_color();

    egui_view_group_add_child(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_layout), EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_tick_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_layout), EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_page_label));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_layout));

    egui_timer_init_timer(&s_hello_multi_display_hetero_tick_timer, NULL, hello_multi_display_hetero_tick_timer_callback);
    egui_view_start_timer(EGUI_VIEW_OF(&s_hello_multi_display_hetero_sub_layout), &s_hello_multi_display_hetero_tick_timer, 500, 500);
}

#if EGUI_PORT == EGUI_PORT_TYPE_PC
int egui_port_get_additional_display_descriptors(egui_port_extra_display_descriptor_t *descriptors, int max_count)
{
    static egui_color_int_t *s_hello_multi_display_hetero_disp1_pfb_bufs[EGUI_ARRAY_SIZE(hello_multi_display_disp1_pfb)] = {
            hello_multi_display_disp1_pfb[0],
    };

    if (descriptors == NULL || max_count < 1)
    {
        return 0;
    }

    descriptors[0].screen_width = EGUI_CONFIG_SCEEN_1_WIDTH;
    descriptors[0].screen_height = EGUI_CONFIG_SCEEN_1_HEIGHT;
    descriptors[0].pfb_width = EGUI_CONFIG_PFB_1_WIDTH;
    descriptors[0].pfb_height = EGUI_CONFIG_PFB_1_HEIGHT;
    descriptors[0].pfb_buffers = s_hello_multi_display_hetero_disp1_pfb_bufs;
    descriptors[0].pfb_buffer_count = EGUI_ARRAY_SIZE(s_hello_multi_display_hetero_disp1_pfb_bufs);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    descriptors[0].touch_register = egui_port_register_touch_driver;
#else
    descriptors[0].touch_register = NULL;
#endif
    descriptors[0].uicode_init = uicode_disp1_init;
    return 1;
}
#endif
