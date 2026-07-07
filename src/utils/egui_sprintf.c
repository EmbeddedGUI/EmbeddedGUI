#include <stdarg.h>

#include "core/egui_api.h"
#include "egui_sprintf.h"

/* Tiny formatter used by egui_api_sprintf to avoid pulling full libc printf on small targets. */

static char *egui_api_sprintf_append_cstr(char *dst, const char *src)
{
    if (src == NULL)
    {
        src = "(null)";
    }

    while (*src != '\0')
    {
        *dst++ = *src++;
    }
    return dst;
}

static char *egui_api_sprintf_append_unsigned(char *dst, unsigned long value, int width, char pad_char)
{
    char tmp[24];
    int len = 0;
    int pad_count;

    if (value == 0UL)
    {
        tmp[len++] = '0';
    }
    else
    {
        while (value != 0UL)
        {
            tmp[len++] = (char)('0' + (value % 10UL));
            value /= 10UL;
        }
    }

    pad_count = width > len ? (width - len) : 0;
    while (pad_count-- > 0)
    {
        *dst++ = pad_char;
    }
    while (len-- > 0)
    {
        *dst++ = tmp[len];
    }
    return dst;
}

static char *egui_api_sprintf_append_signed(char *dst, long value, int width, char pad_char)
{
    unsigned long abs_value;

    if (value < 0)
    {
        if (pad_char == '0')
        {
            *dst++ = '-';
            width--;
        }
        abs_value = (unsigned long)(-(value + 1L)) + 1UL;
        if (pad_char != '0')
        {
            width--;
        }
        if (width < 0)
        {
            width = 0;
        }
        if (pad_char != '0')
        {
            *dst++ = '-';
        }
        return egui_api_sprintf_append_unsigned(dst, abs_value, width, pad_char);
    }

    return egui_api_sprintf_append_unsigned(dst, (unsigned long)value, width, pad_char);
}

#if EGUI_CONFIG_PLATFORM_CUSTOM_SPRINTF_FLOAT
static char *egui_api_sprintf_append_float(char *dst, double value, int width, char pad_char, int precision)
{
    char tmp[48];
    char digits[24];
    char *out = tmp;
    int len = 0;
    int digits_len = 0;
    int negative = 0;
    int pad_count;
    unsigned long scale = 1UL;
    unsigned long whole;
    unsigned long frac;

    if (precision < 0)
    {
        precision = 6;
    }
    if (precision > 9)
    {
        precision = 9;
    }

    if (value < 0.0)
    {
        negative = 1;
        value = -value;
    }

    for (int i = 0; i < precision; ++i)
    {
        scale *= 10UL;
    }

    whole = (unsigned long)value;
    frac = (unsigned long)(((value - (double)whole) * (double)scale) + 0.5);
    if (frac >= scale)
    {
        whole++;
        frac -= scale;
    }

    if (negative)
    {
        *out++ = '-';
    }

    if (whole == 0UL)
    {
        digits[digits_len++] = '0';
    }
    else
    {
        while (whole != 0UL)
        {
            digits[digits_len++] = (char)('0' + (whole % 10UL));
            whole /= 10UL;
        }
    }
    while (digits_len-- > 0)
    {
        *out++ = digits[digits_len];
    }

    if (precision > 0)
    {
        *out++ = '.';
        for (int divisor_index = precision - 1; divisor_index > 0; --divisor_index)
        {
            unsigned long divisor = 1UL;
            for (int i = 0; i < divisor_index; ++i)
            {
                divisor *= 10UL;
            }
            *out++ = (char)('0' + (frac / divisor));
            frac %= divisor;
        }
        *out++ = (char)('0' + frac);
    }

    len = (int)(out - tmp);
    pad_count = width > len ? (width - len) : 0;
    if (pad_char == '0' && negative)
    {
        *dst++ = *tmp;
        for (int i = 0; i < pad_count; ++i)
        {
            *dst++ = pad_char;
        }
        for (int i = 1; i < len; ++i)
        {
            *dst++ = tmp[i];
        }
        return dst;
    }

    for (int i = 0; i < pad_count; ++i)
    {
        *dst++ = pad_char;
    }
    for (int i = 0; i < len; ++i)
    {
        *dst++ = tmp[i];
    }
    return dst;
}
#endif

void egui_api_sprintf(char *str, const char *format, ...)
{
    va_list args;
    char *dst = str;

    if (str == NULL || format == NULL)
    {
        return;
    }

    va_start(args, format);
    while (*format != '\0')
    {
        int width = 0;
        int precision = -1;
        char pad_char = ' ';
        int long_flag = 0;

        if (*format != '%')
        {
            *dst++ = *format++;
            continue;
        }

        format++;
        if (*format == '%')
        {
            *dst++ = *format++;
            continue;
        }

        if (*format == '0')
        {
            pad_char = '0';
            format++;
        }
        while (*format >= '0' && *format <= '9')
        {
            width = width * 10 + (*format - '0');
            format++;
        }
        if (*format == '.')
        {
            precision = 0;
            format++;
            while (*format >= '0' && *format <= '9')
            {
                precision = precision * 10 + (*format - '0');
                format++;
            }
        }
        while (*format == 'l')
        {
            long_flag = 1;
            format++;
        }

        switch (*format)
        {
        case 'd':
        case 'i':
            if (long_flag)
            {
                dst = egui_api_sprintf_append_signed(dst, va_arg(args, long), width, pad_char);
            }
            else
            {
                dst = egui_api_sprintf_append_signed(dst, va_arg(args, int), width, pad_char);
            }
            break;
        case 'u':
            if (long_flag)
            {
                dst = egui_api_sprintf_append_unsigned(dst, va_arg(args, unsigned long), width, pad_char);
            }
            else
            {
                dst = egui_api_sprintf_append_unsigned(dst, va_arg(args, unsigned int), width, pad_char);
            }
            break;
        case 'c':
            *dst++ = (char)va_arg(args, int);
            break;
        case 's':
            dst = egui_api_sprintf_append_cstr(dst, va_arg(args, const char *));
            break;
        case 'f':
#if EGUI_CONFIG_PLATFORM_CUSTOM_SPRINTF_FLOAT
            dst = egui_api_sprintf_append_float(dst, va_arg(args, double), width, pad_char, precision);
            break;
#endif
            /* Fall through to keep the unsupported format text when float support is disabled. */
        case '\0':
            if (*format == '\0')
            {
                *dst++ = '%';
                format--;
                break;
            }
            /* Fall through. */
        default:
            *dst++ = '%';
            if (pad_char == '0')
            {
                *dst++ = '0';
            }
            while (width >= 10)
            {
                int divisor = 1;
                while ((divisor * 10) <= width)
                {
                    divisor *= 10;
                }
                *dst++ = (char)('0' + (width / divisor));
                width %= divisor;
            }
            if (width > 0)
            {
                *dst++ = (char)('0' + width);
            }
            if (precision >= 0)
            {
                *dst++ = '.';
                if (precision >= 10)
                {
                    int divisor = 1;
                    while ((divisor * 10) <= precision)
                    {
                        divisor *= 10;
                    }
                    while (divisor > 0)
                    {
                        *dst++ = (char)('0' + (precision / divisor));
                        precision %= divisor;
                        divisor /= 10;
                    }
                }
                else
                {
                    *dst++ = (char)('0' + precision);
                }
            }
            if (long_flag)
            {
                *dst++ = 'l';
            }
            *dst++ = *format;
            break;
        }

        if (*format != '\0')
        {
            format++;
        }
    }
    va_end(args);

    *dst = '\0';
}

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
