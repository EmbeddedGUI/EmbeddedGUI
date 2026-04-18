#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_deferred_image.h"

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE

typedef struct test_deferred_image_decoder_ctx
{
    int dummy;
} test_deferred_image_decoder_ctx_t;

typedef struct test_deferred_image_request
{
    uint8_t remaining_polls;
    uint8_t terminal_result;
} test_deferred_image_request_t;

typedef struct test_deferred_image_loader_state
{
    int start_count;
    int poll_count;
    int cancel_count;
    int next_start_success;
    uint8_t polls_until_terminal;
    egui_view_deferred_image_loader_poll_result_t terminal_result;
    const char *last_source_uri;
    const char *last_cache_path;
} test_deferred_image_loader_state_t;

static const uint16_t s_test_deferred_image_pixels[2][4] = {
        {0x001F, 0x07E0, 0xF800, 0xFFFF},
        {0x0000, 0x39E7, 0x7BEF, 0xFD20},
};
static int s_test_deferred_image_io_open_count;
static int s_test_deferred_image_decoder_open_count;
static int s_test_deferred_image_decoder_close_count;
static int s_test_deferred_image_read_row_count;
static test_deferred_image_decoder_ctx_t s_test_deferred_image_decoder_ctx;
static egui_view_deferred_image_t s_test_deferred_image_view;
static egui_image_t s_test_deferred_placeholder;
static test_deferred_image_loader_state_t s_test_deferred_image_loader_state;

static void test_deferred_image_reset_decoder_counters(void)
{
    s_test_deferred_image_io_open_count = 0;
    s_test_deferred_image_decoder_open_count = 0;
    s_test_deferred_image_decoder_close_count = 0;
    s_test_deferred_image_read_row_count = 0;
}

static void test_deferred_image_reset_loader_state(void)
{
    memset(&s_test_deferred_image_loader_state, 0, sizeof(s_test_deferred_image_loader_state));
    s_test_deferred_image_loader_state.next_start_success = 1;
    s_test_deferred_image_loader_state.terminal_result = EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_SUCCESS;
}

static void *test_deferred_image_mock_open(void *user_data, const char *path)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(path);
    s_test_deferred_image_io_open_count++;
    return &s_test_deferred_image_decoder_ctx;
}

static int32_t test_deferred_image_mock_read(void *user_data, void *handle, void *buf, uint32_t size)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(handle);
    EGUI_UNUSED(buf);
    EGUI_UNUSED(size);
    return 0;
}

static int test_deferred_image_mock_seek(void *user_data, void *handle, int32_t offset, int whence)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(handle);
    EGUI_UNUSED(offset);
    EGUI_UNUSED(whence);
    return 1;
}

static int32_t test_deferred_image_mock_tell(void *user_data, void *handle)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(handle);
    return 0;
}

static void test_deferred_image_mock_close(void *user_data, void *handle)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(handle);
}

static int test_deferred_image_mock_match(const char *path)
{
    EGUI_UNUSED(path);
    return 1;
}

static int test_deferred_image_mock_decoder_open(const egui_image_file_io_t *io, void *file_handle, const char *path, void **decoder_ctx,
                                                 egui_image_file_open_result_t *out_info)
{
    EGUI_UNUSED(io);
    EGUI_UNUSED(file_handle);
    EGUI_UNUSED(path);
    s_test_deferred_image_decoder_open_count++;
    *decoder_ctx = &s_test_deferred_image_decoder_ctx;
    out_info->width = 4;
    out_info->height = 2;
    out_info->has_alpha = 0;
    out_info->keep_file_open = 1;
    return 1;
}

static int test_deferred_image_mock_decoder_read_row(void *decoder_ctx, uint16_t row, uint16_t *rgb565_row, uint8_t *alpha_row)
{
    EGUI_UNUSED(decoder_ctx);
    EGUI_UNUSED(alpha_row);
    s_test_deferred_image_read_row_count++;
    if (row >= 2 || rgb565_row == NULL)
    {
        return 0;
    }

    memcpy(rgb565_row, s_test_deferred_image_pixels[row], sizeof(s_test_deferred_image_pixels[row]));
    return 1;
}

static void test_deferred_image_mock_decoder_close(void *decoder_ctx)
{
    EGUI_UNUSED(decoder_ctx);
    s_test_deferred_image_decoder_close_count++;
}

static const egui_image_file_io_t s_test_deferred_image_io = {
        .user_data = NULL,
        .open = test_deferred_image_mock_open,
        .read = test_deferred_image_mock_read,
        .seek = test_deferred_image_mock_seek,
        .tell = test_deferred_image_mock_tell,
        .close = test_deferred_image_mock_close,
};

static const egui_image_file_decoder_t s_test_deferred_image_decoder = {
        .name = "mock",
        .match = test_deferred_image_mock_match,
        .open = test_deferred_image_mock_decoder_open,
        .read_row = test_deferred_image_mock_decoder_read_row,
        .close = test_deferred_image_mock_decoder_close,
};

static int test_deferred_image_loader_start(void *user_data, const char *source_uri, const char *cache_path, void **request_handle)
{
    test_deferred_image_request_t *request;

    EGUI_UNUSED(user_data);
    s_test_deferred_image_loader_state.start_count++;
    s_test_deferred_image_loader_state.last_source_uri = source_uri;
    s_test_deferred_image_loader_state.last_cache_path = cache_path;

    if (!s_test_deferred_image_loader_state.next_start_success || request_handle == NULL)
    {
        return 0;
    }

    request = (test_deferred_image_request_t *)egui_malloc(sizeof(*request));
    if (request == NULL)
    {
        return 0;
    }

    request->remaining_polls = s_test_deferred_image_loader_state.polls_until_terminal;
    request->terminal_result = (uint8_t)s_test_deferred_image_loader_state.terminal_result;
    *request_handle = request;
    return 1;
}

static egui_view_deferred_image_loader_poll_result_t test_deferred_image_loader_poll(void *user_data, void *request_handle)
{
    test_deferred_image_request_t *request = (test_deferred_image_request_t *)request_handle;

    EGUI_UNUSED(user_data);
    s_test_deferred_image_loader_state.poll_count++;
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

static void test_deferred_image_loader_cancel(void *user_data, void *request_handle)
{
    EGUI_UNUSED(user_data);
    s_test_deferred_image_loader_state.cancel_count++;
    if (request_handle != NULL)
    {
        egui_free(request_handle);
    }
}

static const egui_view_deferred_image_loader_t s_test_deferred_image_loader = {
        .user_data = NULL,
        .start = test_deferred_image_loader_start,
        .poll = test_deferred_image_loader_poll,
        .cancel = test_deferred_image_loader_cancel,
};

static int test_deferred_image_prepare_widget(const char *source_uri, const char *cache_path, uint16_t delay_ms)
{
    memset(&s_test_deferred_image_view, 0, sizeof(s_test_deferred_image_view));
    memset(&s_test_deferred_placeholder, 0, sizeof(s_test_deferred_placeholder));
    test_deferred_image_reset_decoder_counters();
    test_deferred_image_reset_loader_state();
    egui_image_file_set_default_io(&s_test_deferred_image_io);
    egui_image_file_clear_decoders();
    if (!egui_image_file_register_decoder(&s_test_deferred_image_decoder))
    {
        return 0;
    }

    egui_view_deferred_image_init(EGUI_VIEW_OF(&s_test_deferred_image_view));
    egui_view_set_size(EGUI_VIEW_OF(&s_test_deferred_image_view), 64, 48);
    egui_view_deferred_image_set_placeholder_image(EGUI_VIEW_OF(&s_test_deferred_image_view), &s_test_deferred_placeholder);
    egui_view_deferred_image_set_loader(EGUI_VIEW_OF(&s_test_deferred_image_view), &s_test_deferred_image_loader);
    egui_view_deferred_image_set_source_uri(EGUI_VIEW_OF(&s_test_deferred_image_view), source_uri);
    egui_view_deferred_image_set_cache_path(EGUI_VIEW_OF(&s_test_deferred_image_view), cache_path);
    egui_view_deferred_image_set_load_delay_ms(EGUI_VIEW_OF(&s_test_deferred_image_view), delay_ms);
    return 1;
}

static void test_deferred_image_cleanup_widget(void)
{
    egui_view_deferred_image_deinit(EGUI_VIEW_OF(&s_test_deferred_image_view));
    egui_image_file_set_default_io(NULL);
    egui_image_file_clear_decoders();
}

static void test_deferred_image_attach_starts_delay_timer(void)
{
    EGUI_TEST_ASSERT_TRUE(test_deferred_image_prepare_widget("mock:/source", "mock:/cache.bin", 25));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE, egui_view_deferred_image_get_status(EGUI_VIEW_OF(&s_test_deferred_image_view)));

    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&s_test_deferred_image_view));

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_DELAY, egui_view_deferred_image_get_status(EGUI_VIEW_OF(&s_test_deferred_image_view)));
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&s_test_deferred_image_view.delay_timer));
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&s_test_deferred_image_view.poll_timer));
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_test_deferred_image_loader_state.start_count);

    test_deferred_image_cleanup_widget();
}

static void test_deferred_image_delay_callback_starts_loader_and_polling(void)
{
    EGUI_TEST_ASSERT_TRUE(test_deferred_image_prepare_widget("mock:/source", "mock:/cache.bin", 25));

    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&s_test_deferred_image_view));
    s_test_deferred_image_view.delay_timer.callback(&s_test_deferred_image_view.delay_timer);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_LOADING, egui_view_deferred_image_get_status(EGUI_VIEW_OF(&s_test_deferred_image_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_test_deferred_image_loader_state.start_count);
    EGUI_TEST_ASSERT_TRUE(strcmp(s_test_deferred_image_loader_state.last_source_uri, "mock:/source") == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp(s_test_deferred_image_loader_state.last_cache_path, "mock:/cache.bin") == 0);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&s_test_deferred_image_view.delay_timer));
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&s_test_deferred_image_view.poll_timer));
    EGUI_TEST_ASSERT_NOT_NULL(s_test_deferred_image_view.request_handle);

    test_deferred_image_cleanup_widget();
}

static void test_deferred_image_success_reaches_ready_and_uses_loaded_image(void)
{
    EGUI_TEST_ASSERT_TRUE(test_deferred_image_prepare_widget("mock:/source", "mock:/cache.bin", 0));

    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&s_test_deferred_image_view));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_LOADING, egui_view_deferred_image_get_status(EGUI_VIEW_OF(&s_test_deferred_image_view)));

    s_test_deferred_image_view.poll_timer.callback(&s_test_deferred_image_view.poll_timer);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_READY, egui_view_deferred_image_get_status(EGUI_VIEW_OF(&s_test_deferred_image_view)));
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&s_test_deferred_image_view.poll_timer));
    EGUI_TEST_ASSERT_NULL(s_test_deferred_image_view.request_handle);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_test_deferred_image_loader_state.cancel_count);
    EGUI_TEST_ASSERT_TRUE(s_test_deferred_image_view.display_image == (const egui_image_t *)&s_test_deferred_image_view.loaded_image);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_IMAGE_FILE_STATUS_READY, egui_image_file_get_status(&s_test_deferred_image_view.loaded_image));
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_test_deferred_image_io_open_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_test_deferred_image_decoder_open_count);

    test_deferred_image_cleanup_widget();
}

static void test_deferred_image_fail_reaches_failed_and_keeps_placeholder(void)
{
    EGUI_TEST_ASSERT_TRUE(test_deferred_image_prepare_widget("mock:/source", "mock:/cache.bin", 0));
    s_test_deferred_image_loader_state.terminal_result = EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_FAILED;

    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&s_test_deferred_image_view));
    s_test_deferred_image_view.poll_timer.callback(&s_test_deferred_image_view.poll_timer);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_FAILED, egui_view_deferred_image_get_status(EGUI_VIEW_OF(&s_test_deferred_image_view)));
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&s_test_deferred_image_view.poll_timer));
    EGUI_TEST_ASSERT_NULL(s_test_deferred_image_view.request_handle);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_test_deferred_image_loader_state.cancel_count);
    EGUI_TEST_ASSERT_TRUE(s_test_deferred_image_view.display_image == &s_test_deferred_placeholder);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_IMAGE_FILE_STATUS_IDLE, egui_image_file_get_status(&s_test_deferred_image_view.loaded_image));

    test_deferred_image_cleanup_widget();
}

static void test_deferred_image_detach_cancels_loading_request(void)
{
    EGUI_TEST_ASSERT_TRUE(test_deferred_image_prepare_widget("mock:/source", "mock:/cache.bin", 0));
    s_test_deferred_image_loader_state.polls_until_terminal = 1u;

    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&s_test_deferred_image_view));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_LOADING, egui_view_deferred_image_get_status(EGUI_VIEW_OF(&s_test_deferred_image_view)));

    egui_view_dispatch_detach_from_window(EGUI_VIEW_OF(&s_test_deferred_image_view));

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE, egui_view_deferred_image_get_status(EGUI_VIEW_OF(&s_test_deferred_image_view)));
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&s_test_deferred_image_view.poll_timer));
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_test_deferred_image_loader_state.cancel_count);
    EGUI_TEST_ASSERT_NULL(s_test_deferred_image_view.request_handle);
    EGUI_TEST_ASSERT_TRUE(s_test_deferred_image_view.display_image == &s_test_deferred_placeholder);

    test_deferred_image_cleanup_widget();
}

static void test_deferred_image_detach_cancels_pending_delay(void)
{
    EGUI_TEST_ASSERT_TRUE(test_deferred_image_prepare_widget("mock:/source", "mock:/cache.bin", 25));

    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&s_test_deferred_image_view));
    egui_view_dispatch_detach_from_window(EGUI_VIEW_OF(&s_test_deferred_image_view));

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE, egui_view_deferred_image_get_status(EGUI_VIEW_OF(&s_test_deferred_image_view)));
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&s_test_deferred_image_view.delay_timer));
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&s_test_deferred_image_view.poll_timer));
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_test_deferred_image_loader_state.start_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_test_deferred_image_loader_state.cancel_count);

    test_deferred_image_cleanup_widget();
}

void test_deferred_image_run(void)
{
    EGUI_TEST_SUITE_BEGIN(deferred_image);

    EGUI_TEST_RUN(test_deferred_image_attach_starts_delay_timer);
    EGUI_TEST_RUN(test_deferred_image_delay_callback_starts_loader_and_polling);
    EGUI_TEST_RUN(test_deferred_image_success_reaches_ready_and_uses_loaded_image);
    EGUI_TEST_RUN(test_deferred_image_fail_reaches_failed_and_keeps_placeholder);
    EGUI_TEST_RUN(test_deferred_image_detach_cancels_loading_request);
    EGUI_TEST_RUN(test_deferred_image_detach_cancels_pending_delay);

    EGUI_TEST_SUITE_END();
}

#else

void test_deferred_image_run(void)
{
}

#endif
