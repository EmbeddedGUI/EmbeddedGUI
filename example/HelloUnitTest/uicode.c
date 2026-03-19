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
#include "test/test_auto_suggest_box.h"
#include "test/test_alert_banner.h"
#include "test/test_annotated_scroll_bar.h"
#include "test/test_badge_group.h"
#include "test/test_combobox.h"
#include "test/test_breadcrumb_bar.h"
#include "test/test_view_layer.h"
#include "test/test_calendar_view.h"
#include "test/test_card_panel.h"
#include "test/test_chapter_strip.h"
#include "test/test_color_picker.h"
#include "test/test_command_bar.h"
#include "test/test_data_list_panel.h"
#include "test/test_date_picker.h"
#include "test/test_drop_down_button.h"
#include "test/test_dialog_sheet.h"
#include "test/test_expander.h"
#include "test/test_flip_view.h"
#include "test/test_menu.h"
#include "test/test_menu_bar.h"
#include "test/test_mini_calendar.h"
#include "test/test_master_detail.h"
#include "test/test_message_bar.h"
#include "test/test_menu_flyout.h"
#include "test/test_nav_panel.h"
#include "test/test_node_topology.h"
#include "test/test_number_box.h"
#include "test/test_number_picker.h"
#include "test/test_persona_group.h"
#include "test/test_password_box.h"
#include "test/test_parallax_view.h"
#include "test/test_pips_pager.h"
#include "test/test_rating_control.h"
#include "test/test_skeleton.h"
#include "test/test_scroll_bar.h"
#include "test/test_segmented_control.h"
#include "test/test_settings_panel.h"
#include "test/test_shortcut_recorder.h"
#include "test/test_split_button.h"
#include "test/test_split_view.h"
#include "test/test_swipe_control.h"
#include "test/test_tab_strip.h"
#include "test/test_tab_bar.h"
#include "test/test_tab_view.h"
#include "test/test_skeleton_loader.h"
#include "test/test_teaching_tip.h"
#include "test/test_time_picker.h"
#include "test/test_tree_view.h"
#include "test/test_toggle_button.h"
#include "test/test_toggle_split_button.h"
#include "test/test_transport_bar.h"
#include "test/test_toast_stack.h"
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
    test_auto_suggest_box_run();
    test_alert_banner_run();
    test_dirty_region_run();
    test_annotated_scroll_bar_run();
    test_badge_group_run();
    test_combobox_run();
    test_breadcrumb_bar_run();
    test_view_layer_run();
    test_calendar_view_run();
    test_card_panel_run();
    test_chapter_strip_run();
    test_color_picker_run();
    test_command_bar_run();
    test_data_list_panel_run();
    test_date_picker_run();
    test_drop_down_button_run();
    test_dialog_sheet_run();
    test_expander_run();
    test_flip_view_run();
    test_menu_run();
    test_menu_bar_run();
    test_mini_calendar_run();
    test_master_detail_run();
    test_message_bar_run();
    test_menu_flyout_run();
    test_nav_panel_run();
    test_node_topology_run();
    test_number_box_run();
    test_number_picker_run();
    test_persona_group_run();
    test_password_box_run();
    test_parallax_view_run();
    test_pips_pager_run();
    test_rating_control_run();
    test_skeleton_run();
    test_scroll_bar_run();
    test_segmented_control_run();
    test_settings_panel_run();
    test_shortcut_recorder_run();
    test_split_button_run();
    test_split_view_run();
    test_swipe_control_run();
    test_tab_bar_run();
    test_tab_strip_run();
    test_tab_view_run();
    test_skeleton_loader_run();
    test_teaching_tip_run();
    test_time_picker_run();
    test_tree_view_run();
    test_toggle_button_run();
    test_toggle_split_button_run();
    test_transport_bar_run();
    test_toast_stack_run();
    test_token_input_run();
}
