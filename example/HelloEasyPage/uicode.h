#ifndef _UICODE_H_
#define _UICODE_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define TEST_ACTIVITY_DYNAMIC_ALLOC 1

int uicode_start_next_page(void);
int uicode_start_prev_page(void);
void uicode_create_ui(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _UICODE_H_ */
