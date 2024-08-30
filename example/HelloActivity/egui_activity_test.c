#include "egui.h"
#include <stdlib.h>
#include <math.h>

#include "uicode.h"
#include "egui_activity_test.h"

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_activity_1_param_normal, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_activity_1_params, &bg_activity_1_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_activity_1, &bg_activity_1_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_activity_2_param_normal, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_activity_2_params, &bg_activity_2_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_activity_2, &bg_activity_2_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_activity_3_param_normal, EGUI_COLOR_RED, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_activity_3_params, &bg_activity_3_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_activity_3, &bg_activity_3_params);

static const egui_background_color_t *bg_activity_list[] = {
        &bg_activity_1,
        &bg_activity_2,
        &bg_activity_3,
};

// bg_button
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_pressed, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_disabled, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_button_params, &bg_button_param_normal, &bg_button_param_pressed, &bg_button_param_disabled);
static egui_background_color_t bg_button;

static void button_1_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("button_1_click_cb\n");
    egui_activity_t *p_activity = egui_core_activity_get_by_view(self);
    if (p_activity)
    {
        uicode_start_next_activity(p_activity);
    }
}

static void button_2_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("button_2_click_cb\n");

    egui_activity_t *p_activity = egui_core_activity_get_by_view(self);
    if (p_activity)
    {
        egui_core_activity_finish(p_activity);
    }
}

static void button_3_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("button_3_click_cb\n");

    egui_activity_t *p_activity = egui_core_activity_get_by_view(self);
    if (p_activity)
    {
        uicode_start_dialog(p_activity);
    }
}

#define BUTTON_WIDTH  150
#define BUTTON_HEIGHT 30

#define LABEL_WIDTH  150
#define LABEL_HEIGHT 30

void egui_activity_test_on_create(egui_activity_t *self)
{
    egui_activity_test_t *local = (egui_activity_test_t *)self;
    // Call super on_create
    egui_activity_on_create(self);

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
    egui_view_label_set_text((egui_view_t *)&local->label_1, local->label_str);
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

    // button_3
    egui_view_button_init((egui_view_t *)&local->button_3);
    egui_view_set_position((egui_view_t *)&local->button_3, 0, 0);
    egui_view_set_size((egui_view_t *)&local->button_3, BUTTON_WIDTH, BUTTON_HEIGHT);
    egui_view_set_margin_all((egui_view_t *)&local->button_3, 5);
    egui_view_label_set_text((egui_view_t *)&local->button_3, "Dialog");
    egui_view_label_set_align_type((egui_view_t *)&local->button_3, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->button_3, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->button_3, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_set_on_click_listener((egui_view_t *)&local->button_3, button_3_click_cb);

    // bg_button
    egui_background_color_init((egui_background_t *)&bg_button);
    egui_background_set_params((egui_background_t *)&bg_button, &bg_button_params);
    egui_view_set_background((egui_view_t *)&local->button_1, (egui_background_t *)&bg_button);
    egui_view_set_background((egui_view_t *)&local->button_2, (egui_background_t *)&bg_button);
    egui_view_set_background((egui_view_t *)&local->button_3, (egui_background_t *)&bg_button);

    // Add childs to layout_1
    egui_view_group_add_child((egui_view_t *)&local->layout_1, (egui_view_t *)&local->label_1);
    egui_view_group_add_child((egui_view_t *)&local->layout_1, (egui_view_t *)&local->button_1);
    egui_view_group_add_child((egui_view_t *)&local->layout_1, (egui_view_t *)&local->button_2);
    egui_view_group_add_child((egui_view_t *)&local->layout_1, (egui_view_t *)&local->button_3);

    // Re-layout childs
    egui_view_linearlayout_layout_childs((egui_view_t *)&local->layout_1);

    // Add To Root
    egui_activity_add_view(self, (egui_view_t *)&local->layout_1);

    // Set Background
    egui_view_set_background((egui_view_t *)&self->root_view,
                             (egui_background_t *)bg_activity_list[local->index % (sizeof(bg_activity_list) / sizeof(bg_activity_list[0]))]);
}

void egui_activity_test_on_destroy(egui_activity_t *self)
{
    egui_activity_test_t *local = (egui_activity_test_t *)self;
    // Call super on_destroy
    egui_activity_on_destroy(self);

#if TEST_ACTIVITY_DYNAMIC_ALLOC
    egui_api_free((void *)self);
#endif
}

void egui_activity_test_set_index(egui_activity_t *self, int index)
{
    egui_activity_test_t *local = (egui_activity_test_t *)self;
    local->index = index;

    egui_api_sprintf(local->label_str, "Activity %d", index);

    egui_activity_set_name(self, local->label_str);
}

EGUI_ACTIVITY_API_DEFINE(egui_activity_test_t, egui_activity_test_on_create, NULL, NULL, NULL, NULL, egui_activity_test_on_destroy);

void egui_activity_test_init(egui_activity_t *self)
{
    egui_activity_test_t *local = (egui_activity_test_t *)self;
    // call super init.
    egui_activity_init(self);
    // update api.
    self->api = &EGUI_ACTIVITY_API_TABLE_NAME(egui_activity_test_t);

    // init local data.
    local->index = 0;

    egui_activity_set_name(self, "egui_activity_test");
}
