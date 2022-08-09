//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/features/discoverable_feature.hpp>
#include <uhd/types/trig_io_mode.hpp>
#include <memory>
#include <string>

namespace uhd { namespace features {

/*! Interface to provide access to set the mode of the Trig In/Out port.
 *  Currently, only the X4xx series of devices supports this.
 */
class UHD_API trig_io_mode_iface : public discoverable_feature
{
public:
    using sptr = std::shared_ptr<trig_io_mode_iface>;

    static discoverable_feature::feature_id_t get_feature_id()
    {
        return discoverable_feature::TRIG_IO_MODE;
    }

    std::string get_feature_name() const
    {
        return "Trig IO Mode";
    }

    virtual ~trig_io_mode_iface() = default;

    //! Set the mode for the trig i/o port
    virtual void set_trig_io_mode(const uhd::trig_io_mode_t mode) = 0;
};

}} // namespace uhd::features
