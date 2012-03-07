//
// Copyright 2012 Ettus Research LLC
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

#ifndef INCLUDED_USRP2_FIFO_CTRL_HPP
#define INCLUDED_USRP2_FIFO_CTRL_HPP

#include <uhd/types/time_spec.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include "wb_iface.hpp"
#include <string>

/*!
 * The usrp2 FIFO control class:
 * Provide high-speed peek/poke interface.
 */
class usrp2_fifo_ctrl : public wb_iface, public uhd::spi_iface{
public:
    typedef boost::shared_ptr<usrp2_fifo_ctrl> sptr;

    //! Make a new FIFO control object
    static sptr make(uhd::transport::zero_copy_if::sptr xport);

    //! Set the command time that will activate
    virtual void set_time(const uhd::time_spec_t &time) = 0;

    //! Set the tick rate (converting time into ticks)
    virtual void set_tick_rate(const double rate) = 0;
};

#endif /* INCLUDED_USRP2_FIFO_CTRL_HPP */
