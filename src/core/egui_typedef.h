#ifndef _EGUI_TYPEDEF_H_
#define _EGUI_TYPEDEF_H_

/**
 * @file egui_typedef.h
 * @brief Central forward declarations shared across core modules.
 *
 * This header contains only lightweight type declarations so other headers can refer to core objects without pulling in full struct definitions and creating
 * circular include dependencies.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Animation subsystem */
typedef struct egui_animation egui_animation_t;
typedef struct egui_interpolator egui_interpolator_t;

/* Page and transient scene objects */
typedef struct egui_activity egui_activity_t;
typedef struct egui_page_base egui_page_base_t;
typedef struct egui_dialog egui_dialog_t;
typedef struct egui_toast egui_toast_t;
typedef struct egui_toast_std egui_toast_std_t;

/* Visual assets */
typedef struct egui_background egui_background_t;
typedef int egui_base_t; // saved platform interrupt state or other small platform tokens
typedef struct egui_font egui_font_t;
typedef struct egui_font_std egui_font_std_t;
typedef struct egui_image egui_image_t;
typedef struct egui_image_file egui_image_file_t;
typedef struct egui_image_file_decoder egui_image_file_decoder_t;
typedef struct egui_image_file_io egui_image_file_io_t;
typedef struct egui_image_file_open_result egui_image_file_open_result_t;
typedef struct egui_mask egui_mask_t;

/* Core runtime and view tree */
typedef struct egui_core egui_core_t;
typedef struct egui_theme egui_theme_t;
typedef struct egui_view egui_view_t;
typedef struct egui_view_group egui_view_group_t;

/* Rendering helpers */
typedef struct egui_canvas egui_canvas_t;
typedef struct egui_circle_info egui_circle_info_t;
typedef struct egui_gradient egui_gradient_t;
typedef struct egui_shadow egui_shadow_t;

/* Geometry and diagnostics */
typedef struct egui_location egui_location_t;
typedef struct egui_size egui_size_t;
typedef struct egui_region egui_region_t;
typedef struct egui_mem_monitor egui_mem_monitor_t;

typedef struct egui_key_event egui_key_event_t;

/* Input, platform, and display backends */
typedef struct egui_focus_manager egui_focus_manager_t;
typedef struct egui_event egui_event_t;
typedef struct egui_event_listener egui_event_listener_t;
typedef struct egui_focus_group egui_focus_group_t;

typedef struct egui_display_driver egui_display_driver_t;
typedef struct egui_display_driver_ops egui_display_driver_ops_t;
typedef struct egui_touch_driver egui_touch_driver_t;
typedef struct egui_touch_driver_ops egui_touch_driver_ops_t;
typedef struct egui_platform egui_platform_t;
typedef struct egui_platform_ops egui_platform_ops_t;
typedef struct egui_pfb_manager egui_pfb_manager_t;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TYPEDEF_H_ */
