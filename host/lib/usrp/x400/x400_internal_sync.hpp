//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/features/internal_sync_iface.hpp>
#include <uhdlib/usrp/dboard/fbx/fbx_ctrl.hpp>


namespace uhd { namespace features {

/*!
 * This class should serve as starting point for using the internal sync. Currently it
 * only supports switching on and off the 5 MHz Sync Clock. So it has to be extended once
 * more functionality should be required.
 */
class internal_sync : public internal_sync_iface
{
public:
    internal_sync(uhd::usrp::fbx::fbx_ctrl::sptr fbx_ctrl);

    /*
     * Enables the sync clock for this daughterboard.
     */
    void enable_sync_clk() override;


    /*
     * Disables the sync clock for this daughterboard.
     */
    void disable_sync_clk() override;

private:
    uhd::usrp::fbx::fbx_ctrl::sptr _fbx_ctrl;
};

}} // namespace uhd::features
