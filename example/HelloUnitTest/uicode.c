#include "egui.h"
#include "uicode.h"

#include "test/test_activity_ring_dirty.h"
#include "test/test_animation.h"
#include "test/test_basic_widget_dirty.h"
#include "test/test_canvas_active.h"
#include "test/test_canvas_panner.h"
#include "test/test_circular_widget_dirty.h"
#include "test/test_combobox.h"
#include "test/test_common.h"
#include "test/test_compact_text.h"
#include "test/test_dirty_region.h"
#include "test/test_dlist.h"
#include "test/test_fixmath.h"
#include "test/test_grid_view.h"
#include "test/test_interpolator.h"
#include "test/test_invalidate_region.h"
#include "test/test_linearlayout.h"
#include "test/test_list_view.h"
#include "test/test_lyric_scroller.h"
#include "test/test_mask.h"
#include "test/test_menu.h"
#include "test/test_mini_calendar.h"
#include "test/test_notification_badge.h"
#include "test/test_number_picker.h"
#include "test/test_page_activity_lifecycle.h"
#include "test/test_region.h"
#include "test/test_ringbuffer.h"
#include "test/test_slist.h"
#include "test/test_tab_bar.h"
#include "test/test_textblock.h"
#include "test/test_view.h"
#include "test/test_view_group.h"
#include "test/test_view_layer.h"
#include "test/test_virtual_stage.h"
#include "test/test_virtual_viewport.h"
#include "test/test_widget_timer_lifecycle.h"

void uicode_init_ui(void)
{
}

void uicode_create_ui(void)
{
    uicode_init_ui();

    test_region_run();
    test_dlist_run();
    test_slist_run();
    test_ringbuffer_run();
    test_fixmath_run();
    test_mask_run();
    test_common_run();
    test_view_run();
    test_view_group_run();
    test_canvas_panner_run();
    test_linearlayout_run();
    test_list_view_run();
    test_interpolator_run();
    test_animation_run();
    test_dirty_region_run();
    test_activity_ring_dirty_run();
    test_basic_widget_dirty_run();
    test_circular_widget_dirty_run();
    test_invalidate_region_run();
    test_canvas_active_run();
    test_combobox_run();
    test_view_layer_run();
    test_compact_text_run();
    test_grid_view_run();
    test_lyric_scroller_run();
    test_menu_run();
    test_mini_calendar_run();
    test_notification_badge_run();
    test_number_picker_run();
    test_page_activity_lifecycle_run();
    test_tab_bar_run();
    test_textblock_run();
    test_virtual_stage_run();
    test_virtual_viewport_run();
    test_widget_timer_lifecycle_run();
}
