
// Copyright 2012 Ettus Research LLC

#ifndef INCLUDED_WB_UART_H
#define INCLUDED_WB_UART_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

//! Init the uart in question
void wb_uart_init(const uint32_t base, const size_t div);

//! Put character blocking
void wb_uart_putc(const uint32_t base, const int ch);

//! Put character not blocking - false for fail
bool wb_uart_try_putc(const uint32_t base, const int ch);

//! Get character non blocking, -1 for none
int wb_uart_getc(const uint32_t base);

#endif /* INCLUDED_WB_UART_H */
