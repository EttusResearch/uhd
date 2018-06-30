//
// Copyright 2010-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_CLOCK_CTRL_HPP
#define INCLUDED_CLOCK_CTRL_HPP

#include "usrp2_iface.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <vector>

class usrp2_clock_ctrl : boost::noncopyable{
public:
    typedef boost::shared_ptr<usrp2_clock_ctrl> sptr;

    virtual ~usrp2_clock_ctrl(void) = 0;

    /*!
     * Make a clock config for the ad9510 ic.
     * \param iface a pointer to the usrp2 interface object
     * \param spiface the interface to spi
     * \return a new clock control object
     */
    static sptr make(usrp2_iface::sptr iface, uhd::spi_iface::sptr spiface);

    /*!
     * Get the master clock frequency for the fpga.
     * \return the clock frequency in Hz
     */
    virtual double get_master_clock_rate(void) = 0;

    /*!
     * Enable/disable the rx dboard clock.
     * \param enb true to enable
     */
    virtual void enable_rx_dboard_clock(bool enb) = 0;

    /*!
     * Set the clock rate on the rx dboard clock.
     * \param rate the new clock rate
     * \throw exception when rate invalid
     */
    virtual void set_rate_rx_dboard_clock(double rate) = 0;

    /*!
     * Get a list of possible rx dboard clock rates.
     * \return a list of clock rates in Hz
     */
    virtual std::vector<double> get_rates_rx_dboard_clock(void) = 0;

    /*!
     * Enable/disable the tx dboard clock.
     * \param enb true to enable
     */
    virtual void enable_tx_dboard_clock(bool enb) = 0;

    /*!
     * Set the clock rate on the tx dboard clock.
     * \param rate the new clock rate
     * \throw exception when rate invalid
     */
    virtual void set_rate_tx_dboard_clock(double rate) = 0;

    /*!
     * Get a list of possible tx dboard clock rates.
     * \return a list of clock rates in Hz
     */
    virtual std::vector<double> get_rates_tx_dboard_clock(void) = 0;

    /*!
     * Enable/disable external reference.
     * \param enb true to enable
     */
    virtual void enable_external_ref(bool enb) = 0;
    
    /*!
     * Enable/disable test clock output.
     * \param enb true to enable
     */
    virtual void enable_test_clock(bool enb) = 0;

    /*!
     * Enable/disable the ref clock output over the serdes cable.
     * \param enb true to enable
     */
    virtual void enable_mimo_clock_out(bool enb) = 0;
    
    /*!
     * Set the output delay of the mimo clock
     * Used to synchronise daisy-chained USRPs over the MIMO cable
     * Can also be used to adjust delay for uneven reference cable lengths
     * \param delay the clock delay in seconds
     */
    virtual void set_mimo_clock_delay(double delay) = 0;

};

#endif /* INCLUDED_CLOCK_CTRL_HPP */
