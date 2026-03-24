#include "egui.h"

#include <stdio.h>
#include <string.h>

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#include "app_egui_resource_generate.h"
#include "uicode.h"

#define BASIC_STAGE_NODE_COUNT 12U
#define BASIC_DEVICE_COUNT     3U
#define BASIC_BUTTON_TEXT_LEN  24U
#define BASIC_LIVE_SLOT_LIMIT  8U

#define BASIC_FONT_BODY ((const egui_font_t *)&egui_res_font_montserrat_10_4)
#define BASIC_FONT_META ((const egui_font_t *)&egui_res_font_montserrat_8_4)

#define BASIC_CANVAS_WIDTH  HELLO_VIRTUAL_STAGE_BASIC_CANVAS_WIDTH
#define BASIC_CANVAS_HEIGHT HELLO_VIRTUAL_STAGE_BASIC_CANVAS_HEIGHT

#define BASIC_SCREEN_MARGIN_X 8
#define BASIC_SCREEN_MARGIN_Y 8

#define BASIC_STAGE_X BASIC_SCREEN_MARGIN_X
#define BASIC_STAGE_Y BASIC_SCREEN_MARGIN_Y
#define BASIC_STAGE_W (BASIC_CANVAS_WIDTH - BASIC_SCREEN_MARGIN_X * 2)
#define BASIC_STAGE_H (BASIC_CANVAS_HEIGHT - BASIC_STAGE_Y - BASIC_SCREEN_MARGIN_Y)

enum
{
    BASIC_DEVICE_PUMP = 0,
    BASIC_DEVICE_VALVE,
    BASIC_DEVICE_FAN,
};

enum
{
    BASIC_VIEW_TYPE_IMAGE = 1,
    BASIC_VIEW_TYPE_PROGRESS,
    BASIC_VIEW_TYPE_BUTTON,
    BASIC_VIEW_TYPE_COMBOBOX,
};

enum
{
    BASIC_NODE_PUMP_IMAGE = 100,
    BASIC_NODE_PUMP_PROGRESS = 101,
    BASIC_NODE_PUMP_BUTTON = 102,
    BASIC_NODE_VALVE_IMAGE = 200,
    BASIC_NODE_VALVE_PROGRESS = 201,
    BASIC_NODE_VALVE_BUTTON = 202,
    BASIC_NODE_FAN_IMAGE = 300,
    BASIC_NODE_FAN_PROGRESS = 301,
    BASIC_NODE_FAN_BUTTON = 302,
    BASIC_NODE_MODE_COMBOBOX = 400,
    BASIC_NODE_PIN_PUMP_BUTTON = 401,
    BASIC_NODE_RESET_BUTTON = 402,
};

typedef struct basic_stage_node basic_stage_node_t;
typedef struct basic_stage_context basic_stage_context_t;
typedef struct basic_image_view basic_image_view_t;
typedef struct basic_progress_view basic_progress_view_t;
typedef struct basic_button_view basic_button_view_t;
typedef struct basic_combobox_view basic_combobox_view_t;

struct basic_stage_node
{
    egui_virtual_stage_node_desc_t desc;
};

struct basic_image_view
{
    egui_view_image_t image;
};

struct basic_progress_view
{
    egui_view_progress_bar_t progress;
};

struct basic_button_view
{
    egui_view_button_t button;
    char text[BASIC_BUTTON_TEXT_LEN];
};

struct basic_combobox_view
{
    egui_view_combobox_t combobox;
};

struct basic_stage_context
{
    basic_stage_node_t nodes[BASIC_STAGE_NODE_COUNT];
    uint8_t device_enabled[BASIC_DEVICE_COUNT];
    uint8_t device_progress[BASIC_DEVICE_COUNT];
    uint8_t mode_index;
    uint8_t pin_pump_enabled;
    uint32_t last_clicked_stable_id;
    uint16_t click_count;
};

static const char *basic_mode_items[] = {"Auto", "Boost", "Inspect"};
static const char *basic_device_names[] = {"Pump", "Valve", "Fan"};

static egui_view_canvas_panner_t basic_root;
static egui_view_virtual_stage_t basic_stage_view;
static basic_stage_context_t basic_ctx;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(basic_screen_bg_param, EGUI_COLOR_HEX(0xEEF3F6), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_screen_bg_params, &basic_screen_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_screen_bg, &basic_screen_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_stage_bg_param, EGUI_COLOR_HEX(0xF7FAFC), EGUI_ALPHA_100, 16, 1, EGUI_COLOR_HEX(0xB8CAD6),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_stage_bg_params, &basic_stage_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_stage_bg, &basic_stage_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_button_idle_param, EGUI_COLOR_HEX(0xDCE5EB), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xA9BBC8),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_button_idle_params, &basic_button_idle_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_button_idle_bg, &basic_button_idle_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_button_blue_param, EGUI_COLOR_HEX(0x3E739A), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0x2A5572),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_button_blue_params, &basic_button_blue_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_button_blue_bg, &basic_button_blue_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_button_green_param, EGUI_COLOR_HEX(0x338563), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0x246349),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_button_green_params, &basic_button_green_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_button_green_bg, &basic_button_green_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_button_orange_param, EGUI_COLOR_HEX(0xD28A3C), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xA96B23),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_button_orange_params, &basic_button_orange_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_button_orange_bg, &basic_button_orange_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_button_teal_param, EGUI_COLOR_HEX(0x167C88), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0x0F616A),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_button_teal_params, &basic_button_teal_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_button_teal_bg, &basic_button_teal_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(basic_button_red_param, EGUI_COLOR_HEX(0xC66154), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0x9C4437),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(basic_button_red_params, &basic_button_red_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(basic_button_red_bg, &basic_button_red_params);

static const char *basic_get_mode_text(void)
{
    if (basic_ctx.mode_index >= EGUI_ARRAY_SIZE(basic_mode_items))
    {
        basic_ctx.mode_index = 0U;
    }

    return basic_mode_items[basic_ctx.mode_index];
}

static egui_view_t *basic_find_live_view(uint32_t stable_id)
{
    return EGUI_VIEW_VIRTUAL_STAGE_FIND_VIEW_BY_ID(&basic_stage_view, stable_id);
}

static uint8_t basic_get_device_index_by_stable_id(uint32_t stable_id)
{
    switch (stable_id)
    {
    case BASIC_NODE_PUMP_IMAGE:
    case BASIC_NODE_PUMP_PROGRESS:
    case BASIC_NODE_PUMP_BUTTON:
        return BASIC_DEVICE_PUMP;
    case BASIC_NODE_VALVE_IMAGE:
    case BASIC_NODE_VALVE_PROGRESS:
    case BASIC_NODE_VALVE_BUTTON:
        return BASIC_DEVICE_VALVE;
    case BASIC_NODE_FAN_IMAGE:
    case BASIC_NODE_FAN_PROGRESS:
    case BASIC_NODE_FAN_BUTTON:
        return BASIC_DEVICE_FAN;
    default:
        return BASIC_DEVICE_COUNT;
    }
}

static uint32_t basic_get_image_node_by_device(uint8_t device_index)
{
    switch (device_index)
    {
    case BASIC_DEVICE_PUMP:
        return BASIC_NODE_PUMP_IMAGE;
    case BASIC_DEVICE_VALVE:
        return BASIC_NODE_VALVE_IMAGE;
    default:
        return BASIC_NODE_FAN_IMAGE;
    }
}

static uint32_t basic_get_progress_node_by_device(uint8_t device_index)
{
    switch (device_index)
    {
    case BASIC_DEVICE_PUMP:
        return BASIC_NODE_PUMP_PROGRESS;
    case BASIC_DEVICE_VALVE:
        return BASIC_NODE_VALVE_PROGRESS;
    default:
        return BASIC_NODE_FAN_PROGRESS;
    }
}

static uint32_t basic_get_button_node_by_device(uint8_t device_index)
{
    switch (device_index)
    {
    case BASIC_DEVICE_PUMP:
        return BASIC_NODE_PUMP_BUTTON;
    case BASIC_DEVICE_VALVE:
        return BASIC_NODE_VALVE_BUTTON;
    default:
        return BASIC_NODE_FAN_BUTTON;
    }
}

static egui_color_t basic_get_device_color(uint8_t device_index)
{
    switch (device_index)
    {
    case BASIC_DEVICE_PUMP:
        return EGUI_COLOR_HEX(0x3E739A);
    case BASIC_DEVICE_VALVE:
        return EGUI_COLOR_HEX(0x338563);
    default:
        return EGUI_COLOR_HEX(0xD28A3C);
    }
}

static egui_color_t basic_get_device_dim_color(uint8_t device_index)
{
    switch (device_index)
    {
    case BASIC_DEVICE_PUMP:
        return EGUI_COLOR_HEX(0x5F88A7);
    case BASIC_DEVICE_VALVE:
        return EGUI_COLOR_HEX(0x5D8E76);
    default:
        return EGUI_COLOR_HEX(0xB18451);
    }
}

static uint8_t basic_get_mode_step(void)
{
    switch (basic_ctx.mode_index)
    {
    case 1:
        return 20U;
    case 2:
        return 8U;
    default:
        return 12U;
    }
}

static void basic_mark_clicked(uint32_t stable_id)
{
    basic_ctx.last_clicked_stable_id = stable_id;
    basic_ctx.click_count++;
}

static void basic_notify_device_nodes(uint8_t device_index)
{
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(&basic_stage_view, basic_get_image_node_by_device(device_index), basic_get_progress_node_by_device(device_index),
                                       basic_get_button_node_by_device(device_index));
}

static void basic_apply_default_state(void)
{
    memset(basic_ctx.device_enabled, 0, sizeof(basic_ctx.device_enabled));
    memset(basic_ctx.device_progress, 0, sizeof(basic_ctx.device_progress));

    basic_ctx.device_enabled[BASIC_DEVICE_VALVE] = 1U;
    basic_ctx.device_progress[BASIC_DEVICE_PUMP] = 28U;
    basic_ctx.device_progress[BASIC_DEVICE_VALVE] = 64U;
    basic_ctx.device_progress[BASIC_DEVICE_FAN] = 46U;
    basic_ctx.mode_index = 0U;
    basic_ctx.pin_pump_enabled = 0U;
    basic_ctx.last_clicked_stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
    basic_ctx.click_count = 0U;
}

static void basic_reset_business_state(void)
{
    basic_apply_default_state();
    EGUI_VIEW_VIRTUAL_STAGE_UNPIN(&basic_stage_view, BASIC_NODE_PUMP_IMAGE);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_DATA(&basic_stage_view);
    EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(&basic_stage_view);
}

static void basic_advance_device_progress(uint8_t device_index, uint8_t step)
{
    uint16_t next_value;

    if (device_index >= BASIC_DEVICE_COUNT)
    {
        return;
    }

    next_value = (uint16_t)(basic_ctx.device_progress[device_index] + step);
    if (next_value > 100U)
    {
        next_value = step;
    }

    basic_ctx.device_progress[device_index] = (uint8_t)next_value;
    basic_ctx.device_enabled[device_index] = 1U;
}

static void basic_init_node(uint32_t index, uint32_t stable_id, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, uint16_t view_type,
                            uint8_t flags)
{
    basic_stage_node_t *node = &basic_ctx.nodes[index];

    memset(node, 0, sizeof(*node));
    node->desc.region.location.x = x;
    node->desc.region.location.y = y;
    node->desc.region.size.width = width;
    node->desc.region.size.height = height;
    node->desc.stable_id = stable_id;
    node->desc.view_type = view_type;
    node->desc.flags = flags;
}

static void basic_init_nodes(void)
{
    basic_init_node(0, BASIC_NODE_PUMP_IMAGE, 20, 24, 54, 54, BASIC_VIEW_TYPE_IMAGE,
                    EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE);
    basic_init_node(1, BASIC_NODE_PUMP_PROGRESS, 96, 42, 176, 18, BASIC_VIEW_TYPE_PROGRESS,
                    EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE);
    basic_init_node(2, BASIC_NODE_PUMP_BUTTON, 298, 34, 144, 34, BASIC_VIEW_TYPE_BUTTON,
                    EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE);
    basic_init_node(3, BASIC_NODE_VALVE_IMAGE, 20, 104, 54, 54, BASIC_VIEW_TYPE_IMAGE, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    basic_init_node(4, BASIC_NODE_VALVE_PROGRESS, 96, 122, 176, 18, BASIC_VIEW_TYPE_PROGRESS, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    basic_init_node(5, BASIC_NODE_VALVE_BUTTON, 298, 114, 144, 34, BASIC_VIEW_TYPE_BUTTON, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    basic_init_node(6, BASIC_NODE_FAN_IMAGE, 20, 184, 54, 54, BASIC_VIEW_TYPE_IMAGE, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    basic_init_node(7, BASIC_NODE_FAN_PROGRESS, 96, 202, 176, 18, BASIC_VIEW_TYPE_PROGRESS, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    basic_init_node(8, BASIC_NODE_FAN_BUTTON, 298, 194, 144, 34, BASIC_VIEW_TYPE_BUTTON, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    basic_init_node(9, BASIC_NODE_MODE_COMBOBOX, 20, 252, 176, 34, BASIC_VIEW_TYPE_COMBOBOX,
                    EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE | EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE);
    basic_init_node(10, BASIC_NODE_PIN_PUMP_BUTTON, 212, 252, 112, 34, BASIC_VIEW_TYPE_BUTTON, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
    basic_init_node(11, BASIC_NODE_RESET_BUTTON, 340, 252, 108, 34, BASIC_VIEW_TYPE_BUTTON, EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE);
}

static void basic_make_local_region(const egui_region_t *screen_region, egui_region_t *local_region)
{
    local_region->location.x = 0;
    local_region->location.y = 0;
    local_region->size = screen_region->size;
}

static egui_background_t *basic_get_button_background(uint32_t stable_id)
{
    uint8_t device_index = basic_get_device_index_by_stable_id(stable_id);

    if (device_index < BASIC_DEVICE_COUNT)
    {
        if (!basic_ctx.device_enabled[device_index])
        {
            return EGUI_BG_OF(&basic_button_idle_bg);
        }

        switch (device_index)
        {
        case BASIC_DEVICE_PUMP:
            return EGUI_BG_OF(&basic_button_blue_bg);
        case BASIC_DEVICE_VALVE:
            return EGUI_BG_OF(&basic_button_green_bg);
        default:
            return EGUI_BG_OF(&basic_button_orange_bg);
        }
    }

    if (stable_id == BASIC_NODE_PIN_PUMP_BUTTON)
    {
        return basic_ctx.pin_pump_enabled ? EGUI_BG_OF(&basic_button_teal_bg) : EGUI_BG_OF(&basic_button_idle_bg);
    }

    if (stable_id == BASIC_NODE_RESET_BUTTON)
    {
        return EGUI_BG_OF(&basic_button_red_bg);
    }

    return EGUI_BG_OF(&basic_button_idle_bg);
}

static egui_color_t basic_get_button_text_color(uint32_t stable_id)
{
    uint8_t device_index = basic_get_device_index_by_stable_id(stable_id);

    if (device_index < BASIC_DEVICE_COUNT)
    {
        return basic_ctx.device_enabled[device_index] ? EGUI_COLOR_WHITE : EGUI_COLOR_HEX(0x284153);
    }

    if (stable_id == BASIC_NODE_PIN_PUMP_BUTTON)
    {
        return basic_ctx.pin_pump_enabled ? EGUI_COLOR_WHITE : EGUI_COLOR_HEX(0x284153);
    }

    return stable_id == BASIC_NODE_RESET_BUTTON ? EGUI_COLOR_WHITE : EGUI_COLOR_HEX(0x284153);
}

static void basic_format_button_text(uint32_t stable_id, char *dst, size_t dst_size)
{
    uint8_t device_index = basic_get_device_index_by_stable_id(stable_id);

    if (device_index < BASIC_DEVICE_COUNT)
    {
        snprintf(dst, dst_size, "%s %s", basic_device_names[device_index], basic_ctx.device_enabled[device_index] ? "On" : "Off");
        return;
    }

    if (stable_id == BASIC_NODE_PIN_PUMP_BUTTON)
    {
        snprintf(dst, dst_size, "%s Pump", basic_ctx.pin_pump_enabled ? "Unpin" : "Pin");
        return;
    }

    if (stable_id == BASIC_NODE_RESET_BUTTON)
    {
        snprintf(dst, dst_size, "Reset");
        return;
    }

    snprintf(dst, dst_size, "Action");
}

static void basic_draw_button_preview(const egui_region_t *screen_region, uint32_t stable_id)
{
    char text[BASIC_BUTTON_TEXT_LEN];
    egui_region_t text_region = *screen_region;
    egui_color_t fill;
    egui_color_t border;

    basic_format_button_text(stable_id, text, sizeof(text));

    switch (stable_id)
    {
    case BASIC_NODE_PUMP_BUTTON:
        fill = basic_ctx.device_enabled[BASIC_DEVICE_PUMP] ? EGUI_COLOR_HEX(0x3E739A) : EGUI_COLOR_HEX(0xDCE5EB);
        border = basic_ctx.device_enabled[BASIC_DEVICE_PUMP] ? EGUI_COLOR_HEX(0x2A5572) : EGUI_COLOR_HEX(0xA9BBC8);
        break;
    case BASIC_NODE_VALVE_BUTTON:
        fill = basic_ctx.device_enabled[BASIC_DEVICE_VALVE] ? EGUI_COLOR_HEX(0x338563) : EGUI_COLOR_HEX(0xDCE5EB);
        border = basic_ctx.device_enabled[BASIC_DEVICE_VALVE] ? EGUI_COLOR_HEX(0x246349) : EGUI_COLOR_HEX(0xA9BBC8);
        break;
    case BASIC_NODE_FAN_BUTTON:
        fill = basic_ctx.device_enabled[BASIC_DEVICE_FAN] ? EGUI_COLOR_HEX(0xD28A3C) : EGUI_COLOR_HEX(0xDCE5EB);
        border = basic_ctx.device_enabled[BASIC_DEVICE_FAN] ? EGUI_COLOR_HEX(0xA96B23) : EGUI_COLOR_HEX(0xA9BBC8);
        break;
    case BASIC_NODE_PIN_PUMP_BUTTON:
        fill = basic_ctx.pin_pump_enabled ? EGUI_COLOR_HEX(0x167C88) : EGUI_COLOR_HEX(0xDCE5EB);
        border = basic_ctx.pin_pump_enabled ? EGUI_COLOR_HEX(0x0F616A) : EGUI_COLOR_HEX(0xA9BBC8);
        break;
    default:
        fill = EGUI_COLOR_HEX(0xC66154);
        border = EGUI_COLOR_HEX(0x9C4437);
        break;
    }

    egui_canvas_draw_round_rectangle_fill(screen_region->location.x, screen_region->location.y, screen_region->size.width, screen_region->size.height, 10, fill,
                                          EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle(screen_region->location.x, screen_region->location.y, screen_region->size.width, screen_region->size.height, 10, 1, border,
                                     EGUI_ALPHA_100);
    egui_canvas_draw_text_in_rect(BASIC_FONT_BODY, text, &text_region, EGUI_ALIGN_CENTER, basic_get_button_text_color(stable_id), EGUI_ALPHA_100);
}

static void basic_draw_image_preview(const egui_region_t *screen_region, uint32_t stable_id)
{
    uint8_t device_index = basic_get_device_index_by_stable_id(stable_id);
    egui_color_t color;
    egui_alpha_t alpha;

    if (device_index >= BASIC_DEVICE_COUNT)
    {
        return;
    }

    color = basic_ctx.device_enabled[device_index] ? basic_get_device_color(device_index) : basic_get_device_dim_color(device_index);
    alpha = basic_ctx.device_enabled[device_index] ? EGUI_ALPHA_100 : EGUI_ALPHA_60;
    egui_canvas_draw_image_resize_color((const egui_image_t *)&egui_res_image_star_alpha_4, (egui_dim_t)(screen_region->location.x + 4),
                                        (egui_dim_t)(screen_region->location.y + 4), (egui_dim_t)(screen_region->size.width - 8),
                                        (egui_dim_t)(screen_region->size.height - 8), color, alpha);
}

static void basic_draw_progress_preview(const egui_region_t *screen_region, uint32_t stable_id)
{
    uint8_t device_index = basic_get_device_index_by_stable_id(stable_id);
    egui_dim_t track_y;
    egui_dim_t fill_w;

    if (device_index >= BASIC_DEVICE_COUNT)
    {
        return;
    }

    track_y = (egui_dim_t)(screen_region->location.y + (screen_region->size.height - 8) / 2);
    fill_w = (egui_dim_t)((screen_region->size.width * basic_ctx.device_progress[device_index]) / 100U);

    egui_canvas_draw_round_rectangle_fill(screen_region->location.x, track_y, screen_region->size.width, 8, 4, EGUI_COLOR_HEX(0xD5E0E8), EGUI_ALPHA_100);
    if (fill_w > 0)
    {
        egui_canvas_draw_round_rectangle_fill(screen_region->location.x, track_y, fill_w, 8, 4, basic_get_device_color(device_index), EGUI_ALPHA_100);
    }
}

static void basic_draw_combobox_preview(const egui_region_t *screen_region)
{
    egui_region_t value_region = *screen_region;
    egui_region_t arrow_region = *screen_region;

    egui_canvas_draw_round_rectangle_fill(screen_region->location.x, screen_region->location.y, screen_region->size.width, screen_region->size.height, 10,
                                          EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle(screen_region->location.x, screen_region->location.y, screen_region->size.width, screen_region->size.height, 10, 1,
                                     EGUI_COLOR_HEX(0x8FA4B1), EGUI_ALPHA_100);

    value_region.location.x += 10;
    value_region.location.y += 8;
    value_region.size.width -= 34;
    value_region.size.height = 16;
    egui_canvas_draw_text_in_rect(BASIC_FONT_BODY, basic_get_mode_text(), &value_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, EGUI_COLOR_HEX(0x22384B),
                                  EGUI_ALPHA_100);

    arrow_region.location.x = (egui_dim_t)(screen_region->location.x + screen_region->size.width - 24);
    arrow_region.location.y += 7;
    arrow_region.size.width = 16;
    arrow_region.size.height = 18;
    egui_canvas_draw_text_in_rect(EGUI_FONT_ICON_MS_16, EGUI_ICON_MS_EXPAND_MORE, &arrow_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x557082), EGUI_ALPHA_100);
}

static void basic_image_click_cb(egui_view_t *self)
{
    uint32_t stable_id;
    uint8_t device_index;

    if (!EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_ID_BY_VIEW(&basic_stage_view, self, &stable_id))
    {
        return;
    }

    device_index = basic_get_device_index_by_stable_id(stable_id);
    if (device_index >= BASIC_DEVICE_COUNT)
    {
        return;
    }

    basic_mark_clicked(stable_id);
    basic_advance_device_progress(device_index, (uint8_t)(8U + device_index * 2U));
    basic_notify_device_nodes(device_index);
}

static void basic_progress_click_cb(egui_view_t *self)
{
    uint32_t stable_id;
    uint8_t device_index;

    if (!EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_ID_BY_VIEW(&basic_stage_view, self, &stable_id))
    {
        return;
    }

    device_index = basic_get_device_index_by_stable_id(stable_id);
    if (device_index >= BASIC_DEVICE_COUNT)
    {
        return;
    }

    basic_mark_clicked(stable_id);
    basic_advance_device_progress(device_index, basic_get_mode_step());
    basic_notify_device_nodes(device_index);
}

static void basic_button_click_cb(egui_view_t *self)
{
    uint32_t stable_id;
    uint8_t device_index;

    if (!EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_ID_BY_VIEW(&basic_stage_view, self, &stable_id))
    {
        return;
    }

    device_index = basic_get_device_index_by_stable_id(stable_id);
    if (device_index < BASIC_DEVICE_COUNT)
    {
        basic_mark_clicked(stable_id);
        basic_ctx.device_enabled[device_index] = basic_ctx.device_enabled[device_index] ? 0U : 1U;
        basic_notify_device_nodes(device_index);
        return;
    }

    if (stable_id == BASIC_NODE_PIN_PUMP_BUTTON)
    {
        basic_mark_clicked(stable_id);
        basic_ctx.pin_pump_enabled = EGUI_VIEW_VIRTUAL_STAGE_TOGGLE_PIN(&basic_stage_view, BASIC_NODE_PUMP_IMAGE);
        EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(&basic_stage_view, BASIC_NODE_PIN_PUMP_BUTTON, BASIC_NODE_PUMP_IMAGE);
        EGUI_VIEW_VIRTUAL_STAGE_CALCULATE_LAYOUT(&basic_stage_view);
        return;
    }

    if (stable_id == BASIC_NODE_RESET_BUTTON)
    {
        basic_reset_business_state();
        basic_ctx.last_clicked_stable_id = BASIC_NODE_RESET_BUTTON;
        basic_ctx.click_count = 1U;
    }
}

static void basic_combobox_selected(egui_view_t *self, uint8_t index)
{
    uint32_t stable_id;

    if (!EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_ID_BY_VIEW(&basic_stage_view, self, &stable_id))
    {
        return;
    }

    if (index >= EGUI_ARRAY_SIZE(basic_mode_items))
    {
        return;
    }

    basic_ctx.mode_index = index;
    basic_mark_clicked(stable_id);
    EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(&basic_stage_view, BASIC_NODE_MODE_COMBOBOX);
}

static egui_view_t *basic_adapter_create_view(void *user_context, uint16_t view_type)
{
    EGUI_UNUSED(user_context);

    switch (view_type)
    {
    case BASIC_VIEW_TYPE_IMAGE:
    {
        basic_image_view_t *image_view = (basic_image_view_t *)egui_malloc(sizeof(basic_image_view_t));
        if (image_view == NULL)
        {
            return NULL;
        }

        memset(image_view, 0, sizeof(*image_view));
        egui_view_image_init(EGUI_VIEW_OF(&image_view->image));
        egui_view_image_set_image_type(EGUI_VIEW_OF(&image_view->image), EGUI_VIEW_IMAGE_TYPE_RESIZE);
        egui_view_image_set_image(EGUI_VIEW_OF(&image_view->image), (egui_image_t *)&egui_res_image_star_alpha_4);
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&image_view->image), basic_image_click_cb);
        return EGUI_VIEW_OF(&image_view->image);
    }
    case BASIC_VIEW_TYPE_PROGRESS:
    {
        basic_progress_view_t *progress_view = (basic_progress_view_t *)egui_malloc(sizeof(basic_progress_view_t));
        if (progress_view == NULL)
        {
            return NULL;
        }

        memset(progress_view, 0, sizeof(*progress_view));
        egui_view_progress_bar_init(EGUI_VIEW_OF(&progress_view->progress));
        progress_view->progress.is_show_control = 0U;
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&progress_view->progress), basic_progress_click_cb);
        return EGUI_VIEW_OF(&progress_view->progress);
    }
    case BASIC_VIEW_TYPE_BUTTON:
    {
        basic_button_view_t *button_view = (basic_button_view_t *)egui_malloc(sizeof(basic_button_view_t));
        if (button_view == NULL)
        {
            return NULL;
        }

        memset(button_view, 0, sizeof(*button_view));
        egui_view_button_init(EGUI_VIEW_OF(&button_view->button));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_view->button), basic_button_click_cb);
        egui_view_label_set_font(EGUI_VIEW_OF(&button_view->button), BASIC_FONT_BODY);
        egui_view_label_set_align_type(EGUI_VIEW_OF(&button_view->button), EGUI_ALIGN_CENTER);
        egui_view_set_padding(EGUI_VIEW_OF(&button_view->button), 8, 8, 5, 5);
        return EGUI_VIEW_OF(&button_view->button);
    }
    case BASIC_VIEW_TYPE_COMBOBOX:
    {
        basic_combobox_view_t *combobox_view = (basic_combobox_view_t *)egui_malloc(sizeof(basic_combobox_view_t));
        if (combobox_view == NULL)
        {
            return NULL;
        }

        memset(combobox_view, 0, sizeof(*combobox_view));
        egui_view_combobox_init(EGUI_VIEW_OF(&combobox_view->combobox));
        egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&combobox_view->combobox), basic_combobox_selected);
        egui_view_combobox_set_font(EGUI_VIEW_OF(&combobox_view->combobox), BASIC_FONT_BODY);
        egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&combobox_view->combobox), EGUI_FONT_ICON_MS_16);
        egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&combobox_view->combobox), EGUI_ICON_MS_EXPAND_MORE, EGUI_ICON_MS_EXPAND_LESS);
        egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&combobox_view->combobox), 2);
        return EGUI_VIEW_OF(&combobox_view->combobox);
    }
    default:
        return NULL;
    }
}

static void basic_adapter_destroy_view(void *user_context, egui_view_t *view, uint16_t view_type)
{
    EGUI_UNUSED(user_context);
    EGUI_UNUSED(view_type);
    egui_free(view);
}

static void basic_adapter_bind_view(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    basic_stage_context_t *ctx = (basic_stage_context_t *)user_context;
    uint8_t device_index;

    EGUI_UNUSED(index);

    switch (desc->view_type)
    {
    case BASIC_VIEW_TYPE_IMAGE:
        device_index = basic_get_device_index_by_stable_id(stable_id);
        if (device_index < BASIC_DEVICE_COUNT)
        {
            egui_view_image_set_image(EGUI_VIEW_OF(view), (egui_image_t *)&egui_res_image_star_alpha_4);
            egui_view_image_set_image_type(EGUI_VIEW_OF(view), EGUI_VIEW_IMAGE_TYPE_RESIZE);
            egui_view_image_set_image_color(EGUI_VIEW_OF(view),
                                            ctx->device_enabled[device_index] ? basic_get_device_color(device_index) : basic_get_device_dim_color(device_index),
                                            EGUI_ALPHA_100);
        }
        break;
    case BASIC_VIEW_TYPE_PROGRESS:
        device_index = basic_get_device_index_by_stable_id(stable_id);
        if (device_index < BASIC_DEVICE_COUNT)
        {
            egui_view_progress_bar_t *progress = (egui_view_progress_bar_t *)view;
            progress->bk_color = EGUI_COLOR_HEX(0xD5E0E8);
            progress->progress_color = basic_get_device_color(device_index);
            progress->control_color = basic_get_device_color(device_index);
            progress->is_show_control = 0U;
            egui_view_progress_bar_set_process(EGUI_VIEW_OF(view), ctx->device_progress[device_index]);
        }
        break;
    case BASIC_VIEW_TYPE_BUTTON:
    {
        basic_button_view_t *button_view = (basic_button_view_t *)view;

        basic_format_button_text(stable_id, button_view->text, sizeof(button_view->text));
        egui_view_label_set_text(EGUI_VIEW_OF(&button_view->button), button_view->text);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&button_view->button), basic_get_button_text_color(stable_id), EGUI_ALPHA_100);
        egui_view_set_background(EGUI_VIEW_OF(&button_view->button), basic_get_button_background(stable_id));
        break;
    }
    case BASIC_VIEW_TYPE_COMBOBOX:
        ((egui_view_combobox_t *)view)->collapsed_height = desc->region.size.height;
        egui_view_combobox_set_items(EGUI_VIEW_OF(view), basic_mode_items, EGUI_ARRAY_SIZE(basic_mode_items));
        egui_view_combobox_set_current_index(EGUI_VIEW_OF(view), ctx->mode_index);
        egui_view_combobox_collapse(EGUI_VIEW_OF(view));
        break;
    default:
        break;
    }
}

static uint8_t basic_adapter_hit_test(void *user_context, uint32_t index, const egui_virtual_stage_node_desc_t *desc, const egui_region_t *screen_region,
                                      egui_dim_t screen_x, egui_dim_t screen_y)
{
    egui_view_t *live_view;

    EGUI_UNUSED(user_context);
    EGUI_UNUSED(index);

    if (desc != NULL && desc->view_type == BASIC_VIEW_TYPE_COMBOBOX)
    {
        live_view = basic_find_live_view(desc->stable_id);
        if (live_view != NULL && egui_view_combobox_is_expanded(live_view))
        {
            return egui_region_pt_in_rect(&live_view->region_screen, screen_x, screen_y) ? 1U : 0U;
        }
    }

    return egui_region_pt_in_rect(screen_region, screen_x, screen_y) ? 1U : 0U;
}

static uint8_t basic_adapter_should_keep_alive(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id,
                                               const egui_virtual_stage_node_desc_t *desc)
{
    basic_stage_context_t *ctx = (basic_stage_context_t *)user_context;
    EGUI_UNUSED(index);

    if (desc != NULL && desc->view_type == BASIC_VIEW_TYPE_COMBOBOX)
    {
        if (egui_view_combobox_is_expanded(view))
        {
            return 1U;
        }
    }

    if (stable_id == BASIC_NODE_RESET_BUTTON)
    {
        return 0U;
    }

    if (ctx != NULL && ctx->last_clicked_stable_id == stable_id)
    {
        return 1U;
    }

    return 0U;
}

static void basic_adapter_draw_node(void *user_context, egui_view_t *page, uint32_t index, const egui_virtual_stage_node_desc_t *desc,
                                    const egui_region_t *screen_region)
{
    egui_region_t local_region;

    EGUI_UNUSED(user_context);
    EGUI_UNUSED(page);
    EGUI_UNUSED(index);

    basic_make_local_region(screen_region, &local_region);

    switch (desc->view_type)
    {
    case BASIC_VIEW_TYPE_IMAGE:
        basic_draw_image_preview(&local_region, desc->stable_id);
        break;
    case BASIC_VIEW_TYPE_PROGRESS:
        basic_draw_progress_preview(&local_region, desc->stable_id);
        break;
    case BASIC_VIEW_TYPE_BUTTON:
        basic_draw_button_preview(&local_region, desc->stable_id);
        break;
    case BASIC_VIEW_TYPE_COMBOBOX:
        basic_draw_combobox_preview(&local_region);
        break;
    default:
        break;
    }
}

EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(basic_stage_bridge, BASIC_STAGE_X, BASIC_STAGE_Y, BASIC_STAGE_W, BASIC_STAGE_H,
                                                                      BASIC_LIVE_SLOT_LIMIT, basic_ctx.nodes, basic_stage_node_t, desc,
                                                                      basic_adapter_create_view, basic_adapter_destroy_view, basic_adapter_bind_view,
                                                                      basic_adapter_draw_node, basic_adapter_hit_test, basic_adapter_should_keep_alive,
                                                                      &basic_ctx);

void test_init_ui(void)
{
    memset(&basic_ctx, 0, sizeof(basic_ctx));
#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0U;
#endif

    basic_apply_default_state();
    basic_init_nodes();

    egui_view_canvas_panner_init(EGUI_VIEW_OF(&basic_root));
    egui_view_set_size(EGUI_VIEW_OF(&basic_root), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_canvas_panner_set_canvas_size(EGUI_VIEW_OF(&basic_root), BASIC_CANVAS_WIDTH, BASIC_CANVAS_HEIGHT);
    egui_view_set_background(EGUI_VIEW_OF(&basic_root), EGUI_BG_OF(&basic_screen_bg));

    EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(&basic_stage_view, &basic_stage_bridge);
    EGUI_VIEW_VIRTUAL_STAGE_SET_BACKGROUND(&basic_stage_view, EGUI_BG_OF(&basic_stage_bg));

    egui_view_group_add_child(EGUI_VIEW_OF(&basic_root), EGUI_VIEW_OF(&basic_stage_view));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&basic_root));
}

#if EGUI_CONFIG_RECORDING_TEST
static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1U;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static basic_stage_node_t *basic_find_node(uint32_t stable_id)
{
    uint32_t i;

    for (i = 0; i < BASIC_STAGE_NODE_COUNT; i++)
    {
        if (basic_ctx.nodes[i].desc.stable_id == stable_id)
        {
            return &basic_ctx.nodes[i];
        }
    }

    return NULL;
}

static void basic_get_node_center(uint32_t stable_id, egui_dim_t *out_x, egui_dim_t *out_y)
{
    basic_stage_node_t *node = basic_find_node(stable_id);
    egui_region_t *stage_region = EGUI_VIEW_VIRTUAL_STAGE_SCREEN_REGION(&basic_stage_view);

    if (node == NULL)
    {
        *out_x = stage_region->location.x;
        *out_y = stage_region->location.y;
        return;
    }

    *out_x = (egui_dim_t)(stage_region->location.x + node->desc.region.location.x + node->desc.region.size.width / 2);
    *out_y = (egui_dim_t)(stage_region->location.y + node->desc.region.location.y + node->desc.region.size.height / 2);
}

static void basic_set_click_node_action(egui_sim_action_t *p_action, uint32_t stable_id, uint32_t interval_ms)
{
    egui_dim_t x;
    egui_dim_t y;

    basic_get_node_center(stable_id, &x, &y);
    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = x;
    p_action->y1 = y;
    p_action->interval_ms = interval_ms;
}

static void basic_set_select_mode_action(egui_sim_action_t *p_action, uint8_t item_offset, uint32_t interval_ms)
{
    egui_view_t *view = basic_find_live_view(BASIC_NODE_MODE_COMBOBOX);
    egui_view_combobox_t *combobox;

    if (view == NULL || !egui_view_combobox_is_expanded(view))
    {
        EGUI_SIM_SET_WAIT(p_action, interval_ms);
        return;
    }

    combobox = (egui_view_combobox_t *)view;
    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = (egui_dim_t)(view->region_screen.location.x + view->region_screen.size.width / 2);
    p_action->y1 = (egui_dim_t)(view->region_screen.location.y + combobox->collapsed_height + combobox->item_height * item_offset + combobox->item_height / 2);
    p_action->interval_ms = interval_ms;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    egui_view_t *view;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&basic_stage_view) != 4U)
            {
                report_runtime_failure("basic stage should start with four default live controls");
            }
            if (basic_find_live_view(BASIC_NODE_PUMP_IMAGE) == NULL || basic_find_live_view(BASIC_NODE_PUMP_PROGRESS) == NULL ||
                basic_find_live_view(BASIC_NODE_PUMP_BUTTON) == NULL || basic_find_live_view(BASIC_NODE_MODE_COMBOBOX) == NULL)
            {
                report_runtime_failure("basic stage did not materialize the default learning controls");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        basic_set_click_node_action(p_action, BASIC_NODE_PUMP_IMAGE, 260);
        return true;
    case 2:
        if (first_call)
        {
            if (basic_ctx.device_progress[BASIC_DEVICE_PUMP] <= 28U)
            {
                report_runtime_failure("pump image click did not update progress");
            }
            if (basic_find_live_view(BASIC_NODE_PUMP_IMAGE) == NULL)
            {
                report_runtime_failure("pump image did not materialize");
            }
        }
        basic_set_click_node_action(p_action, BASIC_NODE_MODE_COMBOBOX, 260);
        return true;
    case 3:
        if (first_call)
        {
            view = basic_find_live_view(BASIC_NODE_MODE_COMBOBOX);
            if (view == NULL)
            {
                report_runtime_failure("mode combobox did not materialize");
                basic_ctx.mode_index = 1U;
            }
            else if (!egui_view_combobox_is_expanded(view))
            {
                report_runtime_failure("mode combobox did not expand");
            }
            recording_request_snapshot();
        }
        basic_set_select_mode_action(p_action, 1U, 260);
        return true;
    case 4:
        if (first_call)
        {
            view = basic_find_live_view(BASIC_NODE_MODE_COMBOBOX);
            if (basic_ctx.mode_index != 1U)
            {
                report_runtime_failure("mode combobox did not change selection");
                basic_ctx.mode_index = 1U;
            }
            if (view != NULL && egui_view_combobox_is_expanded(view))
            {
                report_runtime_failure("mode combobox should collapse after selection");
            }
        }
        basic_set_click_node_action(p_action, BASIC_NODE_VALVE_PROGRESS, 260);
        return true;
    case 5:
        if (first_call)
        {
            if (basic_ctx.device_progress[BASIC_DEVICE_VALVE] <= 64U)
            {
                report_runtime_failure("valve progress click did not advance progress");
            }
            recording_request_snapshot();
        }
        basic_set_click_node_action(p_action, BASIC_NODE_FAN_BUTTON, 260);
        return true;
    case 6:
        if (first_call && !basic_ctx.device_enabled[BASIC_DEVICE_FAN])
        {
            report_runtime_failure("fan button did not toggle state");
        }
        basic_set_click_node_action(p_action, BASIC_NODE_PIN_PUMP_BUTTON, 260);
        return true;
    case 7:
        if (first_call)
        {
            if (!basic_ctx.pin_pump_enabled || basic_find_live_view(BASIC_NODE_PUMP_IMAGE) == NULL)
            {
                report_runtime_failure("pin pump did not retain image view");
            }
            recording_request_snapshot();
        }
        basic_set_click_node_action(p_action, BASIC_NODE_RESET_BUTTON, 260);
        return true;
    case 8:
        if (first_call)
        {
            if (basic_ctx.pin_pump_enabled || basic_ctx.mode_index != 0U || basic_ctx.device_progress[BASIC_DEVICE_PUMP] != 28U ||
                basic_ctx.device_progress[BASIC_DEVICE_VALVE] != 64U || basic_ctx.device_progress[BASIC_DEVICE_FAN] != 46U ||
                basic_ctx.device_enabled[BASIC_DEVICE_PUMP] != 0U || basic_ctx.device_enabled[BASIC_DEVICE_VALVE] != 1U ||
                basic_ctx.device_enabled[BASIC_DEVICE_FAN] != 0U)
            {
                report_runtime_failure("reset did not restore default stage state");
            }
            if (EGUI_VIEW_VIRTUAL_STAGE_SLOT_COUNT(&basic_stage_view) != 4U)
            {
                report_runtime_failure("reset did not restore the default live controls");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    default:
        return false;
    }
}
#endif
