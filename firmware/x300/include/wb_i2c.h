
// Copyright 2012 Ettus Research LLC

#ifndef INCLUDED_WB_I2C_H
#define INCLUDED_WB_I2C_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void wb_i2c_init(const uint32_t base, const size_t clk_rate);

bool wb_i2c_read(const uint32_t base, const uint8_t i2c_addr, uint8_t *buf, size_t len);

bool wb_i2c_write(const uint32_t base, const uint8_t i2c_addr, const uint8_t *buf, size_t len);


#endif /* INCLUDED_WB_I2C_H */
