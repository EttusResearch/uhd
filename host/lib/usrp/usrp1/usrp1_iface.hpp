//
// Copyright 2010-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_USRP1_IFACE_HPP
#define INCLUDED_USRP1_IFACE_HPP

#include <uhdlib/usrp/common/fx2_ctrl.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/types/serial.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#define SPI_ENABLE_FPGA  0x01
#define SPI_FMT_HDR_MASK (3 << 5)
#define SPI_FMT_HDR_0    (0 << 5)
#define SPI_FMT_HDR_1    (1 << 5)
#define SPI_FMT_HDR_2    (2 << 5)
#define SPI_FMT_LSB      (1 << 7)
#define SPI_FMT_MSB      (0 << 7)
#define SPI_FMT_xSB_MASK (1 << 7)
#define VRQ_SPI_READ     0x82
#define VRQ_SPI_WRITE    0x09
#define VRQ_FW_COMPAT    0x83


/*!
 * The usrp1 interface class:
 * Provides a set of functions to implementation layer.
 * Including spi, peek, poke, control...
 */
class usrp1_iface : public uhd::wb_iface, public uhd::i2c_iface, public uhd::spi_iface, boost::noncopyable
{
public:
    typedef boost::shared_ptr<usrp1_iface> sptr;

    /*!
     * Make a new usrp1 interface with the control transport.
     * \param ctrl_transport the usrp controller object
     * \return a new usrp1 interface object
     */
    static sptr make(uhd::usrp::fx2_ctrl::sptr ctrl_transport);
};

#endif /* INCLUDED_USRP1_IFACE_HPP */
