//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_CLOCK_CTRL_HPP
#define INCLUDED_X300_CLOCK_CTRL_HPP

#include <uhd/types/serial.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>


enum x300_clock_which_t
{
    X300_CLOCK_WHICH_ADC0,
    X300_CLOCK_WHICH_ADC1,
    X300_CLOCK_WHICH_DAC0,
    X300_CLOCK_WHICH_DAC1,
    X300_CLOCK_WHICH_DB0_RX,
    X300_CLOCK_WHICH_DB0_TX,
    X300_CLOCK_WHICH_DB1_RX,
    X300_CLOCK_WHICH_DB1_TX,
    X300_CLOCK_WHICH_FPGA,
};

class x300_clock_ctrl : boost::noncopyable
{
public:

    typedef boost::shared_ptr<x300_clock_ctrl> sptr;

    virtual ~x300_clock_ctrl(void) = 0;

    static sptr make(uhd::spi_iface::sptr spiface,
            const size_t slaveno,
            const size_t hw_rev,
            const double master_clock_rate,
            const double dboard_clock_rate,
            const double system_ref_rate);

    /*! Get the master clock rate of the device.
     * \return the clock frequency in Hz
     */
    virtual double get_master_clock_rate(void) = 0;

    /*! Get the system reference rate of the device.
     * \return the clock frequency in Hz
     */
    virtual double get_sysref_clock_rate(void) = 0;

    /*! Get the current reference output rate
     * \return the clock frequency in Hz
     */
    virtual double get_refout_clock_rate(void) = 0;

    /*! Set the clock rate on the given daughterboard clock.
     * \param rate the new clock rate
     * \throw exception when rate invalid
     */
    virtual void set_dboard_rate(const x300_clock_which_t which, double rate) = 0;

    /*! Get the clock rate on the given daughterboard clock.
     * \throw exception when rate invalid
     * \return the clock rate in Hz
     */
    virtual double get_dboard_rate(const x300_clock_which_t which) = 0;

    /*! Get a list of possible daughterboard clock rates.
     * \return a list of clock rates in Hz
     */
    virtual std::vector<double> get_dboard_rates(const x300_clock_which_t which) = 0;

    /*! Enable or disable daughterboard clock.
     * \param which which clock
     * \param enable true=enable, false=disable
     * \return a list of clock rates in Hz
     */
    virtual void enable_dboard_clock(const x300_clock_which_t which, const bool enable) = 0;

    /*! Turn the reference output on/off
     * \param true = on, false = off
     */
    virtual void set_ref_out(const bool) = 0;

    /*! Set the clock delay for the given clock divider.
     * \param which which clock
     * \param rate the delay in nanoseconds
     * \param resync resync clocks to apply delays
     * \return the actual delay value set
     * \throw exception when which invalid or delay_ns out of range
     */
    virtual double set_clock_delay(const x300_clock_which_t which, const double delay_ns, const bool resync = true) = 0;

    /*! Get the clock delay for the given clock divider.
     * \param which which clock
     * \return the actual delay value set
     * \throw exception when which invalid
     */
    virtual double get_clock_delay(const x300_clock_which_t which) = 0;

    /*! Reset the clocks.
     *  Should be called if the reference clock changes
     *  to reduce the time required to achieve a lock.
     */
    virtual void reset_clocks(void) = 0;
};

#endif /* INCLUDED_X300_CLOCK_CTRL_HPP */
