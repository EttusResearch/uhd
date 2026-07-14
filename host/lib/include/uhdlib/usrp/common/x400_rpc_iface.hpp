//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/usrp/common/rpc.hpp>
#include <memory>

namespace uhd { namespace usrp {

// Type aliases for X400-specific RPC interfaces
// These map to the generic rpc_client interface for gRPC compatibility

//! X400 motherboard RPC interface
using x400_rpc_iface = uhd::rpc_client;

/*! \brief X400 daughterboard RPC helper class
 *
 * Provides convenient access to X400 dboard-specific RPC methods.
 * X400 accesses dboard methods through get_dboard(idx), similar to N3xx.
 * This helper simplifies the access pattern without inheriting from dboard_iface.
 */
class x400_dboard_rpc_iface
{
public:
    using sptr = std::shared_ptr<x400_dboard_rpc_iface>;

    x400_dboard_rpc_iface(uhd::rpc_client::sptr rpcc, size_t db_idx)
        : _rpcc(rpcc), _db_idx(db_idx)
    {
    }

    // Provide access to the underlying RPC client
    uhd::rpc_client::sptr get_rpc_client()
    {
        return _rpcc;
    }

    // Provide direct access to dboard RPC interface (like N3xx pattern)
    uhd::rpc_client::dboard_iface& get_dboard_iface()
    {
        return _rpcc->get_dboard(_db_idx);
    }

    // Forward main rpc_client methods that daughterboards need
    void enable_iq_swap(bool enable, std::string trx, uint32_t channel)
    {
        get_dboard_iface().enable_iq_swap(enable, trx, channel);
    }

    double get_dboard_sample_rate()
    {
        return get_dboard_iface().get_dboard_sample_rate();
    }

    double get_dboard_prc_rate()
    {
        return get_dboard_iface().get_dboard_prc_rate();
    }

    // Convenience methods for X400-specific dboard methods
    double rfdc_set_nco_freq(
        std::string trx, uint32_t channel, double freq, uint32_t mode)
    {
        return get_dboard_iface().rfdc_set_nco_freq(trx, channel, freq, mode);
    }

    double rfdc_get_nco_freq(std::string trx, uint32_t channel, uint32_t mode)
    {
        return get_dboard_iface().rfdc_get_nco_freq(trx, channel, mode);
    }

    bool get_threshold_status(uint32_t channel, uint32_t mode, uint32_t threshold_idx)
    {
        return get_dboard_iface().get_threshold_status(channel, mode, threshold_idx);
    }

private:
    uhd::rpc_client::sptr _rpcc;
    size_t _db_idx;
};

//! ZBX daughterboard RPC interface
using zbx_rpc_iface = x400_dboard_rpc_iface;

//! FBX daughterboard RPC interface
using fbx_rpc_iface = x400_dboard_rpc_iface;

/*! \brief HBX daughterboard RPC interface
 *
 * Extends x400_dboard_rpc_iface with set_data_path for RFDC mode switching.
 */
class hbx_rpc_iface : public x400_dboard_rpc_iface
{
public:
    using sptr = std::shared_ptr<hbx_rpc_iface>;

    hbx_rpc_iface(uhd::rpc_client::sptr rpcc, size_t db_idx)
        : x400_dboard_rpc_iface(rpcc, db_idx)
    {
    }

    void set_data_path(uint32_t mode, std::string direction)
    {
        get_dboard_iface().set_data_path(mode, direction);
    }
};

//! HBX RPC type alias (matches usage pattern of zbx_rpc / fbx_rpc)
using hbx_rpc = hbx_rpc_iface;

}} // namespace uhd::usrp
