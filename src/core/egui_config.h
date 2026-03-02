#ifndef _EGUI_CONFIG_H_
#define _EGUI_CONFIG_H_

#include "app_egui_config.h"
#include "egui_config_default.h"

// PC port: force 32-bit color depth for native RGB888 display
#ifdef EGUI_PORT_PC
#undef EGUI_CONFIG_COLOR_DEPTH
#define EGUI_CONFIG_COLOR_DEPTH 32
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_H_ */
