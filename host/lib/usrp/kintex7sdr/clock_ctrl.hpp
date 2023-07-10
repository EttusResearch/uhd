//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_KINTEX7SDR_CLOCK_CTRL_HPP
#define INCLUDED_KINTEX7SDR_CLOCK_CTRL_HPP

#include <usrp2/clock_ctrl.hpp>
#include "kintex7sdr_iface.hpp"

class kintex7sdr_clock_ctrl : public usrp2_clock_ctrl{
public:
    typedef std::shared_ptr<kintex7sdr_clock_ctrl> sptr;
    /*!
     * Make a clock config for the ad9510 ic.
     * \param iface a pointer to the usrp2 interface object
     * \param spiface the interface to spi
     * \return a new clock control object
     */
    static sptr make(kintex7sdr_iface::sptr iface, uhd::spi_iface::sptr spiface);
};

#endif /* INCLUDED_KINTEX7SDR_CLOCK_CTRL_HPP */