#ifndef _EGUI_ACTIVITY_H_
#define _EGUI_ACTIVITY_H_

#include "widget/egui_view_group.h"
#include "core/egui_region.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

enum
{
    EGUI_ACTIVITY_STATE_NONE = 0,
    EGUI_ACTIVITY_STATE_CREATE,
    EGUI_ACTIVITY_STATE_START,
    EGUI_ACTIVITY_STATE_RESUME,
    EGUI_ACTIVITY_STATE_PAUSE,
    EGUI_ACTIVITY_STATE_STOP,
    EGUI_ACTIVITY_STATE_DESTROY
};

typedef struct egui_activity_api egui_activity_api_t;
struct egui_activity_api
{
    void (*on_create)(egui_activity_t *self);
    void (*on_start)(egui_activity_t *self);
    void (*on_resume)(egui_activity_t *self);
    void (*on_pause)(egui_activity_t *self);
    void (*on_stop)(egui_activity_t *self);
    void (*on_destroy)(egui_activity_t *self);
};

#define EGUI_ACTIVITY_API_TABLE_NAME(_name) _name##_api_table

struct egui_activity
{
    egui_dnode_t node;      // node of the activity in the activity list
    uint8_t state;          // state of the activity
    uint8_t is_need_finish; // is need finish the activity

    egui_core_t *core; // core instance this activity belongs to

    egui_view_root_group_t root_view; // view of the activity

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    const char *name; // name of the view
#endif

    const egui_activity_api_t *api; // api of the view
};

/** Set the root layout rectangle of this activity. */
void egui_activity_set_layout(egui_activity_t *self, egui_region_t *layout);
/** Add one child view to the activity root container. */
void egui_activity_add_view(egui_activity_t *self, egui_view_t *view);
/** Set the debug name shown by tracing/log helpers. */
void egui_activity_set_name(egui_activity_t *self, const char *name);
/** Return the core that owns this activity. */
egui_core_t *egui_activity_get_core(egui_activity_t *self);
/** Return the default toast bound to the same core. */
egui_toast_t *egui_activity_get_toast(egui_activity_t *self);
/** Show an info toast from this activity with an explicit duration. */
void egui_activity_show_toast_info_with_duration(egui_activity_t *self, const char *text, uint16_t duration);
/** Show an info toast from this activity using the default toast duration. */
void egui_activity_show_toast_info(egui_activity_t *self, const char *text);
/** Start a timer that is automatically associated with this activity's core. */
int egui_activity_start_timer(egui_activity_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period);
/** Stop a timer previously started from this activity. */
void egui_activity_stop_timer(egui_activity_t *self, egui_timer_t *handle);
/** Return non-zero if the timer is already running on this activity's core. */
int egui_activity_check_timer_start(egui_activity_t *self, egui_timer_t *handle);
/** Configure the enter transition used when this activity starts. */
void egui_activity_set_start_anim(egui_activity_t *self, egui_animation_t *open_anim, egui_animation_t *close_anim);
/** Configure the exit transition used when this activity finishes. */
void egui_activity_set_finish_anim(egui_activity_t *self, egui_animation_t *open_anim, egui_animation_t *close_anim);
/** Start this activity, optionally passing the previous activity for transition handling. */
void egui_activity_start(egui_activity_t *self, egui_activity_t *prev_activity);
/** Request this activity to finish and leave the current scene stack. */
void egui_activity_finish(egui_activity_t *self);
/** Return non-zero while this activity is inside a transition process. */
int egui_activity_check_in_process(egui_activity_t *self);

/** Default lifecycle entry called when the activity instance is created. */
void egui_activity_on_create(egui_activity_t *self);
/** Default lifecycle entry called when the activity becomes visible. */
void egui_activity_on_start(egui_activity_t *self);
/** Default lifecycle entry called when the activity can receive input again. */
void egui_activity_on_resume(egui_activity_t *self);
/** Default lifecycle entry called when the activity should stop handling input. */
void egui_activity_on_pause(egui_activity_t *self);
/** Default lifecycle entry called when the activity is removed from the window. */
void egui_activity_on_stop(egui_activity_t *self);
/** Default lifecycle entry called when the activity is destroyed. */
void egui_activity_on_destroy(egui_activity_t *self);
/** Initialize the base activity object and its full-screen root container. */
void egui_activity_init(egui_activity_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ACTIVITY_H_ */
