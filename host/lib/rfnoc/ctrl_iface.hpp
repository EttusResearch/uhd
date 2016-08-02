//
// Copyright 2012-2016 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_RFNOC_CTRL_IFACE_HPP
#define INCLUDED_LIBUHD_RFNOC_CTRL_IFACE_HPP

#include <uhd/utils/msg_task.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/wb_iface.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <string>

namespace uhd { namespace rfnoc {

/*!
 * Provide access to peek, poke for the radio ctrl module
 */
class ctrl_iface : public uhd::timed_wb_iface
{
public:
    typedef boost::shared_ptr<ctrl_iface> sptr;

    virtual ~ctrl_iface(void) = 0;

    //! Make a new control object
    static sptr make(
        const bool big_endian,
        uhd::transport::zero_copy_if::sptr ctrl_xport,
        uhd::transport::zero_copy_if::sptr resp_xport,
        const boost::uint32_t sid,
        const std::string &name = "0"
    );

    //! Hold a ref to a task thats feeding push response
    virtual void hold_task(uhd::msg_task::sptr task) = 0;

    //! Push a response externall (resp_xport is NULL)
    virtual void push_response(const boost::uint32_t *buff) = 0;

    //! Set the command time that will activate
    virtual void set_time(const uhd::time_spec_t &time) = 0;

    //! Get the command time that will activate
    virtual uhd::time_spec_t get_time(void) = 0;

    //! Set the tick rate (converting time into ticks)
    virtual void set_tick_rate(const double rate) = 0;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_CTRL_IFACE_HPP */
