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

#ifndef INCLUDED_UHD_USRP_MBOARD_IFACE_HPP
#define INCLUDED_UHD_USRP_MBOARD_IFACE_HPP

#include <uhd/types/serial.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/cstdint.hpp>
#include <utility>
#include <string>

namespace uhd{ namespace usrp{

/*!
 * The mboard interface class:
 * Provides a set of functions to implementation layer.
 * Including spi, peek, poke, control...
 */
class mboard_iface : public uhd::i2c_iface, public uhd::spi_iface, public uhd::uart_iface {
public:
    typedef boost::shared_ptr<mboard_iface> sptr;
    /*!
     * Write a register (32 bits)
     * \param addr the address
     * \param data the 32bit data
     */
    virtual void poke32(boost::uint32_t addr, boost::uint32_t data) = 0;

    /*!
     * Read a register (32 bits)
     * \param addr the address
     * \return the 32bit data
     */
    virtual boost::uint32_t peek32(boost::uint32_t addr) = 0;

    /*!
     * Write a register (16 bits)
     * \param addr the address
     * \param data the 16bit data
     */
    virtual void poke16(boost::uint32_t addr, boost::uint16_t data) = 0;

    /*!
     * Read a register (16 bits)
     * \param addr the address
     * \return the 16bit data
     */
    virtual boost::uint16_t peek16(boost::uint32_t addr) = 0;

};

}}

#endif //INCLUDED_UHD_USRP_DBOARD_IFACE_HPP
