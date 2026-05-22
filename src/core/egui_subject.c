/**
 * @file egui_subject.c
 * @brief Lightweight Subject-Observer (publish-subscribe) implementation.
 *
 * See egui_subject.h for the full API documentation.
 */

#include "egui_subject.h"

#if EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER

void egui_subject_init(egui_subject_t *subject)
{
    uint8_t i;

    if (subject == NULL)
    {
        return;
    }
    for (i = 0; i < EGUI_CONFIG_SUBJECT_MAX_OBSERVERS; i++)
    {
        subject->observers[i] = NULL;
    }
    subject->count = 0;
}

int egui_subject_subscribe(egui_subject_t *subject, egui_observer_t *observer, egui_observer_callback_t callback, void *user_data)
{
    uint8_t i;

    if (subject == NULL || observer == NULL || callback == NULL)
    {
        return -1;
    }

    /* Reject duplicates */
    for (i = 0; i < subject->count; i++)
    {
        if (subject->observers[i] == observer)
        {
            return -1;
        }
    }

    if (subject->count >= EGUI_CONFIG_SUBJECT_MAX_OBSERVERS)
    {
        return -1;
    }

    observer->callback = callback;
    observer->user_data = user_data;
    subject->observers[subject->count] = observer;
    subject->count++;
    return 0;
}

int egui_subject_unsubscribe(egui_subject_t *subject, egui_observer_t *observer)
{
    uint8_t i;
    uint8_t found = 0;

    if (subject == NULL || observer == NULL)
    {
        return -1;
    }

    for (i = 0; i < subject->count; i++)
    {
        if (subject->observers[i] == observer)
        {
            found = 1;
        }
        if (found && i + 1 < subject->count)
        {
            subject->observers[i] = subject->observers[i + 1];
        }
    }

    if (!found)
    {
        return -1;
    }

    subject->count--;
    subject->observers[subject->count] = NULL;
    return 0;
}

void egui_subject_notify(egui_subject_t *subject, const void *data)
{
    uint8_t i;
    uint8_t snapshot_count;

    if (subject == NULL)
    {
        return;
    }

    /* Snapshot count so late-added observers are not called in this pass */
    snapshot_count = subject->count;
    for (i = 0; i < snapshot_count; i++)
    {
        egui_observer_t *obs = subject->observers[i];
        if (obs != NULL && obs->callback != NULL)
        {
            obs->callback(subject, data, obs->user_data);
        }
    }
}

void egui_subject_clear(egui_subject_t *subject)
{
    uint8_t i;

    if (subject == NULL)
    {
        return;
    }
    for (i = 0; i < subject->count; i++)
    {
        subject->observers[i] = NULL;
    }
    subject->count = 0;
}

uint8_t egui_subject_observer_count(const egui_subject_t *subject)
{
    if (subject == NULL)
    {
        return 0;
    }
    return subject->count;
}

#endif /* EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER */
