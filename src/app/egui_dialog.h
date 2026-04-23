#ifndef _EGUI_DIALOG_H_
#define _EGUI_DIALOG_H_

#include "widget/egui_view_group.h"
#include "core/egui_region.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

enum
{
    EGUI_DIALOG_STATE_NONE = 0,
    EGUI_DIALOG_STATE_CREATE,
    EGUI_DIALOG_STATE_START,
    EGUI_DIALOG_STATE_RESUME,
    EGUI_DIALOG_STATE_PAUSE,
    EGUI_DIALOG_STATE_STOP,
    EGUI_DIALOG_STATE_DESTROY
};

typedef struct egui_dialog_api egui_dialog_api_t;
struct egui_dialog_api
{
    void (*on_create)(egui_dialog_t *self);
    void (*on_start)(egui_dialog_t *self);
    void (*on_resume)(egui_dialog_t *self);
    void (*on_pause)(egui_dialog_t *self);
    void (*on_stop)(egui_dialog_t *self);
    void (*on_destroy)(egui_dialog_t *self);
};

#define EGUI_DIALOG_API_TABLE_NAME(_name) _name##_api_table

struct egui_dialog
{
    uint8_t state;                      // state of the dialog
    uint8_t is_need_finish;             // is need finish the dialog
    uint8_t is_cancel_on_touch_outside; // is cancel the dialog when touch outside

    egui_core_t *core; // core instance this dialog belongs to

    egui_view_root_group_t root_view;      // view of the dialog
    egui_view_root_group_t user_root_view; // view of the dialog

    egui_activity_t *bind_activity; // activity of the dialog

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    const char *name; // name of the view
#endif

    const egui_dialog_api_t *api; // api of the view
};

/** Set the layout rectangle of the dialog user content area. */
void egui_dialog_set_layout(egui_dialog_t *self, egui_region_t *layout);
/** Add one child view to the dialog user content root. */
void egui_dialog_add_view(egui_dialog_t *self, egui_view_t *view);
/** Set the debug name shown by tracing/log helpers. */
void egui_dialog_set_name(egui_dialog_t *self, const char *name);
/** Return the core that owns this dialog. */
egui_core_t *egui_dialog_get_core(egui_dialog_t *self);
/** Return the default toast bound to the same core. */
egui_toast_t *egui_dialog_get_toast(egui_dialog_t *self);
/** Show an info toast from this dialog with an explicit duration. */
void egui_dialog_show_toast_info_with_duration(egui_dialog_t *self, const char *text, uint16_t duration);
/** Show an info toast from this dialog using the default toast duration. */
void egui_dialog_show_toast_info(egui_dialog_t *self, const char *text);
/** Start a timer that is automatically associated with this dialog's core. */
int egui_dialog_start_timer(egui_dialog_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period);
/** Stop a timer previously started from this dialog. */
void egui_dialog_stop_timer(egui_dialog_t *self, egui_timer_t *handle);
/** Return non-zero if the timer is already running on this dialog's core. */
int egui_dialog_check_timer_start(egui_dialog_t *self, egui_timer_t *handle);
/** Configure the open and close transition animations of the dialog. */
void egui_dialog_set_anim(egui_dialog_t *self, egui_animation_t *open_anim, egui_animation_t *close_anim);
/** Start the dialog on top of the given activity. */
void egui_dialog_start(egui_dialog_t *self, egui_activity_t *activity);
/** Return non-zero while the dialog is inside a transition process. */
int egui_dialog_check_in_process(egui_dialog_t *self);
/** Request the dialog to close. */
void egui_dialog_finish(egui_dialog_t *self);

/** Default lifecycle entry called when the dialog instance is created. */
void egui_dialog_on_create(egui_dialog_t *self);
/** Default lifecycle entry called when the dialog becomes visible. */
void egui_dialog_on_start(egui_dialog_t *self);
/** Default lifecycle entry called when the dialog can receive input. */
void egui_dialog_on_resume(egui_dialog_t *self);
/** Default lifecycle entry called when the dialog should stop handling input. */
void egui_dialog_on_pause(egui_dialog_t *self);
/** Default lifecycle entry called when the dialog is removed from the window. */
void egui_dialog_on_stop(egui_dialog_t *self);
/** Default lifecycle entry called when the dialog is destroyed. */
void egui_dialog_on_destroy(egui_dialog_t *self);
/** Initialize the dialog overlay root and the user content root. */
void egui_dialog_init(egui_dialog_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_DIALOG_H_ */
