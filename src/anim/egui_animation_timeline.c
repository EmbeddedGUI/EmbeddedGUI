#include "egui_animation_timeline.h"

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY

void egui_animation_timeline_init(egui_animation_timeline_t *self)
{
    uint8_t i;
    if (self == NULL)
    {
        return;
    }
    self->count = 0;
    for (i = 0; i < EGUI_CONFIG_ANIM_TIMELINE_MAX_ENTRIES; i++)
    {
        self->entries[i].anim = NULL;
        self->entries[i].start_ms = 0;
    }
}

void egui_animation_timeline_add(egui_animation_timeline_t *self, egui_animation_t *anim, uint16_t start_ms)
{
    if (self == NULL || anim == NULL)
    {
        return;
    }
    if (self->count >= EGUI_CONFIG_ANIM_TIMELINE_MAX_ENTRIES)
    {
        return; /* Timeline is full; silently ignore. */
    }
    self->entries[self->count].anim = anim;
    self->entries[self->count].start_ms = start_ms;
    self->count++;
}

void egui_animation_timeline_start(egui_animation_timeline_t *self)
{
    uint8_t i;
    if (self == NULL)
    {
        return;
    }
    for (i = 0; i < self->count; i++)
    {
        if (self->entries[i].anim == NULL)
        {
            continue;
        }
        egui_animation_set_delay(self->entries[i].anim, self->entries[i].start_ms);
        egui_animation_start(self->entries[i].anim);
    }
}

void egui_animation_timeline_stop(egui_animation_timeline_t *self)
{
    uint8_t i;
    if (self == NULL)
    {
        return;
    }
    for (i = 0; i < self->count; i++)
    {
        if (self->entries[i].anim == NULL)
        {
            continue;
        }
        egui_animation_stop(self->entries[i].anim);
    }
}

uint32_t egui_animation_timeline_get_duration(egui_animation_timeline_t *self)
{
    uint32_t max_end = 0;
    uint8_t i;

    if (self == NULL)
    {
        return 0;
    }
    for (i = 0; i < self->count; i++)
    {
        if (self->entries[i].anim == NULL)
        {
            continue;
        }
        uint32_t end = (uint32_t)self->entries[i].start_ms + (uint32_t)self->entries[i].anim->duration;
        if (end > max_end)
        {
            max_end = end;
        }
    }
    return max_end;
}

#endif /* EGUI_CONFIG_FUNCTION_ANIM_DELAY */
