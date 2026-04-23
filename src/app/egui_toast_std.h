#ifndef _EGUI_TOAST_STD_H_
#define _EGUI_TOAST_STD_H_

#include "egui_toast.h"
#include "widget/egui_view_label.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

struct egui_toast_std
{
    egui_toast_t base;
    char toast_str[100];

    egui_view_label_t label;
};

/** Reserved standard-toast helper API. Keep compatibility if older code still references it. */
void egui_toast_std_set_index(egui_toast_t *self, int index);
/** Initialize the built-in label-based toast implementation. */
void egui_toast_std_init(egui_toast_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOAST_STD_H_ */
