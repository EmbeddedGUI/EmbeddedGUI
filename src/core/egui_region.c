#include <stdio.h>
#include <assert.h>

#include "egui_region.h"
#include "egui_api.h"


// Initialize a rectangle
void egui_region_init(egui_region_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    EGUI_ASSERT(width > 0 && height > 0);
    self->location.x = x;
    self->location.y = y;
    self->size.width = width;
    self->size.height = height;
}

void egui_region_copy(egui_region_t *self, const egui_region_t *rect)
{
    if (rect == NULL)
    {
        egui_region_init_empty(self);
        return;
    }
    self->location.x = rect->location.x;
    self->location.y = rect->location.y;
    self->size.width = rect->size.width;
    self->size.height = rect->size.height;
}

// intersect
void egui_region_intersect_with_size(egui_region_t *self, egui_dim_t width, egui_dim_t height, egui_region_t *result)
{
    if (egui_region_is_empty(self))
    {
        egui_region_init_empty(result);
        return;
    }
    if ((width <= 0) || (height <= 0))
    {
        egui_region_init_empty(result);
        return;
    }
    // avoid x,y overflow
    egui_dim_t x = self->location.x;
    egui_dim_t y = self->location.y;
    result->size.width = EGUI_MIN(self->location.x + self->size.width, width) - x;
    result->size.height = EGUI_MIN(self->location.y + self->size.height, height) - y;

    result->location.x = x;
    result->location.y = y;
    if (result->size.width <= 0 || result->size.height <= 0)
    {
        egui_region_init_empty(result);
    }
}

int egui_region_is_intersect(egui_region_t *self, const egui_region_t *rect)
{
    egui_region_t result;
    egui_region_intersect(self, rect, &result);
    return !egui_region_is_empty(&result);
}

void egui_region_union(egui_region_t *self, const egui_region_t *rect, egui_region_t *result)
{
    if (egui_region_is_empty(self))
    {
        egui_region_copy(result, rect);
        return;
    }
    if (egui_region_is_empty((egui_region_t *)rect))
    {
        egui_region_copy(result, self);
        return;
    }
    // avoid x,y overflow
    egui_dim_t x = EGUI_MIN(self->location.x, rect->location.x);
    egui_dim_t y = EGUI_MIN(self->location.y, rect->location.y);
    result->size.width = EGUI_MAX(self->location.x + self->size.width, rect->location.x + rect->size.width) - x;
    result->size.height = EGUI_MAX(self->location.y + self->size.height, rect->location.y + rect->size.height) - y;

    result->location.x = x;
    result->location.y = y;
}

int egui_region_is_same(egui_region_t *self, const egui_region_t *other)
{
    return (self->location.x == other->location.x &&
            self->location.y == other->location.y &&
            self->size.width == other->size.width &&
            self->size.height == other->size.height);
}
