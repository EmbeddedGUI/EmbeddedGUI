#include <stdio.h>
#include <assert.h>

#include "egui_view_canvas.h"
#include "egui.h"

// Bright colors for dark background visibility
#define DEMO_COLOR_LIGHT_BLUE EGUI_COLOR_MAKE(80, 160, 255)
#define DEMO_COLOR_ORANGE     EGUI_COLOR_MAKE(255, 165, 0)

// Section label helper
static void draw_section_label(const char *text, egui_dim_t x, egui_dim_t y)
{
    egui_canvas_draw_text((egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, text, x, y, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

// Page 0: Lines
static void draw_page_lines(void)
{
    draw_section_label("1/11 Lines", 4, 8);

    // Row 1: Thin lines (w=1)
    draw_section_label("thin (w=1)", 4, 40);
    egui_canvas_draw_line(20, 70, 140, 140, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_line(160, 105, 280, 105, 1, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_draw_line(310, 70, 310, 140, 1, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    egui_canvas_draw_line(340, 140, 460, 70, 1, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);

    // Row 2: Thick lines (w=3)
    draw_section_label("thick (w=3)", 4, 155);
    egui_canvas_draw_line(20, 185, 140, 255, 3, EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_line(160, 220, 280, 220, 3, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_draw_line(310, 185, 310, 255, 3, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    egui_canvas_draw_line(340, 255, 460, 185, 3, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);

    // Row 3: Extra thick (w=5) + alpha
    draw_section_label("extra thick (w=5)", 4, 275);
    egui_canvas_draw_line(20, 305, 200, 395, 5, EGUI_COLOR_CYAN, EGUI_ALPHA_100);
    egui_canvas_draw_line(120, 305, 300, 395, 5, EGUI_COLOR_MAGENTA, EGUI_ALPHA_80);
    egui_canvas_draw_line(240, 350, 460, 350, 5, EGUI_COLOR_WHITE, EGUI_ALPHA_80);
}

// Page 1: Rectangles
static void draw_page_rectangles(void)
{
    draw_section_label("2/11 Rectangles", 4, 8);

    // Row 1: Outline rectangles
    draw_section_label("outline", 4, 40);
    egui_canvas_draw_rectangle(20, 70, 160, 80, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_rectangle(220, 70, 160, 80, 3, EGUI_COLOR_GREEN, EGUI_ALPHA_100);

    // Row 2: Filled rectangles
    draw_section_label("fill", 4, 165);
    egui_canvas_draw_rectangle_fill(20, 195, 160, 80, EGUI_COLOR_RED, EGUI_ALPHA_80);
    egui_canvas_draw_rectangle_fill(220, 195, 160, 80, EGUI_COLOR_GREEN, EGUI_ALPHA_80);

    // Row 3: Round rectangles
    draw_section_label("round rect", 4, 295);
    egui_canvas_draw_round_rectangle(20, 325, 160, 80, 15, 1, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle_fill(220, 325, 160, 80, 15, EGUI_COLOR_YELLOW, EGUI_ALPHA_80);
}

// Page 2: Circles & Arcs
static void draw_page_circles(void)
{
    draw_section_label("3/11 Circles & Arcs", 4, 8);

    // Row 1: Circles
    draw_section_label("circle outline & fill", 4, 40);
    egui_canvas_draw_circle(110, 120, 50, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_circle(110, 120, 35, 3, EGUI_COLOR_CYAN, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(340, 120, 50, EGUI_COLOR_GREEN, EGUI_ALPHA_80);

    // Row 2: Arcs
    draw_section_label("arc outline & fill", 4, 185);
    egui_canvas_draw_arc(110, 265, 50, 0, 270, 3, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_fill(340, 265, 50, 30, 330, EGUI_COLOR_YELLOW, EGUI_ALPHA_80);

    // Row 3: Thick ring
    draw_section_label("thick ring", 4, 335);
    egui_canvas_draw_circle(110, 400, 35, 10, EGUI_COLOR_MAGENTA, EGUI_ALPHA_100);
    egui_canvas_draw_circle(340, 400, 35, 15, EGUI_COLOR_WHITE, EGUI_ALPHA_80);
}

// Page 3: Triangles & Ellipses
static void draw_page_triangles(void)
{
    draw_section_label("4/11 Triangles & Ellipses", 4, 8);

    // Row 1: Triangles
    draw_section_label("triangle outline & fill", 4, 40);
    egui_canvas_draw_triangle(20, 170, 200, 170, 110, 70, EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_triangle_fill(260, 170, 440, 170, 350, 70, EGUI_COLOR_GREEN, EGUI_ALPHA_80);

    // Row 2: Ellipses
    draw_section_label("ellipse outline & fill", 4, 190);
    egui_canvas_draw_ellipse(120, 260, 80, 40, 1, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    egui_canvas_draw_ellipse_fill(360, 260, 70, 40, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);

    // Row 3: More ellipses
    draw_section_label("vertical & thick", 4, 320);
    egui_canvas_draw_ellipse(120, 390, 35, 55, 1, EGUI_COLOR_CYAN, EGUI_ALPHA_100);
    egui_canvas_draw_ellipse(360, 390, 50, 40, 6, EGUI_COLOR_MAGENTA, EGUI_ALPHA_100);
}

// Page 4: Polygons
static void draw_page_polygons(void)
{
    draw_section_label("5/11 Polygons", 4, 8);

    // Row 1: Pentagon
    draw_section_label("pentagon outline & fill", 4, 40);
    {
        const egui_dim_t pts[] = {110, 70, 190, 100, 160, 170, 60, 170, 30, 100};
        egui_canvas_draw_polygon(pts, 5, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);
    }
    {
        const egui_dim_t pts[] = {350, 70, 430, 100, 400, 170, 300, 170, 270, 100};
        egui_canvas_draw_polygon_fill(pts, 5, EGUI_COLOR_GREEN, EGUI_ALPHA_80);
    }

    // Row 2: Hexagon
    draw_section_label("hexagon outline & fill", 4, 185);
    {
        const egui_dim_t closed_pts[] = {70, 215, 150, 215, 190, 250, 150, 285, 70, 285, 30, 250, 70, 215};
        egui_canvas_draw_polyline_hq(closed_pts, 7, 2, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    }
    {
        const egui_dim_t pts[] = {310, 215, 390, 215, 430, 250, 390, 285, 310, 285, 270, 250};
        egui_canvas_draw_polygon_fill(pts, 6, EGUI_COLOR_YELLOW, EGUI_ALPHA_80);
    }

    // Row 3: Polyline (wave)
    draw_section_label("polyline (wave)", 4, 310);
    {
        const egui_dim_t wave[] = {20, 380, 80, 340, 140, 380, 200, 340, 260, 380, 320, 340, 380, 380, 440, 340};
        egui_canvas_draw_polyline_hq(wave, 8, 2, EGUI_COLOR_CYAN, EGUI_ALPHA_100);
    }
}

// Page 5: Bezier Curves & Alpha Blending
static void draw_page_bezier(void)
{
    draw_section_label("6/11 Bezier & Alpha", 4, 8);

    // Row 1: Quadratic bezier
    draw_section_label("quadratic bezier", 4, 40);
    egui_canvas_draw_bezier_quad(20, 140, 120, 60, 220, 140, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_bezier_quad(260, 140, 360, 60, 460, 140, 3, EGUI_COLOR_GREEN, EGUI_ALPHA_100);

    // Row 2: Cubic bezier
    draw_section_label("cubic bezier", 4, 155);
    egui_canvas_draw_bezier_cubic(20, 250, 80, 175, 160, 260, 220, 190, 1, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    egui_canvas_draw_bezier_cubic(260, 250, 310, 175, 400, 260, 460, 190, 3, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);

    // Row 3: Alpha blending demo
    draw_section_label("alpha blending", 4, 275);
    egui_canvas_draw_circle_fill(100, 340, 40, EGUI_COLOR_RED, EGUI_ALPHA_60);
    egui_canvas_draw_circle_fill(150, 340, 40, EGUI_COLOR_GREEN, EGUI_ALPHA_60);
    egui_canvas_draw_circle_fill(125, 370, 40, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_60);

    egui_canvas_draw_rectangle_fill(280, 310, 60, 60, EGUI_COLOR_RED, EGUI_ALPHA_80);
    egui_canvas_draw_rectangle_fill(320, 325, 60, 60, EGUI_COLOR_GREEN, EGUI_ALPHA_60);
    egui_canvas_draw_rectangle_fill(360, 340, 60, 60, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_60);
}

// Page 6: Circle - Basic vs HQ comparison
static void draw_page_circle_compare(void)
{
    draw_section_label("7/11 Circle: Basic vs HQ", 4, 4);

    // Column headers
    draw_section_label("Basic", 80, 30);
    draw_section_label("HQ", 340, 30);

    // Vertical divider
    egui_canvas_draw_line(240, 28, 240, 470, 1, EGUI_COLOR_MAKE(80, 80, 80), EGUI_ALPHA_60);

    // Row 1: Small circle outline, r=15 w=1
    draw_section_label("r=15 w=1", 170, 55);
    egui_canvas_draw_circle_basic(110, 95, 15, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_circle_hq(350, 95, 15, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);

    // Row 2: Medium circle outline, r=35 w=2
    draw_section_label("r=35 w=2", 170, 125);
    egui_canvas_draw_circle_basic(110, 180, 35, 2, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_draw_circle_hq(350, 180, 35, 2, EGUI_COLOR_GREEN, EGUI_ALPHA_100);

    // Row 3: Filled circle, r=40
    draw_section_label("r=40 fill", 170, 230);
    egui_canvas_draw_circle_fill_basic(110, 290, 40, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill_hq(350, 290, 40, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);

    // Row 4: Thick ring, r=30 w=10
    draw_section_label("r=30 w=10", 168, 345);
    egui_canvas_draw_circle_basic(110, 405, 30, 10, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
    egui_canvas_draw_circle_hq(350, 405, 30, 10, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
}

// Page 7: Arc - Basic vs HQ comparison
static void draw_page_arc_compare(void)
{
    draw_section_label("8/11 Arc: Basic vs HQ", 4, 4);

    draw_section_label("Basic", 80, 30);
    draw_section_label("HQ", 340, 30);

    egui_canvas_draw_line(240, 28, 240, 470, 1, EGUI_COLOR_MAKE(80, 80, 80), EGUI_ALPHA_60);

    // Row 1: Thin arc, 0-270 r=35 w=1
    draw_section_label("0-270 w=1", 164, 55);
    egui_canvas_draw_arc_basic(110, 105, 35, 0, 270, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_arc_hq(350, 105, 35, 0, 270, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);

    // Row 2: Thick arc, 30-300 r=35 w=5
    draw_section_label("30-300 w=5", 160, 155);
    egui_canvas_draw_arc_basic(110, 210, 35, 30, 300, 5, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_draw_arc_hq(350, 210, 35, 30, 300, 5, EGUI_COLOR_GREEN, EGUI_ALPHA_100);

    // Row 3: Filled arc, 0-270 r=40
    draw_section_label("0-270 fill", 164, 260);
    egui_canvas_draw_arc_fill_basic(110, 320, 40, 0, 270, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_fill_hq(350, 320, 40, 0, 270, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);

    // Row 4: Filled arc, 45-315 r=40
    draw_section_label("45-315 fill", 160, 375);
    egui_canvas_draw_arc_fill_basic(110, 430, 40, 45, 315, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
    egui_canvas_draw_arc_fill_hq(350, 430, 40, 45, 315, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
}

// Page 8: Line - Basic vs HQ comparison
static void draw_page_line_compare(void)
{
    draw_section_label("9/11 Line: Basic vs HQ", 4, 4);

    draw_section_label("Basic", 80, 30);
    draw_section_label("HQ", 340, 30);

    egui_canvas_draw_line(240, 28, 240, 470, 1, EGUI_COLOR_MAKE(80, 80, 80), EGUI_ALPHA_60);

    // Row 1: Diagonal w=3
    draw_section_label("w=3 diag", 172, 55);
    egui_canvas_draw_line(30, 75, 200, 140, 3, EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_line_hq(270, 75, 440, 140, 3, EGUI_COLOR_RED, EGUI_ALPHA_100);

    // Row 2: Steep w=4
    draw_section_label("w=4 steep", 170, 155);
    egui_canvas_draw_line(80, 180, 160, 270, 4, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_draw_line_hq(320, 180, 400, 270, 4, EGUI_COLOR_GREEN, EGUI_ALPHA_100);

    // Row 3: Polyline w=3
    draw_section_label("polyline w=3", 164, 285);
    {
        const egui_dim_t pts[] = {20, 340, 70, 310, 120, 350, 170, 310, 210, 340};
        egui_canvas_draw_polyline(pts, 5, 3, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    }
    {
        const egui_dim_t pts[] = {260, 340, 310, 310, 360, 350, 410, 310, 450, 340};
        egui_canvas_draw_polyline_hq(pts, 5, 3, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    }

    // Row 4: Thick w=6
    draw_section_label("w=6 diag", 172, 365);
    egui_canvas_draw_line(30, 395, 200, 450, 6, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
    egui_canvas_draw_line_hq(270, 395, 440, 450, 6, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
}

// Page 9: Line Round Cap vs Butt Cap
static void draw_page_line_round_cap(void)
{
    draw_section_label("10/11 Line: Butt vs Round Cap", 4, 4);

    draw_section_label("Butt Cap", 60, 30);
    draw_section_label("Round Cap", 320, 30);

    egui_canvas_draw_line(240, 28, 240, 470, 1, EGUI_COLOR_MAKE(80, 80, 80), EGUI_ALPHA_60);

    // Row 1: horizontal short line, large width (cap difference most obvious)
    draw_section_label("short horiz w=10", 150, 55);
    egui_canvas_draw_line_hq(50, 92, 170, 92, 10, EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_line_round_cap_hq(290, 92, 410, 92, 10, EGUI_COLOR_RED, EGUI_ALPHA_100);

    // Row 2: diagonal medium line
    draw_section_label("diag w=8", 172, 150);
    egui_canvas_draw_line_hq(50, 205, 180, 255, 8, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_draw_line_round_cap_hq(290, 205, 420, 255, 8, EGUI_COLOR_GREEN, EGUI_ALPHA_100);

    // Row 3: polyline
    draw_section_label("polyline w=6", 164, 275);
    {
        const egui_dim_t pts[] = {30, 330, 90, 295, 140, 340, 185, 295, 220, 330};
        egui_canvas_draw_polyline_hq(pts, 5, 6, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    }
    {
        const egui_dim_t pts[] = {270, 330, 330, 295, 380, 340, 425, 295, 460, 330};
        egui_canvas_draw_polyline_round_cap_hq(pts, 5, 6, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    }

    // Row 4: vertical line
    draw_section_label("vertical w=12", 162, 380);
    egui_canvas_draw_line_hq(110, 402, 110, 458, 12, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
    egui_canvas_draw_line_round_cap_hq(350, 402, 350, 458, 12, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
}

// Page 10: Arc Round Cap vs No Cap
static void draw_page_arc_round_cap(void)
{
    draw_section_label("11/11 Arc: Normal vs Round Cap", 4, 4);

    draw_section_label("Normal", 70, 30);
    draw_section_label("Round Cap", 320, 30);

    egui_canvas_draw_line(240, 28, 240, 470, 1, EGUI_COLOR_MAKE(80, 80, 80), EGUI_ALPHA_60);

    // Row 1: activity_ring-like 75% progress (start=270, end=540)
    draw_section_label("270-540 w=12", 150, 55);
    egui_canvas_draw_arc_hq(110, 125, 44, 270, 540, 12, EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_arc_round_cap_hq(350, 125, 44, 270, 540, 12, EGUI_COLOR_RED, EGUI_ALPHA_100);

    // Row 2: activity_ring-like 50% progress (start=270, end=450)
    draw_section_label("270-450 w=10", 150, 205);
    egui_canvas_draw_arc_hq(110, 275, 46, 270, 450, 10, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_draw_arc_round_cap_hq(350, 275, 46, 270, 450, 10, EGUI_COLOR_GREEN, EGUI_ALPHA_100);

    // Row 3: activity_ring-like 30% progress (start=270, end=378)
    draw_section_label("270-378 w=12", 150, 355);
    egui_canvas_draw_arc_hq(110, 420, 40, 270, 378, 12, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_round_cap_hq(350, 420, 40, 270, 378, 12, DEMO_COLOR_LIGHT_BLUE, EGUI_ALPHA_100);
}

// Page dispatch table
typedef void (*draw_page_func_t)(void);
static const draw_page_func_t page_funcs[EGUI_VIEW_CANVAS_PAGE_COUNT] = {
        draw_page_lines,          draw_page_rectangles,  draw_page_circles,      draw_page_triangles,      draw_page_polygons,      draw_page_bezier,
        draw_page_circle_compare, draw_page_arc_compare, draw_page_line_compare, draw_page_line_round_cap, draw_page_arc_round_cap,
};

void egui_view_canvas_on_draw(egui_view_t *self)
{
    egui_view_canvas_t *local = (egui_view_canvas_t *)self;
    uint8_t page = local->current_page;
    if (page < EGUI_VIEW_CANVAS_PAGE_COUNT)
    {
        page_funcs[page]();
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_canvas_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_canvas_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
};

void egui_view_canvas_init(egui_view_t *self)
{
    egui_view_canvas_t *local = (egui_view_canvas_t *)self;
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_canvas_t);
    local->current_page = 0;
}

void egui_view_canvas_init_with_page(egui_view_t *self, uint8_t page)
{
    egui_view_canvas_init(self);
    egui_view_canvas_t *local = (egui_view_canvas_t *)self;
    if (page < EGUI_VIEW_CANVAS_PAGE_COUNT)
    {
        local->current_page = page;
    }
}

void egui_view_canvas_set_page(egui_view_t *self, uint8_t page)
{
    egui_view_canvas_t *local = (egui_view_canvas_t *)self;
    if (page < EGUI_VIEW_CANVAS_PAGE_COUNT)
    {
        local->current_page = page;
        egui_view_invalidate(self);
    }
}
