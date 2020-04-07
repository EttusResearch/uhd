//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/transport/adapter_id.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <uhd/utils/static.hpp>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace uhd { namespace transport {

class adapter_info
{
public:
    /*! Returns a unique string identifying the adapter
     *  String contents are not API. Only uniqueness is guaranteed.
     */
    virtual std::string to_string() = 0;
};

}} // namespace uhd::transport
