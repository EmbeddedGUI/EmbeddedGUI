#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "simple_ringbuffer.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define RINGBUFFER_INDEX_TO_PTR(_index, _total_size) ((_index >= _total_size) ? (_index - _total_size) : (_index))

uint32_t simple_ringbuffer_put(simple_ringbuffer_t *ringbuf, uint8_t *buffer, uint32_t len)
{
    uint32_t l;
    uint32_t write_index;
    uint32_t wptr = RINGBUFFER_INDEX_TO_PTR(ringbuf->write_index, ringbuf->total_size);

    len = MIN(len, simple_ringbuffer_reserve_size(ringbuf));

    /* first put the data starting from ringbuf->write_index to buffer end */
    l = MIN(len, ringbuf->total_size - wptr);
    memcpy(ringbuf->buffer + wptr, buffer, l);

    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(ringbuf->buffer, buffer + l, len - l);

    write_index = ringbuf->write_index + len;
    if (write_index >= (ringbuf->total_size << 1))
    {
        write_index -= (ringbuf->total_size << 1);
    }
    ringbuf->write_index = write_index;

    return len;
}

uint32_t simple_ringbuffer_get(simple_ringbuffer_t *ringbuf, uint8_t *buffer, uint32_t len)
{
    uint32_t l;
    uint32_t read_index;
    uint32_t rptr = RINGBUFFER_INDEX_TO_PTR(ringbuf->read_index, ringbuf->total_size);

    len = MIN(len, simple_ringbuffer_size(ringbuf));

    /* first get the data from ringbuf->read_index until the end of the buffer */
    l = MIN(len, ringbuf->total_size - rptr);
    memcpy(buffer, ringbuf->buffer + rptr, l);

    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(buffer + l, ringbuf->buffer, len - l);

    read_index = ringbuf->read_index + len;
    if (read_index >= (ringbuf->total_size << 1))
    {
        read_index -= (ringbuf->total_size << 1);
    }
    ringbuf->read_index = read_index;

    return len;
}
