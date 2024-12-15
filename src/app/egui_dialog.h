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

#define EGUI_DIALOG_API_DEFINE(_name, _on_create, _on_start, _on_resume, _on_pause, _on_stop, _on_destroy)                                                     \
    static const egui_dialog_api_t EGUI_DIALOG_API_TABLE_NAME(_name) = {                                                                                       \
            .on_create = _on_create == NULL ? egui_dialog_on_create : _on_create,                                                                              \
            .on_start = _on_start == NULL ? egui_dialog_on_start : _on_start,                                                                                  \
            .on_resume = _on_resume == NULL ? egui_dialog_on_resume : _on_resume,                                                                              \
            .on_pause = _on_pause == NULL ? egui_dialog_on_pause : _on_pause,                                                                                  \
            .on_stop = _on_stop == NULL ? egui_dialog_on_stop : _on_stop,                                                                                      \
            .on_destroy = _on_destroy == NULL ? egui_dialog_on_destroy : _on_destroy,                                                                          \
    };

struct egui_dialog
{
    uint8_t state;                      // state of the dialog
    uint8_t is_need_finish;             // is need finish the dialog
    uint8_t is_cancel_on_touch_outside; // is cancel the dialog when touch outside

    egui_view_group_t root_view;      // view of the dialog
    egui_view_group_t user_root_view; // view of the dialog

    egui_activity_t *bind_activity; // activity of the dialog

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    const char *name; // name of the view
#endif

    const egui_dialog_api_t *api; // api of the view
};

void egui_dialog_set_layout(egui_dialog_t *self, egui_region_t *layout);
void egui_dialog_add_view(egui_dialog_t *self, egui_view_t *view);
void egui_dialog_set_name(egui_dialog_t *self, const char *name);

void egui_dialog_on_create(egui_dialog_t *self);
void egui_dialog_on_start(egui_dialog_t *self);
void egui_dialog_on_resume(egui_dialog_t *self);
void egui_dialog_on_pause(egui_dialog_t *self);
void egui_dialog_on_stop(egui_dialog_t *self);
void egui_dialog_on_destroy(egui_dialog_t *self);
void egui_dialog_init(egui_dialog_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_DIALOG_H_ */
