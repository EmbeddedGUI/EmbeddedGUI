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

#define EGUI_ACTIVITY_API_DEFINE(_name, _on_create, _on_start, _on_resume, _on_pause, _on_stop, _on_destroy)                                                   \
    static const egui_activity_api_t EGUI_ACTIVITY_API_TABLE_NAME(_name) = {                                                                                   \
            .on_create = _on_create == NULL ? egui_activity_on_create : _on_create,                                                                            \
            .on_start = _on_start == NULL ? egui_activity_on_start : _on_start,                                                                                \
            .on_resume = _on_resume == NULL ? egui_activity_on_resume : _on_resume,                                                                            \
            .on_pause = _on_pause == NULL ? egui_activity_on_pause : _on_pause,                                                                                \
            .on_stop = _on_stop == NULL ? egui_activity_on_stop : _on_stop,                                                                                    \
            .on_destroy = _on_destroy == NULL ? egui_activity_on_destroy : _on_destroy,                                                                        \
    };

struct egui_activity
{
    egui_dnode_t node;      // node of the activity in the activity list
    uint8_t state;          // state of the activity
    uint8_t is_need_finish; // is need finish the activity

    egui_view_group_t root_view; // view of the activity

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    const char *name; // name of the view
#endif

    const egui_activity_api_t *api; // api of the view
};

void egui_activity_set_layout(egui_activity_t *self, egui_region_t *layout);
void egui_activity_add_view(egui_activity_t *self, egui_view_t *view);
void egui_activity_set_name(egui_activity_t *self, const char *name);

void egui_activity_on_create(egui_activity_t *self);
void egui_activity_on_start(egui_activity_t *self);
void egui_activity_on_resume(egui_activity_t *self);
void egui_activity_on_pause(egui_activity_t *self);
void egui_activity_on_stop(egui_activity_t *self);
void egui_activity_on_destroy(egui_activity_t *self);
void egui_activity_init(egui_activity_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ACTIVITY_H_ */
