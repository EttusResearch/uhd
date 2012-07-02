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

#ifndef INCLUDED_B100_CTRL_HPP
#define INCLUDED_B100_CTRL_HPP

#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/serial.hpp>
#include <boost/shared_ptr.hpp>

class e100_ctrl : public uhd::transport::zero_copy_if{
public:
    typedef boost::shared_ptr<e100_ctrl> sptr;

    //! Make a new controller for E100
    static sptr make(const std::string &node);

    //! Make an i2c iface for the i2c device node
    static uhd::i2c_iface::sptr make_dev_i2c_iface(const std::string &node);

    //! Make a spi iface for the spi gpio
    static uhd::spi_iface::sptr make_aux_spi_iface(void);

    //! Make a uart iface for the uart device node
    static uhd::uart_iface::sptr make_gps_uart_iface(const std::string &node);

    virtual void ioctl(int request, void *mem) = 0;

    virtual int get_file_descriptor(void) = 0;

};

#endif /* INCLUDED_B100_CTRL_HPP */
