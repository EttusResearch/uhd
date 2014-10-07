// Copyright 2012 Ettus Research LLC

#include <wb_uart.h>
#include <wb_utils.h>

localparam SUART_CLKDIV = 0;
localparam SUART_TXLEVEL = 1;
localparam SUART_RXLEVEL = 2;
localparam SUART_TXCHAR = 3;
localparam SUART_RXCHAR = 4;

void wb_uart_init(const uint32_t base, const size_t div)
{
    wb_poke32(base + SUART_CLKDIV*4, div);
}

void wb_uart_putc(const uint32_t base, const int ch)
{
    while (wb_peek32(base + SUART_TXLEVEL*4) == 0);
    wb_poke32(base + SUART_TXCHAR*4, ch);
}

bool wb_uart_try_putc(const uint32_t base, const int ch)
{
    if (wb_peek32(base + SUART_TXLEVEL*4) == 0) return false;
    wb_poke32(base + SUART_TXCHAR*4, ch);
    return true;
}

int wb_uart_getc(const uint32_t base)
{
    if (wb_peek32(base + SUART_RXLEVEL*4) == 0) return -1;
    return wb_peek32(base + SUART_RXCHAR*4);
}
