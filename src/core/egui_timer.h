#ifndef _EGUI_TIMER_H_
#define _EGUI_TIMER_H_

#include "stdint.h"
#include "egui_typedef.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Canonical zero value used to reset timer fields. */
#define EGUI_TIMER_ZERO (0)

typedef struct egui_timer egui_timer_t;

/** Callback fired when a timer expires. The timer itself is passed back through `handle`. */
typedef void (*egui_timer_callback_func)(egui_timer_t *);

/** Timer node stored in the core's sorted single-linked timer queue. */
struct egui_timer
{
    struct egui_timer *next;           // next timer in expiry order
    uint32_t expiry_time;              // absolute expiry timestamp in milliseconds
    egui_timer_callback_func callback; // function invoked when the timer expires
    uint32_t period;                   // repeat period in milliseconds, or `0` for one-shot timers
    void *user_data;                   // caller-owned context pointer passed back through the timer handle
};

/** Reprogram the platform timeout source to match the timer currently at the queue head. */
void egui_timer_force_refresh_timer(egui_core_t *core);
/** Expire and dispatch every timer whose deadline is not later than the current time. */
void egui_timer_polling_work(egui_core_t *core);
/** Start or restart one timer after `ms` milliseconds. Set `period` to non-zero for repeating timers. */
int egui_timer_start_timer(egui_core_t *core, egui_timer_t *handle, uint32_t ms, uint32_t period);
/** Stop one running timer and remove it from the queue if present. */
void egui_timer_stop_timer(egui_core_t *core, egui_timer_t *handle);
/** Check whether one timer handle is currently queued and active. */
int egui_timer_check_timer_start(egui_core_t *core, egui_timer_t *handle);
/** Get the current global platform time in milliseconds. */
uint32_t egui_timer_get_current_time(void);
/** Initialize one timer handle to an idle state before its first use. */
void egui_timer_init_timer(egui_timer_t *handle, void *user_data, egui_timer_callback_func callback);
/** Initialize timer management for one core with an empty timer queue. */
void egui_timer_init(egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*!< _EGUI_TIMER_H_ */
