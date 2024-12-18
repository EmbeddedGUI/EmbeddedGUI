#include <stdio.h>
#include <assert.h>

#include "egui_image_std.h"
#include "core/egui_api.h"


const uint8_t egui_image_data_type_size_table[] = {
        4, /* EGUI_IMAGE_DATA_TYPE_RGB32 */
        2, /* EGUI_IMAGE_DATA_TYPE_RGB565 */
        1, /* EGUI_IMAGE_DATA_TYPE_GRAY8 */
};

const uint8_t egui_image_alpha_type_size_table[] = {
        1, /* EGUI_IMAGE_ALPHA_TYPE_1 */
        2, /* EGUI_IMAGE_ALPHA_TYPE_2 */
        4, /* EGUI_IMAGE_ALPHA_TYPE_4 */
        8, /* EGUI_IMAGE_ALPHA_TYPE_8 */
};

typedef void(egui_image_std_get_pixel)(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb32(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    uint32_t sel_color = ((uint32_t *)image->data_buf)[x + y * image->width];
    color->full = EGUI_COLOR_RGB888_TRANS(sel_color);
    *alpha = (sel_color >> 24) & 0xFF;
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_limit(const uint16_t *p_data, egui_dim_t col_index, egui_color_t *color)
{
    uint32_t sel_pos = col_index;
    uint16_t sel_color = p_data[sel_pos];

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_limit(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color)
{
    uint32_t row_start = y * image->width;
    const uint16_t *p_data = image->data_buf;

    egui_image_std_get_col_pixel_rgb565_limit(&p_data[row_start], x, color);
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_image_std_get_pixel_rgb565_limit(image, x, y, color);
    *alpha = EGUI_ALPHA_100;
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_image_std_get_col_pixel_rgb565_limit(p_data, col_index, color);
    *alpha = EGUI_ALPHA_100;
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_8(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color, egui_alpha_t *alpha)
{
    uint32_t sel_pos = col_index;
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha = p_alpha[sel_pos];

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = sel_alpha;
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_8(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_8(&p_data[row_start], &p_alpha[row_start], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_4(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color, egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index;
    // get alpha row position.
    uint32_t sel_alpha_pos = col_index >> 1; // same to: x / 2
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha;

    bit_pos = col_index & 0x01;     // 0x01
    bit_pos = bit_pos << 2; // same to: bit_pos * 4

    sel_alpha = ((p_alpha[sel_alpha_pos]) >> bit_pos) & 0x0F;

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = egui_alpha_change_table_4[sel_alpha];
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_4(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    uint32_t row_start_alpha = y * ((image->width + 1) >> 1); // same to: ((image->width + 1) / 2);
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_4(&p_data[row_start], &p_alpha[row_start_alpha], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_2(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color, egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index;
    // get alpha row position.
    uint32_t sel_alpha_pos = col_index >> 2; // same to: x / 4
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha;

    bit_pos = col_index & 0x03;     // 0x03
    bit_pos = bit_pos << 1; // same to: bit_pos * 2

    sel_alpha = ((p_alpha[sel_alpha_pos]) >> bit_pos) & 0x03;

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = egui_alpha_change_table_2[sel_alpha];
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_2(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    uint32_t row_start_alpha = y * ((image->width + 3) >> 2); // same to: ((image->width + 3) / 4);
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_2(&p_data[row_start], &p_alpha[row_start_alpha], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_1(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color, egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index;
    // get alpha row position.
    uint32_t sel_alpha_pos = col_index >> 3; // same to x / 8
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha;

    bit_pos = col_index & 0x07; // 0x07

    sel_alpha = ((p_alpha[sel_alpha_pos]) >> bit_pos) & 0x01;

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = (sel_alpha ? 0xff : 0x00);
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_1(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    uint32_t row_start_alpha = y * ((image->width + 7) >> 3); // same to ((image->width + 7) / 8);
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_1(&p_data[row_start], &p_alpha[row_start_alpha], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1

egui_image_std_get_pixel *egui_image_get_point_func(const egui_image_t *self)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = NULL;
    if (image)
    {
        switch (image->data_type)
        {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB32:
            get_pixel = egui_image_std_get_pixel_rgb32;
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB565:
            if (image->alpha_buf == NULL)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
                get_pixel = egui_image_std_get_pixel_rgb565;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
            }
            else
            {
                switch (image->alpha_type)
                {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
                case EGUI_IMAGE_ALPHA_TYPE_1:
                    get_pixel = egui_image_std_get_pixel_rgb565_1;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
                case EGUI_IMAGE_ALPHA_TYPE_2:
                    get_pixel = egui_image_std_get_pixel_rgb565_2;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
                case EGUI_IMAGE_ALPHA_TYPE_4:
                    get_pixel = egui_image_std_get_pixel_rgb565_4;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                case EGUI_IMAGE_ALPHA_TYPE_8:
                    get_pixel = egui_image_std_get_pixel_rgb565_8;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                default:
                    EGUI_ASSERT(0);
                    break;
                }
            }
            break;
        default:
            EGUI_ASSERT(0);
            break;
        }
    }

    return get_pixel;
}

int egui_image_std_get_point(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    if ((x >= image->width) || (y >= image->height))
    {
        return 0;
    }

    get_pixel(image, x, y, color, alpha);

    return 1;
}

int egui_image_std_get_point_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                    egui_alpha_t *alpha)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    if(width == 0 || height == 0)
    {
        return 0;
    }
    egui_float_t width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(width));
    egui_float_t height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(height));
    egui_dim_t src_x;
    egui_dim_t src_y;

    if ((x >= width) || (y >= height))
    {
        return 0;
    }

    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    // For speed, use nearestScaler to scale the image.
    src_x = (egui_dim_t)EGUI_FLOAT_MULT(x, width_radio);
    src_y = (egui_dim_t)EGUI_FLOAT_MULT(y, height_radio);

    get_pixel(image, src_x, src_y, color, alpha);

    return 1;
}

void egui_image_std_load_data_resource(void *dest, egui_image_std_info_t *image, uint32_t start_offset, uint32_t size)
{
    // EGUI_LOG_INF("egui_image_std_load_data_resource, start_offset: %d, size: %d\n", start_offset, size);
    if(image->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        egui_memcpy(dest, (const void *)((const uint8_t *)image->data_buf + start_offset), size);
    }
    else
    {
        egui_api_load_external_resource(dest, (uint32_t)(image->data_buf), start_offset, size);
    }
    
    // EGUI_LOG_INF("egui_image_std_load_data_resource, data: %08x:%08x:%08x:%08x:%08x:%08x:%08x:%08x\n", ((uint32_t *)dest)[0], ((uint32_t *)dest)[1], ((uint32_t *)dest)[2], ((uint32_t *)dest)[3], ((uint32_t *)dest)[4], ((uint32_t *)dest)[5], ((uint32_t *)dest)[6], ((uint32_t *)dest)[7]);
}

void egui_image_std_load_alpha_resource(void *dest, egui_image_std_info_t *image, uint32_t start_offset, uint32_t size)
{
    // EGUI_LOG_INF("egui_image_std_load_alpha_resource, start_offset: %d, size: %d\n", start_offset, size);
    if(image->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        egui_memcpy(dest, (const void *)((const uint8_t *)image->alpha_buf + start_offset), size);
    }
    else
    {
        egui_api_load_external_resource(dest, (uint32_t)(image->alpha_buf), start_offset, size);
    }
    // EGUI_LOG_INF("egui_image_std_load_alpha_resource, data: %08x:%08x:%08x:%08x:%08x:%08x:%08x:%08x\n", ((uint32_t *)dest)[0], ((uint32_t *)dest)[1], ((uint32_t *)dest)[2], ((uint32_t *)dest)[3], ((uint32_t *)dest)[4], ((uint32_t *)dest)[5], ((uint32_t *)dest)[6], ((uint32_t *)dest)[7]);
}


#if EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE
void egui_image_std_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_color_t color;
    egui_alpha_t alpha;
    egui_dim_t width = image->width;
    egui_dim_t height = image->height;
    const uint32_t *p_data = image->data_buf;
    // const uint8_t* p_alpha = image->alpha_buf;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);
    if(egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

    if(canvas->mask != NULL)
    {
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
        {
            for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
            {
                get_pixel(image, x_, y_, &color, &alpha);

                // change to real position in canvas.
                egui_canvas_draw_point_limit((x + x_), (y + y_), color, alpha);
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
        {
            for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
            {
                get_pixel(image, x_, y_, &color, &alpha);

                // change to real position in canvas.
                egui_canvas_draw_point_limit_skip_mask((x + x_), (y + y_), color, alpha);
            }
        }
    }
}


void egui_image_std_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_color_t color;
    egui_alpha_t alpha;
    egui_float_t width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(width));
    egui_float_t height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(height));
    const uint32_t *p_data = image->data_buf;
    // const uint8_t* p_alpha = image->alpha_buf;
    egui_dim_t src_x;
    egui_dim_t src_y;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);
    if(egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

    if(canvas->mask != NULL)
    {
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
        {
            for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
            {
                // For speed, use nearestScaler to scale the image.
                src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);

                get_pixel(image, src_x, src_y, &color, &alpha);

                // change to real position in canvas.
                egui_canvas_draw_point_limit((x + x_), (y + y_), color, alpha);
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
        {
            for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
            {
                // For speed, use nearestScaler to scale the image.
                src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);

                get_pixel(image, src_x, src_y, &color, &alpha);

                // change to real position in canvas.
                egui_canvas_draw_point_limit_skip_mask((x + x_), (y + y_), color, alpha);
            }
        }
    }
}

#else // EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE

#define EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(_get_pixel_func, self, x, y, x_total, y_total, x_base, y_base) \
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;\
    egui_color_t color;\
    egui_alpha_t alpha;\
    egui_canvas_t *canvas = egui_canvas_get_canvas();\
    if(canvas->mask != NULL)\
        {\
            for (egui_dim_t y_ = y; y_ < y_total; y_++)\
            {\
                for (egui_dim_t x_ = x; x_ < x_total; x_++)\
                {\
                    _get_pixel_func(image, x_, y_, &color, &alpha);\
\
                    /* change to real position in canvas. */ \
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);\
                }\
            }\
        }\
        else\
        {\
            for (egui_dim_t y_ = y; y_ < y_total; y_++)\
            {\
                for (egui_dim_t x_ = x; x_ < x_total; x_++)\
                {\
                    _get_pixel_func(image, x_, y_, &color, &alpha);\
\
                    /* change to real position in canvas. */ \
                    egui_canvas_draw_point_limit_skip_mask((x_base + x_), (y_base + y_), color, alpha);\
                }\
            }\
        }\

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
void egui_image_std_set_image_rgb565_8(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_8, self, x, y, x_total, y_total, x_base, y_base);

    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1; // same to image->width * 2
    uint16_t alpha_row_size = image->width;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    void *data_buf = egui_malloc(data_row_size); 
    void *alpha_buf = egui_malloc(alpha_row_size);
    if(data_buf == NULL || alpha_buf == NULL)
    {
        EGUI_ASSERT(0);
        return;
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        uint32_t row_start = y_ * image->width;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        const void* p_data = data_buf;
        const void* p_alpha = alpha_buf;
        uint32_t start_pos = x;
        uint32_t end_pos = x_total;

        egui_image_std_load_data_resource((uint8_t *)p_data + (start_pos << 1), image, (row_start + start_pos) << 1, (end_pos - start_pos) << 1);
        egui_image_std_load_alpha_resource((uint8_t *)p_alpha + (start_pos), image, (row_start + start_pos), (end_pos - start_pos));
#else
        const void *p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
        const void *p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start));
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if(canvas->mask != NULL)
        {
            for (egui_dim_t x_ = x; x_ < x_total; x_++)
            {
                egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_, &color, &alpha);

                /* change to real position in canvas. */ 
                egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
            }
        }
        else
        {
            for (egui_dim_t x_ = x; x_ < x_total; x_++)
            {
                egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_, &color, &alpha);

                /* change to real position in canvas. */ 
                egui_canvas_draw_point_limit_skip_mask((x_base + x_), (y_base + y_), color, alpha);
            }
        }
    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_free(data_buf); 
    egui_free(alpha_buf);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
void egui_image_std_set_image_rgb565_4(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_4, self, x, y, x_total, y_total, x_base, y_base);
    
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1; // same to image->width * 2
    uint16_t alpha_row_size = ((image->width + 1) >> 1); // same to: ((image->width + 1) / 2);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    void *data_buf = egui_malloc(data_row_size); 
    void *alpha_buf = egui_malloc(alpha_row_size);
    if(data_buf == NULL || alpha_buf == NULL)
    {
        EGUI_ASSERT(0);
        return;
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        uint32_t row_start = y_ * image->width;
        uint32_t row_start_alpha = y_ * alpha_row_size;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        const void* p_data = data_buf;
        const void* p_alpha = alpha_buf;
        uint32_t start_pos = x;
        uint32_t start_pos_alpha = x >> 1; // same to: x / 2
        uint32_t end_pos = x_total;
        uint32_t end_pos_alpha = (x_total + 1) >> 1; // same to: x / 2

        egui_image_std_load_data_resource((uint8_t *)p_data + (start_pos << 1), image, (row_start + start_pos) << 1, (end_pos - start_pos) << 1);
        egui_image_std_load_alpha_resource((uint8_t *)p_alpha + (start_pos_alpha), image, (row_start_alpha + start_pos_alpha), (end_pos_alpha - start_pos_alpha));
#else
        const void *p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
        const void *p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start_alpha));
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            
        if(canvas->mask != NULL)
        {
            for (egui_dim_t x_ = x; x_ < x_total; x_++)
            {
                egui_image_std_get_col_pixel_rgb565_4(p_data, p_alpha, x_, &color, &alpha);

                /* change to real position in canvas. */ 
                egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
            }
        }
        else
        {
            for (egui_dim_t x_ = x; x_ < x_total; x_++)
            {
                egui_image_std_get_col_pixel_rgb565_4(p_data, p_alpha, x_, &color, &alpha);

                /* change to real position in canvas. */ 
                egui_canvas_draw_point_limit_skip_mask((x_base + x_), (y_base + y_), color, alpha);
            }
        }
    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_free(data_buf); 
    egui_free(alpha_buf);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
void egui_image_std_set_image_rgb565_2(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_2, self, x, y, x_total, y_total, x_base, y_base);
    
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1; // same to image->width * 2
    uint16_t alpha_row_size = ((image->width + 3) >> 2); // same to: ((image->width + 3) / 4);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    void *data_buf = egui_malloc(data_row_size); 
    void *alpha_buf = egui_malloc(alpha_row_size);
    if(data_buf == NULL || alpha_buf == NULL)
    {
        EGUI_ASSERT(0);
        return;
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        uint32_t row_start = y_ * image->width;
        uint32_t row_start_alpha = y_ * alpha_row_size;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        const void* p_data = data_buf;
        const void* p_alpha = alpha_buf;
        uint32_t start_pos = x;
        uint32_t start_pos_alpha = x >> 2; // same to: x / 4
        uint32_t end_pos = x_total;
        uint32_t end_pos_alpha = ((x_total + 3) >> 2); // same to: x / 4

        egui_image_std_load_data_resource((uint8_t *)p_data + (start_pos << 1), image, (row_start + start_pos) << 1, (end_pos - start_pos) << 1);
        egui_image_std_load_alpha_resource((uint8_t *)p_alpha + (start_pos_alpha), image, (row_start_alpha + start_pos_alpha), (end_pos_alpha - start_pos_alpha));
#else
        const void *p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
        const void *p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start_alpha));
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            
        if(canvas->mask != NULL)
        {
            for (egui_dim_t x_ = x; x_ < x_total; x_++)
            {
                egui_image_std_get_col_pixel_rgb565_2(p_data, p_alpha, x_, &color, &alpha);

                /* change to real position in canvas. */ 
                egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
            }
        }
        else
        {
            for (egui_dim_t x_ = x; x_ < x_total; x_++)
            {
                egui_image_std_get_col_pixel_rgb565_2(p_data, p_alpha, x_, &color, &alpha);

                /* change to real position in canvas. */ 
                egui_canvas_draw_point_limit_skip_mask((x_base + x_), (y_base + y_), color, alpha);
            }
        }
    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_free(data_buf); 
    egui_free(alpha_buf);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
void egui_image_std_set_image_rgb565_1(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_1, self, x, y, x_total, y_total, x_base, y_base);

    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1; // same to image->width * 2
    uint16_t alpha_row_size = ((image->width + 7) >> 3); // same to ((image->width + 7) / 8);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    void *data_buf = egui_malloc(data_row_size); 
    void *alpha_buf = egui_malloc(alpha_row_size);
    if(data_buf == NULL || alpha_buf == NULL)
    {
        EGUI_ASSERT(0);
        return;
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        uint32_t row_start = y_ * image->width;
        uint32_t row_start_alpha = y_ * alpha_row_size;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        const void* p_data = data_buf;
        const void* p_alpha = alpha_buf;
        uint32_t start_pos = x;
        uint32_t start_pos_alpha = x  >> 3; // same to x / 8
        uint32_t end_pos = x_total;
        uint32_t end_pos_alpha = ((x_total + 7)  >> 3); // same to x / 8

        egui_image_std_load_data_resource((uint8_t *)p_data + (start_pos << 1), image, (row_start + start_pos) << 1, (end_pos - start_pos) << 1);
        egui_image_std_load_alpha_resource((uint8_t *)p_alpha + (start_pos_alpha), image, (row_start_alpha + start_pos_alpha), (end_pos_alpha - start_pos_alpha));
#else
        const void *p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
        const void *p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start_alpha));
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            
        if(canvas->mask != NULL)
        {
            for (egui_dim_t x_ = x; x_ < x_total; x_++)
            {
                egui_image_std_get_col_pixel_rgb565_1(p_data, p_alpha, x_, &color, &alpha);

                /* change to real position in canvas. */ 
                egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
            }
        }
        else
        {
            for (egui_dim_t x_ = x; x_ < x_total; x_++)
            {
                egui_image_std_get_col_pixel_rgb565_1(p_data, p_alpha, x_, &color, &alpha);

                /* change to real position in canvas. */ 
                egui_canvas_draw_point_limit_skip_mask((x_base + x_), (y_base + y_), color, alpha);
            }
        }
    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_free(data_buf); 
    egui_free(alpha_buf);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1


#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
void egui_image_std_set_image_rgb565(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base)
{
    if((egui_canvas_get_canvas()->alpha == EGUI_ALPHA_100) && (egui_canvas_get_canvas()->mask == NULL))
    {
        egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        void *data_buf = egui_malloc(image->width << 1);
        if(data_buf == NULL)
        {
            EGUI_ASSERT(0);
            return;
        }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_color_t color;
        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            uint32_t row_start = y_ * image->width;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            const void* p_data = data_buf;
            uint32_t start_pos = x;
            uint32_t end_pos = x_total;
            egui_image_std_load_data_resource((uint8_t *)p_data + (start_pos << 1), image, (row_start + start_pos) << 1, (end_pos - start_pos) << 1);
#else
            const void *p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

            for (egui_dim_t x_ = x; x_ < x_total; x_++)
            {
                egui_image_std_get_col_pixel_rgb565_limit(p_data, x_, &color);

                // change to real position in canvas.
                egui_canvas_set_point_color(egui_canvas_get_canvas(), (x_base + x_), (y_base + y_), color);
            }
        }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_free(data_buf);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    }
    else
    {
        EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565, self, x, y, x_total, y_total, x_base, y_base);
    }
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
void egui_image_std_set_image_rgb32(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base)
{
    EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb32, self, x, y, x_total, y_total, x_base, y_base);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32

void egui_image_std_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_dim_t width = image->width;
    egui_dim_t height = image->height;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);

    if(egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

    if (image)
    {
        switch (image->data_type)
        {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB32:
            egui_image_std_set_image_rgb32(self, region.location.x, region.location.y, x_total, y_total, x, y);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB565:
            if (image->alpha_buf == NULL)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
                egui_image_std_set_image_rgb565(self, region.location.x, region.location.y, x_total, y_total, x, y);
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
            }
            else
            {
                switch (image->alpha_type)
                {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
                case EGUI_IMAGE_ALPHA_TYPE_1:
                    egui_image_std_set_image_rgb565_1(self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
                case EGUI_IMAGE_ALPHA_TYPE_2:
                    egui_image_std_set_image_rgb565_2(self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
                case EGUI_IMAGE_ALPHA_TYPE_4:
                    egui_image_std_set_image_rgb565_4(self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                case EGUI_IMAGE_ALPHA_TYPE_8:
                    egui_image_std_set_image_rgb565_8(self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                default:
                    EGUI_ASSERT(0);
                    break;
                }
            }
            break;
        default:
            EGUI_ASSERT(0);
            break;
        }
    }
}




#define EGUI_IMAGE_STD_DRAW_IMAGE_RESIZE_FUNC_DEFINE(_get_pixel_func, self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio) \
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;\
    egui_color_t color;\
    egui_alpha_t alpha;\
    egui_dim_t src_x;\
    egui_dim_t src_y;\
    egui_canvas_t *canvas = egui_canvas_get_canvas();\
    if(canvas->mask != NULL)\
        {\
            for (egui_dim_t y_ = y; y_ < y_total; y_++)\
            {\
                for (egui_dim_t x_ = x; x_ < x_total; x_++)\
                {\
                    /* For speed, use nearestScaler to scale the image. */ \
                    src_x = (egui_dim_t)EGUI_FLOAT_MULT(x_, width_radio);\
                    src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);\
                    _get_pixel_func(image, src_x, src_y, &color, &alpha);\
\
                    /* change to real position in canvas. */ \
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);\
                }\
            }\
        }\
        else\
        {\
            for (egui_dim_t y_ = y; y_ < y_total; y_++)\
            {\
                for (egui_dim_t x_ = x; x_ < x_total; x_++)\
                {\
                    /* For speed, use nearestScaler to scale the image. */ \
                    src_x = (egui_dim_t)EGUI_FLOAT_MULT(x_, width_radio);\
                    src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);\
                    _get_pixel_func(image, src_x, src_y, &color, &alpha);\
\
                    /* change to real position in canvas. */ \
                    egui_canvas_draw_point_limit_skip_mask((x_base + x_), (y_base + y_), color, alpha);\
                }\
            }\
        }\

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
void egui_image_std_set_image_resize_rgb565_8(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base
    , egui_float_t width_radio, egui_float_t height_radio)
{
    EGUI_IMAGE_STD_DRAW_IMAGE_RESIZE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_8, self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
void egui_image_std_set_image_resize_rgb565_4(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base
    , egui_float_t width_radio, egui_float_t height_radio)
{
    EGUI_IMAGE_STD_DRAW_IMAGE_RESIZE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_4, self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
void egui_image_std_set_image_resize_rgb565_2(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base
    , egui_float_t width_radio, egui_float_t height_radio)
{
    EGUI_IMAGE_STD_DRAW_IMAGE_RESIZE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_2, self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
void egui_image_std_set_image_resize_rgb565_1(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base
    , egui_float_t width_radio, egui_float_t height_radio)
{
    EGUI_IMAGE_STD_DRAW_IMAGE_RESIZE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_1, self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
void egui_image_std_set_image_resize_rgb565(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base
    , egui_float_t width_radio, egui_float_t height_radio)
{
    if((egui_canvas_get_canvas()->alpha == EGUI_ALPHA_100) && (egui_canvas_get_canvas()->mask == NULL))
    {
        egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
        egui_color_t color;
        egui_dim_t src_x;
        egui_dim_t src_y;
        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            for (egui_dim_t x_ = x; x_ < x_total; x_++)
            {
                // For speed, use nearestScaler to scale the image.
                src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);

                egui_image_std_get_pixel_rgb565_limit(image, src_x, src_y, &color);

                // change to real position in canvas.
                egui_canvas_set_point_color(egui_canvas_get_canvas(), (x_base + x_), (y_base + y_), color);
            }
        }
    }
    else
    {
        EGUI_IMAGE_STD_DRAW_IMAGE_RESIZE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565, self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
    }
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
void egui_image_std_set_image_resize_rgb32(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base
    , egui_float_t width_radio, egui_float_t height_radio)
{
    EGUI_IMAGE_STD_DRAW_IMAGE_RESIZE_FUNC_DEFINE(egui_image_std_get_pixel_rgb32, self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32


void egui_image_std_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    if(width == 0 || height == 0)
    {
        return;
    }
    egui_color_t color;
    egui_alpha_t alpha;
    egui_float_t width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(width));
    egui_float_t height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(height));
    const uint32_t *p_data = image->data_buf;
    // const uint8_t* p_alpha = image->alpha_buf;
    egui_dim_t src_x;
    egui_dim_t src_y;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);

    if(egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;
    
    if (image)
    {
        switch (image->data_type)
        {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB32:
            egui_image_std_set_image_resize_rgb32(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB565:
            if (image->alpha_buf == NULL)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
                egui_image_std_set_image_resize_rgb565(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
            }
            else
            {
                switch (image->alpha_type)
                {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
                case EGUI_IMAGE_ALPHA_TYPE_1:
                    egui_image_std_set_image_resize_rgb565_1(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
                case EGUI_IMAGE_ALPHA_TYPE_2:
                    egui_image_std_set_image_resize_rgb565_2(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
                case EGUI_IMAGE_ALPHA_TYPE_4:
                    egui_image_std_set_image_resize_rgb565_4(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                case EGUI_IMAGE_ALPHA_TYPE_8:
                    egui_image_std_set_image_resize_rgb565_8(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                default:
                    EGUI_ASSERT(0);
                    break;
                }
            }
            break;
        default:
            EGUI_ASSERT(0);
            break;
        }
    }
}

#endif // EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE



void egui_image_std_get_width_height(egui_image_t *self, egui_dim_t *width, egui_dim_t *height)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    *width = image->width;
    *height = image->height;
}

const egui_image_api_t egui_image_std_t_api_table = {.get_point = egui_image_std_get_point,
                                                     .get_point_resize = egui_image_std_get_point_resize,
                                                     .draw_image = egui_image_std_draw_image,
                                                     .draw_image_resize = egui_image_std_draw_image_resize};

void egui_image_std_init(egui_image_t *self, const void *res)
{
    egui_image_std_t *local = (egui_image_std_t *)self;
    // call super init.
    egui_image_init(self, res);

    // update api.
    self->api = &egui_image_std_t_api_table;
}
