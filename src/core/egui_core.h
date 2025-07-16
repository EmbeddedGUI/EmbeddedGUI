#ifndef _EGUI_CORE_H_
#define _EGUI_CORE_H_

#include "egui_common.h"
#include "egui_motion_event.h"
#include "widget/egui_view_group.h"
#include "anim/egui_animation.h"
#include "app/egui_activity.h"
#include "app/egui_dialog.h"
#include "app/egui_toast.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

struct egui_core
{
    egui_color_int_t *pfb;     // pointer to frame buffer
    int pfb_width;             // width of frame buffer
    int pfb_height;            // height of frame buffer
    int pfb_total_buffer_size; // size of frame buffer in bytes, for speed up.
    int screen_width;          // width of screen
    int screen_height;         // height of screen
    int color_bytes;           // bytes per pixel

    int pfb_width_count;  // width of frame buffer count in screen width
    int pfb_height_count; // height of frame buffer count in screen width

    uint16_t unique_id; // unique id count

    egui_dlist_t activitys; // list of activitys
    egui_slist_t anims;     // list of animation

    egui_animation_t *activity_anim_start_open;   // activity anim start open
    egui_animation_t *activity_anim_start_close;  // activity anim start close
    egui_animation_t *activity_anim_finish_open;  // activity anim finish open
    egui_animation_t *activity_anim_finish_close; // activity anim finish close

    egui_activity_t *activity_open;  // activity current open
    egui_activity_t *activity_close; // activity current close

    egui_animation_t *dialog_anim_start;  // dialog anim start
    egui_animation_t *dialog_anim_finish; // dialog anim finish
    egui_dialog_t *dialog;                // dialog current open

    egui_toast_t *toast; // toast

    egui_view_group_t root_view_group;      // root view group
    egui_view_group_t user_root_view_group; // user root view group

    egui_region_t region_dirty_arr[EGUI_CONFIG_DIRTY_AREA_COUNT]; // dirty region of screen
};

void egui_core_force_refresh(void);
egui_view_group_t *egui_core_get_root_view(void);
void egui_core_add_root_view(egui_view_t *view);
int egui_core_check_region_dirty_intersect(egui_region_t *region_dirty);
void egui_core_update_region_dirty(egui_region_t *region_dirty);
void egui_core_update_region_dirty_all(void);
void egui_core_clear_region_dirty(void);
egui_view_group_t *egui_core_get_user_root_view(void);
void egui_core_add_user_root_view(egui_view_t *view);
void egui_core_remove_user_root_view(egui_view_t *view);
void egui_core_layout_childs_user_root_view(uint8_t is_orientation_horizontal, uint8_t align_type);
egui_region_t *egui_core_get_region_dirty_arr(void);
void egui_core_draw_view_group(egui_region_t *p_region_dirty, int is_debug_mode);
void egui_core_process_input_motion(egui_motion_event_t *motion_event);
uint16_t egui_core_get_unique_id(void);
void egui_core_refresh_screen(void);
void egui_core_stop_auto_refresh_screen(void);
void egui_core_set_pfb_buffer_ptr(egui_color_int_t *pfb);
egui_color_int_t *egui_core_get_pfb_buffer_ptr(void);
void egui_core_pfb_set_buffer(egui_color_int_t *pfb, uint16_t width, uint16_t height);
void egui_core_power_off(void);
void egui_core_power_on(void);
void egui_init(egui_color_int_t *pfb);

egui_activity_t *egui_core_activity_get_current(void);
void egui_core_activity_force_finish_all(void);
void egui_core_activity_force_finish_to_activity(egui_activity_t *activity);
int egui_core_activity_check_in_process(egui_activity_t *activity);
void egui_core_activity_append(egui_activity_t *activity);
void egui_core_activity_remove(egui_activity_t *activity);
void egui_core_activity_start(egui_activity_t *self, egui_activity_t *prev_activity);
void egui_core_activity_start_with_current(egui_activity_t *self);
void egui_core_activity_finish(egui_activity_t *self);
void egui_core_activity_set_start_anim(egui_animation_t *open_anim, egui_animation_t *close_anim);
void egui_core_activity_set_finish_anim(egui_animation_t *open_anim, egui_animation_t *close_anim);
egui_activity_t *egui_core_activity_get_by_view(egui_view_t *view);

egui_dialog_t *egui_core_dialog_get(void);
void egui_core_dialog_start(egui_activity_t *activity, egui_dialog_t *self);
void egui_core_dialog_start_with_current(egui_dialog_t *self);
void egui_core_dialog_finish(egui_dialog_t *self);
void egui_core_dialog_set_anim(egui_animation_t *open_anim, egui_animation_t *close_anim);

egui_toast_t *egui_core_toast_get(void);
void egui_core_toast_set(egui_toast_t *toast);
void egui_core_toast_show_info(const char *text);

void egui_core_animation_append(egui_animation_t *anim);
void egui_core_animation_remove(egui_animation_t *anim);
void egui_polling_refresh_display(void);
int egui_check_need_refresh(void);
void egui_core_draw_view_group_pre_work(void);
void egui_polling_work(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_H_ */
