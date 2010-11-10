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

#ifndef INCLUDED_USRP2_IFACE_HPP
#define INCLUDED_USRP2_IFACE_HPP

#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/cstdint.hpp>
#include "mboard_rev.hpp"
#include <utility>
#include "fw_common.h"
#include "usrp2_regs.hpp"

/*!
 * The usrp2 interface class:
 * Provides a set of functions to implementation layer.
 * Including spi, peek, poke, control...
 */
class usrp2_iface : public uhd::i2c_iface, boost::noncopyable{
public:
    typedef boost::shared_ptr<usrp2_iface> sptr;
    typedef std::pair<boost::uint32_t, boost::uint32_t> pair64;

    /*!
     * Make a new usrp2 interface with the control transport.
     * \param ctrl_transport the udp transport object
     * \return a new usrp2 interface object
     */
    static sptr make(uhd::transport::udp_simple::sptr ctrl_transport);

    /*!
     * Perform a control transaction.
     * \param data a control data struct
     * \return the result control data
     */
    virtual usrp2_ctrl_data_t ctrl_send_and_recv(const usrp2_ctrl_data_t &data) = 0;

    /*!
     * Read a dual register (64 bits)
     * \param addrlo the address for the low-32 bits
     * \param addrhi the address for the high-32 bits
     * \return a pair of 32 bit integers lo, hi
     */
    virtual pair64 peek64(boost::uint32_t addrlo, boost::uint32_t addrhi) = 0;

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
        const uhd::spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits,
        bool readback
    ) = 0;

    virtual void write_uart(boost::uint8_t dev, const std::string &buf) = 0;

    virtual std::string read_uart(boost::uint8_t dev) = 0;

    /*!
     * Set the hardware revision number. Also selects the proper register set for the device.
     * \param rev the 16-bit revision
     */
    virtual void set_hw_rev(mboard_rev_t rev) = 0;

    /*! Return the hardware revision number
     * \return hardware revision
     */
    virtual mboard_rev_t get_hw_rev(void) = 0;

    /*!
     * Register map selected from USRP2/USRP2+.
     */
    usrp2_regs_t regs;
    /*!
     * Hardware revision as returned by the device.
     */
    mboard_rev_t hw_rev;
    //motherboard eeprom map structure
    uhd::usrp::mboard_eeprom_t mb_eeprom;
};

#endif /* INCLUDED_USRP2_IFACE_HPP */
