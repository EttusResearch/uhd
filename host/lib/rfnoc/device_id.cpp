//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/rfnoc/device_id.hpp>
#include <atomic>

using namespace uhd::rfnoc;

device_id_t uhd::rfnoc::allocate_device_id()
{
    static std::atomic<device_id_t> counter{1};
    return counter++;
}
