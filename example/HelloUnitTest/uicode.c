#include "egui.h"
#include "uicode.h"

#include "test/test_dirty_region.h"
#include "test/test_region.h"
#include "test/test_dlist.h"
#include "test/test_slist.h"
#include "test/test_ringbuffer.h"
#include "test/test_fixmath.h"
#include "test/test_common.h"
#include "test/test_view.h"
#include "test/test_view_group.h"
#include "test/test_linearlayout.h"
#include "test/test_interpolator.h"
#include "test/test_animation.h"
#include "test/test_view_layer.h"
#include "test/test_calendar_view.h"
#include "test/test_chapter_strip.h"
#include "test/test_color_picker.h"
#include "test/test_drop_down_button.h"
#include "test/test_flip_view.h"
#include "test/test_menu_bar.h"
#include "test/test_password_box.h"
#include "test/test_pips_pager.h"
#include "test/test_rating_control.h"
#include "test/test_scroll_bar.h"
#include "test/test_swipe_control.h"
#include "test/test_toggle_split_button.h"
#include "test/test_transport_bar.h"
#include "test/test_token_input.h"

void uicode_init_ui(void)
{
}

void uicode_create_ui(void)
{
    uicode_init_ui();

    // Run all test suites
    test_region_run();
    test_dlist_run();
    test_slist_run();
    test_ringbuffer_run();
    test_fixmath_run();
    test_common_run();
    test_view_run();
    test_view_group_run();
    test_linearlayout_run();
    test_interpolator_run();
    test_animation_run();
    test_dirty_region_run();
    test_view_layer_run();
    test_calendar_view_run();
    test_chapter_strip_run();
    test_color_picker_run();
    test_drop_down_button_run();
    test_flip_view_run();
    test_menu_bar_run();
    test_password_box_run();
    test_pips_pager_run();
    test_rating_control_run();
    test_scroll_bar_run();
    test_swipe_control_run();
    test_toggle_split_button_run();
    test_transport_bar_run();
    test_token_input_run();
}
