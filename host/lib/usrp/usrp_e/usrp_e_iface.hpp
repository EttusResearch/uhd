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

#ifndef INCLUDED_USRP_E_IFACE_HPP
#define INCLUDED_USRP_E_IFACE_HPP

#include <uhd/transport/udp_simple.hpp>
#include <uhd/usrp/dboard_iface.hpp> //spi config
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/cstdint.hpp>

/*!
 * The usrp-e interface class:
 * Provides a set of functions to implementation layer.
 * Including spi, peek, poke, control...
 */
class usrp_e_iface : boost::noncopyable{
public:
    typedef boost::shared_ptr<usrp_e_iface> sptr;

    /*!
     * Make a new usrp-e interface with the control transport.
     * \param node_fd the file descriptor for the kernel module node
     * \return a new usrp-e interface object
     */
    static sptr make(int node_fd);

    /*!
     * Perform an ioctl call on the device node file descriptor.
     * This will throw when the internal ioctl call fails.
     * \param request the control word
     * \param mem pointer to some memory
     */
    virtual void ioctl(int request, void *mem) = 0;

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

    /*!
     * Perform an spi transaction.
     * \param which_slave the slave device number
     * \param config spi config args
     * \param data the bits to write
     * \param num_bits how many bits in data
     * \param readback true to readback a value
     * \return spi data if readback set
     */
    virtual boost::uint32_t transact_spi(
        int which_slave,
        const uhd::usrp::spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits,
        bool readback
    ) = 0;
};

#endif /* INCLUDED_USRP_E_IFACE_HPP */
