//
// Copyright 2014 Ettus Research LLC
//

#ifndef INCLUDED_AD9361_PLATFORM_H
#define INCLUDED_AD9361_PLATFORM_H

#include <stdint.h>
#include "ad9361_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Get chip class from handle
 */
ad9361_device_t* get_ad9361_device(uint64_t handle);

/*!
 * Write a register in the AD9361 space
 */
void write_ad9361_reg(ad9361_device_t* device, uint32_t reg, uint8_t val);

/*!
 * Read a register from the AD9361 space
 */
uint8_t read_ad9361_reg(ad9361_device_t* device, uint32_t reg);

/*!
 * Millisecond sleep
 */
void ad9361_msleep(const uint32_t millis);

/*!
 * Pack a double into 2 uint32s
 */
void ad9361_double_pack(const double input, uint32_t output[2]);

/*!
 * Unpack 2 uint32s into a double
 */
double ad9361_double_unpack(const uint32_t input[2]);

/*!
 * Compute the square root of val
 */
double ad9361_sqrt(double val);

/*!
 * Compute the floor of val
 */
int ad9361_floor_to_int(double val);

/*!
 * Compute the ceil of val
 */
int ad9361_ceil_to_int(double val);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_AD9361_PLATFORM_H */
