//
// Copyright 2011-2012 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_USRP_USRP1_SOFT_TIME_CTRL_HPP
#define INCLUDED_LIBUHD_USRP_USRP1_SOFT_TIME_CTRL_HPP

#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

namespace uhd{ namespace usrp{

/*!
 * The soft time control emulates some of the
 * advanced streaming capabilities of the later USRP models.
 * Soft time control uses the system time to emulate
 * timed transmits, timed receive commands, device time,
 * and inline and async error messages.
 */
class soft_time_ctrl : boost::noncopyable{
public:
    typedef boost::shared_ptr<soft_time_ctrl> sptr;
    typedef boost::function<void(bool)> cb_fcn_type;

    /*!
     * Make a new soft time control.
     * \param stream_on_off a function to enable/disable rx
     * \return a new soft time control object
     */
    static sptr make(const cb_fcn_type &stream_on_off);

    //! Set the current time
    virtual void set_time(const time_spec_t &time) = 0;

    //! Get the current time
    virtual time_spec_t get_time(void) = 0;

    //! Call after the internal recv function
    virtual size_t recv_post(rx_metadata_t &md, const size_t nsamps) = 0;

    //! Call before the internal send function
    virtual void send_pre(const tx_metadata_t &md, double &timeout) = 0;

    //! Issue a stream command to receive
    virtual void issue_stream_cmd(const stream_cmd_t &cmd) = 0;

    //! Get access to a buffer of async metadata
    virtual transport::bounded_buffer<async_metadata_t> &get_async_queue(void) = 0;

    //! Get access to a buffer of inline metadata
    virtual transport::bounded_buffer<rx_metadata_t> &get_inline_queue(void) = 0;

    //! Stops threads before deconstruction to avoid race conditions
    virtual void stop(void) = 0;
};

}} //namespace

#endif /* INCLUDED_LIBUHD_USRP_USRP1_SOFT_TIME_CTRL_HPP */
