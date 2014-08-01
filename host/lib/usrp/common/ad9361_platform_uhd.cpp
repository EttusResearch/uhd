//
// Copyright 2014 Ettus Research LLC
//

#include <uhd/utils/msg.hpp>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <ad9361_platform.h>
#include "ad9361_ctrl.hpp"
#include <stdio.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

//If the platform for the AD9361 driver is UHD (host) then the handle is simply
//a pointer to a device class instance
ad9361_device_t* get_ad9361_device(uint64_t handle)
{
    return reinterpret_cast<ad9361_device_t*>(reinterpret_cast<void*>(handle));
}

uint8_t read_ad9361_reg(ad9361_device_t* device, uint32_t reg)
{
    if (device && device->io_iface) {
        //If the platform for the AD9361 driver is UHD (host) then the io_iface is
        //a pointer to a ad9361_io implementation
        ad9361_io* io_iface = reinterpret_cast<ad9361_io*>(device->io_iface);
        return io_iface->peek8(reg);
    } else {
        return 0;
    }
}

void write_ad9361_reg(ad9361_device_t* device, uint32_t reg, uint8_t val)
{
    if (device && device->io_iface) {
        //If the platform for the AD9361 driver is UHD (host) then the io_iface is
        //a pointer to a ad9361_io implementation
        ad9361_io* io_iface = reinterpret_cast<ad9361_io*>(device->io_iface);
        io_iface->poke8(reg, val);
    }
}

typedef union
{
    double d;
    uint32_t x[2];
} ad9361_double_union_t;

void ad9361_double_pack(const double input, uint32_t output[2])
{
    ad9361_double_union_t p = {};
    p.d = input;
    output[0] = p.x[0];
    output[1] = p.x[1];
}

double ad9361_double_unpack(const uint32_t input[2])
{
    ad9361_double_union_t p = {};
    p.x[0] = input[0];
    p.x[1] = input[1];
    return p.d;
}

double ad9361_sqrt(double val)
{
    return std::sqrt(val);
}

void ad9361_msleep(const uint32_t millis)
{
    boost::this_thread::sleep(boost::posix_time::milliseconds(millis));
}

int ad9361_floor_to_int(double val)
{
    return static_cast<int>(std::floor(val));
}

int ad9361_ceil_to_int(double val)
{
    return static_cast<int>(std::ceil(val));
}

