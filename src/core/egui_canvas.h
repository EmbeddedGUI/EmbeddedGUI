#ifndef _EGUI_CANVAS_H_
#define _EGUI_CANVAS_H_

#include "egui_common.h"
#include "egui_region.h"
#include "mask/egui_mask.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_font egui_font_t;
typedef struct egui_image egui_image_t;
typedef struct egui_mask egui_mask_t;

typedef struct egui_circle_item egui_circle_item_t;
struct egui_circle_item
{
    uint16_t start_offset : 10;
    uint16_t valid_count : 6; // In radius 999, max valid count is 42.
    uint16_t data_offset;
};

typedef struct egui_circle_info egui_circle_info_t;
struct egui_circle_info
{
    uint16_t radius;
    uint16_t item_count;
    const void *items;
    const uint8_t *data; // point to data buffer for circle drawing
};

typedef struct egui_canvas egui_canvas_t;
struct egui_canvas
{
    egui_color_int_t *pfb;    // pointer to frame buffer
    egui_region_t pfb_region; // location of the region in the frame buffer that can be used for drawing

    egui_region_t base_view_work_region; // pfb region intersect with the base view region. For fast drawing. in base view coordinates.

    egui_location_t pfb_location_in_base_view; // pfb base location in base view coordinates.

    egui_alpha_t alpha; // global alpha value for all drawing operations

    egui_mask_t *mask; // current mask for alpha blending

    uint16_t res_circle_info_count_spec;
    const egui_circle_info_t *res_circle_info_spec_arr;
};

extern egui_canvas_t canvas_data;


__EGUI_STATIC_INLINE__ void egui_canvas_set_alpha(egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    self->alpha = alpha;
}

__EGUI_STATIC_INLINE__ egui_alpha_t egui_canvas_get_alpha(void)
{
    egui_canvas_t *self = &canvas_data;

    return self->alpha;
}

__EGUI_STATIC_INLINE__ void egui_canvas_mix_alpha(egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    self->alpha = egui_color_alpha_mix(self->alpha, alpha);
}

__EGUI_STATIC_INLINE__ void egui_canvas_clear_mask(void)
{
    egui_canvas_t *self = &canvas_data;

    self->mask = NULL;
}

__EGUI_STATIC_INLINE__ void egui_canvas_set_mask(egui_mask_t *mask)
{
    egui_canvas_t *self = &canvas_data;

    self->mask = mask;
}

__EGUI_STATIC_INLINE__ egui_mask_t *egui_canvas_get_mask(void)
{
    egui_canvas_t *self = &canvas_data;

    return self->mask;
}

__EGUI_STATIC_INLINE__ egui_region_t *egui_canvas_get_base_view_work_region(void)
{
    egui_canvas_t *self = &canvas_data;

    return &self->base_view_work_region;
}

__EGUI_STATIC_INLINE__ egui_region_t *egui_canvas_get_pfb_region(void)
{
    egui_canvas_t *self = &canvas_data;

    return &self->pfb_region;
}

// For speed, we check if the point is within the base_view_work_region before calling egui_canvas_draw_point_limit.
__EGUI_STATIC_INLINE__ void egui_canvas_draw_point_limit_skip_mask(egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    // mix alpha
    alpha = egui_color_alpha_mix(self->alpha, alpha);
    // if color is fully transparent, do not draw anything.
    if (alpha == 0)
    {
        return;
    }

    // skip pt check.
    // if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
        egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;

        egui_color_t *back_color = (egui_color_t *)&self->pfb[pos_y * self->pfb_region.size.width + pos_x];
#if TEST_CANVAS_TEST_WOKR
        back_color->full = alpha; // For test
#else
        if (alpha == EGUI_ALPHA_100)
        {
            *back_color = color;
        }
        else
        {
            egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
        }
#endif
    }
}

// For speed, we check if the point is within the base_view_work_region before calling egui_canvas_draw_point_limit.
__EGUI_STATIC_INLINE__ void egui_canvas_draw_point_limit(egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    if (self->mask != NULL)
    {
        self->mask->api->mask_point(self->mask, x, y, &color, &alpha);
    }

    // mix alpha
    alpha = egui_color_alpha_mix(self->alpha, alpha);
    // if color is fully transparent, do not draw anything.
    if (alpha == 0)
    {
        return;
    }

    // skip pt check.
    // if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
        egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;

        egui_color_t *back_color = (egui_color_t *)&self->pfb[pos_y * self->pfb_region.size.width + pos_x];
#if TEST_CANVAS_TEST_WOKR
        back_color->full = alpha; // For test
#else
        if (alpha == EGUI_ALPHA_100)
        {
            *back_color = color;
        }
        else
        {
            egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
        }
#endif
    }
}

// In pfb coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_raw(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color)
{
    *(egui_color_t *)&self->pfb[y * self->pfb_region.size.width + x] = color;
}



// In pfb coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_with_alpha_raw(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_color_t *back_color = (egui_color_t *)&self->pfb[y * self->pfb_region.size.width + x];

    egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
}


// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color)
{
    egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
    egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;
    egui_canvas_set_point_color_raw(self, pos_x, pos_y, color);
}



// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_with_alpha(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
    egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;
    egui_canvas_set_point_color_with_alpha_raw(self, pos_x, pos_y, color, alpha);
}


// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_with_mask(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    self->mask->api->mask_point(self->mask, x, y, &color, &alpha);
            
    // if color is fully transparent, do not draw anything.
    if (alpha == 0)
    {
        return;
    }

    egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
    egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;

    egui_color_t *back_color = (egui_color_t *)&self->pfb[pos_y * self->pfb_region.size.width + pos_x];

    if (alpha == EGUI_ALPHA_100)
    {
        egui_canvas_set_point_color_raw(self, pos_x, pos_y, color);
    }
    else
    {
        egui_canvas_set_point_color_with_alpha_raw(self, pos_x, pos_y, color, alpha);
    }
}




// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_check(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color)
{
    if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_canvas_set_point_color(self, x, y, color);
    }
}



// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_with_alpha_check(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_canvas_set_point_color_with_alpha(self, x, y, color, alpha);
    }
}


// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_with_mask_check(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_canvas_set_point_color_with_mask(self, x, y, color, alpha);
    }
}





__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color_with_mask(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t xp, yp;
    egui_dim_t x_total, y_total;

    // for speed, calculate total positions outside of the loop
    x_total = x + width;
    y_total = y + height;

    
    for (yp = y; yp < y_total; yp++)
    {
        for (xp = x; xp < x_total; xp++)
        {
            egui_canvas_set_point_color_with_mask(self, xp, yp, color, alpha);
        }
    }
}



__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color_with_alpha(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t xp, yp;
    egui_dim_t x_total, y_total;

    // for speed, calculate total positions outside of the loop
    x_total = x + width - self->pfb_location_in_base_view.x;
    y_total = y + height - self->pfb_location_in_base_view.y;

    
    for (yp = y - self->pfb_location_in_base_view.y; yp < y_total; yp++)
    {
        for (xp = x - self->pfb_location_in_base_view.x; xp < x_total; xp++)
        {
            egui_canvas_set_point_color_with_alpha_raw(self, xp, yp, color, alpha);
        }
    }
}



__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t xp, yp;
    egui_dim_t x_total, y_total;

    // for speed, calculate total positions outside of the loop
    x_total = x + width - self->pfb_location_in_base_view.x;
    y_total = y + height - self->pfb_location_in_base_view.y;

    
    for (yp = y - self->pfb_location_in_base_view.y; yp < y_total; yp++)
    {
        for (xp = x - self->pfb_location_in_base_view.x; xp < x_total; xp++)
        {
            egui_canvas_set_point_color_raw(self, xp, yp, color);
        }
    }
}








__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color_with_mask_intersect(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, &self->base_view_work_region, &region);
    if(egui_region_is_empty(&region))
    {
        return;
    }

    egui_canvas_set_rect_color_with_mask(region.location.x, region.location.y, region.size.width, region.size.height, color, alpha);
}




__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color_with_alpha_intersect(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, &self->base_view_work_region, &region);
    if(egui_region_is_empty(&region))
    {
        return;
    }

    egui_canvas_set_rect_color_with_alpha(region.location.x, region.location.y, region.size.width, region.size.height, color, alpha);
}



__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color_intersect(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color)
{
    egui_canvas_t *self = &canvas_data;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, &self->base_view_work_region, &region);
    if(egui_region_is_empty(&region))
    {
        return;
    }
    egui_canvas_set_rect_color(region.location.x, region.location.y, region.size.width, region.size.height, color);
}






__EGUI_STATIC_INLINE__ egui_canvas_t* egui_canvas_get_canvas(void)
{
    return &canvas_data;
}

void egui_canvas_set_alpha(egui_alpha_t alpha);
egui_alpha_t egui_canvas_get_alpha(void);
void egui_canvas_mix_alpha(egui_alpha_t alpha);

// void egui_canvas_draw_point_limit(egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_point(egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha);
// void egui_canvas_draw_fillrect(egui_dim_t x, egui_dim_t y, egui_dim_t xSize, egui_dim_t ySize, egui_color_t color, egui_alpha_t alpha);
// static inline void egui_canvas_draw_vline(egui_dim_t x, egui_dim_t y, egui_dim_t length, egui_color_t color, egui_alpha_t alpha);
// static inline void egui_canvas_draw_hline(egui_dim_t x, egui_dim_t y, egui_dim_t length, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_line(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color,
                           egui_alpha_t alpha);
void egui_canvas_draw_rectangle(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t stroke_width,
                                egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_rectangle_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                     egui_alpha_t alpha);
void egui_canvas_draw_round_rectangle_corners_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height,
                                                   egui_dim_t radius_left_top, egui_dim_t radius_left_bottom, egui_dim_t radius_right_top,
                                                   egui_dim_t radius_right_bottom, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_round_rectangle_corners(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius_left_top,
                                              egui_dim_t radius_left_bottom, egui_dim_t radius_right_top, egui_dim_t radius_right_bottom,
                                              egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_round_rectangle(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius,
                                      egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_round_rectangle_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t r,
                                           egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_circle(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t stroke_width, egui_color_t color,
                             egui_alpha_t alpha);
void egui_canvas_draw_circle_fill(egui_dim_t x, egui_dim_t y, egui_dim_t r, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_arc(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle,
                          egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_arc_fill(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle,
                               egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_triangle(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t x3, egui_dim_t y3,
                               egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_triangle_fill(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t x3, egui_dim_t y3,
                                    egui_color_t color, egui_alpha_t alpha);
// void egui_canvas_draw_circle_corner(egui_dim_t x0, egui_dim_t y0, egui_dim_t r, uint8_t c, egui_color_t color, egui_alpha_t alpha);
// void egui_canvas_draw_circle_corner_fill(egui_dim_t x0, egui_dim_t y0, egui_dim_t r, uint8_t c, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_text(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_text_in_rect(const egui_font_t *font, const void *string, egui_region_t *rect, uint8_t align_type, egui_color_t color,
                                   egui_alpha_t alpha);
void egui_canvas_draw_image(const egui_image_t *img, egui_dim_t x, egui_dim_t y);
void egui_canvas_draw_image_resize(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height);
void egui_canvas_calc_work_region(egui_region_t *base_region);
void egui_canvas_init(egui_color_int_t *pfb, egui_region_t *region);

int egui_canvas_get_circle_left_top(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y,
                                    egui_alpha_t *alpha);
int egui_canvas_get_circle_left_bottom(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y,
                                       egui_alpha_t *alpha);
int egui_canvas_get_circle_right_top(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y,
                                     egui_alpha_t *alpha);
int egui_canvas_get_circle_right_bottom(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y,
                                        egui_alpha_t *alpha);

void egui_canvas_clear_mask(void);
void egui_canvas_set_mask(egui_mask_t *mask);
egui_mask_t *egui_canvas_get_mask(void);

egui_region_t *egui_canvas_get_base_view_work_region(void);
egui_region_t *egui_canvas_get_pfb_region(void);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CANVAS_H_ */
