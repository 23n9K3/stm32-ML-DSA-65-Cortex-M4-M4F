#include "uart_log.h"
#include "main.h"
#include <stdarg.h>
#include <string.h>

extern UART_HandleTypeDef hlpuart1;

void uart_log_write(const char *text)
{
    size_t remaining = strlen(text);
    const uint8_t *cursor = (const uint8_t *)text;
    while (remaining != 0U) {
        uint16_t chunk = remaining > 0xFFFFU ? 0xFFFFU : (uint16_t)remaining;
        (void)HAL_UART_Transmit(&hlpuart1, (uint8_t *)cursor, chunk, HAL_MAX_DELAY);
        cursor += chunk;
        remaining -= chunk;
    }
}

static void log_putc(char *buffer, size_t capacity, size_t *length, char value)
{
    if (*length + 1U < capacity) {
        buffer[*length] = value;
        ++(*length);
    }
}

static void log_puts(char *buffer, size_t capacity, size_t *length,
                     const char *text)
{
    while (*text != '\0') {
        log_putc(buffer, capacity, length, *text++);
    }
}

static void log_put_unsigned(char *buffer, size_t capacity, size_t *length,
                             unsigned long value, unsigned int base,
                             unsigned int width, char pad)
{
    static const char digits[] = "0123456789ABCDEF";
    char temp[16];
    unsigned int count = 0U;

    do {
        temp[count++] = digits[value % base];
        value /= base;
    } while (value != 0U && count < sizeof(temp));
    while (count < width) {
        log_putc(buffer, capacity, length, pad);
        --width;
    }
    while (count != 0U) {
        log_putc(buffer, capacity, length, temp[--count]);
    }
}

void uart_log_printf(const char *format, ...)
{
    char buffer[256];
    size_t length = 0U;
    va_list args;

    va_start(args, format);
    while (*format != '\0') {
        unsigned int width = 0U;
        char pad = ' ';
        int is_long = 0;
        char spec;

        if (*format != '%') {
            log_putc(buffer, sizeof(buffer), &length, *format++);
            continue;
        }
        ++format;
        if (*format == '0') {
            pad = '0';
            ++format;
        }
        while (*format >= '0' && *format <= '9') {
            width = width * 10U + (unsigned int)(*format++ - '0');
        }
        if (*format == 'l') {
            is_long = 1;
            ++format;
        }
        spec = *format == '\0' ? '\0' : *format++;
        if (spec == 's') {
            log_puts(buffer, sizeof(buffer), &length, va_arg(args, const char *));
        } else if (spec == 'd') {
            long value = is_long ? va_arg(args, long) : va_arg(args, int);
            if (value < 0) {
                log_putc(buffer, sizeof(buffer), &length, '-');
                value = -value;
            }
            log_put_unsigned(buffer, sizeof(buffer), &length,
                             (unsigned long)value, 10U, width, pad);
        } else if (spec == 'u') {
            unsigned long value = is_long ? va_arg(args, unsigned long)
                                          : va_arg(args, unsigned int);
            log_put_unsigned(buffer, sizeof(buffer), &length, value, 10U,
                             width, pad);
        } else if (spec == 'X') {
            unsigned long value = is_long ? va_arg(args, unsigned long)
                                          : va_arg(args, unsigned int);
            log_put_unsigned(buffer, sizeof(buffer), &length, value, 16U,
                             width, pad);
        } else if (spec == '%') {
            log_putc(buffer, sizeof(buffer), &length, '%');
        } else {
            log_putc(buffer, sizeof(buffer), &length, '?');
        }
    }
    va_end(args);
    buffer[length] = '\0';
    uart_log_write(buffer);
}

void uart_log_hex(const char *name, const uint8_t *data,
                  size_t length, size_t max_print)
{
    size_t count = length < max_print ? length : max_print;
    size_t i;
    uart_log_printf("[HEX] %s first %lu bytes: ", name, (unsigned long)count);
    for (i = 0; i < count; ++i) {
        uart_log_printf("%02X", data[i]);
    }
    uart_log_write("\r\n");
}
