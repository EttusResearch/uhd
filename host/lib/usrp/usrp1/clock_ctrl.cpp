//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "clock_ctrl.hpp"
#include "fpga_regs_standard.h"
#include <uhd/utils/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <utility>
#include <iostream>

using namespace uhd;

/***********************************************************************
 * Constants
 **********************************************************************/
static const double master_clock_rate = 64e6;

/***********************************************************************
 * Clock Control Implementation
 **********************************************************************/
class usrp1_clock_ctrl_impl : public usrp1_clock_ctrl {
public:
    usrp1_clock_ctrl_impl(usrp1_iface::sptr iface)
    {
        _iface = iface;
    }

    double get_master_clock_freq(void)
    {
        return master_clock_rate; 
    }

    /***********************************************************************
     * RX Dboard Clock Control (output 9, divider 3)
     **********************************************************************/
    void enable_rx_dboard_clock(bool)
    {
        std::cerr << "USRP: enable_rx_dboard_clock() disabled" << std::endl;
        _iface->poke32(FR_RX_A_REFCLK, 0);
        _iface->poke32(FR_RX_B_REFCLK, 0);
    }

    std::vector<double> get_rx_dboard_clock_rates(void)
    {
#if 0 
        std::vector<double> rates;
        for (size_t div = 1; div <= 127; div++)
            rates.push_back(master_clock_rate / div);
        return rates;
#else
        return std::vector<double>(1, master_clock_rate);
#endif
    }

    /*
     * Daughterboard reference clock register
     *
     * Bit  7    - 1 turns on refclk, 0 allows IO use
     * Bits 6:0  - Divider value
     */
    void set_rx_dboard_clock_rate(double)
    {
#if 0
        assert_has(get_rx_dboard_clock_rates(), rate, "rx dboard clock rate");
        size_t divider = size_t(rate/master_clock_rate);
        _iface->poke32(FR_RX_A_REFCLK, (divider & 0x7f) | 0x80);
#else
        std::cerr << "USRP: set_rx_dboard_clock_rate() disabled" << std::endl;
        _iface->poke32(FR_RX_A_REFCLK, 0);
        _iface->poke32(FR_RX_B_REFCLK, 0);
#endif
    }

    /***********************************************************************
     * TX Dboard Clock Control
     **********************************************************************/
    void enable_tx_dboard_clock(bool)
    {
        std::cerr << "USRP: set_tx_dboard_clock() disabled" << std::endl;
        _iface->poke32(FR_TX_A_REFCLK, 0);
        _iface->poke32(FR_TX_B_REFCLK, 0);

    }

    std::vector<double> get_tx_dboard_clock_rates(void)
    {
        return get_rx_dboard_clock_rates(); //same master clock, same dividers...
    }

    void set_tx_dboard_clock_rate(double)
    {
        std::cerr << "USRP: set_tx_dboard_clock_rate() disabled" << std::endl;
        _iface->poke32(FR_TX_A_REFCLK, 0);
        _iface->poke32(FR_TX_B_REFCLK, 0);
    }

private:
    usrp1_iface::sptr _iface;

};

/***********************************************************************
 * Clock Control Make
 **********************************************************************/
usrp1_clock_ctrl::sptr usrp1_clock_ctrl::make(usrp1_iface::sptr iface)
{
    return sptr(new usrp1_clock_ctrl_impl(iface));
}
