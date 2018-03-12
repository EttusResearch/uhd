//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/noncopyable.hpp>
#include <stdint.h>

#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include "e300_global_regs.hpp"

#ifndef INCLUDED_E300_SENSOR_MANAGER_HPP
#define INCLUDED_E300_SENSOR_MANAGER_HPP

namespace uhd { namespace usrp { namespace e300 {

struct sensor_transaction_t {
    uint32_t which;
    union {
        uint32_t value;
        uint32_t value64;
    };
};



enum sensor {ZYNQ_TEMP=0, REF_LOCK=4};

class e300_sensor_manager : boost::noncopyable
{
public:
    typedef boost::shared_ptr<e300_sensor_manager> sptr;

    virtual ~e300_sensor_manager() {};

    virtual uhd::sensor_value_t get_sensor(const std::string &key) = 0;
    virtual std::vector<std::string> get_sensors(void) = 0;

    virtual uhd::sensor_value_t get_mb_temp(void) = 0;
    virtual uhd::sensor_value_t get_ref_lock(void) = 0;


    static sptr make_proxy(uhd::transport::zero_copy_if::sptr xport);
    static sptr make_local(global_regs::sptr global_regs);

    // Note: This is a hack
    static uint32_t pack_float_in_uint32_t(const float &v)
    {
        const uint32_t *cast = reinterpret_cast<const uint32_t*>(&v);
        return *cast;
    }

    static float unpack_float_from_uint32_t(const uint32_t &v)
    {
        const float *cast = reinterpret_cast<const float*>(&v);
        return *cast;
    }
};


}}} // namespace

#endif // INCLUDED_E300_SENSOR_MANAGER_HPP
