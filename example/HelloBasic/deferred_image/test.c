#include <stdio.h>
#include <string.h>

#include "egui.h"
#include "uicode.h"

#include "decoder_bmp_stream.h"
#include "file_io_stdio.h"

#define DEMO_TITLE_FONT      ((const egui_font_t *)&egui_res_font_montserrat_16_4)
#define DEMO_SUBTITLE_FONT   ((const egui_font_t *)&egui_res_font_montserrat_12_4)
#define DEMO_CARD_TITLE_FONT ((const egui_font_t *)&egui_res_font_montserrat_12_4)
#define DEMO_CARD_BODY_FONT  ((const egui_font_t *)&egui_res_font_montserrat_10_4)

#define CARD_X       8
#define CARD_W       224
#define CARD_H       72
#define CARD_GAP     8
#define CARD_IMAGE_X 16
#define CARD_IMAGE_Y 7
#define CARD_IMAGE_W 72
#define CARD_IMAGE_H 58
#define CARD_TEXT_X  100
#define CARD_TEXT_W  116
#define CARD0_Y      72
#define CARD1_Y      (CARD0_Y + CARD_H + CARD_GAP)
#define CARD2_Y      (CARD1_Y + CARD_H + CARD_GAP)

#define DEFERRED_IMAGE_CACHE_BADGE "sample_badge.bmp"
#define DEFERRED_IMAGE_SOURCE_FAST "mock://badge-fast"
#define DEFERRED_IMAGE_SOURCE_SLOW "mock://badge-slow"
#define DEFERRED_IMAGE_SOURCE_FAIL "mock://badge-fail"

typedef struct deferred_image_mock_request
{
    uint8_t remaining_polls;
    uint8_t terminal_result;
} deferred_image_mock_request_t;

typedef struct deferred_image_demo_card
{
    egui_view_t panel;
    egui_view_deferred_image_t image;
    egui_view_label_t title;
    egui_view_label_t body;
} deferred_image_demo_card_t;

static egui_view_group_t root_group;
static egui_view_label_t title_label;
static egui_view_label_t subtitle_label;
static deferred_image_demo_card_t fast_card;
static deferred_image_demo_card_t slow_card;
static deferred_image_demo_card_t fail_card;

static file_image_stdio_context_t deferred_image_file_ctx = {
        .root_prefix = "example/HelloBasic/file_image/files/",
};
static egui_image_file_io_t deferred_image_file_io;

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

EGUI_VIEW_GROUP_PARAMS_INIT(root_group_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_root_param, EGUI_COLOR_HEX(0x0F172A), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_root_params, &bg_root_param, &bg_root_param, &bg_root_param);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_root, &bg_root_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_card_param, EGUI_COLOR_HEX(0xF8FAFC), EGUI_ALPHA_100, 10, 1, EGUI_COLOR_HEX(0xCBD5E1),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_card_params, &bg_card_param, &bg_card_param, &bg_card_param);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_card, &bg_card_params);

static void deferred_image_demo_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const char *text,
                                           const egui_font_t *font, egui_color_t color, uint8_t align_type)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), width, height);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_text(EGUI_VIEW_OF(label), text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), align_type);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static int deferred_image_mock_loader_parse_request(const char *source_uri, deferred_image_mock_request_t *request)
{
    if (source_uri == NULL || request == NULL)
    {
        return 0;
    }

    if (strcmp(source_uri, DEFERRED_IMAGE_SOURCE_FAST) == 0)
    {
        request->remaining_polls = 1u;
        request->terminal_result = (uint8_t)EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_SUCCESS;
        return 1;
    }
    if (strcmp(source_uri, DEFERRED_IMAGE_SOURCE_SLOW) == 0)
    {
        request->remaining_polls = 4u;
        request->terminal_result = (uint8_t)EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_SUCCESS;
        return 1;
    }
    if (strcmp(source_uri, DEFERRED_IMAGE_SOURCE_FAIL) == 0)
    {
        request->remaining_polls = 2u;
        request->terminal_result = (uint8_t)EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_FAILED;
        return 1;
    }

    return 0;
}

static int deferred_image_mock_loader_start(void *user_data, const char *source_uri, const char *cache_path, void **request_handle)
{
    deferred_image_mock_request_t *request;

    EGUI_UNUSED(user_data);
    if (cache_path == NULL || cache_path[0] == '\0' || request_handle == NULL)
    {
        return 0;
    }

    request = (deferred_image_mock_request_t *)egui_malloc(sizeof(*request));
    if (request == NULL)
    {
        return 0;
    }

    if (!deferred_image_mock_loader_parse_request(source_uri, request))
    {
        egui_free(request);
        return 0;
    }

    *request_handle = request;
    return 1;
}

static egui_view_deferred_image_loader_poll_result_t deferred_image_mock_loader_poll(void *user_data, void *request_handle)
{
    deferred_image_mock_request_t *request = (deferred_image_mock_request_t *)request_handle;

    EGUI_UNUSED(user_data);
    if (request == NULL)
    {
        return EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_FAILED;
    }

    if (request->remaining_polls > 0u)
    {
        request->remaining_polls--;
        return EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_PENDING;
    }

    return (egui_view_deferred_image_loader_poll_result_t)request->terminal_result;
}

static void deferred_image_mock_loader_cancel(void *user_data, void *request_handle)
{
    EGUI_UNUSED(user_data);
    if (request_handle != NULL)
    {
        egui_free(request_handle);
    }
}

static const egui_view_deferred_image_loader_t deferred_image_loader = {
        .user_data = NULL,
        .start = deferred_image_mock_loader_start,
        .poll = deferred_image_mock_loader_poll,
        .cancel = deferred_image_mock_loader_cancel,
};

static void deferred_image_demo_init_card(deferred_image_demo_card_t *card, egui_dim_t y, const char *title, const char *body, const char *source_uri,
                                          const char *cache_path, uint16_t delay_ms)
{
    egui_view_init(EGUI_VIEW_OF(&card->panel));
    egui_view_set_position(EGUI_VIEW_OF(&card->panel), CARD_X, y);
    egui_view_set_size(EGUI_VIEW_OF(&card->panel), CARD_W, CARD_H);
    egui_view_set_background(EGUI_VIEW_OF(&card->panel), EGUI_BG_OF(&bg_card));

    egui_view_deferred_image_init(EGUI_VIEW_OF(&card->image));
    egui_view_set_position(EGUI_VIEW_OF(&card->image), CARD_X + CARD_IMAGE_X, y + CARD_IMAGE_Y);
    egui_view_set_size(EGUI_VIEW_OF(&card->image), CARD_IMAGE_W, CARD_IMAGE_H);
    egui_view_deferred_image_set_loader(EGUI_VIEW_OF(&card->image), &deferred_image_loader);
    egui_view_deferred_image_set_cache_path(EGUI_VIEW_OF(&card->image), cache_path);
    egui_view_deferred_image_set_source_uri(EGUI_VIEW_OF(&card->image), source_uri);
    egui_view_deferred_image_set_load_delay_ms(EGUI_VIEW_OF(&card->image), delay_ms);

    deferred_image_demo_init_label(&card->title, CARD_X + CARD_TEXT_X, y + 16, CARD_TEXT_W, 16, title, DEMO_CARD_TITLE_FONT, EGUI_COLOR_HEX(0x0F172A),
                                   EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    deferred_image_demo_init_label(&card->body, CARD_X + CARD_TEXT_X, y + 38, CARD_TEXT_W, 14, body, DEMO_CARD_BODY_FONT, EGUI_COLOR_HEX(0x475569),
                                   EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);

    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&card->panel));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&card->image));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&card->title));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&card->body));
}

#if EGUI_CONFIG_RECORDING_TEST
static void deferred_image_report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1u;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static void deferred_image_runtime_verify(void)
{
    if (egui_view_deferred_image_get_status(EGUI_VIEW_OF(&fast_card.image)) != EGUI_VIEW_DEFERRED_IMAGE_STATUS_READY)
    {
        deferred_image_report_runtime_failure("fast card did not reach ready");
        return;
    }
    if (egui_view_deferred_image_get_status(EGUI_VIEW_OF(&slow_card.image)) != EGUI_VIEW_DEFERRED_IMAGE_STATUS_READY)
    {
        deferred_image_report_runtime_failure("slow card did not reach ready");
        return;
    }
    if (egui_view_deferred_image_get_status(EGUI_VIEW_OF(&fail_card.image)) != EGUI_VIEW_DEFERRED_IMAGE_STATUS_FAILED)
    {
        deferred_image_report_runtime_failure("fail card did not stay in failed state");
        return;
    }
    if (egui_image_file_get_status(&fast_card.image.loaded_image) != EGUI_IMAGE_FILE_STATUS_READY)
    {
        deferred_image_report_runtime_failure("fast card cache image not ready");
        return;
    }
    if (egui_image_file_get_status(&slow_card.image.loaded_image) != EGUI_IMAGE_FILE_STATUS_READY)
    {
        deferred_image_report_runtime_failure("slow card cache image not ready");
    }
}
#endif

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0u;
#endif

    egui_view_group_init_with_params(EGUI_VIEW_OF(&root_group), &root_group_params);
    egui_view_set_background(EGUI_VIEW_OF(&root_group), EGUI_BG_OF(&bg_root));

    file_image_stdio_io_init(&deferred_image_file_io, &deferred_image_file_ctx);
    egui_image_file_set_default_io(&deferred_image_file_io);
    egui_image_file_clear_decoders();
    EGUI_ASSERT(egui_image_file_register_decoder(&g_file_image_bmp_stream_decoder));

    deferred_image_demo_init_label(&title_label, 8, 12, 224, 20, "Deferred Image", DEMO_TITLE_FONT, EGUI_COLOR_HEX(0xF8FAFC),
                                   EGUI_ALIGN_CENTER | EGUI_ALIGN_VCENTER);
    deferred_image_demo_init_label(&subtitle_label, 12, 38, 216, 20, "Text first. Images join later.", DEMO_SUBTITLE_FONT, EGUI_COLOR_HEX(0xCBD5E1),
                                   EGUI_ALIGN_CENTER | EGUI_ALIGN_VCENTER);

    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&title_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&subtitle_label));

    deferred_image_demo_init_card(&fast_card, CARD0_Y, "Fast card", "160ms / 1 poll", DEFERRED_IMAGE_SOURCE_FAST, DEFERRED_IMAGE_CACHE_BADGE, 160u);
    deferred_image_demo_init_card(&slow_card, CARD1_Y, "Late card", "900ms / late load", DEFERRED_IMAGE_SOURCE_SLOW, DEFERRED_IMAGE_CACHE_BADGE, 900u);
    deferred_image_demo_init_card(&fail_card, CARD2_Y, "Fail safe", "650ms / fail safe", DEFERRED_IMAGE_SOURCE_FAIL, DEFERRED_IMAGE_CACHE_BADGE, 650u);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_group));
}

#if EGUI_CONFIG_RECORDING_TEST
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
        EGUI_SIM_SET_WAIT(p_action, 1350);
        return true;
    case 2:
        if (first_call)
        {
            deferred_image_runtime_verify();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    default:
        return false;
    }
}
#endif
