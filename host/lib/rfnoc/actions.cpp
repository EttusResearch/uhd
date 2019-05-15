//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/actions.hpp>
#include <atomic>

using namespace uhd::rfnoc;

namespace {
    // A static counter, which we use to uniquely label actions
    std::atomic<size_t> action_counter{0};
}

action_info::action_info(const std::string& key_) : id(action_counter++), key(key_)
{
    // nop
}

