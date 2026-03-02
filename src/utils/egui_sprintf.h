#ifndef _EGUI_SPRINTF_H_
#define _EGUI_SPRINTF_H_

#include <stdint.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write an integer to buffer in decimal.
 *
 * @param buf       destination buffer
 * @param buf_size  total buffer size including null terminator
 * @param val       integer value (handles negative)
 * @return int      number of characters written (excluding null terminator)
 */
int egui_sprintf_int(char *buf, int buf_size, int val);

/**
 * @brief Write an integer to buffer with padding to a minimum width.
 *
 * @param buf       destination buffer
 * @param buf_size  total buffer size including null terminator
 * @param val       integer value (handles negative)
 * @param width     minimum field width
 * @param pad_char  padding character ('0' for zero-pad, ' ' for space-pad)
 * @return int      number of characters written (excluding null terminator)
 */
int egui_sprintf_int_pad(char *buf, int buf_size, int val, int width, char pad_char);

/**
 * @brief Write a null-terminated string to buffer.
 *
 * @param buf       destination buffer
 * @param buf_size  total buffer size including null terminator
 * @param str       source string
 * @return int      number of characters written (excluding null terminator)
 */
int egui_sprintf_str(char *buf, int buf_size, const char *str);

/**
 * @brief Write a single character to buffer.
 *
 * @param buf       destination buffer
 * @param buf_size  total buffer size including null terminator
 * @param c         character to write
 * @return int      number of characters written (0 or 1)
 */
int egui_sprintf_char(char *buf, int buf_size, char c);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_SPRINTF_H_ */
