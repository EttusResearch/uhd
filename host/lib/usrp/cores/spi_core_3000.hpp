//
// Copyright 2013-2014 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_USRP_SPI_CORE_3000_HPP
#define INCLUDED_LIBUHD_USRP_SPI_CORE_3000_HPP

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>

class spi_core_3000 : boost::noncopyable, public uhd::spi_iface
{
public:
    typedef boost::shared_ptr<spi_core_3000> sptr;

    virtual ~spi_core_3000(void) = 0;

    //! makes a new spi core from iface and slave base
    static sptr make(uhd::wb_iface::sptr iface, const size_t base, const size_t readback);

    //! Set the spi clock divider to something usable
    virtual void set_divider(const double div) = 0;
};

#endif /* INCLUDED_LIBUHD_USRP_SPI_CORE_3000_HPP */
