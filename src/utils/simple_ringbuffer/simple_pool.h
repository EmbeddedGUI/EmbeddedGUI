#ifndef _SIMPLE_POOL_H_
#define _SIMPLE_POOL_H_

#include "simple_data_ringbuffer.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct simple_pool
{
    simple_data_ringbuffer_t ringbuf;
    uint16_t item_size;
} simple_pool_t;

#define SIMPLE_POOL_ENQUEUE(_spool, _val) simple_data_ringbuffer_put(&(_spool)->ringbuf, (void *)&(_val))

#define SIMPLE_POOL_DEQUEUE(_spool, _val) simple_data_ringbuffer_get(&(_spool)->ringbuf, (void *)&(_val))

#define SIMPLE_POOL_IS_EMPTY(_spool) simple_data_ringbuffer_is_empty(&(_spool)->ringbuf)

#define SIMPLE_POOL_IS_FULL(_spool) simple_data_ringbuffer_is_full(&(_spool)->ringbuf)

#define SIMPLE_POOL_SIZE(_spool) simple_data_ringbuffer_size(&(_spool)->ringbuf)

#define SIMPLE_POOL_RESERVE_SIZE(_spool) simple_data_ringbuffer_reserve_size(&(_spool)->ringbuf)

#define SIMPLE_POOL_TOTAL_CNT(_spool) simple_data_ringbuffer_total_size(&(_spool)->ringbuf)

#define SIMPLE_POOL_ITEM_SIZE(_spool) (_spool)->item_size

#define SIMPLE_POOL_DEFINE(_name, _num, _data_size)                                                                                                            \
    static simple_pool_t _name;                                                                                                                                \
    static void *_name##_fifo_storage[_num];                                                                                                                   \
    static uint8_t _name##_data_storage[_num][MROUND(_data_size)];

#define SIMPLE_POOL_INIT(_name, _num, _data_size) simple_pool_init(&_name, _name##_fifo_storage, (uint8_t *)_name##_data_storage, _num, _data_size)

static inline void simple_pool_init(simple_pool_t *spool, void **fifo_storage, uint8_t *data_storage, uint16_t n, uint16_t data_item_size)
{
    spool->item_size = data_item_size;

    // in 32 system, ptr is 32bit.
    simple_data_ringbuffer_init(&spool->ringbuf, n, sizeof(void *), fifo_storage);
    for (int i = 0; i < n; i++)
    {
        void *data_item = (void *)(data_storage + MROUND(data_item_size) * i);
        SIMPLE_POOL_ENQUEUE(spool, data_item);
    }
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _SIMPLE_POOL_H_ */
