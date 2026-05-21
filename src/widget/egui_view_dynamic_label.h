#ifndef _EGUI_VIEW_DYNAMIC_LABEL_H_
#define _EGUI_VIEW_DYNAMIC_LABEL_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Fixed character capacity stored inside one dynamic label instance. */
#define EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE 20

typedef struct egui_view_dynamic_label egui_view_dynamic_label_t;
/**
 * @brief Label variant that owns a small in-place text buffer.
 *
 * Unlike the plain label widget, this wrapper copies caller text into local
 * storage so short dynamic strings can live safely with the widget itself.
 */
struct egui_view_dynamic_label
{
    egui_view_label_t base; /* Embedded label implementation reused for measurement and drawing. */

    char text_buffer[EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE]; /* Fixed local storage for the current text. */
};

// ============== Dynamic Label Params (reuse Label) ==============
/** Reuse the label-style parameter macro for dynamic-label construction. */
#define EGUI_VIEW_DYNAMIC_LABEL_PARAMS_INIT        EGUI_VIEW_LABEL_PARAMS_INIT
/** Reuse the simplified label-style parameter macro for dynamic-label construction. */
#define EGUI_VIEW_DYNAMIC_LABEL_PARAMS_INIT_SIMPLE EGUI_VIEW_LABEL_PARAMS_INIT_SIMPLE

/** Apply label-style parameters while copying the text into owned storage. */
void egui_view_dynamic_label_apply_params(egui_view_t *self, const egui_view_label_params_t *params);
/** Copy text into the fixed internal buffer. Longer strings are truncated to `MAX_SIZE`. */
void egui_view_dynamic_label_set_text(egui_view_t *self, const char *text);
/** Return the owned dynamic-label text buffer, or NULL when self is NULL. */
const char *egui_view_dynamic_label_get_text(egui_view_t *self);
/** Initialize a label whose text storage lives inside the widget instance. */
void egui_view_dynamic_label_init(egui_view_t *self, egui_core_t *core);
/** Initialize a dynamic label and apply the same parameter block used by label widgets. */
void egui_view_dynamic_label_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_label_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_DYNAMIC_LABEL_H_ */
