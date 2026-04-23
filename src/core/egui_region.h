#ifndef _EGUI_REGION_H_
#define _EGUI_REGION_H_

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

struct egui_location
{
    egui_dim_t x; // horizontal coordinate in screen space
    egui_dim_t y; // vertical coordinate in screen space
};

/** Define one location initialized to `(0, 0)`. */
#define EGUI_LOCATION_DEFINE_EMPTY(_name)   egui_location_t _name = {0, 0}
/** Define one location with explicit `x` and `y` values. */
#define EGUI_LOCATION_DEFINE(_name, _x, _y) egui_location_t _name = {_x, _y}

struct egui_size
{
    egui_dim_t width;  // width of a rectangular area
    egui_dim_t height; // height of a rectangular area
};

struct egui_region
{
    egui_location_t location; // top-left corner of the region
    egui_size_t size;         // region width and height
};

/** Define one empty region initialized to `(0, 0, 0, 0)`. */
#define EGUI_REGION_DEFINE_EMPTY(_name)                    egui_region_t _name = {{0, 0}, {0, 0}}
/** Define one region from explicit location and size values. */
#define EGUI_REGION_DEFINE(_name, _x, _y, _width, _height) egui_region_t _name = {{_x, _y}, {_width, _height}}

/** Check whether one point lies inside the region. The right and bottom edges are exclusive. */
__EGUI_STATIC_INLINE__ int egui_region_pt_in_rect(const egui_region_t *self, egui_dim_t x, egui_dim_t y)
{
    return x >= self->location.x && x < self->location.x + self->size.width && y >= self->location.y && y < self->location.y + self->size.height;
}

/** Compare two regions for exact location and size equality. */
__EGUI_STATIC_INLINE__ int egui_region_equal(const egui_region_t *self, const egui_region_t *rect)
{
    return (self->location.x == rect->location.x && self->location.y == rect->location.y) &&
           (self->size.width == rect->size.width && self->size.height == rect->size.height);
}

/** Reset one region to the canonical empty state `(0, 0, 0, 0)`. */
__EGUI_STATIC_INLINE__ void egui_region_init_empty(egui_region_t *self)
{
    self->location.x = self->location.y = 0;
    self->size.width = self->size.height = 0;
}

/** Check whether a region is empty. `NULL` is treated as empty. */
__EGUI_STATIC_INLINE__ int egui_region_is_empty(egui_region_t *self)
{
    if (self == NULL)
    {
        return true;
    }
    return self->size.width <= 0 || self->size.height <= 0;
}

/** Compute the intersection of two regions, returning an empty result when they do not overlap. */
__EGUI_STATIC_INLINE__ void egui_region_intersect(egui_region_t *self, const egui_region_t *rect, egui_region_t *result)
{
    if (egui_region_is_empty(self) || egui_region_is_empty((egui_region_t *)rect))
    {
        egui_region_init_empty(result);
        return;
    }
    // avoid x,y overflow
    egui_dim_t x = EGUI_MAX(self->location.x, rect->location.x);
    egui_dim_t y = EGUI_MAX(self->location.y, rect->location.y);
    result->size.width = EGUI_MIN(self->location.x + self->size.width, rect->location.x + rect->size.width) - x;
    result->size.height = EGUI_MIN(self->location.y + self->size.height, rect->location.y + rect->size.height) - y;

    result->location.x = x;
    result->location.y = y;

    // if (result->size.width <= 0 || result->size.height <= 0)
    // {
    //     egui_region_init_empty(result);
    // }
}

/** Fast intersection helper for hot paths where both input regions are already known non-empty. */
__EGUI_STATIC_INLINE__ void egui_region_intersect_fast(const egui_region_t *self, const egui_region_t *rect, egui_region_t *result)
{
    egui_dim_t x = EGUI_MAX(self->location.x, rect->location.x);
    egui_dim_t y = EGUI_MAX(self->location.y, rect->location.y);
    result->size.width = EGUI_MIN(self->location.x + self->size.width, rect->location.x + rect->size.width) - x;
    result->size.height = EGUI_MIN(self->location.y + self->size.height, rect->location.y + rect->size.height) - y;
    result->location.x = x;
    result->location.y = y;
}

/** Reset one region to the canonical empty state `(0, 0, 0, 0)`. */
void egui_region_init_empty(egui_region_t *self);
/** Check whether a region is empty. `NULL` is treated as empty. */
int egui_region_is_empty(egui_region_t *self);
/** Initialize one non-empty region from explicit location and size values. */
void egui_region_init(egui_region_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height);
/** Check whether one point lies inside the region. The right and bottom edges are exclusive. */
int egui_region_pt_in_rect(const egui_region_t *self, egui_dim_t x, egui_dim_t y);
/** Compare two regions for exact location and size equality. */
int egui_region_equal(const egui_region_t *self, const egui_region_t *rect);
/** Copy one region. Passing `NULL` as the source resets the destination to empty. */
void egui_region_copy(egui_region_t *self, const egui_region_t *rect);
/** Intersect one region with the top-left area `[0, width) x [0, height)`. */
void egui_region_intersect_with_size(egui_region_t *self, egui_dim_t width, egui_dim_t height, egui_region_t *result);
/** Fast overlap test that uses only boundary comparisons and treats touching edges as non-overlapping. */
__EGUI_STATIC_INLINE__ int egui_region_is_intersect(egui_region_t *self, const egui_region_t *rect)
{
    return (self->location.x < rect->location.x + rect->size.width) && (self->location.x + self->size.width > rect->location.x) &&
           (self->location.y < rect->location.y + rect->size.height) && (self->location.y + self->size.height > rect->location.y);
}
/** Compute the minimal region that covers both inputs. */
void egui_region_union(egui_region_t *self, const egui_region_t *rect, egui_region_t *result);
/** Alias of exact region equality, kept for older call sites. */
int egui_region_is_same(egui_region_t *self, const egui_region_t *other);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_REGION_H_ */
