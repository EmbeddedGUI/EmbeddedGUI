#include "egui_sprintf.h"

int egui_sprintf_int(char *buf, int buf_size, int val)
{
    int pos = 0;
    char tmp[12];
    int tmp_len = 0;
    int v = val;

    if (buf_size < 2)
    {
        if (buf_size > 0)
        {
            buf[0] = '\0';
        }
        return 0;
    }

    if (v < 0)
    {
        buf[pos++] = '-';
        v = -v;

        // Handle INT_MIN overflow
        if (v < 0)
        {
            const char *s = "-2147483648";
            pos = 0;
            while (*s && pos < buf_size - 1)
            {
                buf[pos++] = *s++;
            }
            buf[pos] = '\0';
            return pos;
        }
    }

    // Extract digits in reverse
    if (v == 0)
    {
        tmp[tmp_len++] = '0';
    }
    else
    {
        while (v > 0 && tmp_len < (int)sizeof(tmp))
        {
            tmp[tmp_len++] = '0' + (v % 10);
            v /= 10;
        }
    }

    // Reverse copy to buf
    for (int i = tmp_len - 1; i >= 0 && pos < buf_size - 1; i--)
    {
        buf[pos++] = tmp[i];
    }
    buf[pos] = '\0';
    return pos;
}

int egui_sprintf_int_pad(char *buf, int buf_size, int val, int width, char pad_char)
{
    int pos = 0;
    char tmp[12];
    int tmp_len = 0;
    int v = val;
    int is_neg = 0;

    if (buf_size < 2)
    {
        if (buf_size > 0)
        {
            buf[0] = '\0';
        }
        return 0;
    }

    if (v < 0)
    {
        is_neg = 1;
        v = -v;
        if (v < 0)
        {
            return egui_sprintf_int(buf, buf_size, val);
        }
    }

    // Extract digits in reverse
    if (v == 0)
    {
        tmp[tmp_len++] = '0';
    }
    else
    {
        while (v > 0 && tmp_len < (int)sizeof(tmp))
        {
            tmp[tmp_len++] = '0' + (v % 10);
            v /= 10;
        }
    }

    int digit_len = tmp_len + is_neg;
    int pad_count = (width > digit_len) ? (width - digit_len) : 0;

    // For zero-padding with negative: sign before zeros
    if (is_neg && pad_char == '0')
    {
        if (pos < buf_size - 1)
        {
            buf[pos++] = '-';
        }
        is_neg = 0;
    }

    // Write padding
    for (int i = 0; i < pad_count && pos < buf_size - 1; i++)
    {
        buf[pos++] = pad_char;
    }

    // Write sign if not yet written (space-padding case)
    if (is_neg && pos < buf_size - 1)
    {
        buf[pos++] = '-';
    }

    // Reverse copy digits to buf
    for (int i = tmp_len - 1; i >= 0 && pos < buf_size - 1; i--)
    {
        buf[pos++] = tmp[i];
    }
    buf[pos] = '\0';
    return pos;
}

int egui_sprintf_str(char *buf, int buf_size, const char *str)
{
    int pos = 0;

    if (buf_size < 1 || str == (void *)0)
    {
        if (buf_size > 0)
        {
            buf[0] = '\0';
        }
        return 0;
    }

    while (*str && pos < buf_size - 1)
    {
        buf[pos++] = *str++;
    }
    buf[pos] = '\0';
    return pos;
}

int egui_sprintf_char(char *buf, int buf_size, char c)
{
    if (buf_size < 2)
    {
        if (buf_size > 0)
        {
            buf[0] = '\0';
        }
        return 0;
    }

    buf[0] = c;
    buf[1] = '\0';
    return 1;
}
