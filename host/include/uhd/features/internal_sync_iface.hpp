//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/features/discoverable_feature.hpp>

namespace uhd { namespace features {

/*!
 * This feature provides access to the internal sync feature of the X440.
 */
class UHD_API internal_sync_iface : public discoverable_feature
{
public:
    using sptr = std::shared_ptr<internal_sync_iface>;

    static discoverable_feature::feature_id_t get_feature_id()
    {
        return discoverable_feature::INTERNAL_SYNC;
    }

    std::string get_feature_name() const
    {
        return "Internal Sync";
    }

    virtual ~internal_sync_iface() = default;

    /*!
     * Enables the internal 5 MHz sync clock which can be accessed on antenna name
     * SYNC_INT.
     */
    virtual void enable_sync_clk() = 0;

    /*!
     * Disables the internal 5 MHz sync clock
     */
    virtual void disable_sync_clk() = 0;
};
}} // namespace uhd::features