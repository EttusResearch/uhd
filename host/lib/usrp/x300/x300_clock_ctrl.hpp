//
// Copyright 2013-2014 Ettus Research LLC
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
    X300_CLOCK_WHICH_TEST,
};

class x300_clock_ctrl : boost::noncopyable
{
public:

    typedef boost::shared_ptr<x300_clock_ctrl> sptr;

    static sptr make(uhd::spi_iface::sptr spiface,
            const size_t slaveno,
            const size_t hw_rev,
            const double master_clock_rate,
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

    /*! Get a list of possible daughterboard clock rates.
     * \return a list of clock rates in Hz
     */
    virtual std::vector<double> get_dboard_rates(const x300_clock_which_t which) = 0;

    /*! Turn the reference output on/off
     * \param true = on, false = off
     */
    virtual void set_ref_out(const bool) = 0;
};

#endif /* INCLUDED_X300_CLOCK_CTRL_HPP */
