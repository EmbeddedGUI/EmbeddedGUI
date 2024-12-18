#ifndef _SIMPLE_DATA_RINGBUFFER_H_
#define _SIMPLE_DATA_RINGBUFFER_H_

#include <stdint.h>
#include <stddef.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Define a Memory RINGBUF thread safe, and can full use pool.
 * @details API 1 and 2.
 *   read_index and write_index is normal process.
 *   unhandle_index for spec use.
 *   Thread-A will update read_index, for get buffer from RINGBUF.
 *   Thread-B will update write_index, for put buffer to RINGBUF
 *               update unhandle_index, for use the buffer Thread-A had processed.
 */
typedef struct simple_data_ringbuffer
{
    uint16_t total_size;  /* Number of buffers */
    uint16_t item_size;   /* Stride between elements */
    uint16_t read_index;  /* Read. Read index */
    uint16_t write_index; /* Write. Write index */
    uint8_t *buffer;
} simple_data_ringbuffer_t;

#ifndef MROUND
/**
 * @brief Round up to nearest multiple of 4, for unsigned integers
 * @details
 *   The addition of 3 forces x into the next multiple of 4. This is responsible
 *   for the rounding in the the next step, to be Up.
 *   For ANDing of ~3: We observe y & (~3) == (y>>2)<<2, and we recognize
 *   (y>>2) as a floored division, which is almost undone by the left-shift. The
 *   flooring can't be undone so have achieved a rounding.
 *
 *   Examples:
 *    MROUND( 0) =  0
 *    MROUND( 1) =  4
 *    MROUND( 2) =  4
 *    MROUND( 3) =  4
 *    MROUND( 4) =  4
 *    MROUND( 5) =  8
 *    MROUND( 8) =  8
 *    MROUND( 9) = 12
 *    MROUND(13) = 16
 */
#define MROUND(x) (((uint32_t)(x) + 3) & (~((uint32_t)3)))
#endif

#define SIMPLE_DATA_RINGBUFFER_DEFINE(_name, _num, _data_size)                                                                                                 \
    static uint8_t _name##_data_storage[_num][MROUND(_data_size)];                                                                                             \
    static simple_data_ringbuffer_t _name = {                                                                                                                  \
            .total_size = _num, .item_size = _data_size, .write_index = 0, .read_index = 0, .buffer = (void *)_name##_data_storage}

#define SIMPLE_DATA_RINGBUFFER_INIT(_name, _num, _data_size) simple_data_ringbuffer_init(&_name, _num, MROUND(_data_size), (void *)_name##_data_storage)

/**
 * @brief  Returns the size of the RINGBUF in bytes.
 * @param  [in] ringbuf: The ringbuf to be used.
 * @return The size of the RINGBUF.
 */
static inline uint32_t simple_data_ringbuffer_total_size(simple_data_ringbuffer_t *ringbuf)
{
    return ringbuf->total_size;
}

/**
 * @brief  Returns the item size of the RINGBUF in bytes.
 * @param  [in] ringbuf: The ringbuf to be used.
 * @return The item size of the RINGBUF.
 */
static inline uint32_t simple_data_ringbuffer_item_size(simple_data_ringbuffer_t *ringbuf)
{
    return ringbuf->item_size;
}

/**
 * @brief  Reset the RINGBUF.
 * @param  [in] ringbuf: The ringbuf to be used.
 */
static inline void simple_data_ringbuffer_reset(simple_data_ringbuffer_t *ringbuf)
{
    ringbuf->write_index = 0;
    ringbuf->read_index = 0;
}

/**
 * @brief  Initialize the RINGBUF.
 * @param  [in] ringbuf: The ringbuf to be used.
 * @param  [in] total_size: The total size of the RINGBUF.
 * @param  [in] buffer: The buffer to be used.
 */
static inline void simple_data_ringbuffer_init(simple_data_ringbuffer_t *ringbuf, uint16_t total_size, uint16_t item_size, void *buffer)
{
    ringbuf->total_size = total_size;
    ringbuf->item_size = item_size;
    ringbuf->write_index = 0;
    ringbuf->read_index = 0;
    ringbuf->buffer = buffer;
}

/**
 * @brief  Check if the RINGBUF is empty.
 * @param  [in] ringbuf: The ringbuf to be used.
 * @return 1 if the RINGBUF is empty, 0 otherwise.
 */
static inline int simple_data_ringbuffer_is_empty(simple_data_ringbuffer_t *ringbuf)
{
    return ringbuf->read_index == ringbuf->write_index;
}

/**
 * @brief  Returns the used size of the RINGBUF in bytes.
 * @param  [in] ringbuf: The ringbuf to be used.
 * @return The used size of the RINGBUF in bytes.
 */
static inline uint16_t simple_data_ringbuffer_size(simple_data_ringbuffer_t *ringbuf)
{
    return ringbuf->write_index >= ringbuf->read_index ? ringbuf->write_index - ringbuf->read_index
                                                       : (ringbuf->total_size << 1) - (ringbuf->read_index - ringbuf->write_index);
}

/**
 * @brief  Returns the free size of the RINGBUF in bytes.
 * @param  [in] ringbuf: The ringbuf to be used.
 * @return The free size of the RINGBUF in bytes.
 */
static inline uint16_t simple_data_ringbuffer_reserve_size(simple_data_ringbuffer_t *ringbuf)
{
    return ringbuf->total_size - simple_data_ringbuffer_size(ringbuf);
}

/**
 * @brief  Check if the RINGBUF is full.
 * @param  [in] ringbuf: The ringbuf to be used.
 */
static inline int simple_data_ringbuffer_is_full(simple_data_ringbuffer_t *ringbuf)
{
    return simple_data_ringbuffer_size(ringbuf) == ringbuf->total_size;
}

/**
 * @brief  Put data into the RINGBUF.
 * @param  [in] ringbuf: The ringbuf to be used.
 * @param  [in] buffer: The buffer to be put into the RINGBUF.
 * @return The length of the buffer put into the RINGBUF.
 */
int simple_data_ringbuffer_put(simple_data_ringbuffer_t *ringbuf, void *buffer);

/**
 * @brief  Get data from the RINGBUF.
 * @param  [in] ringbuf: The ringbuf to be used.
 * @param  [in] buffer: The buffer to be put into the RINGBUF.
 * @return The length of the buffer get from the RINGBUF.
 */
int simple_data_ringbuffer_get(simple_data_ringbuffer_t *ringbuf, void *buffer);

/**
 * @brief   Non-destructive: Allocate buffer from named queue
 * @details API 1.
 *   The allocated buffer exists in limbo until committed.
 *   To commit the enqueue process, simple_data_ringbuffer_enqueue() must be called afterwards
 * @return  Index of newly allocated buffer; only valid if mem != NULL
 */
int simple_data_ringbuffer_enqueue_get(simple_data_ringbuffer_t *ringbuf, void **mem);

/**
 * @brief   Atomically commit a previously allocated buffer
 * @details API 1
 *   The buffer should have been allocated using MRINGBUF_ENQUEUE_GET
 * @param idx[in]  Index one-ahead of previously allocated buffer
 */
void simple_data_ringbuffer_enqueue(simple_data_ringbuffer_t *ringbuf, uint16_t write_index);

/**
 * @brief  Peek data from the RINGBUF, but not dequeue.
 * @param  [in] ringbuf: The ringbuf to be used.
 * @return The length of the buffer get from the RINGBUF.
 */
void *simple_data_ringbuffer_dequeue_peek(simple_data_ringbuffer_t *ringbuf);

/**
 * @brief  Dequeue data from the RINGBUF.
 * @param  [in] ringbuf: The ringbuf to be used.
 */
void simple_data_ringbuffer_dequeue(simple_data_ringbuffer_t *ringbuf);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _SIMPLE_DATA_RINGBUFFER_H_ */
