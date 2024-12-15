#ifndef _EGUI_TYPEDEF_H_
#define _EGUI_TYPEDEF_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_animation egui_animation_t;

typedef struct egui_interpolator egui_interpolator_t;

typedef struct egui_activity egui_activity_t;

typedef struct egui_dialog egui_dialog_t;

typedef struct egui_toast egui_toast_t;
typedef struct egui_toast_std egui_toast_std_t;

typedef struct egui_background egui_background_t;

typedef int egui_base_t;

typedef struct egui_font egui_font_t;
typedef struct egui_font_std egui_font_std_t;

typedef struct egui_image egui_image_t;
typedef struct egui_mask egui_mask_t;
typedef struct egui_core egui_core_t;
typedef struct egui_view egui_view_t;
typedef struct egui_view_group egui_view_group_t;

typedef struct egui_canvas egui_canvas_t;

typedef struct egui_location egui_location_t;
typedef struct egui_size egui_size_t;
typedef struct egui_region egui_region_t;


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TYPEDEF_H_ */
