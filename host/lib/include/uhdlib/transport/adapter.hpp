//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/transport/adapter_info.hpp>
#include <uhdlib/transport/udp_boost_asio_link.hpp>

namespace uhd { namespace transport {

class adapter_ctx : uhd::noncopyable
{
public:
    UHD_SINGLETON_FCN(adapter_ctx, get);

    ~adapter_ctx() = default;

    adapter_id_t register_adapter(adapter_info& info);

private:
    adapter_ctx() = default;

    std::mutex _mutex;
    std::unordered_map<std::string, adapter_id_t> _id_map;
};

}} // namespace uhd::transport
