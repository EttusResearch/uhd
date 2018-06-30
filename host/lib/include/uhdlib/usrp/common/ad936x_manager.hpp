//
// Copyright 2015 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_AD9361_MANAGER_HPP
#define INCLUDED_AD9361_MANAGER_HPP

#include <uhd/types/wb_iface.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/direction.hpp>
#include <uhdlib/usrp/common/ad9361_ctrl.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <stdint.h>
#include <functional>

namespace uhd { namespace usrp {

/*! AD936x Manager class
 *
 * This class performs higher (management) tasks on the AD936x.
 * It requires a uhd::usrp::ad9361_ctrl object to do the actual
 * register peeks/pokes etc.
 */
class ad936x_manager
{
public:
    typedef boost::shared_ptr<ad936x_manager> sptr;

    static const double DEFAULT_GAIN;
    static const double DEFAULT_BANDWIDTH;
    static const double DEFAULT_TICK_RATE;
    static const double DEFAULT_FREQ; // Hz
    static const uint32_t DEFAULT_DECIM;
    static const uint32_t DEFAULT_INTERP;
    static const bool DEFAULT_AUTO_DC_OFFSET;
    static const bool DEFAULT_AUTO_IQ_BALANCE;
    static const bool DEFAULT_AGC_ENABLE;

    /*!
     * \param codec_ctrl The actual AD936x control object
     * \param n_frontends Number of frontends (1 or 2)
     */
    static sptr make(
            const ad9361_ctrl::sptr &codec_ctrl,
            const size_t n_frontends
    );

    virtual ~ad936x_manager(void) {};

    /*! Put the AD936x into a default state.
     *
     * Sets gains, LOs, bandwidths, etc. according to the DEFAULT_* constants.
     */
    virtual void init_codec(void) = 0;

    /*! Run a loopback self test.
     *
     * This will write data to the AD936x and read it back again.
     * If this test fails, it generally means the interface is broken,
     * so we assume it passes and throw otherwise. Running this requires
     * a core that we can peek and poke the loopback values into.
     *
     * \param iface An interface to the associated radio control core
     * \param iface The radio control core's address to write the loopback value
     * \param iface The radio control core's readback address to read back the returned value
     *
     * \throws a uhd::runtime_error if the loopback value didn't match.
     */
    virtual void loopback_self_test(
            std::function<void(uint32_t)> poker_functor,
            std::function<uint64_t()> peeker_functor
    ) = 0;

    /*! Determine a tick rate that will work with a given sampling rate
     *  (assuming a DDC/DUC chain is also available elsewhere).
     *
     * Example: If we want to stream with a rate of 5 Msps, then the AD936x
     * must run at an integer multiple of that. Although not strictly necessary,
     * we always try and return a multiple of 2. Let's say we need those 5 Msps
     * on two channels, then a good rate is 20 MHz, which is 4 times the sampling
     * rate (thus we can use 2 halfbands elsewhere).
     * If different rates are used on different channels, this can be particularly
     * useful. The clock rate of the AD936x needs to be a multiple of the least
     * common multiple of all the rates. Example: We want to transmit with 3 Msps
     * and receive with 5 Msps. The LCM of this is 15 Msps, which is used as an
     * argument for this function. A good rate is then 30 MHz, which is twice
     * the LCM.
     *
     * \param lcm_rate Least Common Multiple of all the rates involved.
     * \param num_chans The number of channels used for the stream.
     *
     * \returns a valid tick rate that can be used with the given rate
     * \throws a uhd::value_error if \p lcm_rate exceeds the max tick rate
     */
    virtual double get_auto_tick_rate(
            const double lcm_rate,
            size_t num_chans
    ) = 0;

    /*! Check if a given sampling rate is within the available analog bandwidth.
     *
     * If not, outputs a warning message and returns false.
     */
    virtual bool check_bandwidth(double rate, const std::string dir) = 0;

    /*! Populate the property tree for the device frontend
     */
    virtual void populate_frontend_subtree(
            uhd::property_tree::sptr subtree,
            const std::string &key,
            uhd::direction_t dir
    ) = 0;

}; /* class ad936x_manager */

}} /* namespace uhd::usrp */

#endif /* INCLUDED_AD9361_MANAGER_HPP */
