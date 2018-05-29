//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "magnesium_ad9371_iface.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/utils/log.hpp>

using namespace uhd;

namespace {
    /*! Return a valid 'which' string for use with AD9371 API calls
     *
     * These strings take the form of "RX1", "TX2", ...
     */
    std::string _get_which(
        const direction_t dir,
        const size_t chan
    ) {
        UHD_ASSERT_THROW(dir == RX_DIRECTION or dir == TX_DIRECTION);
        UHD_ASSERT_THROW(chan == 0 or chan == 1);
        return str(boost::format("%s%d")
                   % (dir == RX_DIRECTION ? "RX" : "TX")
                   % (chan+1)
        );
    }
}

/******************************************************************************
 * Structors
 *****************************************************************************/
magnesium_ad9371_iface::magnesium_ad9371_iface(
    uhd::rpc_client::sptr rpcc,
    const size_t slot_idx
) : _rpcc(rpcc)
  , _slot_idx(slot_idx)
  , _rpc_prefix((slot_idx == 0) ? "db_0_" : "db_1_")
  , _log_prefix((slot_idx == 0) ? "AD9371-0" : "AD9371-1")
{
    UHD_LOG_TRACE(_log_prefix,
        "Initialized controls with RPC prefix " << _rpc_prefix <<
        " for slot " << _slot_idx);
}

double magnesium_ad9371_iface::set_frequency(
        const double freq,
        const size_t chan,
        const direction_t dir
) {
    // Note: This sets the frequency for both channels (1 and 2).
    auto which = _get_which(dir, chan);
    auto actual_freq = request<double>("set_freq", which, freq, false);
    UHD_LOG_TRACE(_log_prefix,
            _rpc_prefix << "set_freq returned " << actual_freq);
    return actual_freq;
}

double magnesium_ad9371_iface::set_gain(
        const double gain,
        const size_t chan,
        const direction_t dir
) {
    auto which = _get_which(dir, chan);
    auto retval = request<double>("set_gain", which, gain);
    UHD_LOG_TRACE(_log_prefix, _rpc_prefix << "set_gain returned " << retval);

    return retval;
    //return 0.0;
}


double magnesium_ad9371_iface::set_bandwidth(const double bandwidth, const size_t chan, const direction_t dir)
{
    auto const which = _get_which(dir, chan);
    auto retval = request<double>("set_bw_filter", which, bandwidth);
    UHD_LOG_TRACE(_log_prefix, _rpc_prefix << "set_bw_filter returned " << retval);
    return retval;
}

double magnesium_ad9371_iface::get_frequency(
    const size_t chan,
    const direction_t dir
) {
    auto which = _get_which(dir, chan);
    auto retval = request<double>("get_freq", which);
    UHD_LOG_TRACE(_log_prefix, _rpc_prefix << "get_freq returned " << retval);
    return retval;
}

double magnesium_ad9371_iface::get_gain(const size_t chan, const direction_t dir)
{
    auto which = _get_which(dir, chan);
    auto retval = request<double>("get_gain", which);
    UHD_LOG_TRACE(_log_prefix, _rpc_prefix << "get_gain returned " << retval);
    return retval;
}

double magnesium_ad9371_iface::get_bandwidth(const size_t /*chan*/, const direction_t /*dir*/)
{
    // TODO: implement
    UHD_LOG_WARNING(_log_prefix, "Ignoring attempt to get bandwidth");
    return 0.0;
}

std::string magnesium_ad9371_iface::set_lo_source(
    const std::string &source,
    const uhd::direction_t dir
) {
    // There is only one LO for 2 channels. Using channel 0 for 'which'
    auto which = _get_which(dir, 0);
    auto retval = request<std::string>("set_lo_source", which, source);
    UHD_LOG_TRACE(_log_prefix, _rpc_prefix << "set_lo_source returned " << retval);
    return retval;
}

std::string magnesium_ad9371_iface::get_lo_source(
    const uhd::direction_t dir
) {
    // There is only one LO for 2 channels. Using channel 0 for 'which'
    auto which = _get_which(dir, 0);
    auto retval = request<std::string>("get_lo_source", which);
    UHD_LOG_TRACE(_log_prefix, _rpc_prefix << "get_lo_source returned " << retval);
    return retval;
}

