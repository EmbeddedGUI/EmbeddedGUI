#ifndef _EGUI_TIMER_H_
#define _EGUI_TIMER_H_

#include "stdint.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_TIMER_ZERO (0)

typedef struct egui_timer egui_timer_t;

typedef void (*egui_timer_callback_func)(egui_timer_t *);

struct egui_timer
{
    struct egui_timer *next;
    uint32_t expiry_time;
    egui_timer_callback_func callback;
    uint32_t period;
    void *user_data;
};

void egui_timer_force_refresh_timer(void);
void egui_timer_polling_work(void);
int egui_timer_start_timer(egui_timer_t *handle, uint32_t ms, uint32_t period);
void egui_timer_stop_timer(egui_timer_t *handle);
int egui_timer_check_timer_start(egui_timer_t *handle);
uint32_t egui_timer_get_current_time(void);
void egui_timer_init(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*!< _EGUI_TIMER_H_ */
