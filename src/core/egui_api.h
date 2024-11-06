#ifndef _EGUI_API_H_
#define _EGUI_API_H_

#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif


#define EGUI_LOG_IMPL_LEVEL_NONE 0U
#define EGUI_LOG_IMPL_LEVEL_ERR  1U
#define EGUI_LOG_IMPL_LEVEL_WRN  2U
#define EGUI_LOG_IMPL_LEVEL_INF  3U
#define EGUI_LOG_IMPL_LEVEL_DBG  4U

#define EGUI_LOG_LEVEL EGUI_CONFIG_DEBUG_LOG_LEVEL
// #define EGUI_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_DBG


__EGUI_STATIC_INLINE__ char egui_level_to_char(int level)
{
    switch (level)
    {
    case EGUI_LOG_IMPL_LEVEL_ERR:
        return 'E';
    case EGUI_LOG_IMPL_LEVEL_WRN:
        return 'W';
    case EGUI_LOG_IMPL_LEVEL_INF:
        return 'I';
    case EGUI_LOG_IMPL_LEVEL_DBG:
        return 'D';
    default:
        return '?';
    }
}

#define __EGUI_LOG_IMPL_TO_PRINT(_fun, _line, _level, fmt, ...)                                   \
    do                                                                                             \
    {                                                                                              \
        egui_api_log("%c: "                                                                      \
                           "%s():%d: " fmt,                                                   \
                           egui_level_to_char(_level), _fun, _line,               \
                           ##__VA_ARGS__);                                                         \
    } while (false);

#if EGUI_CONFIG_DEBUG_LOG_SIMPLE
#define __EGUI_LOG_IMPL(_level, fmt, ...)                                                \
    egui_api_log(fmt, ##__VA_ARGS__)
#else
#define __EGUI_LOG_IMPL(_level, ...)                                                \
    __EGUI_LOG_IMPL_TO_PRINT(__func__, __LINE__, _level, __VA_ARGS__)
#endif

#define EGUI_LOG_IMPL_ERR(...) __EGUI_LOG_IMPL(EGUI_LOG_IMPL_LEVEL_ERR, __VA_ARGS__)
#define EGUI_LOG_IMPL_WRN(...) __EGUI_LOG_IMPL(EGUI_LOG_IMPL_LEVEL_WRN, __VA_ARGS__)
#define EGUI_LOG_IMPL_INF(...) __EGUI_LOG_IMPL(EGUI_LOG_IMPL_LEVEL_INF, __VA_ARGS__)
#define EGUI_LOG_IMPL_DBG(...) __EGUI_LOG_IMPL(EGUI_LOG_IMPL_LEVEL_DBG, __VA_ARGS__)


#if EGUI_LOG_LEVEL >= EGUI_LOG_IMPL_LEVEL_ERR
#define EGUI_LOG_ERR(fmt, ...)  EGUI_LOG_IMPL_ERR(fmt, ##__VA_ARGS__)
#else
#define EGUI_LOG_ERR(fmt, ...)
#endif
#if EGUI_LOG_LEVEL >= EGUI_LOG_IMPL_LEVEL_WRN
#define EGUI_LOG_WRN(fmt, ...)  EGUI_LOG_IMPL_WRN(fmt, ##__VA_ARGS__)
#else
#define EGUI_LOG_WRN(fmt, ...)
#endif
#if EGUI_LOG_LEVEL >= EGUI_LOG_IMPL_LEVEL_INF
#define EGUI_LOG_INF(fmt, ...)  EGUI_LOG_IMPL_INF(fmt, ##__VA_ARGS__)
#else
#define EGUI_LOG_INF(fmt, ...)
#endif
#if EGUI_LOG_LEVEL >= EGUI_LOG_IMPL_LEVEL_DBG
#define EGUI_LOG_DBG(fmt, ...)  EGUI_LOG_IMPL_DBG(fmt, ##__VA_ARGS__)
#else
#define EGUI_LOG_DBG(fmt, ...)
#endif




#define EGUI_ASSERT(condition)                                                                                                                                 \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (!(condition))                                                                                                                                      \
            egui_api_assert(__FILE__, __LINE__);                                                                                                                   \
    } while (0)


    
typedef int egui_base_t;
extern egui_base_t egui_hw_interrupt_disable(void);
extern void egui_hw_interrupt_enable(egui_base_t level);

#define __egui_disable_isr() egui_base_t _egui_isr_level = egui_hw_interrupt_disable()
#define __egui_enable_isr()  egui_hw_interrupt_enable(_egui_isr_level)

void egui_api_log(const char *format, ...);
void egui_api_assert(const char *file, int line);
void egui_api_free(void *ptr);
void *egui_api_malloc(int size);
void egui_api_sprintf(char *str, const char *format, ...);
void egui_api_draw_data(int16_t x, int16_t y, int16_t width, int16_t height, const egui_color_int_t *data);
void egui_api_refresh_display(void);
void egui_api_timer_start(uint32_t ms);
void egui_api_timer_stop(void);
uint32_t egui_api_timer_get_current(void);
void egui_api_delay(uint32_t ms);
void egui_api_pfb_clear(void *s, int n);
void egui_api_load_external_resource(void *dest, const uint32_t res_id, uint32_t start_offset, uint32_t size);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_API_H_ */
