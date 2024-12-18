#include <string.h>

#include "simple_data_ringbuffer.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define DATA_RINGBUFFER_INDEX_TO_PTR(_index, _total_size) ((_index >= _total_size) ? (_index - _total_size) : (_index))

int simple_data_ringbuffer_put(simple_data_ringbuffer_t *ringbuf, void *buffer)
{
    uint16_t write_index;
    uint16_t wptr;

    if (simple_data_ringbuffer_reserve_size(ringbuf) == 0)
    {
        return 0;
    }

    wptr = DATA_RINGBUFFER_INDEX_TO_PTR(ringbuf->write_index, ringbuf->total_size);
    memcpy(ringbuf->buffer + wptr * ringbuf->item_size, buffer, ringbuf->item_size);

    write_index = ringbuf->write_index + 1;
    if (write_index >= (ringbuf->total_size << 1))
    {
        write_index -= (ringbuf->total_size << 1);
    }
    ringbuf->write_index = write_index;

    return 1;
}

int simple_data_ringbuffer_get(simple_data_ringbuffer_t *ringbuf, void *buffer)
{
    uint16_t read_index;
    uint16_t rptr;
    if (simple_data_ringbuffer_size(ringbuf) == 0)
    {
        return 0;
    }

    if (buffer != NULL)
    {
        rptr = DATA_RINGBUFFER_INDEX_TO_PTR(ringbuf->read_index, ringbuf->total_size);
        memcpy(buffer, ringbuf->buffer + rptr * ringbuf->item_size, ringbuf->item_size);
    }

    read_index = ringbuf->read_index + 1;
    if (read_index >= (ringbuf->total_size << 1))
    {
        read_index -= (ringbuf->total_size << 1);
    }
    ringbuf->read_index = read_index;

    return 1;
}

int simple_data_ringbuffer_enqueue_get(simple_data_ringbuffer_t *ringbuf, void **mem)
{
    uint16_t wptr = DATA_RINGBUFFER_INDEX_TO_PTR(ringbuf->write_index, ringbuf->total_size);

    if (simple_data_ringbuffer_reserve_size(ringbuf) == 0)
    {
        /* Buffer could not be allocated */
        *mem = NULL; /* Signal the failure */
        return 0;    // full
    }

    /* We keep idx as the always-one-free, so we return preceding
     * buffer (last). Recall that last has not been updated,
     * so idx != last
     */
    *mem = ringbuf->buffer + wptr * ringbuf->item_size; /* preceding buffer */

    uint16_t write_index = ringbuf->write_index + 1;
    if (write_index >= (ringbuf->total_size << 1))
    {
        write_index -= (ringbuf->total_size << 1);
    }

    return write_index;
}

void simple_data_ringbuffer_enqueue(simple_data_ringbuffer_t *ringbuf, uint16_t write_index)
{
    ringbuf->write_index = write_index; /* Commit: Update write index */
}

void *simple_data_ringbuffer_dequeue_peek(simple_data_ringbuffer_t *ringbuf)
{
    uint16_t read_index;
    uint16_t rptr;
    if (simple_data_ringbuffer_size(ringbuf) == 0)
    {
        return NULL;
    }

    rptr = DATA_RINGBUFFER_INDEX_TO_PTR(ringbuf->read_index, ringbuf->total_size);
    return ringbuf->buffer + rptr * ringbuf->item_size;
}

void simple_data_ringbuffer_dequeue(simple_data_ringbuffer_t *ringbuf)
{
    simple_data_ringbuffer_get(ringbuf, NULL);
}
