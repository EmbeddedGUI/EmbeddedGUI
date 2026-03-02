#include "egui.h"
#include "utils/simple_ringbuffer/simple_ringbuffer.h"
#include "test/egui_test.h"
#include "test_ringbuffer.h"

#define TEST_RINGBUF_SIZE 16

static uint8_t test_rb_storage[TEST_RINGBUF_SIZE];
static simple_ringbuffer_t test_rb;

static void test_rb_init(void)
{
    simple_ringbuffer_init(&test_rb, TEST_RINGBUF_SIZE, test_rb_storage);
    EGUI_TEST_ASSERT_TRUE(simple_ringbuffer_is_empty(&test_rb));
    EGUI_TEST_ASSERT_FALSE(simple_ringbuffer_is_full(&test_rb));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)simple_ringbuffer_size(&test_rb));
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_RINGBUF_SIZE, (int)simple_ringbuffer_total_size(&test_rb));
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_RINGBUF_SIZE, (int)simple_ringbuffer_reserve_size(&test_rb));
}

static void test_rb_put_get_basic(void)
{
    simple_ringbuffer_init(&test_rb, TEST_RINGBUF_SIZE, test_rb_storage);

    uint8_t write_data[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t written = simple_ringbuffer_put(&test_rb, write_data, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)written);
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)simple_ringbuffer_size(&test_rb));
    EGUI_TEST_ASSERT_FALSE(simple_ringbuffer_is_empty(&test_rb));

    uint8_t read_data[4] = {0};
    uint32_t read_len = simple_ringbuffer_get(&test_rb, read_data, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)read_len);
    EGUI_TEST_ASSERT_EQUAL_INT(0x01, read_data[0]);
    EGUI_TEST_ASSERT_EQUAL_INT(0x02, read_data[1]);
    EGUI_TEST_ASSERT_EQUAL_INT(0x03, read_data[2]);
    EGUI_TEST_ASSERT_EQUAL_INT(0x04, read_data[3]);
    EGUI_TEST_ASSERT_TRUE(simple_ringbuffer_is_empty(&test_rb));
}

static void test_rb_full(void)
{
    simple_ringbuffer_init(&test_rb, TEST_RINGBUF_SIZE, test_rb_storage);

    uint8_t data[TEST_RINGBUF_SIZE];
    for (int i = 0; i < TEST_RINGBUF_SIZE; i++)
    {
        data[i] = (uint8_t)i;
    }

    uint32_t written = simple_ringbuffer_put(&test_rb, data, TEST_RINGBUF_SIZE);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_RINGBUF_SIZE, (int)written);
    EGUI_TEST_ASSERT_TRUE(simple_ringbuffer_is_full(&test_rb));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)simple_ringbuffer_reserve_size(&test_rb));
}

static void test_rb_reset(void)
{
    simple_ringbuffer_init(&test_rb, TEST_RINGBUF_SIZE, test_rb_storage);

    uint8_t data[] = {0x01, 0x02};
    simple_ringbuffer_put(&test_rb, data, 2);
    EGUI_TEST_ASSERT_FALSE(simple_ringbuffer_is_empty(&test_rb));

    simple_ringbuffer_reset(&test_rb);
    EGUI_TEST_ASSERT_TRUE(simple_ringbuffer_is_empty(&test_rb));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)simple_ringbuffer_size(&test_rb));
}

static void test_rb_wrap_around(void)
{
    simple_ringbuffer_init(&test_rb, TEST_RINGBUF_SIZE, test_rb_storage);

    // Fill half
    uint8_t data[TEST_RINGBUF_SIZE / 2];
    for (int i = 0; i < TEST_RINGBUF_SIZE / 2; i++)
    {
        data[i] = (uint8_t)(i + 1);
    }
    simple_ringbuffer_put(&test_rb, data, TEST_RINGBUF_SIZE / 2);

    // Read half to advance read pointer
    uint8_t read_buf[TEST_RINGBUF_SIZE / 2];
    simple_ringbuffer_get(&test_rb, read_buf, TEST_RINGBUF_SIZE / 2);
    EGUI_TEST_ASSERT_TRUE(simple_ringbuffer_is_empty(&test_rb));

    // Now write again - this will wrap around
    uint8_t data2[TEST_RINGBUF_SIZE];
    for (int i = 0; i < TEST_RINGBUF_SIZE; i++)
    {
        data2[i] = (uint8_t)(0xA0 + i);
    }
    simple_ringbuffer_put(&test_rb, data2, TEST_RINGBUF_SIZE);

    // Read back and verify
    uint8_t read_buf2[TEST_RINGBUF_SIZE];
    uint32_t read_len = simple_ringbuffer_get(&test_rb, read_buf2, TEST_RINGBUF_SIZE);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_RINGBUF_SIZE, (int)read_len);
    for (int i = 0; i < TEST_RINGBUF_SIZE; i++)
    {
        EGUI_TEST_ASSERT_EQUAL_INT((uint8_t)(0xA0 + i), read_buf2[i]);
    }
}

void test_ringbuffer_run(void)
{
    EGUI_TEST_SUITE_BEGIN(ringbuffer);

    EGUI_TEST_RUN(test_rb_init);
    EGUI_TEST_RUN(test_rb_put_get_basic);
    EGUI_TEST_RUN(test_rb_full);
    EGUI_TEST_RUN(test_rb_reset);
    EGUI_TEST_RUN(test_rb_wrap_around);

    EGUI_TEST_SUITE_END();
}
