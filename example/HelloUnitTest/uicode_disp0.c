#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"

#include "test/test_activity_ring_dirty.h"
#include "test/test_app_timer_helpers.h"
#include "test/test_animation.h"
#include "test/test_basic_widget_dirty.h"
#include "test/test_canvas_active.h"
#include "test/test_canvas_panner.h"
#include "test/test_circular_widget_dirty.h"
#include "test/test_combobox.h"
#include "test/test_common.h"
#include "test/test_compact_text.h"
#include "test/test_deferred_image.h"
#include "test/test_dirty_region.h"
#include "test/test_dlist.h"
#include "test/test_fixmath.h"
#include "test/test_font_std.h"
#include "test/test_focus_key_navigation.h"
#include "test/test_image_file.h"
#include "test/test_grid_view.h"
#include "test/test_image_svg.h"
#include "test/test_interpolator.h"
#include "test/test_invalidate_region.h"
#include "test/test_linearlayout.h"
#include "test/test_list_view.h"
#include "test/test_lyric_scroller.h"
#include "test/test_mask.h"
#include "test/test_menu.h"
#include "test/test_mini_calendar.h"
#include "test/test_notification_badge.h"
#include "test/test_dirty_passthrough_container.h"
#include "test/test_number_picker.h"
#include "test/test_page_activity_lifecycle.h"
#include "test/test_region.h"
#include "test/test_ringbuffer.h"
#include "test/test_slist.h"
#include "test/test_tab_bar.h"
#include "test/test_toast.h"
#include "test/test_textblock.h"
#include "test/test_view.h"
#include "test/test_view_group.h"
#include "test/test_view_layer.h"
#include "test/test_virtual_stage.h"
#include "test/test_virtual_viewport.h"
#include "test/test_widget_timer_lifecycle.h"

static egui_core_t *s_core;
static const char *s_test_filter;

static int uicode_is_filter_separator(char ch)
{
    return ch == ',' || ch == ';' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static int uicode_should_run_suite(const char *suite_name)
{
    const char *cursor = s_test_filter;

    if (cursor == NULL || cursor[0] == '\0')
    {
        return 1;
    }

    while (*cursor != '\0')
    {
        const char *token_start;
        const char *token_end;

        while (*cursor != '\0' && uicode_is_filter_separator(*cursor))
        {
            cursor++;
        }
        if (*cursor == '\0')
        {
            break;
        }

        token_start = cursor;
        while (*cursor != '\0' && !uicode_is_filter_separator(*cursor))
        {
            cursor++;
        }
        token_end = cursor;

        if ((size_t)(token_end - token_start) == strlen(suite_name) && strncmp(token_start, suite_name, (size_t)(token_end - token_start)) == 0)
        {
            return 1;
        }
    }

    return 0;
}

void uicode_set_test_filter(const char *filter)
{
    s_test_filter = filter;
}

static void uicode_disp0_init_ui(egui_core_t *core)
{
    EGUI_UNUSED(core);
}

void uicode_disp0_init(egui_core_t *core)
{
    s_core = core;
    uicode_disp0_init_ui(core);

    if (uicode_should_run_suite("region"))
        test_region_run();
    if (uicode_should_run_suite("dlist"))
        test_dlist_run();
    if (uicode_should_run_suite("slist"))
        test_slist_run();
    if (uicode_should_run_suite("ringbuffer"))
        test_ringbuffer_run();
    if (uicode_should_run_suite("fixmath"))
        test_fixmath_run();
    if (uicode_should_run_suite("app_timer_helpers"))
        test_app_timer_helpers_run();
    if (uicode_should_run_suite("font_std"))
        test_font_std_run();
    if (uicode_should_run_suite("focus_key_navigation"))
        test_focus_key_navigation_run();
    if (uicode_should_run_suite("image_file"))
        test_image_file_run();
    if (uicode_should_run_suite("deferred_image"))
        test_deferred_image_run();
    if (uicode_should_run_suite("mask"))
        test_mask_run();
    if (uicode_should_run_suite("common"))
        test_common_run();
    if (uicode_should_run_suite("view"))
        test_view_run();
    if (uicode_should_run_suite("view_group"))
        test_view_group_run();
    if (uicode_should_run_suite("canvas_panner"))
        test_canvas_panner_run();
    if (uicode_should_run_suite("linearlayout"))
        test_linearlayout_run();
    if (uicode_should_run_suite("list_view"))
        test_list_view_run();
    if (uicode_should_run_suite("interpolator"))
        test_interpolator_run();
    if (uicode_should_run_suite("animation"))
        test_animation_run();
    if (uicode_should_run_suite("dirty_region"))
        test_dirty_region_run();
    if (uicode_should_run_suite("dirty_passthrough_container"))
        test_dirty_passthrough_container_run();
    if (uicode_should_run_suite("activity_ring_dirty"))
        test_activity_ring_dirty_run();
    if (uicode_should_run_suite("basic_widget_dirty"))
        test_basic_widget_dirty_run();
    if (uicode_should_run_suite("circular_widget_dirty"))
        test_circular_widget_dirty_run();
    if (uicode_should_run_suite("invalidate_region"))
        test_invalidate_region_run();
    if (uicode_should_run_suite("canvas_active"))
        test_canvas_active_run();
    if (uicode_should_run_suite("combobox"))
        test_combobox_run();
    if (uicode_should_run_suite("view_layer"))
        test_view_layer_run();
    if (uicode_should_run_suite("compact_text"))
        test_compact_text_run();
    if (uicode_should_run_suite("grid_view"))
        test_grid_view_run();
    if (uicode_should_run_suite("image_svg"))
        test_image_svg_run();
    if (uicode_should_run_suite("lyric_scroller"))
        test_lyric_scroller_run();
    if (uicode_should_run_suite("menu"))
        test_menu_run();
    if (uicode_should_run_suite("mini_calendar"))
        test_mini_calendar_run();
    if (uicode_should_run_suite("notification_badge"))
        test_notification_badge_run();
    if (uicode_should_run_suite("number_picker"))
        test_number_picker_run();
    if (uicode_should_run_suite("page_activity_lifecycle"))
        test_page_activity_lifecycle_run();
    if (uicode_should_run_suite("tab_bar"))
        test_tab_bar_run();
    if (uicode_should_run_suite("toast"))
        test_toast_run();
    if (uicode_should_run_suite("textblock"))
        test_textblock_run();
    if (uicode_should_run_suite("virtual_stage"))
        test_virtual_stage_run();
    if (uicode_should_run_suite("virtual_viewport"))
        test_virtual_viewport_run();
    if (uicode_should_run_suite("widget_timer_lifecycle"))
        test_widget_timer_lifecycle_run();
}

egui_core_t *uicode_get_core(void)
{
    return s_core;
}
