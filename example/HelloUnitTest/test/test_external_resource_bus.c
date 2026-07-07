#include <string.h>

#include "egui.h"
#include "image/egui_image_svg.h"
#include "test/egui_test.h"
#include "test_external_resource_bus.h"
#include "uicode_disp0.h"

#define TEST_EXTERNAL_RESOURCE_BUS_PFB_W       4
#define TEST_EXTERNAL_RESOURCE_BUS_PFB_H       4
#define TEST_EXTERNAL_RESOURCE_BUS_PFB_PIXELS  (TEST_EXTERNAL_RESOURCE_BUS_PFB_W * TEST_EXTERNAL_RESOURCE_BUS_PFB_H)
#define TEST_EXTERNAL_RESOURCE_BUS_EXPECT_LOCK (EGUI_CONFIG_EXTERNAL_RESOURCE_SHARED_DISPLAY_BUS ? 1 : 0)
#define TEST_EXTERNAL_RESOURCE_BUS_RECORD_MAX  8

typedef struct test_external_resource_bus_load_record
{
    int load_count;
    egui_core_t *core;
    int bus_locked;
    egui_uintptr_t res_id;
    uint32_t start_offset;
    uint32_t size;
} test_external_resource_bus_load_record_t;

typedef struct test_external_resource_bus_display_record
{
    int draw_count;
    int wait_count;
    int wait_notifies;
    egui_core_t *draw_core[TEST_EXTERNAL_RESOURCE_BUS_RECORD_MAX];
    int draw_bus_locked[TEST_EXTERNAL_RESOURCE_BUS_RECORD_MAX];
    int16_t draw_x[TEST_EXTERNAL_RESOURCE_BUS_RECORD_MAX];
} test_external_resource_bus_display_record_t;

static test_external_resource_bus_load_record_t s_load_record;
static test_external_resource_bus_display_record_t s_display_record;

static void test_external_resource_bus_reset_load_record(void)
{
    memset(&s_load_record, 0, sizeof(s_load_record));
}

static void test_external_resource_bus_reset_display_record(void)
{
    memset(&s_display_record, 0, sizeof(s_display_record));
}

static uint32_t test_external_resource_bus_get_tick_ms(void)
{
    return 0;
}

static egui_base_t test_external_resource_bus_interrupt_disable(void)
{
    return 0;
}

static void test_external_resource_bus_interrupt_enable(egui_base_t level)
{
    EGUI_UNUSED(level);
}

static void test_external_resource_bus_load(egui_core_t *core, void *dest, egui_uintptr_t res_id, uint32_t start_offset, uint32_t size)
{
    s_load_record.load_count++;
    s_load_record.core = core;
    s_load_record.bus_locked = core != NULL ? core->render.pfb_mgr.bus_locked : 0;
    s_load_record.res_id = res_id;
    s_load_record.start_offset = start_offset;
    s_load_record.size = size;

    if (dest != NULL && res_id != (egui_uintptr_t)0 && size > 0U)
    {
        const uint8_t *src = (const uint8_t *)res_id;
        memcpy(dest, src + start_offset, size);
    }
}

static const egui_platform_ops_t s_resource_platform_ops = {
        .get_tick_ms = test_external_resource_bus_get_tick_ms,
        .interrupt_disable = test_external_resource_bus_interrupt_disable,
        .interrupt_enable = test_external_resource_bus_interrupt_enable,
        .load_external_resource = test_external_resource_bus_load,
};

static const egui_platform_ops_t s_no_resource_platform_ops = {
        .get_tick_ms = test_external_resource_bus_get_tick_ms,
        .interrupt_disable = test_external_resource_bus_interrupt_disable,
        .interrupt_enable = test_external_resource_bus_interrupt_enable,
        .load_external_resource = NULL,
};

static void test_external_resource_bus_draw_area(egui_core_t *core, int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    int index = s_display_record.draw_count;

    EGUI_UNUSED(y);
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
    EGUI_UNUSED(data);

    if (index < TEST_EXTERNAL_RESOURCE_BUS_RECORD_MAX)
    {
        s_display_record.draw_core[index] = core;
        s_display_record.draw_bus_locked[index] = core != NULL ? core->render.pfb_mgr.bus_locked : 0;
        s_display_record.draw_x[index] = x;
    }
    s_display_record.draw_count++;
}

static void test_external_resource_bus_wait_draw_complete(egui_core_t *core)
{
    s_display_record.wait_count++;
    if (s_display_record.wait_notifies && core != NULL && core->render.pfb_mgr.dma_busy)
    {
        egui_pfb_manager_notify_flush_complete(&core->render.pfb_mgr);
    }
}

static const egui_display_driver_ops_t s_async_display_ops = {
        .draw_area = test_external_resource_bus_draw_area,
        .wait_draw_complete = test_external_resource_bus_wait_draw_complete,
};

static void test_external_resource_bus_init_local_core(egui_core_t *core, egui_color_int_t *pfb0, egui_color_int_t *pfb1, egui_display_driver_t *driver)
{
    egui_color_int_t *pfb_bufs[2] = {pfb0, pfb1};

    memset(core, 0, sizeof(*core));
    memset(pfb0, 0, TEST_EXTERNAL_RESOURCE_BUS_PFB_PIXELS * sizeof(*pfb0));
    memset(pfb1, 0, TEST_EXTERNAL_RESOURCE_BUS_PFB_PIXELS * sizeof(*pfb1));
    memset(driver, 0, sizeof(*driver));

    egui_init_display(core, 16, 16, pfb_bufs, 2, TEST_EXTERNAL_RESOURCE_BUS_PFB_W, TEST_EXTERNAL_RESOURCE_BUS_PFB_H);

    driver->ops = &s_async_display_ops;
    driver->physical_width = 16;
    driver->physical_height = 16;
    driver->rotation = EGUI_DISPLAY_ROTATION_0;
    driver->brightness = 255;
    driver->power_on = 1;
    egui_display_driver_register(core, driver);
}

static void test_external_resource_bus_finish_pending_flush(egui_core_t *core)
{
    while (core->render.pfb_mgr.pending_count > 0 || core->render.pfb_mgr.dma_busy)
    {
        egui_pfb_manager_notify_flush_complete(&core->render.pfb_mgr);
    }
}

static void test_external_resource_load_canvas_uses_configured_bus_policy(void)
{
    static const uint8_t src[] = {0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76};
    uint8_t dest[4] = {0};
    egui_core_t *core = uicode_get_core();
    egui_platform_t *platform = egui_platform_get();
    const egui_platform_ops_t *old_ops;
    int load_count;
    int bus_locked;
    egui_core_t *load_core;
    int bus_locked_after;

    EGUI_TEST_ASSERT_NOT_NULL(core);
    EGUI_TEST_ASSERT_NOT_NULL(platform);

    old_ops = platform->ops;
    test_external_resource_bus_reset_load_record();
    platform->ops = &s_resource_platform_ops;

    egui_api_load_external_resource(&core->canvas, dest, (egui_uintptr_t)src, 2U, sizeof(dest));

    load_count = s_load_record.load_count;
    bus_locked = s_load_record.bus_locked;
    load_core = s_load_record.core;
    bus_locked_after = core->render.pfb_mgr.bus_locked;
    platform->ops = old_ops;

    EGUI_TEST_ASSERT_EQUAL_INT(1, load_count);
    EGUI_TEST_ASSERT_TRUE(load_core == core);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_EXTERNAL_RESOURCE_BUS_EXPECT_LOCK, bus_locked);
    EGUI_TEST_ASSERT_EQUAL_INT(0, bus_locked_after);
    EGUI_TEST_ASSERT_TRUE(memcmp(dest, src + 2, sizeof(dest)) == 0);
}

static void test_external_resource_load_core_uses_configured_bus_policy(void)
{
    static const uint8_t src[] = {0x91, 0x82, 0x73, 0x64, 0x55};
    uint8_t dest[3] = {0};
    egui_core_t *core = uicode_get_core();
    egui_platform_t *platform = egui_platform_get();
    const egui_platform_ops_t *old_ops;
    int load_count;
    int bus_locked;
    egui_core_t *load_core;
    int bus_locked_after;

    EGUI_TEST_ASSERT_NOT_NULL(core);
    EGUI_TEST_ASSERT_NOT_NULL(platform);

    old_ops = platform->ops;
    test_external_resource_bus_reset_load_record();
    platform->ops = &s_resource_platform_ops;

    egui_api_load_external_resource_core(core, dest, (egui_uintptr_t)src, 1U, sizeof(dest));

    load_count = s_load_record.load_count;
    bus_locked = s_load_record.bus_locked;
    load_core = s_load_record.core;
    bus_locked_after = core->render.pfb_mgr.bus_locked;
    platform->ops = old_ops;

    EGUI_TEST_ASSERT_EQUAL_INT(1, load_count);
    EGUI_TEST_ASSERT_TRUE(load_core == core);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_EXTERNAL_RESOURCE_BUS_EXPECT_LOCK, bus_locked);
    EGUI_TEST_ASSERT_EQUAL_INT(0, bus_locked_after);
    EGUI_TEST_ASSERT_TRUE(memcmp(dest, src + 1, sizeof(dest)) == 0);
}

static void test_external_resource_load_null_core_keeps_unlocked_compatibility(void)
{
    static const uint8_t src[] = {0x31, 0x42, 0x53, 0x64};
    uint8_t dest[2] = {0};
    egui_platform_t *platform = egui_platform_get();
    const egui_platform_ops_t *old_ops;
    int load_count;
    int bus_locked;
    egui_core_t *load_core;

    EGUI_TEST_ASSERT_NOT_NULL(platform);

    old_ops = platform->ops;
    test_external_resource_bus_reset_load_record();
    platform->ops = &s_resource_platform_ops;

    egui_api_load_external_resource_core(NULL, dest, (egui_uintptr_t)src, 2U, sizeof(dest));

    load_count = s_load_record.load_count;
    bus_locked = s_load_record.bus_locked;
    load_core = s_load_record.core;
    platform->ops = old_ops;

    EGUI_TEST_ASSERT_EQUAL_INT(1, load_count);
    EGUI_TEST_ASSERT_NULL(load_core);
    EGUI_TEST_ASSERT_EQUAL_INT(0, bus_locked);
    EGUI_TEST_ASSERT_TRUE(memcmp(dest, src + 2, sizeof(dest)) == 0);
}

static void test_external_svg_resource_load_requires_core_context(void)
{
    static const char svg_text[] = "<svg width=\"2\" height=\"2\"><rect width=\"2\" height=\"2\" fill=\"#112233\"/></svg>";
    const egui_svg_source_t source = {
            .data_buf = svg_text,
            .data_size = (uint32_t)(sizeof(svg_text) - 1U),
            .res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
    };
    egui_image_svg_t image;
    egui_platform_t *platform = egui_platform_get();
    const egui_platform_ops_t *old_ops;
    int loaded;
    int load_count;

    EGUI_TEST_ASSERT_NOT_NULL(platform);

    old_ops = platform->ops;
    test_external_resource_bus_reset_load_record();
    platform->ops = &s_resource_platform_ops;
    egui_image_svg_init(&image);

    loaded = egui_image_svg_load_resource(&image, NULL, &source);
    load_count = s_load_record.load_count;

    egui_image_svg_deinit(&image);
    platform->ops = old_ops;

    EGUI_TEST_ASSERT_FALSE(loaded);
    EGUI_TEST_ASSERT_EQUAL_INT(0, load_count);
}

static void test_external_svg_resource_load_uses_core_bus_lock(void)
{
    static const char svg_text[] = "<svg width=\"2\" height=\"2\"><rect width=\"2\" height=\"2\" fill=\"#112233\"/></svg>";
    const egui_svg_source_t source = {
            .data_buf = svg_text,
            .data_size = (uint32_t)(sizeof(svg_text) - 1U),
            .res_type = EGUI_RESOURCE_TYPE_EXTERNAL,
    };
    egui_core_t *core = uicode_get_core();
    egui_platform_t *platform = egui_platform_get();
    const egui_platform_ops_t *old_ops;
    egui_image_svg_t image;
    int loaded;
    int load_count;
    int bus_locked;
    egui_core_t *load_core;
    int bus_locked_after;

    EGUI_TEST_ASSERT_NOT_NULL(core);
    EGUI_TEST_ASSERT_NOT_NULL(platform);

    old_ops = platform->ops;
    test_external_resource_bus_reset_load_record();
    platform->ops = &s_resource_platform_ops;
    egui_image_svg_init(&image);

    loaded = egui_image_svg_load_resource(&image, core, &source);
    load_count = s_load_record.load_count;
    bus_locked = s_load_record.bus_locked;
    load_core = s_load_record.core;
    bus_locked_after = core->render.pfb_mgr.bus_locked;

    egui_image_svg_deinit(&image);
    platform->ops = old_ops;

    EGUI_TEST_ASSERT_TRUE(loaded);
    EGUI_TEST_ASSERT_TRUE(load_count > 0);
    EGUI_TEST_ASSERT_TRUE(load_core == core);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_EXTERNAL_RESOURCE_BUS_EXPECT_LOCK, bus_locked);
    EGUI_TEST_ASSERT_EQUAL_INT(0, bus_locked_after);
}

static void test_external_resource_load_missing_callback_is_noop(void)
{
    uint8_t dest[3] = {0xAA, 0xAA, 0xAA};
    uint8_t expected[3] = {0xAA, 0xAA, 0xAA};
    egui_core_t *core = uicode_get_core();
    egui_platform_t *platform = egui_platform_get();
    const egui_platform_ops_t *old_ops;
    int bus_locked_after;

    EGUI_TEST_ASSERT_NOT_NULL(core);
    EGUI_TEST_ASSERT_NOT_NULL(platform);

    old_ops = platform->ops;
    test_external_resource_bus_reset_load_record();
    platform->ops = &s_no_resource_platform_ops;

    egui_api_load_external_resource(&core->canvas, dest, (egui_uintptr_t)expected, 0U, sizeof(dest));

    bus_locked_after = core->render.pfb_mgr.bus_locked;
    platform->ops = old_ops;

    EGUI_TEST_ASSERT_EQUAL_INT(0, s_load_record.load_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, bus_locked_after);
    EGUI_TEST_ASSERT_TRUE(memcmp(dest, expected, sizeof(dest)) == 0);
}

static void test_external_resource_load_waits_pending_flush_before_read(void)
{
    static const uint8_t src[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t dest[2] = {0};
    egui_core_t core;
    egui_display_driver_t driver;
    egui_color_int_t pfb0[TEST_EXTERNAL_RESOURCE_BUS_PFB_PIXELS];
    egui_color_int_t pfb1[TEST_EXTERNAL_RESOURCE_BUS_PFB_PIXELS];
    egui_platform_t *platform = egui_platform_get();
    const egui_platform_ops_t *old_ops;
    int load_count;
    int wait_count;
    int bus_locked;
    int bus_locked_after;
    int dma_busy_after_load;

    EGUI_TEST_ASSERT_NOT_NULL(platform);

    test_external_resource_bus_reset_display_record();
    test_external_resource_bus_init_local_core(&core, pfb0, pfb1, &driver);
    egui_pfb_manager_submit(&core.render.pfb_mgr, 0, 0, 1, 1, pfb0);

    old_ops = platform->ops;
    test_external_resource_bus_reset_load_record();
    platform->ops = &s_resource_platform_ops;

    egui_api_load_external_resource(&core.canvas, dest, (egui_uintptr_t)src, 1U, sizeof(dest));

    load_count = s_load_record.load_count;
    wait_count = s_display_record.wait_count;
    bus_locked = s_load_record.bus_locked;
    bus_locked_after = core.render.pfb_mgr.bus_locked;
    dma_busy_after_load = core.render.pfb_mgr.dma_busy;
    platform->ops = old_ops;
    test_external_resource_bus_finish_pending_flush(&core);

    EGUI_TEST_ASSERT_EQUAL_INT(1, load_count);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_EXTERNAL_RESOURCE_BUS_EXPECT_LOCK, bus_locked);
    EGUI_TEST_ASSERT_EQUAL_INT(0, bus_locked_after);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_EXTERNAL_RESOURCE_BUS_EXPECT_LOCK, wait_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_CONFIG_EXTERNAL_RESOURCE_SHARED_DISPLAY_BUS ? 0 : 1, dma_busy_after_load);
    EGUI_TEST_ASSERT_TRUE(memcmp(dest, src + 1, sizeof(dest)) == 0);
}

static void test_pfb_bus_lock_blocks_submit_until_release(void)
{
    egui_core_t core;
    egui_display_driver_t driver;
    egui_color_int_t pfb0[TEST_EXTERNAL_RESOURCE_BUS_PFB_PIXELS];
    egui_color_int_t pfb1[TEST_EXTERNAL_RESOURCE_BUS_PFB_PIXELS];

    test_external_resource_bus_reset_display_record();
    test_external_resource_bus_init_local_core(&core, pfb0, pfb1, &driver);

    egui_pfb_manager_submit(&core.render.pfb_mgr, 0, 0, 1, 1, pfb0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_display_record.draw_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core.render.pfb_mgr.dma_busy);

    egui_pfb_manager_bus_acquire(&core.render.pfb_mgr);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_display_record.wait_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core.render.pfb_mgr.bus_locked);
    EGUI_TEST_ASSERT_EQUAL_INT(0, core.render.pfb_mgr.pending_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, core.render.pfb_mgr.dma_busy);

    egui_pfb_manager_submit(&core.render.pfb_mgr, 4, 0, 1, 1, pfb1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_display_record.draw_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core.render.pfb_mgr.pending_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, core.render.pfb_mgr.dma_busy);

    egui_pfb_manager_bus_release(&core.render.pfb_mgr);
    EGUI_TEST_ASSERT_EQUAL_INT(2, s_display_record.draw_count);
    EGUI_TEST_ASSERT_EQUAL_INT(4, s_display_record.draw_x[1]);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_display_record.draw_bus_locked[1]);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core.render.pfb_mgr.dma_busy);

    test_external_resource_bus_finish_pending_flush(&core);
    EGUI_TEST_ASSERT_EQUAL_INT(0, core.render.pfb_mgr.pending_count);
}

static void test_pfb_bus_lock_prevents_autochain_after_completion(void)
{
    egui_core_t core;
    egui_display_driver_t driver;
    egui_color_int_t pfb0[TEST_EXTERNAL_RESOURCE_BUS_PFB_PIXELS];
    egui_color_int_t pfb1[TEST_EXTERNAL_RESOURCE_BUS_PFB_PIXELS];

    test_external_resource_bus_reset_display_record();
    test_external_resource_bus_init_local_core(&core, pfb0, pfb1, &driver);

    egui_pfb_manager_submit(&core.render.pfb_mgr, 0, 0, 1, 1, pfb0);
    egui_pfb_manager_submit(&core.render.pfb_mgr, 4, 0, 1, 1, pfb1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_display_record.draw_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, core.render.pfb_mgr.pending_count);

    egui_pfb_manager_bus_acquire(&core.render.pfb_mgr);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_display_record.wait_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_display_record.draw_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core.render.pfb_mgr.bus_locked);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core.render.pfb_mgr.pending_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, core.render.pfb_mgr.dma_busy);

    egui_pfb_manager_bus_release(&core.render.pfb_mgr);
    EGUI_TEST_ASSERT_EQUAL_INT(2, s_display_record.draw_count);
    EGUI_TEST_ASSERT_EQUAL_INT(4, s_display_record.draw_x[1]);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core.render.pfb_mgr.dma_busy);

    test_external_resource_bus_finish_pending_flush(&core);
    EGUI_TEST_ASSERT_EQUAL_INT(0, core.render.pfb_mgr.pending_count);
}

static void test_pfb_bus_acquire_accepts_isr_completion_during_wait(void)
{
    egui_core_t core;
    egui_display_driver_t driver;
    egui_color_int_t pfb0[TEST_EXTERNAL_RESOURCE_BUS_PFB_PIXELS];
    egui_color_int_t pfb1[TEST_EXTERNAL_RESOURCE_BUS_PFB_PIXELS];

    test_external_resource_bus_reset_display_record();
    s_display_record.wait_notifies = 1;
    test_external_resource_bus_init_local_core(&core, pfb0, pfb1, &driver);

    egui_pfb_manager_submit(&core.render.pfb_mgr, 0, 0, 1, 1, pfb0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core.render.pfb_mgr.pending_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core.render.pfb_mgr.dma_busy);

    egui_pfb_manager_bus_acquire(&core.render.pfb_mgr);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_display_record.wait_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core.render.pfb_mgr.bus_locked);
    EGUI_TEST_ASSERT_EQUAL_INT(0, core.render.pfb_mgr.pending_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, core.render.pfb_mgr.dma_busy);

    egui_pfb_manager_bus_release(&core.render.pfb_mgr);
    EGUI_TEST_ASSERT_EQUAL_INT(0, core.render.pfb_mgr.bus_locked);
}

void test_external_resource_bus_run(void)
{
    EGUI_TEST_SUITE_BEGIN(external_resource_bus);

    EGUI_TEST_RUN(test_external_resource_load_canvas_uses_configured_bus_policy);
    EGUI_TEST_RUN(test_external_resource_load_core_uses_configured_bus_policy);
    EGUI_TEST_RUN(test_external_resource_load_null_core_keeps_unlocked_compatibility);
    EGUI_TEST_RUN(test_external_svg_resource_load_requires_core_context);
    EGUI_TEST_RUN(test_external_svg_resource_load_uses_core_bus_lock);
    EGUI_TEST_RUN(test_external_resource_load_missing_callback_is_noop);
    EGUI_TEST_RUN(test_external_resource_load_waits_pending_flush_before_read);
    EGUI_TEST_RUN(test_pfb_bus_lock_blocks_submit_until_release);
    EGUI_TEST_RUN(test_pfb_bus_lock_prevents_autochain_after_completion);
    EGUI_TEST_RUN(test_pfb_bus_acquire_accepts_isr_completion_during_wait);

    EGUI_TEST_SUITE_END();
}
