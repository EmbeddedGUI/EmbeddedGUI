#ifndef _EGUI_CORE_TOAST_H_
#define _EGUI_CORE_TOAST_H_

#include "egui_core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Get the toast instance currently registered with the core. */
egui_toast_t *egui_core_toast_get(egui_core_t *core);
/** Register or clear the default toast instance used by higher-level helpers. */
void egui_core_toast_set(egui_core_t *core, egui_toast_t *toast);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_TOAST_H_ */
