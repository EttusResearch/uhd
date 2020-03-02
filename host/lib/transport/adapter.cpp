//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhdlib/transport/adapter.hpp>

using namespace uhd::transport;

adapter_id_t adapter_ctx::register_adapter(adapter_info& info)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto key = info.to_string();
    if (_id_map.count(key) > 0) {
        return _id_map.at(key);
    } else {
        adapter_id_t id = _id_map.size() + 1;
        _id_map.emplace(std::make_pair(key, id));
        return id;
    }
}
