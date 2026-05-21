#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_deferred_image_get_state.h"

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE

static egui_view_deferred_image_t s_deferred_image;
static egui_image_t s_placeholder;
static egui_image_t s_file_placeholder;

static int test_loader_start(void *user_data, const char *source_uri, const char *cache_path, void **request_handle)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(source_uri);
    EGUI_UNUSED(cache_path);
    EGUI_UNUSED(request_handle);
    return 0;
}

static egui_view_deferred_image_loader_poll_result_t test_loader_poll(void *user_data, void *request_handle)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(request_handle);
    return EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_FAILED;
}

static const egui_view_deferred_image_loader_t s_loader = {
        .user_data = NULL,
        .start = test_loader_start,
        .poll = test_loader_poll,
        .cancel = NULL,
};

static void setup(void)
{
    memset(&s_deferred_image, 0, sizeof(s_deferred_image));
    memset(&s_placeholder, 0, sizeof(s_placeholder));
    memset(&s_file_placeholder, 0, sizeof(s_file_placeholder));
    egui_view_deferred_image_init(EGUI_VIEW_OF(&s_deferred_image), uicode_get_core());
}

static void cleanup(void)
{
    egui_view_deferred_image_deinit(EGUI_VIEW_OF(&s_deferred_image));
}

static void test_deferred_image_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_source_uri(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_cache_path(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_placeholder_image(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_file_placeholder(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_loader(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_display_image(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_deferred_image_get_auto_start_on_attach(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_deferred_image_get_load_delay_ms(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE, egui_view_deferred_image_get_status(EGUI_VIEW_OF(&s_deferred_image)));

    cleanup();
}

static void test_deferred_image_get_state_after_setters(void)
{
    const char *source_uri = "mock:/source";
    const char *cache_path = "mock:/cache.bin";
    const char *got_source;
    const char *got_cache;

    setup();
    egui_view_deferred_image_set_source_uri(EGUI_VIEW_OF(&s_deferred_image), source_uri);
    egui_view_deferred_image_set_cache_path(EGUI_VIEW_OF(&s_deferred_image), cache_path);
    egui_view_deferred_image_set_placeholder_image(EGUI_VIEW_OF(&s_deferred_image), &s_placeholder);
    egui_view_deferred_image_set_file_placeholder(EGUI_VIEW_OF(&s_deferred_image), &s_file_placeholder);
    egui_view_deferred_image_set_loader(EGUI_VIEW_OF(&s_deferred_image), &s_loader);
    egui_view_deferred_image_set_auto_start_on_attach(EGUI_VIEW_OF(&s_deferred_image), 0);
    egui_view_deferred_image_set_load_delay_ms(EGUI_VIEW_OF(&s_deferred_image), 125);

    got_source = egui_view_deferred_image_get_source_uri(EGUI_VIEW_OF(&s_deferred_image));
    got_cache = egui_view_deferred_image_get_cache_path(EGUI_VIEW_OF(&s_deferred_image));
    EGUI_TEST_ASSERT_NOT_NULL(got_source);
    EGUI_TEST_ASSERT_NOT_NULL(got_cache);
    EGUI_TEST_ASSERT_TRUE(strcmp(got_source, source_uri) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp(got_cache, cache_path) == 0);
    EGUI_TEST_ASSERT_TRUE(got_source != source_uri);
    EGUI_TEST_ASSERT_TRUE(got_cache != cache_path);
    EGUI_TEST_ASSERT_TRUE(egui_view_deferred_image_get_placeholder_image(EGUI_VIEW_OF(&s_deferred_image)) == &s_placeholder);
    EGUI_TEST_ASSERT_TRUE(egui_view_deferred_image_get_file_placeholder(EGUI_VIEW_OF(&s_deferred_image)) == &s_file_placeholder);
    EGUI_TEST_ASSERT_TRUE(egui_view_deferred_image_get_loader(EGUI_VIEW_OF(&s_deferred_image)) == &s_loader);
    EGUI_TEST_ASSERT_TRUE(egui_view_deferred_image_get_display_image(EGUI_VIEW_OF(&s_deferred_image)) == &s_placeholder);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_deferred_image_get_auto_start_on_attach(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_EQUAL_INT(125, (int)egui_view_deferred_image_get_load_delay_ms(EGUI_VIEW_OF(&s_deferred_image)));

    cleanup();
}

static void test_deferred_image_get_state_clear_optional_values(void)
{
    setup();
    egui_view_deferred_image_set_source_uri(EGUI_VIEW_OF(&s_deferred_image), "mock:/source");
    egui_view_deferred_image_set_cache_path(EGUI_VIEW_OF(&s_deferred_image), "mock:/cache.bin");
    egui_view_deferred_image_set_placeholder_image(EGUI_VIEW_OF(&s_deferred_image), &s_placeholder);
    egui_view_deferred_image_set_file_placeholder(EGUI_VIEW_OF(&s_deferred_image), &s_file_placeholder);
    egui_view_deferred_image_set_loader(EGUI_VIEW_OF(&s_deferred_image), &s_loader);

    egui_view_deferred_image_set_source_uri(EGUI_VIEW_OF(&s_deferred_image), "");
    egui_view_deferred_image_set_cache_path(EGUI_VIEW_OF(&s_deferred_image), NULL);
    egui_view_deferred_image_set_placeholder_image(EGUI_VIEW_OF(&s_deferred_image), NULL);
    egui_view_deferred_image_set_file_placeholder(EGUI_VIEW_OF(&s_deferred_image), NULL);
    egui_view_deferred_image_set_loader(EGUI_VIEW_OF(&s_deferred_image), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_source_uri(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_cache_path(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_placeholder_image(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_file_placeholder(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_loader(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_display_image(EGUI_VIEW_OF(&s_deferred_image)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE, egui_view_deferred_image_get_status(EGUI_VIEW_OF(&s_deferred_image)));

    cleanup();
}

static void test_deferred_image_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_source_uri(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_cache_path(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_placeholder_image(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_file_placeholder(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_loader(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_deferred_image_get_display_image(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_deferred_image_get_auto_start_on_attach(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_deferred_image_get_load_delay_ms(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE, egui_view_deferred_image_get_status(NULL));
}

void test_deferred_image_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(deferred_image_get_state);

    EGUI_TEST_RUN(test_deferred_image_get_state_defaults);
    EGUI_TEST_RUN(test_deferred_image_get_state_after_setters);
    EGUI_TEST_RUN(test_deferred_image_get_state_clear_optional_values);
    EGUI_TEST_RUN(test_deferred_image_get_state_null_self);

    EGUI_TEST_SUITE_END();
}

#else

void test_deferred_image_get_state_run(void)
{
}

#endif
