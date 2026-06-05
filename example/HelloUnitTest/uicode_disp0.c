#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"

#include "test/test_activity_ring_dirty.h"
#include "test/test_activity_ring_get_state.h"
#include "test/test_analog_clock_get_state.h"
#include "test/test_app_timer_helpers.h"
#include "test/test_animation.h"
#include "test/test_animated_image_get_state.h"
#include "test/test_autocomplete_get_state.h"
#include "test/test_basic_widget_dirty.h"
#include "test/test_button_get_state.h"
#include "test/test_button_matrix_get_state.h"
#include "test/test_card_get_state.h"
#include "test/test_canvas_active.h"
#include "test/test_canvas_panner.h"
#include "test/test_canvas_viewport.h"
#include "test/test_circular_widget_dirty.h"
#include "test/test_chart_get_state.h"
#include "test/test_chips_get_state.h"
#include "test/test_combobox.h"
#include "test/test_combobox_get_state.h"
#include "test/test_common.h"
#include "test/test_compass_get_state.h"
#include "test/test_compact_text.h"
#include "test/test_deferred_image.h"
#include "test/test_deferred_image_get_state.h"
#include "test/test_digital_clock_get_state.h"
#include "test/test_divider_get_state.h"
#include "test/test_dirty_region.h"
#include "test/test_dlist.h"
#include "test/test_dynamic_label_get_text.h"
#include "test/test_event_lite.h"
#include "test/test_fixmath.h"
#include "test/test_font_std.h"
#include "test/test_focus_key_navigation.h"
#include "test/test_focus_group.h"
#include "test/test_flexlayout_get_state.h"
#include "test/test_image_file.h"
#include "test/test_i18n_event.h"
#include "test/test_grid_view.h"
#include "test/test_gridlayout_get_state.h"
#include "test/test_image_button_get_state.h"
#include "test/test_image_codec.h"
#include "test/test_image_svg.h"
#include "test/test_input.h"
#include "test/test_interpolator.h"
#include "test/test_invalidate_region.h"
#include "test/test_keyboard_get_state.h"
#include "test/test_line_get_state.h"
#include "test/test_linearlayout.h"
#include "test/test_list_get_state.h"
#include "test/test_list_view.h"
#include "test/test_lyric_scroller.h"
#include "test/test_lyric_scroller_get_state.h"
#include "test/test_mask.h"
#include "test/test_menu.h"
#include "test/test_mini_calendar.h"
#include "test/test_mini_calendar_get_state.h"
#include "test/test_mp4_get_state.h"
#include "test/test_msgbox.h"
#include "test/test_notification_badge.h"
#include "test/test_dirty_passthrough_container.h"
#include "test/test_number_picker.h"
#include "test/test_page_activity_lifecycle.h"
#include "test/test_page_indicator_get_state.h"
#include "test/test_pattern_lock_get_state.h"
#include "test/test_radio_button_get_state.h"
#include "test/test_region.h"
#include "test/test_ringbuffer.h"
#include "test/test_scale_get_state.h"
#include "test/test_segmented_control_get_state.h"
#include "test/test_slist.h"
#include "test/test_spangroup_get_state.h"
#include "test/test_stepper_get_state.h"
#include "test/test_stopwatch_get_state.h"
#include "test/test_tab_bar.h"
#include "test/test_table_get_state.h"
#include "test/test_tileview_get_state.h"
#include "test/test_toast.h"
#include "test/test_textblock.h"
#include "test/test_textblock_get_state.h"
#include "test/test_view.h"
#include "test/test_view_group.h"
#include "test/test_view_layer.h"
#include "test/test_virtual_stage.h"
#include "test/test_virtual_viewport.h"
#include "test/test_widget_timer_lifecycle.h"
#include "test/test_window_get_state.h"
#include "test/test_flexlayout.h"
#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE
#include "test/test_style.h"
#endif
#if EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER
#include "test/test_subject.h"
#endif
#if EGUI_CONFIG_FUNCTION_FONT_TTF
#include "test/test_font_ttf.h"
#endif
#if EGUI_CONFIG_FUNCTION_ENCODER
#include "test/test_encoder.h"
#endif
#if EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA
#include "test/test_ext_click_area.h"
#endif
#if EGUI_CONFIG_FUNCTION_LONG_PRESS
#include "test/test_long_press.h"
#endif
#include "test/test_textinput_password.h"
#if EGUI_CONFIG_FUNCTION_SWIPE_LISTENER
#include "test/test_swipe_listener.h"
#endif
#if EGUI_CONFIG_FUNCTION_SCROLL_SNAP
#include "test/test_scroll_snap.h"
#endif
#if EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW
#include "test/test_draggable_view.h"
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
#include "test/test_image_transform.h"
#endif
#include "test/test_anim_value.h"
#if EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
#include "test/test_label_long_mode.h"
#endif
#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
#include "test/test_scroll_listener.h"
#endif
#include "test/test_scroll_to_child.h"
#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
#include "test/test_view_state_styles.h"
#endif
#if EGUI_CONFIG_FUNCTION_ANIM_DELAY || EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB
#include "test/test_anim_delay.h"
#endif
#include "test/test_view_fade.h"
#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
#include "test/test_scroll_horizontal.h"
#endif
#if EGUI_CONFIG_FUNCTION_LABEL_RECOLOR
#include "test/test_label_recolor.h"
#endif
#include "test/test_view_periodic.h"
#if EGUI_CONFIG_FUNCTION_VIEW_USER_DATA
#include "test/test_view_user_data.h"
#endif
#if EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE
#include "test/test_label_letter_space.h"
#endif
#include "test/test_scroll_to_pos.h"
#include "test/test_child_index.h"
#if EGUI_CONFIG_FUNCTION_ANIM_DELAY
#include "test/test_anim_timeline.h"
#endif
#if EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP && EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
#include "test/test_label_word_wrap.h"
#endif
#include "test/test_label_get_text.h"
#include "test/test_scroll_get_scroll_x.h"
#include "test/test_progress_bar_get.h"
#include "test/test_progress_bar_get_state.h"
#include "test/test_property_lite.h"
#include "test/test_anim_is_running.h"
#if EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT
#include "test/test_label_text_fmt.h"
#endif
#include "test/test_move_child_to_index.h"
#include "test/test_view_get_alpha.h"
#include "test/test_view_get_size.h"
#include "test/test_view_spacing_align.h"
#include "test/test_view_get_pos.h"
#include "test/test_checkbox_get_checked.h"
#include "test/test_switch_get_checked.h"
#include "test/test_anim_duration_get.h"
#include "test/test_view_get_parent.h"
#include "test/test_anim_get_delay.h"
#include "test/test_anim_repeat_count_get.h"
#include "test/test_label_get_align_type.h"
#include "test/test_label_get_line_space.h"
#include "test/test_slider_get_max_value.h"
#include "test/test_slider_get_state.h"
#include "test/test_checkbox_get_state.h"
#include "test/test_circular_progress_bar_get_process.h"
#include "test/test_viewpage_cache_get_state.h"
#include "test/test_viewpage_get_current_page.h"
#include "test/test_viewpage_get_state.h"
#include "test/test_image_get_image.h"
#include "test/test_number_picker_get_min_value.h"
#include "test/test_number_picker_get_max_value.h"
#include "test/test_number_picker_get_step.h"
#include "test/test_number_picker_get_state.h"
#include "test/test_led_get_is_on.h"
#include "test/test_led_get_is_blinking.h"
#include "test/test_tab_bar_get_current_index.h"
#include "test/test_tab_bar_get_state.h"
#include "test/test_spinner_is_spinning.h"
#include "test/test_toggle_button_get_text.h"
#include "test/test_toggle_button_get_state.h"
#include "test/test_label_get_font.h"
#include "test/test_textinput_get_cursor_pos.h"
#include "test/test_textinput_get_max_length.h"
#include "test/test_textinput_get_placeholder.h"
#include "test/test_textinput_get_state.h"
#include "test/test_checkbox_get_text.h"
#include "test/test_slider_get_is_dragging.h"
#include "test/test_arc_slider_get_is_dragging.h"
#include "test/test_arc_slider_get_state.h"
#include "test/test_label_get_font_color.h"
#include "test/test_label_get_alpha.h"
#include "test/test_combobox_get_item_count.h"
#include "test/test_roller_get_item_count.h"
#include "test/test_roller_get_state.h"
#include "test/test_anim_repeat_mode_get.h"
#include "test/test_anim_interpolator_get.h"
#include "test/test_anim_target_view_get.h"
#include "test/test_anim_is_fill_before_get.h"
#include "test/test_anim_is_fill_after_get.h"
#include "test/test_label_get_letter_space.h"
#include "test/test_checkbox_get_text_color.h"
#include "test/test_roller_get_selected_text.h"
#include "test/test_scroll_get_scrollbar_enabled.h"
#include "test/test_scroll_get_horizontal.h"
#include "test/test_led_get_blink_period.h"
#include "test/test_spinner_get_color.h"
#include "test/test_circular_pb_get_stroke_width.h"
#include "test/test_circular_pb_get_progress_color.h"
#include "test/test_circular_pb_get_state.h"
#include "test/test_tab_bar_get_tab_count.h"
#include "test/test_viewpage_get_page_count.h"
#include "test/test_circular_pb_get_bk_color.h"
#include "test/test_circular_pb_get_text_color.h"
#include "test/test_progress_bar_get_bk_color.h"
#include "test/test_progress_bar_get_progress_color.h"
#include "test/test_arc_slider_get_track_color.h"
#include "test/test_arc_slider_get_active_color.h"
#include "test/test_arc_slider_get_thumb_color.h"
#include "test/test_arc_slider_get_stroke_width.h"
#include "test/test_arc_slider_get_thumb_radius.h"
#include "test/test_arc_slider_get_start_angle.h"
#include "test/test_arc_slider_get_sweep_angle.h"
#include "test/test_slider_get_track_color.h"
#include "test/test_slider_get_active_color.h"
#include "test/test_slider_get_thumb_color.h"
#include "test/test_led_get_on_color.h"
#include "test/test_led_get_off_color.h"
#include "test/test_tab_bar_get_text_color.h"
#include "test/test_tab_bar_get_active_text_color.h"
#include "test/test_tab_bar_get_indicator_color.h"
#include "test/test_tab_bar_get_alpha.h"
#include "test/test_led_get_border_color.h"
#include "test/test_led_get_border_width.h"
#include "test/test_tab_bar_get_icon_text_gap.h"
#include "test/test_number_picker_get_text_color.h"
#include "test/test_number_picker_get_button_color.h"
#include "test/test_number_picker_get_alpha.h"
#include "test/test_checkbox_get_box_color.h"
#include "test/test_checkbox_get_check_color.h"
#include "test/test_checkbox_get_box_fill_color.h"
#include "test/test_checkbox_get_alpha.h"
#include "test/test_checkbox_get_text_gap.h"
#include "test/test_switch_get_bk_color_on.h"
#include "test/test_switch_get_bk_color_off.h"
#include "test/test_switch_get_switch_color_on.h"
#include "test/test_switch_get_switch_color_off.h"
#include "test/test_switch_get_alpha.h"
#include "test/test_switch_get_state.h"
#include "test/test_progress_bar_get_control_color.h"
#include "test/test_progress_bar_get_is_show_control.h"
#include "test/test_roller_get_text_color.h"
#include "test/test_roller_get_highlight_color.h"
#include "test/test_roller_get_selected_text_color.h"
#include "test/test_roller_get_visible_count.h"
#include "test/test_roller_get_font.h"
#include "test/test_gauge_get_value.h"
#include "test/test_gauge_get_bk_color.h"
#include "test/test_gauge_get_progress_color.h"
#include "test/test_gauge_get_state.h"
#include "test/test_anim_pause_resume.h"
#include "test/test_group_get_last_child.h"
#include "test/test_heart_rate_get_state.h"
#include "test/test_scroll_is_at_top_bottom.h"
#include "test/test_view_get_content_size.h"
#include "test/test_scroll_scroll_by.h"
#include "test/test_view_get_screen_pos.h"

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
    if (uicode_should_run_suite("dynamic_label_get_text"))
        test_dynamic_label_get_text_run();
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
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    if (uicode_should_run_suite("event_lite"))
        test_event_lite_run();
#endif
#if EGUI_CONFIG_FUNCTION_FOCUS_GROUP
    if (uicode_should_run_suite("focus_group"))
        test_focus_group_run();
#endif
    if (uicode_should_run_suite("i18n_event"))
        test_i18n_event_run();
    if (uicode_should_run_suite("image_file"))
        test_image_file_run();
    if (uicode_should_run_suite("deferred_image"))
        test_deferred_image_run();
    if (uicode_should_run_suite("deferred_image_get_state"))
        test_deferred_image_get_state_run();
    if (uicode_should_run_suite("digital_clock_get_state"))
        test_digital_clock_get_state_run();
    if (uicode_should_run_suite("divider_get_state"))
        test_divider_get_state_run();
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
    if (uicode_should_run_suite("canvas_viewport"))
        test_canvas_viewport_run();
    if (uicode_should_run_suite("line_get_state"))
        test_line_get_state_run();
    if (uicode_should_run_suite("linearlayout"))
        test_linearlayout_run();
    if (uicode_should_run_suite("list_view"))
        test_list_view_run();
    if (uicode_should_run_suite("list_get_state"))
        test_list_get_state_run();
    if (uicode_should_run_suite("interpolator"))
        test_interpolator_run();
    if (uicode_should_run_suite("animation"))
        test_animation_run();
    if (uicode_should_run_suite("animated_image_get_state"))
        test_animated_image_get_state_run();
    if (uicode_should_run_suite("dirty_region"))
        test_dirty_region_run();
    if (uicode_should_run_suite("dirty_passthrough_container"))
        test_dirty_passthrough_container_run();
    if (uicode_should_run_suite("activity_ring_dirty"))
        test_activity_ring_dirty_run();
    if (uicode_should_run_suite("activity_ring_get_state"))
        test_activity_ring_get_state_run();
    if (uicode_should_run_suite("analog_clock_get_state"))
        test_analog_clock_get_state_run();
    if (uicode_should_run_suite("autocomplete_get_state"))
        test_autocomplete_get_state_run();
    if (uicode_should_run_suite("basic_widget_dirty"))
        test_basic_widget_dirty_run();
    if (uicode_should_run_suite("button_get_state"))
        test_button_get_state_run();
    if (uicode_should_run_suite("button_matrix_get_state"))
        test_button_matrix_get_state_run();
    if (uicode_should_run_suite("card_get_state"))
        test_card_get_state_run();
    if (uicode_should_run_suite("chart_get_state"))
        test_chart_get_state_run();
    if (uicode_should_run_suite("chips_get_state"))
        test_chips_get_state_run();
    if (uicode_should_run_suite("circular_widget_dirty"))
        test_circular_widget_dirty_run();
    if (uicode_should_run_suite("invalidate_region"))
        test_invalidate_region_run();
    if (uicode_should_run_suite("canvas_active"))
        test_canvas_active_run();
    if (uicode_should_run_suite("combobox"))
        test_combobox_run();
    if (uicode_should_run_suite("combobox_get_state"))
        test_combobox_get_state_run();
    if (uicode_should_run_suite("view_layer"))
        test_view_layer_run();
    if (uicode_should_run_suite("compass_get_state"))
        test_compass_get_state_run();
    if (uicode_should_run_suite("compact_text"))
        test_compact_text_run();
    if (uicode_should_run_suite("grid_view"))
        test_grid_view_run();
    if (uicode_should_run_suite("gridlayout_get_state"))
        test_gridlayout_get_state_run();
    if (uicode_should_run_suite("image_button_get_state"))
        test_image_button_get_state_run();
    if (uicode_should_run_suite("image_codec"))
        test_image_codec_run();
    if (uicode_should_run_suite("image_svg"))
        test_image_svg_run();
    if (uicode_should_run_suite("input"))
        test_input_run();
    if (uicode_should_run_suite("keyboard_get_state"))
        test_keyboard_get_state_run();
    if (uicode_should_run_suite("lyric_scroller"))
        test_lyric_scroller_run();
    if (uicode_should_run_suite("lyric_scroller_get_state"))
        test_lyric_scroller_get_state_run();
    if (uicode_should_run_suite("menu"))
        test_menu_run();
#if EGUI_CONFIG_FUNCTION_MSGBOX
    if (uicode_should_run_suite("msgbox"))
        test_msgbox_run();
#endif
    if (uicode_should_run_suite("mini_calendar"))
        test_mini_calendar_run();
    if (uicode_should_run_suite("mini_calendar_get_state"))
        test_mini_calendar_get_state_run();
    if (uicode_should_run_suite("mp4_get_state"))
        test_mp4_get_state_run();
    if (uicode_should_run_suite("notification_badge"))
        test_notification_badge_run();
    if (uicode_should_run_suite("number_picker"))
        test_number_picker_run();
    if (uicode_should_run_suite("page_activity_lifecycle"))
        test_page_activity_lifecycle_run();
    if (uicode_should_run_suite("page_indicator_get_state"))
        test_page_indicator_get_state_run();
    if (uicode_should_run_suite("pattern_lock_get_state"))
        test_pattern_lock_get_state_run();
    if (uicode_should_run_suite("scale_get_state"))
        test_scale_get_state_run();
    if (uicode_should_run_suite("segmented_control_get_state"))
        test_segmented_control_get_state_run();
    if (uicode_should_run_suite("spangroup_get_state"))
        test_spangroup_get_state_run();
    if (uicode_should_run_suite("stepper_get_state"))
        test_stepper_get_state_run();
    if (uicode_should_run_suite("stopwatch_get_state"))
        test_stopwatch_get_state_run();
    if (uicode_should_run_suite("tab_bar"))
        test_tab_bar_run();
    if (uicode_should_run_suite("table_get_state"))
        test_table_get_state_run();
    if (uicode_should_run_suite("tileview_get_state"))
        test_tileview_get_state_run();
    if (uicode_should_run_suite("toast"))
        test_toast_run();
    if (uicode_should_run_suite("textblock"))
        test_textblock_run();
    if (uicode_should_run_suite("textblock_get_state"))
        test_textblock_get_state_run();
    if (uicode_should_run_suite("virtual_stage"))
        test_virtual_stage_run();
    if (uicode_should_run_suite("virtual_viewport"))
        test_virtual_viewport_run();
    if (uicode_should_run_suite("widget_timer_lifecycle"))
        test_widget_timer_lifecycle_run();
    if (uicode_should_run_suite("window_get_state"))
        test_window_get_state_run();
    if (uicode_should_run_suite("flexlayout_get_state"))
        test_flexlayout_get_state_run();
    if (uicode_should_run_suite("flexlayout"))
        test_flexlayout_run();
#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE
    if (uicode_should_run_suite("style"))
        test_style_run();
#endif
#if EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER
    if (uicode_should_run_suite("subject"))
        test_subject_run();
#endif
#if EGUI_CONFIG_FUNCTION_FONT_TTF
    if (uicode_should_run_suite("font_ttf"))
        test_font_ttf_run();
#endif
#if EGUI_CONFIG_FUNCTION_ENCODER
    if (uicode_should_run_suite("encoder"))
        test_encoder_run();
#endif
#if EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA
    if (uicode_should_run_suite("ext_click_area"))
        test_ext_click_area_run();
#endif
#if EGUI_CONFIG_FUNCTION_LONG_PRESS
    if (uicode_should_run_suite("long_press"))
        test_long_press_run();
#endif
    if (uicode_should_run_suite("textinput_password"))
        test_textinput_password_run();
#if EGUI_CONFIG_FUNCTION_SWIPE_LISTENER
    if (uicode_should_run_suite("swipe_listener"))
        test_swipe_listener_run();
#endif
#if EGUI_CONFIG_FUNCTION_SCROLL_SNAP
    if (uicode_should_run_suite("scroll_snap"))
        test_scroll_snap_run();
#endif
#if EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW
    if (uicode_should_run_suite("draggable_view"))
        test_draggable_view_run();
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
    if (uicode_should_run_suite("image_transform"))
        test_image_transform_run();
#endif
    if (uicode_should_run_suite("anim_value"))
        test_anim_value_run();
#if EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
    if (uicode_should_run_suite("label_long_mode"))
        test_label_long_mode_run();
#endif
#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
    if (uicode_should_run_suite("scroll_listener"))
        test_scroll_listener_run();
#endif
    if (uicode_should_run_suite("scroll_to_child"))
        test_scroll_to_child_run();
#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
    if (uicode_should_run_suite("view_state_styles"))
        test_view_state_styles_run();
#endif
#if EGUI_CONFIG_FUNCTION_ANIM_DELAY || EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB
    if (uicode_should_run_suite("anim_delay"))
        test_anim_delay_run();
#endif
    if (uicode_should_run_suite("view_fade"))
        test_view_fade_run();
#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
    if (uicode_should_run_suite("scroll_horizontal"))
        test_scroll_horizontal_run();
#endif
#if EGUI_CONFIG_FUNCTION_LABEL_RECOLOR
    if (uicode_should_run_suite("label_recolor"))
        test_label_recolor_run();
#endif
    if (uicode_should_run_suite("view_periodic"))
        test_view_periodic_run();
#if EGUI_CONFIG_FUNCTION_VIEW_USER_DATA
    if (uicode_should_run_suite("view_user_data"))
        test_view_user_data_run();
#endif
#if EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE
    if (uicode_should_run_suite("label_letter_space"))
        test_label_letter_space_run();
#endif
    if (uicode_should_run_suite("scroll_to_pos"))
        test_scroll_to_pos_run();
    if (uicode_should_run_suite("child_index"))
        test_child_index_run();
#if EGUI_CONFIG_FUNCTION_ANIM_DELAY
    if (uicode_should_run_suite("anim_timeline"))
        test_anim_timeline_run();
#endif
#if EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP && EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
    if (uicode_should_run_suite("label_word_wrap"))
        test_label_word_wrap_run();
#endif
    if (uicode_should_run_suite("label_get_text"))
        test_label_get_text_run();
    if (uicode_should_run_suite("scroll_get_scroll_x"))
        test_scroll_get_scroll_x_run();
    if (uicode_should_run_suite("progress_bar_get"))
        test_progress_bar_get_run();
    if (uicode_should_run_suite("progress_bar_get_state"))
        test_progress_bar_get_state_run();
#if EGUI_CONFIG_FUNCTION_PROPERTY_LITE
    if (uicode_should_run_suite("property_lite"))
        test_property_lite_run();
#endif
    if (uicode_should_run_suite("anim_is_running"))
        test_anim_is_running_run();
#if EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT
    if (uicode_should_run_suite("label_text_fmt"))
        test_label_text_fmt_run();
#endif
    if (uicode_should_run_suite("move_child_to_index"))
        test_move_child_to_index_run();
    if (uicode_should_run_suite("view_get_alpha"))
        test_view_get_alpha_run();
    if (uicode_should_run_suite("view_get_size"))
        test_view_get_size_run();
    if (uicode_should_run_suite("view_spacing_align"))
        test_view_spacing_align_run();
    if (uicode_should_run_suite("view_get_pos"))
        test_view_get_pos_run();
    if (uicode_should_run_suite("checkbox_get_checked"))
        test_checkbox_get_checked_run();
    if (uicode_should_run_suite("switch_get_checked"))
        test_switch_get_checked_run();
    if (uicode_should_run_suite("anim_duration_get"))
        test_anim_duration_get_run();
    if (uicode_should_run_suite("view_get_parent"))
        test_view_get_parent_run();
    if (uicode_should_run_suite("anim_get_delay"))
        test_anim_get_delay_run();
    if (uicode_should_run_suite("anim_repeat_count_get"))
        test_anim_repeat_count_get_run();
    if (uicode_should_run_suite("label_get_align_type"))
        test_label_get_align_type_run();
    if (uicode_should_run_suite("label_get_line_space"))
        test_label_get_line_space_run();
    if (uicode_should_run_suite("slider_get_max_value"))
        test_slider_get_max_value_run();
    if (uicode_should_run_suite("slider_get_state"))
        test_slider_get_state_run();
    if (uicode_should_run_suite("circular_progress_bar_get_process"))
        test_circular_progress_bar_get_process_run();
    if (uicode_should_run_suite("viewpage_cache_get_state"))
        test_viewpage_cache_get_state_run();
    if (uicode_should_run_suite("viewpage_get_current_page"))
        test_viewpage_get_current_page_run();
    if (uicode_should_run_suite("viewpage_get_state"))
        test_viewpage_get_state_run();
    if (uicode_should_run_suite("image_get_image"))
        test_image_get_image_run();
    if (uicode_should_run_suite("number_picker_get_min_value"))
        test_number_picker_get_min_value_run();
    if (uicode_should_run_suite("number_picker_get_max_value"))
        test_number_picker_get_max_value_run();
    if (uicode_should_run_suite("number_picker_get_step"))
        test_number_picker_get_step_run();
    if (uicode_should_run_suite("number_picker_get_state"))
        test_number_picker_get_state_run();
    if (uicode_should_run_suite("led_get_is_on"))
        test_led_get_is_on_run();
    if (uicode_should_run_suite("led_get_is_blinking"))
        test_led_get_is_blinking_run();
    if (uicode_should_run_suite("tab_bar_get_current_index"))
        test_tab_bar_get_current_index_run();
    if (uicode_should_run_suite("tab_bar_get_state"))
        test_tab_bar_get_state_run();
    if (uicode_should_run_suite("spinner_is_spinning"))
        test_spinner_is_spinning_run();
    if (uicode_should_run_suite("toggle_button_get_text"))
        test_toggle_button_get_text_run();
    if (uicode_should_run_suite("toggle_button_get_state"))
        test_toggle_button_get_state_run();
    if (uicode_should_run_suite("label_get_font"))
        test_label_get_font_run();
    if (uicode_should_run_suite("textinput_get_cursor_pos"))
        test_textinput_get_cursor_pos_run();
    if (uicode_should_run_suite("textinput_get_max_length"))
        test_textinput_get_max_length_run();
    if (uicode_should_run_suite("textinput_get_placeholder"))
        test_textinput_get_placeholder_run();
    if (uicode_should_run_suite("textinput_get_state"))
        test_textinput_get_state_run();
    if (uicode_should_run_suite("checkbox_get_text"))
        test_checkbox_get_text_run();
    if (uicode_should_run_suite("slider_get_is_dragging"))
        test_slider_get_is_dragging_run();
    if (uicode_should_run_suite("arc_slider_get_is_dragging"))
        test_arc_slider_get_is_dragging_run();
    if (uicode_should_run_suite("arc_slider_get_state"))
        test_arc_slider_get_state_run();
    if (uicode_should_run_suite("label_get_font_color"))
        test_label_get_font_color_run();
    if (uicode_should_run_suite("label_get_alpha"))
        test_label_get_alpha_run();
    if (uicode_should_run_suite("combobox_get_item_count"))
        test_combobox_get_item_count_run();
    if (uicode_should_run_suite("roller_get_item_count"))
        test_roller_get_item_count_run();
    if (uicode_should_run_suite("roller_get_state"))
        test_roller_get_state_run();
    if (uicode_should_run_suite("anim_repeat_mode_get"))
        test_anim_repeat_mode_get_run();
    if (uicode_should_run_suite("anim_interpolator_get"))
        test_anim_interpolator_get_run();
    if (uicode_should_run_suite("anim_target_view_get"))
        test_anim_target_view_get_run();
    if (uicode_should_run_suite("anim_is_fill_before_get"))
        test_anim_is_fill_before_get_run();
    if (uicode_should_run_suite("anim_is_fill_after_get"))
        test_anim_is_fill_after_get_run();
    if (uicode_should_run_suite("label_get_letter_space"))
        test_label_get_letter_space_run();
    if (uicode_should_run_suite("checkbox_get_text_color"))
        test_checkbox_get_text_color_run();
    if (uicode_should_run_suite("roller_get_selected_text"))
        test_roller_get_selected_text_run();
    if (uicode_should_run_suite("scroll_get_scrollbar_enabled"))
        test_scroll_get_scrollbar_enabled_run();
    if (uicode_should_run_suite("scroll_get_horizontal"))
        test_scroll_get_horizontal_run();
    if (uicode_should_run_suite("led_get_blink_period"))
        test_led_get_blink_period_run();
    if (uicode_should_run_suite("spinner_get_color"))
        test_spinner_get_color_run();
    if (uicode_should_run_suite("circular_pb_get_stroke_width"))
        test_circular_pb_get_stroke_width_run();
    if (uicode_should_run_suite("circular_pb_get_progress_color"))
        test_circular_pb_get_progress_color_run();
    if (uicode_should_run_suite("circular_pb_get_state"))
        test_circular_pb_get_state_run();
    if (uicode_should_run_suite("tab_bar_get_tab_count"))
        test_tab_bar_get_tab_count_run();
    if (uicode_should_run_suite("viewpage_get_page_count"))
        test_viewpage_get_page_count_run();
    if (uicode_should_run_suite("circular_pb_get_bk_color"))
        test_circular_pb_get_bk_color_run();
    if (uicode_should_run_suite("circular_pb_get_text_color"))
        test_circular_pb_get_text_color_run();
    if (uicode_should_run_suite("progress_bar_get_bk_color"))
        test_progress_bar_get_bk_color_run();
    if (uicode_should_run_suite("progress_bar_get_progress_color"))
        test_progress_bar_get_progress_color_run();
    if (uicode_should_run_suite("arc_slider_get_track_color"))
        test_arc_slider_get_track_color_run();
    if (uicode_should_run_suite("arc_slider_get_active_color"))
        test_arc_slider_get_active_color_run();
    if (uicode_should_run_suite("arc_slider_get_thumb_color"))
        test_arc_slider_get_thumb_color_run();
    if (uicode_should_run_suite("arc_slider_get_stroke_width"))
        test_arc_slider_get_stroke_width_run();
    if (uicode_should_run_suite("arc_slider_get_thumb_radius"))
        test_arc_slider_get_thumb_radius_run();
    if (uicode_should_run_suite("arc_slider_get_start_angle"))
        test_arc_slider_get_start_angle_run();
    if (uicode_should_run_suite("arc_slider_get_sweep_angle"))
        test_arc_slider_get_sweep_angle_run();
    if (uicode_should_run_suite("slider_get_track_color"))
        test_slider_get_track_color_run();
    if (uicode_should_run_suite("slider_get_active_color"))
        test_slider_get_active_color_run();
    if (uicode_should_run_suite("slider_get_thumb_color"))
        test_slider_get_thumb_color_run();
    if (uicode_should_run_suite("led_get_on_color"))
        test_led_get_on_color_run();
    if (uicode_should_run_suite("led_get_off_color"))
        test_led_get_off_color_run();
    if (uicode_should_run_suite("tab_bar_get_text_color"))
        test_tab_bar_get_text_color_run();
    if (uicode_should_run_suite("tab_bar_get_active_text_color"))
        test_tab_bar_get_active_text_color_run();
    if (uicode_should_run_suite("tab_bar_get_indicator_color"))
        test_tab_bar_get_indicator_color_run();
    if (uicode_should_run_suite("tab_bar_get_alpha"))
        test_tab_bar_get_alpha_run();
    if (uicode_should_run_suite("led_get_border_color"))
        test_led_get_border_color_run();
    if (uicode_should_run_suite("led_get_border_width"))
        test_led_get_border_width_run();
    if (uicode_should_run_suite("tab_bar_get_icon_text_gap"))
        test_tab_bar_get_icon_text_gap_run();
    if (uicode_should_run_suite("number_picker_get_text_color"))
        test_number_picker_get_text_color_run();
    if (uicode_should_run_suite("number_picker_get_button_color"))
        test_number_picker_get_button_color_run();
    if (uicode_should_run_suite("number_picker_get_alpha"))
        test_number_picker_get_alpha_run();
    if (uicode_should_run_suite("checkbox_get_box_color"))
        test_checkbox_get_box_color_run();
    if (uicode_should_run_suite("checkbox_get_check_color"))
        test_checkbox_get_check_color_run();
    if (uicode_should_run_suite("checkbox_get_box_fill_color"))
        test_checkbox_get_box_fill_color_run();
    if (uicode_should_run_suite("checkbox_get_alpha"))
        test_checkbox_get_alpha_run();
    if (uicode_should_run_suite("checkbox_get_text_gap"))
        test_checkbox_get_text_gap_run();
    if (uicode_should_run_suite("checkbox_get_state"))
        test_checkbox_get_state_run();
    if (uicode_should_run_suite("radio_button_get_state"))
        test_radio_button_get_state_run();
    if (uicode_should_run_suite("switch_get_bk_color_on"))
        test_switch_get_bk_color_on_run();
    if (uicode_should_run_suite("switch_get_bk_color_off"))
        test_switch_get_bk_color_off_run();
    if (uicode_should_run_suite("switch_get_switch_color_on"))
        test_switch_get_switch_color_on_run();
    if (uicode_should_run_suite("switch_get_switch_color_off"))
        test_switch_get_switch_color_off_run();
    if (uicode_should_run_suite("switch_get_alpha"))
        test_switch_get_alpha_run();
    if (uicode_should_run_suite("switch_get_state"))
        test_switch_get_state_run();
    if (uicode_should_run_suite("progress_bar_get_control_color"))
        test_progress_bar_get_control_color_run();
    if (uicode_should_run_suite("progress_bar_get_is_show_control"))
        test_progress_bar_get_is_show_control_run();
    if (uicode_should_run_suite("roller_get_text_color"))
        test_roller_get_text_color_run();
    if (uicode_should_run_suite("roller_get_highlight_color"))
        test_roller_get_highlight_color_run();
    if (uicode_should_run_suite("roller_get_selected_text_color"))
        test_roller_get_selected_text_color_run();
    if (uicode_should_run_suite("roller_get_visible_count"))
        test_roller_get_visible_count_run();
    if (uicode_should_run_suite("roller_get_font"))
        test_roller_get_font_run();
    if (uicode_should_run_suite("gauge_get_value"))
        test_gauge_get_value_run();
    if (uicode_should_run_suite("gauge_get_bk_color"))
        test_gauge_get_bk_color_run();
    if (uicode_should_run_suite("gauge_get_progress_color"))
        test_gauge_get_progress_color_run();
    if (uicode_should_run_suite("gauge_get_state"))
        test_gauge_get_state_run();
    if (uicode_should_run_suite("anim_pause_resume"))
        test_anim_pause_resume_run();
    if (uicode_should_run_suite("group_get_last_child"))
        test_group_get_last_child_run();
    if (uicode_should_run_suite("heart_rate_get_state"))
        test_heart_rate_get_state_run();
    if (uicode_should_run_suite("scroll_is_at_top_bottom"))
        test_scroll_is_at_top_bottom_run();
    if (uicode_should_run_suite("view_get_content_size"))
        test_view_get_content_size_run();
    if (uicode_should_run_suite("scroll_scroll_by"))
        test_scroll_scroll_by_run();
    if (uicode_should_run_suite("view_get_screen_pos"))
        test_view_get_screen_pos_run();
}

egui_core_t *uicode_get_core(void)
{
    return s_core;
}
