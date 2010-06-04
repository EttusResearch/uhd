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

#ifndef INCLUDED_USRP_E_CLOCK_CTRL_HPP
#define INCLUDED_USRP_E_CLOCK_CTRL_HPP

#include "usrp_e_iface.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

/*!
 * The usrp-e clock control:
 * - Setup system clocks.
 * - Disable/enable clock lines.
 */
class usrp_e_clock_ctrl : boost::noncopyable{
public:
    typedef boost::shared_ptr<usrp_e_clock_ctrl> sptr;

    /*!
     * Make a new clock control object.
     * \param iface the usrp_e iface object
     * \return the clock control object
     */
    static sptr make(usrp_e_iface::sptr iface);

    /*!
     * Get the rate of the fpga clock line.
     * \return the fpga clock rate in Hz
     */
    virtual double get_fpga_clock_rate(void) = 0;

    /*!
     * Get the rate of the dboard clock clock line.
     * \return the dboard clock rate in Hz
     */
    virtual double get_rx_dboard_clock_rate(void) = 0;

    /*!
     * Get the rate of the dboard clock clock line.
     * \return the dboard clock rate in Hz
     */
    virtual double get_tx_dboard_clock_rate(void) = 0;

    /*!
     * Enable/disable the rx dboard clock.
     * \param enb true to enable
     */
    virtual void enable_rx_dboard_clock(bool enb) = 0;

    /*!
     * Enable/disable the tx dboard clock.
     * \param enb true to enable
     */
    virtual void enable_tx_dboard_clock(bool enb) = 0;

};

#endif /* INCLUDED_USRP_E_CLOCK_CTRL_HPP */
