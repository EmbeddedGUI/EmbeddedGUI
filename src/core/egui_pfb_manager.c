#include "egui_pfb_manager.h"
#include "egui_api.h"
#include "egui_display_driver.h"
#include "egui_platform.h"

void egui_pfb_manager_init(egui_pfb_manager_t *mgr, egui_color_int_t *pfb, int16_t width, int16_t height, int color_bytes)
{
    int i;
    for (i = 0; i < EGUI_PFB_BUFFER_MAX; i++)
    {
        mgr->buffers[i] = NULL;
    }
    mgr->buffers[0] = pfb;
    mgr->buffer_count = 1;
    mgr->width = width;
    mgr->height = height;
    mgr->buffer_size_bytes = width * height * color_bytes;
    mgr->render_idx = 0;
    mgr->flush_idx = 0;
    mgr->pending_count = 0;
    mgr->dma_busy = 0;
    mgr->bus_locked = 0;
}

void egui_pfb_manager_add_buffer(egui_pfb_manager_t *mgr, egui_color_int_t *buf)
{
    if (mgr->buffer_count >= EGUI_PFB_BUFFER_MAX || buf == NULL)
    {
        return;
    }
    mgr->buffers[mgr->buffer_count] = buf;
    mgr->buffer_count++;
}

void egui_pfb_manager_set_backup_buffer(egui_pfb_manager_t *mgr, egui_color_int_t *backup)
{
    egui_pfb_manager_add_buffer(mgr, backup);
}

egui_color_int_t *egui_pfb_manager_get_active(egui_pfb_manager_t *mgr)
{
    return mgr->buffers[mgr->render_idx];
}

egui_color_int_t *egui_pfb_manager_get_render_buffer(egui_pfb_manager_t *mgr)
{
    if (mgr->buffer_count <= 1)
    {
        return mgr->buffers[0];
    }

    // Ring full: all buffers occupied by pending/sending data.
    // Wait for DMA to free one.
    while (mgr->pending_count >= mgr->buffer_count)
    {
        // Try polling: use wait_draw_complete + manual notify for non-interrupt mode
        if (mgr->dma_busy)
        {
            egui_display_driver_t *drv = egui_display_driver_get();
            if (drv != NULL && drv->ops->wait_draw_complete != NULL)
            {
                drv->ops->wait_draw_complete();
            }
            if (mgr->dma_busy)
            {
                egui_pfb_manager_notify_flush_complete(mgr);
            }
        }
    }

    return mgr->buffers[mgr->render_idx];
}

/**
 * Start DMA for the buffer at flush_idx.
 * Called from submit() or from notify_flush_complete() to chain next transfer.
 */
static void egui_pfb_manager_start_flush(egui_pfb_manager_t *mgr)
{
    egui_display_driver_t *drv = egui_display_driver_get();
    if (drv == NULL || drv->ops->draw_area_async == NULL)
    {
        return;
    }

    egui_pfb_flush_params_t *p = &mgr->flush_params[mgr->flush_idx];
    mgr->dma_busy = 1;
    drv->ops->draw_area_async(p->x, p->y, p->w, p->h, mgr->buffers[mgr->flush_idx]);
}

void egui_pfb_manager_submit(egui_pfb_manager_t *mgr, int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    egui_display_driver_t *drv = egui_display_driver_get();

    // Single buffer: synchronous draw
    if (mgr->buffer_count <= 1 || drv == NULL || drv->ops->draw_area_async == NULL)
    {
        if (drv != NULL && drv->ops->draw_area != NULL)
        {
            drv->ops->draw_area(x, y, w, h, data);
        }
        return;
    }

    // Multi-buffer async path: store params and enqueue
    egui_pfb_flush_params_t *p = &mgr->flush_params[mgr->render_idx];
    p->x = x;
    p->y = y;
    p->w = w;
    p->h = h;

    // Advance render index
    mgr->render_idx = (mgr->render_idx + 1) % mgr->buffer_count;

    // Atomically increment pending count
    egui_base_t level = egui_hw_interrupt_disable();
    mgr->pending_count++;
    uint8_t was_idle = !mgr->dma_busy;
    uint8_t locked = mgr->bus_locked;
    egui_hw_interrupt_enable(level);

    // If DMA was idle and bus is not locked, start flushing
    if (was_idle && !locked)
    {
        egui_pfb_manager_start_flush(mgr);
    }
}

void egui_pfb_manager_notify_flush_complete(egui_pfb_manager_t *mgr)
{
    // Advance flush index
    mgr->flush_idx = (mgr->flush_idx + 1) % mgr->buffer_count;
    mgr->pending_count--;

    // Chain next transfer if more pending and bus is not locked
    if (mgr->pending_count > 0 && !mgr->bus_locked)
    {
        egui_pfb_manager_start_flush(mgr);
    }
    else
    {
        mgr->dma_busy = 0;
    }
}

void egui_pfb_manager_wait_all_complete(egui_pfb_manager_t *mgr)
{
    while (mgr->pending_count > 0 || mgr->dma_busy)
    {
        // Try polling: use wait_draw_complete + manual notify for non-interrupt mode
        if (mgr->dma_busy)
        {
            egui_display_driver_t *drv = egui_display_driver_get();
            if (drv != NULL && drv->ops->wait_draw_complete != NULL)
            {
                drv->ops->wait_draw_complete();
            }
            if (mgr->dma_busy)
            {
                egui_pfb_manager_notify_flush_complete(mgr);
            }
        }
    }
}

int egui_pfb_manager_is_async(egui_pfb_manager_t *mgr)
{
    if (mgr->buffer_count < 2)
    {
        return 0;
    }
    egui_display_driver_t *drv = egui_display_driver_get();
    return (drv != NULL && drv->ops->draw_area_async != NULL) ? 1 : 0;
}

void egui_pfb_manager_swap(egui_pfb_manager_t *mgr)
{
    // Legacy no-op: multi-buffer mode uses submit/notify instead.
    // For single buffer, nothing to swap.
}

void egui_pfb_manager_bus_acquire(egui_pfb_manager_t *mgr)
{
    // Set locked flag first to prevent new DMA starts from ISR
    mgr->bus_locked = 1;

    // Wait for any in-progress DMA transfer to complete
    if (mgr->dma_busy)
    {
        // Use display driver's wait callback (works for both interrupt and polling modes)
        egui_display_driver_t *drv = egui_display_driver_get();
        if (drv != NULL && drv->ops->wait_draw_complete != NULL)
        {
            drv->ops->wait_draw_complete();
        }

        // In polling mode (no ISR), dma_busy is still set.
        // Manually advance the ring buffer.
        if (mgr->dma_busy)
        {
            // notify_flush_complete sees bus_locked, won't chain next transfer
            egui_pfb_manager_notify_flush_complete(mgr);
        }
    }
}

void egui_pfb_manager_bus_release(egui_pfb_manager_t *mgr)
{
    mgr->bus_locked = 0;

    // If tiles were queued during the lock period, start flushing now
    if (mgr->pending_count > 0 && !mgr->dma_busy)
    {
        egui_pfb_manager_start_flush(mgr);
    }
}
