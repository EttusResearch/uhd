//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/noc_block_base.hpp>

using namespace uhd::rfnoc;

namespace {

// Static factory for constructing a block controller T given an instance of
// the superclass noc_block_base for the block controller, as might be
// returned from uhd::rfnoc::graph::get_block(). The instance is downcast to
// the derived class and returned to the client as a T::sptr. If block_base
// does not represent an instance of T, nullptr is returned.
template <typename T>
class block_controller_factory
{
public:
    static typename T::sptr make_from(noc_block_base::sptr block_base)
    {
        return std::dynamic_pointer_cast<T>(block_base);
    }
};

} // namespace
