//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_KINTEX7SDR_IFACE_HPP
#define INCLUDED_KINTEX7SDR_IFACE_HPP

#include "kintex7sdr_regs.hpp"
#include <usrp2/usrp2_iface.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <boost/utility.hpp>
#include <functional>
#include <memory>
#include <string>

/*!
 * The kintex7sdr interface class:
 * Provides a set of functions to implementation layer.
 * Including spi, peek, poke, control...
 */
class kintex7sdr_iface : public usrp2_iface {
public:
    //! The list of possible revision types
    enum rev_type : std::underlying_type< usrp2_iface::rev_type >::type{
        USRP_N210_XK = 221,
        USRP_N210_XA = 220,
        USRP_NXXX    = 0
    };
};

#endif /* INCLUDED_KINTEX7SDR_IFACE_HPP */
