//
// Copyright 2011,2014,2015 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_USRP_GPIO_CORE_3000_HPP
#define INCLUDED_LIBUHD_USRP_GPIO_CORE_3000_HPP

#include <uhd/config.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/usrp/gpio_defs.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>

namespace uhd { namespace usrp { namespace gpio_atr {

class gpio_atr_3000 : boost::noncopyable {
public:
    typedef boost::shared_ptr<gpio_atr_3000> sptr;

    static const boost::uint32_t MASK_SET_ALL = 0xFFFFFFFF;

    virtual ~gpio_atr_3000(void) {};

    /*!
     * Create a read-write GPIO ATR interface object
     *
     * \param iface register iface to GPIO ATR registers
     * \param base base settings offset for GPIO ATR registers
     * \param base readback offset for GPIO ATR registers
     */
    static sptr make(
        uhd::wb_iface::sptr iface,
        const uhd::wb_iface::wb_addr_type base,
        const uhd::wb_iface::wb_addr_type rb_addr);

    /*!
     * Create a write-only GPIO ATR interface object
     *
     * \param iface register iface to GPIO ATR registers
     * \param base base settings offset for GPIO ATR registers
     */
    static sptr make_write_only(
        uhd::wb_iface::sptr iface, const uhd::wb_iface::wb_addr_type base);

    /*!
     * Select the ATR mode for all bits in the mask
     *
     * \param mode the mode to apply {ATR = outputs driven by ATR state machine, GPIO = outputs static}
     * \param mask apply the mode to all non-zero bits in the mask
     */
    virtual void set_atr_mode(const gpio_atr_mode_t mode, const boost::uint32_t mask) = 0;

    /*!
     * Select the data direction for all bits in the mask
     *
     * \param dir the direction {OUTPUT, INPUT}
     * \param mask apply the mode to all non-zero bits in the mask
     */
    virtual void set_gpio_ddr(const gpio_ddr_t dir, const boost::uint32_t mask) = 0;

    /*!
     * Write the specified (masked) value to the ATR register
     *
     * \param atr the type of ATR register to write to {IDLE, RX, TX, FDX}
     * \param value the value to write
     * \param mask only writes to the bits where mask is non-zero
     */
    virtual void set_atr_reg(const gpio_atr_reg_t atr, const boost::uint32_t value, const boost::uint32_t mask = MASK_SET_ALL) = 0;

    /*!
     * Write to a static GPIO output
     *
     * \param value the value to write
     * \param mask only writes to the bits where mask is non-zero
     */
    virtual void set_gpio_out(const boost::uint32_t value, const boost::uint32_t mask = MASK_SET_ALL) = 0;

    /*!
     * Read the state of the GPIO pins
     * If a pin is configured as an input, reads the actual value of the pin
     * If a pin is configured as an output, reads the last value written to the pin
     *
     * \return the value read back
     */
    virtual boost::uint32_t read_gpio() = 0;

    /*!
     * Set a GPIO attribute
     *
     * \param attr the attribute to set
     * \param value the value to write to the attribute
     */
    virtual void set_gpio_attr(const gpio_attr_t attr, const boost::uint32_t value) = 0;
};

class db_gpio_atr_3000 {
public:
    typedef boost::shared_ptr<db_gpio_atr_3000> sptr;

    typedef uhd::usrp::dboard_iface::unit_t db_unit_t;

    virtual ~db_gpio_atr_3000(void) {};

    /*!
     * Create a read-write GPIO ATR interface object for a daughterboard connector
     *
     * \param iface register iface to GPIO ATR registers
     * \param base base settings offset for GPIO ATR registers
     * \param base readback offset for GPIO ATR registers
     */
    static sptr make(
        uhd::wb_iface::sptr iface,
        const uhd::wb_iface::wb_addr_type base,
        const uhd::wb_iface::wb_addr_type rb_addr);

    /*!
     * Configure the GPIO mode for all pins in the daughterboard connector
     *
     * \param unit the side of the daughterboard interface to configure (TX or RX)
     * \param value if value[i] is 1, the i'th bit is in ATR mode otherwise it is in GPIO mode
     */
    virtual void set_pin_ctrl(const db_unit_t unit, const boost::uint32_t value, const boost::uint32_t mask) = 0;

    virtual boost::uint32_t get_pin_ctrl(const db_unit_t unit) = 0;

    /*!
     * Configure the direction for all pins in the daughterboard connector
     *
     * \param unit the side of the daughterboard interface to configure (TX or RX)
     * \param value if value[i] is 1, the i'th bit is an output otherwise it is an input
     */
    virtual void set_gpio_ddr(const db_unit_t unit, const boost::uint32_t value, const boost::uint32_t mask) = 0;

    virtual boost::uint32_t get_gpio_ddr(const db_unit_t unit) = 0;

    /*!
     * Write the specified value to the ATR register (all bits)
     *
     * \param atr the type of ATR register to write to {IDLE, RX, TX, FDX}
     * \param unit the side of the daughterboard interface to configure (TX or RX)
     * \param value the value to write
     */
    virtual void set_atr_reg(const db_unit_t unit, const gpio_atr_reg_t atr, const boost::uint32_t value, const boost::uint32_t mask) = 0;

    virtual boost::uint32_t get_atr_reg(const db_unit_t unit, const gpio_atr_reg_t atr) = 0;

    /*!
     * Write the specified value to the GPIO register (all bits)
     *
     * \param atr the type of ATR register to write to {IDLE, RX, TX, FDX}
     * \param value the value to write
     */
    virtual void set_gpio_out(const db_unit_t unit, const boost::uint32_t value, const boost::uint32_t mask) = 0;

    virtual boost::uint32_t get_gpio_out(const db_unit_t unit) = 0;

    /*!
     * Read the state of the GPIO pins
     * If a pin is configured as an input, reads the actual value of the pin
     * If a pin is configured as an output, reads the last value written to the pin
     *
     * \param unit the side of the daughterboard interface to configure (TX or RX)
     * \return the value read back
     */
    virtual boost::uint32_t read_gpio(const db_unit_t unit) = 0;
};

}}} //namespaces

#endif /* INCLUDED_LIBUHD_USRP_GPIO_CORE_3000_HPP */
