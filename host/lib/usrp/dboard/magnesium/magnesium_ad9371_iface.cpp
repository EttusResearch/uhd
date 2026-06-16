//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "magnesium_ad9371_iface.hpp"
#include "magnesium_constants.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/utils/narrow.hpp>

using namespace uhd;

namespace {
/*! Return a valid 'which' string for use with AD9371 API calls
 *
 * These strings take the form of "RX1", "TX2", ...
 */
std::string _get_which(const direction_t dir, const size_t chan)
{
    UHD_ASSERT_THROW(dir == RX_DIRECTION || dir == TX_DIRECTION);
    UHD_ASSERT_THROW(chan == 0 || chan == 1);
    return (dir == RX_DIRECTION ? "RX" : "TX") + std::to_string(chan + 1);
}

/*! Return a valid 'which' string for use with AD9371 API duplex calls
 *
 * Extends _get_which by additionally allowing duplex values
 */
std::string _get_which_duplex(const direction_t dir, const size_t chan)
{
    if (dir == RX_DIRECTION || dir == TX_DIRECTION) {
        return _get_which(dir, chan);
    }
    UHD_ASSERT_THROW(dir == DX_DIRECTION);
    UHD_ASSERT_THROW(chan == 0 || chan == 1);
    return "DX" + std::to_string(chan + 1);
}
} // namespace

/******************************************************************************
 * Structors
 *****************************************************************************/
magnesium_ad9371_iface::magnesium_ad9371_iface(
    uhd::rpc_client::sptr rpcc, const size_t slot_idx)
    : _rpcc(rpcc)
    , _db_idx(slot_idx)
    , _log_prefix((slot_idx == 0) ? "AD9371-0" : "AD9371-1")
{
    UHD_LOG_TRACE(_log_prefix, "Initialized controls for slot " << slot_idx);
}

double magnesium_ad9371_iface::set_frequency(
    const double freq, const size_t chan, const direction_t dir)
{
    // Note: This sets the frequency for both channels (1 and 2).
    auto which         = _get_which(dir, chan);
    double actual_freq = 0.0;
    {
        [[maybe_unused]] auto timeout_scope =
            _rpcc->set_scope_timeout(MAGNESIUM_TUNE_TIMEOUT);
        actual_freq = rpc().set_freq(which, freq, false);
    }
    UHD_LOG_TRACE(_log_prefix, "set_freq returned " << actual_freq);
    return actual_freq;
}

double magnesium_ad9371_iface::set_gain(
    const double gain, const size_t chan, const direction_t dir)
{
    auto which    = _get_which(dir, chan);
    double retval = rpc().set_gain(which, gain);
    UHD_LOG_TRACE(_log_prefix, "set_gain returned " << retval);

    return retval;
    // return 0.0;
}

/*! brief Sets the frontend bandwidth settings for the dboard. Requires
 * re-initializing the dboard, so it may take a significant amount of time.
 *
 * \param bandwidth target rf bandwidth value
 * \param chan -not important- the bandwidth settings affect both channels on
 *        the dboard
 * \param dir specifies which path to set the bandwidth filters for. Supports
 *        rx, tx, or dx for both
 * \return actual rf bandwidth value
 */
double magnesium_ad9371_iface::set_bandwidth(
    const double bandwidth, const size_t chan, const direction_t dir)
{
    const auto which = _get_which_duplex(dir, chan);
    double retval    = 0.0;
    {
        [[maybe_unused]] auto timeout_scope =
            _rpcc->set_scope_timeout(MAGNESIUM_TUNE_TIMEOUT);
        retval = rpc().set_bandwidth(which, bandwidth);
    }
    UHD_LOG_TRACE(_log_prefix, "set_bandwidth returned " << retval);
    return retval;
}

double magnesium_ad9371_iface::get_frequency(const size_t chan, const direction_t dir)
{
    auto which    = _get_which(dir, chan);
    double retval = rpc().get_freq(which);
    UHD_LOG_TRACE(_log_prefix, "get_freq returned " << retval);
    return retval;
}

double magnesium_ad9371_iface::get_gain(const size_t chan, const direction_t dir)
{
    auto which          = _get_which(dir, chan);
    const double retval = rpc().get_gain(which);
    UHD_LOG_TRACE(_log_prefix, "get_gain returned " << retval);
    return retval;
}

double magnesium_ad9371_iface::get_bandwidth(const size_t chan, const direction_t dir)
{
    const auto which  = _get_which(dir, chan);
    const auto retval = rpc().get_bandwidth(which);
    UHD_LOG_TRACE(_log_prefix, "get_bandwidth returned " << retval);
    return retval;
}

std::string magnesium_ad9371_iface::set_lo_source(
    const std::string& source, const uhd::direction_t dir)
{
    // There is only one LO for 2 channels. Using channel 0 for 'which'
    auto which  = _get_which(dir, 0);
    auto retval = rpc().set_lo_source(which, source);
    UHD_LOG_TRACE(_log_prefix, "set_lo_source returned " << retval);
    return retval;
}

std::string magnesium_ad9371_iface::get_lo_source(const uhd::direction_t dir)
{
    // There is only one LO for 2 channels. Using channel 0 for 'which'
    auto which  = _get_which(dir, 0);
    auto retval = rpc().get_lo_source(which);
    UHD_LOG_TRACE(_log_prefix, "get_lo_source returned " << retval);
    return retval;
}

void magnesium_ad9371_iface::set_fir(
    const std::string& name, const int8_t gain, const std::vector<int16_t>& coeffs)
{
    // Convert int16_t to int32_t for gRPC, and widen gain from int8_t to int32_t
    std::vector<int32_t> coeffs_int32(coeffs.begin(), coeffs.end());
    rpc().set_fir(name, static_cast<int32_t>(gain), coeffs_int32);
}

std::pair<int8_t, std::vector<int16_t>> magnesium_ad9371_iface::get_fir(
    const std::string& name)
{
    // rpc().get_fir() returns int32 types (proto wire types); narrow to int8/int16
    // here, mirroring how set_fir widens int8/int16 to int32 before the RPC call.
    auto [gain_i32, coeffs_i32] = rpc().get_fir(name);
    const int8_t gain           = uhd::narrow<int8_t>(gain_i32);
    std::vector<int16_t> coeffs;
    coeffs.reserve(coeffs_i32.size());
    for (int32_t c : coeffs_i32) {
        coeffs.push_back(uhd::narrow<int16_t>(c));
    }
    return {gain, coeffs};
}
