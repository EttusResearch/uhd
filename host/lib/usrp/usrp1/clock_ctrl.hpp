//
// Copyright 2010-2011 Ettus Research LLC
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

#ifndef INCLUDED_USRP1_CLOCK_CTRL_HPP
#define INCLUDED_USRP1_CLOCK_CTRL_HPP

#include "usrp1_iface.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <vector>

/*!
 * The usrp1 clock control:
 * - Setup system clocks.
 * - Disable/enable clock lines.
 */
class usrp1_clock_ctrl : boost::noncopyable{
public:
    typedef boost::shared_ptr<usrp1_clock_ctrl> sptr;

    /*!
     * Make a new clock control object.
     * \param iface the usrp1 iface object
     * \return the clock control object
     */
    static sptr make(usrp1_iface::sptr iface);

    /*!
     * Set the rate of the fpga clock line.
     * Note: does not really set, its all software.
     * \param freq the new clock rate in Hz
     */
    virtual void set_master_clock_freq(double freq) = 0;

    /*!
     * Get the rate of the fpga clock line.
     * \return the fpga clock rate in Hz
     */
    virtual double get_master_clock_freq(void) = 0;

};

#endif /* INCLUDED_USRP1_CLOCK_CTRL_HPP */
