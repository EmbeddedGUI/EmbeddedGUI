#include <stdio.h>

#include "egui.h"
#include "uicode_disp0.h"

#define PUNCH_REGION_X              24
#define PUNCH_REGION_Y              88
#define PUNCH_REGION_W              192
#define PUNCH_REGION_H              128
#define PUNCH_REGION_ROWS_PER_TICK  4
#define PUNCH_REGION_START_DELAY_MS 1200
#define PUNCH_REGION_TICK_MS        45

#define DEMO_TITLE_FONT    ((const egui_font_t *)&egui_res_font_montserrat_16_4)
#define DEMO_BODY_FONT     ((const egui_font_t *)&egui_res_font_montserrat_12_4)
#define DEMO_STATUS_FONT   ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define DEMO_BORDER_COLOR  EGUI_COLOR_HEX(0xF59E0B)
#define DEMO_STATUS_COLOR  EGUI_COLOR_HEX(0xCBD5E1)
#define DEMO_HEADING_COLOR EGUI_COLOR_HEX(0xF8FAFC)

static egui_view_group_t root_group;
static egui_view_label_t title_label;
static egui_view_label_t body_label;
static egui_view_label_t status_label;
static egui_view_t border_top;
static egui_view_t border_bottom;
static egui_view_t border_left;
static egui_view_t border_right;
static egui_timer_t render_timer;
static egui_core_t *s_core;
static uint16_t s_next_row;
static uint8_t s_timer_inited;
static uint8_t s_placeholder_drawn;
static char s_status_text[40];

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static uint8_t s_runtime_fail_reported;
#endif

EGUI_VIEW_GROUP_PARAMS_INIT(root_group_params, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_root_param, EGUI_COLOR_HEX(0x0F172A), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_root_params, &bg_root_param, &bg_root_param, &bg_root_param);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_root, &bg_root_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_border_param, DEMO_BORDER_COLOR, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_border_params, &bg_border_param, &bg_border_param, &bg_border_param);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_border, &bg_border_params);

static egui_color_int_t punch_region_make_color(uint8_t red, uint8_t green, uint8_t blue)
{
    return EGUI_COLOR_MAKE(red, green, blue).full;
}

static void punch_region_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const char *text,
                                    const egui_font_t *font, egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label), s_core);
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), width, height);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_text(EGUI_VIEW_OF(label), text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), EGUI_ALIGN_CENTER | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static void punch_region_init_border(egui_view_t *view, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_view_init(view, s_core);
    egui_view_set_position(view, x, y);
    egui_view_set_size(view, width, height);
    egui_view_set_background(view, EGUI_BG_OF(&bg_border));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), view);
}

static void punch_region_update_status(const char *state, uint16_t rows)
{
    snprintf(s_status_text, sizeof(s_status_text), "%s %u/%u", state, (unsigned)rows, (unsigned)PUNCH_REGION_H);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), s_status_text);
}

static void punch_region_fill_placeholder_row(uint16_t row_y, egui_color_int_t row[PUNCH_REGION_W])
{
    uint16_t x;

    for (x = 0; x < PUNCH_REGION_W; x++)
    {
        uint8_t shade = (uint8_t)(24u + ((uint32_t)(x + row_y) * 28u) / (PUNCH_REGION_W + PUNCH_REGION_H));
        row[x] = punch_region_make_color(shade, (uint8_t)(44u + shade / 2u), (uint8_t)(70u + shade));
    }

    if (row_y >= 42u && row_y < 86u)
    {
        uint16_t icon_y = (uint16_t)(row_y - 42u);
        for (x = 72u; x < 120u; x++)
        {
            uint16_t icon_x = (uint16_t)(x - 72u);
            if ((icon_x > 8u && icon_x < 40u && icon_y > 6u && icon_y < 38u) || (icon_x > 2u && icon_x < 46u && icon_y > 16u && icon_y < 28u))
            {
                row[x] = punch_region_make_color(148u, 163u, 184u);
            }
        }
    }
}

static void punch_region_fill_image_row(uint16_t row_y, egui_color_int_t row[PUNCH_REGION_W])
{
    uint16_t x;
    uint16_t center_x = PUNCH_REGION_W / 2u;
    uint16_t center_y = PUNCH_REGION_H / 2u;

    for (x = 0; x < PUNCH_REGION_W; x++)
    {
        int16_t dx = (int16_t)x - (int16_t)center_x;
        int16_t dy = (int16_t)row_y - (int16_t)center_y;
        uint16_t distance = (uint16_t)((dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy));
        uint8_t red = (uint8_t)(36u + ((uint32_t)x * 120u) / PUNCH_REGION_W);
        uint8_t green = (uint8_t)(72u + ((uint32_t)row_y * 96u) / PUNCH_REGION_H);
        uint8_t blue = (uint8_t)(150u + (distance % 82u));

        if (((x / 12u) + (row_y / 12u)) % 2u == 0u)
        {
            red = (uint8_t)(red + 22u);
            green = (uint8_t)(green + 16u);
        }

        if (distance < 38u)
        {
            red = 245u;
            green = (uint8_t)(180u + distance);
            blue = 42u;
        }

        row[x] = punch_region_make_color(red, green, blue);
    }
}

static void punch_region_direct_draw_rows(uint16_t start_row, uint16_t row_count, uint8_t final_image)
{
    static egui_color_int_t row[PUNCH_REGION_W];
    uint16_t y;

    if (s_core == NULL || row_count == 0u)
    {
        return;
    }

    egui_pfb_bus_acquire(s_core);
    for (y = 0u; y < row_count && (uint16_t)(start_row + y) < PUNCH_REGION_H; y++)
    {
        uint16_t row_y = (uint16_t)(start_row + y);
        if (final_image)
        {
            punch_region_fill_image_row(row_y, row);
        }
        else
        {
            punch_region_fill_placeholder_row(row_y, row);
        }
        egui_api_draw_data(s_core, PUNCH_REGION_X, (int16_t)(PUNCH_REGION_Y + row_y), PUNCH_REGION_W, 1, row);
    }
    egui_pfb_bus_release(s_core);
    egui_api_refresh_display(s_core);
}

static void punch_region_draw_placeholder(void)
{
    punch_region_direct_draw_rows(0u, PUNCH_REGION_H, 0u);
    punch_region_update_status("Placeholder", 0u);
}

static uint8_t punch_region_draw_next_batch(void)
{
    uint16_t rows_left;
    uint16_t rows_to_draw;

    if (s_next_row >= PUNCH_REGION_H)
    {
        return 0u;
    }

    rows_left = (uint16_t)(PUNCH_REGION_H - s_next_row);
    rows_to_draw = rows_left < PUNCH_REGION_ROWS_PER_TICK ? rows_left : PUNCH_REGION_ROWS_PER_TICK;
    punch_region_direct_draw_rows(s_next_row, rows_to_draw, 1u);
    s_next_row = (uint16_t)(s_next_row + rows_to_draw);

    if (s_next_row >= PUNCH_REGION_H)
    {
        punch_region_update_status("Direct draw complete", s_next_row);
        return 0u;
    }

    punch_region_update_status("Direct drawing", s_next_row);
    return 1u;
}

static void punch_region_timer_cb(egui_timer_t *timer)
{
    EGUI_UNUSED(timer);

    if (!s_placeholder_drawn)
    {
        punch_region_draw_placeholder();
        s_placeholder_drawn = 1u;
        egui_timer_start_timer(s_core, &render_timer, PUNCH_REGION_START_DELAY_MS, PUNCH_REGION_TICK_MS);
        return;
    }

    if (!punch_region_draw_next_batch() && s_core != NULL)
    {
        egui_timer_stop_timer(s_core, &render_timer);
    }
}

void test_init_ui(egui_core_t *core)
{
    s_core = core;
    s_next_row = 0u;
    s_placeholder_drawn = 0u;
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
    s_runtime_fail_reported = 0u;
#endif

    if (s_timer_inited)
    {
        egui_timer_stop_timer(core, &render_timer);
    }
    else
    {
        egui_timer_init_timer(&render_timer, NULL, punch_region_timer_cb);
        s_timer_inited = 1u;
    }

    egui_view_group_init_with_params(EGUI_VIEW_OF(&root_group), core, &root_group_params);
    egui_view_set_background(EGUI_VIEW_OF(&root_group), EGUI_BG_OF(&bg_root));

    punch_region_init_label(&title_label, 8, 12, 224, 22, "Punch Region", DEMO_TITLE_FONT, DEMO_HEADING_COLOR);
    punch_region_init_label(&body_label, 14, 40, 212, 32, "EGUI skips center; app draws it.", DEMO_BODY_FONT, DEMO_STATUS_COLOR);
    punch_region_init_label(&status_label, 16, 232, 208, 20, "Waiting", DEMO_STATUS_FONT, DEMO_STATUS_COLOR);

    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&title_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&body_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&status_label));

    punch_region_init_border(&border_top, PUNCH_REGION_X - 2, PUNCH_REGION_Y - 2, PUNCH_REGION_W + 4, 2);
    punch_region_init_border(&border_bottom, PUNCH_REGION_X - 2, PUNCH_REGION_Y + PUNCH_REGION_H, PUNCH_REGION_W + 4, 2);
    punch_region_init_border(&border_left, PUNCH_REGION_X - 2, PUNCH_REGION_Y, 2, PUNCH_REGION_H);
    punch_region_init_border(&border_right, PUNCH_REGION_X + PUNCH_REGION_W, PUNCH_REGION_Y, 2, PUNCH_REGION_H);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_group));
    egui_core_set_punch_region(core, PUNCH_REGION_X, PUNCH_REGION_Y, PUNCH_REGION_W, PUNCH_REGION_H);
    egui_timer_start_timer(core, &render_timer, 1u, 0u);
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static void punch_region_complete_render(void)
{
    if (!s_placeholder_drawn)
    {
        punch_region_draw_placeholder();
        s_placeholder_drawn = 1u;
    }

    while (punch_region_draw_next_batch())
    {
    }
}

static void punch_region_report_runtime_failure(const char *message)
{
    if (s_runtime_fail_reported)
    {
        return;
    }

    s_runtime_fail_reported = 1u;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static void punch_region_runtime_verify(void)
{
    if (s_next_row < PUNCH_REGION_H)
    {
        punch_region_report_runtime_failure("punch region direct render did not finish");
    }
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 120);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 2:
        if (first_call)
        {
            punch_region_complete_render();
            punch_region_runtime_verify();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    default:
        return false;
    }
}
#endif
