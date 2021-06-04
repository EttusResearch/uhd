//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/features/discoverable_feature.hpp>
#include <memory>
#include <string>

namespace uhd { namespace features {

/*! This is the mechanism by which USRPs can perform actions whenever the FPGA
 *  is loaded.
 */
class fpga_load_notification_iface : public discoverable_feature
{
public:
    using sptr = std::shared_ptr<fpga_load_notification_iface>;

    static discoverable_feature::feature_id_t get_feature_id()
    {
        return discoverable_feature::FPGA_LOAD_NOTIFICATION;
    }

    std::string get_feature_name() const
    {
        return "FPGA Load Notification";
    }

    virtual ~fpga_load_notification_iface() = default;

    /*! Called after the FPGA has finished loading.
     */
    virtual void onload() = 0;
};

}} // namespace uhd::features
