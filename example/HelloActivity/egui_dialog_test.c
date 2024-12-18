#include "egui.h"
#include <stdlib.h>
#include <math.h>

#include "uicode.h"
#include "egui_dialog_test.h"

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_dialog_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 20);
EGUI_BACKGROUND_PARAM_INIT(bg_dialog_params, &bg_dialog_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_dialog, &bg_dialog_params);

// bg_button
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_normal, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_pressed, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_disabled, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_button_params, &bg_button_param_normal, &bg_button_param_pressed, &bg_button_param_disabled);
static egui_background_color_t bg_button;

extern int uicode_start_next_dialog(egui_dialog_t *current_dialog);
static void button_1_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("dialog button_1_click_cb\n");
    egui_dialog_t *p_dialog = egui_core_dialog_get();
    if (p_dialog)
    {
        egui_core_dialog_finish(p_dialog);
    }
}

#define DIALOG_WIDTH  (EGUI_CONFIG_SCEEN_WIDTH / 2)
#define DIALOG_HEIGHT (EGUI_CONFIG_SCEEN_HEIGHT / 2)

#define BUTTON_WIDTH  100
#define BUTTON_HEIGHT 30

#define LABEL_WIDTH  100
#define LABEL_HEIGHT 30

void egui_dialog_test_on_create(egui_dialog_t *self)
{
    egui_dialog_test_t *local = (egui_dialog_test_t *)self;
    // Call super on_create
    egui_dialog_on_create(self);

    // Init all views
    // layout_1
    egui_view_linearlayout_init((egui_view_t *)&local->layout_1);
    egui_view_set_position((egui_view_t *)&local->layout_1, 0, 0);
    egui_view_set_size((egui_view_t *)&local->layout_1, DIALOG_WIDTH, DIALOG_HEIGHT);
    egui_view_linearlayout_set_align_type((egui_view_t *)&local->layout_1, EGUI_ALIGN_CENTER);

    // label_1
    egui_view_label_init((egui_view_t *)&local->label_1);
    egui_view_set_position((egui_view_t *)&local->label_1, 0, 0);
    egui_view_set_size((egui_view_t *)&local->label_1, LABEL_WIDTH, LABEL_HEIGHT);
    egui_view_set_margin_all((egui_view_t *)&local->label_1, 5);
    egui_view_label_set_text((egui_view_t *)&local->label_1, "Dialog");
    egui_view_label_set_align_type((egui_view_t *)&local->label_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->label_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->label_1, EGUI_COLOR_BLACK, EGUI_ALPHA_100);

    // button_1
    egui_view_button_init((egui_view_t *)&local->button_1);
    egui_view_set_position((egui_view_t *)&local->button_1, 0, 0);
    egui_view_set_size((egui_view_t *)&local->button_1, BUTTON_WIDTH, BUTTON_HEIGHT);
    egui_view_set_margin_all((egui_view_t *)&local->button_1, 5);
    egui_view_label_set_text((egui_view_t *)&local->button_1, "Close");
    egui_view_label_set_align_type((egui_view_t *)&local->button_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->button_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->button_1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_set_on_click_listener((egui_view_t *)&local->button_1, button_1_click_cb);

    // bg_button
    egui_background_color_init((egui_background_t *)&bg_button);
    egui_background_set_params((egui_background_t *)&bg_button, &bg_button_params);
    egui_view_set_background((egui_view_t *)&local->button_1, (egui_background_t *)&bg_button);

    // Add childs to layout_1
    egui_view_group_add_child((egui_view_t *)&local->layout_1, (egui_view_t *)&local->label_1);
    egui_view_group_add_child((egui_view_t *)&local->layout_1, (egui_view_t *)&local->button_1);

    // Re-layout childs
    egui_view_linearlayout_layout_childs((egui_view_t *)&local->layout_1);

    // Add To Root
    egui_dialog_add_view(self, (egui_view_t *)&local->layout_1);

    // Set Background
    egui_view_set_background((egui_view_t *)&self->user_root_view, (egui_background_t *)&bg_dialog);
    egui_region_t region;
    egui_region_init(&region, (EGUI_CONFIG_SCEEN_WIDTH - DIALOG_WIDTH) / 2, (EGUI_CONFIG_SCEEN_HEIGHT - DIALOG_HEIGHT) / 2, DIALOG_WIDTH, DIALOG_HEIGHT);
    egui_dialog_set_layout(self, &region);
}

void egui_dialog_test_on_destroy(egui_dialog_t *self)
{
    egui_dialog_test_t *local = (egui_dialog_test_t *)self;
    EGUI_UNUSED(local);
    // Call super on_destroy
    egui_dialog_on_destroy(self);

#if TEST_DIALOG_DYNAMIC_ALLOC
    egui_api_free((void *)self);
#endif
}

static const egui_dialog_api_t EGUI_DIALOG_API_TABLE_NAME(egui_dialog_test_t) = {
            .on_create = egui_dialog_test_on_create, // changed
            .on_start = egui_dialog_on_start,
            .on_resume = egui_dialog_on_resume,
            .on_pause = egui_dialog_on_pause,
            .on_stop = egui_dialog_on_stop,
            .on_destroy = egui_dialog_test_on_destroy, // changed
};

void egui_dialog_test_init(egui_dialog_t *self)
{
    egui_dialog_test_t *local = (egui_dialog_test_t *)self;
    EGUI_UNUSED(local);
    // call super init.
    egui_dialog_init(self);
    // update api.
    self->api = &EGUI_DIALOG_API_TABLE_NAME(egui_dialog_test_t);

    // init local data.
    egui_dialog_set_name(self, "egui_dialog_test");
}
