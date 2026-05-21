#ifndef _EGUI_MSGBOX_H_
#define _EGUI_MSGBOX_H_

#include "egui_dialog.h"
#include "widget/egui_view_button.h"
#include "widget/egui_view_group.h"
#include "widget/egui_view_label.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_MSGBOX

typedef struct egui_msgbox egui_msgbox_t;
typedef void (*egui_msgbox_button_cb_t)(egui_msgbox_t *msgbox, uint8_t index, const char *text, void *user_data);

struct egui_msgbox
{
    egui_dialog_t dialog;
    egui_view_group_t panel;
    egui_view_label_t title;
    egui_view_label_t message;
    egui_view_button_t buttons[EGUI_CONFIG_MSGBOX_MAX_BUTTONS];
    const char *button_texts[EGUI_CONFIG_MSGBOX_MAX_BUTTONS];
    egui_msgbox_button_cb_t button_cb;
    void *user_data;
    uint8_t button_count;
};

void egui_msgbox_init(egui_msgbox_t *self, egui_core_t *core);
void egui_msgbox_set_text(egui_msgbox_t *self, const char *title, const char *message);
int egui_msgbox_set_buttons(egui_msgbox_t *self, const char *const *texts, uint8_t count, egui_msgbox_button_cb_t cb, void *user_data);
void egui_msgbox_show(egui_msgbox_t *self, egui_activity_t *activity);
void egui_msgbox_close(egui_msgbox_t *self);
egui_dialog_t *egui_msgbox_get_dialog(egui_msgbox_t *self);
egui_view_t *egui_msgbox_get_button(egui_msgbox_t *self, uint8_t index);

#endif /* EGUI_CONFIG_FUNCTION_MSGBOX */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_MSGBOX_H_ */
