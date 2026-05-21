#ifndef _EGUI_FOCUS_GROUP_H_
#define _EGUI_FOCUS_GROUP_H_

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_FOCUS_GROUP

typedef enum egui_focus_group_wrap
{
    EGUI_FOCUS_GROUP_WRAP = 0,
    EGUI_FOCUS_GROUP_CLAMP = 1,
} egui_focus_group_wrap_t;

struct egui_focus_group
{
    egui_core_t *core;
    egui_view_t *views[EGUI_CONFIG_FOCUS_GROUP_MAX_VIEWS];
    uint8_t count;
    uint8_t index;
    uint8_t wrap_mode;
};

void egui_focus_group_init(egui_focus_group_t *group, egui_core_t *core);
int egui_focus_group_add_view(egui_focus_group_t *group, egui_view_t *view);
int egui_focus_group_remove_view(egui_focus_group_t *group, egui_view_t *view);
void egui_focus_group_clear(egui_focus_group_t *group);
void egui_focus_group_set_wrap_mode(egui_focus_group_t *group, egui_focus_group_wrap_t mode);
int egui_focus_group_focus_index(egui_focus_group_t *group, uint8_t index);
int egui_focus_group_focus_next(egui_focus_group_t *group);
int egui_focus_group_focus_prev(egui_focus_group_t *group);
egui_view_t *egui_focus_group_get_focused_view(egui_focus_group_t *group);
uint8_t egui_focus_group_get_count(egui_focus_group_t *group);
uint8_t egui_focus_group_get_index(egui_focus_group_t *group);

#endif /* EGUI_CONFIG_FUNCTION_FOCUS_GROUP */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_FOCUS_GROUP_H_ */
