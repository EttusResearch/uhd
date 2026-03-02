//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <uhdlib/usrp/common/ltc5594.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_cpld_ctrl.hpp>

namespace uhd { namespace usrp { namespace hbx {

class hbx_demod_ctrl final : public hbx_cpld_ctrl::spi_transactor
{
public:
    // Pass in our demod selection and poke/peek functions
    hbx_demod_ctrl(size_t start_address,
        hbx_cpld_ctrl::poke_fn_type&& poke_fn,
        hbx_cpld_ctrl::peek_fn_type&& peek_fn);

    /*! Sets the LO frequency
     *
     * The demodulator needs that to configure the matching accordingly
     * \param lo_freq The LO frequency in Hz (in HBX this matches the tune frequency)
     */
    void set_lo_matching(const double lo_freq);

private:
    const std::string _log_id = "HBX_DEMOD";
    ltc5594_iface::sptr _ltc;
};
}}} // namespace uhd::usrp::hbx
