//
// Copyright 2011 Ettus Research LLC
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

#ifndef INCLUDED_B100_IFACE_HPP
#define INCLUDED_B100_IFACE_HPP

#include <uhd/usrp/mboard_iface.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <uhd/transport/usb_zero_copy.hpp>
#include "../fx2/fx2_ctrl.hpp"
#include "b100_ctrl.hpp"

/*!
 * The usrp1 interface class:
 * Provides a set of functions to implementation layer.
 * Including spi, peek, poke, control...
 */
class b100_iface : boost::noncopyable, public uhd::usrp::mboard_iface{
public:
    typedef boost::shared_ptr<b100_iface> sptr;

    /*!
     * Make a new b100 interface with the control transport.
     * \param fx2_ctrl the usrp control object
     * \param fpga_ctrl the FPGA interface control object
     * \return a new usrp1 interface object
     */
    static sptr make(uhd::usrp::fx2_ctrl::sptr fx2_ctrl,
                     b100_ctrl::sptr fpga_ctrl = b100_ctrl::sptr()
    );

    //! TODO implement this for multiple hardwares revs in the future
    std::string get_cname(void){
        return "USRP-B100";
    }

    /*!
     * Reset the GPIF interface on the FX2
     * \param which endpoint to reset
     * \return
     */
    virtual void reset_gpif(boost::uint16_t ep) = 0;

    /*!
     * Clear the GPIF FIFOs on the FPGA
     * \return
     */
    virtual void clear_fpga_fifo(void) = 0;

    /*!
     * Enable/disable the GPIF interfaces on the FX2
     * \return
     */
    virtual void enable_gpif(bool en) = 0;

    //! Get access to the FX2 I2C interface
    virtual uhd::i2c_iface &get_fx2_i2c_iface(void) = 0;

    uhd::usrp::mboard_eeprom_t mb_eeprom;
};

#endif /* INCLUDED_USRP1_IFACE_HPP */
