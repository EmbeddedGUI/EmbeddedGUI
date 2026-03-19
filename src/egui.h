#ifndef _EGUI_H_
#define _EGUI_H_

#include "core/egui_config.h"

#include "app/egui_activity.h"
#include "app/egui_dialog.h"
#include "app/egui_toast.h"
#include "app/egui_toast_std.h"
#include "app/egui_page_base.h"

#include "anim/egui_animation.h"
#include "anim/egui_animation_set.h"
#include "anim/egui_animation_alpha.h"
#include "anim/egui_animation_scale_size.h"
#include "anim/egui_animation_translate.h"
#include "anim/egui_animation_resize.h"
#include "anim/egui_animation_color.h"
#include "anim/egui_interpolator.h"
#include "anim/egui_interpolator_accelerate.h"
#include "anim/egui_interpolator_accelerate_decelerate.h"
#include "anim/egui_interpolator_anticipate.h"
#include "anim/egui_interpolator_anticipate_overshoot.h"
#include "anim/egui_interpolator_bounce.h"
#include "anim/egui_interpolator_cycle.h"
#include "anim/egui_interpolator_decelerate.h"
#include "anim/egui_interpolator_overshoot.h"
#include "anim/egui_interpolator_linear.h"

#include "background/egui_background_color.h"
#include "background/egui_background_gradient.h"
#include "background/egui_background_image.h"
#include "background/egui_background.h"

#include "core/egui_api.h"
#include "core/egui_common.h"
#include "core/egui_oop.h"
#include "core/egui_timer.h"
#include "core/egui_core.h"
#include "core/egui_input.h"
#include "core/egui_display_driver.h"
#include "core/egui_platform.h"
#include "core/egui_pfb_manager.h"
#include "core/egui_rotation.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
#include "core/egui_touch_driver.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#include "core/egui_key_event.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "core/egui_focus.h"
#endif

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "font/egui_font.h"
#include "font/egui_font_lattice.h"
#include "font/egui_font_std.h"

#include "image/egui_image.h"
#include "image/egui_image_std.h"

#include "mask/egui_mask.h"
#include "mask/egui_mask_circle.h"
#include "mask/egui_mask_round_rectangle.h"
#include "mask/egui_mask_image.h"

#include "shadow/egui_shadow.h"

#include "resource/egui_resource.h"

#include "widget/egui_view.h"
#include "widget/egui_view_group.h"
#include "widget/egui_view_button.h"
#include "widget/egui_view_dynamic_label.h"
#include "widget/egui_view_label.h"
#include "widget/egui_view_autocomplete.h"
#include "widget/egui_view_chips.h"
#include "widget/egui_view_linearlayout.h"
#include "widget/egui_view_mp4.h"
#include "widget/egui_view_progress_bar.h"
#include "widget/egui_view_image.h"
#include "widget/egui_view_animated_image.h"
#include "widget/egui_view_scroll.h"
#include "widget/egui_view_virtual_list.h"
#include "widget/egui_view_virtual_viewport.h"
#include "widget/egui_view_list.h"
#include "widget/egui_view_switch.h"
#include "widget/egui_view_viewpage.h"
#include "widget/egui_view_viewpage_cache.h"
#include "widget/egui_view_checkbox.h"
#include "widget/egui_view_radio_button.h"
#include "widget/egui_view_slider.h"
#include "widget/egui_view_circular_progress_bar.h"
#include "widget/egui_view_image_button.h"
#include "widget/egui_view_divider.h"
#include "widget/egui_view_page_indicator.h"
#include "widget/egui_view_gauge.h"
#include "widget/egui_view_number_picker.h"
#include "widget/egui_view_tab_bar.h"
#include "widget/egui_view_segmented_control.h"
#include "widget/egui_view_pattern_lock.h"
#include "widget/egui_view_gridlayout.h"
#include "widget/egui_view_led.h"
#include "widget/egui_view_toggle_button.h"
#include "widget/egui_view_spinner.h"
#include "widget/egui_view_card.h"
#include "widget/egui_view_arc_slider.h"
#include "widget/egui_view_roller.h"
#include "widget/egui_view_chart_common.h"
#include "widget/egui_view_chart_line.h"
#include "widget/egui_view_chart_scatter.h"
#include "widget/egui_view_chart_bar.h"
#include "widget/egui_view_chart_pie.h"
#include "widget/egui_view_textblock.h"
#include "widget/egui_view_combobox.h"
#include "widget/egui_view_notification_badge.h"
#include "widget/egui_view_activity_ring.h"
#include "widget/egui_view_heart_rate.h"
#include "widget/egui_view_compass.h"
#include "widget/egui_view_analog_clock.h"
#include "widget/egui_view_stopwatch.h"
#include "widget/egui_view_digital_clock.h"
#include "widget/egui_view_mini_calendar.h"
#include "widget/egui_view_line.h"
#include "widget/egui_view_scale.h"
#include "widget/egui_view_button_matrix.h"
#include "widget/egui_view_stepper.h"
#include "widget/egui_view_table.h"
#include "widget/egui_view_spangroup.h"
#include "widget/egui_view_tileview.h"
#include "widget/egui_view_window.h"
#include "widget/egui_view_menu.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "widget/egui_view_textinput.h"
#include "widget/egui_view_keyboard.h"
#endif

#include "utils/egui_utils.h"

#include "test/egui_test.h"

#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
#include "app_egui_resource_generate.h"
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_H_ */
