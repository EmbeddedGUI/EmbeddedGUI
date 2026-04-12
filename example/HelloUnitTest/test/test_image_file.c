#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_image_file.h"

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE

typedef struct test_image_file_decoder_ctx
{
    int dummy;
} test_image_file_decoder_ctx_t;

static const uint16_t s_test_image_file_pixels[2][4] = {
        {0x001F, 0x07E0, 0xF800, 0xFFFF},
        {0x0000, 0x39E7, 0x7BEF, 0xFD20},
};
static int s_test_image_file_io_open_count;
static int s_test_image_file_io_close_count;
static int s_test_image_file_decoder_open_count;
static int s_test_image_file_decoder_close_count;
static int s_test_image_file_read_row_count;
static test_image_file_decoder_ctx_t s_test_image_file_decoder_ctx;

static void test_image_file_reset_counters(void)
{
    s_test_image_file_io_open_count = 0;
    s_test_image_file_io_close_count = 0;
    s_test_image_file_decoder_open_count = 0;
    s_test_image_file_decoder_close_count = 0;
    s_test_image_file_read_row_count = 0;
}

static void *test_image_file_mock_open(void *user_data, const char *path)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(path);
    s_test_image_file_io_open_count++;
    return &s_test_image_file_decoder_ctx;
}

static int32_t test_image_file_mock_read(void *user_data, void *handle, void *buf, uint32_t size)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(handle);
    EGUI_UNUSED(buf);
    EGUI_UNUSED(size);
    return 0;
}

static int test_image_file_mock_seek(void *user_data, void *handle, int32_t offset, int whence)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(handle);
    EGUI_UNUSED(offset);
    EGUI_UNUSED(whence);
    return 1;
}

static int32_t test_image_file_mock_tell(void *user_data, void *handle)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(handle);
    return 0;
}

static void test_image_file_mock_close(void *user_data, void *handle)
{
    EGUI_UNUSED(user_data);
    EGUI_UNUSED(handle);
    s_test_image_file_io_close_count++;
}

static int test_image_file_mock_match(const char *path)
{
    EGUI_UNUSED(path);
    return 1;
}

static int test_image_file_mock_decoder_open(const egui_image_file_io_t *io, void *file_handle, const char *path, void **decoder_ctx,
                                             egui_image_file_open_result_t *out_info)
{
    EGUI_UNUSED(io);
    EGUI_UNUSED(file_handle);
    EGUI_UNUSED(path);
    s_test_image_file_decoder_open_count++;
    *decoder_ctx = &s_test_image_file_decoder_ctx;
    out_info->width = 4;
    out_info->height = 2;
    out_info->has_alpha = 0;
    out_info->keep_file_open = 1;
    return 1;
}

static int test_image_file_mock_decoder_read_row(void *decoder_ctx, uint16_t row, uint16_t *rgb565_row, uint8_t *alpha_row)
{
    EGUI_UNUSED(decoder_ctx);
    EGUI_UNUSED(alpha_row);
    s_test_image_file_read_row_count++;
    if (row >= 2 || rgb565_row == NULL)
    {
        return 0;
    }

    memcpy(rgb565_row, s_test_image_file_pixels[row], sizeof(s_test_image_file_pixels[row]));
    return 1;
}

static void test_image_file_mock_decoder_close(void *decoder_ctx)
{
    EGUI_UNUSED(decoder_ctx);
    s_test_image_file_decoder_close_count++;
}

static const egui_image_file_io_t s_test_image_file_io = {
        .user_data = NULL,
        .open = test_image_file_mock_open,
        .read = test_image_file_mock_read,
        .seek = test_image_file_mock_seek,
        .tell = test_image_file_mock_tell,
        .close = test_image_file_mock_close,
};

static const egui_image_file_decoder_t s_test_image_file_decoder = {
        .name = "mock",
        .match = test_image_file_mock_match,
        .open = test_image_file_mock_decoder_open,
        .read_row = test_image_file_mock_decoder_read_row,
        .close = test_image_file_mock_decoder_close,
};

static void test_image_file_setup(egui_image_file_t *image)
{
    egui_image_file_clear_decoders();
    EGUI_TEST_ASSERT_TRUE(egui_image_file_register_decoder(&s_test_image_file_decoder));
    egui_image_file_init(image);
    egui_image_file_set_io(image, &s_test_image_file_io);
    EGUI_TEST_ASSERT_TRUE(egui_image_file_set_path(image, "mock:/image.bin"));
}

static void test_image_file_teardown(egui_image_file_t *image)
{
    egui_image_file_deinit(image);
    egui_image_file_clear_decoders();
}

static void test_image_file_resize_get_size_uses_intrinsic_dimensions(void)
{
    egui_image_file_t image;
    egui_dim_t width = 0;
    egui_dim_t height = 0;

    test_image_file_reset_counters();
    test_image_file_setup(&image);

    egui_image_file_set_resize(&image, 8, 4);
    EGUI_TEST_ASSERT_TRUE(egui_image_file_get_resize(&image, &width, &height));
    EGUI_TEST_ASSERT_EQUAL_INT(8, width);
    EGUI_TEST_ASSERT_EQUAL_INT(4, height);
    EGUI_TEST_ASSERT_TRUE(egui_image_get_size((const egui_image_t *)&image, &width, &height));
    EGUI_TEST_ASSERT_EQUAL_INT(8, width);
    EGUI_TEST_ASSERT_EQUAL_INT(4, height);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_test_image_file_io_open_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_test_image_file_decoder_open_count);

    test_image_file_teardown(&image);
}

static void test_image_file_resize_get_point_maps_to_source(void)
{
    egui_image_file_t image;
    egui_color_t color = {0};
    egui_alpha_t alpha = 0;

    test_image_file_reset_counters();
    test_image_file_setup(&image);

    egui_image_file_set_resize(&image, 8, 4);
    EGUI_TEST_ASSERT_TRUE(((const egui_image_t *)&image)->api->get_point((const egui_image_t *)&image, 6, 3, &color, &alpha));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_RGB565_TRANS(s_test_image_file_pixels[1][3]), color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, alpha);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_test_image_file_io_open_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_test_image_file_decoder_open_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_test_image_file_read_row_count);

    test_image_file_teardown(&image);
}

static void test_image_file_get_point_resize_uses_call_dimensions(void)
{
    egui_image_file_t image;
    egui_color_t color = {0};
    egui_alpha_t alpha = 0;
    egui_dim_t width = 0;
    egui_dim_t height = 0;

    test_image_file_reset_counters();
    test_image_file_setup(&image);

    egui_image_file_set_resize(&image, 8, 4);
    EGUI_TEST_ASSERT_TRUE(((const egui_image_t *)&image)->api->get_point_resize((const egui_image_t *)&image, 1, 0, 2, 1, &color, &alpha));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_RGB565_TRANS(s_test_image_file_pixels[0][2]), color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, alpha);

    egui_image_file_clear_resize(&image);
    EGUI_TEST_ASSERT_TRUE(egui_image_get_size((const egui_image_t *)&image, &width, &height));
    EGUI_TEST_ASSERT_EQUAL_INT(4, width);
    EGUI_TEST_ASSERT_EQUAL_INT(2, height);

    test_image_file_teardown(&image);
}

void test_image_file_run(void)
{
    EGUI_TEST_SUITE_BEGIN(image_file);

    EGUI_TEST_RUN(test_image_file_resize_get_size_uses_intrinsic_dimensions);
    EGUI_TEST_RUN(test_image_file_resize_get_point_maps_to_source);
    EGUI_TEST_RUN(test_image_file_get_point_resize_uses_call_dimensions);

    EGUI_TEST_SUITE_END();
}

#else

void test_image_file_run(void)
{
}

#endif
