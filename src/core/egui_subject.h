#ifndef _EGUI_SUBJECT_H_
#define _EGUI_SUBJECT_H_

#include "core/egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER

/**
 * @defgroup subject Subject-Observer (data binding)
 *
 * Lightweight publish-subscribe primitive for embedded UIs.
 *
 * Usage pattern
 * -------------
 *   // 1. Declare a subject (typically inside a model struct or statically)
 *   egui_subject_t  s_counter_subject;
 *   int32_t         s_counter_value;
 *
 *   // 2. Declare observers (one per subscriber, static lifetime required)
 *   egui_observer_t s_label_obs;
 *
 *   // 3. Initialise
 *   egui_subject_init(&s_counter_subject);
 *
 *   // 4. Subscribe
 *   egui_subject_subscribe(&s_counter_subject, &s_label_obs,
 *                          on_counter_changed, my_label_view);
 *
 *   // 5. Notify (from model update)
 *   s_counter_value = 42;
 *   egui_subject_notify(&s_counter_subject, &s_counter_value);
 *
 *   // 6. Unsubscribe when the observer goes out of scope
 *   egui_subject_unsubscribe(&s_counter_subject, &s_label_obs);
 *
 * Constraints
 * -----------
 * - Zero heap: observers are embedded inside egui_observer_t which the caller
 *   allocates (typically as a struct member or static variable).
 * - Maximum observers per subject: EGUI_CONFIG_SUBJECT_MAX_OBSERVERS (default 4).
 * - Subject storage: fixed pointer array inside egui_subject_t.
 * - Thread safety: none — call only from the EmbeddedGUI main loop thread.
 * - The subject does NOT take ownership of the data pointer passed to notify().
 * - Observers are invoked in subscription order (first subscribed, first called).
 */

/* Forward declaration */
typedef struct egui_observer egui_observer_t;
typedef struct egui_subject egui_subject_t;

/**
 * Observer callback.
 * @param subject   The subject that changed.
 * @param data      Opaque pointer to the new value, as passed to egui_subject_notify().
 * @param user_data Context pointer stored when subscribing.
 */
typedef void (*egui_observer_callback_t)(egui_subject_t *subject, const void *data, void *user_data);

/**
 * Observer node.  Allocate one per subscriber with static or struct lifetime.
 * Do not copy or move after subscribing.
 */
struct egui_observer
{
    egui_observer_callback_t callback; /**< notification callback         */
    void *user_data;                   /**< caller context pointer         */
};

/**
 * Subject (observable data source).
 * Holds a fixed array of egui_observer_t pointers.
 */
struct egui_subject
{
    egui_observer_t *observers[EGUI_CONFIG_SUBJECT_MAX_OBSERVERS];
    uint8_t count; /**< number of active observers */
};

/* ----------------------------- API ------------------------------------ */

/**
 * Initialise a subject.  Must be called before any other operation.
 */
void egui_subject_init(egui_subject_t *subject);

/**
 * Subscribe an observer to a subject.
 *
 * @param subject   The subject to watch.
 * @param observer  Caller-owned observer node (static or struct member).
 *                  The observer must outlive the subscription.
 * @param callback  Function called by egui_subject_notify().
 * @param user_data Caller context forwarded verbatim to the callback.
 * @return  0 on success.
 *         -1 if arguments are NULL.
 *         -1 if the observer is already subscribed to this subject.
 *         -1 if the subject's observer array is full.
 */
int egui_subject_subscribe(egui_subject_t *subject, egui_observer_t *observer, egui_observer_callback_t callback, void *user_data);

/**
 * Unsubscribe an observer from a subject.
 * Remaining observers are compacted; relative order is preserved.
 *
 * @return  0 on success, -1 if not found.
 */
int egui_subject_unsubscribe(egui_subject_t *subject, egui_observer_t *observer);

/**
 * Notify all subscribed observers.
 *
 * @param subject  The subject that changed.
 * @param data     Pointer to the new value forwarded to every callback.
 *                 May be NULL if the observer only cares about the event.
 */
void egui_subject_notify(egui_subject_t *subject, const void *data);

/**
 * Remove all observers from a subject without calling any callbacks.
 */
void egui_subject_clear(egui_subject_t *subject);

/**
 * Return the number of active observers.
 */
uint8_t egui_subject_observer_count(const egui_subject_t *subject);

#endif /* EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_SUBJECT_H_ */
