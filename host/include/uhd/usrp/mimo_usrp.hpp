//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_UHD_USRP_MIMO_USRP_HPP
#define INCLUDED_UHD_USRP_MIMO_USRP_HPP

#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/tune_result.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <vector>

namespace uhd{ namespace usrp{

/*!
 * The MIMO USRP device class:
 * A mimo usrp facilitates ease-of-use for multi-usrp scenarios.
 * The wrapper provides convenience functions to control the group
 * of underlying devices as if they consisted of a single device.
 */
class UHD_API mimo_usrp : boost::noncopyable{
public:
    typedef boost::shared_ptr<mimo_usrp> sptr;

    /*!
     * Make a new mimo usrp from the device address.
     * \param dev_addr the device address
     * \return a new mimo usrp object
     */
    static sptr make(const device_addr_t &dev_addr);

    /*!
     * Get the underlying device object.
     * This is needed to get access to the streaming API and properties.
     * \return the device object within this simple usrp
     */
    virtual device::sptr get_device(void) = 0;

    /*!
     * Get a printable name for this mimo usrp.
     * \return a printable string
     */
    virtual std::string get_pp_string(void) = 0;

    /*!
     * Get the number of channels in this mimo configuration.
     * The number of rx channels == the number of tx channels.
     * \return the number of channels
     */
    virtual size_t get_num_channels(void) = 0;

    /*******************************************************************
     * Misc
     ******************************************************************/
    /*!
     * Set the time registers on the usrp at the next pps tick.
     * The values will not be latched in until the pulse occurs.
     * It is recommended that the user sleep(1) after calling to ensure
     * that the time registers will be in a known state prior to use.
     *
     * Note: Because this call sets the time on the "next" pps,
     * the seconds in the time spec should be current seconds + 1.
     *
     * \param time_spec the time to latch into the usrp device
     */
    virtual void set_time_next_pps(const time_spec_t &time_spec) = 0;

    /*!
     * Issue a stream command to the usrp device.
     * This tells the usrp to send samples into the host.
     * See the documentation for stream_cmd_t for more info.
     * \param stream_cmd the stream command to issue
     */
    virtual void issue_stream_cmd(const stream_cmd_t &stream_cmd) = 0;

    /*******************************************************************
     * RX methods
     ******************************************************************/

    /*******************************************************************
     * TX methods
     ******************************************************************/

};

}}

#endif /* INCLUDED_UHD_USRP_MIMO_USRP_HPP */
