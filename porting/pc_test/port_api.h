#ifndef _PC_TEST_PORT_API_H_
#define _PC_TEST_PORT_API_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_port_alloc_stats
{
    uint32_t alloc_count;
    uint32_t free_count;
    uint32_t live_alloc_count;
    size_t alloc_bytes;
    size_t free_bytes;
    size_t live_bytes;
    size_t peak_live_bytes;
} egui_port_alloc_stats_t;

void egui_port_reset_alloc_stats(void);
void egui_port_get_alloc_stats(egui_port_alloc_stats_t *out_stats);

#ifdef __cplusplus
}
#endif

#endif /* _PC_TEST_PORT_API_H_ */
