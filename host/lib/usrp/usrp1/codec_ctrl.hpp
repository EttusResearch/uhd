//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_USRP1_CODEC_CTRL_HPP
#define INCLUDED_USRP1_CODEC_CTRL_HPP

#include <uhd/types/serial.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

/*!
 * The usrp1 codec control:
 * - Init/power down codec.
 * - Read aux adc, write aux dac.
 */
class usrp1_codec_ctrl : boost::noncopyable{
public:
    typedef boost::shared_ptr<usrp1_codec_ctrl> sptr;

    static const uhd::gain_range_t tx_pga_gain_range;
    static const uhd::gain_range_t rx_pga_gain_range;

    virtual ~usrp1_codec_ctrl(void) = 0;

    /*!
     * Make a new clock control object.
     * \param iface the spi iface object
     * \param spi_slave which spi device
     */
    static sptr make(uhd::spi_iface::sptr iface, int spi_slave);

    //! aux adc identifier constants
    enum aux_adc_t{
        AUX_ADC_A2 = 0xA2,
        AUX_ADC_A1 = 0xA1,
        AUX_ADC_B2 = 0xB2,
        AUX_ADC_B1 = 0xB1
    };

    /*!
     * Read an auxiliary adc:
     * The internals remember which aux adc was read last.
     * Therefore, the aux adc switch is only changed as needed.
     * \param which which of the 4 adcs
     * \return a value in volts
     */
    virtual double read_aux_adc(aux_adc_t which) = 0;

    //! aux dac identifier constants
    enum aux_dac_t{
        AUX_DAC_A = 0xA,
        AUX_DAC_B = 0xB,
        AUX_DAC_C = 0xC,
        AUX_DAC_D = 0xD
    };

    /*!
     * Write an auxiliary dac.
     * \param which which of the 4 dacs
     * \param volts the level in in volts
     */
    virtual void write_aux_dac(aux_dac_t which, double volts) = 0;

    //! Set the TX PGA gain
    virtual void set_tx_pga_gain(double gain) = 0;

    //! Get the TX PGA gain
    virtual double get_tx_pga_gain(void) = 0;

    //! Set the RX PGA gain ('A' or 'B')
    virtual void set_rx_pga_gain(double gain, char which) = 0;

    //! Get the RX PGA gain ('A' or 'B')
    virtual double get_rx_pga_gain(char which) = 0;

    //! Set the TX modulator frequency
    virtual void set_duc_freq(double freq, double rate) = 0;

    //! Enable or disable the digital part of the DAC
    virtual void enable_tx_digital(bool enb) = 0;

    //! Enable or disable ADC buffer bypass
    virtual void bypass_adc_buffers(bool bypass) = 0;
};

#endif /* INCLUDED_USRP1_CODEC_CTRL_HPP */
