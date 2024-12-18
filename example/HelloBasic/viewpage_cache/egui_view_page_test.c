#include "egui.h"
#include <stdlib.h>
#include <math.h>

#include "uicode.h"
#include "egui_view_page_test.h"

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_1_param_normal, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_1_params, &bg_1_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_1, &bg_1_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_2_param_normal, EGUI_COLOR_ORANGE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_2_params, &bg_2_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_2, &bg_2_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_3_param_normal, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_3_params, &bg_3_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_3, &bg_3_params);

static const egui_background_color_t *bg_list[] = {
        &bg_1,
        &bg_2,
        &bg_3,
};

void egui_view_page_test_sub_view_init(egui_view_t *self)
{
    egui_view_page_test_t *local = (egui_view_page_test_t *)self;

    // Init all views
    // label_1
    egui_view_label_init((egui_view_t *)&local->label_1);
    egui_view_set_size((egui_view_t *)&local->label_1, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&local->label_1, local->label_str);
    egui_view_label_set_align_type((egui_view_t *)&local->label_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->label_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->label_1, EGUI_COLOR_BLACK, EGUI_ALPHA_100);

    // Add childs to layout_1
    egui_view_group_add_child((egui_view_t *)&local->base, (egui_view_t *)&local->label_1);

    // Set Background
    egui_view_set_background((egui_view_t *)&local->label_1,
                             (egui_background_t *)bg_list[local->index % (sizeof(bg_list) / sizeof(bg_list[0]))]);
}

void egui_view_page_test_set_index(egui_view_t *self, int index)
{
    egui_view_page_test_t *local = (egui_view_page_test_t *)self;
    local->index = index;

    egui_api_sprintf(local->label_str, "Page %d", index);

    // reinit.
    egui_view_page_test_sub_view_init(self);
}

void egui_view_page_test_init(egui_view_t *self)
{
    egui_view_page_test_t *local = (egui_view_page_test_t *)self;
    // call super init.
    egui_view_group_init(self);

    // init local data.
    egui_view_page_test_sub_view_init(self);
}
