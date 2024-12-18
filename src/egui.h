#ifndef _EGUI_H_
#define _EGUI_H_

#include "core/egui_config.h"

#include "app/egui_activity.h"
#include "app/egui_dialog.h"
#include "app/egui_toast.h"
#include "app/egui_toast_std.h"

#include "anim/egui_animation.h"
#include "anim/egui_animation_set.h"
#include "anim/egui_animation_alpha.h"
#include "anim/egui_animation_scale_size.h"
#include "anim/egui_animation_translate.h"
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
#include "background/egui_background_image.h"
#include "background/egui_background.h"

#include "core/egui_api.h"
#include "core/egui_common.h"
#include "core/egui_theme.h"
#include "core/egui_timer.h"
#include "core/egui_core.h"
#include "core/egui_input.h"

#include "font/egui_font.h"
#include "font/egui_font_lattice.h"
#include "font/egui_font_std.h"

#include "image/egui_image.h"
#include "image/egui_image_std.h"

#include "mask/egui_mask.h"
#include "mask/egui_mask_circle.h"
#include "mask/egui_mask_round_rectangle.h"
#include "mask/egui_mask_image.h"

#include "resource/egui_resource.h"

#include "widget/egui_view.h"
#include "widget/egui_view_group.h"
#include "widget/egui_view_button.h"
#include "widget/egui_view_label.h"
#include "widget/egui_view_linearlayout.h"
#include "widget/egui_view_progress_bar.h"
#include "widget/egui_view_image.h"
#include "widget/egui_view_scroll.h"
#include "widget/egui_view_switch.h"
#include "widget/egui_view_viewpage.h"
#include "widget/egui_view_viewpage_cache.h"

#include "utils/egui_utils.h"

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
