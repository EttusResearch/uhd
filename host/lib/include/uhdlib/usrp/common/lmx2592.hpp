//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "lmx2592_regs.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/safe_call.hpp>
#include <boost/format.hpp>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

class lmx2592_iface
{
public:
    typedef std::shared_ptr<lmx2592_iface> sptr;

    //! SPI write functor: Can take a SPI transaction and clock it out
    using write_spi_t = std::function<void(uint32_t)>;

    //! SPI read functor: Return SPI
    using read_spi_t = std::function<uint32_t(uint32_t)>;

    static sptr make(write_spi_t write, read_spi_t read);

    virtual ~lmx2592_iface() = default;

    enum output_t { RF_OUTPUT_A, RF_OUTPUT_B };

    enum mash_order_t { INT_N, FIRST, SECOND, THIRD, FOURTH };

    virtual double set_frequency(double target_freq,
        const bool spur_dodging,
        const double spur_dodging_threshold) = 0;

    virtual void set_mash_order(mash_order_t mash_order) = 0;

    virtual void set_reference_frequency(double ref_freq) = 0;

    virtual void set_output_power(output_t output, unsigned int power) = 0;

    virtual void set_output_enable(output_t output, bool enable) = 0;

    virtual bool get_lock_status() = 0;

    virtual void commit() = 0;
};
