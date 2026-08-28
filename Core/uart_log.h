#ifndef UART_LOG_H
#define UART_LOG_H

#include <stddef.h>
#include <stdint.h>

void uart_log_write(const char *text);
void uart_log_printf(const char *format, ...);
void uart_log_hex(const char *name, const uint8_t *data,
                  size_t length, size_t max_print);

#endif
