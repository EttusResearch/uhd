//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_MAGNESIUM_AD9371_IFACE_HPP
#define INCLUDED_LIBUHD_RFNOC_MAGNESIUM_AD9371_IFACE_HPP

#include <uhd/types/direction.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <iostream>
#include <string>

class magnesium_ad9371_iface
{
public:
    using uptr = std::unique_ptr<magnesium_ad9371_iface>;

    magnesium_ad9371_iface(uhd::rpc_client::sptr rpcc, const size_t slot_idx);

    double set_frequency(
        const double freq, const size_t chan, const uhd::direction_t dir);

    double get_frequency(const size_t chan, const uhd::direction_t dir);

    double set_gain(const double gain, const size_t chan, const uhd::direction_t dir);

    double get_gain(const size_t chan, const uhd::direction_t dir);

    double set_bandwidth(
        const double bandwidth, const size_t chan, const uhd::direction_t dir);

    double get_bandwidth(const size_t chan, const uhd::direction_t dir);

    std::string set_lo_source(const std::string& source, const uhd::direction_t dir);

    std::string get_lo_source(const uhd::direction_t dir);

    void set_fir(
        const std::string& name, const int8_t gain, const std::vector<int16_t>& coeffs);

    std::pair<int8_t, std::vector<int16_t>> get_fir(const std::string& name);

private:
    /*! Shorthand to perform an RPC request. Saves some typing.
     */
    template <typename return_type, typename... Args>
    return_type request(std::string const& func_name, Args&&... args)
    {
        UHD_LOG_TRACE(_log_prefix, "[RPC] Calling " << func_name);
        return _rpcc->request_with_token<return_type>(
            _rpc_prefix + func_name, std::forward<Args>(args)...);
    };

    /*! Shorthand to perform an RPC request with timeout.
     */
    template <typename return_type, typename... Args>
    return_type request(uint64_t timeout_ms, std::string const& func_name, Args&&... args)
    {
        UHD_LOG_TRACE(_log_prefix, "[RPC] Calling " << func_name);
        return _rpcc->request_with_token<return_type>(
            timeout_ms, _rpc_prefix + func_name, std::forward<Args>(args)...);
    };

    //! Reference to the RPC client
    uhd::rpc_client::sptr _rpcc;

    //! Stores the prefix to RPC calls
    const std::string _rpc_prefix;

    //! Logger prefix
    const std::string _log_prefix;
};

#endif /* INCLUDED_LIBUHD_RFNOC_MAGNESIUM_AD9371_IFACE_HPP */
