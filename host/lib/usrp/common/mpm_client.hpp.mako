//
// Auto-generated C++ client interface header
// Generated from protobuf definition
//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// clang-format off

#pragma once

#include <uhd/config.hpp>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <cstdint>

namespace uhd {

using rpc_exception = std::runtime_error;

class UHD_API rpc_client
{
public:
    using sptr = std::shared_ptr<rpc_client>;

    class timeout_scope
    {
    public:
        timeout_scope() = default;
        timeout_scope(const timeout_scope&) = delete;
        timeout_scope& operator=(const timeout_scope&) = delete;
        timeout_scope(timeout_scope&&) = default;
        timeout_scope& operator=(timeout_scope&&) = default;
        virtual ~timeout_scope() = default;
    };

    using timeout_scope_uptr = std::unique_ptr<timeout_scope>;

    // Non-RPC calls
    virtual void set_token(const std::string &token) = 0;
    virtual void set_timeout(uint64_t timeout_ms) = 0;
    virtual timeout_scope_uptr set_scope_timeout(uint64_t timeout_ms) = 0;

    // Unique per-client id, used to correlate this client with its gRPC
    // channel lifecycle logs (see "MPM_CLIENT rpc_client #<id> ..." messages).
    virtual uint64_t get_client_id() const = 0;

    // Return this RPC client (for compatibility)
    virtual sptr get_raw_rpc_client() = 0;

    // Main domain methods
% for method in main_methods:
    virtual ${method['return_type']} ${method['name']}(${', '.join(method['parameters'])}) = 0;
% endfor

    virtual ~rpc_client() = default;

    // Daughterboard interface
    class dboard_iface
    {
    public:
        using sptr = std::shared_ptr<dboard_iface>;
% for method in dboard_methods:
        virtual ${method['return_type']} ${method['name']}(${', '.join(method['parameters'])}) = 0;
% endfor
        virtual ~dboard_iface() = default;
    };

    virtual dboard_iface& get_dboard(size_t db_idx) = 0;

    // Factory method
    static sptr make(const std::string &server_name, uint16_t port, uint64_t timeout_ms);
};

}

// clang-format on