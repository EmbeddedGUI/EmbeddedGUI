#include "egui.h"
#include <stdlib.h>
#include <math.h>

#include "uicode.h"
#include "egui_page_0.h"

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_page_1_param_normal, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_page_1_params, &bg_page_1_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_page_1, &bg_page_1_params);

// bg_button
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_pressed, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_disabled, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_button_params, &bg_button_param_normal, &bg_button_param_pressed, &bg_button_param_disabled);
static egui_background_color_t bg_button;

static void button_1_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("button_1_click_cb\n");
    uicode_start_next_page();
}

static void button_2_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("button_2_click_cb\n");

    uicode_start_prev_page();
}

static void egui_page_0_timer_callback(egui_timer_t *timer)
{
    egui_page_0_t *local = (egui_page_0_t *)timer->user_data;
    EGUI_UNUSED(local);
    EGUI_LOG_INF("egui_page_0_timer_callback\n");

    local->index += 1;
    egui_api_sprintf(local->label_str, "Tick: %d", local->index);

    egui_view_label_set_text((egui_view_t *)&local->label_1, local->label_str);
}


#define BUTTON_WIDTH  150
#define BUTTON_HEIGHT 30

#define LABEL_WIDTH  150
#define LABEL_HEIGHT 30

void egui_page_0_on_open(egui_page_base_t *self)
{
    egui_page_0_t *local = (egui_page_0_t *)self;
    // Call super on_open
    egui_page_base_on_open(self);

    // start timer
    egui_timer_start_timer(&local->timer, 1000, 1000);

    // Init all views
    // layout_1
    egui_view_linearlayout_init((egui_view_t *)&local->layout_1);
    egui_view_set_position((egui_view_t *)&local->layout_1, 0, 0);
    egui_view_set_size((egui_view_t *)&local->layout_1, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_linearlayout_set_align_type((egui_view_t *)&local->layout_1, EGUI_ALIGN_CENTER);

    // label_1
    egui_view_label_init((egui_view_t *)&local->label_1);
    egui_view_set_position((egui_view_t *)&local->label_1, 0, 0);
    egui_view_set_size((egui_view_t *)&local->label_1, LABEL_WIDTH, LABEL_HEIGHT);
    egui_view_set_margin_all((egui_view_t *)&local->label_1, 5);
    egui_view_label_set_text((egui_view_t *)&local->label_1, "egui_page_0");
    egui_view_label_set_align_type((egui_view_t *)&local->label_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->label_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->label_1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    // button_1
    egui_view_button_init((egui_view_t *)&local->button_1);
    egui_view_set_position((egui_view_t *)&local->button_1, 0, 0);
    egui_view_set_size((egui_view_t *)&local->button_1, BUTTON_WIDTH, BUTTON_HEIGHT);
    egui_view_set_margin_all((egui_view_t *)&local->button_1, 5);
    egui_view_label_set_text((egui_view_t *)&local->button_1, "Next");
    egui_view_label_set_align_type((egui_view_t *)&local->button_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->button_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->button_1, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_set_on_click_listener((egui_view_t *)&local->button_1, button_1_click_cb);

    // button_2
    egui_view_button_init((egui_view_t *)&local->button_2);
    egui_view_set_position((egui_view_t *)&local->button_2, 0, 0);
    egui_view_set_size((egui_view_t *)&local->button_2, BUTTON_WIDTH, BUTTON_HEIGHT);
    egui_view_set_margin_all((egui_view_t *)&local->button_2, 5);
    egui_view_label_set_text((egui_view_t *)&local->button_2, "Finish");
    egui_view_label_set_align_type((egui_view_t *)&local->button_2, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->button_2, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->button_2, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_set_on_click_listener((egui_view_t *)&local->button_2, button_2_click_cb);

    // bg_button
    egui_background_color_init((egui_background_t *)&bg_button);
    egui_background_set_params((egui_background_t *)&bg_button, &bg_button_params);
    egui_view_set_background((egui_view_t *)&local->button_1, (egui_background_t *)&bg_button);
    egui_view_set_background((egui_view_t *)&local->button_2, (egui_background_t *)&bg_button);

    // Add childs to layout_1
    egui_view_group_add_child((egui_view_t *)&local->layout_1, (egui_view_t *)&local->label_1);
    egui_view_group_add_child((egui_view_t *)&local->layout_1, (egui_view_t *)&local->button_1);
    egui_view_group_add_child((egui_view_t *)&local->layout_1, (egui_view_t *)&local->button_2);

    // Re-layout childs
    egui_view_linearlayout_layout_childs((egui_view_t *)&local->layout_1);

    // Add To Root
    egui_page_base_add_view(self, (egui_view_t *)&local->layout_1);

    // Set Background
    egui_view_set_background((egui_view_t *)&self->root_view, (egui_background_t *)&bg_page_1);
}

void egui_page_0_on_close(egui_page_base_t *self)
{
    egui_page_0_t *local = (egui_page_0_t *)self;
    EGUI_UNUSED(local);
    // Call super on_destroy
    egui_page_base_on_close(self);
    // stop timer
    egui_timer_stop_timer(&local->timer);
}

void egui_page_0_on_key_pressed(egui_page_base_t *self, uint16_t keycode)
{
    egui_page_0_t *local = (egui_page_0_t *)self;
    EGUI_UNUSED(local);
    
    if(keycode == 1)
    {
        uicode_start_prev_page();
    }
    else if(keycode == 2)
    {
        uicode_start_next_page();
    }
}

static const egui_page_base_api_t EGUI_VIEW_API_TABLE_NAME(egui_page_0_t) = {
        .on_open = egui_page_0_on_open, // changed
        .on_close = egui_page_0_on_close, // changed
        .on_key_pressed = egui_page_0_on_key_pressed, // changed
};

void egui_page_0_init(egui_page_base_t *self)
{
    egui_page_0_t *local = (egui_page_0_t *)self;
    EGUI_UNUSED(local);
    // call super init.
    egui_page_base_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_page_0_t);

    // init local data.
    local->index = 0;
    egui_timer_init_timer(&local->timer, (void *)local, egui_page_0_timer_callback);

    egui_page_base_set_name(self, "egui_page_0");
}
