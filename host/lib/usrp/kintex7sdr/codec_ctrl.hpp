//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_KINTEX7SDR_CODEC_CTRL_HPP
#define INCLUDED_KINTEX7SDR_CODEC_CTRL_HPP

#include <usrp2/codec_ctrl.hpp>
#include "kintex7sdr_iface.hpp"

class kintex7sdr_codec_ctrl : public usrp2_codec_ctrl{
public:
    typedef std::shared_ptr<usrp2_codec_ctrl> sptr;
    /*!
     * Make a codec control for the DAC and ADC.
     * \param iface a pointer to the usrp2 interface object
     * \param spiface the interface to spi
     * \return a new codec control object
     */
    static sptr make(kintex7sdr_iface::sptr iface, uhd::spi_iface::sptr spiface);
};

#endif /* INCLUDED_KINTEX7SDR_CODEC_CTRL_HPP */