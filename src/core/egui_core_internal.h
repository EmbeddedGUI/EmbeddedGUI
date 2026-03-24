#ifndef _EGUI_CORE_INTERNAL_H_
#define _EGUI_CORE_INTERNAL_H_

#include "egui_core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

extern egui_core_t egui_core;

void egui_core_set_pfb_scan_direction(uint8_t reverse_x, uint8_t reverse_y);
void egui_core_reset_pfb_scan_direction(void);
uint8_t egui_core_get_pfb_scan_reverse_x(void);
uint8_t egui_core_get_pfb_scan_reverse_y(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_INTERNAL_H_ */
