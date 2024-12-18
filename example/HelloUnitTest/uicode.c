#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

// views in root
static egui_view_label_t label_1;
static egui_view_label_t label_2;
// static egui_view_button_t button_1;
static egui_view_linearlayout_t layout_1;

// EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_button_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 15);
// EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_button_param_pressed, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_100, 15);
// EGUI_BACKGROUND_PARAM_INIT(bg_button_params, &bg_button_param_normal, &bg_button_param_pressed, NULL);
// EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_button, &bg_button_params);

#if EGUI_CONFIG_DIRTY_AREA_COUNT != 2
#error "EGUI_CONFIG_DIRTY_AREA_COUNT should be 2"
#endif // EGUI_CONFIG_DIRTY_AREA_COUNT != 2

void test_case_1(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_1: start\r\n");

    egui_core_clear_region_dirty();

    // update dirty region.
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    // check.
    if(!egui_region_is_same(&region, &region_region_arr[0]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    for(int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}


void test_case_2(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_2: start\r\n");

    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_core_clear_region_dirty();

    // update dirty region.
    egui_core_update_region_dirty(&region);

    // check.
    if(!egui_region_is_same(&region, &region_region_arr[0]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    for(int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}


void test_case_3(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_3: start\r\n");

    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH + 30, EGUI_CONFIG_SCEEN_HEIGHT + 30);
    egui_core_clear_region_dirty();

    // update dirty region.
    egui_core_update_region_dirty(&region);

    // check.
    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    if(!egui_region_is_same(&region, &region_region_arr[0]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    for(int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}




void test_case_4(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_4: start\r\n");

    egui_region_init(&region, 30, 30, EGUI_CONFIG_SCEEN_WIDTH + 30, EGUI_CONFIG_SCEEN_HEIGHT + 30);
    egui_core_clear_region_dirty();

    // update dirty region.
    egui_core_update_region_dirty(&region);

    // check.
    egui_region_init(&region, 30, 30, EGUI_CONFIG_SCEEN_WIDTH - 30, EGUI_CONFIG_SCEEN_HEIGHT - 30);
    if(!egui_region_is_same(&region, &region_region_arr[0]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    for(int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}





void test_case_5(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_5: start\r\n");

    egui_region_init(&region, EGUI_CONFIG_SCEEN_WIDTH + 30, EGUI_CONFIG_SCEEN_HEIGHT + 30, 30, 30);
    egui_core_clear_region_dirty();

    // update dirty region.
    egui_core_update_region_dirty(&region);

    // check.
    for(int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}



void test_case_6(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_6: start\r\n");

    egui_region_init(&region, EGUI_CONFIG_SCEEN_WIDTH + 30, 0, 30, 30);
    egui_core_clear_region_dirty();

    // update dirty region.
    egui_core_update_region_dirty(&region);

    // check.
    for(int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}




void test_case_7(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_7: start\r\n");

    egui_region_init(&region, 0, EGUI_CONFIG_SCEEN_HEIGHT + 30, 30, 30);
    egui_core_clear_region_dirty();

    // update dirty region.
    egui_core_update_region_dirty(&region);

    // check.
    for(int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}









void test_case_two_1(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_two_1: start\r\n");

    egui_core_clear_region_dirty();

    // update dirty region.
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&region, 50, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    // check.
    egui_region_init(&region, 0, 0, 50, 50);
    if(!egui_region_is_same(&region, &region_region_arr[0]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    egui_region_init(&region, 50, 0, 50, 50);
    if(!egui_region_is_same(&region, &region_region_arr[1]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    for(int i = 2; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}


void test_case_two_2(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_two_2: start\r\n");

    egui_core_clear_region_dirty();

    // update dirty region.
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_core_update_region_dirty(&region);

    // check.
    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    if(!egui_region_is_same(&region, &region_region_arr[0]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    for(int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}


void test_case_two_3(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_two_3: start\r\n");

    egui_core_clear_region_dirty();

    // update dirty region.
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH + 30, EGUI_CONFIG_SCEEN_HEIGHT + 30);
    egui_core_update_region_dirty(&region);

    // check.
    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    if(!egui_region_is_same(&region, &region_region_arr[0]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    for(int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}

void test_case_two_4(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_two_4: start\r\n");

    egui_core_clear_region_dirty();

    // update dirty region.
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&region, EGUI_CONFIG_SCEEN_WIDTH, 0, EGUI_CONFIG_SCEEN_WIDTH + 30, EGUI_CONFIG_SCEEN_HEIGHT + 30);
    egui_core_update_region_dirty(&region);

    // check.
    egui_region_init(&region, 0, 0, 50, 50);
    if(!egui_region_is_same(&region, &region_region_arr[0]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    for(int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}




void test_case_two_5(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_two_5: start\r\n");

    egui_core_clear_region_dirty();

    // update dirty region.
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&region, 50, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_core_update_region_dirty(&region);

    // check.
    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    if(!egui_region_is_same(&region, &region_region_arr[0]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    for(int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}


void test_case_two_6(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_two_6: start\r\n");

    egui_core_clear_region_dirty();

    // update dirty region.
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&region, 40, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&region, 0, 40, 50, 50);
    egui_core_update_region_dirty(&region);

    // check.
    egui_region_init(&region, 0, 0, 50 + 40, 50 + 40);
    if(!egui_region_is_same(&region, &region_region_arr[0]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    for(int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}



void test_case_three_1(void)
{
    egui_region_t region;
    // init.
    egui_region_t *region_region_arr = egui_core_get_region_dirty_arr();

    EGUI_LOG_INF("test_case_three_1: start\r\n");

    egui_core_clear_region_dirty();

    // update dirty region.
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&region, 50, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&region, 0, 50, 50, 50);
    egui_core_update_region_dirty(&region);

    // check.
    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    if(!egui_region_is_same(&region, &region_region_arr[0]))
    {
        // error
        EGUI_LOG_ERR("region is not same\r\n");
        return;
    }

    for(int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if(!egui_region_is_empty(&region_region_arr[i]))
        {
            // error
            EGUI_LOG_ERR("region is not empty\r\n");
            return;
        }
    }

    EGUI_LOG_INF("PASS\r\n");
}

void test_case_update_dirty_region(void)
{
    test_case_1();
    test_case_2();
    test_case_3();
    test_case_4();
    test_case_5();
    test_case_6();
    test_case_7();

    test_case_two_1();
    test_case_two_2();
    test_case_two_3();
    test_case_two_4();
    test_case_two_5();
    test_case_two_6();

    test_case_three_1();
}

#define BUTTON_WIDTH  150
#define BUTTON_HEIGHT 50

#define LABEL_WIDTH  150
#define LABEL_HEIGHT 30
void uicode_init_ui(void)
{
    // Init all views
    // layout_1
    egui_view_linearlayout_init((egui_view_t *)&layout_1);
    egui_view_set_position((egui_view_t *)&layout_1, 0, 0);
    egui_view_set_size((egui_view_t *)&layout_1, EGUI_CONFIG_SCEEN_WIDTH, LABEL_HEIGHT + LABEL_HEIGHT);
    egui_view_linearlayout_set_align_type((egui_view_t *)&layout_1, EGUI_ALIGN_CENTER);

    // label_1
    egui_view_label_init((egui_view_t *)&label_1);
    egui_view_set_position((egui_view_t *)&label_1, 0, 0);
    egui_view_set_size((egui_view_t *)&label_1, LABEL_WIDTH, LABEL_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&label_1, "Test Case");
    egui_view_label_set_align_type((egui_view_t *)&label_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    // label_2
    egui_view_label_init((egui_view_t *)&label_2);
    egui_view_set_position((egui_view_t *)&label_2, 0, 0);
    egui_view_set_size((egui_view_t *)&label_2, LABEL_WIDTH, LABEL_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&label_2, "Error!");
    egui_view_label_set_align_type((egui_view_t *)&label_2, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_2, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_2, EGUI_COLOR_RED, EGUI_ALPHA_100);

    // button_1
    // egui_view_button_init((egui_view_t *)&button_1);
    // egui_view_set_position((egui_view_t *)&button_1, 0, 0);
    // egui_view_set_size((egui_view_t *)&button_1, BUTTON_WIDTH, BUTTON_HEIGHT);
    // egui_view_label_set_text((egui_view_t *)&button_1, button_str);
    // egui_view_label_set_align_type((egui_view_t *)&button_1, EGUI_ALIGN_CENTER);
    // egui_view_label_set_font((egui_view_t *)&button_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    // egui_view_label_set_font_color((egui_view_t *)&button_1, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    // egui_view_set_on_click_listener((egui_view_t *)&button_1, button_click_cb);

    // bg_button
    // egui_view_set_background((egui_view_t *)&button_1, (egui_background_t *)&bg_button);

    // Add childs to layout_1
    egui_view_group_add_child((egui_view_t *)&layout_1, (egui_view_t *)&label_1);
    egui_view_group_add_child((egui_view_t *)&layout_1, (egui_view_t *)&label_2);

    // Re-layout childs
    egui_view_linearlayout_layout_childs((egui_view_t *)&layout_1);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&layout_1);
}

void uicode_create_ui(void)
{
    uicode_init_ui();

    test_case_update_dirty_region();
}
