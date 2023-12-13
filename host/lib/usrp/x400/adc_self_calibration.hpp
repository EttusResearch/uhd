//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/features/adc_self_calibration_iface.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <uhdlib/usrp/dboard/x400_dboard_iface.hpp>
#include <string>

namespace uhd { namespace features {

using namespace uhd::rfnoc;

class adc_self_calibration : public adc_self_calibration_iface
{
public:
    adc_self_calibration(uhd::usrp::x400_rpc_iface::sptr rpcc,
        const std::string rpc_prefix,
        const std::string unique_id,
        size_t db_number,
        uhd::usrp::x400::x400_dboard_iface::sptr daughterboard);

    void run(const size_t channel) override;

    void run(const size_t chan, const uhd::device_addr_t params) override;

    // Maximum duration for self-cal in ms. From tests we found that 2 seconds are good,
    // so we limit the maximum value to twice 2 seconds to keep the overall calibration
    // duration at a convenient level.
    const uint32_t MAX_CAL_DURATION = 4000;

private:
    //! Reference to the RPC client
    uhd::usrp::x400_rpc_iface::sptr _rpcc;

    const std::string _rpc_prefix;

    const size_t _db_number;

    uhd::usrp::x400::x400_dboard_iface::sptr _daughterboard;

    const std::string _unique_id;
    std::string get_unique_id() const
    {
        return _unique_id;
    }
};

}} // namespace uhd::features
