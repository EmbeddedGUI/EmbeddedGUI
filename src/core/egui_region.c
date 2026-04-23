#include <stdio.h>
#include <assert.h>

#include "egui_region.h"
#include "egui_api.h"

/**
 * @file egui_region.c
 * @brief Rectangle/region helpers used by layout, clipping, and dirty-region tracking.
 */

/** Initialize one non-empty region from explicit position and size. */
void egui_region_init(egui_region_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    EGUI_ASSERT(width > 0 && height > 0);
    self->location.x = x;
    self->location.y = y;
    self->size.width = width;
    self->size.height = height;
}

/** Copy one region, or reset to empty when the source pointer is `NULL`. */
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

/** Intersect one region with a `[0,width) x [0,height)` bounds box. */
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
    // Compute the clipped extents from the original top-left without changing the origin first.
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

// Full region-vs-region intersection lives in `egui_region.h` as an inline helper for hot paths.

/** Compute the bounding region that covers both input regions. */
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
    // Expand from the minimum top-left corner to the maximum bottom-right corner.
    egui_dim_t x = EGUI_MIN(self->location.x, rect->location.x);
    egui_dim_t y = EGUI_MIN(self->location.y, rect->location.y);
    result->size.width = EGUI_MAX(self->location.x + self->size.width, rect->location.x + rect->size.width) - x;
    result->size.height = EGUI_MAX(self->location.y + self->size.height, rect->location.y + rect->size.height) - y;

    result->location.x = x;
    result->location.y = y;
}

/** Return non-zero when both regions have exactly the same position and size. */
int egui_region_is_same(egui_region_t *self, const egui_region_t *other)
{
    return (self->location.x == other->location.x && self->location.y == other->location.y && self->size.width == other->size.width &&
            self->size.height == other->size.height);
}
