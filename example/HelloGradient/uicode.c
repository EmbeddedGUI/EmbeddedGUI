#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

/* ============================================================
 * HelloGradient - Gradient canvas drawing demo
 * 5 pages via ViewPage, each showing different gradient shapes
 * ============================================================ */

#define PAGE_COUNT 7
#define SCR_W      EGUI_CONFIG_SCEEN_WIDTH
#define SCR_H      EGUI_CONFIG_SCEEN_HEIGHT

/* ---------- ViewPage and page views ---------- */
static egui_view_viewpage_t viewpage;
static egui_view_t page_views[PAGE_COUNT]; /* PAGE_COUNT = 7 */

EGUI_VIEW_VIEWPAGE_PARAMS_INIT(viewpage_params, 0, 0, SCR_W, SCR_H);

/* ---------- Gradient definitions ---------- */

/* 2-color vertical: red -> blue */
EGUI_GRADIENT_LINEAR_2COLOR(grad_v_rb, EGUI_GRADIENT_TYPE_LINEAR_VERTICAL, EGUI_COLOR_RED, EGUI_COLOR_BLUE, EGUI_ALPHA_100);

/* 2-color horizontal: green -> yellow */
EGUI_GRADIENT_LINEAR_2COLOR(grad_h_gy, EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL, EGUI_COLOR_GREEN, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);

/* 3-color vertical: red -> green -> blue */
static const egui_gradient_stop_t stops_rgb[] = {
        {.position = 0, .color = EGUI_COLOR_RED},
        {.position = 128, .color = EGUI_COLOR_GREEN},
        {.position = 255, .color = EGUI_COLOR_BLUE},
};
EGUI_GRADIENT_LINEAR(grad_v_rgb, EGUI_GRADIENT_TYPE_LINEAR_VERTICAL, EGUI_ALPHA_100, stops_rgb, 3);

/* Radial: white center -> dark blue edge */
static const egui_gradient_stop_t stops_radial_wb[] = {
        {.position = 0, .color = EGUI_COLOR_WHITE},
        {.position = 255, .color = EGUI_COLOR_NAVY},
};

/* 4-color horizontal: rainbow-like */
static const egui_gradient_stop_t stops_rainbow[] = {
        {.position = 0, .color = EGUI_COLOR_RED},
        {.position = 85, .color = EGUI_COLOR_YELLOW},
        {.position = 170, .color = EGUI_COLOR_GREEN},
        {.position = 255, .color = EGUI_COLOR_BLUE},
};

/* Radial: cyan center -> magenta edge */
static const egui_gradient_stop_t stops_radial_cm[] = {
        {.position = 0, .color = EGUI_COLOR_CYAN},
        {.position = 255, .color = EGUI_COLOR_MAGENTA},
};

/* Vertical: orange -> purple */
EGUI_GRADIENT_LINEAR_2COLOR(grad_v_op, EGUI_GRADIENT_TYPE_LINEAR_VERTICAL, EGUI_COLOR_ORANGE, EGUI_COLOR_PURPLE, EGUI_ALPHA_100);

/* Horizontal: yellow -> cyan */
EGUI_GRADIENT_LINEAR_2COLOR(grad_h_yc, EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL, EGUI_COLOR_YELLOW, EGUI_COLOR_CYAN, EGUI_ALPHA_100);

/* Radial: yellow center -> red edge (for circle sphere effect) */
static const egui_gradient_stop_t stops_radial_yr[] = {
        {.position = 0, .color = EGUI_COLOR_YELLOW},
        {.position = 255, .color = EGUI_COLOR_RED},
};

/* ============================================================
 * Page 1: Rectangle gradients
 * ============================================================ */
static void page1_on_draw(egui_view_t *self)
{
    egui_dim_t bx = self->region_screen.location.x;
    egui_dim_t by = self->region_screen.location.y + (SCR_H - 278) / 2; /* vertical center */
    egui_dim_t rw = 100;
    egui_dim_t rh = 60;
    egui_dim_t gap = 10;
    egui_dim_t x_left = bx + (SCR_W / 2 - rw - gap / 2);
    egui_dim_t x_right = bx + (SCR_W / 2 + gap / 2);
    egui_dim_t y_start = by + 30;

    /* Title */
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Rect Gradients", bx + 50, by + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 1: vertical 2-color, horizontal 2-color */
    egui_canvas_draw_rectangle_fill_gradient(x_left, y_start, rw, rh, &grad_v_rb);
    egui_canvas_draw_rectangle_fill_gradient(x_right, y_start, rw, rh, &grad_h_gy);

    /* Labels */
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "V 2-color", x_left, y_start + rh + 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "H 2-color", x_right, y_start + rh + 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 2: vertical 3-color, radial */
    egui_dim_t y2 = y_start + rh + 25;
    egui_canvas_draw_rectangle_fill_gradient(x_left, y2, rw, rh, &grad_v_rgb);

    /* Radial gradient for rectangle */
    egui_gradient_t grad_radial_rect = {
            .type = EGUI_GRADIENT_TYPE_RADIAL,
            .stop_count = 2,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_radial_wb,
            .center_x = rw / 2,
            .center_y = rh / 2,
            .radius = 60,
    };
    egui_canvas_draw_rectangle_fill_gradient(x_right, y2, rw, rh, &grad_radial_rect);

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "V 3-color", x_left, y2 + rh + 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Radial", x_right, y2 + rh + 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 3: 4-color horizontal rainbow */
    egui_dim_t y3 = y2 + rh + 25;
    egui_gradient_t grad_rainbow = {
            .type = EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL,
            .stop_count = 4,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_rainbow,
    };
    egui_canvas_draw_rectangle_fill_gradient(bx + (SCR_W - 200) / 2, y3, 200, rh, &grad_rainbow);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "H 4-color Rainbow", bx + (SCR_W - 200) / 2, y3 + rh + 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

/* ============================================================
 * Page 2: Round rectangle gradients
 * ============================================================ */
static void page2_on_draw(egui_view_t *self)
{
    egui_dim_t bx = self->region_screen.location.x;
    egui_dim_t by = self->region_screen.location.y + (SCR_H - 288) / 2; /* vertical center */
    egui_dim_t rw = 100;
    egui_dim_t rh = 70;
    egui_dim_t gap = 10;
    egui_dim_t x_left = bx + (SCR_W / 2 - rw - gap / 2);
    egui_dim_t x_right = bx + (SCR_W / 2 + gap / 2);
    egui_dim_t y_start = by + 30;

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "RoundRect Gradients", bx + 30, by + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 1: vertical, horizontal */
    egui_canvas_draw_round_rectangle_fill_gradient(x_left, y_start, rw, rh, 15, &grad_v_op);
    egui_canvas_draw_round_rectangle_fill_gradient(x_right, y_start, rw, rh, 15, &grad_h_yc);

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Vertical", x_left, y_start + rh + 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Horizontal", x_right, y_start + rh + 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 2: radial, per-corner radii with rainbow */
    egui_dim_t y2 = y_start + rh + 25;
    egui_gradient_t grad_rr_radial = {
            .type = EGUI_GRADIENT_TYPE_RADIAL,
            .stop_count = 2,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_radial_cm,
            .center_x = rw / 2,
            .center_y = rh / 2,
            .radius = 70,
    };
    egui_canvas_draw_round_rectangle_fill_gradient(x_left, y2, rw, rh, 15, &grad_rr_radial);

    egui_gradient_t grad_rr_rainbow = {
            .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
            .stop_count = 4,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_rainbow,
    };
    egui_canvas_draw_round_rectangle_corners_fill_gradient(x_right, y2, rw, rh, 5, 20, 20, 5, &grad_rr_rainbow);

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Radial", x_left, y2 + rh + 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Corners", x_right, y2 + rh + 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 3: wide round rect with 3-color vertical */
    egui_dim_t y3 = y2 + rh + 25;
    egui_canvas_draw_round_rectangle_fill_gradient(bx + (SCR_W - 200) / 2, y3, 200, 50, 20, &grad_v_rgb);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "V 3-color Wide", bx + (SCR_W - 200) / 2, y3 + 52, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

/* ============================================================
 * Page 3: Circle gradients
 * ============================================================ */
static void page3_on_draw(egui_view_t *self)
{
    egui_dim_t bx = self->region_screen.location.x;
    egui_dim_t by = self->region_screen.location.y + (SCR_H - 241) / 2; /* vertical center */
    egui_dim_t cr = 40;
    //     egui_dim_t gap = 10;
    egui_dim_t cx_left = bx + SCR_W / 4;
    egui_dim_t cx_right = bx + SCR_W * 3 / 4;
    egui_dim_t y_start = by + 30 + cr;

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Circle Gradients", bx + 40, by + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 1: vertical linear, horizontal linear */
    egui_canvas_draw_circle_fill_gradient(cx_left, y_start, cr, &grad_v_rb);
    egui_canvas_draw_circle_fill_gradient(cx_right, y_start, cr, &grad_h_gy);

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Vertical", cx_left - 30, y_start + cr + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Horizontal", cx_right - 35, y_start + cr + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 2: radial (sphere-like), 3-color vertical */
    egui_dim_t y2 = y_start + cr + 30 + cr;
    egui_gradient_t grad_circle_radial = {
            .type = EGUI_GRADIENT_TYPE_RADIAL,
            .stop_count = 2,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_radial_yr,
            .center_x = -10,
            .center_y = -10,
            .radius = (egui_dim_t)(cr * 2),
    };
    egui_canvas_draw_circle_fill_gradient(cx_left, y2, cr, &grad_circle_radial);
    egui_canvas_draw_circle_fill_gradient(cx_right, y2, cr, &grad_v_rgb);

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Radial", cx_left - 25, y2 + cr + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "V 3-color", cx_right - 35, y2 + cr + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

/* ============================================================
 * Page 4: Triangle & Ellipse gradients
 * ============================================================ */
static void page4_on_draw(egui_view_t *self)
{
    egui_dim_t bx = self->region_screen.location.x;
    egui_dim_t by = self->region_screen.location.y + (SCR_H - 236) / 2; /* vertical center */
    egui_dim_t y_start = by + 30;

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Tri & Ellipse", bx + 50, by + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Triangle 1: vertical gradient */
    egui_dim_t tx1 = bx + SCR_W / 4;
    egui_dim_t ty1 = y_start;
    egui_canvas_draw_triangle_fill_gradient(tx1, ty1, tx1 - 50, ty1 + 80, tx1 + 50, ty1 + 80, &grad_v_rb);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "V Linear", tx1 - 30, ty1 + 85, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Triangle 2: radial gradient */
    egui_dim_t tx2 = bx + SCR_W * 3 / 4;
    egui_gradient_t grad_tri_radial = {
            .type = EGUI_GRADIENT_TYPE_RADIAL,
            .stop_count = 2,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_radial_cm,
            .center_x = 0,
            .center_y = 30,
            .radius = 60,
    };
    egui_canvas_draw_triangle_fill_gradient(tx2, ty1, tx2 - 50, ty1 + 80, tx2 + 50, ty1 + 80, &grad_tri_radial);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Radial", tx2 - 25, ty1 + 85, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Ellipse 1: horizontal gradient */
    egui_dim_t ey1 = y_start + 150;
    egui_dim_t ecx1 = bx + SCR_W / 4;
    egui_canvas_draw_ellipse_fill_gradient(ecx1, ey1, 50, 35, &grad_h_yc);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "H Linear", ecx1 - 30, ey1 + 40, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Ellipse 2: radial gradient */
    egui_dim_t ecx2 = bx + SCR_W * 3 / 4;
    egui_gradient_t grad_ell_radial = {
            .type = EGUI_GRADIENT_TYPE_RADIAL,
            .stop_count = 2,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_radial_wb,
            .center_x = -10,
            .center_y = -5,
            .radius = 50,
    };
    egui_canvas_draw_ellipse_fill_gradient(ecx2, ey1, 50, 35, &grad_ell_radial);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Radial", ecx2 - 25, ey1 + 40, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

/* ============================================================
 * Page 5: Polygon gradients
 * ============================================================ */
static void page5_on_draw(egui_view_t *self)
{
    egui_dim_t bx = self->region_screen.location.x;
    egui_dim_t by = self->region_screen.location.y + (SCR_H - 281) / 2; /* vertical center */

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Polygon Gradients", bx + 35, by + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Hexagon 1: vertical gradient */
    egui_dim_t hcx1 = bx + SCR_W / 4;
    egui_dim_t hcy1 = by + 30 + 50;
    egui_dim_t hr = 45;
    /* Hexagon vertices (flat-top) */
    egui_dim_t hex1[] = {
            hcx1 - hr, hcy1, hcx1 - hr / 2, hcy1 - (hr * 7 / 8), hcx1 + hr / 2, hcy1 - (hr * 7 / 8),
            hcx1 + hr, hcy1, hcx1 + hr / 2, hcy1 + (hr * 7 / 8), hcx1 - hr / 2, hcy1 + (hr * 7 / 8),
    };
    egui_canvas_draw_polygon_fill_gradient(hex1, 6, &grad_v_rgb);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "V 3-color", hcx1 - 35, hcy1 + hr + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Hexagon 2: radial gradient */
    egui_dim_t hcx2 = bx + SCR_W * 3 / 4;
    egui_dim_t hcy2 = hcy1;
    egui_dim_t hex2[] = {
            hcx2 - hr, hcy2, hcx2 - hr / 2, hcy2 - (hr * 7 / 8), hcx2 + hr / 2, hcy2 - (hr * 7 / 8),
            hcx2 + hr, hcy2, hcx2 + hr / 2, hcy2 + (hr * 7 / 8), hcx2 - hr / 2, hcy2 + (hr * 7 / 8),
    };
    egui_gradient_t grad_hex_radial = {
            .type = EGUI_GRADIENT_TYPE_RADIAL,
            .stop_count = 2,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_radial_yr,
            .center_x = 0,
            .center_y = 0,
            .radius = hr,
    };
    egui_canvas_draw_polygon_fill_gradient(hex2, 6, &grad_hex_radial);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Radial", hcx2 - 25, hcy2 + hr + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Pentagon: horizontal rainbow */
    egui_dim_t pcx = bx + SCR_W / 2;
    egui_dim_t pcy = by + 220;
    egui_dim_t pr = 40;
    egui_dim_t pent[] = {
            pcx,
            pcy - pr,
            pcx + (pr * 19 / 20),
            pcy - (pr * 6 / 20),
            pcx + (pr * 12 / 20),
            pcy + (pr * 16 / 20),
            pcx - (pr * 12 / 20),
            pcy + (pr * 16 / 20),
            pcx - (pr * 19 / 20),
            pcy - (pr * 6 / 20),
    };
    egui_gradient_t grad_pent = {
            .type = EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL,
            .stop_count = 4,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_rainbow,
    };
    egui_canvas_draw_polygon_fill_gradient(pent, 5, &grad_pent);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "H Rainbow", pcx - 35, pcy + pr + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

/* ============================================================
 * Page 6: Ring Gradients (circle ring + round rect ring)
 * ============================================================ */
static void page6_on_draw(egui_view_t *self)
{
    egui_dim_t bx = self->region_screen.location.x;
    egui_dim_t by = self->region_screen.location.y + (SCR_H - 279) / 2; /* vertical center */
    egui_dim_t cr = 40;
    egui_dim_t cx_left = bx + SCR_W / 4;
    egui_dim_t cx_right = bx + SCR_W * 3 / 4;
    egui_dim_t y_start = by + 30 + cr;

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Ring Gradients", bx + 45, by + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 1: circle rings */
    egui_canvas_draw_circle_ring_fill_gradient(cx_left, y_start, cr, cr - 12, &grad_v_rb);
    egui_canvas_draw_circle_ring_fill_gradient(cx_right, y_start, cr, cr - 12, &grad_h_gy);

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Ring V", cx_left - 25, y_start + cr + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Ring H", cx_right - 25, y_start + cr + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 2: round rect rings */
    egui_dim_t y2 = y_start + cr + 30;
    egui_dim_t rw = 90;
    egui_dim_t rh = 55;
    egui_dim_t rl = cx_left - rw / 2;
    egui_dim_t rr = cx_right - rw / 2;

    egui_canvas_draw_round_rectangle_ring_fill_gradient(rl, y2, rw, rh, 8, 14, &grad_v_op);

    egui_gradient_t grad_rr_ring_radial = {
            .type = EGUI_GRADIENT_TYPE_RADIAL,
            .stop_count = 2,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_radial_cm,
            .center_x = rw / 2,
            .center_y = rh / 2,
            .radius = 60,
    };
    egui_canvas_draw_round_rectangle_ring_fill_gradient(rr, y2, rw, rh, 8, 14, &grad_rr_ring_radial);

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "RRect V", rl, y2 + rh + 3, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "RRect Rad", rr, y2 + rh + 3, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 3: plain rect ring */
    egui_dim_t y3 = y2 + rh + 25;
    egui_canvas_draw_rectangle_ring_fill_gradient(bx + (SCR_W - 180) / 2, y3, 180, 40, 6, &grad_h_yc);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Rect Ring H", bx + (SCR_W - 180) / 2, y3 + 43, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

/* ============================================================
 * Page 7: Capsule Line & Arc Ring Gradients
 * ============================================================ */
static void page7_on_draw(egui_view_t *self)
{
    egui_dim_t bx = self->region_screen.location.x;
    egui_dim_t by = self->region_screen.location.y + (SCR_H - 400) / 2; /* vertical center */

    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Capsule & Arc", bx + 45, by + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 1: horizontal capsule - vertical gradient */
    egui_dim_t cap_y1 = by + 40;
    egui_canvas_draw_line_capsule_fill_gradient(bx + 20, cap_y1, bx + SCR_W - 20, cap_y1, 18, &grad_v_rb);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Capsule V", bx + 20, cap_y1 + 14, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 2: diagonal capsule - horizontal gradient */
    egui_dim_t cap_y2 = cap_y1 + 50;
    egui_canvas_draw_line_capsule_fill_gradient(bx + 30, cap_y2, bx + SCR_W - 30, cap_y2 + 25, 14, &grad_h_gy);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Capsule H", bx + SCR_W / 2 - 35, cap_y2 + 38, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 3: arc rings */
    egui_dim_t arc_y = cap_y2 + 110;
    egui_dim_t acx_l = bx + SCR_W / 4;
    egui_dim_t acx_r = bx + SCR_W * 3 / 4;
    egui_dim_t ar = 45;

    /* Arc ring 270 degrees (progress style, -225 to 45 deg clockwise) */
    egui_canvas_draw_arc_ring_fill_gradient(acx_l, arc_y, ar, ar - 14, -225, 45, &grad_v_rb);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Arc 270", acx_l - 30, arc_y + ar + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Arc ring 180 degrees (semicircle) with rainbow gradient */
    egui_gradient_t grad_arc_rainbow = {
            .type = EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL,
            .stop_count = 4,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_rainbow,
    };
    egui_canvas_draw_arc_ring_fill_gradient(acx_r, arc_y, ar, ar - 14, 180, 360, &grad_arc_rainbow);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Arc 180", acx_r - 30, arc_y + ar + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* Row 4: two more arc rings */
    egui_dim_t arc_y2 = arc_y + ar + 70;

    /* Full 360 arc ring = circle ring */
    egui_canvas_draw_arc_ring_fill_gradient(acx_l, arc_y2, ar - 5, ar - 20, 0, 0, &grad_h_yc);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Full 360", acx_l - 30, arc_y2 + ar + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    /* 90-degree arc */
    egui_gradient_t grad_arc_radial = {
            .type = EGUI_GRADIENT_TYPE_RADIAL,
            .stop_count = 2,
            .alpha = EGUI_ALPHA_100,
            .stops = stops_radial_yr,
            .center_x = (ar - 5),
            .center_y = (ar - 5),
            .radius = ar,
    };
    egui_canvas_draw_arc_ring_fill_gradient(acx_r, arc_y2, ar, ar - 15, 0, 90, &grad_arc_radial);
    egui_canvas_draw_text(EGUI_FONT_OF(EGUI_CONFIG_FONT_DEFAULT), "Arc 90", acx_r - 25, arc_y2 + ar + 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

/* ============================================================
 * Custom API tables - override on_draw for each page
 * Use function names directly (not via extern struct member access)
 * ============================================================ */

#define DEFINE_PAGE_API(_idx)                                                                                                                                  \
    static const egui_view_api_t page##_idx##_api = {                                                                                                          \
            .dispatch_touch_event = egui_view_dispatch_touch_event,                                                                                            \
            .on_touch_event = egui_view_on_touch_event,                                                                                                        \
            .on_intercept_touch_event = egui_view_on_intercept_touch_event,                                                                                    \
            .compute_scroll = egui_view_compute_scroll,                                                                                                        \
            .calculate_layout = egui_view_calculate_layout,                                                                                                    \
            .request_layout = egui_view_request_layout,                                                                                                        \
            .draw = egui_view_draw,                                                                                                                            \
            .on_attach_to_window = egui_view_on_attach_to_window,                                                                                              \
            .on_draw = page##_idx##_on_draw,                                                                                                                   \
            .on_detach_from_window = egui_view_on_detach_from_window,                                                                                          \
    }

DEFINE_PAGE_API(1);
DEFINE_PAGE_API(2);
DEFINE_PAGE_API(3);
DEFINE_PAGE_API(4);
DEFINE_PAGE_API(5);
DEFINE_PAGE_API(6);
DEFINE_PAGE_API(7);

/* Dark background for all pages */
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_dark_param, EGUI_COLOR_MAKE(20, 20, 30), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_dark_params, &bg_dark_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_dark, &bg_dark_params);

void uicode_create_ui(void)
{
    /* Init ViewPage */
    egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&viewpage), &viewpage_params);

    /* Init each page view and override API */
    static const egui_view_api_t *page_apis[PAGE_COUNT] = {
            &page1_api, &page2_api, &page3_api, &page4_api, &page5_api, &page6_api, &page7_api,
    };

    int i;
    for (i = 0; i < PAGE_COUNT; i++)
    {
        egui_view_init(EGUI_VIEW_OF(&page_views[i]));
        egui_view_set_size(EGUI_VIEW_OF(&page_views[i]), SCR_W, SCR_H);
        page_views[i].api = page_apis[i];
        egui_view_set_background(EGUI_VIEW_OF(&page_views[i]), EGUI_BG_OF(&bg_dark));
        egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_views[i]));
    }

    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    /* Each page: swipe left then wait to capture settled frame */
    case 0: /* wait for page 1 render */
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 1:  /* swipe left -> page 2 */
    case 3:  /* swipe left -> page 3 */
    case 5:  /* swipe left -> page 4 */
    case 7:  /* swipe left -> page 5 */
    case 9:  /* swipe left -> page 6 */
    case 11: /* swipe left -> page 7 */
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = SCR_W * 3 / 4;
        p_action->y1 = SCR_H / 2;
        p_action->x2 = SCR_W / 4;
        p_action->y2 = SCR_H / 2;
        p_action->steps = 5;
        p_action->interval_ms = 500;
        return true;
    case 2:  /* wait for page 2 */
    case 4:  /* wait for page 3 */
    case 6:  /* wait for page 4 */
    case 8:  /* wait for page 5 */
    case 10: /* wait for page 6 */
    case 12: /* wait for page 7 */
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    default:
        return false;
    }
}
#endif
