
// Copyright 2012 Ettus Research LLC

#ifndef INCLUDED_WB_UTILS_H
#define INCLUDED_WB_UTILS_H

#include <stdint.h>

#define localparam static const int

static inline void wb_poke32(const uint32_t addr, const uint32_t data)
{
    *((volatile uint32_t *)addr) = data;
}

static inline uint32_t wb_peek32(const uint32_t addr)
{
    const uint32_t data = *((volatile uint32_t *)addr);
    return data;
}

#define SR_ADDR(base, offset) ((base) + (offset)*4)

#endif /* INCLUDED_WB_UTILS_H */
