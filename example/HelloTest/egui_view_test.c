#include <stdio.h>
#include <assert.h>

#include "egui_view_test.h"
#include "egui.h"

void egui_view_test_on_draw(egui_view_t *self)
{
    egui_view_test_t *view = (egui_view_test_t *)self;

    view->last_pos += 1;

    if (view->last_pos > self->region.size.width)
    {
        view->last_pos = 0;
    }

    int base_offset = 1;
    // egui_canvas_draw_point(base_offset, base_offset, EGUI_COLOR_WHITE);
    // egui_canvas_draw_point(base_offset + 1, base_offset + 1, EGUI_COLOR_WHITE);
    // egui_canvas_draw_point(base_offset, base_offset + 1, EGUI_COLOR_WHITE);
    // egui_canvas_draw_point(base_offset + 1, base_offset, EGUI_COLOR_WHITE);

    egui_canvas_draw_rectangle_fill(base_offset, base_offset, 100, 100, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_draw_rectangle_fill(base_offset + 50, base_offset + 50, 100, 100, EGUI_COLOR_WHITE, EGUI_ALPHA_50);

    // egui_canvas_draw_line(base_offset, base_offset, base_offset + 100, base_offset + 100, EGUI_COLOR_WHITE);

    // egui_canvas_draw_hline(base_offset, base_offset, 10, EGUI_COLOR_WHITE);
    // egui_canvas_draw_vline(base_offset, base_offset, 10, EGUI_COLOR_WHITE);

    // egui_canvas_draw_rect(base_offset, base_offset, 100, 100, EGUI_COLOR_WHITE);

    // egui_canvas_draw_rectangle(base_offset, base_offset, 100, 100, EGUI_COLOR_WHITE);

    // egui_canvas_draw_rectangle_fill(base_offset, base_offset, 100, 100, EGUI_COLOR_WHITE);

    // egui_canvas_draw_round_rectangle(base_offset, base_offset, 100, 100, 10, EGUI_COLOR_WHITE);

    // egui_canvas_draw_round_rectangle_fill(base_offset, base_offset, 100, 100, 10, EGUI_COLOR_WHITE);

    // egui_canvas_draw_circle(base_offset + 50, base_offset + 50, 50, EGUI_COLOR_WHITE);

    // egui_canvas_draw_circle_fill(base_offset + 50, base_offset + 50, 50, EGUI_COLOR_WHITE);

    // egui_canvas_draw_triangle(base_offset, base_offset, base_offset + 100, base_offset + 100, base_offset + 50, base_offset, EGUI_COLOR_WHITE);

    // egui_canvas_draw_triangle_fill(base_offset, base_offset, base_offset + 100, base_offset + 100, base_offset + 50, base_offset, EGUI_COLOR_WHITE);

    // egui_canvas_draw_circle_corner(base_offset + 50, base_offset + 50, 50, EGUI_DRAW_CIRCLE_ALL, EGUI_COLOR_WHITE);

    // egui_canvas_draw_circle_corner_fill(base_offset + 50, base_offset + 50, 50, EGUI_DRAW_CIRCLE_ALL, EGUI_COLOR_WHITE);

#if 0
    // image
    extern const egui_image_std_t egui_res_image_star_rgb32_8;
    extern const egui_image_std_t egui_res_image_star_rgb565_1;
    extern const egui_image_std_t egui_res_image_star_rgb565_2;
    extern const egui_image_std_t egui_res_image_star_rgb565_4;
    extern const egui_image_std_t egui_res_image_star_rgb565_8;
    // egui_canvas_draw_image((egui_image_t *)&egui_res_image_star_rgb565_4, base_offset, base_offset);
    
    egui_mask_circle_t mask;
    egui_mask_circle_init((egui_mask_t *)&mask);
    egui_mask_set_position((egui_mask_t *)&mask, base_offset+20, base_offset+20);
    egui_mask_set_size((egui_mask_t *)&mask, 30, 30);
    egui_mask_circle_param_init_solid(&mask, EGUI_COLOR_WHITE);
    egui_mask_circle_param_init_circle(&mask, EGUI_COLOR_WHITE, 10);

    canvas->mask = (egui_mask_t *)&mask;

    egui_canvas_draw_image_resize((egui_image_t *)&egui_res_image_star_rgb32_8, base_offset, base_offset, 50, 50);
#endif

#if 1
    extern const egui_image_std_t egui_res_image_test_rgb565_8;
    // extern const egui_image_std_t egui_res_image_test_rgb32_8;

    egui_mask_circle_t mask;
    egui_mask_circle_init((egui_mask_t *)&mask);
    egui_mask_set_position((egui_mask_t *)&mask, base_offset + 5, base_offset + 5);
    egui_mask_set_size((egui_mask_t *)&mask, 50, 50);

    // set mask for canvas.
    egui_canvas_set_mask((egui_mask_t *)&mask);
    egui_canvas_draw_image((egui_image_t *)&egui_res_image_test_rgb565_8, base_offset, base_offset);
    // clear mask for canvas.
    egui_canvas_clear_mask();
    // egui_canvas_draw_image_resize((egui_image_t *)&egui_res_image_star_rgb32_8, base_offset, base_offset, 50, 50);
#endif

#if 0
    extern const egui_image_std_t egui_res_image_test_rgb565_8;
    extern const egui_image_std_t egui_res_image_test_rgb32_8;


    egui_mask_round_rectangle_t mask;
    egui_mask_round_rectangle_init((egui_mask_t *)&mask);
    egui_mask_round_rectangle_set_radius((egui_mask_t *)&mask, 10);
    egui_mask_set_position((egui_mask_t *)&mask, base_offset+1, base_offset+1);
    egui_mask_set_size((egui_mask_t *)&mask, 50, 50);

    // set mask for canvas.
    egui_canvas_set_mask((egui_mask_t *)&mask);
    egui_canvas_draw_image((egui_image_t *)&egui_res_image_test_rgb565_8, base_offset, base_offset);
    // clear mask for canvas.
    egui_canvas_clear_mask(canvas);
    // egui_canvas_draw_image_resize((egui_image_t *)&egui_res_image_star_rgb32_8, base_offset, base_offset, 50, 50);
#endif

#if 0
    extern const egui_image_std_t egui_res_image_star_rgb32_8;
    extern const egui_image_std_t egui_res_image_star_rgb565_1;
    extern const egui_image_std_t egui_res_image_star_rgb565_2;
    extern const egui_image_std_t egui_res_image_star_rgb565_4;
    extern const egui_image_std_t egui_res_image_star_rgb565_8;

    extern const egui_image_std_t egui_res_image_test_rgb565_8;
    extern const egui_image_std_t egui_res_image_test_rgb32_8;

    egui_mask_image_t mask;
    egui_mask_image_init((egui_mask_t *)&mask);
    egui_mask_set_position((egui_mask_t *)&mask, base_offset + 10, base_offset + 10);
    egui_mask_set_size((egui_mask_t *)&mask, 50, 50);
    mask.img = (egui_image_t *)&egui_res_image_star_rgb32_8;

    // set mask for canvas.
    egui_canvas_set_mask((egui_mask_t *)&mask);
    egui_canvas_draw_image((egui_image_t *)&egui_res_image_test_rgb565_8, base_offset, base_offset);
    // clear mask for canvas.
    egui_canvas_clear_mask(canvas);
#endif

    // egui_font_t font;
    // extern const LATTICE_FONT_INFO Consolas_19;
    // extern const LATTICE_FONT_INFO Consolas_28;
    // extern const LATTICE_FONT_INFO KaiTi_33B;
    // egui_font_init(&font, &Consolas_19);

    // // egui_canvas_draw_text(&font, "Hello World!", base_offset, base_offset, EGUI_COLOR_WHITE, EGUI_COLOR_TRANSPARENT);
    // egui_canvas_draw_text(&font, "Hello World!", base_offset, base_offset, EGUI_COLOR_BLUE, EGUI_COLOR_DARK_GREEN);

    // EGUI_REGION_DEFINE(region, 0, 0, 128, 64);
    // egui_canvas_draw_rectangle_fill(0, 0, 128, 64, EGUI_COLOR_RED);

    // egui_canvas_draw_text_in_rect(&font, "Hello World!", &region, EGUI_ALIGN_TOP, EGUI_COLOR_BLUE, EGUI_COLOR_DARK_GREEN);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_test_t) = {
    .dispatch_touch_event = egui_view_dispatch_touch_event,
    .on_touch_event = egui_view_on_touch_event,
    .on_intercept_touch_event = egui_view_on_intercept_touch_event,
    .compute_scroll = egui_view_compute_scroll,
    .calculate_layout = egui_view_calculate_layout,
    .request_layout = egui_view_request_layout,
    .draw = egui_view_draw,
    .on_attach_to_window = egui_view_on_attach_to_window,
    .on_draw = egui_view_test_on_draw, // changed
    .on_detach_from_window = egui_view_on_detach_from_window,
};

void egui_view_test_init(egui_view_t *self)
{
    egui_view_test_t *local = (egui_view_test_t *)self;
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_test_t);

    // init local data.
    local->last_pos = 0;
}
