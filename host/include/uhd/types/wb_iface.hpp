//
// Copyright 2011-2013 Ettus Research LLC
// Copyright 2014 Per Vices Corporation
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

#ifndef INCLUDED_UHD_TYPES_WB_IFACE_HPP
#define INCLUDED_UHD_TYPES_WB_IFACE_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd
{

class UHD_API wb_iface
{
public:
    typedef boost::shared_ptr<wb_iface> sptr;
    typedef boost::uint32_t wb_addr_type;

    virtual ~wb_iface(void);

    /*!
     * Write a register (64 bits)
     * \param addr the address
     * \param data the 64bit data
     */
    virtual void poke64(const wb_addr_type addr, const boost::uint64_t data);

    /*!
     * Read a register (64 bits)
     * \param addr the address
     * \return the 64bit data
     */
    virtual boost::uint64_t peek64(const wb_addr_type addr);

    /*!
     * Write a register (32 bits)
     * \param addr the address
     * \param data the 32bit data
     */
    virtual void poke32(const wb_addr_type addr, const boost::uint32_t data);

    /*!
     * Read a register (32 bits)
     * \param addr the address
     * \return the 32bit data
     */
    virtual boost::uint32_t peek32(const wb_addr_type addr);

    /*!
     * Write a register (16 bits)
     * \param addr the address
     * \param data the 16bit data
     */
    virtual void poke16(const wb_addr_type addr, const boost::uint16_t data);

    /*!
     * Read a register (16 bits)
     * \param addr the address
     * \return the 16bit data
     */
    virtual boost::uint16_t peek16(const wb_addr_type addr);

    /*!
     * Send/write a data packet (string), null terminated
     * \param data the data packet
     * \return the 16bit data
     */
    virtual void poke_str(const std::string data);

    /*!
     * Recieve/read a data packet (string), null terminated
     * \return the string
     */
    virtual std::string peek_str(void);
};

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_WB_IFACE_HPP */
