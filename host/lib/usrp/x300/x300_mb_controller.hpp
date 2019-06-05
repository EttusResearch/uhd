//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_X300_MB_CONTROLLER_HPP
#define INCLUDED_LIBUHD_X300_MB_CONTROLLER_HPP

#include "x300_clock_ctrl.hpp"
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/types/wb_iface.hpp>

namespace uhd { namespace rfnoc {

/*! X300-Specific version of the mb_controller
 *
 * Reminder: There is one of these per motherboard.
 */
class x300_mb_controller : public mb_controller
{
public:
    x300_mb_controller(uhd::i2c_iface::sptr zpu_i2c,
        uhd::wb_iface::sptr zpu_ctrl,
        x300_clock_ctrl::sptr clock_ctrl)
        : _zpu_i2c(zpu_i2c), _zpu_ctrl(zpu_ctrl), _clock_ctrl(clock_ctrl)
    {
        // nop
    }

    //! Return reference to the ZPU-owned I2C controller
    uhd::i2c_iface::sptr get_zpu_i2c()
    {
        return _zpu_i2c;
    }

    //! Reference to the ZPU peek/poke interface
    uhd::wb_iface::sptr get_zpu_ctrl() { return _zpu_ctrl; }

    //! Return reference to LMK clock controller
    x300_clock_ctrl::sptr get_clock_ctrl() { return _clock_ctrl; }

    //! X300-specific version of the timekeeper controls
    class x300_timekeeper : public mb_controller::timekeeper
    {
    public:
        x300_timekeeper(uhd::wb_iface::sptr zpu_ctrl) : _zpu_ctrl(zpu_ctrl) {}

        uint64_t get_ticks_now();

        uint64_t get_ticks_last_pps();

        void set_ticks_now(const uint64_t ticks);

        void set_ticks_next_pps(const uint64_t ticks);

        void set_period(const uint64_t period_ns);

    private:
        uhd::wb_iface::sptr _zpu_ctrl;
    };

private:
    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Reference to the ZPU-owned I2C controller
    uhd::i2c_iface::sptr _zpu_i2c;

    //! Reference to the ZPU peek/poke interface
    uhd::wb_iface::sptr _zpu_ctrl;

    //! Reference to LMK clock controller
    x300_clock_ctrl::sptr _clock_ctrl;
};

}} // namespace uhd::rfnoc

#endif /* INCLUDED_LIBUHD_X300_MB_CONTROLLER_HPP */
