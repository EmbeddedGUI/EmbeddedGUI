#ifndef _EGUI_REGION_H_
#define _EGUI_REGION_H_

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

struct egui_location
{
    egui_dim_t x; // x in Cartesian coordinate system
    egui_dim_t y; // y in Cartesian coordinate system
};

#define EGUI_LOCATION_DEFINE_EMPTY(_name)   egui_location_t _name = {0, 0}
#define EGUI_LOCATION_DEFINE(_name, _x, _y) egui_location_t _name = {_x, _y}

struct egui_size
{
    egui_dim_t width;  // width of an rectangular area
    egui_dim_t height; // height of an rectangular area
};

struct egui_region
{
    egui_location_t location; // origin of the region
    egui_size_t size;         // size of the region
};

#define EGUI_REGION_DEFINE_EMPTY(_name)                    egui_region_t _name = {{0, 0}, {0, 0}}
#define EGUI_REGION_DEFINE(_name, _x, _y, _width, _height) egui_region_t _name = {{_x, _y}, {_width, _height}}

// Check if a point is inside the rectangle
__EGUI_STATIC_INLINE__ int egui_region_pt_in_rect(const egui_region_t *self, egui_dim_t x, egui_dim_t y)
{
    return x >= self->location.x && x < self->location.x + self->size.width && y >= self->location.y && y < self->location.y + self->size.height;
}

// Compare two rectangles for equality
__EGUI_STATIC_INLINE__ int egui_region_equal(const egui_region_t *self, const egui_region_t *rect)
{
    return (self->location.x == rect->location.x && self->location.y == rect->location.y) &&
           (self->size.width == rect->size.width && self->size.height == rect->size.height);
}


// Initialize an empty rectangle
__EGUI_STATIC_INLINE__ void egui_region_init_empty(egui_region_t *self)
{
    self->location.x = self->location.y = 0;
    self->size.width = self->size.height = 0;
}

// Check if a rectangle is empty
__EGUI_STATIC_INLINE__ int egui_region_is_empty(egui_region_t *self)
{
    if (self == NULL)
    {
        return true;
    }
    return self->size.width <= 0 || self->size.height <= 0;
}

// intersect
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

void egui_region_init_empty(egui_region_t *self);
int egui_region_is_empty(egui_region_t *self);
void egui_region_init(egui_region_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height);
int egui_region_pt_in_rect(const egui_region_t *self, egui_dim_t x, egui_dim_t y);
int egui_region_equal(const egui_region_t *self, const egui_region_t *rect);
void egui_region_copy(egui_region_t *self, const egui_region_t *rect);
// void egui_region_intersect(egui_region_t *self, const egui_region_t *rect, egui_region_t *result);
void egui_region_intersect_with_size(egui_region_t *self, egui_dim_t width, egui_dim_t height, egui_region_t *result);
int egui_region_is_intersect(egui_region_t *self, const egui_region_t *rect);
void egui_region_union(egui_region_t *self, const egui_region_t *rect, egui_region_t *result);
int egui_region_is_same(egui_region_t *self, const egui_region_t *other);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_REGION_H_ */
