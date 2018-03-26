//
// Copyright 2010-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_CODEC_CTRL_HPP
#define INCLUDED_CODEC_CTRL_HPP

#include "usrp2_iface.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

class usrp2_codec_ctrl : boost::noncopyable{
public:
    typedef boost::shared_ptr<usrp2_codec_ctrl> sptr;

    virtual ~usrp2_codec_ctrl(void) = 0;

    /*!
     * Make a codec control for the DAC and ADC.
     * \param iface a pointer to the usrp2 interface object
     * \param spiface the interface to spi
     * \return a new codec control object
     */
    static sptr make(usrp2_iface::sptr iface, uhd::spi_iface::sptr spiface);

    /*!
     * Set the modulation mode for the DAC.
     * Possible modes are 0, +/-1, +/-2, +/-4, +/-8
     * which correspond to shifts of fs/mod_mode.
     * A mode of 0 or +/-1 means no modulation.
     * \param mod_mode the modulation mode
     */
    virtual void set_tx_mod_mode(int mod_mode) = 0;

    /*!
     * Set the analog preamplifier on the USRP2+ ADC (ADS62P44).
     * \param gain enable or disable the 3.5dB preamp
     */

    virtual void set_rx_analog_gain(bool gain) = 0;

    /*!
     * Set the digital gain on the USRP2+ ADC (ADS62P44).
     * \param gain from 0-6dB
     */

    virtual void set_rx_digital_gain(double gain) = 0;

    /*!
     * Set the digital gain correction on the USRP2+ ADC (ADS62P44).
     * \param gain from 0-0.5dB
     */

    virtual void set_rx_digital_fine_gain(double gain) = 0;

    /*! Return the internal interpolation on the TX side. This is the factor
     *  between the rate of incoming samples and rate at which the DAC is
     *  actually clocked. Values > 1 will enable features, such as internal
     *  modulation.
     */
    virtual size_t get_tx_interpolation() const = 0;

};

#endif /* INCLUDED_CODEC_CTRL_HPP */
